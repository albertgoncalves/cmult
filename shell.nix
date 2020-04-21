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
    darwin = llvmPackages_10.stdenv.mkDerivation {
        name = "_";
        buildInputs = shared;
        shellHook = hook;
    };
    linux = mkShell {
        buildInputs = [
            clang_10
            valgrind
        ] ++ shared;
        shellHook = hook;
    };
}
