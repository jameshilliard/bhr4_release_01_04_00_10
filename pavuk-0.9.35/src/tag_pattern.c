/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <string.h>
#include "tag_pattern.h"
#include "tools.h"

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "fnmatch.h"
#endif

tag_pattern_t *tag_pattern_new(int type, char *tag, char *attrib, char *urlp)
{
  tag_pattern_t *rv;
  int r[3];

  rv = _malloc(sizeof(tag_pattern_t));
  memset(rv, '\0', sizeof(tag_pattern_t));

  rv->type = type;

  if(type == TAGP_WC)
  {
    r[0] = fnmatch(tag, "", 0);
    if(r[0] != 0 && r[0] != FNM_NOMATCH)
    {
      xperror(tag);
      xprintf(0, gettext("Bad -tag_pattern tag wildcard pattern\n"));
    }

    r[1] = fnmatch(attrib, "", 0);
    if(r[1] != 0 && r[1] != FNM_NOMATCH)
    {
      xperror(rv->attrib);
      xprintf(0, gettext("Bad -tag_pattern attribute wildcard pattern\n"));
    }

    r[2] = fnmatch(urlp, "", 0);
    if(r[2] != 0 && r[2] != FNM_NOMATCH)
    {
      xperror(rv->attrib);
      xprintf(0, gettext("Bad -tag_pattern url wildcard pattern\n"));
    }

    if((r[0] != 0 && r[0] != FNM_NOMATCH) ||
      (r[1] != 0 && r[1] != FNM_NOMATCH) ||
      (r[2] != 0 && r[2] != FNM_NOMATCH))
    {
      tag_pattern_free(rv);
      rv = NULL;
    }
  }
#ifdef HAVE_REGEX
  else
  {
    if(!(rv->tag_re = re_make(tag)))
      xprintf(0, gettext("Bad -tag_rpattern tag RE pattern\n"));
    if(!(rv->attrib_re = re_make(attrib)))
      xprintf(0, gettext("Bad -tag_rpattern attribute RE pattern\n"));
    if(!(rv->urlp_re = re_make(urlp)))
      xprintf(0, gettext("Bad -tag_rpattern url RE pattern\n"));

    if(!rv->tag_re || !rv->attrib_re || !rv->urlp_re)
    {
      tag_pattern_free(rv);
      rv = NULL;
    }
  }
#endif

  if(rv)
  {
    rv->tag = tl_strdup(tag);
    rv->attrib = tl_strdup(attrib);
    rv->urlp = tl_strdup(urlp);
  }

  return rv;
}

void tag_pattern_free(tag_pattern_t *tp)
{
  _free(tp->tag);
  _free(tp->attrib);
  _free(tp->urlp);

#ifdef HAVE_REGEX
  if(tp->type == TAGP_RE)
  {
    if(tp->tag_re)
      re_free(tp->tag_re);
    if(tp->attrib_re)
      re_free(tp->attrib_re);
    if(tp->urlp_re)
      re_free(tp->urlp_re);
  }
#endif

  _free(tp);
}

int tag_pattern_match(tag_pattern_t *tp, char *tag, char *attrib, char *urlp)
{
  if(tp->type == TAGP_WC)
  {
    return (!fnmatch(tp->tag, tag, 0) &&
      !fnmatch(tp->attrib, attrib, 0) && !fnmatch(tp->urlp, urlp, 0));
  }
#ifdef HAVE_REGEX
  else
  {
    return (re_pmatch(tp->tag_re, tag) &&
      re_pmatch(tp->attrib_re, attrib) && re_pmatch(tp->urlp_re, urlp));
  }
#endif

  return FALSE;
}
