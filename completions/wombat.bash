# Return subdirectories of the current directory that contain
# `__ws__.toml` or `__pkg__.toml`. Only these are valid targets for
# `wombat build <dir>` and the other `[dir]` commands.
_wombat_ws_dirs() {
    local d
    for d in */; do
        [ -d "$d" ] || continue
        if [ -f "$d/__ws__.toml" ] || [ -f "$d/__pkg__.toml" ]; then
            printf '%s\n' "${d%/}"
        fi
    done
}

_wombat() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local subcommands="new build check run test fmt doc clean help version"

    # `wombat <TAB>`: complete the subcommand name.
    if [ "$COMP_CWORD" -eq 1 ]; then
        COMPREPLY=( $(compgen -W "$subcommands" -- "$cur") )
        return
    fi

    local cmd="${COMP_WORDS[1]}"
    case "$cmd" in
        build)
            # `wombat build [dir] [-o DIR]`
            if [[ "$cur" == -* ]]; then
                COMPREPLY=( $(compgen -W "-o --output" -- "$cur") )
            else
                # Only directories that look like a workspace or package.
                COMPREPLY=( $(compgen -W "$(_wombat_ws_dirs)" -- "$cur") )
            fi
            ;;
        check|run|test|fmt|doc|clean)
            # `wombat <cmd> [dir]`: only workspace/package directories.
            if [[ "$cur" == -* ]]; then
                COMPREPLY=()
                compopt +o default
            else
                COMPREPLY=( $(compgen -W "$(_wombat_ws_dirs)" -- "$cur") )
            fi
            ;;
        new)
            # `wombat new <name>`: no candidates, no filename fallback.
            COMPREPLY=()
            compopt +o default
            ;;
        help)
            # `wombat help <command>`: complete subcommand names.
            COMPREPLY=( $(compgen -W "$subcommands" -- "$cur") )
            ;;
        version)
            COMPREPLY=()
            compopt +o default
            ;;
    esac
}
complete -F _wombat wombat
