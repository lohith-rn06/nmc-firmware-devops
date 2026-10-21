// Install the four simulators into an ephemeral namespace with Helm.
def call(Map args) {
  container('deploy') {
    sh """
      kubectl create ns ${args.namespace} --dry-run=client -o yaml | kubectl apply -f -
      kubectl label ns ${args.namespace} nmc.io/ephemeral=true --overwrite
      kubectl annotate ns ${args.namespace} nmc.io/branch=${env.BRANCH_NAME} --overwrite
      kubectl -n ${args.namespace} create secret generic nmc-sim-credentials \
        --from-literal=admin-password=\$(openssl rand -base64 18) \
        --from-literal=snmp-community=ci-\$(openssl rand -hex 6) --dry-run=client -o yaml | kubectl apply -f -
    """
    args.variants.each { v ->
      def name = "nmc-${v.toLowerCase()}"
      sh """
        helm upgrade --install ${name} helm/nmc-simulator -n ${args.namespace} --wait --timeout 5m \
          --set variant=${v} --set image.repository=${env.ECR_REGISTRY}/nmc-simulator-${v.toLowerCase()} \
          --set image.tag=${args.tag} --set simulation=true \
          --set pcnsClients=nmc-itest-${v.toLowerCase()}.${args.namespace}.svc.cluster.local:3052
      """
    }
  }
}
