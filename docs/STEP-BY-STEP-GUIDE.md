# NMC Firmware DevOps — Step-by-Step Practice Guide

**Scenario.** You are the DevOps engineer on a team building the firmware for the Network Management
Card (NMC) of a 3-phase UPS family. The developers own the features: SNMP, Modbus TCP, TCP/IP, PCNS,
HTTP/REST and security. You own everything that turns their commits into **trusted, signed firmware for
four card variants**: GVS, GVL, GVXL and GVXL-NAM. That covers Jenkins, Docker, Kubernetes, security
scanning and AWS.

Work through the phases in order. Each step gives you three things:

- **Do** — the exact commands to run.
- **Expect** — what you should see if it worked.
- **Why** — why a real company does it this way. This is what you will explain in interviews.

| Phase | Topic | Cost |
|---|---|---|
| 0 | Set up your machine and Git repo | free |
| 1 | Build and run the firmware locally | free |
| 2 | Git workflow like the team does it | free |
| 3 | Docker: toolchain, cross-compile, simulators | free |
| 4 | Jenkins locally | free |
| 5 | Security scans hands-on | free |
| 6 | Kubernetes locally with kind | free |
| 7 | AWS foundation with Terraform | ~$8–12/day while running |
| 8 | Jenkins on EKS — the full corporate pipeline | included above |
| 9 | Signed release with approvals | included above |
| 10 | Operate: nightly runs, incidents, HIL, teardown | — |

---

## Phase 0 — Set up your machine

### 0.1 Install the tools
Use Ubuntu 22.04/24.04 or Windows with WSL2 (Ubuntu).

```bash
sudo apt update && sudo apt install -y build-essential cmake ninja-build git python3 python3-pip \
  python3-pytest snmp jq curl unzip clang clang-tidy cppcheck gcovr
# Docker Engine: https://docs.docker.com/engine/install/ubuntu/  then:
sudo usermod -aG docker $USER && newgrp docker
# kubectl, helm, kind, terraform, aws cli v2, trivy, gitleaks, syft, grype, pre-commit:
pip install --user pre-commit
```

Install links: kubectl (kubernetes.io/docs/tasks/tools), Helm (helm.sh/docs/intro/install), kind
(kind.sigs.k8s.io), Terraform ≥ 1.6 (developer.hashicorp.com/terraform/install), AWS CLI v2, Trivy
(aquasecurity.github.io/trivy), Gitleaks, Syft and Grype (github.com/anchore).

**Expect.** Each of these prints a version:
`gcc --version`, `cmake --version`, `docker run hello-world`, `kubectl version --client`,
`helm version`, `terraform version`, `aws --version`.

### 0.2 Put the project in your own Git repository
```bash
unzip nmc-firmware-devops.zip && cd nmc-firmware-devops
git init -b main && git add . && git commit -m "chore: initial import of NMC firmware project"
# Create an EMPTY repo on GitHub named nmc-firmware-devops, then:
git remote add origin git@github.com:<you>/nmc-firmware-devops.git
git push -u origin main
```
Replace `your-org` with your GitHub user in these files: `CODEOWNERS`, `jenkins/casc/jenkins.yaml`,
`jenkins/job-dsl/seed-jobs.groovy`. Find them with `grep -rn your-org .`.

### 0.3 Protect `main`
On GitHub go to **Settings → Branches → Add rule** for `main` and enable:
- Require a pull request (1 approval).
- Require review from Code Owners.
- Require status checks to pass. You'll add the Jenkins check in Phase 4.
- Block force pushes.

**Why.** Firmware ships to customers' data centers. Nobody, including you, pushes straight to `main`.

---

## Phase 1 — Build and run the firmware locally

Read `firmware/` for 15 minutes first. You need to know what you're building:

| Folder | What it is |
|---|---|
| `src/snmp` | SNMP agent: BER codec, MIB-II and RFC 1628 UPS-MIB |
| `src/modbus` | Modbus TCP server and register map (see `mibs/modbus-register-map.csv`) |
| `src/net` | TCP/IP helpers: dual-stack listen, connect with timeout |
| `src/pcns` | Sends shutdown events to PowerChute clients |
| `src/http` | REST API with authentication and security headers |
| `src/security` | SHA-256, login with lockout, audit log |
| `src/ota` | Firmware image validation (variant lock + SHA-256) |
| `variants/*.cmake` | What makes GVS different from GVXL-NAM |

