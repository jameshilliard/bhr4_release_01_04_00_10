/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <utime.h>

#include "url.h"
#include "doc.h"
#include "html.h"
#include "htmlparser.h"
#include "gui_api.h"
#include "mime.h"
#include "errcode.h"
#include "uexit.h"

/*****************************************/
/* get requested attribute from HTML tag */
/*****************************************/
char *html_get_attrib_from_tag(char *tag, char *link_attrib)
{
  char *p;
  char *retval = NULL;
  char *attrstart = NULL;
  char *attrend = NULL;
  int llen = strlen(link_attrib);
  bool_t was_sep = TRUE;

  for(p = tag; *p; p++)
  {
    if(was_sep && !attrstart && !strncasecmp(link_attrib, p, llen) &&
      (tl_ascii_isspace(*(p + llen)) || (*(p + llen) == '=')))
    {
      attrstart = p + llen;

      while(*attrstart)
      {
        if(tl_ascii_isspace(*attrstart) || (*attrstart == '='))
          attrstart++;
        else
          break;
      }
      if(*attrstart == '\"' || *attrstart == '\'')
      {
        if(!(attrend = strchr(attrstart + 1, *attrstart)))
          attrend = attrstart + strcspn(attrstart, " \t\r\n>");
        attrstart++;
      }
      else
      {
        attrend = attrstart + strcspn(attrstart, " \t\r\n\"\'>");
      }
      break;
    }
    was_sep = tl_ascii_isspace(*p) != 0;
    if(!attrend && !was_sep)
      was_sep = (*p == ';');
    if(*p == '\"' || *p == '\'')
    {
      if(!(p = strchr(p + 1, *p)))
        break;
    }
  }
  if(attrstart)
  {
    /* strip leading/trailing spaces */
    while(tl_ascii_isspace(*attrstart))
      attrstart++;
    while(attrend > attrstart && tl_ascii_isspace(*(attrend - 1)))
      attrend--;

    /* to workaround broken tags which are missing closing */
    /* quotes and contain leading space characters        */
    if(attrstart > attrend)
      attrend = attrstart + strcspn(attrstart, "> ");

    retval = tl_strndup(attrstart, attrend - attrstart);
    omit_chars(retval, "\t\n\r");
  }
  return retval;
}

/********************************************************/
/* overwrite content of specified attribute in HTML tag */
/********************************************************/
void html_replace_url_in_stack(char *tag, char *link_attrib, char *urlin,
  int pare)
{
  char *pom;
  char *p;
  char *attrstart = NULL;
  char *pattrstart = NULL;
  char *attrend = NULL;
  int llen = strlen(link_attrib);
  bool_t was_sep = TRUE;

  for(p = tag; *p; p++)
  {
    if(was_sep && !attrstart && !strncasecmp(link_attrib, p, llen) &&
      (tl_ascii_isspace(*(p + llen)) || (*(p + llen) == '=')))
    {
      pattrstart = attrstart = p + llen;

      while(*attrstart)
      {
        if(tl_ascii_isspace(*attrstart) || (*attrstart == '='))
          attrstart++;
        else
          break;
      }
      if(*attrstart == '\"' || *attrstart == '\'')
      {
        if(!(attrend = strchr(attrstart + 1, *attrstart)))
          attrend = attrstart + strcspn(attrstart, " \r\n\t>");
        attrstart++;
      }
      else
      {
        attrend = attrstart + strcspn(attrstart, " \t\r\n\"\'>");
      }
      break;
    }
    was_sep = tl_ascii_isspace(*p) != 0;
    if(*p == '\"' || *p == '\'')
    {
      if(!(p = strchr(p + 1, *p)))
        break;
    }
  }
  if(attrstart)
  {
    /* to workaround broken tags which are missing closing */
    /* quotes and contain leading space characters        */
    if(attrstart > attrend)
      attrend = attrstart + strcspn(attrstart, "> ");

    pom = (*attrend == '\'' || *attrend == '\"') ?
      tl_strdup(attrend + 1) : tl_strdup(attrend);

    if(!pare)
      strcpy(pattrstart, "=\"");
    else
      strcpy(pattrstart, "=");
    strcat(pattrstart, urlin);
    if(!pare)
      strcat(pattrstart, "\"");

    strcat(pattrstart, pom);

    _free(pom);
  }
  return;
}

