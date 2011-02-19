Summary: A small, cross-platform display library for OpenGL
Name: libglv0
Version: 0.3.1
Release: 1
License: MIT
Group: Development/Libraries
Source: libglv-%{version}.tar.gz
Url: http://outguard.sourceforge.net/download.html
Packager: Karl Robillard <wickedsmoke@users.sf.net>
BuildRoot: %{_tmppath}/libglv-%{version}-build
Prefix: /usr/local
%if 0%{?fedora_version}
BuildRequires: mesa-libGL-devel libXxf86vm-devel
%endif
%if 0%{?mandriva_version}
BuildRequires: libmesagl1-devel
%endif
%if 0%{?suse_version}
BuildRequires: Mesa-devel
%endif

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
install -m 644 x11/libglv.so.0.3 $RPM_BUILD_ROOT%{_libdir}
ln -s libglv.so.0.3 $RPM_BUILD_ROOT%{_libdir}/libglv.so.0
ln -s libglv.so.0.3 $RPM_BUILD_ROOT%{_libdir}/libglv.so

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc ChangeLog LICENSE README
%{_libdir}/libglv.so
%{_libdir}/libglv.so.0
%{_libdir}/libglv.so.0.3
%{_includedir}/GL/glv.h
%{_includedir}/GL/glv_keys.h

%changelog
