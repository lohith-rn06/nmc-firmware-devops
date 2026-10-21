#!/usr/bin/env bash
# Render k8s/jenkins/values-rendered.yaml = values.yaml + JCasC (jenkins/casc/jenkins.yaml)
# + Job DSL seed (inlined) + pinned plugin list (jenkins/plugins.txt).
#   scripts/render-jenkins-values.sh && helm upgrade --install jenkins jenkins/jenkins -n jenkins -f k8s/jenkins/values-rendered.yaml
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PY'
import re
casc = open("jenkins/casc/jenkins.yaml").read()
seed = open("jenkins/job-dsl/seed-jobs.groovy").read()
# replace the 'jobs:' block (file reference) with the inlined Job DSL script
casc = re.sub(r"(?ms)^jobs:\n.*\Z", "jobs:\n  - script: |\n" + "".join("      " + l for l in seed.splitlines(True)), casc)
plugins = [l.strip() for l in open("jenkins/plugins.txt") if l.strip() and not l.startswith("#")]
values = open("k8s/jenkins/values.yaml").read()
values = values.replace("  installPlugins: false            # plugins come from the pinned list below\n",
                        "  installPlugins:\n" + "".join(f"    - {p}\n" for p in plugins))
block = "      nmc: |\n" + "".join("        " + l for l in casc.splitlines(True))
values = re.sub(r"      nmc: \|\n        # contents of .*\n", block, values)
open("k8s/jenkins/values-rendered.yaml", "w").write(values)
print("wrote k8s/jenkins/values-rendered.yaml")
PY
