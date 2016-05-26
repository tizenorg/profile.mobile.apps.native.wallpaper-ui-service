Name:       org.tizen.wallpaper-ui-service
#VCS_FROM:   profile/mobile/apps/native/wallpaper-ui-service#fb0b88b17e95d6b0c5054af3898acf982ad6578d
#RS_Ver:    20160526_2 
Summary:    wallpaper-ui-service window
Version:    1.0.0
Release:    1
Group:      utils
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

ExcludeArch:  aarch64 x86_64
BuildRequires:  pkgconfig(libtzplatform-config)
Requires(post):  /usr/bin/tpk-backend

%define internal_name org.tizen.wallpaper-ui-service
%define preload_tpk_path %{TZ_SYS_RO_APP}/.preload-tpk 

%ifarch i386 i486 i586 i686 x86_64
%define target i386
%else
%ifarch arm armv7l aarch64
%define target arm
%else
%define target noarch
%endif
%endif

%description
profile/mobile/apps/native/wallpaper-ui-service#fb0b88b17e95d6b0c5054af3898acf982ad6578d
This is a container package which have preload TPK files

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{preload_tpk_path}
install %{internal_name}-%{version}-%{target}.tpk %{buildroot}/%{preload_tpk_path}/

%post

%files
%defattr(-,root,root,-)
%{preload_tpk_path}/*
