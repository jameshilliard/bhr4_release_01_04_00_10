dnl Process this file with autoconf to produce a configure script.
AC_INIT([ulogd], [2.0.2])
AC_PREREQ([2.50])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall foreign tar-pax no-dist-gzip dist-bzip2 1.10b])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
dnl kernel style compile messages
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl Checks for libraries.
AC_SEARCH_LIBS([dlopen], [dl], [libdl_LIBS="$LIBS"; LIBS=""])
AC_SUBST([libdl_LIBS])
AC_SEARCH_LIBS([pcap_close], [pcap], [libpcap_LIBS="-lpcap"; LIBS=""])
AC_SUBST([libpcap_LIBS])
AM_CONDITIONAL([HAVE_PCAP], [test -n "$libpcap_LIBS"])
if test "x$libpcap_LIBS" != "x"; then
	enable_pcap="yes"
else
	enable_pcap="no"
fi

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_SYS_LARGEFILE

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(socket strerror)

regular_CFLAGS="-Wall -Wextra -Wno-unused-parameter"
AC_SUBST([regular_CFLAGS])

dnl Check for the right nfnetlink version
PKG_CHECK_MODULES([LIBNFNETLINK], [libnfnetlink >= 1.0.1])
PKG_CHECK_MODULES([LIBMNL], [libmnl >= 1.0.3])
AC_ARG_ENABLE(nflog,
       AS_HELP_STRING([--enable-nflog], [Enable nflog module [default=yes]]),,[enable_nflog=yes])
AS_IF([test "x$enable_nflog" = "xyes"], [
    PKG_CHECK_MODULES([LIBNETFILTER_LOG], [libnetfilter_log >= 1.0.0])
    AC_DEFINE([BUILD_NFLOG], [1], [Building nflog module])
])
AM_CONDITIONAL([BUILD_NFLOG], [test "x$enable_nflog" = "xyes"])
AC_ARG_ENABLE(nfct,
       AS_HELP_STRING([--enable-nfct], [Enable nfct module [default=yes]]),,[enable_nfct=yes])
AS_IF([test "x$enable_nfct" = "xyes"], [
    PKG_CHECK_MODULES([LIBNETFILTER_CONNTRACK], [libnetfilter_conntrack >= 1.0.2])
    AC_DEFINE([BUILD_NFCT], [1], [Building nfct module])
])
AM_CONDITIONAL([BUILD_NFCT], [test "x$enable_nfct" = "xyes"])
AC_ARG_ENABLE(nfacct,
       AS_HELP_STRING([--enable-nfacct], [Enable nfacct module [default=yes]]),,[enable_nfacct=yes])
AS_IF([test "x$enable_nfacct" = "xyes"], [
    PKG_CHECK_MODULES([LIBNETFILTER_ACCT], [libnetfilter_acct >= 1.0.1])
    AC_DEFINE([BUILD_NFACCT], [1], [Building nfacct module])
])
AM_CONDITIONAL([BUILD_NFACCT], [test "x$enable_nfacct" = "xyes"])

CT_CHECK_POSTGRES_DB()
AM_CONDITIONAL(HAVE_PGSQL, test "x$PQLIBPATH" != "x")
if test "x$PQLIBPATH" != "x"; then
	enable_pgsql="yes"
else
	enable_pgsql="no"
fi

CT_CHECK_MYSQL_DB()
AM_CONDITIONAL(HAVE_MYSQL, test "x$MYSQL_LIB" != "x")
if test "x$MYSQL_LIB" != "x"; then
	enable_mysql="yes"
else
	enable_mysql="no"
fi

PKG_CHECK_MODULES([libsqlite3], [sqlite3], [], [:])
AM_CONDITIONAL([HAVE_SQLITE3], [test -n "$libsqlite3_LIBS"])
if test "x$libsqlite3_LIBS" != "x"; then
	enable_sqlite3="yes"
else
	enable_sqlite3="no"
fi

CT_CHECK_DBI()
AM_CONDITIONAL(HAVE_DBI, test "x$DBI_LIB" != "x")
if test "x$DBI_LIB" != "x"; then
	enable_dbi="yes"
else
	enable_dbi="no"
fi

dnl AC_SUBST(DATABASE_DIR)
dnl AC_SUBST(DATABASE_LIB)
dnl AC_SUBST(DATABASE_LIB_DIR)
dnl AC_SUBST(DB_DEF)
dnl AC_SUBST(EXTRA_MYSQL_DEF)
dnl AC_SUBST(EXTRA_PGSQL_DEF)

dnl AC_SUBST(DATABASE_DRIVERS)

dnl AM_CONDITIONAL(HAVE_MYSQL, test x$mysqldir != x)
dnl AM_CONDITIONAL(HAVE_PGSQL, test x$pgsqldir != x)

AC_CONFIG_FILES(include/Makefile include/ulogd/Makefile include/libipulog/Makefile \
	  include/linux/Makefile include/linux/netfilter/Makefile \
	  libipulog/Makefile \
	  input/Makefile input/packet/Makefile input/flow/Makefile \
	  input/sum/Makefile \
	  filter/Makefile filter/raw2packet/Makefile filter/packet2flow/Makefile \
	  output/Makefile output/pcap/Makefile output/mysql/Makefile output/pgsql/Makefile output/sqlite3/Makefile \
	  output/dbi/Makefile \
	  src/Makefile Makefile Rules.make)
AC_OUTPUT

echo "
Ulogd configuration:
  Input plugins:
    NFLOG plugin:			${enable_nflog}
    NFCT plugin:			${enable_nfct}
    NFACCT plugin:			${enable_nfacct}
  Output plugins:
    PCAP plugin:			${enable_pcap}
    PGSQL plugin:			${enable_pgsql}
    MySQL plugin:			${enable_mysql}
    SQLITE3 plugin:			${enable_sqlite3}
    DBI plugin:				${enable_dbi}
"
echo "You can now run 'make' and 'make install'"
