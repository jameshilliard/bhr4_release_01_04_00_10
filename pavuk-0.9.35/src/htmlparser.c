/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "htmlparser.h"

#include "tools.h"
#include "css.h"
#include "re.h"
#include "ftp.h"
#include "jstrans.h"

static dlhash *html_parser_tag_hash = NULL;

#define COMMENT_PREFIX "<!-- "
#define COMMENT_SUFFIX " -->"
#define ADVERT_PREFIX "adv<!-- Removed by pavuk "
#define ADVERT_SUFFIX " -->"

static int html_parser_tag_comp_func(dllist_t key1, dllist_t key2)
{
  return (!strcasecmp((void *) key1, (void *) key2));
}

static unsigned int html_parser_tag_hash_func(unsigned int size, dllist_t key)
{
  unsigned char *p = (unsigned char *) key;
  unsigned int retv = 0;

  while(*p)
  {
    retv = (retv + tl_ascii_tolower(*p)) % size;
    p++;
  }

  return retv;
}

static dllist_t html_parser_tag_key_func(dllist_t data)
{
  return (dllist_t) ((html_tag_t *) data)->tag;
}

html_parser_t *html_parser_init(html_tag_t * tags, int ntags,
  int with_tag_rewriting, int purestyle, int purescript)
{
  html_parser_t *rv;
  int i;

  rv = _malloc(sizeof(html_parser_t));

  rv->rewrite = with_tag_rewriting;
  rv->purestyle = purestyle;
  rv->purescript = purescript;
  rv->in_content = NULL;
  rv->out_content = NULL;
  rv->in_size = 0;
  rv->aout_size = 0;
  rv->out_offset = 0;
  rv->in_offset = 0;

  rv->stack = NULL;
  rv->stack_size = 0;
  rv->stack_offset = 0;

  rv->base = NULL;
  rv->baset = NULL;

  rv->tag_attrib = NULL;

  rv->tag_funcs = NULL;
  rv->attrib_funcs = NULL;
  rv->style_funcs = NULL;
  rv->script_funcs = NULL;

  rv->current_tag = NULL;
  rv->current_attrib = NULL;

  LOCK_TAG_HASH;
  if(html_parser_tag_hash)
    rv->tag_hash = html_parser_tag_hash;
  else
  {

    rv->tag_hash = dlhash_new(20,
      html_parser_tag_key_func,
      html_parser_tag_hash_func, html_parser_tag_comp_func);

    for(i = 0; i < ntags; i++)
      dlhash_insert(rv->tag_hash, (dllist_t) &tags[i]);

    html_parser_tag_hash = rv->tag_hash;

  }
  UNLOCK_TAG_HASH;

  return rv;
}

void html_parser_do_cleanup(void)
{
  if(html_parser_tag_hash)
    dlhash_free(html_parser_tag_hash);
}

void html_parser_kill(html_parser_t * hpinfo)
{
#define KILL_FUNC_CHAIN(chain) \
  while (chain) \
  { \
    if(chain->data) free((void *)chain->data);\
    chain = dllist_remove_entry(chain, chain);\
  }

  KILL_FUNC_CHAIN(hpinfo->tag_funcs);
  KILL_FUNC_CHAIN(hpinfo->attrib_funcs);
  KILL_FUNC_CHAIN(hpinfo->style_funcs);
  KILL_FUNC_CHAIN(hpinfo->script_funcs);

  _free(hpinfo->stack);
  _free(hpinfo->out_content);

  _free(hpinfo->base);
  _free(hpinfo->baset);
  _free(hpinfo);
}

void html_parser_add_tag_func(html_parser_t * hpinfo, html_parser_func_t func,
  void *data)
{
  html_parser_func_info_t *nfunc;

  nfunc = _malloc(sizeof(html_parser_func_info_t));
  nfunc->func = func;
  nfunc->data = data;
  hpinfo->tag_funcs = dllist_append(hpinfo->tag_funcs, (dllist_t) nfunc);
}

void html_parser_add_attrib_func(html_parser_t * hpinfo,
  html_parser_func_t func, void *data)
{
  html_parser_func_info_t *nfunc;

  nfunc = _malloc(sizeof(html_parser_func_info_t));
  nfunc->func = func;
  nfunc->data = data;
  hpinfo->attrib_funcs = dllist_append(hpinfo->attrib_funcs, (dllist_t) nfunc);
}

void html_parser_add_style_func(html_parser_t * hpinfo,
  html_parser_func_t func, void *data)
{
  html_parser_func_info_t *nfunc;

  nfunc = _malloc(sizeof(html_parser_func_info_t));
  nfunc->func = func;
  nfunc->data = data;
  hpinfo->style_funcs = dllist_append(hpinfo->style_funcs, (dllist_t) nfunc);
}

void html_parser_add_script_func(html_parser_t * hpinfo,
  html_parser_func_t func, void *data)
{
  html_parser_func_info_t *nfunc;

  nfunc = _malloc(sizeof(html_parser_func_info_t));
  nfunc->func = func;
  nfunc->data = data;
  hpinfo->script_funcs = dllist_append(hpinfo->script_funcs, (dllist_t) nfunc);
}

void html_parser_set_document(html_parser_t * hpinfo, url * doc_url,
  char *content, ssize_t size)
{
  hpinfo->doc_url = doc_url;
  hpinfo->in_content = content;
  hpinfo->in_size = size;
}

void html_parser_take_document(html_parser_t * hpinfo, char **out_content,
  ssize_t * out_size)
{
  *out_content = hpinfo->out_content;
  *out_size = hpinfo->out_offset;

  hpinfo->out_content = NULL;
  hpinfo->out_offset = 0;
  hpinfo->aout_size = 0;
}

void html_parser_set_base(html_parser_t * hpinfo, char *base, char *baset)
{
  if(base)
  {
    _free(hpinfo->base);
    hpinfo->base = base;
  }

  if(baset)
  {
    _free(hpinfo->baset);
    hpinfo->baset = baset;
  }
}

