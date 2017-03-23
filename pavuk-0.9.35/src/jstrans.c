/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "jstrans.h"
#include "tools.h"

#ifdef HAVE_REGEX

void js_transform_free(js_transform_t *jt)
{
  re_free(jt->re);
  _free(jt->transform);
  _free(jt->tag);
  _free(jt->attrib);
  _free(jt);
}

js_transform_t *js_transform_new(const char *pattern, const char *transform,
const char *tag, const char *attrib, int type)
{
  js_transform_t *rv;
  re_entry *re;

  if(!pattern || !transform || !tag || !attrib || !*pattern || !*transform)
    return NULL;

  re = re_make(pattern);

  if(!re)
    return NULL;

  rv = _malloc(sizeof(js_transform_t));
  rv->type = type;
  rv->re = re;
  rv->transform = tl_strdup(transform);
  rv->tag = tl_strdup(tag);
  rv->attrib = tl_strdup(attrib);

  return rv;
}

int js_transform_match_tag(js_transform_t *jt, const char *tag)
{
  int l;

  if(jt->tag[0] == '*')
    return TRUE;

  l = strcspn(tag + 1, " \t\r\n>");

  if(!strncasecmp(tag + 1, jt->tag, l) && (l == strlen(jt->tag)))
    return TRUE;

  return FALSE;
}

char *js_transform_apply(js_transform_t *jt, const char *attr, int nsub,
int *subs)
{
  char *rv;
  int n, l;

  l = strcspn(jt->transform, "$");
  rv = tl_strndup(jt->transform, l);

  do
  {
    if(jt->transform[l] == '$')
    {
      n = atoi(jt->transform + l + 1);
      if(n <= nsub)
      {
        rv = tl_str_nappend(rv, attr + subs[2 * n],
          subs[2 * n + 1] - subs[2 * n]);
      }
      l += 1 + strspn(jt->transform + l + 1, "0123456789");
    }
    n = strcspn(jt->transform + l, "$");
    rv = tl_str_nappend(rv, jt->transform + l, n);
    l += n;
  }
  while(jt->transform[l]);

  return rv;
}

#endif
