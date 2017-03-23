/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#include "url.h"
#include "doc.h"
#include "tools.h"
#include "html.h"
#include "http.h"
#include "ftp.h"
#include "myssl.h"
#include "abstract.h"
#include "recurse.h"
#include "mime.h"
#include "robots.h"
#include "mode.h"
#include "times.h"
#include "stats.h"
#include "errcode.h"
#include "cookie.h"
#include "log.h"
#include "gui_api.h"
#include "form.h"
#include "ainterface.h"
#include "gcinfo.h"

static void dump_ftp_list(dllist *);
static void dump_urls_list(dllist *);

#define SETNEXTURL  doc_cleanup(docu); \
      _free(pstr); \
                        return docu->errcode;

#ifdef HAVE_MT
static void _sigintthr(int nr)
{
#ifdef I_FACE
  if(!cfg.processing)
  {
    exit(0);
  }
#endif

  errno = EINTR;
  cfg.stop = TRUE;
  cfg.rbreak = TRUE;
}

static void _sigquitthr(int nr)
{
  pthread_exit(NULL);
}
#endif

static void reschedule_url(url * urlp)
{
  DEBUG_MISC(gettext("Rescheduling locked URL as no. %d\n"), cfg.total_cnt);
  LOCK_CFG_URLSTACK;
  cfg.urlstack = dllist_append(cfg.urlstack, (dllist_t) urlp);
#ifdef HAVE_MT
  mt_semaphore_up(&cfg.urlstack_sem);
#endif
  cfg.total_cnt++;
  UNLOCK_CFG_URLSTACK;
}

static void run_post_command(doc * docp)
{
  char *urlstr;
  char *cmd;

  DEBUG_MISC(gettext("Running post-processing command\n"));
  urlstr = url_to_urlstr(docp->doc_url, TRUE);

  cmd = tl_str_concat(NULL, priv_cfg.post_cmd, " \'",
    url_to_filename(docp->doc_url, FALSE),
    docp->is_parsable ? "\' 1 \'" : "\' 0 \'", urlstr, "\'", NULL);

  _free(urlstr);

  tl_system(cmd);

  _free(cmd);
}

static void add_matching_form(doc * docp, int nform, url_info * ui)
{
  char *ftext;
  int flen;
  form_info *fi;
  dllist *ptr, *fields, *sfields;
  url_info *nui;

  if(!(ftext = form_get_text(nform, docp->contents, docp->size, &flen)))
  {
    return;
  }

  fi = form_parse(ftext, flen);

  if(!fi)
    return;

  /* copy all fields supplied on cmdln */
  fields = NULL;
  for(ptr = ui->fields; ptr; ptr = ptr->next)
  {
    fields = dllist_prepend(fields, (dllist_t)
      form_field_duplicate((form_field *) ptr->data));
  }

  /* copy all suitable fields from HTML form */
  sfields = NULL;
  form_get_default_successful(NULL, fi->infos, &sfields);

  for(; sfields; sfields = dllist_remove_entry(sfields, sfields))
  {
    form_field *ff = (form_field *) sfields->data;

    if(dllist_find2(fields, (dllist_t) ff, form_field_compare_name))
    {
      _free(ff->name);
      _free(ff->value);
      _free(ff);
    }
    else
    {
      fields = dllist_prepend(fields, (dllist_t) ff);
    }
  }

  nui = url_info_new(fi->action);
  nui->type = URLI_FORM;
  nui->fields = fields;
  nui->encoding = fi->encoding;
  nui->method = fi->method;
  nui->localname = tl_strdup(ui->localname);

  form_free(fi);

  append_starting_url(nui, docp->doc_url);

  url_info_free(nui);
}

static void add_matching_forms(doc * docp, dllist * formlist)
{
  dllist *fptr, *uptr;
  int nform;

  for(fptr = formlist, nform = 0; fptr; fptr = fptr->next, nform++)
  {
    url *urlp;

    urlp = url_parse((char *) fptr->data);
    assert(urlp->type != URLT_FROMPARENT);


    if((urlp->type != URLT_HTTP) && (urlp->type != URLT_HTTPS))
    {
      free_deep_url(urlp);
      _free(urlp);
      continue;
    }
    free_deep_url(urlp);
    _free(urlp);

    for(uptr = priv_cfg.formdata; uptr; uptr = uptr->next)
    {
      url_info *ui = (url_info *) uptr->data;

      if(!strcmp(ui->urlstr, (char *) fptr->data))
      {
        add_matching_form(docp, nform, ui);
      }
    }
  }
}

