# Return subdirectories of the current directory that contain
# `__ws__.toml` or `__pkg__.toml`. Used for `wombat build <dir>`,
# `wombat test <pkg>`, and `wombat run <pkg>` completion.
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
            # `wombat build [dir] [-o DIR] [--force]`
            if [[ "$cur" == -* ]]; then
                COMPREPLY=( $(compgen -W "-o --output --force" -- "$cur") )
            else
                # Only directories that look like a workspace or package.
                COMPREPLY=( $(compgen -W "$(_wombat_ws_dirs)" -- "$cur") )
            fi
            ;;
        run)
            # `wombat run [--no-jit] <target> [args...]`
            #
            # <target> may be a package name, a .kl source file, or a
            # .klc artifact. Arguments after <target> belong to the
            # program, so no wombat-level candidates are offered there.
            #
            # The target is the first non-option word after the
            # subcommand, not necessarily the word at COMP_CWORD 2:
            # --no-jit may come before it.
            local i seen_target=0
            for (( i=2; i < COMP_CWORD; i++ )); do
                case "${COMP_WORDS[i]}" in
                    -*) ;;
                    *) seen_target=1; break ;;
                esac
            done
            if [ "$seen_target" -eq 1 ]; then
                # Program arguments: no wombat-level candidates.
                COMPREPLY=()
                compopt +o default
            elif [[ "$cur" == -* ]]; then
                COMPREPLY=( $(compgen -W "--no-jit" -- "$cur") )
            else
                local dirs="$( _wombat_ws_dirs )"
                local files="$( compgen -f -X '!*.@(kl|klc)' -- "$cur" 2>/dev/null )"
                COMPREPLY=( $(compgen -W "$dirs $files" -- "$cur") )
            fi
            ;;
        test)
            # `wombat test [--no-jit] [target]`; `@` means only ws/test/.
            if [[ "$cur" == -* ]]; then
                COMPREPLY=( $(compgen -W "--no-jit" -- "$cur") )
            else
                COMPREPLY=( $(compgen -W "@ $(_wombat_ws_dirs)" -- "$cur") )
            fi
            ;;
        check|fmt|doc)
            # `wombat <cmd> [pkg]`: only workspace/package directories.
            if [[ "$cur" == -* ]]; then
                COMPREPLY=()
                compopt +o default
            else
                COMPREPLY=( $(compgen -W "$(_wombat_ws_dirs)" -- "$cur") )
            fi
            ;;
        clean)
            # `wombat clean [dir] [-y]`
            if [[ "$cur" == -* ]]; then
                COMPREPLY=( $(compgen -W "-y --yes" -- "$cur") )
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