### 1.1 Build one variant and run the unit tests
```bash
scripts/build.sh GVS
```
**Expect.** CMake prints `NMC GVS v1.4.0-rc.0+<sha> target=armhf`. On `main` the version is `-rc`; on a feature branch it is `-dev` (see `scripts/version.sh`). Then you see
`100% tests passed, 0 tests failed out of 7`, and `file` reports an x86-64 ELF.

**Why.** The developers write the unit tests. Your job is to make sure they run on every commit, for
every variant.

### 1.2 Build all four cards
```bash
make all-variants
for v in GVS GVL GVXL GVXL-NAM; do build/$v-host/nmc-fw --version; done
```
**Expect.** The feature lists differ by variant. GVXL adds `parallel-ups`, and GVXL-NAM adds
`fips-mode`.

### 1.3 Run a card and talk to it with each protocol
Terminal 1:
```bash
NMC_SIMULATION=1 NMC_ADMIN_PASS='Test#Pass1' NMC_SNMP_COMMUNITY=lab-ro \
NMC_PCNS_CLIENTS=127.0.0.1:3052 build/GVS-host/nmc-fw
```
Terminal 2:
```bash
# HTTP / REST
curl -s localhost:8080/api/version | jq
curl -s localhost:8080/api/status | jq
curl -i localhost:8080/api/config                                  # 401: needs auth
curl -s -u 'admin:Test#Pass1' localhost:8080/api/config | jq       # works, no secrets returned

# SNMP (MIB-II + UPS-MIB)
snmpget  -v2c -c lab-ro localhost:1161 1.3.6.1.2.1.1.1.0
snmpwalk -v2c -c lab-ro localhost:1161 1.3.6.1.2.1.33
snmpget  -v2c -c public localhost:1161 1.3.6.1.2.1.1.1.0           # times out: wrong community

# Modbus TCP: read 20 holding registers
python3 - <<'PY'
import socket, struct
s = socket.create_connection(("127.0.0.1", 1502))
s.send(struct.pack(">HHHBBHH", 1, 0, 6, 1, 3, 0, 20))
r = s.recv(256); print(struct.unpack(">20H", r[9:49]))
PY

# PCNS: listen like a PowerChute server, then simulate a mains failure
nc -l 3052 &                    # on some systems: nc -l -p 3052
curl -s -u 'admin:Test#Pass1' -X POST localhost:8080/api/simulate/onbattery
curl -s -u 'admin:Test#Pass1' -X POST localhost:8080/api/simulate/online
```
**Expect.**
- `nc` prints `{"event":"ON_BATTERY",...,"action":"PREPARE_SHUTDOWN"}`.
- Terminal 1 shows audit lines.
- The first register is `2` (online), and the last register is the model ID.

### 1.4 Run the integration test suite
With the card from 1.3 still running (stop the `nc` listener first):
```bash
cd firmware/tests/integration && NMC_VARIANT=GVS pytest
```
**Expect.** 18 tests pass. They cover HTTP, SNMP, Modbus, PCNS and the security requirements.
Later, this same suite runs in Docker, in Kubernetes and against real cards on the HIL rack.

### 1.5 Coverage
```bash
scripts/build.sh GVS --coverage && xdg-open reports/coverage-GVS.html
```
**Why.** The pipeline fails if line coverage drops below 70 %. You just ran the same gate locally.

### 1.6 Package a firmware image
```bash
scripts/build.sh GVS --cross       # needs the ARM cross-compiler (Phase 3); try it there if it fails here
scripts/package.sh GVS
build/GVS-host/nmc-fw --validate dist/nmc-GVS-*.nmcfw     # ok
build/GVXL-host/nmc-fw --validate dist/nmc-GVS-*.nmcfw    # variant mismatch -> exit 2
```
**Why.** A GVS image must never flash onto a GVXL card. The card checks the variant and SHA-256 in
the header before it accepts an image.

---

