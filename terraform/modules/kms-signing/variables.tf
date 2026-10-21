variable "signers" {
  type        = map(list(string))
  description = "key name (dev|prod) => IAM role ARNs allowed to sign with it"
}

variable "key_admin_role_arn" {
  type = string
}
