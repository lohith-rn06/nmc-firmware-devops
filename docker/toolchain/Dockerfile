# NMC build toolchain image - every Jenkins build runs inside this, so builds are
# identical on a laptop, the Jenkins agent pod and the release pipeline.
# Rebuilt by jenkins/Jenkinsfile.toolchain when this file changes; pinned by digest in pipelines.
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build git ca-certificates curl unzip file xz-utils \
      gcc-arm-linux-gnueabihf libc6-dev-armhf-cross \
      gcc-aarch64-linux-gnu libc6-dev-arm64-cross \
      clang-18 clang-tidy-18 clang-format-18 libclang-rt-18-dev llvm-18 \
      cppcheck flawfinder gcovr python3 python3-pip python3-venv \
      binutils jq openssl snmp \
    && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/bin/clang-18 /usr/local/bin/clang \
    && ln -s /usr/bin/clang-tidy-18 /usr/local/bin/clang-tidy \
    && ln -s /usr/bin/clang-format-18 /usr/local/bin/clang-format

# Python tooling for packaging/tests (pinned)
RUN pip3 install --no-cache-dir --break-system-packages pytest==8.3.3 semgrep==1.90.0

# AWS CLI v2 (for KMS signing + S3 upload from inside the build container)
RUN curl -sSL https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip -o /tmp/aws.zip \
    && unzip -q /tmp/aws.zip -d /tmp && /tmp/aws/install && rm -rf /tmp/aws /tmp/aws.zip

# Non-root build user with the same UID as the Jenkins agent (1000)
RUN userdel -r ubuntu 2>/dev/null || true \
    && useradd -m -u 1000 -s /bin/bash builder
USER builder
WORKDIR /workspace
LABEL org.opencontainers.image.title="nmc-toolchain" \
      org.opencontainers.image.description="Cross-compile + analysis toolchain for NMC firmware"
