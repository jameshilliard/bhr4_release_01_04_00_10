/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "css.h"
#include "url.h"
#include "tools.h"
#include "mode.h"

#define BUFFER        1024
#define MEXPAND(sv) \
  if((sz - (sv + r + BUFFER)) < 0) \
  { \
    result = _realloc(result, sz + sv  + BUFFER); \
    sz += BUFFER + sv; \
  }

#define SEXPAND(sv) \
  if((ssz - (sv + sr + BUFFER)) < 0) \
  { \
    stack = _realloc(stack, ssz + sv  + BUFFER); \
    ssz += BUFFER + sv; \
  }

/* FIXME: Security (the whole file) ! */

dllist *css_get_all_links(url * doc_url, char *stylestr, char *base,
  char *baset, int no_limits)
{
  bool_t urlstart = FALSE;
  char *p;
  char *stack;
  char *sp;
  int sr, ssz;
  dllist *retval = NULL;

  ssz = 2 * BUFFER;
  stack = _malloc(ssz);
  sr = 0;

  for(p = stylestr; *p; p++)
  {
    if(urlstart)
    {
      if(*p == ')')
      {
        char *pom;

        stack[sr] = '\0';

        sp = stack + sr - 1;
        while(tl_ascii_isspace(*sp) && sp > stack)
        {
          *sp = '\0';
          sp--;
        }
        if(*sp == '\"' || *sp == '\'')
          *sp = '\0';
        sp = stack;
        while(tl_ascii_isspace(*sp))
          sp++;
        if(*sp == '\"' || *sp == '\'')
          sp++;

        pom = url_to_absolute_url(base, baset, doc_url, sp);

        if(pom)
        {
          url *purl;
          cond_info_t condp;

          condp.level = 0;
          condp.urlnr = 0;
          condp.size = 0;
          condp.time = 0L;
          condp.mimet = NULL;
          condp.full_tag = NULL;
          condp.params = NULL;
          condp.html_doc = stylestr;
          condp.html_doc_offset = p - stylestr;
          condp.tag = NULL;
          condp.attrib = NULL;

          purl = url_parse(pom);
          assert(purl->type != URLT_FROMPARENT);
          purl->parent_url = dllist_append(purl->parent_url, (dllist_t)doc_url);
          purl->level = doc_url->level + 1;
          purl->status |= URL_INLINE_OBJ;
          url_path_abs(purl);

          if((cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR) && cfg.request
            && (purl->type == URLT_FILE))
          {
            url *pomurl = filename_to_url(purl->p.file.filename);

            if(pomurl)
            {
              free_deep_url(purl);
              _free(purl);
              purl = pomurl;
            }
          }

          if(no_limits || url_append_condition(purl, &condp))
          {
            retval = dllist_append(retval, (dllist_t)purl);
          }
          else
          {
            free_deep_url(purl);
            _free(purl);
          }
          free(pom);
        }
        urlstart = FALSE;
      }
      else
      {
        SEXPAND(1);
        stack[sr] = *p;
        sr++;
      }
      continue;
    }

    if(((p - stylestr) > 3) && !strncasecmp(p - 3, "url(", 4))
    {
      urlstart = TRUE;
      sr = 0;
    }
  }

  _free(stack);
  return retval;
}

