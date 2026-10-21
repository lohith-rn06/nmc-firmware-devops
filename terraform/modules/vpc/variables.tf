variable "name" {
  type = string
}

variable "region" {
  type = string
}

variable "cidr" {
  type = string
}

variable "azs" {
  type = list(string)
}
