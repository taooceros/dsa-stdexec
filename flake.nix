{
  description = "Development shell with gcc15 and xmake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/bee1c5a592c2adb705b44666b071b1e558119ab2";
    idxd-config = {
      url = "git+https://github.com/intel/idxd-config.git?ref=stable&rev=b78f65f5d342643815124d36a52023fda4a945e3";
      flake = false;
    };
    stdexec = {
      url = "git+https://github.com/NVIDIA/stdexec.git?rev=7bb7b5b6d26e5ccedd88a2d7a131e86b58270108";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      idxd-config,
      stdexec,
      ...
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        system = system;
        config = { };
        overlays = [ ];
      };
      stdenv = pkgs.impureUseNativeOptimizations pkgs.stdenv;
    in
    {

      idxd-config-package = stdenv.mkDerivation {
        pname = "idxd-config";
        version = "unstable";
        src = idxd-config;

        nativeBuildInputs = with pkgs; [
          gcc15
          xmake
          libtool
          autoconf
          automake
          pkg-config
          asciidoc
          which
          xmlto
          libuuid
          json_c
          autoreconfHook
        ];

        patchPhase = ''
          patchShebangs ./git-version
          patchShebangs ./git-version-gen
          patchShebangs ./autogen.sh
          ./autogen.sh
        '';
        configurePhase = ''
          # autoupdate
          ./configure CFLAGS='-g -O2' --disable-docs  --prefix=$out --libdir=$out/lib64
        '';
        buildPhase = ''
          make -j
        '';
        checkPhase = ''
          make check
        '';
        installPhase = ''
          make install
        '';
      };

      stdexec-package = stdenv.mkDerivation {
        pname = "stdexec";
        version = "unstable";
        src = stdexec;

        nativeBuildInputs = with pkgs; [
          pkg-config
        ];

        installPhase = ''
          mkdir -p $out/include
          cp -r include/* $out/include/
        '';
      };

      devShells.${system}.default = pkgs.mkShell {

        NIX_ENFORCE_NO_NATIVE = "0";

        packages = with pkgs; [
          gcc15
          xmake
          llvmPackages.libstdcxxClang
          llvmPackages.clang-tools
          self.idxd-config-package
          cmake
          aria2
          llvmPackages.bintools
          gcc15.cc.lib
          self.stdexec-package
        ];

        nativeBuildInputs = with pkgs; [
          libuuid
          json_c
          libtool
          autoconf
          automake
          pkg-config
          asciidoc
          which
          xmlto
          fmt_12
          cmake
          ninja
          llvmPackages.bintools
          llvmPackages.libstdcxxClang
          llvmPackages.clang-tools
          stdenv
          gcc15.cc.lib
          self.stdexec-package
        ];
      };
    };
}
