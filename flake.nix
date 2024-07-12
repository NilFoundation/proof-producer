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
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.crypto3.follows = "crypto3";
    };
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, crypto3, parallel-crypto3, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        proof-producer = { custom-boost ? null, enableDebug ? false } :
          let
            crypto3-boost = pkgs.boost183; # used for removing original boost from crypto3 packages
            crypto3-with-custom-boost = crypto3.packages.${system}.crypto3.overrideAttrs (oldAttrs: {
              propagatedBuildInputs = [ custom-boost ] ++ (nixpkgs.lib.filter (input: input != crypto3-boost) oldAttrs.propagatedBuildInputs);
            });
            parallel-crypto3-with-custom-boost = parallel-crypto3.packages.${system}.default.overrideAttrs (oldAttrs: {
              propagatedBuildInputs = [ custom-boost ] ++ (nixpkgs.lib.filter (input: input != crypto3-boost) oldAttrs.propagatedBuildInputs);
            });
          in
            pkgs.stdenv.mkDerivation {
              name = "proof-producer";
              src = self;
              dontStrip = enableDebug;  # Do not strip debug symbols

              buildInputs = with pkgs; [
                cmake
                ninja
                pkg-config
                (if custom-boost == null then crypto3.packages.${system}.crypto3 else crypto3-with-custom-boost)
                (if custom-boost == null then parallel-crypto3.packages.${system}.default else parallel-crypto3-with-custom-boost)
              ];

              # HACK: nix put `-isystem /path/to/include` for each include dir from inputs.
              #   This leads to CMake not adding `-i` options to build command. And this
              #   beahvior doesn't let us to replace crypto3 with parallel-crypto3, since
              #   their order is set by nix. Just remove every `-isystem ...` arg from this env.
              #   Related ticket: https://github.com/NixOS/nixpkgs/issues/79303
              preConfigure = ''
                export NIX_CFLAGS_COMPILE=$(echo "$NIX_CFLAGS_COMPILE" | sed -e 's|-isystem [^ ]*||g')
              '';

              cmakeFlags = [
                "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
                (if enableDebug then "-DCMAKE_BUILD_TYPE=Debug" else "-DCMAKE_BUILD_TYPE=Release")
                (if enableDebug then "-DZK_PLACEHOLDER_DEBUG_ENABLED=1 -DCMAKE_CXX_FLAGS=-ggdb -DCMAKE_CXX_FLAGS=-O0" else "")
              ];

              doCheck = false; # tests are inside crypto3-tests derivation

              installPhase = ''
                cmake --build . --target install
              '';
            };
      in {
        packages.default = proof-producer{};
        apps.single-threaded = {
          type = "app";
          program = "${proof-producer{}}/bin/proof-producer-single-threaded";
        };
        apps.default = {
          type = "app";
          program = "${proof-producer{}}/bin/proof-producer-multi-threaded";
        };
      }
    );
}
