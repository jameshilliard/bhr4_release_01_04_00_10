/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _digest_auth_h_
#define _digest_auth_h_

#include "url.h"
#include "doc.h"

typedef struct
{
  char *nonce;
  char *opaque;
  char *realm;
  char *site;
  unsigned short port;
} http_digest_info;

extern http_digest_info *http_digest_parse(char *);
extern void http_digest_deep_free(http_digest_info *);
extern char *http_get_digest_auth_str(http_digest_info *, char *, char *,
  char *, url *, char *, size_t);
extern int http_digest_do_proxy_auth(doc *, char *);
extern int http_digest_do_auth(doc *, char *);

#endif
