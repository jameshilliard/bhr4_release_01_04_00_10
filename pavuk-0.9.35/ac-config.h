/* ac-config.h.  Generated from ac-config.h.in by configure.  */
/* ac-config.h.in.  Generated from configure.in by autoheader.  */

/* Berkeley DB 1.8x comatibility */
/* #undef DB_COMPATIBILITY_API */

/* Define if you want to compile with debuging options */
#define DEBUG 1

/* largest data type */
#define DLLISTTYPE void *

/* path to EGD/PRNGD socket */
/* #undef EGD_SOCKET_NAME */

/* NTLM authorization support */
#define ENABLE_NTLM 1

/* getpgrp() need PID parameter */
/* #undef GETPGRP_NEED_PID */

/* gettext intl stuffs */
/* #undef GETTEXT_NLS */

/* with GTK+ GUI */
/* #undef GTK_FACE */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Berkeley DB 1.8x */
/* #undef HAVE_BDB_18x */

/* BSD REs */
/* #undef HAVE_BSD_REGEX */

/* have _nl_msg_cat_cntr in libintl */
/* #undef HAVE_CAT_CNTR */

/* have db185.h */
/* #undef HAVE_DB185_H */

/* have db1/db.h */
/* #undef HAVE_DB1_H */

/* have db2/db185.h */
/* #undef HAVE_DB2_DB185_H */

/* have db3/db185.h */
/* #undef HAVE_DB3_DB185_H */

/* have db4/db185.h */
/* #undef HAVE_DB4_DB185_H */

/* dprintf */
#define HAVE_DPRINTF 1

/* fcntl() file locking */
/* #undef HAVE_FCNTL_LOCK */

/* have flock() in libc */
#define HAVE_FLOCK 1

/* Define to 1 if your system has a working POSIX `fnmatch' function. */
/* #undef HAVE_FNMATCH */

/* Define to 1 if you have the `freeaddrinfo' function. */
#define HAVE_FREEADDRINFO 1

/* Define to 1 if you have the `fstatfs' function. */
/* #undef HAVE_FSTATFS */

/* Define to 1 if you have the `fstatvfs' function. */
#define HAVE_FSTATVFS 1

/* Define to 1 if you have the `gai_strerror' function. */
#define HAVE_GAI_STRERROR 1

/* gcrypt support for NTLM */
/* #undef HAVE_GCRYPT */

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* threadsafe gethostbyname_r function */
/* #undef HAVE_GETHOSTBYNAME_R */

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* have tm_gmtoff inside struct tm */
#define HAVE_GMTOFF 1

/* GNU REs */
/* #undef HAVE_GNU_REGEX */

/* IPv6 support */
#define HAVE_INET6 1

/* Define to 1 if you have the `inet_ntop' function. */
#define HAVE_INET_NTOP 1

/* Define to 1 if you have the `inet_pton' function. */
#define HAVE_INET_PTON 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <jsapi.h> header file. */
/* #undef HAVE_JSAPI_H */

/* Define to 1 if you have the `db' library (-ldb). */
/* #undef HAVE_LIBDB */

/* Define to 1 if you support file names longer than 14 characters. */
#define HAVE_LONG_FILE_NAMES 1

/* have libmcrypt */
/* #undef HAVE_MCRYPT */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* with JavaScript bindings */
/* #undef HAVE_MOZJS */

/* multithreading support */
/* #undef HAVE_MT */

/* Define if you have cygwin with new paths style */
/* #undef HAVE_NEWSTYLE_CYGWIN */

/* Define to 1 if you have the <nss.h> header file. */
/* #undef HAVE_NSS_H */

/* libcrypto from OpenSSL contains MD4 cipher */
#define HAVE_OPENSSL_MD4 1

/* PCRE REs */
/* #undef HAVE_PCRE_REGEX */

/* POSIX REs */
#define HAVE_POSIX_REGEX 1

/* Define to 1 if you have the <prio.h> header file. */
/* #undef HAVE_PRIO_H */

/* OpenSSL lib contains EGD/PRNGD support */
#define HAVE_RAND_EGD 1

/* REs support */
#define HAVE_REGEX 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the <smime.h> header file. */
/* #undef HAVE_SMIME_H */

/* have declared struct sockaddr_storage */
#define HAVE_SOCKADDR_STORAGE 1

/* <socks.h> */
/* #undef HAVE_SOCKS_H */

/* Define to 1 if you have the <ssl.h> header file. */
/* #undef HAVE_SSL_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/statfs.h> header file. */
#define HAVE_SYS_STATFS_H 1

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#define HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
#define HAVE_SYS_VFS_H 1

/* Define if you have tcgetpgrp() */
#define HAVE_TERMIOS 1

/* have timezon function tzset() */
#define HAVE_TZSET 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* SYSV 8 REs */
/* #undef HAVE_V8_REGEX */

/* SYSV 8 RE exports regsub */
/* #undef HAVE_V8_REGSUB */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* have Xmu library */
/* #undef HAVE_XMU */

/* have libz */
#define HAVE_ZLIB 1

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Set host type */
#define HOSTTYPE "linux-gnueabihf"

/* with GUI */
/* #undef I_FACE */

/* libc doesn't export h_errno variable */
/* #undef NEED_DECLARE_H_ERRNO */

/* improper alignment of NTLM structures */
/* #undef NTLM_UNPACKED_STRUCT */

/* have new OpenSSL not old SSLeay libs */
#define OPENSSL 1

/* Name of package */
#define PACKAGE "pavuk"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "pavuk"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "pavuk 0.9.35"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pavuk"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.9.35"

/* revision number of software */
#define REVISION "2016-12-02T00:38"

/* Socks proxy support */
/* #undef SOCKS */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* want SSL support */
#define USE_SSL 1

/* use Mozilla NSS3 for SSL support */
/* #undef USE_SSL_IMPL_NSS */

/* use OpenSSL for SSL support */
#define USE_SSL_IMPL_OPENSSL 1

/* Version number of package */
#define VERSION "0.9.35"

/* have new OpenSSL with TLS1 support */
#define WITH_SSL_TLS1 1

/* with preview dialog for HTML tree */
/* #undef WITH_TREE */

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Define to `long' if <sys/types.h> does not define. */
/* #undef ssize_t */
