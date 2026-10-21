output "cluster_name" {
  value = module.eks.cluster_name
}

output "ecr_urls" {
  value = module.ecr.repository_urls
}

output "signing_key_arns" {
  value = module.signing.key_arns
}

output "jenkins_agent_role_arn" {
  value = module.jenkins_agent_role.role_arn
}

output "external_secrets_role_arn" {
  value = module.external_secrets_role.role_arn
}

output "alb_controller_role_arn" {
  value = module.alb_controller_role.role_arn
}

output "vpc_id" {
  value = module.vpc.vpc_id
}
