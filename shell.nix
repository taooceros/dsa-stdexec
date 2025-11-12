let
  nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/bee1c5a592c2adb705b44666b071b1e558119ab2";
  pkgs = import nixpkgs { config = {}; overlays = []; };
in

pkgs.mkShell {

  packages = with pkgs; [
    gcc15
    glibc
    clang-tools
  ];
  # Use buildInputs to add all your development dependencies.
  # This makes the C++ compiler (g++), C compiler (gcc), and build tool (xmake)
  # available in the shell's PATH and sets up include/library paths.
  buildInputs = with pkgs; [
    gcc15
    glibc.dev
    xmake
    cmake
    ninja
    libuv
    spdlog
    llvm_21
    clang_21
    libtool
    autoconf
    automake
  ];

  # LIBRARY_PATH = "/usr/lib:/usr/local/lib";
}
