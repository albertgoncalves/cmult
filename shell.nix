with import <nixpkgs> {};
mkShell {
    buildInputs = [
        clang_10
        glibcLocales
        python3
        shellcheck
        valgrind
    ];
    shellHook = ''
        . .shellhook
    '';
}
