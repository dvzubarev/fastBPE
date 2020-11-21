{
  pkgs ? import <textapp-pkgs> {},
  version ? "dev"
}:
with pkgs;
stdenv.mkDerivation rec {
  name = "fast-bpe-${version}";
  inherit version;
  src = if lib.inNixShell then null else ./.;

  nativeBuildInputs = [ cmake ];
  buildInputs = [
    boost
  ];
}