int process_document(doc * docu, int check_lim)
{
  url *urlr;
  int nreget = 0, nredir = 0, pokus = 0;
  time_t atm;
  char cpom[64];
  char *pstr = NULL;
  int store_stat;
  struct stat estat;

  urlr = docu->doc_url;

  docu->check_limits = check_lim;

  _Xt_Serve;

  if(docu->check_limits)
    docu->check_limits = (urlr->parent_url != NULL);

  while(!cfg.stop && !cfg.rbreak)
  {
    _free(docu->ftp_pasv_host);
    docu->errcode = ERR_NOERROR;
    docu->mime = NULL;
    docu->type_str = NULL;
    docu->doc_url = urlr;
    docu->dtime = 0L;
    docu->contents = NULL;
    docu->is_chunked = FALSE;
    docu->read_chunksize = FALSE;
    docu->read_trailer = FALSE;
    docu->ftp_fatal_err = FALSE;
    pstr = url_to_urlstr(urlr, FALSE);

    if(pokus)
      xprintf(1, gettext("retry no. %d\n"), pokus);

#ifdef HAVE_MT
    xprintf(1, gettext("URL[%2d]: %5d(%d) of %5d  %s\n"), docu->threadnr + 1,
      docu->doc_nr, cfg.fail_cnt, cfg.total_cnt, pstr);
#else
    xprintf(1, gettext("URL: %5d(%d) of %5d  %s\n"), docu->doc_nr,
      cfg.fail_cnt, cfg.total_cnt, pstr);
#endif

#ifdef I_FACE
    if(cfg.xi_face)
    {
      gui_set_doccounter();

      gui_set_url(pstr);

      gui_set_status(gettext("Starting download"));
    }
#endif
    /*** to be able to revisit moved documents ***/
    /*** especially for authorization purposes ***/
    if((urlr->status & URL_PROCESSED) && urlr->moved_to && nredir)
    {
      urlr->status &= ~URL_PROCESSED;
    }

    if(docu->check_limits)
    {
      cond_info_t condp;

      condp.level = 2;
      condp.urlnr = docu->doc_nr;
      condp.size = 0;
      condp.time = 0L;
      condp.mimet = NULL;
      condp.full_tag = NULL;
      condp.params = NULL;
      condp.html_doc = NULL;
      condp.html_doc_offset = 0;
      condp.tag = NULL;
      condp.attrib = NULL;

      if(urlr->status & URL_PROCESSED)
      {
        xprintf(1, gettext("Already processed\n"));
        docu->errcode = ERR_PROCESSED;
        SETNEXTURL;
      }

      if(urlr->status & URL_USER_DISABLED)
      {
        xprintf(1, gettext("Disallowed by user\n"));
        docu->errcode = ERR_UDISABLED;
        SETNEXTURL;
      }

      if(!prottable[urlr->type].supported || (urlr->parent_url
      && (urlr->type == URLT_FTP || urlr->type == URLT_FTPS)
      && urlr->p.ftp.dir && !cfg.condition.ftpdir)
      || (urlr->parent_url && !url_append_condition(urlr, &condp)))
      {
        xprintf(1, gettext("Disallowed by rules\n"));

        urlr->status |= URL_REJECTED;
        docu->errcode = ERR_RDISABLED;
        SETNEXTURL;
      }

      gui_set_status(gettext("Checking \"robots.txt\""));

      if(!robots_check(urlr))
      {
        xprintf(1, gettext("Disallowed by \"robots.txt\"\n"));
        urlr->status |= URL_REJECTED;
        docu->errcode = ERR_RDISABLED;
        SETNEXTURL;
      }
    }

    if(cfg.mode == MODE_FTPDIR &&
      (urlr->type != URLT_FTP && urlr->type != URLT_FTPS))
    {
      xprintf(1,
        gettext("This URL type is not supported with ftpdir mode\n"));

      urlr->status |= URL_REJECTED;
      docu->errcode = ERR_RDISABLED;
      SETNEXTURL;
    }

    _Xt_Serve;

    if(cfg.mode == MODE_SYNC)
    {
      char *pp = url_to_filename(urlr, TRUE);

      if(!stat(pp, &estat) && !S_ISDIR(estat.st_mode))
      {
        atm = time(NULL) - 86400 * cfg.ddays;
        /*
           pro: We do not want the message
           "No transfer - file not expired"
           if the server's clock is ahead of our clock.
           If no parameter cfg.ddays is given, then
           we do not compare the file modification times.
         */
        if(cfg.ddays == 0 || estat.st_mtime < atm)
          docu->dtime = estat.st_mtime;
        else
        {
          xprintf(1, gettext("No transfer - file not expired\n"));
          urlr->status |= URL_REJECTED;
          docu->errcode = ERR_RDISABLED;
          SETNEXTURL;
        }
        urlr->status |= URL_ISLOCAL;
        docu->origsize = estat.st_size;
      }
    }

    if(cfg.show_time)
    {
      atm = time(NULL);
      LOCK_TIME;
      strftime(cpom, sizeof(cpom), "%H:%M:%S", localtime(&atm));
      UNLOCK_TIME;
      xprintf(1, gettext("Starting time :  %s\n"), cpom);
    }

#ifdef I_FACE
    if(cfg.stop || cfg.rbreak)
    {
      _free(pstr);
      break;
    }
#endif

    _Xt_Serve;

    if((urlr->type == URLT_FTP || urlr->type == URLT_FTP)
      && urlr->extension &&
      ((ftp_url_extension *) urlr->extension)->type == FTP_TYPE_L &&
      ((ftp_url_extension *) urlr->extension)->slink)
    {
      if(cfg.retrieve_slink)
      {
        /** need to kill extension, because we must **/
        /** guess the file type beside the symlink  **/
        ftp_url_ext_free(urlr->extension);
        urlr->extension = NULL;
      }
      else
      {
        ftp_make_symlink(urlr);
        urlr->status |= URL_PROCESSED;
        docu->errcode = ERR_NOERROR;
        SETNEXTURL;
      }
    }

    gui_set_status(gettext("Starting download"));

    if(doc_download(docu, FALSE, FALSE))
    {
      if(cfg.show_time)
      {
        atm = time(NULL);
        LOCK_TIME;
        strftime(cpom, sizeof(cpom), "%H:%M:%S", localtime(&atm));
        UNLOCK_TIME;
        xprintf(1, gettext("Ending time :    %s\n"), cpom);
      }

      _Xt_Serve;
      doc_remove_lock(docu);
      _free(docu->contents);

      report_error(docu, gettext("download"));
      DEBUG_USER("Error status code - (%d)\n");

      if((nreget < cfg.nreget &&
          (docu->errcode == ERR_HTTP_TRUNC ||
            docu->errcode == ERR_FTP_TRUNC ||
            docu->errcode == ERR_LOW_TRANSFER_RATE ||
            docu->errcode == ERR_HTTP_FAILREGET ||
            docu->errcode == ERR_HTTP_TIMEOUT ||
            docu->errcode == ERR_HTTP_GW_TIMEOUT)) ||
        (nredir < cfg.nredir &&
          docu->errcode == ERR_HTTP_REDIR) ||
        (docu->errcode == ERR_HTTP_AUTH) ||
        (docu->errcode == ERR_HTTP_PROXY_AUTH))
      {
        if(docu->errcode == ERR_HTTP_REDIR)
        {
          urlr->status |= URL_PROCESSED;
          if((urlr->moved_to->status & URL_PROCESSED) &&
            (!urlr->moved_to->moved_to))
          {
            SETNEXTURL;
          }
          else
          {
#ifdef I_FACE
            if(cfg.xi_face)
              gui_tree_set_icon_for_doc(docu);
#endif
            urlr = urlr->moved_to;
          }
        }

        if(docu->errcode == ERR_HTTP_TRUNC)
        {
          urlr->status |= URL_TRUNCATED;
          _free(docu->etag);

          docu->etag = get_mime_param_val_str("ETag:", docu->mime);
          if(!docu->etag)
            docu->etag =
              get_mime_param_val_str("Content-Location:", docu->mime);
          if(!docu->etag)
            docu->etag = get_mime_param_val_str("Last-Modified", docu->mime);
        }

        if(docu->errcode == ERR_HTTP_AUTH)
        {
          docu->doc_url->status |= URL_PROCESSED;
          docu->doc_url->status |= URL_ERR_REC;
          SETNEXTURL;
        }

        if(docu->errcode == ERR_HTTP_PROXY_AUTH)
        {
          docu->doc_url->status |= URL_PROCESSED;
          docu->doc_url->status |= URL_ERR_REC;
          SETNEXTURL;
        }

        _free(docu->mime);
        _free(docu->type_str);

        nreget += (docu->errcode == ERR_HTTP_TRUNC ||
          docu->errcode == ERR_FTP_TRUNC) && cfg.mode != MODE_SREGET;
        nredir += (docu->errcode == ERR_HTTP_REDIR);
        _free(pstr);
        continue;
      }

      if(docu->errcode == ERR_FTP_UNKNOWN ||
        docu->errcode == ERR_FTP_CONNECT ||
        docu->errcode == ERR_FTP_DATACON ||
        docu->errcode == ERR_FTPS_CONNECT ||
        docu->errcode == ERR_FTPS_DATASSLCONNECT ||
        docu->errcode == ERR_HTTP_UNKNOWN ||
        docu->errcode == ERR_HTTP_CONNECT ||
        docu->errcode == ERR_HTTP_SNDREQ ||
        docu->errcode == ERR_HTTP_SNDREQDATA ||
        docu->errcode == ERR_HTTP_RCVRESP ||
        docu->errcode == ERR_HTTP_SERV ||
        docu->errcode == ERR_HTTP_TIMEOUT ||
        docu->errcode == ERR_HTTP_PROXY_CONN ||
        docu->errcode == ERR_HTTPS_CONNECT ||
        docu->errcode == ERR_READ ||
        docu->errcode == ERR_ZERO_SIZE ||
        docu->errcode == ERR_GOPHER_CONNECT ||
        docu->errcode == ERR_PROXY_CONNECT || docu->errcode == ERR_HTTP_SERV)
      {
        urlr->status |= URL_ERR_REC;
        pokus++;
        /*** retry only when allowed ***/
        if(pokus >= cfg.nretry)
        {
          urlr->status |= URL_PROCESSED;
          SETNEXTURL;
        }
        _free(pstr);
        _free(docu->mime);
        _free(docu->type_str);
        continue;
      }
      else if(docu->errcode == ERR_LOCKED)
      {
        if(!cfg.urlstack)
        {
          xprintf(1,
            gettext("last document locked -> sleeping for 5 seconds\n"));
          tl_sleep(5);
        }
        reschedule_url(urlr);
        SETNEXTURL;
      }
      else if(docu->errcode == ERR_BIGGER ||
        docu->errcode == ERR_SMALLER ||
        docu->errcode == ERR_NOMIMET ||
        docu->errcode == ERR_OUTTIME || docu->errcode == ERR_SCRIPT_DISABLED)
      {
        urlr->status |= URL_PROCESSED;
        urlr->status |= URL_ERR_REC;
        SETNEXTURL;
      }
      else
      {
        /*** remove improper documents if required ***/
        if((cfg.remove_old &&
            (cfg.mode == MODE_SYNC ||
              cfg.mode == MODE_MIRROR)) &&
          (((docu->errcode == ERR_FTP_GET ||
                docu->errcode == ERR_FTP_BDIR ||
                docu->errcode == ERR_FTP_NODIR) &&
              docu->ftp_respc == 550) ||
            docu->errcode == ERR_HTTP_NFOUND ||
            docu->errcode == ERR_HTTP_GONE))
        {
          doc_remove(docu->doc_url);
        }

        urlr->status |= URL_ERR_UNREC;
        urlr->status |= URL_PROCESSED;

        SETNEXTURL;
      }
    }

    _Xt_Serve;

    if(urlr->status & URL_TRUNCATED)
      urlr->status &= ~URL_TRUNCATED;

    if(urlr->status & URL_ERR_REC)
      urlr->status &= ~URL_ERR_REC;

    if(cfg.show_time)
    {
      atm = time(NULL);
      LOCK_TIME;
      strftime(cpom, sizeof(cpom), "%H:%M:%S", localtime(&atm));
      UNLOCK_TIME;
      xprintf(1, gettext("Ending time :    %s\n"), cpom);
    }

    report_error(docu, gettext("download"));

    _Xt_Serve;

    if(docu->contents)
    {
      if(docu->is_parsable)
      {
        dllist *formlist = NULL;
        dllist *urls;

        gui_set_status(gettext("Relocating and scanning HTML document"));

        urls =
          html_process_document(docu, priv_cfg.formdata ? &formlist : NULL);

        _Xt_Serve;

        if(urls && cfg.dump_urlfd >= 0)
        {
          dump_urls_list(urls);
        }

        if(priv_cfg.formdata && formlist)
        {
          add_matching_forms(docu, formlist);
          while(formlist)
          {
            if(formlist->data) free((void *) formlist->data);
            formlist = dllist_remove_entry(formlist, formlist);
          }
        }

        if(cfg.mode != MODE_SREGET &&
          cfg.mode != MODE_FTPDIR && !(docu->doc_url->status & URL_NORECURSE))
        {
          gui_tree_add_start();
          cat_links_to_url_list(urls);
          gui_tree_add_end();
        }
        else if(cfg.mode == MODE_FTPDIR)
        {
          dump_ftp_list(urls);
        }
        else
        {
          for(; urls; urls = dllist_remove_entry(urls, urls))
          {
            free_deep_url((url *) urls->data);
            if(urls->data) free((url *)urls->data);
          }
        }

        _Xt_Serve;
      }

      store_stat = 0;

      if(cfg.dumpfd >= 0 && cfg.dump_after)
      {
        bufio *fd;

        gui_set_status(gettext("Dumping processed document"));
        LOCK_DUMPFD;
        fd = bufio_dupfd(cfg.dumpfd);

        if(docu->mime && cfg.dump_resp)
          bufio_write(fd, docu->mime, strlen(docu->mime));

        bufio_write(fd, docu->contents, docu->size);

        bufio_close(fd);
        UNLOCK_DUMPFD;
      }
      else if((docu->doc_url->type != URLT_FILE) &&
        !(docu->doc_url->status & URL_REDIRECT) &&
        (docu->errcode != ERR_HTTP_ACTUAL) &&
        (docu->errcode != ERR_FTP_ACTUAL) &&
        (cfg.mode != MODE_NOSTORE) &&
        (cfg.dumpfd < 0) && (cfg.mode != MODE_FTPDIR))
      {
        gui_set_status(gettext("Storing document"));

        store_stat = doc_store(docu, TRUE);

        if(store_stat)
        {
          xprintf(1, gettext("Store failed\n"));
          urlr->status &= ~URL_ERR_REC;
        }
      }

      _Xt_Serve;

      if(priv_cfg.post_cmd)
        run_post_command(docu);

      doc_remove_lock(docu);

      doc_update_parent_links(docu);
    }
    else
    {
      if(priv_cfg.post_cmd)
        run_post_command(docu);

      doc_remove_lock(docu);

      doc_update_parent_links(docu);
    }

    urlr->status |= URL_DOWNLOADED;
    urlr->status |= URL_PROCESSED;
    SETNEXTURL;
  }
  return ERR_UNKNOWN;
}