## Phase 2 — Git workflow like the team does it

### 2.1 Install the hooks
```bash
pre-commit install && pre-commit install --hook-type commit-msg
pre-commit run --all-files
```

### 2.2 Make a feature branch and add a small feature
Example: expose battery temperature over SNMP. It is already in the MIB, so practise with a new
REST field or a log line instead.
```bash
git switch -c feature/NMC-101-uptime-in-version
# edit firmware/src/http/web_server.c, then:
make build
git commit -am "update stuff"                          # REJECTED by commit-msg hook
git commit -am "feat(http): report uptime in /api/version"
git push -u origin feature/NMC-101-uptime-in-version
```
Open a PR. CODEOWNERS automatically requests reviewers. Changes under `src/security` also pull in
the security team.

### 2.3 Practise the history-recovery tools
```bash
git log --oneline --graph
git revert <sha>                         # undo a merged commit safely
git bisect start HEAD v1.3.0 && git bisect run scripts/build.sh GVS   # find the commit that broke tests
git reflog                               # recover a branch you deleted by mistake
```

### 2.4 Cut a release branch and a hotfix (dry run)
```bash
git switch -c release/1.4 && git push -u origin release/1.4
git switch main && echo 1.5.0 > VERSION && git commit -am "chore: start 1.5 development"
BRANCH_NAME=release/1.4 BUILD_NUMBER=7 scripts/version.sh   # -> 1.4.0-rc.7+<sha>
```
Read `docs/branching-and-versioning.md` and `scripts/commit-lint.sh`.

---

## Phase 3 — Docker: toolchain, cross-compile, simulators

### 3.1 Build the toolchain image
```bash
make toolchain                       # docker build -t nmc-toolchain:local docker/toolchain
docker run --rm nmc-toolchain:local arm-linux-gnueabihf-gcc --version
docker run --rm nmc-toolchain:local aarch64-linux-gnu-gcc --version
```
**Why.** Every build runs inside this image: your laptop, every Jenkins agent and the release
pipeline. "Works on my machine" disappears, and the release pipeline can prove a build is
reproducible.

### 3.2 Cross-compile all variants inside the container
```bash
make shell
# inside the container:
for v in GVS GVL GVXL GVXL-NAM; do scripts/build.sh $v --cross && scripts/package.sh $v; done
file build/GVS-armhf/nmc-fw build/GVXL-aarch64/nmc-fw
security/checksec.sh build/*-arm*/nmc-fw build/*-aarch64/nmc-fw
exit
```
**Expect.** `ARM, EABI5` for GVS and `ARM aarch64` for the others. All checksec lines show `[PASS]`
(PIE, full RELRO, NX, stack canary). `dist/` holds four `.nmcfw` files, each with a `.json`
manifest and a `.sha256`.

### 3.3 Sign and verify with a local key
```bash
scripts/gen-dev-signing-key.sh
make sign && make verify
printf 'X' | dd of=dist/nmc-GVS-1.4.0-rc.0.nmcfw bs=1 seek=500 conv=notrunc   # tamper (use your file name)
make verify                                                                      # Verification Failure
```
**Why.** In AWS this key lives in KMS (Phase 7). Practising locally first shows you exactly what
the pipeline automates.

### 3.4 Run the 4-card lab with Docker Compose
```bash
scripts/run-local-lab.sh
docker compose -f docker/docker-compose.yml ps
curl -s localhost:8084/api/status | jq .output_frequency        # GVXL-NAM -> ~60.0
snmpwalk -v2c -c lab-ro localhost:3161 1.3.6.1.2.1.33.1.1       # GVXL ident group
```
**Expect.** The integration suite passes against all four containers. The containers are
read-only, drop all capabilities and run as non-root. Check with
`docker inspect nmc-gvs | jq '.[0].HostConfig.CapDrop'` (use the real container name from
`docker compose ps`).

### 3.5 Scan the images yourself
```bash
trivy image --severity HIGH,CRITICAL nmc-simulator-gvs:local
syft nmc-simulator-gvs:local -o table | head -30
```

---

## Phase 4 — Jenkins locally

