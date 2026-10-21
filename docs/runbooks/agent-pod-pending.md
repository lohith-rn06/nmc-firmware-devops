# Runbook: Jenkins agent pod stuck in Pending
1. `kubectl -n jenkins-agents get pods` → `kubectl describe pod <pod>` → read **Events**.
2. `Insufficient cpu/memory` → build-agents node group at max? `aws eks describe-nodegroup --cluster-name nmc-build --nodegroup-name build-agents`. Raise `max_build_nodes` in Terraform.
3. `untolerated taint` → the pod template lost its toleration (`nmc-build-pod.yaml`).
4. `ImagePullBackOff` → check ECR tag exists and node role has `AmazonEC2ContainerRegistryReadOnly`.
5. Quota exceeded → `kubectl -n jenkins-agents describe resourcequota agents`.
