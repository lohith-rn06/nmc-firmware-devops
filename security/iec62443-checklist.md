# IEC 62443-4-1 (secure development lifecycle) - where each practice lives

| Practice | Requirement | Evidence in this project |
|---|---|---|
| SM-1 Development process | Documented, repeatable SDL | `docs/STEP-BY-STEP-GUIDE.md`, Jenkinsfiles |
| SM-8 Controls for private keys | Signing keys protected | AWS KMS asymmetric keys, prod key only usable by release role (`terraform/modules/kms-signing`) |
| SM-9 Third-party component security | SBOM + vuln tracking | `syft` SBOMs, `grype`/`trivy` gates |
| SR-2 Threat model | Maintained threat model | `security/threat-model.md` |
| SD-4 Secure design best practices | Defense in depth | Secure defaults (no default pw, Modbus writes off) |
| SI-1 Security implementation review | Static analysis | cppcheck, clang-tidy, semgrep, flawfinder, SonarQube gate |
| SI-2 Secure coding standards | Banned APIs | `security/semgrep/c-banned-functions.yml`, CERT C checks |
| SVV-1 Security requirements testing | Tests for each security requirement | `firmware/tests/integration/test_security.py` |
| SVV-3 Vulnerability testing | Fuzzing, DAST | libFuzzer nightly, OWASP ZAP baseline |
| DM-1..5 Defect management | Triage SLAs | `security/vulnerability-management.md` |
| SUM-1..5 Security update management | Signed, verifiable updates | `.nmcfw` + `.sig`, WORM release bucket |
