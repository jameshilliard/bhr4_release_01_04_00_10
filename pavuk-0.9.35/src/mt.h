/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _have_mt_h_
#define _have_mt_h_

#ifdef HAVE_MT
#include <pthread.h>

#define MT_STACK_SIZE 131072

extern pthread_key_t _mt_key_main_thread;
#define MT_IS_MAIN_THREAD() (pthread_getspecific(_mt_key_main_thread))

extern pthread_mutex_t _mt_urlstack_lock;
extern pthread_mutex_t _mt_urlhash_lock;
extern pthread_mutex_t _mt_filehash_lock;
extern pthread_mutex_t _mt_cookies_lock;
extern pthread_mutex_t _mt_authinfo_lock;
extern pthread_mutex_t _mt_dns_lock;
extern pthread_mutex_t _mt_log_lock;
extern pthread_mutex_t _mt_slog_lock;
extern pthread_mutex_t _mt_tlog_lock;
extern pthread_mutex_t _mt_dcnt_lock;
extern pthread_mutex_t _mt_time_lock;
extern pthread_mutex_t _mt_ghbn_lock;
extern pthread_mutex_t _mt_getlfname_lock;
extern pthread_mutex_t _mt_proxy_lock;
extern pthread_mutex_t _mt_output_lock;
extern pthread_mutex_t _mt_dirr_lock;
extern pthread_mutex_t _mt_tcnt_lock;
extern pthread_mutex_t _mt_gtktree_lock;
extern pthread_mutex_t _mt_gtkstatus_lock;
extern pthread_mutex_t _mt_gtklog_lock;
extern pthread_mutex_t _mt_rejcnt_lock;
extern pthread_mutex_t _mt_failcnt_lock;
extern pthread_mutex_t _mt_gcfg_lock;
extern pthread_mutex_t _mt_nscache_lock;
extern pthread_mutex_t _mt_robots_lock;
extern pthread_mutex_t _mt_dumpurls_lock;
extern pthread_mutex_t _mt_dumpfd_lock;
extern pthread_mutex_t _mt_taghash_lock;
extern pthread_mutex_t _mt_jsres_lock;
extern pthread_mutex_t _mt_inet_ntoa_lock;
extern pthread_mutex_t _mt_mozjs_lock;
extern pthread_mutex_t _mt_ssl_map_lock;

