// Upload firmware, signatures, manifests, SBOMs and reports to S3 with integrity metadata.
def call(Map args) {
  def prefix = args.prefix ?: "${env.BRANCH_NAME}/${env.NMC_VERSION}"
  sh """
    cd dist && sha256sum *.nmcfw > SHA256SUMS
    aws s3 cp . s3://${args.bucket}/${prefix}/ --recursive --only-show-errors \
      --metadata git-commit=\$GIT_COMMIT,jenkins-build=\$BUILD_URL
    cd .. && aws s3 cp reports/ s3://${args.bucket}/${prefix}/reports/ --recursive --only-show-errors || true
  """
  echo "Published to s3://${args.bucket}/${prefix}/"
}
