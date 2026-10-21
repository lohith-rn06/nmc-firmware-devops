# IAM role assumable by one Kubernetes service account (IRSA) - no static AWS keys in Jenkins.
data "aws_iam_policy_document" "trust" {
  statement {
    actions = ["sts:AssumeRoleWithWebIdentity"]
    principals {
      type        = "Federated"
      identifiers = [var.oidc_provider_arn]
    }
    condition {
      test     = "StringEquals"
      variable = "${replace(var.oidc_issuer, "https://", "")}:sub"
      values   = ["system:serviceaccount:${var.namespace}:${var.service_account}"]
    }
    condition {
      test     = "StringEquals"
      variable = "${replace(var.oidc_issuer, "https://", "")}:aud"
      values   = ["sts.amazonaws.com"]
    }
  }
}

resource "aws_iam_role" "this" {
  name                 = var.role_name
  assume_role_policy   = data.aws_iam_policy_document.trust.json
  max_session_duration = 3600
}

resource "aws_iam_role_policy" "this" {
  count  = var.policy_json == null ? 0 : 1
  role   = aws_iam_role.this.id
  policy = var.policy_json
}
