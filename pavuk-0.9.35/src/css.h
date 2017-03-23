/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _css_h_
#define _css_h_

#include "url.h"
#include "dllist.h"

extern dllist *css_get_all_links(url *, char *, char *, char *, int);
extern char *css_to_absolute_links(url *, char *, char *, char *);
extern char *css_remote_to_local_links(url *, char *, int, int, char *,
  char *);
extern char *css_change_url(url *, char *, url *, char *);

#endif
