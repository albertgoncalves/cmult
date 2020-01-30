with import <nixpkgs> {};
let
    shared = [
        clang_9
        python3
        shellcheck
    ];
    hook = ''
        . .shellhook
    '';
in
{
    darwin = stdenvNoCC.mkDerivation {
        name = "_";
        buildInputs = shared;
        shellHook = hook;
    };
    linux = stdenvNoCC.mkDerivation {
        name = "_";
        buildInputs = [
            valgrind
        ] ++ shared;
        shellHook = hook;
    };
}
