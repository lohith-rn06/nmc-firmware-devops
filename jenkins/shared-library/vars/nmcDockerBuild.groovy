// Build + push one simulator image per variant with Kaniko (no Docker daemon, no privileged pod).
def call(Map args) {
  container('kaniko') {
    args.variants.each { v ->
      def repo = "${env.ECR_REGISTRY}/nmc-simulator-${v.toLowerCase()}"
      sh """
        /kaniko/executor --context \$WORKSPACE --dockerfile docker/simulator/Dockerfile \
          --build-arg VARIANT=${v} --build-arg VERSION=${env.NMC_VERSION} --build-arg GIT_SHA=\${GIT_COMMIT:0:7} \
          --build-arg TOOLCHAIN_IMAGE=${env.ECR_REGISTRY}/nmc-toolchain:latest \
          --destination ${repo}:${args.tag} --cache=true --cache-repo=${env.ECR_REGISTRY}/kaniko-cache \
          --label org.opencontainers.image.revision=\$GIT_COMMIT
      """
    }
  }
}
