Download the official AWS Load Balancer Controller IAM policy here before `terraform plan`:

    curl -fsSL -o aws-load-balancer-controller.json \
      https://raw.githubusercontent.com/kubernetes-sigs/aws-load-balancer-controller/v2.9.2/docs/install/iam_policy.json

It is not vendored so you always review the exact version you deploy.
