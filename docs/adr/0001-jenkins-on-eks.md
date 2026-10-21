# ADR 0001: Run Jenkins on EKS with ephemeral Kubernetes agent pods

**Status:** Accepted

## Decision
Build capacity scales with demand (spot pool scales to zero at night); every build starts from a clean pod; config lives in git (JCasC + Job DSL). The controller is the only stateful piece (EBS gp3, daily AWS Backup).

## Consequences
Write down what got easier and harder after you practise this part.
