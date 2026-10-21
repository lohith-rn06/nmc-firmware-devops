# NMC simulator image: the real firmware compiled for x86_64/arm64 Linux so it can run
# as a pod. One image per card variant: --build-arg VARIANT=GVS|GVL|GVXL|GVXL-NAM
ARG TOOLCHAIN_IMAGE=nmc-toolchain:local
FROM ${TOOLCHAIN_IMAGE} AS build
ARG VARIANT=GVS
ARG VERSION=0.0.0-dev
ARG GIT_SHA=unknown
COPY --chown=builder:builder VERSION /workspace/VERSION
COPY --chown=builder:builder firmware /workspace/firmware
RUN cmake -S firmware -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DNMC_VARIANT=${VARIANT} \
          -DNMC_VERSION=${VERSION} -DNMC_GIT_SHA=${GIT_SHA} -DNMC_BUILD_TESTS=OFF \
 && cmake --build build \
 && cmake --install build --prefix /workspace/install

# Runtime: same Ubuntu base as the toolchain so the glibc versions match (a Debian 12
# distroless image would fail with "GLIBC_2.38 not found"). Nothing but the firmware is added.
FROM ubuntu:24.04
RUN userdel -r ubuntu 2>/dev/null; useradd -r -u 10001 -s /usr/sbin/nologin nmc
ARG VARIANT=GVS
ARG VERSION=0.0.0-dev
COPY --from=build /workspace/install /opt/nmc
ENV NMC_SNMP_PORT=1161 NMC_MODBUS_PORT=1502 NMC_HTTP_PORT=8080
EXPOSE 1161/udp 1502/tcp 8080/tcp
USER 10001
ENTRYPOINT ["/opt/nmc/bin/nmc-fw"]
LABEL org.opencontainers.image.title="nmc-simulator-${VARIANT}" \
      org.opencontainers.image.version="${VERSION}" \
      com.nmc.variant="${VARIANT}"
