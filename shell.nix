with import <nixpkgs> {};
mkShell {
    buildInputs = [
        clang_10
        cppcheck
        glibcLocales
        python3
        shellcheck
        valgrind
    ];
    shellHook = ''
        . .shellhook
    '';
}
