%define git_repo cfengine
%define git_head mandriva

#define distsuffix xrg

%define	name	cfengine
%define version %git_get_ver
%define release %mkrel %git_get_rel

# _localstatedir is inconsistent..
%define varlibdir /var/lib

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
Source0:	%{name}-%{version}.tar.gz
#Source4:	cfservd.init
#Source5:	cfexecd.init
#Source6:	cfenvd.init
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
%git_get_source
%setup -q

chmod 644 inputs/*

%build
%serverbuild
%configure2_5x --with-workdir=%{varlibdir}/%{name} --enable-shared
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
install -d -m 755 %{buildroot}%{varlibdir}/%{name}
install -m 755 contrib/mandriva/cfservd.init %{buildroot}%{_initrddir}/cfservd
install -m 755 contrib/mandriva/cfexecd.init %{buildroot}%{_initrddir}/cfexecd
install -m 755 contrib/mandriva/cfenvd.init %{buildroot}%{_initrddir}/cfenvd

# everything installed there is doc, actually
rm -rf %{buildroot}%{_datadir}/%{name}

%post base
if [ $1 = 1 ]; then
    [ -f "%{varlibdir}/%{name}/ppkeys/localhost.priv" ] || cfkey >/dev/null 2>&1
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
%{varlibdir}/%{name}
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
