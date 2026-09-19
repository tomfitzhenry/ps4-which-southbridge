{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  nativeBuildInputs = [
    pkgs.gcc
    pkgs.binutils
    pkgs.gnumake
  ];
}
