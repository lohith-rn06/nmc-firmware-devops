# Runbook: KMS signing step fails
* `AccessDeniedException kms:Sign` → wrong key alias for this pipeline (dev vs prod) or the IRSA role is not attached: `kubectl -n jenkins-agents get sa jenkins-agent -o yaml`.
* `AssumeRole denied` in release → role session name must start with `nmc-sign-` (trust policy condition).
* Verify the public key matches what the cards trust: `aws kms get-public-key --key-id alias/nmc-fw-signing-prod`.
Never fall back to a local key for a release.