/******************************************/
/* look if tag contains specified element */
/******************************************/
int html_tag_co_elem(char *tag, char *elem)
{
  char *p;
  int llen = strlen(elem);
  bool_t was_sep = TRUE;

  for(p = tag; *p; p++)
  {
    if(was_sep && !strncasecmp(elem, p, llen) &&
      (tl_ascii_isspace(*(p + llen)) ||
        (*(p + llen) == '=') || (!*(p + llen))))
    {
      return TRUE;
    }
    was_sep = tl_ascii_isspace(*p) != 0;
    if(!was_sep)
      was_sep = (*p == ';');
    if(*p == '\"' || *p == '\'')
    {
      if(!(p = strchr(p + 1, *p)))
        break;
    }
  }
  return FALSE;
}

/**********************************************************/
/* determine base URL for document looking at request URL */
/**********************************************************/
static void html_get_init_base_url(url * urlp, char **base, char **baset)
{
  char *p;

  *baset = url_to_urlstr(urlp, FALSE);
  *base = tl_strdup(*baset);
  if((p = strrchr(*baset, '#')))
    *p = '\0';
  DEBUG_HTML("BASE URL - %s\n", *base);

  if((p = strrchr(*base, '?')))
    *p = '\0';
  if(!tl_is_dirname(*base))
  {
    p = strrchr(*base, '/');
    if(p)
      *(p + 1) = '\0';
  }
}

/********************************************************************/
/* determine base URL for document looking on request URL && server */
/* response header fields Content-Location: & Content-Base: & Base: */
/********************************************************************/
static void html_get_base_url(doc * docp, char **base, char **baset)
{
  char *p;

  html_get_init_base_url(docp->doc_url, base, baset);

  /* get possible base URL from server response header */
  if(docp->mime &&
    ((p = get_mime_param_val_str("Content-Location:", docp->mime)) ||
      (p = get_mime_param_val_str("Content-Base:", docp->mime)) ||
      (p = get_mime_param_val_str("Base:", docp->mime))) && p)
  {
    char *p2;
    url *urlp;

    p2 = url_to_absolute_url(*base, *baset, docp->doc_url, p);
    urlp = url_parse(p2);
    assert(urlp->type != URLT_FROMPARENT);

    if(!prottable[urlp->type].supported)
    {
      xprintf(1,
        gettext("Unsupported BASE URL -  %s (probably bad handled)\n"), p);
      _free(*base);
      *base = tl_strdup(p);
    }
    else
    {
      _free(p);
      _free(*base);
      html_get_init_base_url(urlp, base, &p);
      _free(p);
    }
    free_deep_url(urlp);
    _free(urlp);
    _free(p2);
  }
}

