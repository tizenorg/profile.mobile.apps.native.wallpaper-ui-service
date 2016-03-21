%define PREFIX    %{TZ_SYS_RO_APP}/%{name}
%define RESDIR    %{PREFIX}/res

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
BuildRequires: pkgconfig(capi-appfw-app-manager)
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

cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DBRANCH=%{BRANCH}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%clean
rm -rf %{buildroot}

%post

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{PREFIX}/*
%{TZ_SYS_RO_PACKAGES}/*
%license LICENSE
