/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _robots_h_
#define _robots_h_

#include "tools.h"
#include "mt.h"
#include "url.h"

typedef struct
{
  char *site;                   /*** host name of server for "robots.txt" checking ***/
  int port;                     /*** server port ***/
  char **dpat;                  /*** list of disallowed prefixes ***/
  char **apat;                  /*** list of allowed prefixes */
#ifdef HAVE_MT
  pthread_mutex_t lock;         /*** lock to prevent downloading concurrently with multiple threads ***/
#endif
} robotlim;

extern bool_t robots_check(url *);
extern void robots_do_cleanup(void);

#endif
