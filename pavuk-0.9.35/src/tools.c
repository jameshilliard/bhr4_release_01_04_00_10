/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifdef HAVE_FLOCK
#include <sys/file.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <signal.h>

#ifdef HAVE_TERMIOS
#include <termios.h>
#endif

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "fnmatch.h"
#endif

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

#ifdef GTK_FACE
#include <glib.h>
#endif

#include "gui_api.h"
#include "url.h"
#include "doc.h"
#include "ftp.h"
#include "dinfo.h"
#include "log.h"

void strip_nl(char *str)
{
  char *p;

  p = strchr(str, '\n');
  if(p)
    *p = '\0';
  p = strchr(str, '\r');
  if(p)
    *p = '\0';
}

void omit_chars(char *str, char *chars)
{
  int src, dst;

  for(src = 0, dst = 0; str[src]; src++)
  {
    if(strchr(chars, str[src]))
      continue;
    str[dst] = str[src];
    dst++;
  }
  str[dst] = '\0';
}

#ifndef HAVE_SETENV
int tl_setenv(const char *var, const char *val, int ovr)
{
  char *pom = _malloc(strlen(var) + strlen(val) + 2);

  sprintf(pom, "%s=%s", var, val);
  return putenv(pom);
}
#endif

#ifndef HAVE_INET6
static char *xstrherror(int enr)
{
  char *p;

  switch (enr)
  {
#ifdef NETDB_INTERNAL
  case NETDB_INTERNAL:
    p = strerror(errno);
    break;
#endif
#ifdef NETDB_SUCCESS
  case NETDB_SUCCESS:
    p = gettext("no error");
    break;
#endif
#ifdef HOST_NOT_FOUND
  case HOST_NOT_FOUND:
    p = gettext("host not found");
    break;
#endif
#ifdef TRY_AGAIN
  case TRY_AGAIN:
    p = gettext("temporary error (try again later)");
    break;
#endif
#ifdef NO_RECOVERY
  case NO_RECOVERY:
    p = gettext("non recoverable error");
    break;
#endif
#ifdef NO_ADDRESS
  case NO_ADDRESS:
    p = gettext("name is valid, but doesn't have an IP address");
    break;
#endif
  default:
    p = gettext("unknown hostname translation error");
  }
  return p;
}
#endif

void xherror(const char *str)
{
#ifdef HAVE_INET6
  xprintf(1, "%s: %s\n", str, gai_strerror(_h_errno_));
#else
  xprintf(1, "%s: %s\n", str, xstrherror(_h_errno_));
#endif
}

void xperror(const char *str)
{
  xprintf(1, "%s: %s\n", str, strerror(errno));
}

#ifdef HAVE_MT
static void st_xvaprintf(int log, const char *strs, va_list * args)
#else
void xvaprintf(int log, const char *strs, va_list * args)
#endif
{
#ifdef GTK_FACE
  char *buf = g_strdup_vprintf(strs, *args);
#else
  char buf[4096];
#ifdef HAVE_VSNPRINTF
  int l = vsnprintf(buf, sizeof(buf), strs, *args);
  if(TL_BETWEEN(l, 0, sizeof(buf) - 1))
    buf[l] = '\0';
#else
  vsprintf(buf, strs, *args);
#endif
#endif

  if(log && cfg.logfile)
    log_str(buf);

#ifdef I_FACE
  if(cfg.xi_face && cfg.done && !cfg.quiet)
  {
    gui_xprint(buf);
  }
  else
#endif
  {
    if(!cfg.quiet && !cfg.bgmode)
    {
      bool_t wout = TRUE;
#ifdef HAVE_TERMIOS
      if(cfg.tccheck)
      {
        static int _istty = -1;
        static pid_t _pgrp;

        if(_istty == -1)
        {
          _istty = isatty(1);
#ifdef GETPGRP_NEED_PID
          _pgrp = getpgrp(getpid());
#else
          _pgrp = getpgrp();
#endif
        }

        if(_istty && tcgetpgrp(1) != _pgrp)
          wout = FALSE;
      }
#endif
      if(wout)
      {
        fwrite(buf, sizeof(char), strlen(buf), stdout);
        fflush(stdout);
      }
    }
  }
#ifdef GTK_FACE
  g_free(buf);
#endif
}

#ifdef HAVE_MT
void xvaprintf(int log, const char *strs, va_list * args)
{
  doc *cdoc;

  cdoc = (doc *) pthread_getspecific(cfg.currdoc_key);

  if(!cdoc || (cfg.nthr == 1) || cfg.immessages)
    st_xvaprintf(log, strs, args);
  else if(cdoc)
  {
    doc_msg *dm = _malloc(sizeof(doc_msg));
    char buf[4096];

    buf[0] = '\0';
    vsprintf(buf, strs, *args);

    dm->log = log;
    dm->msg = tl_strdup(buf);

    cdoc->msgbuf = dllist_append(cdoc->msgbuf, dm);
  }
  else
    st_xvaprintf(log, strs, args);
}
#endif

