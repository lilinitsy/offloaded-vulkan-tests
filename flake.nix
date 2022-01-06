{
  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        defaultPackage = packages.offloaded-vulkan-tests;
        devShell = pkgs.mkShell {
          inputsFrom = builtins.attrValues packages;
          nativeBuildInputs = [ pkgs.cargo-watch pkgs.crate2nix ];
        };
        packages = {
          obj-loader =
            (import ./obj-loader/Cargo.nix { inherit pkgs; }).rootCrate.build;
          offloaded-vulkan-tests = pkgs.stdenv.mkDerivation {
            name = "offloaded-vulkan-tests";
            src =
              pkgs.nix-gitignore.gitignoreSourcePure [ ./.gitignore "shaders/" ]
              ./src;
            buildInputs = [
              pkgs.coz
              pkgs.glfw
              pkgs.glm
              pkgs.vulkan-loader
              packages.obj-loader.lib
            ];
            nativeBuildInputs = [ pkgs.pkg-config ];
            buildPhase = ''
              for unit in main client; do
                g++ $(pkg-config --cflags glfw3 vulkan) \
                  -std=c++17 -march=native -O3 -I${./obj-loader/include} \
                  -c -o "$unit.o" "$unit.cpp"
              done
              g++ main.o $(pkg-config --libs glfw3 vulkan) -lobj_loader -pthread -o rendertest
              g++ $(pkg-config --libs glfw3 vulkan) -lobj_loader -pthread -o client client.o
            '';
            installPhase = ''
              install -Dt $out/bin rendertest
              install -Dt $out/bin client
              cp -r ${./models} $out/models
              cp -r ${packages.shaders} $out/shaders
              cp -r ${./textures} $out/textures
            '';
          };
          shaders = pkgs.stdenvNoCC.mkDerivation {
            name = "shaders";
            src = ./src/shaders;
            nativeBuildInputs = [ pkgs.shaderc ];
            buildPhase = ''
              mkdir -p $out
              glslc defaultmodelclient.vert -o $out/vertexmodelclient.spv
              glslc defaultmodelclient.frag -o $out/fragmentmodelclient.spv
              glslc defaultfsquadclient.vert -o $out/vertexfsquadclient.spv
              glslc defaultfsquadclient.frag -o $out/fragmentfsquadclient.spv
              glslc defaultserver.vert -o $out/vertexdefaultserver.spv
              glslc defaultserver.frag -o $out/fragmentdefaultserver.spv
            '';
            installPhase = "true";
          };
        };
      });
}
