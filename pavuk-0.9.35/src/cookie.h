/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _cookie_h_
#define _cookie_h_

#include <time.h>

#include "url.h"

/* private */
typedef struct _cookie_entry
{
  struct _cookie_entry *next;
  struct _cookie_entry *prev;
  char *domain;
  char *host;
  char *path;
  time_t expires;
  int secure;
  int flag;
  char *name;
  char *value;
  bool_t loaded;
} cookie_entry;

extern int cookie_read_file(const char *);
extern int cookie_update_file(int);
extern char *cookie_get_field(url *);
extern void cookie_insert_field(char *, url *);
#endif
