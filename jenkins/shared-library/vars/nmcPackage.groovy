def call(Map args) {
  sh "scripts/package.sh ${args.variant}"
  stash name: "fw-${args.variant}", includes: "dist/nmc-${args.variant}-*"
}