/*******************************************************/
/* parse HTML document and extract URLs from it and if */
/* requested, also adjust content of document          */
/*******************************************************/
dllist *html_process_document(doc * html_doc, dllist ** formlist)
{
  char *base, *baset;
  html_parser_t *hp;
  html_extract_info_t einfo;
  html_rewrite_info_t rinfo;
  html_robots_info_t oinfo;
  int rewrite;
  int purestyle;
  int purescript;
  int follow = TRUE;

  /** call the -follow_cmd script **/
  if(priv_cfg.condition.follow_cmd)
  {
    int rv = uexit_follow_cmd(html_doc);

    if(rv == 0)
      follow = FALSE;
  }

  purestyle = (html_doc->doc_url->status & URL_STYLE);
  purescript = (html_doc->doc_url->status & URL_ISSCRIPT);

  einfo.prev_a = NULL;
  einfo.urls = NULL;
  einfo.no_limits = (cfg.mode == MODE_FTPDIR) || (cfg.dump_urlfd >= 0);
  einfo.only_inline = (cfg.mode == MODE_SINGLE) || cfg.singlepage;
  einfo.enable_js = cfg.enable_js;

  rinfo.einfo = &einfo;
  rinfo.all_to_local = cfg.all_to_local;
  rinfo.selected_to_local = cfg.sel_to_local;
  rinfo.all_to_remote = cfg.all_to_remote;

  oinfo.index = TRUE;
  oinfo.follow = TRUE;
  oinfo.images = TRUE;

  rewrite = cfg.rewrite_links && cfg.mode != MODE_FTPDIR;

  hp = html_parser_init(html_link_tags, html_link_tags_num(),
    rewrite, purestyle, purescript);

  /** urls in script are relative to HTML document     **/
  /** where it is called not relative to script itself **/
  if(purescript && html_doc->doc_url->parent_url)
    html_get_init_base_url((url *) html_doc->doc_url->parent_url->data, &base,
      &baset);
  else
    html_get_base_url(html_doc, &base, &baset);

  html_parser_set_base(hp, base, baset);
  html_parser_set_document(hp, html_doc->doc_url,
    html_doc->contents, html_doc->size);

  html_parser_add_tag_func(hp, html_parser_parse_tag, NULL);
  html_parser_add_tag_func(hp,
    (html_parser_func_t) html_parser_parse_tag_slash_a, &einfo);
  html_parser_add_tag_func(hp, html_parser_parse_tag_meta_refresh, NULL);

  if(cfg.condition.allow_robots)
    html_parser_add_tag_func(hp,
      (html_parser_func_t) html_parser_parse_tag_meta_robots, &oinfo);

  html_parser_add_attrib_func(hp, html_parser_url_to_absolute_url, NULL);
#ifdef HAVE_REGEX
  if(rewrite && cfg.remove_adv && priv_cfg.advert_res)
    html_parser_add_attrib_func(hp, html_parser_remove_advertisement, NULL);
#endif
  html_parser_add_attrib_func(hp, html_parser_process_base, NULL);
  html_parser_add_attrib_func(hp,
    (html_parser_func_t) html_parser_process_form, formlist);

  if(follow)
    html_parser_add_attrib_func(hp,
      (html_parser_func_t) html_parser_get_url, &einfo);

  if(rewrite && !cfg.post_update)
    html_parser_add_attrib_func(hp,
      (html_parser_func_t) html_parser_url_to_local, &rinfo);

  html_parser_add_style_func(hp, html_parser_style_to_absolute_urls, NULL);
  if(follow)
    html_parser_add_style_func(hp,
      (html_parser_func_t) html_parser_get_style_urls, &einfo);

  if(rewrite && !cfg.post_update)
    html_parser_add_style_func(hp,
      (html_parser_func_t) html_parser_style_to_local_urls, &rinfo);


  if(cfg.enable_js)
  {
    html_parser_add_script_func(hp, html_parser_parse_jspatterns, NULL);
    html_parser_add_script_func(hp, html_parser_parse_body_jspatterns, NULL);

#ifdef HAVE_REGEX
    if(priv_cfg.js_transform)
    {
      html_parser_add_tag_func(hp, html_parser_parse_tag_jstransform, NULL);
      html_parser_add_script_func(hp, html_parser_parse_body_jstransform,
        NULL);
    }
#endif
  }

  html_parser_parse(hp);

  if(rewrite)
  {
    _free(html_doc->contents);
    html_parser_take_document(hp, &html_doc->contents, &html_doc->size);
  }

  html_parser_kill(hp);

  /*** support for robots limits in META only ***/
  /*** nofollow supported, rest doesn't have  ***/
  /*** any real meaning in pavuk              ***/
  if(!oinfo.follow)
  {
    DEBUG_HTML("NOFOLLOW attribute in meta data found\n");
    while(einfo.urls)
    {
      free_deep_url((url *) einfo.urls->data);
      free((url *)einfo.urls->data);
      einfo.urls = dllist_remove_entry(einfo.urls, einfo.urls);
    }
  }

  return einfo.urls;
}

