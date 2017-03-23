/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "doc.h"
#include "mime.h"
#include "mimetype.h"
#include "tools.h"

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "fnmatch.h"
#endif

/*********************************************/
/* oddelenie MIME hlavicky od tela dokumentu */
/* FIXME: Translate me!                      */
/*********************************************/
void split_mime_doc(doc * docp)
{
  char *p1, *p2;
  char *doc_c;
  int mime_size;

  p1 = strstr(docp->contents, "\n\n");
  p2 = strstr(docp->contents, "\r\n\r\n");

  if(p2 && ((p2 < p1) || !p1))
  {
    doc_c = p2 + 4;
    mime_size = p2 - docp->contents;
  }
  else if(p1)
  {
    doc_c = p1 + 2;
    mime_size = p1 - docp->contents;
  }
  else
  {
    doc_c = docp->contents;
    mime_size = 0;
  }

  p1 = docp->contents;

  docp->size = docp->size - (doc_c - docp->contents);

  docp->contents = (char *) _malloc(docp->size);
  memcpy(docp->contents, doc_c, docp->size);

  docp->mime = NULL;
  if(mime_size)
  {
    docp->mime = (char *) _malloc(mime_size + 3);
    strncpy(docp->mime, p1, mime_size + 2);
    *(docp->mime + mime_size + 2) = '\0';
  }
  free(p1);
}

/************************************************/
/* vrati n-tu hodnotu atributu z MIME hlavicky  */
/* FIXME: Translate me!                         */
/************************************************/
char *get_mime_n_param_val_str(char *param_id, char *mimet, int n)
{
  char *p;
  int i = 0;
  int ilen;

  if(!mimet)
    return NULL;

  p = mimet;

  while(*p)
  {
    ilen = strcspn(p, "\r\n");

    if(!strncasecmp(p, param_id, strlen(param_id)))
    {
      if(i == n)
      {
        char *p1;
        p1 = p + strlen(param_id);
        while(tl_ascii_isspace(*p1))
          p1++;
        p += ilen;
        while(*p)
        {
          p += strspn(p, "\n\r");
          ilen = strcspn(p, "\r\n");
          if((*p == ' ' || *p == '\t'))
          {
            p += ilen;
          }
          else
            break;
        }
        p1 = tl_strndup(p1, p - p1);
        omit_chars(p1, "\r\n");
        return p1;
      }
      i++;
    }
    p += ilen;
    p += strspn(p, "\n\r");
  }
  return NULL;
}

const char *mime_get_type_ext(const char *type)
{
  const struct mime_type_ext *p;

  if(!type)
    return NULL;

  for(p = mime_type_exts; p->mimet; ++p)
  {
    if(!fnmatch(p->mimet, type, 0))
    {
      return p->ext;
    }
  }
  return NULL;
}
