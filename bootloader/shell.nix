let
    pkgs = import <nixpkgs> {};
    cross = pkgs.pkgsCross.i686-embedded;
in
pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [
        cmake
        nasm
        cross.buildPackages.binutils
        cross.buildPackages.gcc
    ];
}