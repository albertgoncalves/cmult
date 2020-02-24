with import <nixpkgs> {};
let
    shared = [
        python3
        shellcheck
    ];
    hook = ''
        . .shellhook
    '';
in
{
    darwin = llvmPackages_9.stdenv.mkDerivation {
        name = "_";
        buildInputs = shared;
        shellHook = hook;
    };
    linux = llvmPackages_9.stdenv.mkDerivation {
        name = "_";
        buildInputs = [
            valgrind
        ] ++ shared;
        shellHook = hook;
    };
}