#ifdef I_FACE
int download_single_doc(url * urlp)
{
  int rv;
  doc docu;
  global_connection_info con_info;
#if defined(HAVE_MT) && defined(I_FACE)
  _config_struct_priv_t privcfg;

#if defined (__OSF__) || defined (__osf__)
#define __builtin_try
#define __builtin_finally
#endif

  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif

  gui_start_download(FALSE);

#ifdef HAVE_MT
  {
    sigset_t smask;

    sigemptyset(&smask);
    sigaddset(&smask, SIGINT);
    sigaddset(&smask, SIGQUIT);
    pthread_sigmask(SIG_UNBLOCK, &smask, NULL);

    signal(SIGINT, _sigintthr);
    signal(SIGQUIT, _sigquitthr);
  }

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setspecific(cfg.thrnr_key, (void *) 0);
  DEBUG_MTTHR("starting thread(%ld) %d\n", pthread_self(), 0);

  cfg.allthreadsnr = 0;
  gui_mt_thread_start(cfg.allthreadsnr);
#endif

  cfg.rbreak = FALSE;
  cfg.stop = FALSE;
  cfg.processing = TRUE;

  doc_init(&docu, urlp);
#ifdef HAVE_MT
  docu.threadnr = 0;
  pthread_setspecific(cfg.currdoc_key, (void *) NULL);
  pthread_setspecific(cfg.herrno_key, (void *) (&(docu.__herrno)));
#endif
  rv = process_document(&docu, FALSE);

  init_global_connection_data(&con_info);
  save_global_connection_data(&con_info, &docu);
  kill_global_connection_data(&con_info);

  cfg.processing = FALSE;

  cfg.rbreak = FALSE;
  cfg.stop = FALSE;

#if defined(HAVE_MT) && defined(I_FACE)
  pthread_cleanup_pop(TRUE);
#endif
#ifdef HAVE_MT
  doc_finish_processing(&docu);
  cfg.allthreadsnr = 0;
  gui_mt_thread_end(0);
#endif
  gui_beep();
  gui_set_msg(gettext("Done"), 0);

  return rv;
}
#endif

