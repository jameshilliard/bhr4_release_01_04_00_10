/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include "remind.h"

#include "bufio.h"
#include "url.h"
#include "mime.h"
#include "times.h"
#include "http.h"
#include "dllist.h"
#include "tools.h"
#include "gcinfo.h"
#include "gui_api.h"

static dllist *remind_urls = NULL;

static int remind_check_if_newer(doc * docp, time_t modtime, time_t * nmdtm)
{
  char *p;
  int retv = 0;
  http_response *rsp;
  char *ustr;

  ustr = url_to_urlstr(docp->doc_url, FALSE);

  xprintf(1, gettext("Checking: %s\n"), ustr);

  if(!(docp->doc_url->type == URLT_HTTP
#ifdef USE_SSL
      || docp->doc_url->type == URLT_HTTPS
#endif
    ))
  {
    xprintf(1, "This URL type is not supported in reminder mode\n");
    return -1;
  }

  /*** to init properly proxy ... ***/
  doc_download_init(docp, FALSE);

  docp->request_type = HTTP_REQ_HEAD;
  docp->datasock = http_get_data_socket(docp);

  if(!docp->datasock)
    return -1;

  if(!docp->mime)
  {
    xprintf(1, "Bad response on HEAD request\n");
    return -1;
  }

  rsp = http_get_response_info(docp->mime);

  if(rsp->ver_maj == 1 && rsp->ver_min == 1)
  {
    docp->is_http11 = TRUE;
    docp->is_persistent = TRUE;
  }

  p = get_mime_param_val_str("Connection:", docp->mime);
  if(p)
  {
    if(!strcasecmp(p, "close"))
      docp->is_persistent = FALSE;
  }

  if(!docp->is_persistent)
  {
    bufio_close(docp->datasock);
    docp->datasock = NULL;
  }

  if(rsp->ret_code == 304)
  {
    retv = 0;
  }
  else if((p = get_mime_param_val_str("Last-Modified:", docp->mime)))
  {
    *nmdtm = scntime(p);

    if(modtime && (difftime(*nmdtm, modtime) > 0))
      retv = 1;
  }
  else
  {
    retv = -1;
  }

  _free(rsp->text);
  _free(rsp);
  return retv;
}

static void remind_check_entry(doc * docp, remind_entry * e)
{
  time_t t = 0L;
  int rv;

  rv = remind_check_if_newer(docp, e->mdtm, &t);

  if(!(cfg.stop || cfg.rbreak))
  {
    if(t)
      e->mdtm = t;

    if(rv == 1)
      e->status |= REMIND_MODIFIED;
    else if(rv == -1)
      e->status |= REMIND_ERROR;
  }

}

void remind_load_db(void)
{
  bufio *fd;
  char buf[PATH_MAX];

  snprintf(buf, sizeof(buf), "%s/.pavuk_remind_db", cfg.path_to_home);

  while(remind_urls)
  {
    free_deep_url(((remind_entry *) remind_urls->data)->urlp);
    free((remind_entry *)remind_urls->data);
    remind_urls = dllist_remove_entry(remind_urls, remind_urls);
  }

  fd = bufio_copen(buf, O_BINARY | O_RDWR | O_CREAT, 0644);

  if(!fd)
  {
    xperror(buf);
    return;
  }

  _flock(bufio_getfd(fd), buf, O_BINARY | O_RDWR | O_CREAT, TRUE);

  while(bufio_readln(fd, buf, sizeof(buf)) > 0)
  {
    time_t t;
    url *purl;
    char *eptr;
    remind_entry *entry;

    strip_nl(buf);
    t = strtol(buf, &eptr, 10);
    if(!t && errno == ERANGE)
    {
      xprintf(1, gettext("Bad reminder db entry - %s\n"), buf);
      continue;
    }

    while(*eptr && tl_ascii_isspace(*eptr))
      eptr++;
    purl = url_parse(eptr);

    entry = _malloc(sizeof(remind_entry));
    entry->urlp = purl;
    entry->mdtm = t;
    entry->status = 0;
    remind_urls = dllist_append(remind_urls, (dllist_t) entry);
  }

  _funlock(bufio_getfd(fd));
  bufio_close(fd);
}

