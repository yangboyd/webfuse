ARG REGISTRY_PREFIX=''
ARG CODENAME=3.9

FROM ${REGISTRY_PREFIX}arm32v7/alpine:${CODENAME} as builder

ARG QEMU_VERSION_=v3.1.0-2

COPY docker/qemu-arm-static-$QEMU_VERSION_ /usr/bin/qemu-arm-static

RUN set -x \
  && apk add --no-cache \
       bash \
       coreutils \
       gcc \
       g++ \
       make \
       cmake \
       ninja \
       pkgconf \
       rsync \
       gdb \
       valgrind \
       util-linux \
       util-linux-dev

COPY src /usr/local/src

ARG PARALLELMFLAGS=-j2

ARG DUMB_INIT_VERSION=1.2.2

RUN set -x \
  && builddeps="vim" \
  && apk add --no-cache --virtual .build-deps $builddeps \
  && builddir="/tmp/out" \
  && mkdir -p "$builddir" \
  && cd "$builddir" \
  && cp -R "/usr/local/src/dumb-init-$DUMB_INIT_VERSION" . \
  && cd "dumb-init-$DUMB_INIT_VERSION" \
  && make "$PARALLELMFLAGS" \
  && chmod +x dumb-init \
  && mv dumb-init /usr/local/bin/dumb-init \
  && dumb-init --version \
  && rm -rf "$builddir" \
  && apk del .build-deps

ARG GTEST_VERSION=1.8.1

RUN set -x \
  && builddir="/tmp/out" \
  && mkdir -p "$builddir" \
  && cd "$builddir" \
  && cmake "/usr/local/src/googletest-release-$GTEST_VERSION" \
  && make "$PARALLELMFLAGS" install \
  && rm -rf "$builddir"

ARG FUSE_VERSION=3.1.1

RUN set -x \
  && builddeps="libtool automake autoconf gettext-dev m4 linux-headers" \
  && apk add --no-cache --virtual .build-deps $builddeps \
  && cd "/usr/local/src/libfuse-fuse-$FUSE_VERSION" \
  && ./makeconf.sh \
  && builddir="/tmp/out" \
  && mkdir -p "$builddir" \
  && cd "$builddir" \
  && "/usr/local/src/libfuse-fuse-$FUSE_VERSION/configure" \
  && make "$PARALLELMFLAGS" install \
  && rm -rf "$builddir" \
  && apk del .build-deps

ARG WEBSOCKETS_VERSION=3.1.0

RUN set -x \
  && apk add --no-cache \
       ca-certificates \
       openssl \
       openssl-dev \
  && builddir="/tmp/out" \
  && mkdir -p "$builddir" \
  && cd "$builddir" \
  && cmake "/usr/local/src/libwebsockets-$WEBSOCKETS_VERSION" \
  && make "$PARALLELMFLAGS" install \
  && rm -rf "$builddir"

ARG JANSSON_VERSION=2.12

RUN set -x \
  && builddir="/tmp/out" \
  && mkdir -p "$builddir" \
  && cd "$builddir" \
  && cmake -DJANSSON_BUILD_DOCS=OFF "/usr/local/src/jansson-$JANSSON_VERSION" \
  && make "$PARALLELMFLAGS" install \
  && rm -rf "$builddir"

ENV LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"
ENV PKG_CONFIG_PATH=/usr/local/lib32/pkgconfig

ARG USERID=1000

ARG PROJECTDIR=/workspace/src
ARG OUTDIR=/workspace/out
ARG SCRIPTDIR=/workspace/bin

RUN set -x \
  && adduser -u "$USERID" -s /bin/bash -D user \
  && mkdir -p "$PROJECTDIR" "$OUTDIR" "$SCRIPTDIR" \
  && chown user:user "$PROJECTDIR" "$OUTDIR" "$SCRIPTDIR"

WORKDIR "$OUTDIR"

ENTRYPOINT ["dumb-init", "--"]
