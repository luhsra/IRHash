################################### Build LLVM Pass ###################################
FROM ubuntu:24.04@sha256:80dd3c3b9c6cecb9f1667e9290b3bc61b78c2678c02cbdae5f0fea92cc6734ab

RUN apt-get update && \
    apt-get -qq install make clang-18 libclang-18-dev llvm-18-dev libxxhash-dev

COPY pass /pass

RUN make -C pass clean && \
    make -C pass LLVM-CONFIG=llvm-config-18 -j $(nproc) pass-skip.so pass-debug.so

################################### Demo Environment ###################################

FROM ubuntu:24.04@sha256:80dd3c3b9c6cecb9f1667e9290b3bc61b78c2678c02cbdae5f0fea92cc6734ab

ENV CC=clang-18
ENV CXX=clang++-18
ENV LLVM_VERSION=18

RUN <<EOF
apt-get update
apt-get -qq install clang-18 vim nano make
rm -rf /var/lib/apt/lists/*
EOF

ENV IRHASH_CACHE=/irhashcache
RUN mkdir $IRHASH_CACHE

COPY --from=0 pass/*.so /
COPY example/*.c example/*.cpp example/*.h example/*.hpp example/Makefile /example/

WORKDIR /example