static void html_parser_process_new_base_url(html_parser_t * hpinfo,
  char *baseattr)
{
  url *purl;
  char *newbase;

  purl = url_parse(baseattr);
  assert(purl->type != URLT_FROMPARENT);

  if(!prottable[purl->type].supported)
  {
    xprintf(1, gettext("Unsupported BASE URL -  %s (probably bad handled)\n"),
      baseattr);
    newbase = tl_strdup(baseattr);
  }
  else
  {
    char *idx;

    newbase =
      url_to_absolute_url(hpinfo->base, hpinfo->baset, hpinfo->doc_url,
      baseattr);

    if(!newbase) return; /* collect base="" and ignore it */

    if((idx = strrchr(newbase, '?')))
      *idx = '\0';
    if(!tl_is_dirname(newbase))
    {
      idx = strrchr(newbase, '/');
      if(idx)
        *(idx + 1) = '\0';
    }
  }
  DEBUG_HTML("NEW BASE URL - %s\n", newbase);

  free_deep_url(purl);
  _free(purl);
  _free(hpinfo->base);
  hpinfo->base = newbase;
}

static void html_parser_call_funcs(html_parser_t * hpinfo, dllist * funcs)
{
  dllist *ptr;

  for(ptr = funcs; ptr; ptr = ptr->next)
  {
    html_parser_func_info_t *fi = (html_parser_func_info_t *) ptr->data;

    fi->func(hpinfo, hpinfo->stack, fi->data);
  }
}

static void html_parser_flush_stack_to_output(html_parser_t * hpinfo)
{
  int l;

  if(!hpinfo->rewrite)
    return;

  l = strlen(hpinfo->stack);
  html_parser_MEXPAND(hpinfo, l)
    memcpy(hpinfo->out_content + hpinfo->out_offset, hpinfo->stack, l);
  hpinfo->out_offset += l;
  hpinfo->stack_offset = 0;
}

static int html_parser_check_tag(html_parser_t * hpinfo, char *tagstart)
{
  int tl;

  hpinfo->current_tag = NULL;

  for(tl = 0; tl_ascii_isalpha(tagstart[tl]); tl++);

  if(strchr(" \t\r\n>", tagstart[tl]))
  {
    char *tagname;

    tagname = tl_strndup(tagstart, tl);
    hpinfo->current_tag = (html_tag_t *) dlhash_find_by_key(hpinfo->tag_hash,
    (dllist_t) tagname);
    _free(tagname);
  }
  return (hpinfo->current_tag != NULL);
}

static void html_parser_parse_init(html_parser_t * hpinfo)
{
  hpinfo->in_offset = 0;
  if(hpinfo->rewrite)
  {
    hpinfo->aout_size = hpinfo->in_size + html_parser_FENDER;
    hpinfo->out_content = _malloc(hpinfo->aout_size);
    hpinfo->out_offset = 0;
  }

  hpinfo->stack_size = 2 * html_parser_FENDER;
  hpinfo->stack = _malloc(hpinfo->stack_size);
  hpinfo->stack_offset = 0;
}

