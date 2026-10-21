// One entry point per SAST tool so the Jenkinsfile stays readable.
def call(Map args) {
  sh 'mkdir -p reports'
  switch (args.tool) {
    case 'cppcheck':
      sh '''cppcheck --enable=warning,style,performance,portability --std=c11 --inline-suppr \
              --suppressions-list=security/cppcheck-suppressions.txt --error-exitcode=0 \
              -I firmware/include --xml firmware/src 2> reports/cppcheck.xml'''
      break
    case 'clang-tidy':
      sh '''cmake -S firmware -B build/tidy -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DNMC_BUILD_TESTS=OFF >/dev/null
            find firmware/src -name '*.c' | xargs clang-tidy -p build/tidy --config-file=firmware/.clang-tidy | tee reports/clang-tidy.log'''
      break
    case 'semgrep':
      sh 'semgrep --config security/semgrep --sarif -o reports/semgrep.sarif --error firmware/src'
      break
    case 'flawfinder':
      sh 'flawfinder --minlevel=3 --columns --context firmware/src | tee reports/flawfinder.txt'
      break
    default:
      error "unknown SAST tool ${args.tool}"
  }
}
