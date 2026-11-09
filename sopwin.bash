# bash completion for sopwin

_sopwin()
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="-s --suite -S --sop-dir -c --console -g --gui -C --chdir -v --verbose -q --quiet -h --help --version"
    cmds="open run sopdlg copydlg edit version"

    case "${prev}" in
        -S|--sop-dir|--sop|-C|--chdir)
            COMPREPLY=( $(compgen -d -- "${cur}") )
            return 0
            ;;
        -s|--suite)
            local suites=""
            if [ -d suite ]; then
                suites=$(ls -1 suite 2>/dev/null)
            elif [ -d /usr/share/soptools/suite ]; then
                suites=$(ls -1 /usr/share/soptools/suite 2>/dev/null)
            fi
            COMPREPLY=( $(compgen -W "${suites}" -- "${cur}") )
            return 0
            ;;
    esac

    if [[ ${cur} == -* ]]; then
        COMPREPLY=( $(compgen -W "${opts}" -- "${cur}") )
        return 0
    fi

    # Suggest CMD when first non-option argument
    local i
    local has_cmd=0
    for ((i=1; i < COMP_CWORD; i++)); do
        case "${COMP_WORDS[i]}" in
            open|run|sopdlg|copydlg|edit|version) has_cmd=1; break ;;
        esac
    done
    if [[ $has_cmd -eq 0 ]]; then
        COMPREPLY=( $(compgen -W "${cmds}" -- "${cur}") )
        if [[ ${#COMPREPLY[@]} -gt 0 ]]; then
            return 0
        fi
    fi

    COMPREPLY=( $(compgen -d -- "${cur}") )
}

complete -F _sopwin sopwin
