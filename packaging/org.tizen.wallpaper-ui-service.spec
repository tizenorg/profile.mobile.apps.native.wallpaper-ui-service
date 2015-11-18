%define PREFIX    %{TZ_SYS_RO_APP}/%{name}
%define RESDIR    %{PREFIX}/res
%define PREFIXRW  %{TZ_SYS_RO_APP}/%{name}

Name:       org.tizen.wallpaper-ui-service
Summary:    wallpaper-ui-service window
Version:    0.0.1
Release:    1
Group:      utils
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?tizen_profile_name}"=="tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(ui-gadget-1)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(edbus)
BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: gettext-tools
BuildRequires: hash-signer
BuildRequires: pkgconfig(libtzplatform-config)

Requires(post): /usr/bin/vconftool

%description
wallpaper-ui-service window.

%prep
%setup -q

%build
%define BRANCH "SDK"

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
CFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export CXXFLAGS
FFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export FFLAGS

cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DBRANCH=%{BRANCH} -DCMAKE_INSTALL_PREFIXRW=%{PREFIXRW} \

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

if [ ! -d /opt/usr/share/lockscreen/wallpaper_list ]
then
	mkdir -p %{buildroot}/opt/usr/share/lockscreen/wallpaper_list
fi

mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}/opt/usr/apps/org.tizen.wallpaper-ui-service/shared
mkdir -p %{buildroot}/opt/usr/apps/org.tizen.wallpaper-ui-service/shared/res
mkdir -p %{buildroot}/opt/usr/apps/org.tizen.wallpaper-ui-service/data
mkdir -p %{buildroot}/opt/usr/apps/org.tizen.wallpaper-ui-service/data/wallpaper

%define tizen_sign 1
%define tizen_sign_base /usr/apps/org.tizen.wallpaper-ui-service
%define tizen_sign_level platform
%define tizen_author_sign 1
%define tizen_dist_sign 1

%clean
rm -rf %{buildroot}

%post

#usr/bin/signing-client/hash-signer-client.sh -a -d -p platform /usr/apps/org.tizen.wallpaper-ui-service

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.wallpaper-ui-service.manifest
%defattr(-,root,root,-)
%{PREFIX}/bin/*
/usr/apps/org.tizen.wallpaper-ui-service/res/locale/*
%{RESDIR}/icons/*
%{RESDIR}/edje/*
/usr/share/packages/org.tizen.wallpaper-ui-service.xml
/usr/share/license/%{name}
/etc/smack/accesses.d/%{name}.efl
/usr/apps/org.tizen.wallpaper-ui-service/author-signature.xml
/usr/apps/org.tizen.wallpaper-ui-service/signature1.xml

%attr(-,app,app) %dir /opt/usr/apps/org.tizen.wallpaper-ui-service/shared
%attr(-,app,app) %dir /opt/usr/apps/org.tizen.wallpaper-ui-service/shared/res
%attr(-,app,app) %dir /opt/usr/apps/org.tizen.wallpaper-ui-service/data
%attr(-,app,app) %dir /opt/usr/apps/org.tizen.wallpaper-ui-service/data/wallpaper

%attr(755,app,app)/opt/usr/share/lockscreen
%attr(755,app,app)/opt/usr/share/lockscreen/wallpaper_list