void xprintf(int log, const char *strs, ...)
{
  va_list args;

  va_start(args, strs);
  xvaprintf(log, strs, &args);
  va_end(args);
}

void xdebug(int level, const char *strs, ...)
{
#ifdef HAVE_MT
  static pthread_mutex_t unique_lock = PTHREAD_MUTEX_INITIALIZER;
  static volatile int unique = 0;
#endif
#ifdef DEBUG
  if(cfg.debug && (level & cfg.debug_level))
  {
    va_list args;
    va_start(args, strs);
#ifdef HAVE_MT
    pthread_mutex_lock(&unique_lock);
    printf("%4d ", unique++);
    st_xvaprintf(1, strs, &args);
    pthread_mutex_unlock(&unique_lock);
#else
    xvaprintf(1, strs, &args);
#endif
    va_end(args);
  }
#endif /* DEBUG */
}

void xvadebug(int level, const char *strs, va_list * args)
{
#ifdef DEBUG
  if(cfg.debug && (level & cfg.debug_level))
  {
    xvaprintf(1, strs, args);
  }
#endif
}

unsigned int hash_func(const char *str, int num)
{
  const char *p = str;
  unsigned int rv = 0;

  while(*p)
  {
    rv = (rv + (unsigned char) *p) % num;
    p++;
  }
  rv = rv % num;
  return rv;
}

static void report_unsup_locking(void)
{
  static int visited = FALSE;
  if(!visited)
  {
    visited = TRUE;
    xprintf(1,
      "------------------------------------------------------------------------------\n");
    xprintf(1,
      gettext
      ("Warning: locking not supported ... don't run multiple processes or threads!\n"));
    xprintf(1,
      "------------------------------------------------------------------------------\n");
  }
}

int tl_flock(int *fd, const char *filename, int opt, int b_lock)
{
  int i = 0;

/* currently it seemds to me that BeOS  */
/* doesn't support file locking :-(     */
/* so just without real locking, report */
/* successfully acquired lock           */
#ifdef __BEOS__
  report_unsup_locking();
  return 0;
#endif

  DEBUG_LOCKS("Locking file - %s\n", filename);
  if(b_lock)
  {
    bool_t ready = FALSE;
    while(!ready)
    {
#ifdef HAVE_FLOCK
      if(flock(*fd, LOCK_EX | LOCK_NB))
      {
        if(errno == EWOULDBLOCK)
        {
          xprintf(1, gettext("waiting to release lock on FD : %d\n"), *fd);
          i = flock(*fd, LOCK_EX);
        }
        else if(errno == ENOSYS ||
#ifdef ENOTSUP
          errno == ENOTSUP ||
#endif
          errno == EOPNOTSUPP)
        {
          report_unsup_locking();
          break;
        }
        else
          perror(filename);
      }
      else
      {
        i = 0;
      }
#else
#ifdef HAVE_FCNTL_LOCK
      struct flock fl;

      memset(&fl, '\0', sizeof(fl));
      fl.l_type = F_WRLCK;
      if(fcntl(*fd, F_SETLK, &fl))
      {
        if(errno == EWOULDBLOCK)
        {
          xprintf(1, gettext("waiting to release lock on FD : %d\n"), *fd);
          memset(&fl, '\0', sizeof(fl));
          fl.l_type = F_WRLCK;
          i = fcntl(*fd, F_SETLKW, &fl);
        }
        else if(errno == ENOSYS ||
#ifdef ENOTSUP
          errno == ENOTSUP ||
#endif
          errno == EOPNOTSUPP)
        {
          report_unsup_locking();
          break;
        }
        else
          perror(filename);
      }
      else
      {
        i = 0;
      }
#endif
#endif
      if(access(filename, F_OK))
      {
        DEBUG_LOCKS("Lock file was removed - creating new one.\n");
        close(*fd);
        if(!makealldirs(filename))
        {
          *fd = open(filename, opt, 0644);
          if(*fd < 0)
          {
            i = -1;
            ready = TRUE;
          }
        }
        else
        {
          i = -1;
          ready = TRUE;
        }
      }
      else
        ready = TRUE;
    }

  }
  else
  {
#ifdef HAVE_FLOCK
    i = flock(*fd, LOCK_EX | LOCK_NB);
#else
#ifdef HAVE_FCNTL_LOCK
    struct flock fl;

    memset(&fl, '\0', sizeof(fl));
    fl.l_type = F_WRLCK;
    i = fcntl(*fd, b_lock ? F_SETLKW : F_SETLK, &fl);
#endif
#endif
    if(i < 0 && (errno == ENOSYS ||
#ifdef ENOTSUP
        errno == ENOTSUP ||
#endif
        errno == EOPNOTSUPP))
    {
      report_unsup_locking();
      i = 0;
    }
    else if(i < 0)
      perror(filename);
  }

  return i;
}

