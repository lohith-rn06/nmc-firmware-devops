variable "role_name" {
  type = string
}

variable "oidc_provider_arn" {
  type = string
}

variable "oidc_issuer" {
  type = string
}

variable "namespace" {
  type = string
}

variable "service_account" {
  type = string
}

variable "policy_json" {
  type        = string
  default     = null
  description = "Inline policy; leave null and attach policies in the caller to avoid dependency cycles"
}
