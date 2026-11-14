{
  description = "onedse - quantum mechanics in phase space: Wigner functions, open systems, and beyond";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});

      mkOnedse = pkgs: pkgs.stdenv.mkDerivation {
        pname = "onedse";
        version = "2.0.0";

        src = pkgs.lib.fileset.toSource {
          root = ./.;
          fileset = pkgs.lib.fileset.unions [ ./cpp ./lua ./web ./python ];
        };

        nativeBuildInputs = [ pkgs.cmake ];
        buildInputs = with pkgs; [ boost eigen fftw lua5_4 nlohmann_json httplib openssl gtest ];

        cmakeDir = "../cpp";
        cmakeFlags = [ "-DONEDSE_BUILD_TESTS=ON" ];

        # Unit tests plus a headless run of every Lua example
        doCheck = true;
        checkPhase = ''
          runHook preCheck
          ctest --output-on-failure -j $NIX_BUILD_CORES
          runHook postCheck
        '';

        meta = {
          description = "Quantum mechanics in phase space, scriptable in Lua, visualised in the browser";
          mainProgram = "onedse_server";
          platforms = systems;
        };
      };
    in
    {
      packages = forAllSystems (pkgs: rec {
        onedse = mkOnedse pkgs;
        default = onedse;
      });

      apps = forAllSystems (pkgs:
        let pkg = self.packages.${pkgs.stdenv.hostPlatform.system}.onedse; in rec {
          # nix run            -> web UI on http://localhost:8080
          server = { type = "app"; program = "${pkg}/bin/onedse_server"; };
          # nix run .#script -- lua/tunnelling.lua
          script = { type = "app"; program = "${pkg}/bin/onedse"; };
          default = server;
        });

      checks = forAllSystems (pkgs: {
        onedse = self.packages.${pkgs.stdenv.hostPlatform.system}.onedse;
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.onedse ];
          packages = [
            (pkgs.python3.withPackages (ps: [ ps.numpy ps.matplotlib ]))
          ];
          shellHook = ''
            echo "onedse dev shell:"
            echo "  cmake -S cpp -B build && cmake --build build -j"
            echo "  ctest --test-dir build --output-on-failure"
            echo "  ./build/main/server/onedse_server"
          '';
        };
      });
    };
}
