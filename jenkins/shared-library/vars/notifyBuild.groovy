// Teams/Slack + e-mail notification. Webhook URL comes from a Jenkins secret-text credential.
def call(Map args) {
  def msg = "NMC firmware ${env.JOB_NAME} #${env.BUILD_NUMBER} (${env.NMC_VERSION ?: '?'}) - ${args.status}: ${env.BUILD_URL}"
  try {
    withCredentials([string(credentialsId: 'teams-webhook-url', variable: 'HOOK')]) {
      writeFile file: '.notify.json', text: groovy.json.JsonOutput.toJson([text: msg])
      sh(script: 'curl -fsS -H "Content-Type: application/json" -d @.notify.json "$HOOK"', label: 'notify Teams')
    }
  } catch (e) {
    echo "notification skipped: ${e.message}"
  }
  if (args.status == 'FAILURE') {
    emailext subject: "FAILED: ${env.JOB_NAME} #${env.BUILD_NUMBER}", body: msg, recipientProviders: [culprits(), developers()]
  }
}
