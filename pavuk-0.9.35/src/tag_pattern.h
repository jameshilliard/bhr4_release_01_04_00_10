/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _tag_pattern_h_
#define _tag_pattern_h_

#include "re.h"

typedef struct _tag_pattern_t
{
  enum
  {
    TAGP_WC,
    TAGP_RE,
  } type;
  char *tag;
  char *attrib;
  char *urlp;
#ifdef HAVE_REGEX
  re_entry *tag_re;
  re_entry *attrib_re;
  re_entry *urlp_re;
#endif
} tag_pattern_t;

extern tag_pattern_t *tag_pattern_new(int, char *, char *, char *);
extern void tag_pattern_free(tag_pattern_t *);
extern int tag_pattern_match(tag_pattern_t *, char *, char *, char *);

#endif
