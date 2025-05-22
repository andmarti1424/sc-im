{ lib
, stdenv
, fetchFromGitHub
, makeWrapper
, pkg-config
, which
, bison
, gnuplot
, libxls
, libxlsxwriter
, libxml2
, libzip
, ncurses
, xlsSupport ? false
}:

stdenv.mkDerivation rec {
  pname = "sc-im";
  version = "9.9.9";

  src = ./src;

  nativeBuildInputs = [
    makeWrapper
    pkg-config
    which
    bison
  ];

  buildInputs = [
    gnuplot
    libxml2
    libzip
    ncurses
  ] ++ lib.optionals xlsSupport [
    libxls
    libxlsxwriter
  ];

  makeFlags = [ "prefix=${placeholder "out"}" ];

  hardeningDisable = [ "fortify" ];

  env.NIX_CFLAGS_COMPILE = lib.optionalString stdenv.cc.isClang "-Wno-error=implicit-function-declaration";

  postInstall = ''
    wrapProgram "$out/bin/sc-im" --prefix PATH : "${lib.makeBinPath [ gnuplot ]}"
  '';

  meta = with lib; {
    changelog = "https://github.com/andmarti1424/sc-im/blob/${src.rev}/CHANGES";
    homepage = "https://github.com/andmarti1424/sc-im";
    description = "An ncurses spreadsheet program for terminal";
    license = licenses.bsdOriginal;
    maintainers = with maintainers; [ dotlambda ];
    platforms = platforms.unix;
  };
}
