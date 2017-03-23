/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _tools_h_
#define _tools_h_

#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#include "dllist.h"

typedef unsigned char bool_t;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define TL_MAX(x,y)             ((x) > (y) ? (x) : (y))
#define TL_MIN(x,y)             ((x) < (y) ? (x) : (y))
#define TL_BETWEEN(c,a,b)       ((c) >= (a) && (c) <= (b))

#define NUM_ELEM(x)     ((unsigned int) (sizeof(x) / sizeof(x[0])))

#define _free(ptr) if (ptr) {free(ptr); ptr = NULL;}

extern char *tl_strdup(const char *);
extern char *tl_strndup(const char *, int);
extern char **tl_str_split(const char *, const char *);
extern dllist *tl_numlist_split(char *, char *);
extern void tl_strv_free(char **);
extern int tl_strv_find(char **, char *);
extern int tl_strv_length(char **);
extern void tl_strv_sort(char **);
extern char *tl_str_append(char *, char *);
extern char *tl_str_nappend(char *, const char *, int);
extern char *tl_str_concat(char *, ...);
extern char *tl_data_concat_data(int *, char *, int, char *);
extern char *tl_data_concat_str(int *, char *, ...);

extern void tl_msleep(unsigned int);
extern void tl_sleep(unsigned int);

extern char *tl_adjust_filename(char *);
extern int tl_filename_needs_adjust(char *);
extern int tl_is_dirname(const char *);
extern char *tl_get_extension(char *);
extern char *tl_get_basename(char *);

extern int tl_flock(int *, const char *, int, int);

extern int tl_mkstemp(char *);

extern char *tl_load_text_file(char *);
extern int tl_save_text_file(char *, char *, int);

extern void strip_nl(char *);
extern void omit_chars(char *, char *);
extern char *upperstr(char *);
extern char *lowerstr(char *);
extern int makealldirs(const char *);
extern char *get_abs_file_path(char *);
extern char *get_abs_file_path_oss(char *);
extern char *get_relative_path(char *, char *);
extern bool_t is_in_list(char *, char **);
extern bool_t is_in_dllist(char *, dllist *);
extern bool_t is_in_pattern_list(char *, char **);
extern bool_t is_in_pattern_dllist(char *, dllist *);
extern char *get_1qstr(const char *);
extern char *strtokc_r(char *, int, char **);
extern char *strfindnchr(char *, int, int);
extern void *_malloc(int);
extern void *_realloc(void *, int);
extern int get_line(char *, int, int);
extern bool_t file_is_html(char *);
extern bool_t ext_is_html(char *);
extern long int _atoi(char *);
extern double _atof(char *);
extern char *_strtrchr(char *, int, int);
extern void xprintf(int, const char *, ...);
extern void xvaprintf(int, const char *, va_list *);
extern void xdebug(int, const char *, ...);
extern void xvadebug(int, const char *, va_list *);
extern void xperror(const char *);
extern void xherror(const char *);
extern int unlink_recursive(char *);
extern unsigned int hash_func(const char *, int);
extern int str_is_in_list(int, char *, ...);
extern int copy_fd_to_file(int, char *);
extern char *escape_str(char *, char *);

#ifdef __CYGWIN__
extern char *cvt_win32_to_unix_path(char *);
extern char *cvt_unix_to_win32_path(char *);
extern int tl_win32_system(char *);
#define tl_system(cmd)  tl_win32_system(cmd)
#else
#define tl_system(cmd)  system(cmd)
#endif

#ifndef HAVE_SETENV
extern int tl_setenv(const char *, const char *, int);
#define setenv          tl_setenv
#define unsetenv(var)   tl_setenv(var, "", 1)
#endif

#ifdef HAVE_FLOCK
#include <sys/file.h>
#define _flock(fd, fname, opt, blk)     tl_flock(&fd, fname, opt, blk)
#define _funlock(fd)                    flock(fd, LOCK_UN)
#else
#ifdef HAVE_FCNTL_LOCK
extern int tl_funlock(int);
#define _flock(fd, fname, opt, blk)     tl_flock(&fd, fname, opt, blk)
#define _funlock(fd)                    tl_funlock(fd)
#else
#define _flock(fd, fname, opt, blk)     0
#define _funlock(fd)                    0
#endif
#endif

#define new_string      tl_strdup
#define new_n_string    tl_strndup



#define tl_ascii_isalnum(c) \
        (TL_BETWEEN((c),'a','z') || TL_BETWEEN((c),'A','Z') || \
         TL_BETWEEN((c),'0','9'))

#define tl_ascii_isalpha(c) \
        (TL_BETWEEN((c),'a','z') || TL_BETWEEN((c),'A','Z'))

#define tl_ascii_isascii(c) \
        (TL_BETWEEN((c),0,127))

#define tl_ascii_isblank(c) \
        (((c) == ' ') || ((c) == '\t'))

#define tl_ascii_iscntrl(c) \
        (TL_BETWEEN((c),0,31) || ((c) == 127))

#define tl_ascii_isdigit(c) \
        (TL_BETWEEN((c),'0','9'))

#define tl_ascii_isgraph(c) \
        (TL_BETWEEN((c),33,126))

#define tl_ascii_islower(c) \
        (TL_BETWEEN((c),'a','z'))

#define tl_ascii_isprint(c) \
        (TL_BETWEEN((c),32,126))

#define tl_ascii_ispunct(c) \
        (TL_BETWEEN((c),33,47) || TL_BETWEEN((c),58,64) || \
         TL_BETWEEN((c),91,96) || TL_BETWEEN((c),123,126))

#define tl_ascii_isspace(c) \
        (((c) == ' ') || ((c) == '\f') || ((c) == '\n') || ((c) == '\r') || \
         ((c) == '\t') || ((c) == '\v'))

#define tl_ascii_isupper(c) \
        (TL_BETWEEN((c),'A','Z'))

#define tl_ascii_isxdigit(c) \
        (TL_BETWEEN((c),'0','9') || TL_BETWEEN((c),'a','f') || \
         TL_BETWEEN((c),'A','F'))

#define tl_ascii_tolower(c) \
        (tl_ascii_isupper(c) ? ('a' + (c - 'A')) : c)

#define tl_ascii_toupper(c) \
        (tl_ascii_islower(c) ? ('A' + (c - 'a')) : c)

#endif