void html_parser_parse(html_parser_t * hpinfo)
{
  int tagstart = FALSE;
  int scriptstart = FALSE;
  int commentstart = FALSE;
  int stylestart = FALSE;
  int singlequoteintag = FALSE;
  int doublequoteintag = FALSE;
  char *p;

  html_parser_parse_init(hpinfo);

  if(hpinfo->purestyle)
    stylestart = TRUE;

  if(hpinfo->purescript)
    scriptstart = TRUE;

  for(p = hpinfo->in_content; (p - hpinfo->in_content) < hpinfo->in_size;
    p++, hpinfo->in_offset++)
  {
    if(stylestart)
    {
      if(!strncasecmp(p, "</STYLE", 7))
      {
        stylestart = FALSE;

        hpinfo->stack[hpinfo->stack_offset] = *p;
        hpinfo->stack[hpinfo->stack_offset + 1] = '\0';

        html_parser_call_funcs(hpinfo, hpinfo->style_funcs);
        html_parser_flush_stack_to_output(hpinfo);
      }
      else
      {
        html_parser_SEXPAND(hpinfo, 1)
          hpinfo->stack[hpinfo->stack_offset] = *p;
        hpinfo->stack_offset++;
      }

      continue;
    }

    if(scriptstart)
    {
      if(!strncasecmp(p + 1, "</SCRIPT", 8))
      {
        scriptstart = FALSE;

        hpinfo->stack[hpinfo->stack_offset] = *p;
        hpinfo->stack[hpinfo->stack_offset + 1] = '\0';

        html_parser_call_funcs(hpinfo, hpinfo->script_funcs);
        html_parser_flush_stack_to_output(hpinfo);
      }
      else
      {
        html_parser_SEXPAND(hpinfo, 1)
          hpinfo->stack[hpinfo->stack_offset] = *p;
        hpinfo->stack_offset++;
      }

      continue;
    }

    if(commentstart)
    {
      if(!strncmp(p, "-->", 3))
        commentstart = FALSE;

      if(hpinfo->rewrite)
      {
        hpinfo->out_content[hpinfo->out_offset] = *p;
        hpinfo->out_offset++;
      }
      continue;
    }

    if((*p == '\"') && tagstart && !singlequoteintag)
    {
      if(doublequoteintag)
      {
        doublequoteintag = FALSE;
      }
      else
      {
        doublequoteintag = TRUE;
      }
    }
    else if((*p == '\'') && tagstart && !doublequoteintag)
    {
      if(singlequoteintag)
      {
        singlequoteintag = FALSE;
      }
      else
      {
        singlequoteintag = TRUE;
      }
    }
    else if(*p == '<')
    {
      if(singlequoteintag || doublequoteintag)
      {
        continue;
      }
      if(tagstart)
      {
        hpinfo->stack[hpinfo->stack_offset] = '\0';
        html_parser_flush_stack_to_output(hpinfo);
      }
      tagstart = FALSE;

      if(!strncasecmp(p, "<STYLE", 6))
      {
        stylestart = TRUE;
        hpinfo->stack_offset = 0;
      }
      else if(!strncmp(p, "<!--", 4))
      {
        commentstart = TRUE;
      }
      else
      {
        hpinfo->stack_offset = 0;
        tagstart = TRUE;
        singlequoteintag = FALSE;
        doublequoteintag = FALSE;
      }
    }
    else if(*p == '>' && tagstart)
    {
      if(singlequoteintag || doublequoteintag)
      {
        continue;
      }
      hpinfo->stack[hpinfo->stack_offset] = *p;
      hpinfo->stack[hpinfo->stack_offset + 1] = '\0';
      html_parser_call_funcs(hpinfo, hpinfo->tag_funcs);
      html_parser_flush_stack_to_output(hpinfo);

      if(hpinfo->current_tag &&
        hpinfo->current_tag->type == HTML_TAG_SCRIPT &&
        !html_tag_co_elem(hpinfo->stack, "SRC"))
      {
        scriptstart = TRUE;
      }

      tagstart = FALSE;
      singlequoteintag = FALSE;
      doublequoteintag = FALSE;
      continue;
    }

    if(tagstart || stylestart || scriptstart)
    {
      hpinfo->stack[hpinfo->stack_offset] = *p;
      hpinfo->stack_offset++;
      html_parser_SEXPAND(hpinfo, 1);
    }
    else
    {
      if(hpinfo->rewrite)
      {
        hpinfo->out_content[hpinfo->out_offset] = *p;
        hpinfo->out_offset++;
      }
    }
  }

  /* pure style don't need to end with </STYLE>   */
  /* so we must parse CSS also at end of document */
  if(stylestart && hpinfo->purestyle)
  {
    stylestart = FALSE;

    hpinfo->stack[hpinfo->stack_offset] = *p;
    hpinfo->stack[hpinfo->stack_offset + 1] = '\0';

    html_parser_call_funcs(hpinfo, hpinfo->style_funcs);
    html_parser_flush_stack_to_output(hpinfo);
  }

  /* pure script don't need to end with </SCRIPT> so we */
  /* must parse script patterns also at end of document */
  if(scriptstart && hpinfo->purescript)
  {
    scriptstart = FALSE;

    hpinfo->stack[hpinfo->stack_offset] = *p;
    hpinfo->stack[hpinfo->stack_offset + 1] = '\0';

    html_parser_call_funcs(hpinfo, hpinfo->script_funcs);
    html_parser_flush_stack_to_output(hpinfo);
  }

  if(tagstart || stylestart)
  {
    hpinfo->stack[hpinfo->stack_offset] = '\0';
    html_parser_flush_stack_to_output(hpinfo);
  }

  if(hpinfo->rewrite)
    hpinfo->out_content[hpinfo->out_offset] = '\0';
}

/********************************************/
/* functions for processing whole HTML tags */
/********************************************/
void html_parser_parse_tag(html_parser_t * hpinfo, char *stack, void *data)
{
  int j;
  dllist *ptr;

  if(!html_parser_check_tag(hpinfo, hpinfo->stack + 1))
    return;

  if(hpinfo->current_tag->type == HTML_TAG_META)
    return;

  for(j = 0; hpinfo->current_tag->attribs[j].attrib; j++)
  {
    hpinfo->current_attrib = &hpinfo->current_tag->attribs[j];

    if(hpinfo->current_attrib->stat & LINK_DISABLED)
      continue;

    hpinfo->tag_attrib = html_get_attrib_from_tag(hpinfo->stack,
      hpinfo->current_attrib->attrib);

    /*** -dont_touch_url_pattern support ***/
    if(hpinfo->tag_attrib && cfg.dont_touch_url_pattern)
    {
      if(is_in_pattern_list(hpinfo->tag_attrib, cfg.dont_touch_url_pattern))
      {
        _free(hpinfo->tag_attrib);
      }
    }

#ifdef HAVE_REGEX
    /*** -dont_touch_url_rpattern support ***/
    for(ptr = cfg.dont_touch_url_rpattern;
      ptr && hpinfo->tag_attrib; ptr = ptr->next)
    {
      if(re_pmatch((re_entry *) ptr->data, hpinfo->tag_attrib))
        _free(hpinfo->tag_attrib);
    }

    /*** -dont_touch_tag_rpattern support ***/
    for(ptr = cfg.dont_touch_tag_rpattern;
      ptr && hpinfo->tag_attrib; ptr = ptr->next)
    {
      if(re_pmatch((re_entry *)ptr->data, hpinfo->stack))
        _free(hpinfo->tag_attrib);
    }
#endif

    if(hpinfo->tag_attrib)
    {
      /* to support javascript:... URLs  */
      /* inside any attribute            */
      if(!strncasecmp(hpinfo->tag_attrib, "javascript:", 11))
      {
        char *saved_attrib = hpinfo->tag_attrib;

        hpinfo->tag_attrib = tl_strdup(saved_attrib + 11);
        html_parser_call_funcs(hpinfo, hpinfo->script_funcs);
        if(hpinfo->rewrite)
        {
          int len;

          len = strlen(hpinfo->tag_attrib);
          saved_attrib = _realloc(saved_attrib, 12 + len);
          memcpy(saved_attrib + 11, hpinfo->tag_attrib, len + 1);
          _free(hpinfo->tag_attrib);
          hpinfo->tag_attrib = saved_attrib;
        }
        else
          _free(saved_attrib);
      }
      else if(hpinfo->current_attrib->stat & LINK_STYLE)
        html_parser_call_funcs(hpinfo, hpinfo->style_funcs);
      else if(hpinfo->current_attrib->stat & LINK_JS)
        html_parser_call_funcs(hpinfo, hpinfo->script_funcs);
      else
        html_parser_call_funcs(hpinfo, hpinfo->attrib_funcs);
    }

    if(hpinfo->rewrite && hpinfo->tag_attrib)
    {
      int l = strlen(hpinfo->tag_attrib);

      html_parser_SEND(hpinfo);
      html_parser_SEXPAND(hpinfo, l);
      html_replace_url_in_stack(hpinfo->stack,
        hpinfo->current_attrib->attrib, hpinfo->tag_attrib, FALSE);
    }

    _free(hpinfo->tag_attrib);
  }
}

