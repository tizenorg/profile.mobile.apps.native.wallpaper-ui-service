CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(wallpaper-ui-service C)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

SET(VENDOR "org.tizen")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "${VENDOR}.${PACKAGE}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(PREFIXRW ${CMAKE_INSTALL_PREFIXRW})
SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(LOCALEDIR "${RESDIR}/locale")
SET(ICONDIR "${RESDIR}/icons")
SET(EDJDIR "${RESDIR}/edje")
SET(DESKTOPDIR "/usr/share/applications")
SET(MANIFESTXMLDIR "/usr/share/packages")

#AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/src MAIN_SRCS)

#SET(SRCS ${MAIN_SRCS})


SET(SRCS
	${CMAKE_SOURCE_DIR}/src/wallpaper-ui-service-main.c
	${CMAKE_SOURCE_DIR}/src/wallpaper-ui-service.c
)
INCLUDE_DIRECTORIES(
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/res
		${CMAKE_CURRENT_SOURCE_DIR}/include
		)

SET(CMAKE_SKIP_BUILD_RPATH TRUE)

MESSAGE("BRANCH : ${BRANCH}")

IF("${BRANCH}" STREQUAL "PRIVATE")
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	capi-appfw-application
	capi-system-system-settings
	dlog
	elementary
	vconf
	csc-feature
	efl-extension
	feedback
	ui-gadget-1
	notification
	capi-content-media-content
	wallpaper-service
	core-samsung-log-client
	edbus
	libtzplatform-config
)
ELSE()
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	capi-appfw-application
	capi-system-system-settings
	dlog
	elementary
	vconf
	efl-extension
	feedback
	ui-gadget-1
	notification
	capi-content-media-content
	edbus
	libtzplatform-config
)
ENDIF()

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag} -fpie")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")

#SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -finstrument-functions -g")

#SET(CMAKE_C_COMPILER "/scratchbox/devkits/sbwrap")
#SET(CMAKE_C_COMPILER_ARG1 "/home/serena/working/wallpaper-ui-service.trace /scratchbox/compilers/bin/gcc gcc")


#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wall -g -O2")

ADD_DEFINITIONS("-DVENDOR=\"${VENDOR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${PKGNAME}\"")
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DICONDIR=\"${ICONDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DHOME_KEY_EMULATION")
#ADD_DEFINITIONS("-DFILE_DEBUG")
ADD_DEFINITIONS("-D_DLOG_USED")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-lm")

INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.wallpaper-ui-service.xml DESTINATION /usr/share/packages)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PKGNAME}.efl DESTINATION /etc/smack/accesses.d)

ADD_SUBDIRECTORY(res)
IF("${BRANCH}" STREQUAL "PRIVATE")
ADD_SUBDIRECTORY(po)
ELSE()
ADD_SUBDIRECTORY(po_sdk)
ENDIF()