### 4.1 Start Jenkins, DinD and SonarQube
```bash
cd local/jenkins
cp ../../jenkins/plugins.txt plugins.txt        # optional: pinned corporate list (adds k8s/aws plugins)
export NMC_REPO_URL=https://github.com/<you>/nmc-firmware-devops.git
export JENKINS_ADMIN_PASSWORD='ChangeMe#123'
docker compose up -d --build
```
Open http://localhost:8090 and log in as `admin` with the password above.

**Expect.** No setup wizard. The job `nmc-firmware-local` already exists, because Configuration as
Code created it from `casc-local.yaml`.

### 4.2 Run the pipeline
Open **nmc-firmware-local → Scan Multibranch Pipeline Now → main**. Watch the stages from
`jenkins/Jenkinsfile.docker`: toolchain → static analysis → 4-way parallel build matrix →
simulator images → integration tests.

**Practise:**
1. Break a unit test (change an expected value in `firmware/tests/unit/test_modbus.c`), push it to a
   branch, and watch only that branch go red.
2. Add a `sprintf` call to `src/core/log.c` and watch the semgrep stage fail.
3. Open **Test Result** and the archived `reports/` folder.

### 4.3 Connect SonarQube
1. Open http://localhost:9000 (admin/admin, then change the password). Create the project
   `nmc-firmware` and generate a token.
2. In Jenkins, go to **Manage → Credentials** and add a Secret text with ID `sonar-token`. Then go to
   **System → SonarQube servers**, add `sonarqube` pointing at `http://sonarqube:9000`.
3. Run `sonar-scanner` from the toolchain container, or add a Sonar stage, and look at the quality
   gate.

**Why.** Corporate Jenkins is never configured by hand in the UI. Every setting lives in `casc`
YAML, and every job is Job DSL. If the server dies, you rebuild it from Git in minutes.

---

## Phase 5 — Security scans hands-on

Run each tool, read its output, then **break something on purpose** and watch the tool catch it.

| # | Scan | Command | Break it |
|---|---|---|---|
| 1 | Secrets | `gitleaks detect --config security/gitleaks.toml -v` | Commit a fake `AKIA...` key on a branch |
| 2 | cppcheck | `cppcheck --enable=warning,style -I firmware/include firmware/src` | Add an out-of-bounds array write |
| 3 | clang-tidy | `cmake -S firmware -B build/tidy -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && clang-tidy -p build/tidy --config-file=firmware/.clang-tidy firmware/src/modbus/modbus_tcp.c` | Use `atoi` on network input |
| 4 | semgrep (banned APIs) | `semgrep --config security/semgrep firmware/src` | `strcpy`, `system()`, `rand()` |
| 5 | flawfinder | `flawfinder --minlevel=3 firmware/src` | `char buf[10]; gets(buf);` |
| 6 | Hardening | `security/checksec.sh build/GVS-host/nmc-fw` | Build with `-DNMC_WERROR=OFF` and remove `-pie` |
| 7 | Sanitizers | `scripts/build.sh GVS --sanitize` | Remove a bounds check in `ber.c` and rerun the tests |
| 8 | Fuzzing | `make fuzz` (needs `libclang-rt-18-dev`, already in the toolchain image) | Remove the `qty > 125` check in `modbus_tcp.c` |
| 9 | SBOM + SCA | `syft dir:firmware -o cyclonedx-json > sbom.json && grype sbom:sbom.json` | — |
| 10 | Container | `trivy image nmc-simulator-gvs:local` | Change the base image to an old `ubuntu:20.04` |
| 11 | DAST | `docker run --rm --network host ghcr.io/zaproxy/zaproxy:stable zap-baseline.py -t http://localhost:8081` | Remove `X-Frame-Options` in `web_server.c` |
| 12 | Security tests | `pytest firmware/tests/integration/test_security.py` | Return the password hash from `/api/config` |

Then read `security/threat-model.md` and `security/iec62443-checklist.md`. For each security test,
find the threat it mitigates.

**Why.** Industrial products must follow **IEC 62443-4-1** (secure development lifecycle). Auditors
ask for evidence. The reports this pipeline archives *are* that evidence.

---

## Phase 6 — Kubernetes locally with kind