void html_parser_parse_tag_slash_a(html_parser_t * hpinfo, char *stack,
  html_extract_info_t * einfo)
{
  if(einfo->prev_a && !strcasecmp(hpinfo->stack, "</A>"))
  {
    einfo->prev_a = NULL;
  }
}

void html_parser_parse_tag_meta_refresh(html_parser_t * hpinfo, char *stack,
  void *data)
{
  char *saved_meta = (char *) 0;
  char *meta_type;

  if(!hpinfo->current_tag || hpinfo->current_tag->type != HTML_TAG_META)
    return;

  hpinfo->current_attrib = &hpinfo->current_tag->attribs[0];

  meta_type = html_get_attrib_from_tag(hpinfo->stack, "HTTP-EQUIV");

  if(!meta_type || strcasecmp(meta_type, "Refresh"))
  {
    _free(meta_type);
    return;
  }
  _free(meta_type);

  saved_meta = html_get_attrib_from_tag(hpinfo->stack, "CONTENT");

  if(!saved_meta)
    return;

  hpinfo->tag_attrib = html_get_attrib_from_tag(saved_meta, "URL");

  if(hpinfo->tag_attrib)
  {
    html_parser_call_funcs(hpinfo, hpinfo->attrib_funcs);

    if(hpinfo->rewrite)
    {
      /* little hack to prevent writing    */
      /* outside of allocated memory chunk */
      saved_meta = _realloc(saved_meta,
        strlen(saved_meta) + strlen(hpinfo->tag_attrib) + 4);

      html_replace_url_in_stack(saved_meta, "URL", hpinfo->tag_attrib, TRUE);
      _free(hpinfo->tag_attrib);

      hpinfo->tag_attrib = saved_meta;

      if(hpinfo->tag_attrib)
      {
        int l = strlen(hpinfo->tag_attrib);

        html_parser_SEND(hpinfo);
        html_parser_SEXPAND(hpinfo, l);
        html_replace_url_in_stack(hpinfo->stack,
          hpinfo->current_attrib->attrib, hpinfo->tag_attrib, FALSE);
        hpinfo->tag_attrib = 0;
      }
    }
    else
    {
      _free(hpinfo->tag_attrib);
    }
  }

  _free(saved_meta);
}

void html_parser_parse_tag_meta_robots(html_parser_t * hpinfo, char *stack,
  html_robots_info_t * oinfo)
{
  char *meta_type;
  char *content;
  char **flags;
  int i;

  if(!hpinfo->current_tag || hpinfo->current_tag->type != HTML_TAG_META)
    return;

  meta_type = html_get_attrib_from_tag(hpinfo->stack, "NAME");

  if(!meta_type || strcasecmp(meta_type, "Robots"))
  {
    _free(meta_type);
    return;
  }

  _free(meta_type);

  content = html_get_attrib_from_tag(hpinfo->stack, "CONTENT");

  if(!content)
    return;

  flags = tl_str_split(content, ",");
  _free(content);

  for(i = 0; flags && flags[i]; i++)
  {
    if(!strcasecmp(flags[i], "all"))
    {
      oinfo->index = TRUE;
      oinfo->follow = TRUE;
      oinfo->images = TRUE;
    }
    else if(!strcasecmp(flags[i], "none"))
    {
      oinfo->index = FALSE;
      oinfo->follow = FALSE;
      oinfo->images = FALSE;
    }
    else if(!strcasecmp(flags[i], "index"))
      oinfo->index = TRUE;
    else if(!strcasecmp(flags[i], "follow"))
      oinfo->follow = TRUE;
    else if(!strcasecmp(flags[i], "noimageindex"))
      oinfo->images = FALSE;
    else if(!strcasecmp(flags[i], "noindex"))
      oinfo->index = FALSE;
    else if(!strcasecmp(flags[i], "nofollow"))
      oinfo->follow = FALSE;
    _free(flags[i]);
  }
  _free(flags);
}

void html_parser_parse_tag_jstransform(html_parser_t * hpinfo, char *stack,
  void *data)
{
#ifdef HAVE_REGEX
  dllist *ptr;
  html_tag_t t = { HTML_TAG_HACK, "HACK",
    {{HTML_ATTRIB_HACK, "HACK", LINK_INLINE | LINK_DOWNLD},
      {HTML_ATTRIB_NULL, NULL, 0}}
  };

  for(ptr = priv_cfg.js_transform; ptr; ptr = ptr->next)
  {
    js_transform_t *jt = (js_transform_t *) ptr->data;

    if(js_transform_match_tag(jt, hpinfo->stack))
    {
      int nsub, *subs;
      char *attr = html_get_attrib_from_tag(hpinfo->stack,
        jt->attrib);

      if(!attr)
        continue;

      if(!re_pmatch_subs(jt->re, attr, &nsub, &subs))
      {
        _free(attr);
        continue;
      }

      hpinfo->tag_attrib = js_transform_apply(jt, attr, nsub, subs);

      /*****************************************/
      /* quite dirty hack to make happy attrib */
      /* parsing funcs which require valid     */
      /* current_tag & current_attrib          */
      /*****************************************/
      hpinfo->current_tag = &t;
      hpinfo->current_attrib = &(t.attribs[0]);

      if(hpinfo->tag_attrib)
        html_parser_call_funcs(hpinfo, hpinfo->attrib_funcs);

      if(hpinfo->rewrite && jt->type == 1 && nsub)
      {
        int l = strlen(hpinfo->tag_attrib);

        attr = _realloc(attr, strlen(attr) + l + 1);
        memmove(attr + l + subs[2], attr + subs[3],
          strlen(attr + subs[3]) + 1);
        memcpy(attr + subs[2], hpinfo->tag_attrib, l);

        l = strlen(attr);
        html_parser_SEND(hpinfo);
        html_parser_SEXPAND(hpinfo, l);
        html_replace_url_in_stack(hpinfo->stack, jt->attrib, attr, FALSE);
      }

      _free(subs);
      _free(attr);

      /* :-) unhack */
      hpinfo->current_tag = NULL;
      hpinfo->current_attrib = NULL;

      _free(hpinfo->tag_attrib);
    }
  }
#endif
}

