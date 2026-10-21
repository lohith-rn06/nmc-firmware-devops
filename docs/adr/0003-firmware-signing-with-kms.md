# ADR 0003: Sign firmware with AWS KMS asymmetric keys

**Status:** Accepted

## Decision
Private keys never leave the HSM. Dev and prod keys are separate; only the `nmc-release-signer` role (assumed by the release pipeline) can use the prod key. Every sign operation is logged in CloudTrail.

## Consequences
Write down what got easier and harder after you practise this part.
