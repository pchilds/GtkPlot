Summary:	Provides shared libraries for plotting with gtk+3
Name:		gtk3plot
Version:	3.0.0
Release:	1%{?dist}
License:	LGPLv3+
Group:		Development/Libraries
Source:		%{name}-%{version}.tar.gz
BuildRequires:	automake, autoconf, libtool, gtk3 >= 3.8, pkgconfig
Requires:	gtk+3 >= 3.8

%description
%{name} makes available to gtk+3 programs, a pi/donut, 
linear and polar plotting widgets. Features include: multiplot 
capability, zoom control, smart range setting and mouse location 
tracking. Processing is optimised towards speed for large data sets as 
commonly occurs in scientific applications and outputs to a neat 
publication ready format.

%package devel
Summary:	Provides headers for the %{name} package
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}, gtk3 >= 3.8, pkgconfig

%description devel
This package contains the header files and libraries needed to develop 
applications that use %{name}.

%package static
Summary:	Static development files for %{name}
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description static
This package contains static libraries to develop applications that use %{name}.

%prep
%setup -q

%build
%configure --disable-testbinaries
make %{?_smp_mflags}

%install
%make_install
rm -f $RPM_BUILD_ROOT/%{_libdir}/*.la

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS COPYING README NEWS ChangeLog
%{_libdir}/libgtkplot-3.0.so.*

%files devel
%defattr(-,root,root)
%{_includedir}/gtkplot-3.0/gtkplot*.h
%{_libdir}/gtkplot-3.0/include/gtkplotconfig.h
%{_libdir}/libgtkplot-3.0.so
%{_libdir}/pkgconfig/gtkplot-3.0.pc

%files static
%defattr(-,root,root)
%{_libdir}/libgtkplot-3.0.a

%changelog
* Sun Jun 22 2014 Paul Childs <pchilds@physics.org> 3.0.0-1
- Initial build
