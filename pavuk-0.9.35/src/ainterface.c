/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "ainterface.h"
#include "recurse.h"
#include "update_links.h"
#include "tools.h"
#include "remind.h"
#include "form.h"
#include "stats.h"
#include "gui_api.h"
#include "jsbind.h"
#include "myssl.h"

void free_all(void)
{
  while(cfg.urlstack)
    cfg.urlstack = dllist_remove_entry(cfg.urlstack, cfg.urlstack);
  while(cfg.urls_in_dir)
    cfg.urls_in_dir = dllist_remove_entry(cfg.urls_in_dir, cfg.urls_in_dir);

  if(cfg.url_hash_tbl)
    dlhash_empty(cfg.url_hash_tbl);
  if(cfg.fn_hash_tbl)
    dlhash_empty(cfg.fn_hash_tbl);

  cfg.urlstack = NULL;
  cfg.docnr = 0;

#ifdef GTK_FACE
  gui_clear_tree();
  form_edit_dlg_clear();
  stats_clear();
#endif
}

url *append_starting_url(url_info * ui, url * parent)
{
  url *urlp;

  urlp = url_parse(ui->urlstr);
  assert(urlp->type != URLT_FROMPARENT);

  if((urlp->type == URLT_FILE) && access(urlp->p.file.filename, F_OK) &&
    (*ui->urlstr != '/') && prottable[urlp->type].supported)
  {
    char *p = NULL;

    free_deep_url(urlp);
    _free(urlp);
    if(!strncasecmp(ui->urlstr, "gopher.", 7))
      p = tl_str_concat(p, "gopher://", ui->urlstr, NULL);
    else if(!strncasecmp(ui->urlstr, "ftp.", 4))
      p = tl_str_concat(p, "ftp://", ui->urlstr, NULL);
    else if(!strncasecmp(ui->urlstr, "ssl.", 4))
      p = tl_str_concat(p, "https://", ui->urlstr, NULL);
    else
      p = tl_str_concat(p, "http://", ui->urlstr, NULL);
    urlp = url_parse(p);
    assert(urlp->type != URLT_FROMPARENT);
    _free(p);
  }
  if(!cfg.urlstack)
    urlp->status |= URL_ISFIRST;

  if(ui->localname)
    url_set_filename(urlp, ui->localname);

  if(parent)
  {
    urlp->parent_url = dllist_append(urlp->parent_url, (dllist_t)parent);
    urlp->level = parent->level + 1;
  }
  else
  {
    urlp->status |= URL_ISSTARTING;
  }

  if(ui->type == URLI_FORM)
  {
    form_info *fi;
    dllist *ptr;

    urlp->status |= URL_FORM_ACTION;

    fi = _malloc(sizeof(form_info));

    fi->method = ui->method;
    fi->encoding = ui->encoding;
    fi->action = NULL;
    fi->text = NULL;
    fi->infos = NULL;
    fi->parent_url = NULL;

    ptr = ui->fields;
    while(ptr)
    {
      fi->infos = dllist_append(fi->infos,
        (dllist_t)form_field_duplicate((form_field *) ptr->data));

      ptr = ptr->next;
    }
    urlp->extension = fi;
  }

  if(!prottable[urlp->type].supported || url_was_befor(urlp))
  {
    if(!prottable[urlp->type].supported)
    {
      xprintf(1, gettext("Removing unsupported URL: %s\n"), ui->urlstr);
    }
    free_deep_url(urlp);
    _free(urlp);
  }
  else
    append_url_to_list(urlp);

  return urlp;
}

static void _append_starting_urls(void)
{
  dllist *dptr;

  for(dptr = cfg.request; dptr; dptr = dptr->next)
  {
    url_info *ui = (url_info *) dptr->data;
    append_starting_url(ui, NULL);
  }
}

static void absi_conf(void)
{
#ifdef HAVE_MOZJS
  pjs_destroy();
  pjs_init();
#endif
#ifdef USE_SSL
  my_ssl_init_start();
#endif
}

