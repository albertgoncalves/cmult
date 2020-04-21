with import <nixpkgs> {};
mkShell {
    buildInputs = [
        clang_10
        python3
        shellcheck
        valgrind
    ];
    shellHook = ''
        . .shellhook
    '';
}
