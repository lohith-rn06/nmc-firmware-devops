# Changelog (Keep a Changelog / SemVer)

## [1.4.0] - unreleased
### Added
- PCNS notifications with PREPARE_SHUTDOWN / SHUTDOWN_NOW actions
- GVXL-NAM variant (480 V / 60 Hz, FIPS mode flag)
- Firmware image format v1 with variant lock and SHA-256 integrity
### Security
- No factory default password; account lockout after 5 failures
- Modbus TCP writes disabled by default