#ifdef HAVE_FCNTL_LOCK
int tl_funlock(int fd)
{
  struct flock fl;
  int i;

  memset(&fl, '\0', sizeof(fl));
  fl.l_type = F_UNLCK;
  i = fcntl(fd, F_SETLK, &fl);

  return i;
}
#endif

int tl_mkstemp(char *pattern)
{
#ifdef HAVE_MKSTEMP
  return mkstemp(pattern);
#else
  tmpnam(pattern);

  return open(pattern, O_CREAT | O_RDWR | O_TRUNC | O_EXCL | O_BINARY, 0600);
#endif
}

long int _atoi(char *str)
{
  char *__eptr__;
  long int rv;

  if(!*str)
  {
    errno = ERANGE;
    rv = 0;
  }
  else
  {
    errno = 0;
    rv = strtol(str, (char **) &__eptr__, 10);
    if(*__eptr__ != '\0')
      errno = ERANGE;
    else
      errno = 0;
  }

  return rv;
}

double _atof(char *str)
{
  char *__eptr__;
  double rv;

  if(!*str)
  {
    errno = ERANGE;
    rv = 0.0;
  }
  else
  {
    errno = 0;
    rv = strtod(str, (char **) &__eptr__);
    if(*__eptr__ != '\0')
      errno = ERANGE;
    else
      errno = 0;
  }

  return rv;
}


char *_strtrchr(char *str, int cfrom, int cto)
{
  char *p = str;

  while((p = strchr(p, cfrom)))
    *p = cto;

  return str;
}

void *_malloc(int sz)
{
  void *ret = malloc(sz);
  if(!ret)
  {
    perror("malloc");
  }
  return ret;
}

void *_realloc(void *from, int sz)
{
  void *ret = from ? realloc(from, sz) : malloc(sz);
  if(!ret)
  {
    perror("realloc");
  }
  return ret;
}

char *get_1qstr(const char *str)
{
  static const char *p = 0;
  static char pom[4096];
  const char *pom1 = NULL, *pom2 = NULL;
  int found;
  bool_t wasesc = FALSE;

  if(str)
    p = str;

  for(; p && *p && (!pom1 || !pom2); p++)
  {
    found = FALSE;

    if(!wasesc && *p == '\"')
      found = TRUE;

    if(!wasesc && *p == '\\')
      wasesc = TRUE;
    else
      wasesc = FALSE;

    if(!pom1 && found)
      pom1 = p + 1;
    else if(!pom2 && found)
      pom2 = p;
  }
  if(pom1 && pom2)
  {
    char *p2;
    p2 = pom;
    while(pom1 < pom2)
    {
      if(*pom1 == '\\')
      {
        pom1++;
        *p2 = *pom1;
      }
      else
      {
        *p2 = *pom1;
      }
      p2++;
      pom1++;
    }
    *p2 = '\0';
    return pom;
  }
  else
  {
    p = NULL;
    return NULL;
  }
}

char *escape_str(char *str, char *unsafe)
{
  char sbuf[4096];
  char *p, *r;

  for(p = str, r = sbuf; *p; p++)
  {
    if(strchr(unsafe, *p))
    {
      *r = '\\';
      r++;
      *r = *p;
      r++;
    }
    else
    {
      *r = *p;
      r++;
    }
  }
  *r = '\0';
  return tl_strdup(sbuf);
}

/**************************************************************/
/* implementation of standard function strtok                 */
/* split string at occurence of <chr> by setting \0-character */
/* store next element (one behind \0) pointer for next call   */
/* calling with str equal to zero uses stored pointer         */
/**************************************************************/
char *strtokc_r(char *str, int chr, char **save)
{
  char *ret;

  if(str)
    *save = str;

  if((ret = *save))
  {
    char *pom;

    if((pom = strchr(*save, chr)))
    {
      *pom = '\0';
      *save = pom + 1;
    }
    else
      *save = NULL;
  }

  return ret;
}

