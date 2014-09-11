%define git_repo cfengine
%define git_head HEAD

%define	name	cfengine3
%define version %git_get_ver

# We call ourselves "cfengine3" but still use "/var/lib/cfengine"
%define workdir %{_localstatedir}/lib/cfengine

%define major 3
#define libname %mklibname %{name} %{major}
%define libname %mklibname %{name}
%define develname %mklibname -d %{name}

%if %{mgaver} >= 3
%define _with_systemd 1
%else
%define _with_systemd 0
%endif

Name:		%{name}
Version:	%{version}
Release:	%mkrel %git_get_rel2
Summary:	CFEngine helps administer remote BSD and System-5-like systems
License:	GPL
Group:		Monitoring
URL:		http://www.cfengine.org
Source0:	%git_bs_source %{name}-%{version}.tar.gz
BuildRequires:	flex
BuildRequires:	bison
BuildRequires:  texinfo
BuildRequires:	openssl-devel
BuildRequires:	db4-devel
%if %{_target_cpu} == i586 && %{mgaver} == 1
BuildRequires: tokyocabinet-devel
%else
BuildRequires: libldap-devel
%endif
BuildRequires: postgresql-devel
BuildRequires: libpcre-devel
Requires:       %{libname} = %{version}-%{release}
Requires(pre):	rpm-helper
Requires(preun):rpm-helper
# BuildRequires: texinfo
Provides:	cfengine-base
Obsoletes:	cfengine-base

%description
CFEngine, the configuration engine, is a very high level language for
simplifying the task of administrating and configuring large numbers
of workstations. CFEngine uses the idea of classes and a primitive
form of intelligence to define and automate the configuration of large
systems in the most economical way possible.

This package contain the cfengine base files needed by all subpackages.

Note: these packages use the Unix-standard "/var/lib" prefix, rather
than the proprietary "/var/cfengine" path.

%package cfagent
Summary:	Configuration Client daemon
Group:		Monitoring
Requires:	%{name} = %{version}-%{release}
Provides:	cfengine-cfagent
Obsoletes:	cfengine-cfagent

%description cfagent
This package contain the cfengine agent.

It should be installed on all computers to be monitored/configured by
CFEngine.


%package cfserver
Summary:	Policy Distribution Server
Group:		Monitoring
Requires:	%{name} = %{version}-%{release}
Provides:	cfengine-cfserver
Obsoletes:	cfengine-cfserver
Requires(post):rpm-helper
Requires(preun):rpm-helper

%description cfserver
This package contain the files for CFEngine policy distribution. Such a
node "serves" the clients (cf-agents) with CFEngine configuration files.

Install it on the box you want to act as the policy hub, controlling
the other monitored boxes.

%package cfupgrade
Summary:    Configuration Engine upgrade helper
Group:      Monitoring

%description cfupgrade
This is a statically-built upgrade helper for CFEngine

In some cases, when eg. upgrading from official packages or pre-3.4 releases,
you may want this helper in order to perform the upgrade in an atomic manner.


%package pds
Summary:	Policy Definition Server
Group:		Monitoring
BuildArch:	noarch
Suggests:	%{name}-cfserver = %{version}
Suggests:	git-core

%description pds
Install this on a single, master node, which will be the main definition
point for CFEngine policies. The machine you will be writting/keeping the
policies at.

Even in a full deployment, you should prefer to have a single ever point
of keeping/authoring the policies. From this one you may push the policies
to the Policy Distribution machines (ones with %{name}-cfserver), so that
clients can then pick them up and be configured.

This package also contains the documentation and examples, for the policy
authors.


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
	--disable-dependency-tracking \
	--disable-maintainer-mode \
	--enable-shared --enable-fhs  \
%if %{_target_cpu} == i586 && %{mgaver} == 1
	--with-tokyocabinet --without-lmdb \
%endif
	--without-mysql
    $CONFIGURE_XPATH

%make

%install
rm -rf %{buildroot}
%make DESTDIR=%{buildroot} install