void remind_save_db(void)
{
  bufio *fd;
  char buf[PATH_MAX];
  dllist *ptr;

  snprintf(buf, sizeof(buf), "%s/.pavuk_remind_db", cfg.path_to_home);

  fd = bufio_copen(buf, O_BINARY | O_RDWR | O_CREAT, 0644);

  if(!fd)
  {
    xperror(buf);
    return;
  }

  _flock(bufio_getfd(fd), buf, O_BINARY | O_RDWR | O_CREAT, TRUE);

  ftruncate(bufio_getfd(fd), 0);

  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    char *p;

    p = url_to_urlstr(((remind_entry *) ptr->data)->urlp, FALSE);
    snprintf(buf, sizeof(buf), "%ld %s\n", ((remind_entry *) ptr->data)->mdtm, p);
    _free(p);

    bufio_write(fd, buf, strlen(buf));
  }

  _funlock(bufio_getfd(fd));
  bufio_close(fd);
}

static remind_entry *remind_find_entry(url * urlp)
{
  dllist *ptr = remind_urls;
  remind_entry *rv = NULL;
  char *ustr;

  ustr = url_to_urlstr(urlp, FALSE);

  while(ptr && !rv)
  {
    remind_entry *e = (remind_entry *) ptr->data;
    char *p;

    p = url_to_urlstr(e->urlp, FALSE);

    if(!strcmp(ustr, p))
      rv = e;
    _free(p);
    ptr = ptr->next;
  }
  _free(ustr);
  return rv;
}

void remind_start_add(void)
{
  dllist *ptr = cfg.request;

  while(ptr)
  {
    url_info *ui = (url_info *) ptr->data;
    url *urlp = url_parse(ui->urlstr);

    if(!remind_find_entry(urlp))
    {
      remind_entry *e = _malloc(sizeof(remind_entry));

      e->urlp = urlp;
      e->status = 0;
      e->mdtm = 0L;

      remind_urls = dllist_append(remind_urls, (dllist_t) e);
    }
    else
    {
      free_deep_url(urlp);
      _free(urlp);
    }

    ptr = ptr->next;
  }
}

#ifdef HAVE_MT
static void _remind_do(int thnr)
#else
void remind_do(void)
#endif
{
  dllist *ptr;
  remind_entry *e;
  doc docu;
  global_connection_info con_info;

  init_global_connection_data(&con_info);

  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    e = (remind_entry *) ptr->data;

    if(e->status & REMIND_PROCESSED)
      continue;

    doc_init(&docu, e->urlp);
#ifdef HAVE_MT
    docu.threadnr = thnr;
    pthread_setspecific(cfg.currdoc_key, (void *) (&docu));
    pthread_setspecific(cfg.herrno_key, (void *) (&(docu.__herrno)));
#endif
    restore_global_connection_data(&con_info, &docu);

    remind_check_entry(&docu, e);

    save_global_connection_data(&con_info, &docu);

#ifdef HAVE_MT
    doc_finish_processing(&docu);
#endif

    if(cfg.stop || cfg.rbreak)
      break;
    else
      e->status |= REMIND_PROCESSED;
  }

  kill_global_connection_data(&con_info);
}

#ifdef HAVE_MT
static void _sigintthr(int nr)
{
  errno = EINTR;
  cfg.stop = TRUE;
  cfg.rbreak = TRUE;
}

static void _sigquitthr(int nr)
{
  pthread_exit(NULL);
}

static void _remind_do_thrd(int thrnr)
{
  bool_t init = (thrnr == 0);
  bool_t frst = TRUE;
#ifdef I_FACE
  _config_struct_priv_t privcfg;

#if defined (__OSF__) || defined (__osf__)
#define __builtin_try
#define __builtin_finally
#endif

#endif

  signal(SIGINT, _sigintthr);
  signal(SIGQUIT, _sigquitthr);

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_setspecific(cfg.currdoc_key, (void *) NULL);
  pthread_setspecific(cfg.thrnr_key, (void *) thrnr);
  DEBUG_MTTHR("starting thread(%ld) %d\n", pthread_self(), thrnr);

#ifdef I_FACE
  privcfg_make_copy(&privcfg);
  pthread_setspecific(cfg.privcfg_key, (void *) (&privcfg));
  pthread_cleanup_push((void *) privcfg_free, (void *) (&privcfg));
#endif

  for(;;)
  {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DEBUG_MTTHR("thread %d awaking\n", thrnr);
    if(!frst)
      mt_semaphore_decrement(&cfg.nrunning_sem);
    else
      frst = FALSE;
    _remind_do(thrnr);
    init = FALSE;
    gui_clear_status();
    DEBUG_MTTHR("thread %d sleeping\n", thrnr);
    gui_set_status(gettext("Sleeping ..."));
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    mt_semaphore_up(&cfg.nrunning_sem);
    if(cfg.rbreak || cfg.stop)
      break;
/*
    mt_semaphore_down(&cfg.urlstack_sem);
*/
    while(!cfg.stop && !cfg.rbreak &&
      mt_semaphore_timed_down(&cfg.urlstack_sem, 400) < 0);
    if(cfg.rbreak || cfg.stop)
      break;
  }
#ifdef I_FACE
  pthread_cleanup_pop(TRUE);
#endif
  DEBUG_MTTHR("thread %d exiting\n", thrnr);
  gui_set_status(gettext("Exiting ..."));
  pthread_exit(NULL);
}

