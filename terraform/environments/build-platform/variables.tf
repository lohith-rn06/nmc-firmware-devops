variable "region" {
  type    = string
  default = "ap-south-1"
}

variable "admin_cidrs" {
  type        = list(string)
  description = "Office / VPN egress CIDRs"
}

variable "admin_role_arns" {
  type        = list(string)
  description = "SSO roles that get cluster-admin on EKS"
}

variable "key_admin_role_arn" {
  type = string
}
