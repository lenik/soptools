# bash completion for sopwin

_sopwin()
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="-s --sop -c --console -g --gui -C --chdir -v --verbose -q --quiet -h --help --version"

    case "${prev}" in
        -s|--sop|-C|--chdir)
            COMPREPLY=( $(compgen -d -- "${cur}") )
            return 0
            ;;
    esac

    if [[ ${cur} == -* ]]; then
        COMPREPLY=( $(compgen -W "${opts}" -- "${cur}") )
        return 0
    fi

    COMPREPLY=( $(compgen -d -- "${cur}") )
}

complete -F _sopwin sopwin
