# Jenkins

| File | Purpose |
|---|---|
| `Jenkinsfile` | Main multibranch CI (EKS agents) |
| `Jenkinsfile.release` | Tag-triggered release with approvals + production signing |
| `Jenkinsfile.nightly` | Sanitizers, fuzzing, soak test, CVE re-scan |
| `Jenkinsfile.toolchain` | Rebuilds the toolchain image |
| `Jenkinsfile.docker` | Local-lab pipeline using plain Docker agents |
| `shared-library/` | Reusable steps (`nmcBuild`, `nmcSign`, …) loaded as `nmc-shared-lib` |
| `casc/jenkins.yaml` | Jenkins Configuration as Code for the EKS controller |
| `job-dsl/seed-jobs.groovy` | All jobs defined as code |

Pipeline flow:
```
checkout → pre-checks (gitleaks, format, commit-lint) → SAST (cppcheck, clang-tidy, semgrep, flawfinder)
  → matrix[GVS,GVL,GVXL,GVXL-NAM]: host build+unit tests+coverage → cross-compile → checksec → package .nmcfw
  → SonarQube gate → SBOM+SCA → sign (KMS) → Kaniko images → Trivy → Helm deploy sims → pytest Jobs
  → ZAP DAST → (HIL rack) → S3 publish → cleanup
```