char *css_to_absolute_links(url * doc_url, char *stylestr, char *base,
  char *baset)
{
  bool_t urlstart = FALSE;
  char *result;
  int sz, r;
  char *p;
  char *sp;
  char *stack;
  int ssz, sr, l;

  sz = strlen(stylestr) + BUFFER;
  result = _malloc(sz);
  r = 0;

  ssz = 2 * BUFFER;
  stack = _malloc(ssz);
  sr = 0;

  for(p = stylestr; *p; p++)
  {
    if(urlstart)
    {
      if(*p == ')')
      {
        char *pom;

        stack[sr] = '\0';

        sp = stack + sr - 1;
        while(tl_ascii_isspace(*sp) && sp > stack)
        {
          *sp = '\0';
          sp--;
        }
        if(*sp == '\"' || *sp == '\'')
          *sp = '\0';
        sp = stack;
        while(tl_ascii_isspace(*sp))
          sp++;
        if(*sp == '\"' || *sp == '\'')
          sp++;

        pom = url_to_absolute_url(base, baset, doc_url, sp);

        if(pom)
        {
          url *purl;

          purl = url_parse(pom);
          assert(purl->type != URLT_FROMPARENT);
          _free(pom);
          url_path_abs(purl);
          pom = url_to_urlstr(purl, TRUE);
          free_deep_url(purl);
          _free(purl);
        }
        else
          pom = tl_strdup(sp);

        l = strlen(pom);
        MEXPAND(l);
        result[r] = '\'';
        r++;
        strcpy(result + r, pom);
        r += l;
        result[r] = '\'';
        r++;
        result[r] = ')';
        r++;

        urlstart = FALSE;

        _free(pom);
      }
      else
      {
        SEXPAND(1);
        stack[sr] = *p;
        sr++;
      }
      continue;
    }

    if(((p - stylestr) > 3) && !strncasecmp(p - 3, "url(", 4))
    {
      urlstart = TRUE;
      sr = 0;
    }
    MEXPAND(1);
    result[r] = *p;
    r++;
  }
  result[r] = '\0';

  _free(stack);

  return result;
}

char *css_remote_to_local_links(url * doc_url, char *stylestr, int all,
  int sel, char *base, char *baset)
{
  bool_t urlstart = FALSE;
  char *result;
  int sz, r;
  char *p;
  char *act_name = url_to_filename(doc_url, TRUE);
  char *anchor;
  char *sp;
  char *stack;
  int ssz, sr, l;

  sz = strlen(stylestr) + BUFFER;
  result = _malloc(sz);
  r = 0;

  ssz = 2 * BUFFER;
  stack = _malloc(ssz);
  sr = 0;

  for(p = stylestr; *p; p++)
  {
    if(urlstart)
    {
      if(*p == ')')
      {
        char *fn;
        url *urlp;
        url *purl;
        struct stat estat;

        stack[sr] = '\0';

        sp = stack + sr - 1;
        while(tl_ascii_isspace(*sp) && sp > stack)
        {
          *sp = '\0';
          sp--;
        }
        if(*sp == '\"' || *sp == '\'')
          *sp = '\0';
        sp = stack;
        while(tl_ascii_isspace(*sp))
          sp++;
        if(*sp == '\"' || *sp == '\'')
          sp++;

        urlp = url_parse(sp);
        assert(urlp->type != URLT_FROMPARENT);
        if(all && urlp->type == URLT_FILE)
        {
          fn = url_to_absolute_url(base, baset, doc_url, sp);
          free_deep_url(urlp);
          _free(urlp);
          urlp = url_parse(fn);
          assert(urlp->type != URLT_FROMPARENT);
        }
        anchor = url_get_anchor_name(urlp);

        /** for better performance with info files **/
        if((purl = url_was_befor(urlp)))
          fn = url_to_filename(purl, TRUE);
        else
          fn = url_to_filename(urlp, FALSE);

        char* pom = _malloc(strlen(sp) + 1);
        strcpy(pom, sp);

        if(((all || (sel && purl)) && fn) || (!sel &&
            (((urlp->type != URLT_FILE) && fn && ((!access(fn, R_OK) &&
                    !stat(fn, &estat)) || !strcmp(act_name, fn))))))
        {
          if(all || (sel && purl) || (!sel && ((!access(fn, R_OK)
                  && !S_ISDIR(estat.st_mode)) || !strcmp(act_name, fn))))
          {
            char *relfn = get_relative_path(act_name, fn);
            
            _free(pom);
            
            if(anchor) {
              pom = _malloc(strlen(relfn) + strlen(anchor) + 100);
              sprintf(pom, "%s#%s", relfn, anchor);
            }
            else {
              pom = _malloc(strlen(relfn) + 100);
              strcpy(pom, relfn);
            }

            _free(relfn);
          }
          else
          {
            _free(pom);
            pom = _malloc(strlen(fn) + strlen(priv_cfg.index_name) + 100);
            sprintf(pom, "%s/%s", fn, priv_cfg.index_name);

            if(!stat(pom, &estat) && !S_ISDIR(estat.st_mode))
            {
              char *relfn = get_relative_path(act_name, pom);

              _free(pom);
              
              if(anchor) {
                pom = _malloc(strlen(relfn) + strlen(anchor) + 100);
                sprintf(pom, "%s#%s", relfn, anchor);
              }
              else {
                pom = _malloc(strlen(relfn) + 100);
                strcpy(pom, relfn);
              }

              free(relfn);
            }
          }
        }
        free_deep_url(urlp);
        _free(urlp);

        l = strlen(pom);
        MEXPAND(l);
        result[r] = '\'';
        r++;
        strcpy(result + r, pom);
        r += l;
        result[r] = '\'';
        r++;
        result[r] = ')';
        r++;

        urlstart = FALSE;

        _free(pom);
      }
      else
      {
        SEXPAND(1);
        stack[sr] = *p;
        sr++;
      }
      continue;
    }

    if(((p - stylestr) > 3) && !strncasecmp(p - 3, "url(", 4))
    {
      urlstart = TRUE;
      sr = 0;
    }
    MEXPAND(1);
    result[r] = *p;
    r++;
  }
  result[r] = '\0';

  _free(stack);

  return result;
}

