/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#include "authinfo.h"
#include "tools.h"
#include "url.h"
#include "dllist.h"
#include "bufio.h"

dllist *authdata = NULL;

#define AUTHI_UNKNOWN   0
#define AUTHI_PROTO   1
#define AUTHI_HOST    2
#define AUTHI_USER    3
#define AUTHI_PASS    4
#define AUTHI_BASE    5
#define AUTHI_TYPE    6
#define AUTHI_REALM   7
#define AUTHI_NTLM_DOMAIN 8

typedef struct
{
  char *id;
  int type;
} authi_entry;

static authi_entry authidef[] = {
  {"Proto:", AUTHI_PROTO},
  {"Host:", AUTHI_HOST},
  {"User:", AUTHI_USER},
  {"Pass:", AUTHI_PASS},
  {"Base:", AUTHI_BASE},
  {"Type:", AUTHI_TYPE},
  {"Realm:", AUTHI_REALM},
  {"NTLMDomain:", AUTHI_NTLM_DOMAIN},
};

http_auth_type_t authinfo_get_type(char *str)
{
  http_auth_type_t type;

  type = http_get_authorization_type(str);

  if(type == HTTP_AUTH_NONE)
  {
    type = _atoi(str);
    if(errno == ERANGE || type >= HTTP_AUTH_LAST)
      type = HTTP_AUTH_NONE;
  }

  return type;
}

int authinfo_load(char *fn)
{
  bufio *fd;
  int i;
  char lnbuf[4096];
  char *lns;
  char *p;
  bool_t new_section = TRUE;
  bool_t found;
  authinfo *ap = NULL;

  LOCK_AUTHINFO;
  while(authdata)
  {
    authinfo *ai = (authinfo *) authdata->data;

    free_deep_authinfo(ai);
    authdata = dllist_remove_entry(authdata, authdata);
  }

  if(!(fd = bufio_open(fn, O_BINARY | O_RDONLY)))
  {
    xperror(fn);
    UNLOCK_AUTHINFO;
    return -1;
  }

  while(bufio_readln(fd, lnbuf, sizeof(lnbuf)) > 0)
  {
    strip_nl(lnbuf);
    for(lns = lnbuf; *lns && tl_ascii_isspace(*lns); lns++);
    if(*lns == '#')
      continue;

    if(!*lns)
    {
      if(ap && ap->prot == URLT_UNKNOWN)
      {
        xprintf(0, gettext("Bad section - specify protocol\n"));
        free_deep_authinfo(ap);
      }
      if(ap && !ap->host)
      {
        xprintf(0, gettext("Bad section - specify hostname\n"));
        free_deep_authinfo(ap);
      }
      if(ap && ap->port == 0)
      {
        ap->port = prottable[ap->prot].default_port;
      }
      if(ap)
      {
        authdata = dllist_append(authdata, (dllist_t)ap);
        ap = NULL;
      }
      new_section = TRUE;
    }

    if(*lns == '#' || !*lns)
      continue;

    if(new_section)
    {
      new_section = FALSE;
      ap = (authinfo *) _malloc(sizeof(authinfo));
      ap->prot = URLT_UNKNOWN;
      ap->host = NULL;
      ap->port = 0;
      ap->user = NULL;
      ap->pass = NULL;
      ap->base = NULL;
      ap->realm = NULL;
      ap->ntlm_domain = NULL;
      ap->type = HTTP_AUTH_BASIC;
    }

    found = FALSE;
    for(i = 0; i < NUM_ELEM(authidef); i++)
    {
      if(!strncasecmp(lns, authidef[i].id, strlen(authidef[i].id)))
      {
        lns += strlen(authidef[i].id);
        for(; *lns && tl_ascii_isspace(*lns); lns++);
        for(p = lns + strlen(lns); *p && tl_ascii_isspace(*p); p--)
          *p = '\0';
        found = TRUE;
        break;
      }
    }

    if(!found)
    {
      xprintf(0, gettext("Unable to parse : \"%s\"\n"), lns);
      continue;
    }

    switch (authidef[i].type)
    {
    case AUTHI_PROTO:
      found = FALSE;
      for(i = 0; i < NUM_ELEM(prottable); i++)
      {
        if(prottable[i].urlid && !strcasecmp(lns, prottable[i].urlid))
        {
          found = TRUE;
          ap->prot = prottable[i].id;
          break;
        }
      }
      if(!found)
      {
        xprintf(0, gettext("Bad protocol - %s\n"), lns);
      }
      break;
    case AUTHI_HOST:
      p = strrchr(lns, ':');
      ap->host = p ? tl_strndup(lns, p - lns) : tl_strdup(lns);
      if(p)
      {
        ap->port = _atoi(p + 1);
        if(errno == ERANGE)
        {
          xprintf(0, gettext("Bad port number %s\n"), p);
        }
      }
      break;
    case AUTHI_USER:
      ap->user = tl_strdup(lns);
      break;
    case AUTHI_PASS:
      ap->pass = tl_strdup(lns);
      break;
    case AUTHI_BASE:
      ap->base = tl_strdup(lns);
      break;
    case AUTHI_REALM:
      ap->realm = tl_strdup(lns);
      break;
    case AUTHI_TYPE:
      ap->type = authinfo_get_type(lns);
      _atoi(lns);
      if(ap->type == HTTP_AUTH_NONE)
      {
        xprintf(0, gettext("Unknown auth type - %s\n"), lns);
        ap->type = HTTP_AUTH_BASIC;
      }
      break;
    case AUTHI_NTLM_DOMAIN:
      ap->ntlm_domain = tl_strdup(lns);
      break;
    }
  }
  bufio_close(fd);
  UNLOCK_AUTHINFO;
  return 0;
}

