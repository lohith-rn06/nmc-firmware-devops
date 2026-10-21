# Release process

1. Feature freeze → cut `release/1.4` from main. Bump `VERSION` on main to `1.5.0`.
2. QA runs regression + HIL on `release/1.4` builds (`-rc.N`).
3. Release manager updates `CHANGELOG.md`, then `git tag -s v1.4.0 && git push origin v1.4.0`.
4. `Jenkinsfile.release` runs: verify tag signature → clean rebuild → reproducibility check →
   full security gate (no medium+ CVEs) → **approval** (Product Owner + Security Officer) →
   sign with **production KMS key** → upload to the WORM release bucket → GitHub release.
5. Publish customer download + PSIRT notes. Release bucket objects cannot be deleted for 10 years.
