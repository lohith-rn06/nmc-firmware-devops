# Architecture

## Firmware
```
                ┌──────────────────────────── NMC card ─────────────────────────────┐
 Mgmt LAN ──►   │  TCP/IP (dual stack)                                              │
                │   ├─ SNMP agent  udp/161   (BER codec → MIB table → UPS model)     │
                │   ├─ Modbus TCP  tcp/502   (MBAP → register map → UPS model)       │
                │   ├─ HTTP(S)/REST tcp/443  (auth → API → UPS model / config)       │
                │   └─ PCNS notifier ──────► servers running PowerChute (tcp/3052)    │
                │  Security: auth+lockout · audit log · secure defaults              │
                │  OTA: header + variant lock + SHA-256 (+ signature, verified first) │
                └───────────────▲───────────────────────────────────────────────────┘
                                │ internal bus (simulated by ups_model.c)
                           3-phase UPS
```
Variant differences are **compile-time** (`firmware/variants/*.cmake`): CPU target, grid nominal values,
rating and feature flags. One codebase → four images.

## CI/CD platform on AWS
```
 GitHub ──webhook──► Jenkins controller (EKS, system pool, EBS gp3, JCasC)
                         │ creates agent pods (jenkins-agents ns, spot build pool, IRSA)
                         ▼
     ┌──────── agent pod: toolchain · security · kaniko · sonar · zap · deploy ────────┐
     │ build/test/package ─► KMS sign ─► Kaniko ─► ECR ─► Trivy                         │
     │ helm install sims ─► nmc-ci-<branch> ns ─► pytest Job ─► JUnit                   │
     │ aws s3 cp ─► s3://nmc-firmware-builds (90 d)                                      │
     └──────────────────────────────────────────────────────────────────────────────────┘
 Release pipeline ─► assume nmc-release-signer ─► KMS prod key ─► s3://nmc-firmware-releases (Object Lock)
 VPC endpoints for S3/ECR/KMS/STS/Secrets Manager keep build traffic off the internet.
```