### 6.1 Create a cluster and load the images
```bash
kind create cluster --name nmc
for v in gvs gvl gvxl gvxl-nam; do kind load docker-image nmc-simulator-$v:local --name nmc; done
docker build -f docker/test-runner/Dockerfile -t nmc-test-runner:local . && kind load docker-image nmc-test-runner:local --name nmc
```

### 6.2 Deploy the four simulators with Helm, like `nmcDeploySim` does
```bash
NS=nmc-ci-local
kubectl create ns $NS
kubectl -n $NS create secret generic nmc-sim-credentials \
  --from-literal=admin-password='Test#Pass1' --from-literal=snmp-community=lab-ro
for v in GVS GVL GVXL GVXL-NAM; do
  lv=$(echo $v | tr A-Z a-z)
  helm upgrade --install nmc-$lv helm/nmc-simulator -n $NS --wait \
    --set variant=$v --set image.repository=nmc-simulator-$lv --set image.tag=local \
    --set simulation=true --set pcnsClients=nmc-itest-$lv.$NS.svc.cluster.local:3052
done
kubectl -n $NS get pods,svc
helm test nmc-gvs -n $NS
```

### 6.3 Run the integration tests as Kubernetes Jobs, like `nmcIntegrationTest` does
```bash
for v in GVS GVL GVXL GVXL-NAM; do lv=$(echo $v | tr A-Z a-z)
  sed -e "s/__VARIANT__/$v/g" -e "s/__LVARIANT__/$lv/g" -e "s|__IMAGE__|nmc-test-runner:local|" \
      k8s/test-jobs/integration-test-job.yaml | kubectl -n $NS apply -f -
  kubectl -n $NS wait --for=condition=complete --timeout=5m job/nmc-itest-$lv && echo "$v PASSED"
done
kubectl -n $NS logs job/nmc-itest-gvs | head -40
```

### 6.4 Prove the NetworkPolicy works
kind's default CNI does not enforce NetworkPolicy. Recreate the cluster with Calico if you want to
see it block traffic. Then:
```bash
kubectl -n $NS run intruder --rm -it --image=curlimages/curl -- curl -m3 http://nmc-gvs:8080/healthz   # blocked
kubectl -n $NS run ok --rm -it --labels=app=nmc-itest --image=curlimages/curl -- curl -m3 http://nmc-gvs:8080/healthz  # allowed
```

### 6.5 kubectl drills
`kubectl -n $NS port-forward svc/nmc-gvxl-nam 9090:8080`, `kubectl logs -f`, `kubectl describe`,
`kubectl exec`. Delete a pod and watch it come back. Set the image to a tag that doesn't exist and
debug the `ImagePullBackOff`.

---

## Phase 7 — AWS foundation with Terraform

> ⚠️ This costs money: EKS ≈ $0.10/h, NAT ≈ $0.05/h, nodes and endpoints extra. A budget alarm is
> included. **Run Phase 10.4 (destroy) when you finish each session.**

### 7.1 Set up AWS access
Create an AWS account, enable IAM Identity Center (SSO), and create an admin permission set.
Then run `aws configure sso` and check it with `aws sts get-caller-identity`.

### 7.2 Create remote state (once)
```bash
cd terraform/bootstrap && terraform init && terraform apply
```
Copy the `state_bucket` output into `terraform/environments/build-platform/backend.tf`, replacing
`nmc-tfstate-ACCOUNT_ID`.

### 7.3 Create the build platform
```bash
cd ../environments/build-platform
cp terraform.tfvars.example terraform.tfvars           # set your office IP/32 and SSO role ARNs
curl -fsSL -o policies/aws-load-balancer-controller.json \
  https://raw.githubusercontent.com/kubernetes-sigs/aws-load-balancer-controller/v2.9.2/docs/install/iam_policy.json
terraform init && terraform fmt -check -recursive ../../ && terraform validate
terraform plan -out tfplan        # READ IT: ~80 resources
terraform apply tfplan            # ~15-20 min (EKS)
```

**What you just created:**
- **VPC** with private subnets, one NAT gateway, and endpoints for S3, ECR, KMS, STS and Secrets
  Manager. Build traffic to AWS services never crosses the internet.
