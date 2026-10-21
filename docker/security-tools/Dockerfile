# All security scanners in one pinned image (used as the 'security' container in agent pods).
FROM alpine:3.20
ARG TRIVY=0.56.2 GRYPE=0.82.0 SYFT=1.14.0 GITLEAKS=8.21.1 COSIGN=2.4.1
RUN apk add --no-cache bash curl git jq ca-certificates \
 && curl -sSfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin v${TRIVY} \
 && curl -sSfL https://raw.githubusercontent.com/anchore/grype/main/install.sh | sh -s -- -b /usr/local/bin v${GRYPE} \
 && curl -sSfL https://raw.githubusercontent.com/anchore/syft/main/install.sh | sh -s -- -b /usr/local/bin v${SYFT} \
 && curl -sSfL https://github.com/gitleaks/gitleaks/releases/download/v${GITLEAKS}/gitleaks_${GITLEAKS}_linux_x64.tar.gz | tar xz -C /usr/local/bin gitleaks \
 && curl -sSfL -o /usr/local/bin/cosign https://github.com/sigstore/cosign/releases/download/v${COSIGN}/cosign-linux-amd64 \
 && chmod +x /usr/local/bin/cosign \
 && adduser -D -u 1000 scanner
USER scanner