/*********************************************/
/* rekurzivne prechadzanie stromu dokumentov */
/* FIXME: Translate me!                      */
/*********************************************/
#ifdef HAVE_MT
static void _recurse(int thnr)
#else
void recurse(int thnr)
#endif
{
  bool_t rbreaksave, stopsave;

  global_connection_info con_info;

  if(cfg.urlstack == NULL)
    return;

  init_global_connection_data(&con_info);

/**** obsluzenie vsetkych URL v zozname ****/
/**** FIXME: Translate me!              ****/
  while(cfg.urlstack && !cfg.stop)
  {
    doc docu;
    url *urlp;

    LOCK_CFG_URLSTACK;
    if(cfg.urlstack)
    {
      urlp = (url *) cfg.urlstack->data;
      cfg.urlstack = dllist_remove_entry(cfg.urlstack, cfg.urlstack);
#ifdef HAVE_MT
      mt_semaphore_decrement(&cfg.urlstack_sem);
#endif
      UNLOCK_CFG_URLSTACK;
    }
    else
    {
      UNLOCK_CFG_URLSTACK;
      break;
    }

    doc_init(&docu, urlp);

#ifdef HAVE_MT
    docu.threadnr = thnr;
    pthread_setspecific(cfg.currdoc_key, (void *) (&docu));
    pthread_setspecific(cfg.herrno_key, (void *) (&(docu.__herrno)));
#endif

    LOCK_DCNT;
    cfg.docnr++;
    docu.doc_nr = cfg.docnr;
    UNLOCK_DCNT;

    restore_global_connection_data(&con_info, &docu);

    process_document(&docu, TRUE);

    save_global_connection_data(&con_info, &docu);

#ifdef HAVE_MT
    doc_finish_processing(&docu);
#endif

    if(docu.errcode == ERR_QUOTA_FS ||
      docu.errcode == ERR_QUOTA_TRANS ||
      docu.errcode == ERR_QUOTA_TIME || cfg.rbreak)
    {
      LOCK_CFG_URLSTACK;
      cfg.docnr--;
      cfg.urlstack = dllist_prepend(cfg.urlstack, (dllist_t) urlp);
#ifdef HAVE_MT
      mt_semaphore_up(&cfg.urlstack_sem);
#endif
      UNLOCK_CFG_URLSTACK;
      break;
    }
  }
#if defined(I_FACE) && !defined(HAVE_MT)
  if(cfg.xi_face)
  {
    gui_set_status(gettext("Done"));
  }
#endif

#ifdef I_FACE
  if(cfg.xi_face)
    gui_set_doccounter();
#endif

  stopsave = cfg.stop;
  rbreaksave = cfg.rbreak;
  cfg.stop = FALSE;
  cfg.rbreak = FALSE;

  kill_global_connection_data(&con_info);

  if(cfg.update_cookies)
  {
    cookie_update_file(TRUE);
  }

  cfg.stop = stopsave;
  cfg.rbreak = rbreaksave;

  if(!cfg.rbreak && !cfg.stop && cfg.stats_file)
  {
    stats_fill_spage(cfg.stats_file, NULL);
  }
}