- **EKS** with two node groups:
  - `system` (on-demand) for the Jenkins controller, SonarQube and the lab simulators.
  - `build-agents` (Spot, tainted, scales to zero) for agent pods only.
- **ECR**: immutable tags, scan on push, continuous Inspector scanning, lifecycle rules.
- **S3**: `nmc-firmware-builds` (expires after 90 days) and `nmc-firmware-releases`
  (**Object Lock COMPLIANCE**, 10 years). Bucket policies allow TLS only.
- **KMS**: two ECC P-256 signing keys. Only the agent role can use `dev`. Only `nmc-release-signer`
  can use `prod`.
- **IRSA roles**: Jenkins agent, Jenkins controller, External Secrets, ALB controller. There are no
  AWS access keys anywhere.

### 7.4 Check it
```bash
aws eks update-kubeconfig --name nmc-build --region ap-south-1
kubectl get nodes -L nmc.io/pool
aws kms get-public-key --key-id alias/nmc-fw-signing-dev --query KeySpec
aws s3api get-object-lock-configuration --bucket nmc-firmware-releases
```

### 7.5 Push the images to ECR
```bash
REG=$(aws sts get-caller-identity --query Account --output text).dkr.ecr.ap-south-1.amazonaws.com
aws ecr get-login-password | docker login --username AWS --password-stdin $REG
for img in nmc-toolchain nmc-test-runner; do docker tag $img:local $REG/$img:latest && docker push $REG/$img:latest; done
docker build -t $REG/nmc-security-tools:latest docker/security-tools && docker push $REG/nmc-security-tools:latest
```
Replace `ACCOUNT_ID` everywhere: `grep -rln ACCOUNT_ID . | xargs sed -i "s/ACCOUNT_ID/<your-id>/g"`.

---

## Phase 8 — Jenkins on EKS (the full corporate pipeline)

### 8.1 Cluster add-ons
```bash
kubectl apply -f k8s/namespaces.yaml -f k8s/cluster/storageclass-gp3.yaml
helm repo add eks https://aws.github.io/eks-charts
helm repo add external-secrets https://charts.external-secrets.io
helm repo add jenkins https://charts.jenkins.io
helm repo add sonarqube https://SonarSource.github.io/helm-chart-sonarqube
helm repo add kyverno https://kyverno.github.io/kyverno && helm repo update

helm upgrade --install aws-load-balancer-controller eks/aws-load-balancer-controller -n kube-system \
  --set clusterName=nmc-build --set region=ap-south-1 --set serviceAccount.name=aws-load-balancer-controller \
  --set vpcId=$(terraform -chdir=terraform/environments/build-platform output -raw vpc_id) \
  --set serviceAccount.annotations."eks\.amazonaws\.com/role-arn"=$(terraform -chdir=terraform/environments/build-platform output -raw alb_controller_role_arn)
helm upgrade --install external-secrets external-secrets/external-secrets -n external-secrets --create-namespace \
  --set serviceAccount.annotations."eks\.amazonaws\.com/role-arn"=$(terraform -chdir=terraform/environments/build-platform output -raw external_secrets_role_arn)
kubectl apply -f k8s/cluster/cluster-secret-store.yaml
helm upgrade --install kyverno kyverno/kyverno -n kyverno --create-namespace && kubectl apply -f k8s/cluster/kyverno-policies.yaml
helm upgrade --install sonarqube sonarqube/sonarqube -n sonarqube -f k8s/sonarqube/values.yaml
```

### 8.2 Jenkins secrets in AWS Secrets Manager
```bash
aws secretsmanager put-secret-value --secret-id nmc/jenkins/casc --secret-string '{
  "SSO_METADATA_URL":"https://login.microsoftonline.com/<tenant>/federationmetadata/2007-06/federationmetadata.xml",
  "ECR_REGISTRY":"<id>.dkr.ecr.ap-south-1.amazonaws.com",
  "TEAMS_WEBHOOK_URL":"https://example.webhook.office.com/...",
  "SONAR_TOKEN":"<token>", "GITHUB_APP_ID":"123456", "GITHUB_APP_KEY":"-----BEGIN PRIVATE KEY-----...",
  "HIL_CARD_PASSWORD":"<lab card password>" }'
kubectl apply -f k8s/jenkins/externalsecret.yaml
kubectl -n jenkins get secret jenkins-casc-secrets            # synced by External Secrets
```
No Azure AD available? Swap the `saml:` block in `jenkins/casc/jenkins.yaml` for the `local:` realm
from `local/jenkins/casc-local.yaml`.

