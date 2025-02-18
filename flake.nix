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
              gnumake
            ];
            buildPhase = ''
              make
            '';
            installPhase = ''
              mkdir -p $out/bin
              cp out/exception-filter $out/bin/
            '';
          };
        };

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/exception-filter";
        };

        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            gcc
            gnumake
          ];
          shellHook = ''
            echo "exception-filter development shell"
            echo "Build: nix build"
          '';
        };
      }
    );
}
