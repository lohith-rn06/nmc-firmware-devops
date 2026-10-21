# Runbook: Trivy/Grype blocks the build with a HIGH/CRITICAL CVE
1. Which package and layer? (`reports/trivy-<variant>.sarif`). Base image (ubuntu) or our code?
2. Base image: bump the base / rebuild the toolchain image; most fixes are already published.
3. No fix available and not exploitable on the NMC → add to `security/.trivyignore` with reason, owner and expiry, approved by security.
4. Released firmware affected → open a PSIRT ticket (see `security/vulnerability-management.md`).
