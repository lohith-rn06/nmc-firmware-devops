# Runbook: SonarQube quality gate failed
1. Open the Sonar link in the build log. New code must have ≥ 70 % coverage, 0 new bugs/vulnerabilities, ≤ 3 % duplication.
2. Coverage: add unit tests in `firmware/tests/unit`. Security hotspot: fix or mark "reviewed – safe" with a reason (needs security group).
3. Do not lower the gate in a PR; gate changes go through the security officer.
