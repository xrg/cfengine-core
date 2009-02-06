%define	name	cfengine
%define version 2.2.9
%define release %mkrel 1

%define major 1
%define libname %mklibname %{name} %{major}
%define develname %mklibname -d %{name}

Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:	Cfengine helps administer remote BSD and System-5-like systems
License:	GPL
Group:		Monitoring
URL:		http://www.cfengine.org
Source0:	http://www.cfengine.org/downloads/%{name}-%{version}.tar.gz
Source4:	cfservd.init
Source5:	cfexecd.init
Source6:	cfenvd.init
Patch:      cfengine-2.2.9-fix-format-errors.patch
BuildRequires:	flex
BuildRequires:	bison
BuildRequires:	openssl-devel
BuildRequires:	db4-devel
Requires(pre):	rpm-helper
Requires(preun):rpm-helper
BuildRoot:      %{_tmppath}/%{name}-%{version}

%description
Cfengine, the configuration engine, is a very high level language for
simplifying the task of administrating and configuring large numbers
of workstations. Cfengine uses the idea of classes and a primitive
form of intelligence to define and automate the configuration of large
systems in the most economical way possible.

%package base
Summary:	Cfengine base files
Group:		Monitoring

%description base
This package contain the cfengine base files needed by all subpackages.

%package cfagent
Summary:	Cfengine agent
Group:		Monitoring
Requires:	%{name}-base = %{version}-%{release}

%description cfagent
This package contain the cfengine agent.

%package cfservd
Summary:	Cfengine server daemon
Group:		Monitoring
Requires:	%{name}-base = %{version}-%{release}
Requires(post):rpm-helper
Requires(preun):rpm-helper

%description cfservd
This package contain the cfengine server daemon.

%package cfexecd
Summary:	Cfengine agent execution wrapper
Group:		Monitoring
Requires:	%{name}-base = %{version}-%{release}
Requires(post):	rpm-helper
Requires(preun):rpm-helper

%description cfexecd
This package contain the cfengine agent execution wrapper.

%package cfenvd
Summary:	Cfengine anomaly detection daemon
Group:		Monitoring
Requires:	%{name}-base = %{version}-%{release}
Requires(pre):	rpm-helper
Requires(preun):rpm-helper

%description cfenvd
This package contain the cfengine anomaly detection daemon.

%package -n	%{libname}
Summary:	Dynamic libraries for %{name}
Group:		System/Libraries

%description -n	%{libname}
This package contains the library needed to run programs dynamically
linked with %{name}.

%package -n	%{develname}
Summary:	Development files for %{name}
Group:		Development/C
Requires:	%{libname} = %{version}
Provides:	%{name}-devel = %{version}-%{release}

%description -n	%{develname}
This package contains the header files and libraries needed for
developing programs using the %{name} library.

%prep
%setup -q
%patch -p 1

chmod 644 inputs/*

%build
%serverbuild
%configure2_5x --with-workdir=%{_localstatedir}/lib/%{name} --enable-shared
%make

%install
rm -rf %{buildroot}
%makeinstall

# install man page manually
install -d -m 755 %{buildroot}%{_mandir}/man8
install -m 644 doc/cfengine.8 %{buildroot}%{_mandir}/man8

install -d -m 755 %{buildroot}%{_sysconfdir}/%{name}
install -d -m 755 %{buildroot}%{_sysconfdir}/cron.daily
install -d -m 755 %{buildroot}%{_sysconfdir}/sysconfig
install -d -m 755 %{buildroot}%{_initrddir}
install -d -m 755 %{buildroot}%{_localstatedir}/lib/%{name}
install -m 755 %{SOURCE4} %{buildroot}%{_initrddir}/cfservd
install -m 755 %{SOURCE5} %{buildroot}%{_initrddir}/cfexecd
install -m 755 %{SOURCE6} %{buildroot}%{_initrddir}/cfenvd

# everything installed there is doc, actually
rm -rf %{buildroot}%{_datadir}/%{name}

%post base
if [ $1 = 1 ]; then
    [ -f "%{_localstatedir}/lib/%{name}/ppkeys/localhost.priv" ] || cfkey >/dev/null 2>&1
fi

%post cfexecd
%_post_service cfexecd

%preun cfexecd
%_preun_service cfexecd

%post cfenvd
%_post_service cfenvd

%preun cfenvd
%_preun_service cfenvd

%post cfservd
%_post_service cfservd

%preun cfservd
%_preun_service cfservd

%clean
rm -rf %{buildroot}

%files base
%defattr(-,root,root)
%doc inputs/*.example
%{_sysconfdir}/cfengine
%{_sbindir}/cfkey
%{_sbindir}/cfshow
%{_sbindir}/cfdoc
%{_localstatedir}/lib/%{name}
%{_mandir}/man8/cfengine.*


%files cfagent
%defattr(-,root,root)
%{_sbindir}/cfagent
%{_sbindir}/cfenvgraph
%{_sbindir}/cfrun
%{_sbindir}/cfetool*

%files cfservd
%defattr(-,root,root)
%{_initrddir}/cfservd
%{_sbindir}/cfservd

%files cfenvd
%defattr(-,root,root)
%{_initrddir}/cfenvd
%{_sbindir}/cfenvd

%files cfexecd
%defattr(-,root,root)
%{_initrddir}/cfexecd
%{_sbindir}/cfexecd

%files -n %{libname}
%defattr(-,root,root)
%{_libdir}/*.so.*

%files -n %{develname}
%defattr(-,root,root)
%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/*.la
