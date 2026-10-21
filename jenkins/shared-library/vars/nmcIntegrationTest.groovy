// Run the pytest suite as a Kubernetes Job per variant, then copy the JUnit XML back.
def call(Map args) {
  container('deploy') {
    args.variants.each { v ->
      def lv = v.toLowerCase()
      sh """
        sed -e 's/__VARIANT__/${v}/g' -e 's/__LVARIANT__/${lv}/g' -e 's|__IMAGE__|${env.ECR_REGISTRY}/nmc-test-runner:latest|' \
            k8s/test-jobs/integration-test-job.yaml | kubectl -n ${args.namespace} apply -f -
        kubectl -n ${args.namespace} wait --for=condition=complete --timeout=10m job/nmc-itest-${lv} || \
          { kubectl -n ${args.namespace} logs job/nmc-itest-${lv}; exit 1; }
        kubectl -n ${args.namespace} logs job/nmc-itest-${lv} | sed -n '/<?xml/,\$p' > reports/integration-${v}.xml || true
      """
    }
  }
}