# install -d -m 755 %{buildroot}%{_sysconfdir}/%{name}
install -d -m 755 %{buildroot}%{_sysconfdir}/cron.daily
install -d -m 755 %{buildroot}%{_sysconfdir}/sysconfig
install -d -m 755 %{buildroot}%{workdir}
install -d -m 755 %{buildroot}%{workdir}/lib
install -d -m 755 %{buildroot}%{workdir}/reports
%if %{_with_systemd}
install -d -m 755 %{buildroot}%{_unitdir}
install -m 755 contrib/systemd/*.service %{buildroot}%{_unitdir}/
sed -i 's|/var/cfengine/bin/|%{_sbindir}/|' %{buildroot}%{_unitdir}/cf-*.service
%else
install -d -m 755 %{buildroot}%{_initrddir}
install -m 755 contrib/mandriva/cfservd.init %{buildroot}%{_initrddir}/cf-serverd
install -m 755 contrib/mandriva/cfexecd.init %{buildroot}%{_initrddir}/cf-execd
install -m 755 contrib/mandriva/cf-monitord.init %{buildroot}%{_initrddir}/cf-monitord
%endif

pushd %{buildroot}%{workdir}
install -d -m 755 ./bin
for BPROG in %{buildroot}%{_sbindir}/*  ; do
	BINP="$(basename $BPROG)"
	ln -s %{_sbindir}/$BINP ./bin/
done

popd

install -d %{buildroot}%{workdir}/masterfiles
cat '-' << EOF > %{buildroot}%{workdir}/masterfiles/README.first
In this directory you should keep your original version of the policy
files.
For a start, you could copy files from %{_datadir}/cfengine/CoreBase/
into this folder, and start tweaking them.

It is also recommended that you put this folder under version control.

EOF

#%post base
# bootstrap? Not yet, since our "failsafe.cf" is too generic

%post cfagent
%_post_service cf-execd
%_post_service cf-serverd
# We are not going to start cf-monitord, because this can be done inside
# some cf-execd->cf-agent check.

%preun cfagent
%_preun_service cf-execd
%_preun_service cf-serverd
%_preun_service cf-monitord

%clean
rm -rf %{buildroot}

# All binaries have 2 files:
#  1. the bin at /usr/sbin/xxx
#  2. the symlink at /var/lib/cfengine/bin/xxx
# so, write a macro for them

%define cfprog(:)  %{_sbindir}/%1 \
	%{workdir}/bin/%1 \
	%()

%files
%defattr(-,root,root)
%doc %{_defaultdocdir}/cfengine/ChangeLog
%doc %{_defaultdocdir}/cfengine/README.md
%dir %{workdir}
%dir %{workdir}/bin
%dir %{workdir}/ppkeys
%dir %{workdir}/modules
%dir %{workdir}/lib
%cfprog cf-key
%cfprog cf-promises
%cfprog cf-serverd
%{_sbindir}/rpmvercmp
%{workdir}/bin/rpmvercmp
# TODO: write a manpage and make rpmvercmp a "cfprog"
%if %{_with_systemd}
%{_unitdir}/cf-serverd.service
%{_unitdir}/cf-monitord.service
%else
%{_initrddir}/cf-serverd
%{_initrddir}/cf-monitord
%endif


%files cfagent
%defattr(-,root,root)
%cfprog cf-agent
%cfprog cf-monitord
%cfprog cf-execd
%if %{_with_systemd}
%{_unitdir}/cf-execd.service
%else
%{_initrddir}/cf-execd
%endif
%dir %{workdir}/inputs
%dir %{workdir}/outputs

%files cfupgrade
%defattr(-,root,root)
%cfprog cf-upgrade


%files cfserver
%defattr(-,root,root)
%cfprog cf-runagent
%dir %{workdir}/masterfiles
%dir %{workdir}/reports

%files pds
# doc %{_defaultdocdir}/cfengine/guides/
# doc %{_defaultdocdir}/cfengine/reference/
%{_defaultdocdir}/cfengine/examples/*
%{workdir}/masterfiles/README.first


%files -n %{libname}
%defattr(-,root,root)
%{_libdir}/cfengine/*.so.*

%files -n %{develname}
%defattr(-,root,root)
%{_libdir}/cfengine/*.so
# %{_libdir}/*.a
%{_libdir}/cfengine/*.la

%changelog -f %{_sourcedir}/%{name}-changelog.gitrpm.txt
