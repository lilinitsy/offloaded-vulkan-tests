{ pkgs ? import <nixpkgs> { } }:

pkgs.stdenv.mkDerivation {
  name = "offloaded-vulkan-tests";
  src = pkgs.nix-gitignore.gitignoreSourcePure [ ./.gitignore "*.spv" ] ./.;
  buildInputs = with pkgs; [ coz glfw glm pkg-config shaderc vulkan-loader ];
  buildPhase = ''
    pushd src
    make -f Makefile_nix
    popd
  '';
  installPhase = ''
    install -Dt $out/bin src/client src/rendertest
    install -Dt $out/bin/shaders -m0644 \
      src/shaders/vertexdefaultserver.spv \
      src/shaders/fragmentdefaultserver.spv \
      src/shaders/vertexmodelclient.spv \
      src/shaders/fragmentmodelclient.spv \
      src/shaders/vertexfsquadclient.spv \
      src/shaders/fragmentfsquadclient.spv
    cp -r models $out/models
    cp -r textures $out/textures
  '';
}
