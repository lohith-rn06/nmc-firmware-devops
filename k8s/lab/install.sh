#!/usr/bin/env bash
# Always-on lab: 4 simulated cards in namespace nmc-lab (used for demos, soak tests, manual SNMP/Modbus tools).
set -euo pipefail
NS=nmc-lab
kubectl get ns $NS >/dev/null 2>&1 || kubectl create ns $NS
kubectl -n $NS get secret nmc-sim-credentials >/dev/null 2>&1 || kubectl -n $NS create secret generic nmc-sim-credentials \
  --from-literal=admin-password="$(openssl rand -base64 18)" --from-literal=snmp-community="lab-$(openssl rand -hex 4)"
for v in gvs gvl gvxl gvxl-nam; do
  helm upgrade --install nmc-$v helm/nmc-simulator -n $NS -f k8s/lab/values-$v.yaml --wait
done
kubectl -n $NS get pods,svc