### 8.3 Install Jenkins
```bash
kubectl apply -f k8s/jenkins/agents-rbac.yaml -f k8s/jenkins/agents-quota.yaml
scripts/render-jenkins-values.sh            # merges JCasC + Job DSL + pinned plugins into the values
helm upgrade --install jenkins jenkins/jenkins -n jenkins -f k8s/jenkins/values-rendered.yaml --wait
kubectl -n jenkins get ingress              # internal ALB → map DNS jenkins.nmc.example.com via VPN
```
**Expect.** The `nmc/` folder contains `nmc-firmware` (CI), `nmc-firmware-release`, `nightly` and
`toolchain-image`. The kubernetes cloud `eks` is configured. You created nothing in the UI.

### 8.4 Run the full pipeline and watch it work
Push a branch, then watch:
```bash
kubectl -n jenkins-agents get pods -w             # agent pod with 7 containers appears
kubectl get nodes -L nmc.io/pool -w               # spot build node scales up from 0
kubectl get ns -l nmc.io/ephemeral=true           # nmc-ci-<branch> namespace with 4 simulators
```
Follow each stage of `jenkins/Jenkinsfile` and find where it happens:
1. gitleaks, clang-format and commit-lint run in parallel.
2. Four SAST tools run, and their results go into Warnings NG with a quality gate.
3. **The matrix** builds 4 variants × (host + tests + coverage, cross-compile, checksec, package).
4. SonarQube runs, then `waitForQualityGate`.
5. Syft produces the SBOM, and Grype fails the build on HIGH.
6. On main and release branches, KMS signs the images. In CloudTrail, check that an `kms:Sign`
   event appeared.
7. Kaniko builds and pushes four simulator images to ECR, with no Docker daemon and no privileged
   pod.
8. Trivy scans every image.
9. Helm deploys the simulators to the ephemeral namespace, and four pytest Jobs run. JUnit results
   appear in Jenkins.
10. The ZAP baseline scan runs against the GVS web API.
11. On main, the build uploads to `s3://nmc-firmware-builds/main/<version>/`.
12. `post` deletes the namespace and sends the Teams notification.

### 8.5 Make Jenkins a required GitHub check
Create a GitHub App, install it on the repo, and set up its webhook to `https://jenkins…/github-webhook/`.
Then add the `continuous-integration/jenkins/branch` check to the branch protection rule from 0.3.

---

## Phase 9 — Signed release with approvals

```bash
git switch release/1.4 && git pull
# update CHANGELOG.md, make sure VERSION is 1.4.0
git tag -s v1.4.0 -m "NMC firmware 1.4.0"          # needs your GPG key configured in git
git push origin v1.4.0
```
Watch `nmc/nmc-firmware-release` run:
1. Verify the tag signature and that the tag matches `VERSION`.
2. Rebuild from scratch. It never reuses CI binaries.
3. **Reproducibility check**: build twice and compare byte for byte.
4. Full security gate (fails on MEDIUM and above).
5. **Input step**: only `nmc-product-owners` or `nmc-security-officers` can approve.
6. Assume `nmc-release-signer` and sign with `alias/nmc-fw-signing-prod`.
7. Upload to the WORM bucket and create the GitHub release.

Then verify what you shipped:
```bash
aws s3 cp s3://nmc-firmware-releases/releases/1.4.0/ ./rel --recursive
for f in rel/*.nmcfw; do security/verify-firmware.sh $f rel/signing-public.pem; done
aws s3 rm s3://nmc-firmware-releases/releases/1.4.0/nmc-GVS-1.4.0.nmcfw --version-id <id>   # AccessDenied: Object Lock
```

---

## Phase 10 — Operate

