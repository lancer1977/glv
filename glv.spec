Summary: A small, cross-platform display library for OpenGL
Name: glv
Version: 0.3
Release: 1
Copyright: MIT
Group: Development/Libraries
Source: http://outguard.sourceforge.net/arc/%{name}-%{version}.tgz
Url: http://outguard.sourceforge.net/download.html
Packager: Karl Robillard <wickedsmoke@users.sf.net>
BuildRoot: /var/tmp/%{name}-buildroot
Prefix: /usr/local

%description
The GLV library provides a small, cross-platform interface for creating
a window or fullscreen display with an OpenGL context.

%prep
%setup -q

%build
cd unix
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{prefix}/lib
mkdir -p $RPM_BUILD_ROOT%{prefix}/include/GL
install -m 644 lib/*.so $RPM_BUILD_ROOT%{prefix}/lib
install -m 644 unix/*.h $RPM_BUILD_ROOT%{prefix}/include/GL

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc ChangeLog LICENSE README
%dir %{prefix}/include/%{name}
%{prefix}/lib/libglv.so
%{prefix}/include/%{name}


%changelog