/*****************************************/
/* adjust URLs inside document to point  */
/* to present local documents            */
/*****************************************/
void html_process_parent_document(doc * html_doc, url * url_old,
  char *url_new)
{
  char *base, *baset;
  html_parser_t *hp;
  html_extract_info_t einfo;
  html_rewrite_info_t rinfo;
  html_change_info_t chinfo;
  int purestyle;
  int purescript;
  char *relfn = NULL;

  purestyle = (html_doc->doc_url->status & URL_STYLE);
  purescript = (html_doc->doc_url->status & URL_ISSCRIPT);

  if(cfg.all_to_local || cfg.sel_to_local || cfg.all_to_remote)
    return;

  einfo.prev_a = NULL;
  einfo.urls = NULL;
  einfo.no_limits = FALSE;
  einfo.only_inline = FALSE;
  einfo.enable_js = cfg.enable_js;

  rinfo.einfo = &einfo;
  rinfo.all_to_local = cfg.all_to_local;
  rinfo.selected_to_local = cfg.sel_to_local;
  rinfo.all_to_remote = cfg.all_to_remote;

  chinfo.url_old = url_old;

  if(url_new)
    chinfo.url_new = url_new;
  else if(cfg.post_update)
  {
    relfn = get_relative_path(url_to_filename(html_doc->doc_url, FALSE),
      url_to_filename(url_old, FALSE));

    chinfo.url_new = relfn;
  }
  else
    chinfo.url_new = NULL;

  hp = html_parser_init(html_link_tags, html_link_tags_num(),
    TRUE, purestyle, purescript);
  html_get_base_url(html_doc, &base, &baset);
  html_parser_set_base(hp, base, baset);
  html_parser_set_document(hp, html_doc->doc_url,
    html_doc->contents, html_doc->size);

  html_parser_add_tag_func(hp, html_parser_parse_tag, NULL);
  html_parser_add_tag_func(hp, html_parser_parse_tag_meta_refresh, NULL);

  if(chinfo.url_new)
    html_parser_add_attrib_func(hp,
      (html_parser_func_t) html_parser_change_url, &chinfo);

  if(!cfg.post_update)
    html_parser_add_attrib_func(hp,
      (html_parser_func_t) html_parser_url_to_local, &rinfo);

  if(chinfo.url_new)
    html_parser_add_style_func(hp,
      (html_parser_func_t) html_parser_style_change_url, &chinfo);

  if(!cfg.post_update)
    html_parser_add_style_func(hp,
      (html_parser_func_t) html_parser_style_to_local_urls, &rinfo);


  if(cfg.enable_js)
  {
    html_parser_add_script_func(hp, html_parser_parse_jspatterns, NULL);
    html_parser_add_script_func(hp, html_parser_parse_body_jspatterns, NULL);
  }

  html_parser_parse(hp);

  _free(html_doc->contents);
  html_parser_take_document(hp, &html_doc->contents, &html_doc->size);

  html_parser_kill(hp);

  _free(relfn);
}

