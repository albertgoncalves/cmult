with import <nixpkgs> {};
mkShell {
    buildInputs = [
        clang-tools
        shellcheck
    ];
    shellHook = ''
        . .shellhook
    '';
}
