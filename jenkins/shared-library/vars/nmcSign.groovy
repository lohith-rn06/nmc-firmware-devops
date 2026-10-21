// Sign every .nmcfw in dist/ with an AWS KMS asymmetric key (ECDSA P-256).
// The private key never leaves KMS; Jenkins only gets kms:Sign via IRSA.
def call(Map args) {
  def cmd = "security/sign-firmware.sh kms ${args.keyAlias}"
  if (args.roleArn) {
    withAWS(role: args.roleArn, roleSessionName: "nmc-sign-${env.BUILD_NUMBER}", region: env.AWS_REGION) { sh cmd }
  } else {
    sh cmd
  }
  sh 'ls -l dist/*.sig'
}
