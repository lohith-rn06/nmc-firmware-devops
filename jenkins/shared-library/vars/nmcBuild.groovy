// nmcBuild variant: 'GVS', mode: 'host'|'cross'|'coverage'|'sanitize'
def call(Map args) {
  def v = args.variant
  def flag = [host: '', cross: '--cross', coverage: '--coverage', sanitize: '--sanitize'][args.mode ?: 'host']
  if (flag == null) { error "unknown build mode ${args.mode}" }
  echo "Building ${v} (${com.nmc.Variants.arch(v)}) mode=${args.mode}"
  sh "scripts/build.sh ${v} ${flag}"
  if (args.mode == 'cross') {
    // prove we really produced a binary for the card CPU, not the build host
    sh "file build/${v}-${com.nmc.Variants.arch(v)}/nmc-fw | grep -q '${com.nmc.Variants.elfMachine(v)}'"
  }
}
