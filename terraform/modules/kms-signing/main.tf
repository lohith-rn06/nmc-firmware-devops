# Asymmetric ECC keys for firmware signing. Private keys are generated and kept inside
# AWS KMS HSMs (FIPS 140-2 L3) and can never be exported - only kms:Sign is exposed.
data "aws_caller_identity" "me" {}

data "aws_iam_policy_document" "key" {
  for_each = var.signers
  statement {
    sid       = "AccountAdmin"
    actions   = ["kms:Create*", "kms:Describe*", "kms:Enable*", "kms:List*", "kms:Put*", "kms:Update*", "kms:Revoke*", "kms:Disable*", "kms:Get*", "kms:Delete*", "kms:TagResource", "kms:UntagResource", "kms:ScheduleKeyDeletion", "kms:CancelKeyDeletion"]
    resources = ["*"]
    principals {
      type        = "AWS"
      identifiers = [var.key_admin_role_arn]
    }
  }
  statement {
    sid       = "SignOnly"
    actions   = ["kms:Sign", "kms:GetPublicKey", "kms:DescribeKey"]
    resources = ["*"]
    principals {
      type        = "AWS"
      identifiers = each.value
    }
  }
  statement {
    sid       = "AnyoneInAccountMayVerify"
    actions   = ["kms:Verify", "kms:GetPublicKey"]
    resources = ["*"]
    principals {
      type        = "AWS"
      identifiers = ["arn:aws:iam::${data.aws_caller_identity.me.account_id}:root"]
    }
  }
}

resource "aws_kms_key" "signing" {
  for_each                 = var.signers
  description              = "NMC firmware signing key (${each.key})"
  customer_master_key_spec = "ECC_NIST_P256"
  key_usage                = "SIGN_VERIFY"
  deletion_window_in_days  = 30
  policy                   = data.aws_iam_policy_document.key[each.key].json
}

resource "aws_kms_alias" "signing" {
  for_each      = var.signers
  name          = "alias/nmc-fw-signing-${each.key}"
  target_key_id = aws_kms_key.signing[each.key].key_id
}
