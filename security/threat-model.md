# NMC threat model (STRIDE)

**Assets:** UPS control & telemetry, admin credentials, firmware image, PCNS shutdown signal (availability of protected servers).
**Trust boundaries:** management LAN ↔ card, card ↔ UPS internal bus, build system ↔ release bucket ↔ field update.

| Threat | Example | Mitigation in this repo |
|---|---|---|
| **S**poofing | Attacker logs in with factory default password | No default credentials; random one-time password (`config.c`); lockout after 5 failures (`auth.c`) |
| **S**poofing | Malicious firmware pushed to card | ECDSA signature with KMS key + variant/sha256 check on card (`fw_update.c`, `sign-firmware.sh`) |
| **T**ampering | Modbus write turns UPS to bypass | Modbus writes disabled by default (`modbus_tcp.c`) |
| **T**ampering | Build server compromised injects code | Ephemeral agent pods, pinned toolchain digest, reproducible build check, SBOM |
| **R**epudiation | Admin denies changing config | Audit log for logins, simulations, denied writes (`audit_log.c`) |
| **I**nformation disclosure | `/api/config` leaks hash/community | Secrets never serialized; integration test `test_config_never_leaks_secrets` |
| **I**nformation disclosure | SNMP v1/v2c community sniffed | Read-only community; SNMPv3 USM on roadmap (ADR-0004) |
| **D**enial of service | Malformed SNMP/Modbus packet crashes agent | Strict BER/MBAP bounds checks, fuzzing nightly, ASan/UBSan builds |
| **D**enial of service | Fake "on battery" event shuts down servers | PCNS only to configured clients; simulate API disabled in production builds |
| **E**levation of privilege | Buffer overflow → code execution | Banned functions (semgrep), `-fstack-protector-strong`, PIE, full RELRO, NX (`checksec.sh`) |
