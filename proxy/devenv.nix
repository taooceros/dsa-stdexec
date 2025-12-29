{
  pkgs,
  lib,
  config,
  inputs,
  ...
}:
let
  proxy-package = pkgs.stdenv.mkDerivation {
    pname = "proxy";
    version = "unstable";
    src = inputs.proxy;
    installPhase = ''
      mkdir -p $out/include
      cp -r $src/include $out/include
    '';

  };
in
{
  packages = with pkgs; [
    proxy-package
  ];
}
