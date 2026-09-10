# bash completion for sopwin

_sopwin()
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="-s --suite -S --sop-dir -c --console -g --gui -C --chdir -v --verbose -q --quiet -h --help --version"

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

    COMPREPLY=( $(compgen -d -- "${cur}") )
}

complete -F _sopwin sopwin
