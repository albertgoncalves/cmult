with import <nixpkgs> {};
mkShell {
    buildInputs = [
        clang-tools
        python3
        shellcheck
    ];
    shellHook = ''
        . .shellhook
    '';
}
