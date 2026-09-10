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
BuildRequires:  pkgconfig(bas-cpp)
BuildRequires:  wxGTK3-devel

%description
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
%{_mandir}/man1/soptools.1*
%{_mandir}/man1/sopwin.1*
%{_mandir}/*/man1/soptools.1*
%{_mandir}/*/man1/sopwin.1*
%{_mandir}/ar/man1/ar-soptools.1*
%{_mandir}/ar/man1/ar-sopwin.1*
%{_mandir}/de/man1/de-soptools.1*
%{_mandir}/de/man1/de-sopwin.1*
%{_mandir}/es_MX/man1/es_MX-soptools.1*
%{_mandir}/es_MX/man1/es_MX-sopwin.1*
%{_mandir}/fr/man1/fr-soptools.1*
%{_mandir}/fr/man1/fr-sopwin.1*
%{_mandir}/it/man1/it-soptools.1*
%{_mandir}/it/man1/it-sopwin.1*
%{_mandir}/ja/man1/ja-soptools.1*
%{_mandir}/ja/man1/ja-sopwin.1*
%{_mandir}/ko/man1/ko-soptools.1*
%{_mandir}/ko/man1/ko-sopwin.1*
%{_mandir}/zh_CN/man1/zh_CN-soptools.1*
%{_mandir}/zh_CN/man1/zh_CN-sopwin.1*
%{_datadir}/soptools/
%{_datadir}/locale/*/LC_MESSAGES/soptools.mo
%{_datadir}/doc/soptools/
%changelog
* Thu Sep 10 2026 Lenik <soptools@bodz.net>
- Package locale-prefixed man pages for zfr lint ZL047.
* Wed Sep 09 2026 Lenik <soptools@bodz.net>
- Fix %%description typo; package locale man pages and bas-cpp/wx deps.
* Tue Sep 01 2026 Lenik <soptools@bodz.net>
- Package sopwin binary, completion, and man page from Meson install.
* Thu Aug 20 2026 Lenik <soptools@bodz.net>
- Align spec with debian/control (Meson, AGPL-3.0-or-later).
- Version comes from `zfr version`, the same method meson.build uses.
