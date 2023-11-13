FROM ghcr.io/nilfoundation/proof-market-toolchain:base

WORKDIR /opt/nil-toolchain

COPY . /opt/nil-toolchain

RUN mkdir -p build \
    && cd ./build \
    && cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/usr/bin/clang-12 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-12 .. \
    && cmake --build . \
    && ln -s /opt/nil-toolchain/build/bin/proof-generator/proof-generator /usr/bin/proof-generator \
    && mkdir /root/.config \
    && touch /root/.config/config.ini

ARG ZKLLVM_VERSION=0.1.7

RUN DEBIAN_FRONTEND=noninteractive \
    echo 'deb [trusted=yes]  http://deb.nil.foundation/ubuntu/ all main' >> /etc/apt/sources.list \
    && apt-get update \
    && apt-get -y --no-install-recommends --no-install-suggests install \
      build-essential \
      cmake \
      git \
      zkllvm=${ZKLLVM_VERSION} \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
