let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    buildInputs = with pkgs; [
      libcxx
      gcc
      gnumake
      clang
    ];
  }
