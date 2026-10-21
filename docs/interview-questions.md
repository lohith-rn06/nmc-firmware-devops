# Interview questions this project prepares you for

1. How do you build firmware for several hardware variants from one codebase without copying pipelines?
2. Why build inside a Docker toolchain image? How do you keep it reproducible (pinning, digests)?
3. Kaniko vs Docker-in-Docker on Kubernetes — why avoid privileged pods?
4. How are Jenkins agents secured on EKS (IRSA, namespaces, quotas, taints, no controller executors)?
5. Explain firmware signing end to end. Where is the private key? Who can use the prod key? How does the card verify?
6. What is an SBOM and how do you act on a new CVE in an already-shipped firmware?
7. SAST vs SCA vs DAST vs fuzzing — what does each find in an embedded product?
8. How do you test SNMP/Modbus without hardware? When do you still need HIL?
9. What does IEC 62443-4-1 require from a DevOps pipeline?
10. A release build is not bit-for-bit reproducible — how do you debug it?
11. How would you speed up a 60-minute firmware pipeline? (caching, matrix, ccache, spot nodes, test sharding)
12. How do you make the release bucket tamper-proof?