#define LOCK_CFG_URLSTACK       mt_pthread_mutex_lock(&_mt_urlstack_lock, "urlstack")
#define UNLOCK_CFG_URLSTACK     mt_pthread_mutex_unlock(&_mt_urlstack_lock, "urlstack")
#define LOCK_CFG_URLHASH        mt_pthread_mutex_lock(&_mt_urlhash_lock, "urlhash")
#define UNLOCK_CFG_URLHASH      mt_pthread_mutex_unlock(&_mt_urlhash_lock, "urlhash")
#define LOCK_CFG_FILEHASH       mt_pthread_mutex_lock(&_mt_filehash_lock, "filehash")
#define UNLOCK_CFG_FILEHASH     mt_pthread_mutex_unlock(&_mt_filehash_lock, "filehash")
#define LOCK_COOKIES            mt_pthread_mutex_lock(&_mt_cookies_lock, "cookies")
#define UNLOCK_COOKIES          mt_pthread_mutex_unlock(&_mt_cookies_lock, "cookies")
#define LOCK_AUTHINFO           mt_pthread_mutex_lock(&_mt_authinfo_lock, "authinfo")
#define UNLOCK_AUTHINFO         mt_pthread_mutex_unlock(&_mt_authinfo_lock, "authinfo")
#define LOCK_GETLFNAME          mt_pthread_mutex_lock(&_mt_getlfname_lock, "getlfname")
#define UNLOCK_GETLFNAME        mt_pthread_mutex_unlock(&_mt_getlfname_lock, "getlfname")
#define LOCK_DNS                mt_pthread_mutex_lock(&_mt_dns_lock, "dns")
#define UNLOCK_DNS              mt_pthread_mutex_unlock(&_mt_dns_lock, "dns")
#define LOCK_SSL_MAP            mt_pthread_mutex_lock(&_mt_ssl_map_lock, "ssl_map")
#define UNLOCK_SSL_MAP          mt_pthread_mutex_unlock(&_mt_ssl_map_lock, "ssl_map")
#define LOCK_LOG                mt_pthread_mutex_lock(&_mt_log_lock, "log")
#define UNLOCK_LOG              mt_pthread_mutex_unlock(&_mt_log_lock, "log")
#define LOCK_SLOG               mt_pthread_mutex_lock(&_mt_slog_lock, "slog")
#define UNLOCK_SLOG             mt_pthread_mutex_unlock(&_mt_slog_lock, "slog")
#define LOCK_TLOG               mt_pthread_mutex_lock(&_mt_tlog_lock, "tlog")
#define UNLOCK_TLOG             mt_pthread_mutex_unlock(&_mt_tlog_lock, "tlog")
#define LOCK_DCNT               mt_pthread_mutex_lock(&_mt_dcnt_lock, "dcnt")
#define UNLOCK_DCNT             mt_pthread_mutex_unlock(&_mt_dcnt_lock, "dcnt")
#define LOCK_TIME               mt_pthread_mutex_lock(&_mt_time_lock, "time")
#define UNLOCK_TIME             mt_pthread_mutex_unlock(&_mt_time_lock, "time")
#define LOCK_GHBN               mt_pthread_mutex_lock(&_mt_ghbn_lock, "ghbn")
#define UNLOCK_GHBN             mt_pthread_mutex_unlock(&_mt_ghbn_lock, "ghbn")
#define LOCK_OUTPUT             mt_pthread_mutex_lock(&_mt_output_lock, "output")
#define UNLOCK_OUTPUT           mt_pthread_mutex_unlock(&_mt_output_lock, "output")
#define LOCK_PROXY              mt_pthread_mutex_lock(&_mt_proxy_lock, "proxy")
#define UNLOCK_PROXY            mt_pthread_mutex_unlock(&_mt_proxy_lock, "proxy")
#define LOCK_DIRR               mt_pthread_mutex_lock(&_mt_dirr_lock, "dirr")
#define UNLOCK_DIRR             mt_pthread_mutex_unlock(&_mt_dirr_lock, "dirr")
#define LOCK_TCNT               mt_pthread_mutex_lock(&_mt_tcnt_lock, "tcnt")
#define UNLOCK_TCNT             mt_pthread_mutex_unlock(&_mt_tcnt_lock, "tcnt")
#define LOCK_GTKTREE            mt_pthread_mutex_lock(&_mt_gtktree_lock, "gtktree")
#define UNLOCK_GTKTREE          mt_pthread_mutex_unlock(&_mt_gtktree_lock, "gtktree")
#define LOCK_GTKSTATUS          mt_pthread_mutex_lock(&_mt_gtkstatus_lock, "gtkstatus")
#define UNLOCK_GTKSTATUS        mt_pthread_mutex_unlock(&_mt_gtkstatus_lock, "gtkstatus")
#define LOCK_GTKLOG             mt_pthread_mutex_lock(&_mt_gtklog_lock, "gtklog")
#define UNLOCK_GTKLOG           mt_pthread_mutex_unlock(&_mt_gtklog_lock, "gtklog")
#define LOCK_REJCNT             mt_pthread_mutex_lock(&_mt_rejcnt_lock, "rejcnt")
#define UNLOCK_REJCNT           mt_pthread_mutex_unlock(&_mt_rejcnt_lock, "rejcnt")
#define LOCK_FAILCNT            mt_pthread_mutex_lock(&_mt_failcnt_lock, "failcnt")
#define UNLOCK_FAILCNT          mt_pthread_mutex_unlock(&_mt_failcnt_lock, "failcnt")
#define LOCK_URL(lurl)          mt_pthread_mutex_lock(&(lurl)->lock, "url")
#define UNLOCK_URL(lurl)        mt_pthread_mutex_unlock(&(lurl)->lock, "url")
#define LOCK_GCFG               mt_pthread_mutex_lock(&_mt_gcfg_lock, "gcfg")
#define UNLOCK_GCFG             mt_pthread_mutex_unlock(&_mt_gcfg_lock, "gcfg")
#define LOCK_NSCACHE            mt_pthread_mutex_lock(&_mt_nscache_lock, "nscache")
#define UNLOCK_NSCACHE          mt_pthread_mutex_unlock(&_mt_nscache_lock, "nscache")
#define LOCK_ROBOTS             mt_pthread_mutex_lock(&_mt_robots_lock, "robots")
#define UNLOCK_ROBOTS           mt_pthread_mutex_unlock(&_mt_robots_lock, "robots")
#define LOCK_DUMPURLS           mt_pthread_mutex_lock(&_mt_dumpurls_lock, "dumpurls")
#define UNLOCK_DUMPURLS         mt_pthread_mutex_unlock(&_mt_dumpurls_lock, "dumpurls")
#define LOCK_DUMPFD             mt_pthread_mutex_lock(&_mt_dumpurls_lock, "dumpfd")
#define UNLOCK_DUMPFD           mt_pthread_mutex_unlock(&_mt_dumpfd_lock, "dumpfd")
#define LOCK_TAG_HASH           mt_pthread_mutex_lock(&_mt_taghash_lock, "tag_hash")
#define UNLOCK_TAG_HASH         mt_pthread_mutex_unlock(&_mt_taghash_lock, "tag_hash")
#define LOCK_INETNTOA           mt_pthread_mutex_lock(&_mt_inet_ntoa_lock, "inet_ntoa")
#define UNLOCK_INETNTOA         mt_pthread_mutex_unlock(&_mt_inet_ntoa_lock, "inet_ntoa")
#define LOCK_MOZJS              mt_pthread_mutex_lock(&_mt_mozjs_lock, "mozjs")
#define UNLOCK_MOZJS            mt_pthread_mutex_unlock(&_mt_mozjs_lock, "mozjs")