void remind_do(void)
{
  pthread_attr_t thrdattr;
  int i;
  int num = cfg.nthr;

  cfg.allthreadsnr = 0;
  cfg.allthreads = _malloc(num * sizeof(pthread_t));

  signal(SIGQUIT, _sigquitthr);

  pthread_attr_init(&thrdattr);
  pthread_attr_setscope(&thrdattr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setstacksize(&thrdattr, MT_STACK_SIZE);
  mt_semaphore_init(&cfg.nrunning_sem);

  if(num <= 0)
    num = 1;

  for(i = 0; i < num; i++)
  {
    if(!pthread_create(&(cfg.allthreads[cfg.allthreadsnr]),
        &thrdattr, (void *) _remind_do_thrd, (void *) cfg.allthreadsnr))
    {
      cfg.allthreadsnr++;
      gui_mt_thread_start(cfg.allthreadsnr);
      mt_semaphore_decrement(&cfg.nrunning_sem);
      if(cfg.rbreak || cfg.stop)
        break;
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

/*
  mt_semaphore_down(&cfg.nrunning_sem);
*/

  while(!cfg.stop && !cfg.rbreak &&
    mt_semaphore_timed_down(&cfg.nrunning_sem, 500) < 0);

  cfg.stop = TRUE;

  for(i = 0; i < cfg.allthreadsnr; i++)
  {
/*
    pthread_cancel(cfg.allthreads[i]);
*/
    pthread_kill(cfg.allthreads[i], SIGQUIT);
    pthread_join(cfg.allthreads[i], NULL);
  }

  _free(cfg.allthreads);
  cfg.allthreadsnr = 0;
  gui_mt_thread_end(0);
}
#endif

void remind_send_result(void)
{
  FILE *fp;
  char buf[PATH_MAX];
  char *p, *dp;
  int cnt;
  dllist *ptr;

  if(cfg.remind_cmd)
  {
    for(dp = buf, p = cfg.remind_cmd; *p; p++)
    {
      if(*p == '%')
      {
        p++;
        switch (*(p))
        {
        case '%':
          *dp = *p;
          dp++;
          break;
        case 'e':
          strcpy(dp, priv_cfg.from);
          while(*dp)
            dp++;
          break;
        case 'd':
          {
            time_t t = time(NULL);
            char pom[100];

            LOCK_TIME;
            strftime(pom, sizeof(pom), "%a %H:%M %d.%m.%Y", localtime(&t));
            UNLOCK_TIME;
            strcpy(dp, pom);
            while(*dp)
              dp++;
          }
          break;
        default:
          *dp = *p;
          dp++;
          p--;
        }
      }
      else
      {
        *dp = *p;
        dp++;
      }
    }
    *dp = '\0';
  }
  else
    snprintf(buf, sizeof(buf), "mailx %s -s \"pavuk reminder result\"\n", priv_cfg.from);


  fp = popen(buf, "w");

  if(!fp)
  {
    xperror("popen");
  }

  fprintf(fp,
    gettext("This is the result of running pavuk reminder mode\n\n"));

  cnt = 0;
  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    remind_entry *e = (remind_entry *) ptr->data;

    if(e->status & REMIND_MODIFIED)
    {
      if(!cnt)
      {
        fprintf(fp, gettext("Changed URLs\n"));
        fprintf(fp, "-------------------------\n");
        cnt++;
      }
      p = url_to_urlstr(e->urlp, FALSE);
      fprintf(fp, "%s %s", p, ctime(&e->mdtm));
      _free(p);
    }
  }

  fprintf(fp, "\n\n");

  cnt = 0;
  for(ptr = remind_urls; ptr; ptr = ptr->next)
  {
    remind_entry *e = (remind_entry *) ptr->data;

    if(e->status & REMIND_ERROR)
    {
      if(!cnt)
      {
        fprintf(fp, gettext("URLs with some errors\n"));
        fprintf(fp, "-------------------------\n");
        cnt++;
      }
      p = url_to_urlstr(e->urlp, FALSE);
      fprintf(fp, "%s\n", p);
      _free(p);
    }
  }


  pclose(fp);
}
