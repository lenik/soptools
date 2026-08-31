# Version is injected by packaging/rpm/Makefile via `zfr version`.
# RPM Version cannot contain '-'; use `zfr version -r` (hyphens → '_').
# srcversion is the unsanitized Meson/git version and names the tarball.
%{!?version:%global version 0.0.0}
%{!?srcversion:%global srcversion %{version}}

Name:           soptools
Version:        %{version}
Release:        1%{?dist}
Summary:        SOP workflow assistant for project construction

License:        AGPL-3.0-or-later
URL:            https://github.com/lenik/soptools
Packager:       Lenik <soptools@bodz.net>
Source0:        %{name}-%{srcversion}.tar.xz

BuildRequires:  meson
BuildRequires:  ninja-build
BuildRequires:  pkgconf
BuildRequires:  asciidoctor

%sdescription
soptools guides project construction through Standard Operating Procedure
workflows with a wxWidgets GUI or console interface. It ships builtin SOP
definitions, gettext translations, AsciiDoc man pages, and bash completion.

%prep
%setup -q -n %{name}-%{srcversion}

%build
meson setup build \
    --prefix=%{_prefix} \
    --bindir=%{_bindir} \
    --datadir=%{_datadir} \
    --mandir=%{_mandir} \
    --sysconfdir=%{_sysconfdir} \
    --localstatedir=%{_localstatedir} \
    --buildtype=plain
meson compile -C build

%install
meson install -C build --destdir=%{buildroot}

%files
%{_bindir}/sopwin
%{_datadir}/bash-completion/completions/sopwin
%{_datadir}/bash-completion/completions/ar-soptools
%{_datadir}/bash-completion/completions/de-soptools
%{_datadir}/bash-completion/completions/es_MX-soptools
%{_datadir}/bash-completion/completions/fr-soptools
%{_datadir}/bash-completion/completions/it-soptools
%{_datadir}/bash-completion/completions/ja-soptools
%{_datadir}/bash-completion/completions/ko-soptools
%{_datadir}/bash-completion/completions/sop_ai_output
%{_datadir}/bash-completion/completions/sop_console
%{_datadir}/bash-completion/completions/sop_gui
%{_datadir}/bash-completion/completions/sop_model
%{_datadir}/bash-completion/completions/sop_paths
%{_datadir}/bash-completion/completions/sop_runtime
%{_datadir}/bash-completion/completions/stream_copy
%{_datadir}/bash-completion/completions/zh_CN-soptools
%{_datadir}/soptools/
%{_datadir}/locale/*/LC_MESSAGES/soptools.mo
%{_mandir}/man1/sopwin.1*
%{_mandir}/*/man1/ar-soptools.1*
%{_mandir}/*/man1/de-soptools.1*
%{_mandir}/*/man1/es_MX-soptools.1*
%{_mandir}/*/man1/fr-soptools.1*
%{_mandir}/*/man1/it-soptools.1*
%{_mandir}/*/man1/ja-soptools.1*
%{_mandir}/*/man1/ko-soptools.1*
%{_mandir}/*/man1/soptools.1*
%{_mandir}/*/man1/zh_CN-soptools.1*
%{_datadir}/doc/soptools/
%{_datadir}/doc/%{name}/
%changelog
* Tue Sep 01 2026 Lenik <soptools@bodz.net>
- Package sopwin binary, completion, and man page from Meson install.
* Thu Aug 20 2026 Lenik <soptools@bodz.net>
- Align spec with debian/control (Meson, AGPL-3.0-or-later).
- Version comes from `zfr version`, the same method meson.build uses.
