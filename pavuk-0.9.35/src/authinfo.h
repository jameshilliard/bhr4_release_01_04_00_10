/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _authinfo_h_
#define _authinfo_h_

#include "url.h"
#include "http.h"
#include "dllist.h"

typedef struct
{
  protocol prot;
  char *host;
  int port;
  char *user;
  char *pass;
  char *base;
  char *realm;
  char *ntlm_domain;
  http_auth_type_t type;
} authinfo;

extern dllist *authdata;

extern http_auth_type_t authinfo_get_type(char *);

extern authinfo *authinfo_match_entry(protocol, char *, int, char *, char *);

extern int authinfo_load(char *);
extern int authinfo_save(char *);

#define free_deep_authinfo(entry)\
        _free((entry)->pass);\
        _free((entry)->user);\
        _free((entry)->host);\
        _free((entry)->base);\
        _free((entry)->ntlm_domain);\
        _free(entry);


#endif
