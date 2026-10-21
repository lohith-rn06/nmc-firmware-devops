# Two buckets:
#  * builds   - every CI build (versioned, expires after 90 days)
#  * releases - released firmware, Object Lock COMPLIANCE mode (WORM): nobody, not even root,
#               can delete or overwrite a shipped firmware image during the retention period.
resource "aws_s3_bucket" "builds" {
  bucket = "${var.prefix}-builds"
}

resource "aws_s3_bucket" "releases" {
  bucket              = "${var.prefix}-releases"
  object_lock_enabled = true
}

resource "aws_s3_bucket_versioning" "this" {
  for_each = { builds = aws_s3_bucket.builds.id, releases = aws_s3_bucket.releases.id }
  bucket   = each.value
  versioning_configuration {
    status = "Enabled"
  }
}

resource "aws_s3_bucket_object_lock_configuration" "releases" {
  bucket = aws_s3_bucket.releases.id
  rule {
    default_retention {
      mode  = "COMPLIANCE"
      years = var.release_retention_years
    }
  }
}

resource "aws_s3_bucket_server_side_encryption_configuration" "this" {
  for_each = { builds = aws_s3_bucket.builds.id, releases = aws_s3_bucket.releases.id }
  bucket   = each.value
  rule {
    apply_server_side_encryption_by_default {
      sse_algorithm = "aws:kms"
    }
    bucket_key_enabled = true
  }
}

resource "aws_s3_bucket_public_access_block" "this" {
  for_each                = { builds = aws_s3_bucket.builds.id, releases = aws_s3_bucket.releases.id }
  bucket                  = each.value
  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_s3_bucket_lifecycle_configuration" "builds" {
  bucket = aws_s3_bucket.builds.id
  rule {
    id     = "expire-ci-builds"
    status = "Enabled"
    filter {}
    expiration {
      days = 90
    }
    noncurrent_version_expiration {
      noncurrent_days = 7
    }
  }
}

data "aws_iam_policy_document" "tls_only" {
  for_each = { builds = aws_s3_bucket.builds.arn, releases = aws_s3_bucket.releases.arn }
  statement {
    sid       = "DenyInsecureTransport"
    effect    = "Deny"
    actions   = ["s3:*"]
    resources = [each.value, "${each.value}/*"]
    principals {
      type        = "*"
      identifiers = ["*"]
    }
    condition {
      test     = "Bool"
      variable = "aws:SecureTransport"
      values   = ["false"]
    }
  }
}

resource "aws_s3_bucket_policy" "this" {
  for_each = { builds = aws_s3_bucket.builds.id, releases = aws_s3_bucket.releases.id }
  bucket   = each.value
  policy   = data.aws_iam_policy_document.tls_only[each.key].json
}
