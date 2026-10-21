output "builds_bucket_arn" {
  value = aws_s3_bucket.builds.arn
}

output "releases_bucket_arn" {
  value = aws_s3_bucket.releases.arn
}
