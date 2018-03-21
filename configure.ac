AC_PREREQ([2.59])
AC_INIT([modulename], m4_esyscmd([./generate_version_number.sh]), [you@example.com])
AC_CONFIG_SRCDIR([module.c])
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AM_CFLAGS=$naemon_CFLAGS
AM_CPPFLAGS=-D_GNU_SOURCE
# check for compiler flags. Enabling warnings must come before disabling
AX_CHECK_COMPILE_FLAG([-Wall], AM_CFLAGS+=" -Wall")
AX_CHECK_COMPILE_FLAG([-fPIC], AM_CFLAGS+=" -fPIC")
AX_CHECK_COMPILE_FLAG([-pipe], AM_CFLAGS+=" -pipe")
AX_CHECK_COMPILE_FLAG([-ggdb3], AM_CFLAGS+=" -ggdb3")
#AX_CHECK_COMPILE_FLAG([-Werror], AM_CFLAGS+=" -Werror")
AX_CHECK_COMPILE_FLAG([-Wextra], AM_CFLAGS+=" -Wextra")
AX_CHECK_COMPILE_FLAG([-Wredundant-decls], AM_CFLAGS+=" -Wredundant-decls")
AX_CHECK_COMPILE_FLAG([-Wtrigraphs], AM_CFLAGS+=" -Wtrigraphs")
AX_CHECK_COMPILE_FLAG([-Wdeclaration-after-statement], AM_CFLAGS+=" -Wdeclaration-after-statement")
AX_CHECK_COMPILE_FLAG([-Wshadow], AM_CFLAGS+=" -Wshadow")
AX_CHECK_COMPILE_FLAG([-Wstrict-prototypes], AM_CFLAGS+=" -Wstrict-prototypes")
AX_CHECK_COMPILE_FLAG([-Wno-unused-parameter], AM_CFLAGS+=" -Wno-unused-parameter")
AX_CHECK_COMPILE_FLAG([-Wno-strict-aliasing], AM_CFLAGS+=" -Wno-strict-aliasing")
AX_CHECK_COMPILE_FLAG([-Wno-error=unused-result], AM_CFLAGS+=" -Wno-error=unused-result")

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])

PKG_CHECK_MODULES([naemon], [naemon >= 0.8])
naemon_cfg=`$PKG_CONFIG --variable=mainconf naemon`
AS_IF([test "x$naemon_cfg" == "x"],
	  [naemon_cfg=/etc/naemon/naemon.cfg])
AC_SUBST(naemon_cfg)
AC_SYS_LARGEFILE

AC_ARG_ENABLE(auto_postinstall, AS_HELP_STRING([--disable-auto-postinstall], [Don't try to modify naemon's configuration or setup the SQL database. If you disable this, you must manually run the install-merlin.sh script in the source distribution after installation. You should disable this if you build redistributable packages.]))
AM_CONDITIONAL(AUTO_POSTINSTALL, [test "x$enable_auto_postinstall" != "xno"])

AC_ARG_WITH(naemon-config-dir, AS_HELP_STRING([--with-naemon-config-dir], [Install config into this directory)]), [naemonconfdir=$withval], [naemonconfdir=`AS_DIRNAME([${naemon_cfg}])`])
AC_SUBST(naemonconfdir)

AC_ARG_WITH(naemon-user, AS_HELP_STRING([--with-naemon-user], [The (system) user naemon runs as, which merlin will run as, too (default=autodetect)]), [naemon_user=$withval], [naemon_user=`$PKG_CONFIG --variable=naemon_user naemon`])
AC_SUBST(naemon_user)
AC_ARG_WITH(naemon-group, AS_HELP_STRING([--with-naemon-group], [The (system) group naemon runs as, which merlin will run as, too (default=autodetect)]), [naemon_group=$withval], [naemon_group=`$PKG_CONFIG --variable=naemon_group naemon`])
AC_SUBST(naemon_group)

AC_ARG_WITH(pkgconfdir, AS_HELP_STRING([--with-pkgconfdir], [The directory where merlin installs its configuration (default=${sysconfdir}/merlin)]), [pkgconfdir=$withval], [pkgconfdir=\${sysconfdir}/merlin])
AC_SUBST(pkgconfdir)

AC_CONFIG_FILES([Makefile loadme.cfg])
AC_CONFIG_MACRO_DIR([m4])
AC_OUTPUT