/********************************************************/
/* functions for processing URL attributes of HTML tags */
/********************************************************/
void html_parser_url_to_absolute_url(html_parser_t * hpinfo, char *stack,
  void *data)
{
  char *ustr;

  /*
     printf("http_parser sees %s %s=\"%s\"<\n",
     hpinfo->current_tag->tag,
     hpinfo->current_attrib->attrib,
     hpinfo->tag_attrib);
   */

  ustr = url_to_absolute_url(hpinfo->base, hpinfo->baset,
    hpinfo->doc_url, hpinfo->tag_attrib);

  if(ustr && *ustr)
  {
    DEBUG_HTML("Rewriting URL (to abs) - %s -> %s\n", hpinfo->tag_attrib,
      ustr);
    _free(hpinfo->tag_attrib);
    hpinfo->tag_attrib = ustr;
  }
}

void html_parser_process_base(html_parser_t * hpinfo, char *stack, void *data)
{
  if(hpinfo->current_tag->type == HTML_TAG_BASE &&
    hpinfo->current_attrib->type == HTML_ATTRIB_HREF)
  {
    int lp, ls;

    html_parser_process_new_base_url(hpinfo, hpinfo->tag_attrib);

    /* comment BASE tag because pavuk        */
    /* overwrites URLs according to this tag */
    lp = strlen(COMMENT_PREFIX);
    ls = strlen(COMMENT_SUFFIX);

    html_parser_SEND(hpinfo);
    html_parser_SEXPAND(hpinfo, (lp + ls));

    memmove(hpinfo->stack + lp, hpinfo->stack, strlen(hpinfo->stack) + 1);
    memcpy(hpinfo->stack, COMMENT_PREFIX, lp);
    strcat(hpinfo->stack, COMMENT_SUFFIX);
  }
}

void html_parser_process_form(html_parser_t * hpinfo, char *stack,
  dllist ** formlist)
{
  if(hpinfo->current_attrib->stat & LINK_FORM &&
    hpinfo->current_attrib->type == HTML_ATTRIB_ACTION)
  {
    hpinfo->doc_url->status |= URL_HAVE_FORMS;

    if(formlist && hpinfo->tag_attrib)
    {
      *formlist = dllist_append(*formlist,
      (dllist_t) tl_strdup(hpinfo->tag_attrib));
    }
  }
}

void html_parser_get_url(html_parser_t * hpinfo, char *stack,
  html_extract_info_t * einfo)
{
  if(*hpinfo->tag_attrib /* Never follow "" */  &&
    (hpinfo->current_attrib->stat & LINK_DOWNLD) &&
    (!einfo->only_inline ||
      (einfo->only_inline &&
        hpinfo->current_attrib->stat & LINK_INLINE)) &&
    (!(hpinfo->current_attrib->stat & LINK_SCRIPT) ||
      (einfo->enable_js && hpinfo->current_attrib->stat & LINK_SCRIPT)))
  {
    url *purl = (url *) 0;
    cond_info_t condp;

    condp.level = 0;
    condp.urlnr = 0;
    condp.size = 0;
    condp.time = 0L;
    condp.mimet = NULL;
    condp.full_tag = stack;
    condp.params = NULL;
    condp.html_doc = hpinfo->in_content;
    condp.html_doc_offset = hpinfo->in_offset;
    condp.tag = hpinfo->current_tag ? hpinfo->current_tag->tag : NULL;
    condp.attrib = hpinfo->current_attrib ?
      hpinfo->current_attrib->attrib : NULL;

    purl = url_parse(hpinfo->tag_attrib);
    assert(purl->type != URLT_FROMPARENT);
    url_path_abs(purl);

    if(hpinfo->current_attrib->stat & LINK_INLINE)
      purl->status |= URL_INLINE_OBJ;

    if(hpinfo->current_attrib->stat & LINK_SCRIPT)
      purl->status |= URL_ISSCRIPT;

    purl->level = hpinfo->doc_url->level + 1;
    purl->parent_url = dllist_append(purl->parent_url,
    (dllist_t) hpinfo->doc_url);

    /*****************************************************/
    /* if we are in SYNC/MIRROR mode try to get original */
    /* URL rather than processing it as file             */
    /* (mandatory thing to get working SYNC/MIRROR mode) */
    /*****************************************************/
    if((cfg.mode == MODE_SYNC || cfg.mode == MODE_MIRROR) &&
      cfg.request && (purl->type == URLT_FILE))
    {
      url *pomurl = filename_to_url(purl->p.file.filename);

      if(pomurl)
      {
        free_deep_url(purl);
        _free(purl);
        purl = pomurl;
      }
    }

    /**********************************/
    /* remove last anchor URL because */
    /* it is server side image map    */
    /**********************************/
    if(einfo->prev_a &&
      hpinfo->current_tag->type == HTML_TAG_IMG &&
      hpinfo->current_attrib->type == HTML_ATTRIB_SRC &&
      html_tag_co_elem(hpinfo->stack, "ISMAP"))
    {
      DEBUG_HTML("Removing server image map\n");
      free_deep_url((url *) einfo->prev_a->data);
      free((url *) einfo->prev_a->data);
      einfo->urls = dllist_remove_entry(einfo->urls, einfo->prev_a);
      einfo->prev_a = NULL;
    }

    if(hpinfo->current_tag->type == HTML_TAG_A &&
      hpinfo->current_attrib->type == HTML_ATTRIB_HREF)
    {
      einfo->prev_a = NULL;
    }

    /* Do not accept links, which only link inside the already loaded document
       like <a href="#top">. This is a local relative reference, so remove it.
    */
    if((hpinfo->current_attrib->type == HTML_ATTRIB_USEMAP ||
    hpinfo->current_attrib->type == HTML_ATTRIB_HREF) &&
    hpinfo->tag_attrib[0] == '#')
    {
      LOCK_REJCNT;
      cfg.reject_cnt++;
      UNLOCK_REJCNT;

      DEBUG_HTML("Rejecting local anchor URL - %s\n", hpinfo->tag_attrib);
    }
    else if(einfo->no_limits || url_append_condition(purl, &condp))
    {
      DEBUG_HTML("Accepting URL - %s\n", hpinfo->tag_attrib);

      /***************************************/
      /* process special add-on tag PAVUKEXT */
      /* where are stored some additional    */
      /* informations about FTP URLs         */
      /***************************************/
      if(purl->type == URLT_FTP || purl->type == URLT_FTPS)
      {
        char *pext;

        pext = html_get_attrib_from_tag(hpinfo->stack, "PAVUKEXT");
        if(pext)
        {
          ftp_url_extension *uext;

          uext = ftp_parse_ftpinf_ext(pext);
          purl->extension = uext;

          if(uext->type == FTP_TYPE_D)
            purl->p.ftp.dir = TRUE;
        }
        _free(pext);
      }

      einfo->urls = dllist_append(einfo->urls, (dllist_t) purl);

      if(hpinfo->current_tag->type == HTML_TAG_A &&
        hpinfo->current_attrib->type == HTML_ATTRIB_HREF)
      {
        einfo->prev_a = dllist_last(einfo->urls);
      }
    }
    else
    {
      LOCK_REJCNT;
      cfg.reject_cnt++;
      UNLOCK_REJCNT;

      DEBUG_HTML("Rejecting URL - %s\n", hpinfo->tag_attrib);
      free_deep_url(purl);
      _free(purl);
    }
  }
}

