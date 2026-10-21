provider "aws" {
  region = var.region
  default_tags {
    tags = {
      Project    = "nmc-firmware"
      Owner      = "nmc-devops"
      ManagedBy  = "terraform"
      CostCenter = "ups-connectivity"
    }
  }
}