void absi_restart(void)
{
  int i;

#ifdef I_FACE
#ifdef HAVE_MT
  _config_struct_priv_t privcfg;
#if defined (__OSF__) || defined (__osf__)
#define __builtin_try
#define __builtin_finally
#endif

  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif
  cfg.rbreak = FALSE;
  cfg.stop = FALSE;
#endif

  absi_conf();

  cfg.start_time = time(NULL);
  gettimeofday(&cfg.hr_start_time, NULL);
  cfg.fail_cnt = 0;
  cfg.process_cnt = 0;
  cfg.reject_cnt = 0;
  cfg.mode_started = FALSE;
  cfg.prev_mode = cfg.mode;

  cfg.trans_size = 0;

  /*** cleanup ***/
  free_all();

  gui_create_tree_root_node();

  cfg.total_cnt = 0;
  cfg.urlstack = NULL;

  switch (cfg.mode)
  {
  case MODE_SINGLE:
  case MODE_SREGET:
  case MODE_NORMAL:
  case MODE_NOSTORE:
  case MODE_FTPDIR:
    if(!cfg.xi_face)
      if(!cfg.request)
        usage_short();

    if(cfg.request)
    {
      _append_starting_urls();
      cfg.mode_started = TRUE;
      recurse(TRUE);
    }
    else if(cfg.xi_face)
      gui_set_msg(gettext("Specify at least one starting URL!"), 5);
    else
      xprintf(1, gettext("Specify at least one starting URL!\n"));
    break;
  case MODE_LNUPD:
    if(!priv_cfg.subdir)
    {
      for(i = 0; i < NUM_ELEM(prottable); i++)
      {
        if(prottable[i].supported && prottable[i].dirname)
        {
          char *pom;

          pom = tl_str_concat(NULL, priv_cfg.cache_dir,
            "/", prottable[i].dirname, NULL);

          if(!access(pom, F_OK))
          {
            LOCK_DIRR;
            update_links(pom);
            UNLOCK_DIRR;
          }
          _free(pom);
        }
      }
    }
    else
    {
      LOCK_DIRR;
      update_links(priv_cfg.subdir);
      UNLOCK_DIRR;
    }
    break;
  case MODE_SYNC:
    cfg.total_cnt = 0;
    cfg.urlstack = NULL;
    if(cfg.request)
    {
      _append_starting_urls();
    }
    if((cfg.remove_old && priv_cfg.subdir) || !cfg.request)
    {
      if(!priv_cfg.subdir)
      {
        for(i = 0; i < NUM_ELEM(prottable); i++)
        {
          if(prottable[i].supported && prottable[i].dirname)
          {
            char *pom;

            pom = tl_str_concat(NULL, priv_cfg.cache_dir,
              "/", prottable[i].dirname, NULL);

            if(!access(pom, F_OK))
            {
              LOCK_DIRR;
              get_urls_to_synchronize(pom, &cfg.urls_in_dir);
              UNLOCK_DIRR;
            }
            _free(pom);
          }
        }
      }
      else
      {
        LOCK_DIRR;
        get_urls_to_synchronize(priv_cfg.subdir, &cfg.urls_in_dir);
        UNLOCK_DIRR;
      }
      /* rather check files from directory scan */
      /* before each others because we can this */
      /* way workaround the inability to use %E */
      /* and %M in -fnrules and it won't break  */
      /* anything         */
      /* if (!cfg.request) */
      {
        while(cfg.urls_in_dir)
        {
          url *purl = (url *) cfg.urls_in_dir->data;

          append_url_to_list(purl);
          cfg.urls_in_dir =
            dllist_remove_entry(cfg.urls_in_dir, cfg.urls_in_dir);
        }
      }
    }
    cfg.mode_started = TRUE;
    recurse(TRUE);
    if(!cfg.urlstack)
    {
      while(cfg.urls_in_dir)
      {
        url *purl = (url *) cfg.urls_in_dir->data;

        purl->status |= URL_NORECURSE;
        if(url_was_befor(purl))
        {
          free_deep_url(purl);
          _free(purl);
        }
        else
          append_url_to_list(purl);
        cfg.urls_in_dir =
          dllist_remove_entry(cfg.urls_in_dir, cfg.urls_in_dir);
      }
      if(cfg.urlstack)
        recurse(FALSE);
    }
    break;

  case MODE_MIRROR:
    cfg.total_cnt = 0;
    cfg.urlstack = NULL;
    if(cfg.request)
    {
      _append_starting_urls();
    }
    if((cfg.remove_old && priv_cfg.subdir) || !cfg.request)
    {
      if(!priv_cfg.subdir)
      {
        for(i = 0; i < NUM_ELEM(prottable); i++)
        {
          if(prottable[i].supported && prottable[i].dirname)
          {
            char *pom;

            pom = tl_str_concat(NULL, priv_cfg.cache_dir,
              "/", prottable[i].dirname, NULL);

            if(!access(pom, F_OK))
            {
              LOCK_DIRR;
              get_urls_to_synchronize(pom, &cfg.urls_in_dir);
              UNLOCK_DIRR;
            }
            _free(pom);
          }
        }
      }
      else
      {
        LOCK_DIRR;
        get_urls_to_synchronize(priv_cfg.subdir, &cfg.urls_in_dir);
        UNLOCK_DIRR;
      }

      /* we will not be able to use %E */
      /* and %M in -fnrules            */
    }
    cfg.mode_started = TRUE;
    recurse(TRUE);

    if(!cfg.urlstack)
    {
      /*
         If everything was successful we remove all files
         we had before that were not downloaded
       */
      while(cfg.urls_in_dir)
      {
        url *purl = (url *) cfg.urls_in_dir->data;

        purl->status |= URL_NORECURSE;
        if(url_was_befor(purl))
        {
          free_deep_url(purl);
          _free(purl);
        }
        else
        {
          printf("no longer there; delete '%s'\n", purl->local_name);
          doc_remove(purl);
        }
        cfg.urls_in_dir =
          dllist_remove_entry(cfg.urls_in_dir, cfg.urls_in_dir);
      }
    }

    break;
  case MODE_RESUME:
    cfg.total_cnt = 0;
    cfg.urlstack = NULL;
    if(!priv_cfg.subdir)
    {
      for(i = 0; i < NUM_ELEM(prottable); i++)
      {
        if(prottable[i].supported && prottable[i].dirname)
        {
          char *pom;

          pom = tl_str_concat(NULL, priv_cfg.cache_dir,
            "/", prottable[i].dirname, NULL);

          if(!access(pom, F_OK))
          {
            LOCK_DIRR;
            get_urls_to_resume(pom);
            UNLOCK_DIRR;
          }
          _free(pom);
        }
      }
    }
    else
    {
      LOCK_DIRR;
      get_urls_to_resume(priv_cfg.subdir);
      UNLOCK_DIRR;
    }
    cfg.mode_started = TRUE;
    recurse(TRUE);
    break;
  case MODE_REMIND:
    remind_load_db();
    remind_start_add();
    remind_do();
    remind_save_db();
    if(!cfg.stop && !cfg.rbreak)
      remind_send_result();
    break;
  default:
    break;
  }

#if defined(I_FACE) && defined(HAVE_MT)
  pthread_cleanup_pop(TRUE);
  if(cfg.xi_face)
    gui_finish_download(FALSE);
#endif
}

