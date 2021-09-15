{ pkgs ? import <nixpkgs> { } }:

pkgs.stdenv.mkDerivation {
  name = "offloaded-vulkan-tests";
  src = pkgs.nix-gitignore.gitignoreSourcePure [ ../.gitignore ] ./.;
  buildInputs = with pkgs; [ coz glfw glm pkg-config vulkan-loader ];
  makeFlags = [ "-f" "Makefile_nix" ];
  installPhase = "install -Dt $out client rendertest";
}
