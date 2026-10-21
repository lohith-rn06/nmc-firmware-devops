variable "name" {
  type = string
}

variable "k8s_version" {
  type    = string
  default = "1.31"
}

variable "subnet_ids" {
  type = list(string)
}

variable "public_endpoint" {
  type    = bool
  default = true
}

variable "admin_cidrs" {
  type        = list(string)
  description = "Office/VPN CIDRs allowed to reach the public API endpoint"
}

variable "admin_role_arns" {
  type    = list(string)
  default = []
}

variable "max_build_nodes" {
  type    = number
  default = 6
}
