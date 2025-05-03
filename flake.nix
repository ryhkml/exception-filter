{
  description = "Throwing HTTP exception in Nginx";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachSystem [ "x86_64-linux" ] (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = {
          exception-filter = pkgs.stdenv.mkDerivation {
            pname = "exception-filter";
            version = "0.0.0-placeholder";
            src = ./.;
            nativeBuildInputs = with pkgs; [
              gcc
            ];
            buildPhase = ''
              gcc -o nob nob.c
              ./nob
            '';
            installPhase = ''
              mkdir -p $out/bin
              cp out/exception-filter $out/bin/
            '';
          };
          default = self.packages.${system}.exception-filter;
        };

        apps.default = {
          type = "app";
          program =
            let
              wrapper = pkgs.writeShellScriptBin "exception-filter-wrapper" ''
                ${self.packages.${system}.exception-filter}/bin/exception-filter "$@"
              '';
            in
            "${wrapper}/bin/exception-filter-wrapper";
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            gcc
          ];
          shellHook = ''
            echo "exception-filter development shell"
            echo "Build: nix build"
          '';
        };
      }
    );
}