/**************************************/
/* najdi poziciu n-teho vyskytu znaku */
/* FIXME: Translate me!               */
/**************************************/
char *strfindnchr(char *str, int chr, int n)
{
  int cnt;
  char *p;

  for(p = str, cnt = 0; *p && cnt < n; p++)
  {
    if(*p == chr)
      cnt++;
  }
  if(cnt != n)
    return NULL;
  else
    return p;
}

/********************************************/
/* search in list for string occurrence     */
/********************************************/
bool_t is_in_list(char *str, char **list)
{
  char **p = list;

  while(*p)
  {
    if(!strcasecmp(*p, str))
      return TRUE;
    p++;
  }
  return FALSE;
}

bool_t is_in_dllist(char *str, dllist * list)
{
  dllist *p;

  for(p = list; p; p = p->next)
  {
    if(!strcasecmp((char *) p->data, str))
      return TRUE;
  }
  return FALSE;
}

/****************************************************/
/* match string again list of wildcard patterns     */
/****************************************************/
bool_t is_in_pattern_list(char *str, char **list)
{
  char **p = list;

  while(*p)
  {
    if(!fnmatch(*p, str, 0))
      return TRUE;
    p++;
  }
  return FALSE;
}

bool_t is_in_pattern_dllist(char *str, dllist * list)
{
  dllist *p;

  for(p = list; p; p = p->next)
  {
    if(!fnmatch((char *) p->data, str, 0))
      return TRUE;
  }
  return FALSE;
}

/*******************************************************/
/* split string to NULL terminated array of strings    */
/* separated with some of characters in sep            */
/*******************************************************/
char **tl_str_split(const char *liststr, const char *sep)
{
  const char *p;
  int i = 0;
  char **ret_val = NULL;
  int ilen;

  if(!liststr || !*liststr)
    return NULL;

  ret_val = _malloc(sizeof(char **));
  ret_val[0] = NULL;

  p = liststr;

  while(*p)
  {
    ilen = strcspn(p, sep);
    ret_val = _realloc(ret_val, sizeof(char **) * (i + 2));
    ret_val[i] = tl_strndup(p, ilen);
    ret_val[i + 1] = NULL;
    p += ilen;
    if(*p)
      p++;
    i++;
  }

  return ret_val;
}

dllist *tl_numlist_split(char *str, char *sep)
{
  dllist *rv = NULL;
  char **v;
  int i;

  v = tl_str_split(str, sep);

  for(i = 0; v && v[i]; i++)
  {
    long n = _atoi(v[i]);
    if(errno == ERANGE)
    {
      while(rv)
        rv = dllist_remove_entry(rv, rv);
      break;
    }

    rv = dllist_append(rv, (dllist_t) n);
  }

  tl_strv_free(v);

  return rv;
}

/* free null terminated string vector */
void tl_strv_free(char **v)
{
  int i;

  for(i = 0; v && v[i]; i++)
    _free(v[i]);

  _free(v);
}

/* count length of null terminated string vector */
int tl_strv_length(char **v)
{
  int i;

  for(i = 0; v && v[i]; i++);

  return i;
}

int tl_strv_find(char **v, char *s)
{
  int i;
  for(i = 0; v && v[i]; i++)
  {
    if(!strcmp(v[i], s))
      return i;
  }
  return -1;
}

static int sort_strcmp(const char **s1, const char **s2)
{
  return strcmp(*s1, *s2);
}

void tl_strv_sort(char **v)
{
  if(v)
  {
    qsort((void *) v, tl_strv_length(v), sizeof(char *),
      (int (*)(const void *, const void *)) sort_strcmp);
  }
}

/*************************************************/
/* change all characters in string to upper case */
/*************************************************/
char *upperstr(char *s)
{
  char *p;

  for(p = s; *p != '\0'; p++)
    *p = tl_ascii_toupper(*p);

  return s;
}

/*************************************************/
/* change all characters in string to lower case */
/*************************************************/
char *lowerstr(char *s)
{
  char *p;

  for(p = s; *p != '\0'; p++)
    *p = tl_ascii_tolower(*p);

  return s;
}

/**********************/
/* duplicate string   */
/**********************/
char *tl_strdup(const char *s)
{
  char *p;

  if(s)
  {
    p = (char *) _malloc(strlen(s) + 1);
    strcpy(p, s);
  }
  else
    p = NULL;

  return p;
}

/****************************************/
/* duplicate n characters from string   */
/****************************************/
char *tl_strndup(const char *s, int n)
{
  char *p;

  if(!s)
    return NULL;

  p = (char *) _malloc(n + 1);
  if(n)
    strncpy(p, s, n);
  *(p + n) = '\0';
  return p;
}

