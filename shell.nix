with import <nixpkgs> {};
let
    shared = [
        clang-tools
        python3
        shellcheck
    ];
    hook = ''
        . .shellhook
    '';
in
{
    darwin = mkShell {
        buildInputs = shared;
        shellHook = hook;
    };
    linux = mkShell {
        buildInputs = [
        valgrind
        ] ++ shared;
        shellHook = hook;
    };
}
