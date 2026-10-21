locals {
  name = "nmc-build"
  azs  = ["ap-south-1a", "ap-south-1b"]
}

data "aws_caller_identity" "me" {}

module "vpc" {
  source = "../../modules/vpc"
  name   = local.name
  region = var.region
  cidr   = "10.40.0.0/16"
  azs    = local.azs
}

module "eks" {
  source          = "../../modules/eks"
  name            = local.name
  subnet_ids      = module.vpc.private_subnet_ids
  admin_cidrs     = var.admin_cidrs
  admin_role_arns = var.admin_role_arns
}

module "ecr" {
  source = "../../modules/ecr"
  repositories = [
    "nmc-toolchain", "nmc-security-tools", "nmc-test-runner", "kaniko-cache",
    "nmc-simulator-gvs", "nmc-simulator-gvl", "nmc-simulator-gvxl", "nmc-simulator-gvxl-nam",
  ]
}

module "artifacts" {
  source = "../../modules/s3-artifacts"
  prefix = "nmc-firmware"
}

# --- IRSA roles ---------------------------------------------------------------
module "jenkins_agent_role" {
  source            = "../../modules/irsa"
  role_name         = "nmc-jenkins-agent"
  oidc_provider_arn = module.eks.oidc_provider_arn
  oidc_issuer       = module.eks.oidc_issuer
  namespace         = "jenkins-agents"
  service_account   = "jenkins-agent"
}

# Agent permissions are attached here (not inside the module) so the KMS key policy, which
# names this role, can be created after the role exists without a dependency cycle.
resource "aws_iam_role_policy" "jenkins_agent" {
  name = "nmc-jenkins-agent"
  role = module.jenkins_agent_role.role_name
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      { Sid = "EcrAuth", Effect = "Allow", Action = ["ecr:GetAuthorizationToken"], Resource = "*" },
      { Sid = "EcrPushPull", Effect = "Allow", Resource = module.ecr.repository_arns,
      Action = ["ecr:BatchCheckLayerAvailability", "ecr:BatchGetImage", "ecr:GetDownloadUrlForLayer", "ecr:InitiateLayerUpload", "ecr:UploadLayerPart", "ecr:CompleteLayerUpload", "ecr:PutImage"] },
      { Sid = "Builds", Effect = "Allow", Action = ["s3:PutObject", "s3:GetObject", "s3:ListBucket"],
      Resource = [module.artifacts.builds_bucket_arn, "${module.artifacts.builds_bucket_arn}/*"] },
      { Sid = "DevSigning", Effect = "Allow", Action = ["kms:Sign", "kms:GetPublicKey"], Resource = module.signing.key_arns["dev"] },
      { Sid = "AssumeReleaseSigner", Effect = "Allow", Action = "sts:AssumeRole", Resource = aws_iam_role.release_signer.arn },
    ]
  })
}

module "jenkins_controller_role" {
  source            = "../../modules/irsa"
  role_name         = "nmc-jenkins-controller"
  oidc_provider_arn = module.eks.oidc_provider_arn
  oidc_issuer       = module.eks.oidc_issuer
  namespace         = "jenkins"
  service_account   = "jenkins"
  policy_json = jsonencode({
    Version   = "2012-10-17"
    Statement = [{ Effect = "Allow", Action = ["secretsmanager:GetSecretValue"], Resource = "arn:aws:secretsmanager:${var.region}:${data.aws_caller_identity.me.account_id}:secret:nmc/jenkins/*" }]
  })
}

# Release signer: only this role may use the PRODUCTION key and write to the release bucket.
# Assumable by Jenkins agents, but only from the release pipeline (session tag checked by SCP/permission boundary).
resource "aws_iam_role" "release_signer" {
  name       = "nmc-release-signer"
  depends_on = [module.jenkins_agent_role] # trust policy names the agent role; it must exist first
  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [{
      Effect    = "Allow"
      Principal = { AWS = "arn:aws:iam::${data.aws_caller_identity.me.account_id}:role/nmc-jenkins-agent" }
      Action    = "sts:AssumeRole"
      Condition = { StringLike = { "sts:RoleSessionName" = "nmc-sign-*" } }
    }]
  })
}

resource "aws_iam_role_policy" "release_signer" {
  role = aws_iam_role.release_signer.id
  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      { Effect = "Allow", Action = ["kms:Sign", "kms:GetPublicKey"], Resource = module.signing.key_arns["prod"] },
      { Effect = "Allow", Action = ["s3:PutObject", "s3:PutObjectRetention", "s3:GetObject", "s3:ListBucket"],
      Resource = [module.artifacts.releases_bucket_arn, "${module.artifacts.releases_bucket_arn}/*"] },
    ]
  })
}

module "signing" {
  source             = "../../modules/kms-signing"
  depends_on         = [module.jenkins_agent_role, aws_iam_role.release_signer] # KMS rejects policies naming unknown roles
  key_admin_role_arn = var.key_admin_role_arn
  signers = {
    dev  = ["arn:aws:iam::${data.aws_caller_identity.me.account_id}:role/nmc-jenkins-agent"]
    prod = ["arn:aws:iam::${data.aws_caller_identity.me.account_id}:role/nmc-release-signer"]
  }
}

resource "aws_secretsmanager_secret" "jenkins_casc" {
  name                    = "nmc/jenkins/casc"
  recovery_window_in_days = 7
}

resource "aws_budgets_budget" "monthly" {
  name         = "nmc-build-platform"
  budget_type  = "COST"
  limit_amount = "400"
  limit_unit   = "USD"
  time_unit    = "MONTHLY"
  notification {
    comparison_operator        = "GREATER_THAN"
    threshold                  = 80
    threshold_type             = "PERCENTAGE"
    notification_type          = "FORECASTED"
    subscriber_email_addresses = ["nmc-devops@example.com"]
  }
}

# --- IRSA for cluster add-ons ---------------------------------------------------
module "external_secrets_role" {
  source            = "../../modules/irsa"
  role_name         = "nmc-external-secrets"
  oidc_provider_arn = module.eks.oidc_provider_arn
  oidc_issuer       = module.eks.oidc_issuer
  namespace         = "external-secrets"
  service_account   = "external-secrets"
  policy_json = jsonencode({
    Version = "2012-10-17"
    Statement = [{ Effect = "Allow", Action = ["secretsmanager:GetSecretValue", "secretsmanager:DescribeSecret"],
    Resource = "arn:aws:secretsmanager:${var.region}:${data.aws_caller_identity.me.account_id}:secret:nmc/*" }]
  })
}

module "alb_controller_role" {
  source            = "../../modules/irsa"
  role_name         = "nmc-aws-load-balancer-controller"
  oidc_provider_arn = module.eks.oidc_provider_arn
  oidc_issuer       = module.eks.oidc_issuer
  namespace         = "kube-system"
  service_account   = "aws-load-balancer-controller"
  policy_json       = file("${path.module}/policies/aws-load-balancer-controller.json")
}
