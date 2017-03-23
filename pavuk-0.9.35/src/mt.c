/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef HAVE_MT
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "gui.h"
#include "mt.h"

pthread_key_t _mt_key_main_thread;

pthread_mutex_t _mt_urlstack_lock;
pthread_mutex_t _mt_urlhash_lock;
pthread_mutex_t _mt_filehash_lock;
pthread_mutex_t _mt_cookies_lock;
pthread_mutex_t _mt_authinfo_lock;
pthread_mutex_t _mt_getlfname_lock;
pthread_mutex_t _mt_dns_lock;
pthread_mutex_t _mt_log_lock;
pthread_mutex_t _mt_slog_lock;
pthread_mutex_t _mt_tlog_lock;
pthread_mutex_t _mt_dbase_lock;
pthread_mutex_t _mt_dcnt_lock;
pthread_mutex_t _mt_time_lock;
pthread_mutex_t _mt_ghbn_lock;
pthread_mutex_t _mt_output_lock;
pthread_mutex_t _mt_proxy_lock;
pthread_mutex_t _mt_dirr_lock;
pthread_mutex_t _mt_tcnt_lock;
pthread_mutex_t _mt_gtktree_lock;
pthread_mutex_t _mt_gtkstatus_lock;
pthread_mutex_t _mt_gtklog_lock;
pthread_mutex_t _mt_rejcnt_lock;
pthread_mutex_t _mt_failcnt_lock;
pthread_mutex_t _mt_gcfg_lock;
pthread_mutex_t _mt_nscache_lock;
pthread_mutex_t _mt_robots_lock;
pthread_mutex_t _mt_dumpurls_lock;
pthread_mutex_t _mt_taghash_lock;
pthread_mutex_t _mt_dumpfd_lock;
pthread_mutex_t _mt_inet_ntoa_lock;
pthread_mutex_t _mt_mozjs_lock;
pthread_mutex_t _mt_ssl_map_lock;

void mt_init(void)
{
  static int __h_errno;

  pthread_mutex_init(&_mt_urlstack_lock, NULL);
  pthread_mutex_init(&_mt_urlhash_lock, NULL);
  pthread_mutex_init(&_mt_filehash_lock, NULL);
  pthread_mutex_init(&_mt_cookies_lock, NULL);
  pthread_mutex_init(&_mt_authinfo_lock, NULL);
  pthread_mutex_init(&_mt_dns_lock, NULL);
  pthread_mutex_init(&_mt_log_lock, NULL);
  pthread_mutex_init(&_mt_slog_lock, NULL);
  pthread_mutex_init(&_mt_tlog_lock, NULL);
  pthread_mutex_init(&_mt_dcnt_lock, NULL);
  pthread_mutex_init(&_mt_time_lock, NULL);
  pthread_mutex_init(&_mt_ghbn_lock, NULL);
  pthread_mutex_init(&_mt_getlfname_lock, NULL);
  pthread_mutex_init(&_mt_output_lock, NULL);
  pthread_mutex_init(&_mt_proxy_lock, NULL);
  pthread_mutex_init(&_mt_dirr_lock, NULL);
  pthread_mutex_init(&_mt_tcnt_lock, NULL);
  pthread_mutex_init(&_mt_gtktree_lock, NULL);
  pthread_mutex_init(&_mt_gtkstatus_lock, NULL);
  pthread_mutex_init(&_mt_gtklog_lock, NULL);
  pthread_mutex_init(&_mt_rejcnt_lock, NULL);
  pthread_mutex_init(&_mt_failcnt_lock, NULL);
  pthread_mutex_init(&_mt_gcfg_lock, NULL);
  pthread_mutex_init(&_mt_nscache_lock, NULL);
  pthread_mutex_init(&_mt_robots_lock, NULL);
  pthread_mutex_init(&_mt_dumpurls_lock, NULL);
  pthread_mutex_init(&_mt_taghash_lock, NULL);
  pthread_mutex_init(&_mt_dumpfd_lock, NULL);
  pthread_mutex_init(&_mt_inet_ntoa_lock, NULL);
  pthread_mutex_init(&_mt_mozjs_lock, NULL);
  pthread_mutex_init(&_mt_ssl_map_lock, NULL);

  pthread_key_create(&cfg.currdoc_key, NULL);
  pthread_key_create(&cfg.herrno_key, NULL);
  pthread_key_create(&cfg.thrnr_key, NULL);
  pthread_key_create(&cfg.privcfg_key, NULL);
  pthread_key_create(&_mt_key_main_thread, NULL);

  mt_semaphore_init(&cfg.nrunning_sem);
  mt_semaphore_init(&cfg.urlstack_sem);

  pthread_setspecific(cfg.herrno_key, &__h_errno);
  pthread_setspecific(cfg.currdoc_key, NULL);
  pthread_setspecific(cfg.thrnr_key, (void *) -1);
  pthread_setspecific(cfg.privcfg_key, &cfg);
  pthread_setspecific(_mt_key_main_thread, (void *) TRUE);

  cfg.cfg_changed = 0L;

#ifdef GTK_FACE
  g_thread_init(NULL);
#endif
}