#define _h_errno_ (*((int *)pthread_getspecific(cfg.herrno_key)))

extern int mt_pthread_mutex_lock(pthread_mutex_t *, char *);
extern int mt_pthread_mutex_unlock(pthread_mutex_t *, char *);

typedef struct _mt_semaphore
{
  long v;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} mt_semaphore;

extern int mt_semaphore_init(mt_semaphore *);
extern int mt_semaphore_destroy(mt_semaphore *);
extern long mt_semaphore_up(mt_semaphore *);
extern long mt_semaphore_down(mt_semaphore *);
extern long mt_semaphore_timed_down(mt_semaphore *, int);
extern long mt_semaphore_timed_wait(mt_semaphore *, int);
extern long mt_semaphore_decrement(mt_semaphore *);

#else

#define MT_IS_MAIN_THREAD() TRUE

#define LOCK_CFG_URLSTACK
#define UNLOCK_CFG_URLSTACK
#define LOCK_CFG_URLHASH
#define UNLOCK_CFG_URLHASH
#define LOCK_CFG_FILEHASH
#define UNLOCK_CFG_FILEHASH
#define LOCK_COOKIES
#define UNLOCK_COOKIES
#define LOCK_AUTHINFO
#define UNLOCK_AUTHINFO
#define LOCK_DNS
#define UNLOCK_DNS
#define LOCK_SSL_MAP
#define UNLOCK_SSL_MAP
#define LOCK_LOG
#define UNLOCK_LOG
#define UNLOCK_SLOG
#define LOCK_SLOG
#define LOCK_TLOG
#define UNLOCK_TLOG
#define LOCK_GETLFNAME
#define UNLOCK_GETLFNAME
#define LOCK_DCNT
#define UNLOCK_DCNT
#define LOCK_TIME
#define UNLOCK_TIME
#define LOCK_OUTPUT
#define UNLOCK_OUTPUT
#define LOCK_GHBN
#define UNLOCK_GHBN
#define LOCK_PROXY
#define UNLOCK_PROXY
#define LOCK_DIRR
#define UNLOCK_DIRR
#define LOCK_TCNT
#define UNLOCK_TCNT
#define LOCK_GTKTREE
#define UNLOCK_GTKTREE
#define LOCK_GTKSTATUS
#define UNLOCK_GTKSTATUS
#define LOCK_GTKLOG
#define UNLOCK_GTKLOG
#define LOCK_REJCNT
#define UNLOCK_REJCNT
#define LOCK_FAILCNT
#define UNLOCK_FAILCNT
#define LOCK_URL(lurl)
#define UNLOCK_URL(lurl)
#define LOCK_GCFG
#define UNLOCK_GCFG
#define LOCK_NSCACHE
#define UNLOCK_NSCACHE
#define LOCK_ROBOTS
#define UNLOCK_ROBOTS
#define LOCK_DUMPURLS
#define UNLOCK_DUMPURLS
#define LOCK_DUMPFD
#define UNLOCK_DUMPFD
#define LOCK_TAG_HASH
#define UNLOCK_TAG_HASH
#define LOCK_INETNTOA
#define UNLOCK_INETNTOA
#define LOCK_MOZJS
#define UNLOCK_MOZJS

#define _h_errno_ h_errno

#endif

extern void mt_init(void);

#endif