int authinfo_save(char *fn)
{
  int fd;
  char pom[4096];
  dllist *ptr;

  LOCK_AUTHINFO;
  fd = open(fn, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if(fd < 0)
  {
    xperror(fn);
    UNLOCK_AUTHINFO;
    return -1;
  }

  ptr = authdata;
  while(ptr)
  {
    authinfo *ai = (authinfo *) ptr->data;

    snprintf(pom, sizeof(pom), "%s %s\n",
      authidef[AUTHI_PROTO].id, prottable[ai->prot].urlid);
    write(fd, pom, strlen(pom));

    snprintf(pom, sizeof(pom), "%s %s:%d\n",
      authidef[AUTHI_HOST].id, ai->host, ai->port);
    write(fd, pom, strlen(pom));

    if(ai->user)
    {
      snprintf(pom, sizeof(pom), "%s %s\n",
        authidef[AUTHI_USER].id, ai->user);
      write(fd, pom, strlen(pom));
    }

    if(ai->pass)
    {
      snprintf(pom, sizeof(pom), "%s %s\n",
        authidef[AUTHI_PASS].id, ai->pass);
      write(fd, pom, strlen(pom));
    }

    if(ai->base)
    {
      snprintf(pom, sizeof(pom), "%s %s\n",
        authidef[AUTHI_BASE].id, ai->base);
      write(fd, pom, strlen(pom));
    }

    if(ai->realm)
    {
      snprintf(pom, sizeof(pom), "%s %s\n",
        authidef[AUTHI_REALM].id, ai->realm);
      write(fd, pom, strlen(pom));
    }

    if(ai->prot == URLT_HTTP || ai->prot == URLT_HTTPS)
    {
      snprintf(pom, sizeof(pom), "%s %s\n",
        authidef[AUTHI_TYPE].id, http_auths[ai->type].name);
      write(fd, pom, strlen(pom));
    }


    write(fd, "\n", 1);
    ptr = ptr->next;
  }

  close(fd);
  UNLOCK_AUTHINFO;
  return 0;
}

authinfo *authinfo_match_entry(protocol prot, char *host, int port,
  char *path, char *realm)
{
  int mlen = -1, len;
  authinfo *ret = NULL;
  authinfo *ai = NULL;
  dllist *ptr;

  LOCK_AUTHINFO;
  ptr = authdata;
  while(ptr)
  {
    ai = (authinfo *) ptr->data;

    if(ai->prot == prot && !strcmp(ai->host, host) && port == ai->port)
    {
      if(realm && ai->realm && !strcmp(realm, ai->realm))
      {
        ret = ai;
        break;
      }
      if((ai->base &&
          !strncmp(path, ai->base, strlen(ai->base))) || !ai->base)
      {
        if(ai->base)
          len = strlen(ai->base);
        else
          len = 0;

        if(len > mlen)
        {
          ret = ai;
          mlen = len;
        }
      }
    }
    ptr = ptr->next;
  }

  UNLOCK_AUTHINFO;
  return ret;
}
