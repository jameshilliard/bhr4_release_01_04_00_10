/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _jstrans_h_
#define _jstrans_h_

#ifdef HAVE_REGEX
#include "re.h"

typedef struct
{
  int type;
  re_entry *re;
  char *transform;
  char *tag;
  char *attrib;
} js_transform_t;

extern js_transform_t *js_transform_new(const char *, const char *,
const char *, const char *, int);
extern void js_transform_free(js_transform_t *);
extern int js_transform_match_tag(js_transform_t *, const char *);
extern char *js_transform_apply(js_transform_t *, const char *, int, int *);

#endif

#endif
