# Branching & versioning

* **main** – always releasable; protected (PR + green CI + 1 CODEOWNER review, signed commits).
* **feature/<JIRA>-short-name** – short-lived, merged by squash.
* **release/1.4** – cut at feature freeze; only `fix`/`sec` commits (cherry-picked from main).
* **hotfix/1.4.1** – from the release tag for urgent security fixes.
* **Tags** `v1.4.0` – GPG-signed by a release manager; triggers `Jenkinsfile.release`.

Versions (SemVer, computed by `scripts/version.sh`):
| Where | Example |
|---|---|
| feature branch | `1.4.0-dev.57+a1b2c3d` |
| main / release branch | `1.4.0-rc.112+a1b2c3d` |
| tag | `1.4.0` |

Commit messages: Conventional Commits with an NMC scope — `feat(snmp): …`, `fix(modbus): …`,
`sec(auth): …`. Enforced by `scripts/commit-lint.sh` in CI and by pre-commit locally.