#ifdef HAVE_MT
static void _recurse_thrd(int thrnr)
{
  bool_t init = (thrnr == 0);
#ifdef I_FACE
  _config_struct_priv_t privcfg;
#endif

  {
    sigset_t smask;

    sigemptyset(&smask);
    sigaddset(&smask, SIGINT);
    sigaddset(&smask, SIGQUIT);
    pthread_sigmask(SIG_UNBLOCK, &smask, NULL);

    signal(SIGINT, _sigintthr);
    signal(SIGQUIT, _sigquitthr);
  }

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setspecific(cfg.currdoc_key, (void *) NULL);
  pthread_setspecific(cfg.thrnr_key, (void *) thrnr);
  DEBUG_MTTHR("starting thread(%ld) %d\n", pthread_self(), thrnr);

#ifdef I_FACE
  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif

  for(; !cfg.rbreak && !cfg.stop;)
  {
    int v;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DEBUG_MTTHR("thread %d awaking\n", thrnr);

    _recurse(thrnr);
    init = FALSE;
    gui_clear_status();
    DEBUG_MTTHR("thread %d sleeping\n", thrnr);
    gui_set_status(gettext("Sleeping ..."));
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    mt_semaphore_up(&cfg.nrunning_sem);
    /* UN-critical section */

    while(!cfg.stop && !cfg.rbreak &&
      (v = mt_semaphore_timed_wait(&cfg.urlstack_sem, 400)) < 0);

    mt_semaphore_decrement(&cfg.nrunning_sem);
  }
#ifdef I_FACE
  pthread_cleanup_pop(TRUE);
#endif
  DEBUG_MTTHR("thread %d exiting\n", thrnr);
  gui_set_status(gettext("Exiting ..."));
  pthread_exit(NULL);
}