/*************************************************/
/* load parent document adjust it and store back */
/* with locking and modification time preserving */
/*************************************************/
void rewrite_one_parent_links(url * doc_url, url * parent_url, char *dst_name)
{
  char pom[PATH_MAX];
  char *fnamep;
  char *rfn = NULL;
  char *savetmp, *p;
  int fd;
  doc pdoc;
  struct stat estat;
  struct utimbuf ut;
  int perm;
  url dum;

  DEBUG_PROCS("rewrite_one_parent_links()");
  if(!parent_url || !(parent_url->status & URL_DOWNLOADED))
    return;

  /*** parent document was not stored ***/
  if(!cfg.store_index && url_is_dir_index(parent_url))
    return;

  fnamep = url_to_filename(parent_url, FALSE);
  if(stat(fnamep, &estat) == 0)
  {
    if(S_ISDIR(estat.st_mode))
    {
      xprintf(1, gettext("Can't work on directory\n"));
      return;
    }
  }
  else
  {
    xperror("stat");
    return;
  }

  perm = estat.st_mode;
  ut.actime = estat.st_atime;
  ut.modtime = estat.st_mtime;

  memset(&dum, 0, sizeof(url));
  dum.type = URLT_FILE;
  dum.p.file.filename = fnamep;
  dum.local_name = fnamep;
  dum.status = parent_url->status & URL_STYLE;
  dum.status &= ~URL_REDIRECT;
  doc_init(&pdoc, &dum);
  pdoc.report_size = FALSE;

  if(doc_download(&pdoc, TRUE, TRUE))
  {
    doc_remove_lock(&pdoc);
    if(pdoc.errcode)
      report_error(&pdoc, gettext("rewrite parent"));
    return;
  }

  if(pdoc.errcode)
    report_error(&pdoc, gettext("rewrite parent"));

  _free(pdoc.mime);

  /* dst_name != NULL means child document was moved */
  if(dst_name &&
    !access(dst_name, R_OK) &&
    !stat(dst_name, &estat) && !S_ISDIR(estat.st_mode))
  {
    rfn = get_relative_path(fnamep, dst_name);
  }

  html_process_parent_document(&pdoc, doc_url, rfn);
  _free(rfn);

  strncpy(pom, fnamep, sizeof(pom) - 20);
  pom[sizeof(pom) - 21] = '\0';
  p = strrchr(pom, '/');
  if(p)
    sprintf(p + 1, "_*%d", (int) getpid());
  else
    snprintf(pom, sizeof(pom), "%s/_*%d", pom, (int) getpid());

  savetmp = tl_strdup(pom);
  rename(fnamep, savetmp);


  if((fd =
      open(fnamep, O_BINARY | O_CREAT | O_TRUNC | O_WRONLY,
        S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)) < 0)
  {
    xperror(fnamep);
    rename(savetmp, fnamep);
    doc_remove_lock(&pdoc);
    free(savetmp);
    free(pdoc.contents);
    return;
  }
  if(write(fd, pdoc.contents, pdoc.size) != pdoc.size)
  {
    xperror(fnamep);
    close(fd);
    rename(savetmp, fnamep);
    doc_remove_lock(&pdoc);
    free(savetmp);
    free(pdoc.contents);
    return;
  }
  close(fd);
  doc_remove_lock(&pdoc);
  utime(fnamep, &ut);
  chmod(fnamep, perm);
  unlink(savetmp);
  free(savetmp);
  free(pdoc.contents);
  DEBUG_PROCE("rewrite_one_parent_links()");
}

/*************************************************/
/* take all parent documents and adjust inside   */
/* all URLs, recurse up when document was moved  */
/*************************************************/
void rewrite_parents_links(url * doc_url, char *dst_name)
{
  char *fn = NULL;
  dllist *ptr;

  if((doc_url->status & URL_MOVED) && !dst_name)
    return;

  LOCK_URL(doc_url);
  for(ptr = doc_url->parent_url; ptr; ptr = ptr->next)
  {
    url *parent_url = (url *) ptr->data;

    if(cfg.rbreak)
      break;

    if(parent_url->status & URL_MOVED)
    {
      fn = dst_name ? dst_name : url_to_filename(doc_url, FALSE);

      rewrite_parents_links(parent_url, fn);
    }
    else
    {
      rewrite_one_parent_links(doc_url, parent_url, dst_name);
    }
  }
  UNLOCK_URL(doc_url);
}
