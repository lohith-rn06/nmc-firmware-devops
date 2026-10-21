output "key_arns" {
  value = { for k, v in aws_kms_key.signing : k => v.arn }
}
