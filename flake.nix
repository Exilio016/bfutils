{
  description = "An over-engineered Hello World in C";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-24.05";

  outputs =
    { self, nixpkgs }:
    let

      # to work with older version of flakes
      lastModifiedDate = self.lastModifiedDate or self.lastModified or "19700101";

      # Generate a user-friendly version number.
      version = builtins.substring 0 8 lastModifiedDate;

      # System types to support.
      supportedSystems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });

    in

    {
      # Provide some binary packages for selected system types.
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.stdenv.mkDerivation {
            inherit version;
            name = "bfutils";
            src = ./.;
            buildInputs = with pkgs; [
              gcc
              pkg-config
              openssl
              ninja
            ];
            buildPhase = # bash
              ''
                gcc -o build build.c
                ./build
              '';
            installPhase = # bash
              ''
                mkdir -p $out/bin
                cp target/bin/bfutils $out/bin/
              '';
          };
        }
      );

      defaultPackage = forAllSystems (system: self.packages.${system}.default);

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              ninja
              pkg-config
              valgrind
              clang-tools
              openssl
              bear
              gdb
              gcovr
            ];
          };
        }
      );

    };
}