/***************************************************/
/* create all directories in path specification    */
/***************************************************/
int makealldirs(const char *path)
{
  char pom[PATH_MAX];
  const char *p;

  pom[0] = '\0';

  if(path)
  {
    p = path;
#ifdef __CYGWIN__
/* we can't create new drive on WIN32, so the drive */
/* specification part of path must be skipped     */
#ifdef HAVE_NEWSTYLE_CYGWIN
    if(p[0] == '/' && p[1] == '/')
    {
      char *sp;

      if((sp = strchr(p + 2, '/')) && (sp = strchr(sp + 1, '/')))
      {
        strncpy(pom, p, sp - p);
        pom[sp - p] = '\0';
        p = sp;
      }
    }
    else if(!strncmp(p, "/cygdrive/", 10) &&
      (strlen(p) > 10) && tl_ascii_isalpha(p[10]))
    {
      strncpy(pom, p, 11);
      pom[11] = '\0';
      p += 11;
    }
    p += strspn(p, "/");
#else
    if(strlen(p) > 2 && p[0] == '/' && p[1] == '/' &&
      tl_ascii_isalpha(p[2]) && (p[3] == '\0' || p[3] == '/'))
    {
      strncpy(pom, p, 3);
      pom[3] = '\0';
      p += 3;
    }
    p += strspn(p, "/");
#endif
#endif
    while(*p)
    {
      int ilen = strcspn(p, "/");

      strcat(pom, "/");
      strncat(pom, p, ilen);

      p += ilen;
      p += strspn(p, "/");

      if(*p && access(pom, F_OK))
      {
        if(mkdir(pom, S_IRWXU | S_IRGRP | S_IROTH | S_IXGRP | S_IXOTH))
          return -1;
      }
    }
  }
  return 0;
}

