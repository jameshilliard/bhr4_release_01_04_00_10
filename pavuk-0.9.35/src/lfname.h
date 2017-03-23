/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _lfname_h_
#define _lfname_h_

#ifdef HAVE_REGEX
#ifdef HAVE_PCRE_REGEX
#include <pcre.h>
#endif
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#ifdef HAVE_V8_REGEX
#ifndef NSUBEXP
#include <regexp.h>
#endif
#endif
#endif
#endif

#include "url.h"

typedef enum
{
  LFNAME_UNKNOWN,
#ifdef HAVE_REGEX
  LFNAME_REGEX,
#endif
  LFNAME_FNMATCH
} lfname_type;

typedef struct
{
  lfname_type type;
#ifdef HAVE_POSIX_REGEX
  regex_t preg;
  regmatch_t *pmatch;
#endif
#ifdef HAVE_V8_REGEX
  regexp *preg;
#endif
#ifdef HAVE_GNU_REGEX
  struct re_pattern_buffer preg;
  struct re_registers pmatch;
#endif
#ifdef HAVE_PCRE_REGEX
  pcre *preg;
  pcre_extra *preg_extra;
  int pmatch_nr;
  int *pmatch;
#endif
  char *matchstr;
  char *transstr;
} lfname;

/* need to export for jsbind.c */
struct lfname_lsp_interp
{
  url *urlp;
  const char *urlstr;
  char *scheme;
  char *passwd;
  char *user;
  char *host;
  char *domain;
  char *port;
  char *path;
  char *name;
  char *basename;
  char *extension;
  char *query;
  char *post_query;
  char *deflt;
  const char *mime_type;
  const char *mime_type_ext;
  lfname *orig;
};

extern lfname *lfname_new(lfname_type, const char *, const char *);
extern void lfname_free(lfname *);
extern char *lfname_get_by_url(url *, const char *, const char *, lfname *);
extern int lfname_match(lfname *, const char *);
extern int lfname_check_pattern(lfname_type, const char *);
extern int lfname_check_rule(const char *);
extern const char *lfname_interp_get_macro(struct lfname_lsp_interp *, int);
extern int lfname_check_macro(int);
extern char *lfname_re_sub(lfname *, const char *, int);


#endif
