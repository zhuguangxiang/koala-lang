_wombat() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local subcommands="new build check run test fmt lint doc add clean help version"

    if [ "$COMP_CWORD" -eq 1 ]; then
        COMPREPLY=( $(compgen -W "$subcommands" -- "$cur") )
        return
    fi

    local cmd="${COMP_WORDS[1]}"
    case "$cmd" in
        build|check|run|test|fmt)
            COMPREPLY=( $(compgen -W "--release --workspace --all-targets -p --package --target -j --jobs --src -o --output" -- "$cur") )
            ;;
        new)
            COMPREPLY=( $(compgen -W "package workspace --lib --path --vcs --edition" -- "$cur") )
            ;;
    esac
}
complete -F _wombat wombat