/***********************************************/
/* z relativnej cesty urobi absolutnu a vyhodi */
/* vsetky "." a ".." adresare                  */
/* FIXME: Translate me!                        */
/***********************************************/
char *get_abs_file_path_oss(char *path)
{
  char *p, pom[PATH_MAX], *tmp, result[PATH_MAX] = "/";
  int ilen;
  bool_t last = 1;

  for(p = path; tl_ascii_isspace(*p) && *p; p++);

  if(*p != '/')
  {
    tmp = (char *) getcwd(NULL, PATH_MAX);
    if(priv_cfg.cache_dir)
      sprintf(pom, "%s/%s", priv_cfg.cache_dir, p);
    else
      sprintf(pom, "%s/%s", tmp, p);
    _free(tmp);
  }
  else
  {
    sprintf(pom, "%s", p);
  }

  p = pom;

#ifdef __CYGWIN__
#ifndef HAVE_NEWSTYLE_CYGWIN
  /* workaround to allow //[drive]/... paths on WIN32 */
  if(strlen(pom) > 2 && pom[0] == '/' && pom[1] == '/' &&
    tl_ascii_isalpha(pom[2]) && (pom[3] == '\0' || pom[3] == '/'))
  {
    strncpy(result, pom, 3);
    result[3] = '\0';
    p = pom + 3;
  }
#else
  /* workaround to allow //host/share/... paths on WIN32 */
  /* AFAIK this type of paths work with cygwin-1.1 =<    */
  if(pom[0] == '/' && pom[1] == '/')
  {
    strcpy(result, "//");
    p++;
  }
#endif
#endif
  if(!*p)
    strcpy(result, "/");

  while(*p)
  {
    ilen = strcspn(p, "/");
    if(*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    if(strcmp(p, "."))
    {
      if(strcmp(p, ".."))
      {
        if(!tl_is_dirname(result))
          strcat(result, "/");
        strcat(result, p);
      }
      else
      {
        tmp = strrchr(result, '/');
        *(tmp + 1 - (tmp != result)) = '\0';
      }
    }
    p += ilen + last;
    p += strspn(p, "/");
  }

  ilen = strlen(path);
  p = path + ilen - 1;
  if(ilen && *p != '/' && tl_is_dirname(result))
  {
    result[strlen(result) - 1] = '\0';
  }

  if((tl_is_dirname(path) && !tl_is_dirname(result)) || (strlen(result) == 0))
  {
    result[strlen(result) + 1] = '\0';
    result[strlen(result)] = '/';
  }

  return tl_strdup(result);
}

/***********************************************/
/* z relativnej cesty urobi absolutnu a vyhodi */
/* vsetky "." a ".." adresare                  */
/* FIXME: Translate me!                        */
/***********************************************/
char *get_abs_file_path(char *path)
{
  char *p, pom[PATH_MAX], *tmp, result[PATH_MAX] = "/";
  int ilen;
  bool_t last = 1;

  for(p = path; tl_ascii_isspace(*p) && *p; p++);

  if(*p != '/')
  {
    tmp = (char *) getcwd(NULL, PATH_MAX);
    sprintf(pom, "%s/%s", tmp ? tmp : "", p);
    _free(tmp);
  }
  else
  {
    sprintf(pom, "%s", p);
  }

  p = pom;

  if(!*p)
    strcpy(result, "/");

  while(*p)
  {
    ilen = strcspn(p, "/");
    if(*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    if(strcmp(p, "."))
    {
      if(strcmp(p, ".."))
      {
        if(!tl_is_dirname(result))
          strcat(result, "/");
        strcat(result, p);
      }
      else
      {
        tmp = strrchr(result, '/');
        *(tmp + 1 - (tmp != result)) = '\0';
      }
    }
    p += ilen + last;
    p += strspn(p, "/");
  }

  ilen = strlen(path);
  p = path + ilen - 1;
  if(ilen && *p != '/' && tl_is_dirname(result) == '/')
  {
    result[strlen(result) - 1] = '\0';
  }

  if((tl_is_dirname(path) && !tl_is_dirname(result)) || (strlen(result) == 0))
  {
    result[strlen(result) + 1] = '\0';
    result[strlen(result)] = '/';
  }

  return tl_strdup(result);
}

#ifdef __CYGWIN__
char *cvt_win32_to_unix_path(char *path)
{
  char pom[PATH_MAX];
  char *p = tl_strdup(path);
  char *p1;

  pom[0] = '\0';
  p1 = p;

  if(strlen(p) >= 2 && tl_ascii_isalpha(p[0]) && p[1] == ':')
  {
#ifdef HAVE_NEWSTYLE_CYGWIN
    sprintf(pom, "/cygdrive/%c", p[0]);
    p += 2;
#else
    sprintf(pom, "//%c", p[0]);
    p += 2;
#endif
  }

  _strtrchr(p, '\\', '/');

  strcat(pom, p);
  _free(p1);

  return get_abs_file_path_oss(pom);
}

char *cvt_unix_to_win32_path(char *path)
{
  char res[PATH_MAX];
  cygwin32_conv_to_win32_path(path, res);
  return tl_strdup(res);
}
#endif

/**********************************/
/* spocita vyskyt znaku v retazci */
/* FIXME: Translate me!           */
/**********************************/
static int str_cnt_chr(char *s, int c)
{
  char *p = s;
  int ret = 0;

  while(*p)
  {
    if(*p == c)
      ret++;

    p++;
  }

  return ret;
}

/********************************************/
/* urci relativnu cestu z adresara v ktorom */
/* sa nachadza prvy subor na druhy subor    */
/* FIXME: Translate me!                     */
/********************************************/
char *get_relative_path(char *fromabs, char *toabs)
{
  char *p1, *p2, *pom1, *pom2;
  int offset = 0, i, plom;
  char *rv = NULL;

  pom1 = p1 = get_abs_file_path_oss(fromabs);
  pom2 = p2 = get_abs_file_path_oss(toabs);

  while(*p1 && *p2 && *p1 == *p2)
  {
    p1++;
    p2++;
  }

#if 0
  /* this is not good behaviour, as lynx and netscape behaves */
  /* differently on empty HREFs                               */
  if(!strcmp(p1, p2))
  {
    free(pom1);
    free(pom2);
    return tl_strdup("");
  }
#endif

  if(*p1)
    p1--;

  while((p1 >= pom1) && *p1 != '/')
    p1--;

  if(*p1 != '/')
  {
    free(pom1);
    free(pom2);

    return NULL;
  }
  offset = p1 - pom1;

  plom = str_cnt_chr(p1 + 1, '/');

  for(i = 0; i < plom; i++)
  {
    rv = tl_str_concat(rv, "../", NULL);
  }

  rv = tl_str_concat(rv, pom2 + offset + 1, NULL);

  free(pom1);
  free(pom2);

  return rv;
}

/********************************/
/* vrati poziciu pripony suboru */
/* FIXME: Translate me!         */
/********************************/
char *tl_get_extension(char *fname)
{
  char *p1, *p2;

  p1 = strrchr(fname, '.');
  p2 = strrchr(fname, '/');

  if(p1 > p2)
  {
    return (p1 + 1);
  }
  else
    return "";

}

char *tl_get_basename(char *fname)
{
  char *p;

  if((p = strrchr(fname, '/')))
    p++;
  else
    p = fname;

  return p;
}

static const char *html_tag_tab[] = {
  "<HTML",
  "<html",
  "<HEAD",
  "<head",
  "<META",
  "<meta",
  "<TITLE",
  "<title",
  "<BODY",
  "<body",
  "<script",
  "<SCRIPT",
  "<style",
  "<STYLE",
  "<!DOCTYPE HTML",
  "<!doctype html",
  "<!--",
};

bool_t ext_is_html(char *fn)
{
  char *ext = tl_get_extension(fn);

  return str_is_in_list(0, ext, "html", "htm", "shtml", "phtml", "css", NULL);
}

static bool_t ext_is_nothtml(char *fn)
{
  char *ext = tl_get_extension(fn);

  return str_is_in_list(0, ext, "gif", "jpg", "jpeg", "png", "mpeg",
    "mpg", "avi", "pdf", "gz", "tgz", "zip", "arj",
    "hqx", "rar", "tar", "Z", "doc", "doc", "xls", "wav", "au", "mp3", NULL);
}

bool_t file_is_html(char *fn)
{
  int i, j, len;
  char pom[256];
  bufio *fd;

  if(ext_is_html(fn))
    return TRUE;

  if(ext_is_nothtml(fn))
    return FALSE;

  if((fd = bufio_open(fn, O_BINARY | O_RDONLY)))
  {
    for(j = 0; j < 10; j++)
    {
      if((len = bufio_readln(fd, pom, sizeof(pom))) > 0)
      {
        for(i = 0; i < NUM_ELEM(html_tag_tab); i++)
          if(strstr(pom, html_tag_tab[i]))
          {
            bufio_close(fd);
            return TRUE;
          }
      }
      else
      {
        if(len < 0)
          xperror("file_is_html");
        bufio_close(fd);
        return FALSE;
      }
    }
    bufio_close(fd);
  }
  return FALSE;
}

void tl_sleep(unsigned int s)
{
  /* if we measure timings, we don't sleep */
  if(cfg.time_logfile)
  {
    return;
  }

#ifndef HAVE_MT
  if(cfg.xi_face)
  {
    gui_msleep(s * 1000);
  }
  else
#endif
  {
#ifdef HAVE_MT
    struct timeval tout;

    tout.tv_sec = s;
    tout.tv_usec = 0;

    select(0, NULL, NULL, NULL, &tout);
#else
    sleep(s);
#endif
  }
}

void tl_msleep(unsigned int ms)
{
  /* if we measure timings, we don't sleep */
  if(cfg.time_logfile)
  {
    return;
  }

#ifndef HAVE_MT
  if(cfg.xi_face)
  {
    gui_msleep(ms);
  }
  else
#endif

#if defined HAVE_USLEEP && !defined HAVE_MT
    usleep(ms * 1000);
#else
  {
    struct timeval tout;

    tout.tv_sec = ms / 1000;
    tout.tv_usec = (ms % 1000) * 1000;

    select(0, NULL, NULL, NULL, &tout);
  }
#endif
}

int unlink_recursive(char *fn)
{
  struct stat estat;

  if(lstat(fn, &estat))
  {
    xperror(fn);
    return -1;
  }

  if(!S_ISDIR(estat.st_mode))
  {
    if(unlink(fn))
    {
      xperror(fn);
      return (-1);
    }
  }
  else
  {
    DIR *dir;
    struct dirent *dent;
    char next_dir[PATH_MAX];

    if(!(dir = opendir(fn)))
    {
      xperror(fn);
      return -1;
    }

    while((dent = readdir(dir)))
    {
      sprintf(next_dir, "%s/%s", fn, dent->d_name);
      if(!strcmp(dent->d_name, "."))
        continue;
      if(!strcmp(dent->d_name, ".."))
        continue;

      unlink_recursive(next_dir);
    }

    closedir(dir);

    if(rmdir(fn))
    {
      xperror(next_dir);
      return -1;
    }
  }

  return 0;
}

int str_is_in_list(int casesensitive, char *str, ...)
{
  char *which;
  va_list args;
  int found = FALSE;

  va_start(args, str);

  for(which = va_arg(args, char *); which; which = va_arg(args, char *))
  {
    if(casesensitive ? !strcmp(str, which) : !strcasecmp(str, which))
    {
      found = TRUE;
      break;
    }
  }

  va_end(args);

  return found;
}

int copy_fd_to_file(int fd, char *filename)
{
  char pom[32768];
  int len;
  int dfd;

  if((dfd = open(filename, O_BINARY | O_WRONLY | O_CREAT, 0644)) < 0)
  {
    xperror(filename);
    return -1;
  }

  lseek(fd, 0, SEEK_SET);
  while((len = read(fd, pom, sizeof(pom))) > 0)
  {
    if(len != write(dfd, pom, len))
      return -1;
  }

  close(dfd);

  lseek(fd, 0, SEEK_END);

  return len;
}

char *tl_adjust_filename(char *path)
{
  char *pom;
  char *p, *p2;
  int l, n;

  pom = _malloc(strlen(path) + 1);

  p = pom;
#ifdef __CYGWIN__
#ifndef HAVE_NEWSTYLE_CYGWIN
  l = strspn(path, "/");
  strncpy(p, path, l);
  p += l;
  *p = '\0';
  path += l;
#endif
#endif

  while(*path)
  {
    n = strspn(path, "/");
    path += n;
    l = strcspn(path, "/");
    if(n)
    {
      *p = '/';
      p++;
    }

    if(!*(path + l) && ((NAME_MAX - 4) < l))
    {
      strncpy(p, path + l - (NAME_MAX - 4), NAME_MAX - 4);
      p += NAME_MAX - 4;
    }
    else if(l > NAME_MAX)
    {
      strncpy(p, path, NAME_MAX);
      p += NAME_MAX;
    }
    else
    {
      strncpy(p, path, l);
      p += l;
    }
    path += l;
  }
  *p = '\0';

  n = strlen(pom);
  p = strrchr(pom, '/');

  while(p && (n > PATH_MAX))
  {
    *p = '\0';
    p2 = strrchr(pom, '/');

    if(p2)
    {
      strcpy(p2 + 1, p + 1);
      n -= p - p2;
    }
    p = p2;
  }
  return pom;
}

int tl_filename_needs_adjust(char *path)
{
  int l;

  if(strlen(path) > PATH_MAX)
    return TRUE;

  while(*path)
  {
    path += strspn(path, "/");
    l = strcspn(path, "/");

    if(!*(path + l) && ((NAME_MAX - 4) < l))
      return TRUE;
    else if(l > NAME_MAX)
      return TRUE;
    path += l;
  }


  return FALSE;
}

int tl_is_dirname(const char *path)
{
  const char *p = strrchr(path, '/');
  return (p && (*(p + 1) == '\0'));
}

char *tl_str_append(char *str1, char *str2)
{
  int l1, l2;
  char *rv;

  l1 = str1 ? strlen(str1) : 0;
  l2 = strlen(str2);
  rv = _realloc(str1, l1 + l2 + 1);
  strcpy(rv + l1, str2);

  return rv;
}

char *tl_str_nappend(char *str1, const char *str2, int n)
{
  int l1;
  char *rv;

  l1 = str1 ? strlen(str1) : 0;
  rv = _realloc(str1, l1 + n + 1);
  strncpy(rv + l1, str2, n);
  rv[l1 + n] = '\0';

  return rv;
}


char *tl_str_concat(char *str1, ...)
{
  char *p;
  va_list args;
  int len;
  char *rv = str1;

  len = str1 ? strlen(str1) : 0;

  va_start(args, str1);

  for(p = va_arg(args, char *); p; p = va_arg(args, char *))
  {
    int slen = strlen(p);

    rv = _realloc(rv, len + slen + 1);
    strcpy(rv + len, p);
    len += slen;
  }

  va_end(args);

  return rv;
}

char *tl_data_concat_str(int *len, char *data, ...)
{
  char *p;
  va_list args;
  char *rv = data;

  va_start(args, data);

  for(p = va_arg(args, char *); p; p = va_arg(args, char *))
  {
    int slen = strlen(p);

    rv = _realloc(rv, *len + slen + 1);
    strcpy(rv + *len, p);
    *len += slen;
  }

  va_end(args);

  return rv;
}

char *tl_data_concat_data(int *tlen, char *tdata, int len, char *data)
{
  tdata = _realloc(tdata, *tlen + len);
  memcpy(tdata + *tlen, data, len);
  *tlen += len;

  return tdata;
}

char *tl_load_text_file(char *filename)
{
  char pom[1024];
  int tlen, len, fd;
  char *rv = NULL;

  if((fd = open(filename, O_RDONLY | O_BINARY)) < 0)
  {
    xperror(filename);
    return NULL;
  }

  tlen = 0;

  while((len = read(fd, pom, sizeof(pom))) > 0)
  {
    rv = tl_data_concat_data(&tlen, rv, len, pom);
  }

  close(fd);

  if(rv)
  {
    rv = _realloc(rv, tlen + 1);
    rv[tlen] = '\0';
  }

  return rv;
}

int tl_save_text_file(char *filename, char *content, int length)
{
  int fd;
  int rv = 0;

  if(length < 0)
    length = strlen(content);

  if((fd = open(filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR)) < 0)
  {
    xperror(filename);
    return -1;
  }

  if(write(fd, content, length) != length)
  {
    xperror(filename);
    rv = -1;
  }

  close(fd);

  return rv;
}
