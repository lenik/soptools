project(
    'soptools',
    'cpp',
    version: run_command(
        'sh',
        '-c', '''
        v=$(
            command -v zfr >/dev/null 2>&1 && zfr version 2>/dev/null || true
        )
        if [ -z "$v" ] && [ -f VERSION ]; then
            v=$(head -n1 VERSION | tr -d '\r')
            v=${v#v}
        fi
        if [ -z "$v" ]; then
            v="0.0.0" # FIXED TO 0.0.0, DO NOT MODIFY
        fi
        printf '%s' "$v"
        ''',
        check: false,
    ).stdout().strip(),
    license: 'AGPL-3.0-or-later',
    default_options: [
        'cpp_std=c++17',
        'warning_level=2',
    ],
)

project_author = 'Lenik'
project_email = 'soptools@bodz.net'
project_year = 2026

asciidoctor = find_program('asciidoctor', required: true)
fs = import('fs')

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
datadir = prefix / get_option('datadir')
mandir = prefix / get_option('mandir')
pkgdocdir = datadir / 'doc' / meson.project_name()

source_root = meson.project_source_root()
sop_builtin_dir = source_root / 'figma.sop'

config_h = configuration_data()
config_h.set_quoted('PROJECT_VERSION', meson.project_version())
config_h.set_quoted('PROJECT_AUTHOR', project_author)
config_h.set_quoted('PROJECT_EMAIL', project_email)
config_h.set('PROJECT_YEAR', project_year)
config_h.set_quoted('SOURCE_ROOT', source_root)
config_h.set_quoted('SOP_BUILTIN_DIR', sop_builtin_dir)

configure_file(
    output: 'config.h',
    configuration: config_h,
)

inc = include_directories('src', '.')

wx_config = find_program('wx-config', required: true)
wx_cflags = wx_config.full_path() != '' ? run_command(wx_config, '--cxxflags', check: true).stdout().strip().split() : []
wx_libs = wx_config.full_path() != '' ? run_command(wx_config, '--libs', 'std', check: true).stdout().strip().split() : []
wxwidgets = declare_dependency(
    compile_args: wx_cflags,
    link_args: wx_libs,
)

app_sources = [
    'src/sopwin.cpp',
    'src/sop_model.cpp',
    'src/sop_runtime.cpp',
    'src/sop_paths.cpp',
    'src/sop_project.cpp',
    'src/sop_ai_output.cpp',
    'src/sop_console.cpp',
    'src/sop_gui.cpp',
    'src/gui/sop_graph_canvas.cpp',
    'src/gui/sop_graph_style.cpp',
    'src/gui/sop_log_view.cpp',
]

executable(
    'sopwin',
    app_sources,
    include_directories: inc,
    dependencies: [wxwidgets],
    install: true,
    install_dir: bindir,
)

install_subdir(
    'figma.sop',
    install_dir: datadir / meson.project_name(),
    strip_directory: false,
)

bash_files = [
    'sopwin.bash',
    'completions/ar-soptools.bash',
    'completions/de-soptools.bash',
    'completions/es_MX-soptools.bash',
    'completions/fr-soptools.bash',
    'completions/it-soptools.bash',
    'completions/ja-soptools.bash',
    'completions/ko-soptools.bash',
    'completions/sop_ai_output.bash',
    'completions/sop_console.bash',
    'completions/sop_gui.bash',
    'completions/sop_model.bash',
    'completions/sop_paths.bash',
    'completions/sop_runtime.bash',
    'completions/stream_copy.bash',
    'completions/zh_CN-soptools.bash',
]

foreach file : bash_files
    name = fs.stem(file)
    install_data(
        file,
        install_dir: datadir / 'bash-completion' / 'completions',
        rename: name,
    )
endforeach

install_data(
    [
        'LICENSE',
        'README.md',
        'README-zh.md',
    ],
    install_dir: pkgdocdir,
)

subdir('po')
subdir('tests')

run_target(
    'look',
    command: [
        'bash',
        '-euc', '''
            tmpdir=$(mktemp -d)
            DESTDIR=$tmpdir meson install -C "@BUILD_ROOT@"
            tree "$tmpdir"
            rm -fr "$tmpdir"
        ''',
    ],
)

run_target(
    'install-symlinks',
    command: [
        'bash',
        '-euc', '''
            prefix="@0@"
            bindir="@1@"
            datadir="@2@"
            mandir="@3@"
            mkdir -p "$bindir" "$datadir/bash-completion/completions" "$mandir/man1"
            sudo ln -sfn "@BUILD_ROOT@/sopwin" "$bindir/sopwin"
            sudo ln -sfn "@BUILD_ROOT@/sopwin.1" "$mandir/man1/sopwin.1"
            sudo ln -sfn "@SOURCE_ROOT@/sopwin.bash" "$datadir/bash-completion/completions/sopwin"
            printf "Symlinks installed under %s\n" "$prefix"
        '''.format(get_option('prefix'), bindir, datadir, mandir),
    ],
)

run_target(
    'uninstall-symlinks',
    command: [
        'bash',
        '-euc', '''
            for p in \
                "@1@/sopwin" \
                "@3@/man1/sopwin.1" \
                "@2@/bash-completion/completions/sopwin"
            do
                if [ -L "$p" ]; then
                    sudo rm -f "$p"
                    printf "Removed %s\n" "$p"
                else
                    printf "Skipped (not a symlink): %s\n" "$p"
                fi
            done
        '''.format(get_option('prefix'), bindir, datadir, mandir),
    ],
)

man_puffs = [
    'sopwin',
]
foreach puff : man_puffs
  custom_target(
    puff + '-man',
    input: 'docs' / (puff + '.adoc'),
    output: puff + '.1',
    command: [
      asciidoctor,
      '-b', 'manpage',
      '-a', 'project-version=' + meson.project_version(),
      '-a', 'project-year=@0@'.format(project_year),
      '-a', 'project-author=' + project_author,
      '-a', 'project-email=' + project_email,
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: true,
    install: true,
    install_dir: mandir / 'man1',
  )
endforeach

man_locales = ['ar', 'de', 'es_MX', 'fr', 'it', 'ja', 'ko', 'zh_CN']
foreach locale : man_locales
  man_src = 'docs' / locale / 'soptools.adoc'
  if fs.exists(man_src)
    custom_target(
      'soptools-man-' + locale,
      input: man_src,
      output: locale + '-soptools.1',
      command: [
        asciidoctor,
        '-b', 'manpage',
        '-a', 'project-version=' + meson.project_version(),
        '-a', 'project-year=@0@'.format(project_year),
        '-a', 'project-author=' + project_author,
        '-a', 'project-email=' + project_email,
        '-o', '@OUTPUT@',
        '@INPUT@',
      ],
      build_by_default: true,
      install: false,
    )
  endif
endforeach

meson.add_install_script(
  'sh', '-euc', '''
    build="@0@"
    prefix="${MESON_INSTALL_DESTDIR_PREFIX:-${DESTDIR:-}${MESON_INSTALL_PREFIX:-}}"
    for loc in @1@; do
      src="$build/${loc}-soptools.1"
      [ -f "$src" ] || continue
      dest="$prefix/share/man/${loc}/man1/soptools.1"
      mkdir -p "$(dirname "$dest")"
      install -m644 "$src" "$dest"
    done
  '''.format(meson.current_build_dir(), ' '.join(man_locales)),
)
