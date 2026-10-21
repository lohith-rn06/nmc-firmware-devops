terraform {
  backend "s3" {
    bucket         = "nmc-tfstate-ACCOUNT_ID"
    key            = "build-platform/terraform.tfstate"
    region         = "ap-south-1"
    dynamodb_table = "nmc-tf-lock"
    encrypt        = true
  }
}