void recurse(int dumb)
{
  pthread_attr_t thrdattr;
  int i;
  int num = cfg.nthr;
  sigset_t smask;

  sigemptyset(&smask);
  sigaddset(&smask, SIGINT);
  sigaddset(&smask, SIGQUIT);
  pthread_sigmask(SIG_UNBLOCK, &smask, NULL);

  signal(SIGQUIT, _sigquitthr);

  pthread_attr_init(&thrdattr);
  pthread_attr_setscope(&thrdattr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setstacksize(&thrdattr, MT_STACK_SIZE);
  mt_semaphore_init(&cfg.nrunning_sem);

  if(num <= 0)
    num = 1;

  cfg.allthreadsnr = 0;
  cfg.allthreads = _malloc(num * sizeof(pthread_t));

  mt_semaphore_decrement(&cfg.urlstack_sem);

  for(i = 0; i < num; i++)
  {
    if(!pthread_create(&(cfg.allthreads[cfg.allthreadsnr]),
        &thrdattr, (void *) _recurse_thrd, (void *) cfg.allthreadsnr))
    {
      cfg.allthreadsnr++;
      gui_mt_thread_start(cfg.allthreadsnr);
      mt_semaphore_decrement(&cfg.nrunning_sem);
    }
    else
    {
      char pom[100];
      sprintf(pom, "Create downloading thread %d", i);
      xperror(pom);
    }
    if(cfg.rbreak || cfg.stop)
      break;
  }

  while(!cfg.stop && !cfg.rbreak &&
    mt_semaphore_timed_down(&cfg.nrunning_sem, 500) < 0);

  cfg.stop = TRUE;

  tl_msleep(300);

  for(i = 0; i < cfg.allthreadsnr; i++)
  {
/*
    pthread_cancel(cfg.allthreads[i]);
    pthread_kill(cfg.allthreads[i], SIGQUIT);
*/
    pthread_join(cfg.allthreads[i], NULL);
  }

  mt_semaphore_destroy(&cfg.nrunning_sem);
  _free(cfg.allthreads);
  cfg.allthreadsnr = 0;
  gui_mt_thread_end(0);
}
#endif

static void dump_ftp_list(dllist * urllst)
{
  dllist *ptr = urllst;

  while(ptr)
  {
    url *urlp = (url *) ptr->data;
    void *dupl;

    dupl = dllist_find2(ptr->next, (dllist_t) urlp, dllist_url_compare);

    if(!dupl && !(urlp->status & URL_INLINE_OBJ) &&
      (urlp->type == URLT_FTP || urlp->type == URLT_FTPS))
    {
      char *p, *pp;

      p = url_get_path(urlp);
      pp = strrchr(p, '/');
      if(pp)
      {
        pp++;
        if(!*pp)
        {
          pp -= 2;
          while(pp > p && *pp != '/')
            pp--;
          pp++;
        }
        if(urlp->extension)
        {
          ftp_url_extension *fe = urlp->extension;

          if(fe->type == FTP_TYPE_F)
            xprintf(1, gettext("\t%s    (%d bytes)\n"), pp, fe->size);
          else if(fe->type == FTP_TYPE_L)
            xprintf(1, "\t%s    -> %s\n", pp, fe->slink);
          else if(fe->type == FTP_TYPE_D)
            xprintf(1, "\t%s/\n", pp, fe->slink);
        }
        else
          xprintf(1, "\t%s\n", pp);
      }
    }
    free_deep_url(urlp);
    free(urlp);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

static void dump_urls_list(dllist * urls)
{
  dllist *ptr;

  LOCK_DUMPURLS;
  for(ptr = urls; ptr; ptr = ptr->next)
  {
    void *dupl;

    dupl = dllist_find2(ptr->next, (dllist_t) ptr->data, dllist_url_compare);

    if(!dupl)
    {
      char *ustr = url_to_urlstr((url *) ptr->data, FALSE);

      if(ustr)
      {
        write(cfg.dump_urlfd, ustr, strlen(ustr));
        write(cfg.dump_urlfd, "\n", 1);
        free(ustr);
      }
    }
  }
  UNLOCK_DUMPURLS;
}

void get_urls_to_resume(char *dirname)
{
  DIR *dir;
  struct dirent *dent;
  char next_dir[PATH_MAX];
  struct stat estat;
  url *purl;

  if(!(dir = opendir(dirname)))
  {
    xperror(dirname);
    return;
  }

  gui_set_msg(gettext("Searching for files to resume"), 0);

  while((dent = readdir(dir)))
  {
    _Xt_Serve;

    snprintf(next_dir, sizeof(next_dir), "%s/%s", dirname, dent->d_name);
    if(!strcmp(dent->d_name, "."))
      continue;
    if(!strcmp(dent->d_name, ".."))
      continue;
    if(lstat(next_dir, &estat))
    {
      xperror(next_dir);
      continue;
    }

    if(S_ISDIR(estat.st_mode))
    {
      if(!strcmp(dent->d_name, ".pavuk_info") && cfg.enable_info)
        continue;

      get_urls_to_resume(next_dir);
    }
    else if(!strncmp(".in_", dent->d_name, 4))
    {
      snprintf(next_dir, sizeof(next_dir), "%s/%s", dirname, dent->d_name + 4);
      if((purl = filename_to_url(next_dir)))
      {
        if(cfg.mode != MODE_MIRROR)
        {
          xprintf(1, gettext("Adding %s to resume list\n"), next_dir);
        }
        purl->status |= URL_ISSTARTING;
        url_set_filename(purl, tl_strdup(next_dir));
        append_url_to_list(purl);
      }
    }

#ifdef I_FACE
    if(cfg.xi_face && (cfg.rbreak || cfg.stop))
      break;
#endif
  }

  closedir(dir);
}

void get_urls_to_synchronize(char *dirname, dllist ** list)
{
  DIR *dir;
  struct dirent *dent;
  char next_dir[PATH_MAX];
  struct stat estat;
  url *purl;


  if(!(dir = opendir(dirname)))
  {
    xperror(dirname);
    return;
  }

  gui_set_msg(gettext("Searching for documents to synchronize"), 0);

  while((dent = readdir(dir)))
  {
    _Xt_Serve;

    snprintf(next_dir, sizeof(next_dir), "%s/%s", dirname, dent->d_name);
    if(!strcmp(dent->d_name, "."))
      continue;
    if(!strcmp(dent->d_name, ".."))
      continue;
    if(lstat(next_dir, &estat))
    {
      xperror(next_dir);
      continue;
    }

    if(S_ISDIR(estat.st_mode))
    {
      if(!strcmp(dent->d_name, ".pavuk_info") && cfg.enable_info)
        continue;

      strcat(next_dir, "/");

      if((purl = filename_to_url(next_dir)) &&
        purl->type == URLT_FTP && !cfg.store_index)
      {
        purl->status |= URL_ISSTARTING;
        purl->extension = ftp_url_ext_new(FTP_TYPE_D, -1, -1, NULL, 0);
        url_set_filename(purl,
          tl_str_concat(NULL, next_dir, priv_cfg.index_name, NULL));
        *list = dllist_prepend(*list, (dllist_t) purl);
      }
      else if(purl)
      {
        free_deep_url(purl);
        _free(purl);
      }

      next_dir[strlen(next_dir) - 1] = '\0';

      get_urls_to_synchronize(next_dir, list);
    }
    else if(cfg.enable_info && !strcmp(dent->d_name, ".lock"))
    {
      /* do nothing */
      continue;
    }
    else if(!strncmp(".in_", dent->d_name, 4))
    {
      snprintf(next_dir, sizeof(next_dir), "%s/%s", dirname, dent->d_name + 4);
      if((purl = filename_to_url(next_dir)))
      {
        char *ustr;

        ustr = url_to_urlstr(purl, FALSE);
        if(cfg.mode != MODE_MIRROR)
        {
          xprintf(1, gettext("Adding file %s to sync list as URL %s\n"),
            next_dir, ustr);
        }
        _free(ustr);
        if(purl->type == URLT_FTP)
        {
          int tp;

          if(purl->p.ftp.dir)
            tp = FTP_TYPE_D;
          else
            tp = FTP_TYPE_F;
          purl->extension = ftp_url_ext_new(tp, -1, -1, NULL, 0);
        }
        purl->status |= URL_ISSTARTING;

        url_set_filename(purl, tl_strdup(next_dir));
        *list = dllist_prepend(*list, (dllist_t) purl);
      }
    }
    else
    {
      if((purl = filename_to_url(next_dir)))
      {
        char *ustr;

        ustr = url_to_urlstr(purl, FALSE);
        if(cfg.mode != MODE_MIRROR)
        {
          xprintf(1, gettext("Adding file %s to sync list as URL %s\n"),
            next_dir, ustr);
        }
        _free(ustr);

        if(purl->type == URLT_FTP)
        {
          int tp;

          if(purl->p.ftp.dir)
            tp = FTP_TYPE_D;
#ifdef S_ISLNK
          else if(S_ISLNK(estat.st_mode))
            tp = FTP_TYPE_L;
#endif
          else
            tp = FTP_TYPE_F;
          purl->extension = ftp_url_ext_new(tp, -1, -1, NULL, 0);
        }

        purl->status |= URL_ISSTARTING;
        url_set_filename(purl, tl_strdup(next_dir));
        *list = dllist_prepend(*list, (dllist_t) purl);
      }
    }

#ifdef I_FACE
    if(cfg.xi_face && (cfg.rbreak || cfg.stop))
      break;
#endif
  }

  closedir(dir);
}