void html_parser_url_to_local(html_parser_t * hpinfo, char *stack,
  html_rewrite_info_t * rinfo)
{
  url *urlp, *before_url;
  char *anchor, *fn;
  int is_local;

  if(!hpinfo->rewrite || rinfo->all_to_remote)
    return;

  urlp = url_parse(hpinfo->tag_attrib);
  assert(urlp->type != URLT_FROMPARENT);

  if(urlp->type == URLT_FILE || !prottable[urlp->type].supported)
  {
    free_deep_url(urlp);
    _free(urlp);
    return;
  }

  anchor = url_get_anchor_name(urlp);

  /*******************************************/
  /* for better performance with info files  */
  /* we should rather use filename generated */
  /* for previous occurence of this URL      */
  /*******************************************/
  before_url = url_was_befor(urlp);

  if(!before_url)
  {
    dllist *ptr;

    ptr = dllist_find2(rinfo->einfo->urls, (dllist_t) urlp,
    dllist_url_compare);
    if(ptr)
      before_url = (url *) ptr->data;
  }

  if(before_url)
    fn = url_to_filename(before_url, TRUE);
  else
    fn = url_to_filename(urlp, FALSE);

  is_local = !access(fn, R_OK);

  if(is_local || rinfo->all_to_local ||
    (rinfo->selected_to_local && before_url) ||
    url_compare(urlp, hpinfo->doc_url))
  {
    char *actname, *relname;
    struct stat estat;

    if(is_local && !stat(fn, &estat) && S_ISDIR(estat.st_mode))
      fn = tl_str_concat(NULL, fn, "/", priv_cfg.index_name, NULL);
    else
      fn = tl_strdup(fn);

    actname = url_to_filename(hpinfo->doc_url, FALSE);

    /* it seems that lynx and netscape behave different on   */
    /* empty HREFs, so use it only in case when is specified */
    /* partname of document (#xxx)         */
    /* this is URL of current document -> "" */
    if(anchor && !strcmp(actname, fn))
      relname = tl_strdup("");
    else
      relname = get_relative_path(actname, fn);

    _free(fn);

    /* workaround for -sel_to_local && -nostore_index */
    if(rinfo->selected_to_local && !rinfo->store_index)
    {
      char *slp = strrchr(relname, '/');

      if(!slp)
        slp = relname;
      else
        slp++;

      if(!strcmp(slp, priv_cfg.index_name))
        *slp = '\0';
    }

    if(anchor)
      relname = tl_str_concat(relname, "#", anchor, NULL);

    DEBUG_HTML("Rewriting URL (to loc) - %s -> %s\n", hpinfo->tag_attrib,
      relname);

    _free(hpinfo->tag_attrib);
    hpinfo->tag_attrib = relname;
  }
  free_deep_url(urlp);
  _free(urlp);
}

