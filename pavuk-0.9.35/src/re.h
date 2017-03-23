/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _re_h_
#define _re_h_

#ifdef HAVE_REGEX
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#ifdef HAVE_POSIX_REGEX

typedef struct
{
  regex_t preg;
  char *pattern;
} re_entry;

#endif

#ifdef HAVE_V8_REGEX
#ifndef NSUBEXP
#include <regexp.h>
#endif

typedef struct
{
  regexp *preg;
  char *pattern;
} re_entry;

#endif

#ifdef HAVE_BSD_REGEX
typedef struct
{
  char *pattern;
} re_entry;
#endif

#ifdef HAVE_GNU_REGEX
typedef struct
{
  struct re_pattern_buffer preg;
  char *pattern;
} re_entry;
#endif

#ifdef HAVE_PCRE_REGEX
#include <pcre.h>

typedef struct
{
  pcre *preg;
  pcre_extra *preg_extra;
  char *pattern;
} re_entry;
#endif

extern re_entry *re_make(const char *);
extern void re_free(re_entry *);
extern int re_pmatch(re_entry *, char *);
extern int re_pmatch_sub(re_entry *, char *, int, int *, int *);
extern int re_pmatch_subs(re_entry *, char *, int *, int **);

#endif
#endif
