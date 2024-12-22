Summary: A small, cross-platform display library for OpenGL
Name: libglv0
Version: 0.5.0
Release: 1
License: MIT
Group: Development/Libraries
Source: libglv-%{version}.tar.gz
Url: http://outguard.sourceforge.net/download.html
Packager: Karl Robillard <wickedsmoke@users.sf.net>
BuildRoot: %{_tmppath}/libglv-%{version}-build
Prefix: /usr/local
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires: mesa-libGL-devel libXxf86vm-devel
%endif
%if 0%{?mandriva_version}
BuildRequires: libmesagl1-devel
%endif
%if 0%{?suse_version}
BuildRequires: Mesa-devel
%endif

%global debug_package %{nil}

%description
The GLV library provides a small, cross-platform, C interface for creating
a window or fullscreen display with an OpenGL context.

%prep
%setup -q -n libglv-%{version}

%build
make -C x11

%install
mkdir -p $RPM_BUILD_ROOT%{_libdir}
mkdir -p $RPM_BUILD_ROOT%{_includedir}/GL
install -m 644 x11/*.h $RPM_BUILD_ROOT%{_includedir}/GL
install -m 755 x11/libglv.so.0.5 $RPM_BUILD_ROOT%{_libdir}
ln -s libglv.so.0.5 $RPM_BUILD_ROOT%{_libdir}/libglv.so.0
ln -s libglv.so.0.5 $RPM_BUILD_ROOT%{_libdir}/libglv.so

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc ChangeLog LICENSE README
%{_libdir}/libglv.so
%{_libdir}/libglv.so.0
%{_libdir}/libglv.so.0.5
%{_includedir}/GL/glv.h
%{_includedir}/GL/glv_keys.h

%changelog
* Sat Aug 24 2019 Karl Robillard <wickedsmoke@users.sf.net> - 0.3.2-2
  - Set debug_package to nil to enable rpmbuild to complete.
  - Remove unneeded post/postun ldconfig.
