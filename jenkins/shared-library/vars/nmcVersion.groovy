// Returns the SemVer for this build (see scripts/version.sh for the rules).
def call() {
  return sh(script: 'scripts/version.sh', returnStdout: true).trim()
}