char *css_change_url(url * doc_url, char *stylestr, url * url_old,
  char *url_new)
{
  bool_t urlstart = FALSE;
  char *result;
  int sz, r;
  char *p;
  char pom[4096];
  char *anchor;
  char *oldfn;
  char *sp;
  char *stack;
  int ssz, sr, l;

  sz = strlen(stylestr) + BUFFER;
  result = _malloc(sz);
  r = 0;

  ssz = 2 * BUFFER;
  stack = _malloc(ssz);
  sr = 0;

  oldfn = url_to_urlstr(url_old, FALSE);

  for(p = stylestr; *p; p++)
  {
    if(urlstart)
    {
      if(*p == ')')
      {
        char *pfn;
        url *urlp;

        stack[sr] = '\0';

        sp = stack + sr - 1;
        while(tl_ascii_isspace(*sp) && sp > stack)
        {
          *sp = '\0';
          sp--;
        }
        if(*sp == '\"' || *sp == '\'')
          *sp = '\0';
        sp = stack;
        while(tl_ascii_isspace(*sp))
          sp++;
        if(*sp == '\"' || *sp == '\'')
          sp++;

        pfn = NULL;
        urlp = url_parse(sp);
        assert(urlp->type != URLT_FROMPARENT);
        anchor = url_get_anchor_name(urlp);

        strcpy(pom, sp);

        if(urlp && (urlp->type != URLT_FILE) &&
          (pfn = url_to_urlstr(urlp, FALSE)))
        {
          if(!strcmp(pfn, oldfn))
          {
            if(anchor)
              sprintf(pom, "%s#%s", url_new, anchor);
            else
              strcpy(pom, url_new);
          }
        }

        free_deep_url(urlp);
        free(urlp);
        _free(pfn);

        l = strlen(pom);
        MEXPAND(l);
        result[r] = '\'';
        r++;
        strcpy(result + r, pom);
        r += l;
        result[r] = '\'';
        r++;
        result[r] = ')';
        r++;

        urlstart = FALSE;
      }
      else
      {
        SEXPAND(1);
        stack[sr] = *p;
        sr++;
      }
      continue;
    }

    if(((p - stylestr) > 3) && !strncasecmp(p - 3, "url(", 4))
    {
      urlstart = TRUE;
      sr = 0;
    }
    MEXPAND(1);
    result[r] = *p;
    r++;
  }
  result[r] = '\0';

  _free(stack);

  return result;
}
