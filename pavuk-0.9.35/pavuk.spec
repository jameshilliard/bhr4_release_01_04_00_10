#
# spec file for package pavuk
#
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# norootforbuild

Name:           pavuk
Version:        0.9.35
Release:        1
Autoreqprov:    on
Group:          Productivity/Networking/Web/Utilities
License:        GNU General Public License (GPL)
URL:            http://www.pavuk.org/
Summary:        Powerful WWW or FTP site mirror tool
Summary(de):    Maechtiges Programm zum Spiegeln von Webseiten
Source:         %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%if 0%{?suse_version}
BuildRequires:  dante-devel db-devel gcc glib2-devel gtk2-devel openssl-devel
%else
%if 0%{?fedora_version}
BuildRequires:  db-devel gcc glib2-devel gtk+-devel libjs-devel openssl-devel
%else
BuildRequires:  dante-devel db-devel gcc glib2-devel gtk2-devel libjs-devel openssl-devel
%endif
%endif

%description
Pavuk is used to download or mirror web sites or files. It transfers documents
from HTTP, FTP, Gopher and optionally from HTTPS (HTTP over SSL) servers. An
optional GTK GUI allows easy configuration. Many options allow fine-tuning for
the usage scenario. This is an tool for experts and much to complicated for
beginners.

%description -l de
Pavuk wird genutzt, um Webseiten oder Dateien herunterzuladen oder zu spiegeln.
Es kann Dokumente von HTTP-, FTP-, Gopher- und optional HTTPS-Servern (HTTP
ueber SSL) laden. Eine optionale GTK-Oberflaeche erlaubt einfaches konfigurieren.
Viele Optionen ermoeglichen eine Feinanpassung an die Aufgabenstellung. Dieses
Programm ist fuer Anfaenger sehr kompliziert und eher fuer Experten gedacht.

%if 0%{?suse_version}
%debug_package
%endif
%prep
%setup -q

%build
%if 0%{?suse_version}
%{suse_update_config -f . }
%endif
CXXFLAGS="$RPM_OPT_FLAGS -DNDEBUG -Wall" ./configure \
        --disable-gnome \
        --enable-utf-8 \
        --prefix=%{_prefix} \
        --sysconfdir=%{_sysconfdir} \
        --mandir=%{_mandir}
make %{?jobs:-j%jobs}

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall
rm $RPM_BUILD_ROOT%{_bindir}/*.sh
%find_lang %name
mkdir -p $RPM_BUILD_ROOT%{_datadir}/pixmaps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
cp $RPM_BUILD_ROOT%{_datadir}/icons/pavuk_32x32.xpm \
   $RPM_BUILD_ROOT%{_datadir}/pixmaps/pavuk.xpm
iconv %{name}.desktop -f iso-8859-1 -t utf-8 >$RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop
%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc ABOUT-NLS AUTHORS BUGS ChangeLog COPYING CREDITS MAILINGLIST NEWS README TODO wget-pavuk.HOWTO
%doc %{_mandir}/man?/*
%doc pavukrc.sample
%{_bindir}/*
%{_datadir}/icons/*
%{_datadir}/pixmaps/*
%{_datadir}/applications/*.desktop
%{_prefix}/share/locale/*/LC_MESSAGES/%{name}.mo

%changelog
* Tue Feb 13 2007 - soft@dstoecker.de
- recreated for openSUSE Build service
