# drm.spec
#
%define name            drm
%define version         0.9.3
%define release         1
 
Summary:	Digital Radio Mondiale (DRM) software receiver.
Name:		%{name}
Version:	%{version}
Release:	%{release}
License:	GPL
Group:		Applications/Communications
Source0:	%{name}-%{version}.tar.gz
URL:		http://drm.sourceforge.net/
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:	autoconf, automake
BuildRequires:	fftw, fftw-devel, qwt, qwt-devel, faad2, faad2-devel

%description
Digital Radio Mondiale (DRM) is a new digital radio standard for
the long, medium and short wave ranges.

Dream is an open source software implementation of a DRM receiver.

%prep
%setup -q

%build
chmod 755 bootstrap && ./bootstrap
%configure
make

%install
rm -rf %{buildroot}
%makeinstall

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)
%doc AUTHORS ChangeLog COPYING INSTALL NEWS README TODO
%{_bindir}/drm

%changelog
* Sun Nov 30 2003 Tomi Manninen <oh2bns@sral.fi>
- First try