void absi_cont(void)
{
  int i;

#if defined(I_FACE) && defined(HAVE_MT)
  _config_struct_priv_t privcfg;
#endif

  if(cfg.mode == MODE_MIRROR)
    return;

#ifdef I_FACE
#ifdef HAVE_MT
  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif
  cfg.rbreak = FALSE;
  cfg.stop = FALSE;
#endif

  absi_conf();

  switch (cfg.mode)
  {
  case MODE_SINGLE:
  case MODE_SREGET:
  case MODE_RESUME:
  case MODE_NORMAL:
  case MODE_NOSTORE:
  case MODE_FTPDIR:
    recurse(FALSE);
    break;
  case MODE_SYNC:
    recurse(FALSE);
    if(!cfg.urlstack)
    {
      while(cfg.urls_in_dir)
      {
        url *purl = (url *) cfg.urls_in_dir->data;

        purl->status |= URL_NORECURSE;
        if(url_was_befor(purl))
        {
          free_deep_url(purl);
          _free(purl);
        }
        else
          append_url_to_list(purl);
        cfg.urls_in_dir =
          dllist_remove_entry(cfg.urls_in_dir, cfg.urls_in_dir);
      }
      if(cfg.urlstack)
        recurse(FALSE);
    }
    break;
  case MODE_LNUPD:
    if(!priv_cfg.subdir)
    {
      for(i = 0; i < NUM_ELEM(prottable); i++)
      {
        if(prottable[i].supported && prottable[i].dirname)
        {
          char *pom;

          pom = tl_str_concat(NULL, priv_cfg.cache_dir,
            "/", prottable[i].dirname, NULL);

          if(!access(pom, F_OK))
          {
            LOCK_DIRR;
            update_links(pom);
            UNLOCK_DIRR;
          }

          _free(pom);
        }
      }
    }
    else
    {
      LOCK_DIRR;
      update_links(priv_cfg.subdir);
      UNLOCK_DIRR;
    }
    break;
  case MODE_REMIND:
    remind_start_add();
    remind_do();
    remind_save_db();
    if(!cfg.stop && !cfg.rbreak)
      remind_send_result();
  default:
    break;
  }
#if defined(I_FACE) && defined(HAVE_MT)
  pthread_cleanup_pop(TRUE);
  if(cfg.xi_face)
    gui_finish_download(FALSE);
#endif
}

#ifdef GETTEXT_NLS
char **get_available_languages(void)
{
  DIR *dir;
  struct dirent *dent;
  char msgfile[PATH_MAX];
  char **retv = NULL;
  int nr = 0;

  LOCK_DIRR;
  if(!cfg.msgcatd || !(dir = opendir(cfg.msgcatd)))
  {
    UNLOCK_DIRR;
    xprintf(0, gettext("Can't list available message catalogs\n"));
    return NULL;
  }

  while((dent = readdir(dir)))
  {
    if(!strcmp(dent->d_name, "."))
      continue;
    if(!strcmp(dent->d_name, ".."))
      continue;
    snprintf(msgfile, sizeof(msgfile), "%s/%s/LC_MESSAGES/%s.mo",
      cfg.msgcatd, dent->d_name, PACKAGE);

    if(!access(msgfile, R_OK))
    {
      nr++;
      retv = _realloc(retv, (nr + 1) * sizeof(char *));
      retv[nr - 1] = tl_strdup(dent->d_name);
      retv[nr] = NULL;
    }
  }

  closedir(dir);
  UNLOCK_DIRR;

  if(retv)
    tl_strv_sort(retv);

  return retv;
}
#endif
