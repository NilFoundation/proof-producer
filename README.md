# proof-producer
Proof producer for the =nil; Proof Market https://proof.market.nil.foundation/
# How to use
It is needed to have [zkllvm](https://raw.githubusercontent.com/NilFoundation/zkllvm).

To generate proof:

1. Run zkllv assigner to prepare assignment table and circuit
```
${ZKLLVM_BUILD:-build}/bin/assigner/assigner -b ${ZKLLVM_BUILD:-build}/examples/cpp/merkle_tree_sha2_256_cpp_example.ll -i ${ZKLLVM_BUILD:-build}/examples/inputs/merkle_tree_sha2_256.inp -t merkle_tree_sha2_256_assignment.tbl -c merkle_tree_sha2_256_circuit.crct -e pallas
```
2. Run proof generator
```
./bin/proof-generator/proof-generator --circuit=./merkle_tree_sha2_256_circuit.crt --assignment-table=/balances_tree.tbl --proof=./proof.txt
```

# Building
1. Install dependencies:
    ```
    sudo apt-get install \
        build-essential \
        liblz4-dev \
        libgnutls28-dev \
        libyaml-cpp-dev \
        libsctp-dev \
        ragel \
        xfslibs-dev \
        systemtap-sdt-dev \
        libc-ares-dev \
        libhwloc-dev
        libssl-dev \
        libicu-dev \
        libprotobuf-dev \
        lsb-release \
        gnutls-dev \
        pkg-config \
        lksctp-tools \
        numactl
    ```

    Install fmt-6
    ```
    wget http://launchpadlibrarian.net/466750967/libfmt-dev_6.1.2+ds-2_amd64.deb
    sudo apt remove libfmt-dev
    sudo apt install ./libfmt-dev_6.1.2+ds-2_amd64.deb
    ```

2. Build with CMake:
    ```mkdir build
    cd build
    cmake ..
    make -j $(nrpoc)
    ```

# Releases

## DEB packages

Building DEB packages is tested on every CI run.
This way developers can be sure that their code can be built to a package
before they merge it.

To make a DEB release:

1.  Update the version in the `VERSION` file, for example, `v0.42.0`.
2.  Push this commit to the `master` branch.
3.  Push a tag on this commit, named same as the version:

    ```
    git tag v0.42.0
    git push origin v0.42.0
    ```

4.  Wait for CI to build and publish the package.

## Toolchain Docker image

Build an image:

```bash
docker build -t ghcr.io/nilfoundation/toolchain:0.1.7 -t ghcr.io/nilfoundation/toolchain:latest .
```

Push it to the registry:

```bash
docker login ghcr.io
docker push ghcr.io/nilfoundation/toolchain:0.1.7
docker push ghcr.io/nilfoundation/toolchain:latest
```
