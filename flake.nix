{
  description = "Flake for building nil; Proof Producer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
    crypto3 = {
      url = "git+https://github.com/NilFoundation/crypto3";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    parallel-crypto3 = {
      url = "git+https://github.com/NilFoundation/parallel-crypto3?submodules=1";
      # inputs.nixpkgs.follows = "nixpkgs";
      # inputs.crypto3.follows = "crypto3";
    };
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, crypto3, parallel-crypto3, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        proof-producer = pkgs.stdenv.mkDerivation {
          name = "proof-producer";
          src = self;

          buildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            crypto3.packages.${system}.default
            parallel-crypto3.packages.${system}.default
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
          ];

          doCheck = false; # tests are inside crypto3-tests derivation

          installPhase = ''
            cmake --build . --target install
          '';
        };
      in {
        packages.default = proof-producer;
        apps.default = {
          type = "app";
          program = "${proof-producer}/bin/proof-producer-multi-threaded";
        };
        apps.single-threaded = {
          type = "app";
          program = "${proof-producer}/bin/proof-producer-single-threaded";
        };
      }
    );
}
