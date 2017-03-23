/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _htmlparser_h_
#define _htmlparser_h_

#include "dlhash.h"
#include "dllist.h"
#include "html.h"
#include "url.h"

typedef struct
{
  int rewrite;                  /* == TRUE -> wi will adjust content */
  int purestyle;                /* == TRUE if document is text/css */
  int purescript;               /* == TRUE if document is script */

  url *doc_url;                 /* source URL of document */

  ssize_t in_size;              /* size of HTML document */
  char *in_content;             /* content of HTML document */
  int in_offset;                /* current offset in HTML document */

  ssize_t aout_size;            /* allocated size for adjusted HTML */
  char *out_content;            /* content of adjusted HTML document */
  int out_offset;               /* current offset in adj. HTML doc */

  char *base;                   /* current BASE of HTML document */
  char *baset;                  /* BASE of document derived from URL */

  int stack_size;               /* allocated size of processing stack */
  int stack_offset;             /* current ofset in stack */
  char *stack;                  /* stack for buffering tags of HTML doc */

  char *tag_attrib;             /* content of currently processed tag */

  dlhash *tag_hash;             /* hash for speedup lookup of tags */
  html_tag_t *current_tag;      /* tag of currently processed tag */
  html_tag_atrib_t *current_attrib;     /* current attrib of current tag */

  dllist *tag_funcs;            /* func chain for processing tags */
  dllist *attrib_funcs;         /* func chain for processing tag attributes */
  dllist *style_funcs;          /* func chain for procesing CSS */
  dllist *script_funcs;         /* func chain for procesing scripts */
} html_parser_t;

typedef void (*html_parser_func_t) (html_parser_t *, char *, void *);

/***********************************/
/* structure of this type holds    */
/* infos about functions in chain  */
/***********************************/
typedef struct
{
  html_parser_func_t func;
  void *data;
} html_parser_func_info_t;

/***********************************/
/* struct for passing informations */
/* to/from URL extracting routine  */
/***********************************/
typedef struct
{
  int no_limits;
  int only_inline;
  int enable_js;

  dllist *urls;
  dllist *prev_a;
} html_extract_info_t;

/***********************************/
/* struct for passing informations */
/* to/from URL rewriting routine   */
/***********************************/
typedef struct
{
  int all_to_local;
  int all_to_remote;
  int selected_to_local;
  int store_index;
  html_extract_info_t *einfo;
} html_rewrite_info_t;

/***********************************/
/* struct for passing informations */
/* to/from URL changing routine    */
/***********************************/
typedef struct
{
  url *url_old;
  char *url_new;
} html_change_info_t;

/***********************************/
/* struct for getting informations */
/* from META Robots parser         */
/***********************************/
typedef struct
{
  int index;
  int follow;
  int images;
} html_robots_info_t;

/******************************************/
/* macros for safe handling of expandable */
/* memory chunks required in HTML parser  */
/******************************************/
#define html_parser_FENDER      2048

#define html_parser_MEXPAND(hpinfo, sv) \
{ \
  int psize = 100 + (sv) + hpinfo->in_size \
    + hpinfo->out_offset - hpinfo->in_offset; \
  if(hpinfo->aout_size < psize) \
  { \
    hpinfo->aout_size = psize + html_parser_FENDER; \
    hpinfo->out_content = \
      _realloc(hpinfo->out_content, hpinfo->aout_size); \
  } \
}

#define html_parser_SEXPAND(hpinfo, sv) \
  if((hpinfo->stack_size - (hpinfo->stack_offset \
    + (sv) + html_parser_FENDER)) < 0)\
  { \
    hpinfo->stack_size += html_parser_FENDER + sv; \
    hpinfo->stack = _realloc(hpinfo->stack, hpinfo->stack_size); \
  }

#define html_parser_SEND(hpinfo) \
  while(hpinfo->stack[hpinfo->stack_offset]) \
    hpinfo->stack_offset++;

extern html_parser_t *html_parser_init(html_tag_t *, int, int, int, int);
extern void html_parser_kill(html_parser_t *);
extern void html_parser_do_cleanup(void);
extern void html_parser_add_tag_func(html_parser_t *, html_parser_func_t,
  void *);
extern void html_parser_add_attrib_func(html_parser_t *, html_parser_func_t,
  void *);
extern void html_parser_add_style_func(html_parser_t *, html_parser_func_t,
  void *);
extern void html_parser_add_script_func(html_parser_t *, html_parser_func_t,
  void *);
extern void html_parser_set_document(html_parser_t *, url *, char *, ssize_t);
extern void html_parser_take_document(html_parser_t *, char **, ssize_t *);
extern void html_parser_set_base(html_parser_t *, char *, char *);
extern void html_parser_parse(html_parser_t *);

extern void html_parser_parse_tag(html_parser_t *, char *, void *);
extern void html_parser_parse_tag_slash_a(html_parser_t *, char *,
  html_extract_info_t *);
extern void html_parser_parse_tag_meta_refresh(html_parser_t *, char *,
  void *);
extern void html_parser_parse_tag_meta_robots(html_parser_t *, char *,
  html_robots_info_t *);
extern void html_parser_parse_tag_jstransform(html_parser_t *, char *,
  void *);

extern void html_parser_url_to_absolute_url(html_parser_t *, char *, void *);
extern void html_parser_remove_advertisement(html_parser_t *, char *, void *);
extern void html_parser_get_url(html_parser_t *, char *,
  html_extract_info_t *);
extern void html_parser_process_base(html_parser_t *, char *, void *);
extern void html_parser_process_form(html_parser_t *, char *, dllist **);
extern void html_parser_url_to_local(html_parser_t *, char *,
  html_rewrite_info_t *);
extern void html_parser_change_url(html_parser_t *, char *,
  html_change_info_t *);

extern void html_parser_get_style_urls(html_parser_t *, char *,
  html_extract_info_t *);
extern void html_parser_style_to_absolute_urls(html_parser_t *, char *,
  void *);
extern void html_parser_style_to_local_urls(html_parser_t *, char *,
  html_rewrite_info_t *);
extern void html_parser_style_change_url(html_parser_t *, char *,
  html_change_info_t *);

extern void html_parser_parse_jspatterns(html_parser_t *, char *, void *);
extern void html_parser_parse_body_jspatterns(html_parser_t *, char *,
  void *);
extern void html_parser_parse_body_jstransform(html_parser_t *, char *,
  void *);

#endif
