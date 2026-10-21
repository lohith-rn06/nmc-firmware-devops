# NMC Firmware DevOps — practice project

A realistic, **runnable** practice repo for the DevOps work around a 3-phase UPS
**Network Management Card (NMC)** firmware with four card variants:

| Variant | Class | CPU (cross target) | Grid | Extra features |
|---|---|---|---|---|
| **GVS** | compact 3-phase | ARM 32-bit (armhf) | 400 V / 50 Hz | — |
| **GVL** | mid-range | ARM 64-bit (aarch64) | 400 V / 50 Hz | — |
| **GVXL** | large data-center | aarch64 | 400 V / 50 Hz | parallel-UPS |
| **GVXL-NAM** | North America | aarch64 | 480 V / 60 Hz | parallel-UPS, FIPS mode |

Firmware features (real, working C code in `firmware/`): **SNMP v1/v2c agent** (MIB-II + RFC 1628 UPS-MIB),
**Modbus TCP server**, **TCP/IP** dual-stack networking, **PCNS** shutdown notifications, **HTTP/REST** web API,
**security** (salted hashes, lockout, audit log, no default credentials), **OTA** firmware image validation.

> All code here is an original simulation for learning. It is not vendor source code; the
> variant names, ratings and register maps are illustrative.

## The DevOps around it
```
Git (branches, PRs, Conventional Commits, CODEOWNERS)
  └─► Jenkins on EKS (JCasC, Job DSL, shared library, Kubernetes agent pods)
        ├─ SAST: cppcheck · clang-tidy · semgrep · flawfinder · SonarQube   ├─ secrets: gitleaks
        ├─ matrix build: 4 variants × (host+tests+coverage, ARM cross-compile) ├─ checksec hardening
        ├─ package .nmcfw → sign with AWS KMS (ECDSA P-256)                  ├─ SBOM (syft) + SCA (grype)
        ├─ Docker: toolchain image, simulator image per variant (Kaniko → ECR) ├─ Trivy image scan
        ├─ Kubernetes: Helm-deploy 4 simulators per branch → pytest Jobs     ├─ DAST: OWASP ZAP
        ├─ HIL: Ansible flashes real cards on a locked lab rack (optional)
        └─ publish to S3 (builds) / WORM S3 (releases) after approvals
AWS (Terraform): VPC + endpoints · EKS (system + spot build pool) · ECR · S3 · KMS · IRSA · Secrets Manager · Budgets
```

## Start here
**➡ [`docs/STEP-BY-STEP-GUIDE.md`](docs/STEP-BY-STEP-GUIDE.md)** — 10 phases, from compiling on your laptop to a
signed release on AWS. Phases 0–4 are free; phases 5+ need an AWS account.

Quick taste (only gcc, cmake and python3 needed):
```bash
make build VARIANT=GVXL-NAM          # build + 7 unit-test suites
NMC_SIMULATION=1 NMC_ADMIN_PASS='Test#Pass1' NMC_SNMP_COMMUNITY=lab-ro build/GVXL-NAM-host/nmc-fw
curl localhost:8080/api/status        # in a second terminal
```

## Layout
```
firmware/      C firmware: src/{core,net,snmp,modbus,http,pcns,security,ota}, variants/, tests/{unit,integration,fuzz}
tools/         fwpack.py - builds the .nmcfw image
docker/        toolchain, simulator, test-runner, security-tools images + docker-compose lab
jenkins/       Jenkinsfile(s), shared library, JCasC, Job DSL, plugins
local/jenkins/ Jenkins + DinD + SonarQube in Docker for local practice
helm/          nmc-simulator chart
k8s/           Jenkins on EKS values, RBAC, quotas, policies, test Job template, lab install
terraform/     AWS modules + build-platform environment
ansible/       HIL rack: flash + verify physical cards, provision lab agent
security/      scanner configs, signing/verification, threat model, IEC 62443 mapping
scripts/       build, package, version, release notes, lab helpers
docs/          step-by-step guide, architecture, branching, release, ADRs, runbooks
```