void html_parser_remove_advertisement(html_parser_t * hpinfo, char *stack,
  void *data)
{
#ifdef HAVE_REGEX
  int is_adver = FALSE;

  if(hpinfo->current_tag->type != HTML_TAG_IMG ||
    hpinfo->current_attrib->type != HTML_ATTRIB_SRC)
    return;

  if(cfg.remove_adv && priv_cfg.advert_res)
  {
    dllist *ptr = priv_cfg.advert_res;

    while(ptr)
    {
      if(re_pmatch((re_entry *) ptr->data, hpinfo->tag_attrib))
      {
        DEBUG_HTML("Removing advert URL - %s\n", hpinfo->tag_attrib);
        is_adver = TRUE;
        break;
      }
      ptr = ptr->next;
    }
  }

  if(is_adver)
  {
    int lp = strlen(ADVERT_PREFIX);
    int ls = strlen(ADVERT_SUFFIX);

    html_parser_SEND(hpinfo);
    html_parser_SEXPAND(hpinfo, (lp + ls));

    memmove(hpinfo->stack + lp, hpinfo->stack, strlen(hpinfo->stack) + 1);
    memcpy(hpinfo->stack, ADVERT_PREFIX, lp);
    strcat(hpinfo->stack, ADVERT_SUFFIX);
  }
#endif
}

void html_parser_change_url(html_parser_t * hpinfo, char *stack,
  html_change_info_t * chinfo)
{
  url *urlp;

  urlp = url_parse(hpinfo->tag_attrib);
  assert(urlp->type != URLT_FROMPARENT);

  if(urlp->type == URLT_FILE || !prottable[urlp->type].supported)
  {
    free_deep_url(urlp);
    _free(urlp);
    return;
  }

  if(url_compare(urlp, chinfo->url_old))
  {
    DEBUG_HTML("Rewriting URL (change) - %s -> %s\n",
      chinfo->url_new, chinfo->url_new);

    _free(hpinfo->tag_attrib);
    hpinfo->tag_attrib = tl_strdup(chinfo->url_new);
  }

  free_deep_url(urlp);
  _free(urlp);
}

/********************************************************/
/* functions for processing CSS parts of HTML documents */
/********************************************************/
void html_parser_style_to_absolute_urls(html_parser_t * hpinfo, char *stack,
  void *data)
{
  char *alttag;

  if(!hpinfo->rewrite)
    return;

  if(hpinfo->tag_attrib)
  {
    alttag = css_to_absolute_links(hpinfo->doc_url,
      hpinfo->tag_attrib, hpinfo->base, hpinfo->baset);
    _free(hpinfo->tag_attrib);
    hpinfo->tag_attrib = alttag;
  }
  else
  {
    int l;

    alttag = css_to_absolute_links(hpinfo->doc_url,
      hpinfo->stack, hpinfo->base, hpinfo->baset);

    l = strlen(alttag);
    if(l > hpinfo->stack_offset)
    {
      hpinfo->stack_offset = 0;
      html_parser_SEXPAND(hpinfo, l);
    }
    memcpy(hpinfo->stack, alttag, l + 1);
    hpinfo->stack_offset = l;
    _free(alttag);
  }
}

void html_parser_get_style_urls(html_parser_t * hpinfo, char *stack,
  html_extract_info_t * einfo)
{
  dllist *pv;
  if(!cfg.read_css)
  {                             /* don't fetch from css if not wanted */
    return;
  }
  if(hpinfo->tag_attrib)
    pv = css_get_all_links(hpinfo->doc_url, hpinfo->tag_attrib,
      hpinfo->base, hpinfo->baset, einfo->no_limits);
  else
    pv = css_get_all_links(hpinfo->doc_url, hpinfo->stack,
      hpinfo->base, hpinfo->baset, einfo->no_limits);

  einfo->urls = dllist_concat(einfo->urls, pv);
}

void html_parser_style_to_local_urls(html_parser_t * hpinfo, char *stack,
  html_rewrite_info_t * rinfo)
{
  char *alttag;

  if(!hpinfo->rewrite || rinfo->all_to_remote)
    return;

  if(hpinfo->tag_attrib)
  {
    alttag = css_remote_to_local_links(hpinfo->doc_url,
      hpinfo->tag_attrib, rinfo->all_to_local,
      rinfo->selected_to_local, hpinfo->base, hpinfo->baset);
    _free(hpinfo->tag_attrib);
    hpinfo->tag_attrib = alttag;
  }
  else
  {
    int l;

    alttag = css_remote_to_local_links(hpinfo->doc_url,
      hpinfo->stack, rinfo->all_to_local,
      rinfo->selected_to_local, hpinfo->base, hpinfo->baset);

    l = strlen(alttag);
    if(l > hpinfo->stack_offset)
    {
      hpinfo->stack_offset = 0;
      html_parser_SEXPAND(hpinfo, l);
    }
    memcpy(hpinfo->stack, alttag, l + 1);
    hpinfo->stack_offset = l;
    _free(alttag);
  }
}

void html_parser_style_change_url(html_parser_t * hpinfo, char *stack,
  html_change_info_t * chinfo)
{
  char *alttag;

  if(!hpinfo->rewrite)
    return;

  if(hpinfo->tag_attrib)
  {
    alttag = css_change_url(hpinfo->doc_url, hpinfo->tag_attrib,
      chinfo->url_old, chinfo->url_new);
    _free(hpinfo->tag_attrib);
    hpinfo->tag_attrib = alttag;
  }
  else
  {
    int l;

    alttag = css_change_url(hpinfo->doc_url, hpinfo->stack,
      chinfo->url_old, chinfo->url_new);

    l = strlen(alttag);
    if(l > hpinfo->stack_offset)
    {
      hpinfo->stack_offset = 0;
      html_parser_SEXPAND(hpinfo, l);
    }
    memcpy(hpinfo->stack, alttag, l + 1);
    hpinfo->stack_offset = l;
    _free(alttag);
  }
}

