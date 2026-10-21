# ADR 0002: One codebase and one pipeline for all card variants

**Status:** Accepted

## Decision
Variants differ only by compile-time configuration (`firmware/variants/*.cmake`). The Jenkins `matrix` builds all four in parallel, so a change can never silently break one card.

## Consequences
Write down what got easier and harder after you practise this part.