int mt_pthread_mutex_lock(pthread_mutex_t * mutex, char *id)
{
  int rv;

  DEBUG_MTLOCK("Try lock(%ld) - %s\n", pthread_self(), id);
  rv = pthread_mutex_lock(mutex);
  DEBUG_MTLOCK("Locking(%ld) - %s\n", pthread_self(), id);

  return rv;
}

int mt_pthread_mutex_unlock(pthread_mutex_t * mutex, char *id)
{
  DEBUG_MTLOCK("Unlocking(%ld) - %s\n", pthread_self(), id);
  return pthread_mutex_unlock(mutex);
}

/********************************************************/
/* semaphore code based on Tom Wagner and Don Towsley's */
/* document "Getting Started With POSIX Threads"        */
/********************************************************/
int mt_semaphore_init(mt_semaphore * sem)
{
  sem->v = 1;
  if(pthread_mutex_init(&(sem->mutex), NULL))
  {
    xperror("mt_semaphore_init - mutex_init");
    return -1;
  }
  if(pthread_cond_init(&(sem->cond), NULL))
  {
    xperror("mt_semaphore_init - cond_init");
    return -1;
  }
  return 0;
}

int mt_semaphore_destroy(mt_semaphore * sem)
{
  if(pthread_mutex_destroy(&(sem->mutex)))
  {
    xperror("mt_semaphore_destroy - mutex_destroy");
    return -1;
  }
  if(pthread_cond_destroy(&(sem->cond)))
  {
    xperror("mt_semaphore_destroy - cond_destroy");
    return -1;
  }
  return 0;
}

long mt_semaphore_up(mt_semaphore * sem)
{
  long rv;

  pthread_mutex_lock(&(sem->mutex));

  sem->v++;
  rv = sem->v;

  pthread_mutex_unlock(&(sem->mutex));
  pthread_cond_signal(&(sem->cond));
  return rv;
}

long mt_semaphore_down(mt_semaphore * sem)
{
  long rv;

  pthread_mutex_lock(&(sem->mutex));

  while(sem->v <= 0)
  {
    pthread_cond_wait(&(sem->cond), &(sem->mutex));
  }

  sem->v--;
  rv = sem->v;

  pthread_mutex_unlock(&(sem->mutex));

  return rv;
}

static void set_ts(struct timespec *ts, int msec)
{
#ifdef HAVE_PT_EXPIRATION_NP
  {
    struct timespec t;
    t.tv_sec = msec / 1000;
    t.tv_nsec = (msec % 1000) * 1000000;
    pthread_get_expiration_np(&t, ts);
  }
#else /* HAVE_PT_EXPIRATION_NP */

#ifdef HAVE_GETTIMEOFDAY
  {
    struct timeval t;
    gettimeofday(&t, NULL);
    ts->tv_sec = t.tv_sec;
    ts->tv_nsec = t.tv_usec * 1000;
  }
#else /* HAVE_GETTIMEOFDAY */
  ts->tv_sec = time(NULL);
  ts->tv_nsec = 0;
#endif /* HAVE_GETTIMEOFDAY */

  ts->tv_sec += msec / 1000;
  ts->tv_nsec += (msec % 1000) * 1000000;
  ts->tv_sec += ts->tv_nsec / 1000000000;
  ts->tv_nsec %= 1000000000;
#endif /* HAVE_PT_EXPIRATION_NP */
}

long mt_semaphore_timed_down(mt_semaphore * sem, int msec)
{
  long rv;
  struct timespec ts;
  int errnum = 0;

  pthread_mutex_lock(&(sem->mutex));

  set_ts(&ts, msec);
  while(sem->v <= 0)
  {
    errnum = pthread_cond_timedwait(&(sem->cond), &(sem->mutex), &ts);

    if(errnum)
      break;
  }

  if(!errnum)
  {
    sem->v--;
    rv = sem->v;
  }
  else
    rv = -1;

  pthread_mutex_unlock(&(sem->mutex));

  return rv;
}

long mt_semaphore_timed_wait(mt_semaphore * sem, int msec)
{
  long rv;
  struct timespec ts;
  int errnum = 0;

  pthread_mutex_lock(&(sem->mutex));

  set_ts(&ts, msec);
  while(sem->v <= 0)
  {
    errnum = pthread_cond_timedwait(&(sem->cond), &(sem->mutex), &ts);

    if(errnum)
      break;
  }

  if(!errnum)
  {
    rv = sem->v;
  }
  else
    rv = -1;

  pthread_mutex_unlock(&(sem->mutex));

  return rv;
}

long mt_semaphore_decrement(mt_semaphore * sem)
{
  long rv;

  pthread_mutex_lock(&(sem->mutex));

  sem->v--;
  rv = sem->v;

  pthread_mutex_unlock(&(sem->mutex));

  return rv;
}
#endif
