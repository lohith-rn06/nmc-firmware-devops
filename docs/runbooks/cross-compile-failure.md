# Runbook: cross-compile stage fails but host build passes
1. Look for `-Werror` warnings that only appear on 32-bit ARM (GVS): `size_t` vs `long`, `%zu` formats, `-Wconversion`.
2. Reproduce locally: `make shell` then `scripts/build.sh GVS --cross`.
3. `file build/GVS-armhf/nmc-fw` must say `ARM, EABI5`. If it says x86-64 the toolchain file was not applied.
4. Missing headers → `libc6-dev-armhf-cross` missing in the toolchain image; rebuild it via `Jenkinsfile.toolchain`.