### 10.1 Nightly
Trigger `nmc/nightly` by hand once. It runs:
- ASan/UBSan builds.
- 15 minutes of libFuzzer per protocol parser.
- A 2-hour soak against the lab simulators (`k8s/lab/install.sh`).
- A CVE re-scan of yesterday's images.
- Cleanup of stale PR namespaces.

### 10.2 Incident drills
Use `docs/runbooks/`. Cause each problem, then fix it by following the runbook:

| Drill | How to cause it |
|---|---|
| Agent pod Pending | Set the build-agents `max_size` to 0 |
| Signing failure | Change `keyAlias` to the prod key in the CI Jenkinsfile; the agent role is denied |
| Simulator CrashLoop | Set `ports.http=80` in the Helm values (non-root can't bind ports below 1024) |
| Trivy blocks the build | Pin the simulator runtime to an old base image |
| Quality gate failed | Delete a unit-test file |

After each drill, write a short postmortem: timeline, root cause, action items.

### 10.3 Hardware-in-the-loop (if you have a lab or a spare Linux box)
`ansible/` shows how real cards get flashed: verify the signature first, upload, wait for reboot,
check the variant, then run the same pytest suite against the physical card.
```bash
ansible-playbook -i ansible/inventories/hil-lab.yml ansible/playbooks/hil-agent.yml   # provision the lab agent
```
The pipeline's HIL stage takes the Jenkins **lockable resource** `hil-rack-1`, so two builds never
flash the same cards at the same time. Without hardware, point the inventory at your Docker
simulators and practise with `verify-cards.yml`. Drop the flash step, because the simulator has no
`/api/firmware`.

### 10.4 Tear down
```bash
helm -n jenkins uninstall jenkins; helm -n sonarqube uninstall sonarqube
kubectl delete ns jenkins-agents nmc-lab $(kubectl get ns -l nmc.io/ephemeral=true -o name | cut -d/ -f2)
cd terraform/environments/build-platform && terraform destroy
```
The releases bucket **cannot** be emptied while Object Lock retention is active. That is by design.
For practice accounts, set `release_retention_years = 1` (or use GOVERNANCE mode) before your first
apply.

---

## Appendix A — Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `bind port 8080 failed` | Another simulator is still running | `pkill nmc-fw` or change `NMC_HTTP_PORT` |
| SNMP request times out | Wrong community string, or UDP port not published | Check `NMC_SNMP_COMMUNITY`; compose needs `/udp` |
| `GLIBC_2.38 not found` in the container | Runtime base is older than the toolchain base | Keep both on ubuntu:24.04 (see simulator Dockerfile) |
| `-Werror=format-truncation` only on ARM | 32-bit types | See `docs/runbooks/cross-compile-failure.md` |
| PCNS test fails in Kubernetes | `pcnsClients` doesn't point at the test Job service | Check the `--set pcnsClients=` value in `nmcDeploySim` |
| Jenkins shows the setup wizard | JCasC not loaded | Check `CASC_JENKINS_CONFIG` and the configScripts mount |
| Terraform `file(...policies/...)` error | ALB policy not downloaded | Step 7.3 curl |

## Appendix B — Glossary
**NMC**: network card inside the UPS that exposes it on the LAN.
**PCNS**: PowerChute Network Shutdown, the agent on servers that shuts them down gracefully on a power
event.
**MIB/OID**: SNMP's data model and object identifiers.
**MBAP**: the Modbus TCP header.
**HIL**: hardware-in-the-loop testing on real cards.
**SBOM**: software bill of materials.
**SCA/SAST/DAST**: dependency, static and dynamic security testing.
**IRSA**: IAM Roles for Service Accounts.
**JCasC**: Jenkins Configuration as Code.
**WORM**: write once, read many.
**IEC 62443**: the industrial cybersecurity standard family.

## Appendix C — Daily practice checklist
- [ ] Make one feature-branch change with a Conventional Commit, and get it green in CI.
- [ ] Read one scanner report end to end and fix or justify each finding.
- [ ] Do one kubectl or Terraform drill from this guide without looking at the commands.
- [ ] Explain one ADR (`docs/adr/`) out loud in two minutes.
- [ ] Answer one question from `docs/interview-questions.md` in writing.
