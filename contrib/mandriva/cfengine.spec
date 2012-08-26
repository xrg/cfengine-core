%define git_repo cfengine
%define git_head HEAD
%define release_class experimental

#define distsuffix xrg

%define	name	cfengine3
%define version %git_get_ver

# We call ourselves "cfengine3" but still use "/var/lib/cfengine"
%define workdir %{_localstatedir}/lib/cfengine

%define major 3
#define libname %mklibname %{name} %{major}
%define libname %mklibname %{name}
%define develname %mklibname -d %{name}

Name:		%{name}
Version:	%{version}
Release:	%mkrel %git_get_rel
Summary:	CFEngine helps administer remote BSD and System-5-like systems
License:	GPL
Group:		Monitoring
URL:		http://www.cfengine.org
Source0:	%git_bs_source %{name}-%{version}.tar.gz
BuildRequires:	flex
BuildRequires:	bison
BuildRequires:	openssl-devel
BuildRequires:	db4-devel
BuildRequires: tokyocabinet-devel
BuildRequires: postgresql-devel
Requires(pre):	rpm-helper
Requires(preun):rpm-helper
# BuildRequires: texinfo

%description
CFEngine, the configuration engine, is a very high level language for
simplifying the task of administrating and configuring large numbers
of workstations. CFEngine uses the idea of classes and a primitive
form of intelligence to define and automate the configuration of large
systems in the most economical way possible.

%package base
Summary:	CFEngine base files
Group:		Monitoring
Provides:	cfengine-base
Obsoletes:	cfengine-base

%description base
This package contain the cfengine base files needed by all subpackages.

%package cfagent
Summary:	CFEngine agent
Group:		Monitoring
Requires:	%{name}-base = %{version}-%{release}
Provides:	cfengine-cfagent
Obsoletes:	cfengine-cfagent

%description cfagent
This package contain the cfengine agent.
It should be installed on all computers to be monitored/configured by
CFEngine.


%package cfserver
Summary:	CFEngine Hub server
Group:		Monitoring
Requires:	%{name}-base = %{version}-%{release}
Provides:	cfengine-cfserver
Obsoletes:	cfengine-cfserver
Requires(post):rpm-helper
Requires(preun):rpm-helper

%description cfserver
This package contain the cfengine server daemon.

Install it on the box you want to act as the policy hub, controlling
the other monitored boxes.

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

# chmod 644 inputs/*

%build
%serverbuild

  %before_configure ; \
  %{?!_disable_libtoolize:%{?__libtoolize_configure:%{__libtoolize_configure};}} \
  [ -f $CONFIGURE_TOP/configure.in -o -f $CONFIGURE_TOP/configure.ac ] && \
  CONFIGURE_XPATH="--x-includes=%{_prefix}/include --x-libraries=%{_prefix}/%{_lib}" \
  ./autogen.sh --target=%{_target_platform} \
	--program-prefix=%{?_program_prefix} \
 	--prefix=%{_prefix} \
	--exec-prefix=%{_exec_prefix} \
	--bindir=%{_bindir} \
	--sbindir=%{_sbindir} \
	--sysconfdir=%{_sysconfdir} \
	--datadir=%{_datadir} \
	--includedir=%{_includedir} \
	--libdir=%{_libdir} \
	--libexecdir=%{_libexecdir} \
	--localstatedir=%{_localstatedir} \
	--sharedstatedir=%{_sharedstatedir} \
	--mandir=%{_mandir} \
	--infodir=%{_infodir} \
	--with-workdir=%{workdir} \
	--enable-shared --enable-fhs  \
	--without-mysql
    $CONFIGURE_XPATH
#	 --enable-docs=all

%make

%install
rm -rf %{buildroot}
%make DESTDIR=%{buildroot} install

# install man page manually
# install -d -m 755 %{buildroot}%{_mandir}/man8
# install -m 644 doc/cfengine.8 %{buildroot}%{_mandir}/man8

# install -d -m 755 %{buildroot}%{_sysconfdir}/%{name}
install -d -m 755 %{buildroot}%{_sysconfdir}/cron.daily
install -d -m 755 %{buildroot}%{_sysconfdir}/sysconfig
install -d -m 755 %{buildroot}%{_initrddir}
install -d -m 755 %{buildroot}%{workdir}
install -m 755 contrib/mandriva/cfservd.init %{buildroot}%{_initrddir}/cf-serverd
install -m 755 contrib/mandriva/cfexecd.init %{buildroot}%{_initrddir}/cf-execd
# install -m 755 contrib/mandriva/cfenvd.init %{buildroot}%{_initrddir}/cfenvd

%post base
if [ $1 = 1 ]; then
    [ -f "%{workdir}/ppkeys/localhost.priv" ] || cfkey >/dev/null 2>&1
fi

%post cfagent
%_post_service cf-execd

%preun cfagent
%_preun_service cf-execd

%post cfserver
%_post_service cf-serverd

%preun cfserver
%_preun_service cfserver

%clean
rm -rf %{buildroot}

%files base
%defattr(-,root,root)
%doc %{_defaultdocdir}/cfengine/ChangeLog
%doc %{_defaultdocdir}/cfengine/README
# {_sysconfdir}/cfengine
%{_sbindir}/cf-key
%{workdir}
%{_mandir}/man8/*
%{_datadir}/cfengine/CoreBase/*


%files cfagent
%defattr(-,root,root)
%{_sbindir}/cf-agent
%{_sbindir}/cf-runagent
%{_sbindir}/cf-monitord
%{_sbindir}/cf-execd
%{_initrddir}/cf-execd

%files cfserver
%defattr(-,root,root)
%{_initrddir}/cf-serverd
%{_sbindir}/cf-serverd
%{_sbindir}/cf-report
%{_sbindir}/cf-promises
%{_defaultdocdir}/cfengine/examples/*


%files -n %{libname}
%defattr(-,root,root)
%{_libdir}/cfengine/*.so.*

%files -n %{develname}
%defattr(-,root,root)
%{_libdir}/cfengine/*.so
# %{_libdir}/*.a
%{_libdir}/cfengine/*.la

%changelog -f %{_sourcedir}/%{name}-changelog.gitrpm.txt