/***********************************************************/
/* functions for processing SCRIPTs part of HTML documents */
/***********************************************************/
void html_parser_parse_jspatterns(html_parser_t * hpinfo, char *stack,
  void *data)
{
#ifdef HAVE_REGEX
  dllist *ptr;
  int found = FALSE;
  int start, end;

  start = -1;
  end = -1;

  if(!hpinfo->tag_attrib)
    return;

  for(ptr = priv_cfg.js_patterns; ptr; ptr = ptr->next)
  {
    if(re_pmatch_sub((re_entry *) ptr->data, hpinfo->tag_attrib,
        1, &start, &end))
    {
      found = TRUE;
      break;
    }
  }

  if(found && (start >= 0))
  {
    char *saved_attrib, *new_attrib;

    saved_attrib = hpinfo->tag_attrib;

    hpinfo->tag_attrib = tl_strndup(hpinfo->tag_attrib + start, end - start);
    html_parser_call_funcs(hpinfo, hpinfo->attrib_funcs);

    new_attrib = _malloc(start + strlen(saved_attrib + end) +
      strlen(hpinfo->tag_attrib) + 1);

    strncpy(new_attrib, saved_attrib, start);
    strcpy(new_attrib + start, hpinfo->tag_attrib);
    strcat(new_attrib, saved_attrib + end);

    _free(hpinfo->tag_attrib);
    _free(saved_attrib);

    hpinfo->tag_attrib = new_attrib;
  }
#endif
}

void html_parser_parse_body_jspatterns(html_parser_t * hpinfo, char *stack,
  void *data)
{
#ifdef HAVE_REGEX
  char *stackc = NULL;
  char *p;
  int ilen;

  html_tag_t t = { HTML_TAG_HACK, "HACK",
    {{HTML_ATTRIB_HACK, "HACK", LINK_INLINE | LINK_DOWNLD},
      {HTML_ATTRIB_NULL, NULL, 0}}
  };

  if(hpinfo->tag_attrib)
    return;

  /*****************************************/
  /* quite dirty hack to make happy attrib */
  /* parsing funcs which require valid     */
  /* current_tag & current_attrib          */
  /*****************************************/
  hpinfo->current_tag = &t;
  hpinfo->current_attrib = &(t.attribs[0]);

  p = hpinfo->stack;

  while(*p)
  {
    ilen = strcspn(p, "\r\n");

    hpinfo->tag_attrib = tl_strndup(p, ilen);

    html_parser_parse_jspatterns(hpinfo, stack, data);

    if(hpinfo->rewrite)
      stackc = tl_str_concat(stackc, hpinfo->tag_attrib, "\n", NULL);
    _free(hpinfo->tag_attrib);

    p += ilen;
    p += strspn(p, "\n\r");
  }

  if(hpinfo->rewrite)
  {
    ilen = strlen(stackc);
    hpinfo->stack_offset = 0;
    html_parser_SEXPAND(hpinfo, ilen);
    memcpy(hpinfo->stack, stackc, ilen + 1);
    hpinfo->stack_offset = ilen;
    _free(stackc);
  }

  /* :-) unhack */
  hpinfo->current_tag = NULL;
  hpinfo->current_attrib = NULL;

  hpinfo->tag_attrib = NULL;
#endif
}

void html_parser_parse_body_jstransform(html_parser_t * hpinfo, char *stack,
  void *data)
{
#ifdef HAVE_REGEX
  char *p;
  int ilen;
  char *stackc = NULL;

  html_tag_t t = { HTML_TAG_HACK, "HACK",
    {{HTML_ATTRIB_HACK, "HACK", LINK_INLINE | LINK_DOWNLD},
      {HTML_ATTRIB_NULL, NULL, 0}}
  };

  if(hpinfo->tag_attrib)
    return;

  /*****************************************/
  /* quite dirty hack to make happy attrib */
  /* parsing funcs which require valid     */
  /* current_tag & current_attrib          */
  /*****************************************/
  hpinfo->current_tag = &t;
  hpinfo->current_attrib = &(t.attribs[0]);

  p = hpinfo->stack;

  while(*p)
  {
    dllist *ptr;
    char *ln;

    ilen = strcspn(p, "\r\n");

    ln = tl_strndup(p, ilen);

    for(ptr = priv_cfg.js_transform; ptr; ptr = ptr->next)
    {
      int nsub, *subs;
      js_transform_t *jt = (js_transform_t *) ptr->data;

      if(jt->tag[0])
        continue;

      if(!re_pmatch_subs(jt->re, ln, &nsub, &subs))
      {
        continue;
      }

      hpinfo->tag_attrib = js_transform_apply(jt, ln, nsub, subs);

      /*****************************************/
      /* quite dirty hack to make happy attrib */
      /* parsing funcs which require valid     */
      /* current_tag & current_attrib          */
      /*****************************************/
      hpinfo->current_tag = &t;
      hpinfo->current_attrib = &(t.attribs[0]);

      if(hpinfo->tag_attrib)
        html_parser_call_funcs(hpinfo, hpinfo->attrib_funcs);

      if(hpinfo->rewrite && jt->type == 1 && nsub)
      {
        int l = strlen(hpinfo->tag_attrib);

        ln = _realloc(ln, strlen(ln) + l + 1);
        memmove(ln + l + subs[2], ln + subs[3], strlen(ln + subs[3]) + 1);
        memcpy(ln + subs[2], hpinfo->tag_attrib, l);
      }

      _free(subs);

      /* :-) unhack */
      hpinfo->current_tag = NULL;
      hpinfo->current_attrib = NULL;

      _free(hpinfo->tag_attrib);
    }

    if(hpinfo->rewrite)
      stackc = tl_str_concat(stackc, ln, "\n", NULL);

    _free(ln);
    p += ilen;
    p += strspn(p, "\n\r");
  }

  if(hpinfo->rewrite)
  {
    ilen = strlen(stackc);
    hpinfo->stack_offset = 0;
    html_parser_SEXPAND(hpinfo, ilen);
    memcpy(hpinfo->stack, stackc, ilen + 1);
    hpinfo->stack_offset = ilen;
    _free(stackc);
  }

  /* :-) unhack */
  hpinfo->current_tag = NULL;
  hpinfo->current_attrib = NULL;

  hpinfo->tag_attrib = NULL;
#endif
}
