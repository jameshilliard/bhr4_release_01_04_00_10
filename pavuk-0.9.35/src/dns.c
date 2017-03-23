/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#include "tools.h"
#include "bufio.h"
#include "dns.h"

static char** g_dns_infos = NULL;
static int current_count = 0;
static int current_max = 1;
static void add_dnsinf_str_to_final_print(const char* dns_info) {
  if (NULL == g_dns_infos) {
    g_dns_infos = malloc(sizeof(char*) * current_max);
  }

  if (current_count == current_max) {
    current_max *= 2;
    char** new_dns_infos = malloc(sizeof(char*) * current_max);
    memcpy(new_dns_infos, g_dns_infos, sizeof(char*) * current_count);
    free(g_dns_infos);
    g_dns_infos = new_dns_infos;
  }

  g_dns_infos[current_count] = malloc(strlen(dns_info) + 1);
  memcpy(g_dns_infos[current_count], dns_info, strlen(dns_info) + 1);
  
  ++current_count;
}

static const char* DNS_TAG = "dnsZl9Kq7za";
void print_all_dns_infos(void) {
  int i = 0;
  for (; i < current_count; ++i) {
    printf("<%s>%s</%s>\n", DNS_TAG, g_dns_infos[i], DNS_TAG);
  } 
}


/* We use list instead of array to avoid race conditions while reallocating and
 * to avoid relocating condition variables.
 */
typedef struct dns_entry
{
  struct dns_entry *next;
  char *h_name;
  int h_family;
  volatile int h_length;
  char *h_addres;
#ifdef HAVE_MT
  pthread_cond_t h_cond;
#endif
} dns_entry;

typedef struct
{
  dns_entry *e;
} htab_entry;

#define DNS_HASH_NUM 50
static htab_entry dns_htab[DNS_HASH_NUM];

#if defined(I_FACE) && !defined(HAVE_MT)

typedef struct
{
  int serial;
  int status;
  int syserr;
  int herr;
  int family;
  int length;
  char addr[64];
} dns_response_rec_t;

typedef struct
{
  int serial;
  char hostname[256];
} dns_request_rec_t;

static pid_t dns_pid = 0;
static bufio *dns_w = NULL;
static bufio *dns_r = NULL;

static void dns_serv_loop(bufio * in, bufio * out)
{
  int len;

  cfg.xi_face = FALSE;
  cfg.ctimeout = .0;

  for(;;)
  {
    int ok = FALSE;
    dns_request_rec_t req;
    dns_response_rec_t resp;

    if((len = bufio_read(in, (char *) &req, sizeof(req))) != sizeof(req))
    {
      if(len)
        perror("dns server - protocol error");
      else
        printf("dns server - client exited\n");
      exit(1);
    }

    resp.serial = req.serial;

#ifdef HAVE_INET6
    {
      struct addrinfo hints;
      struct addrinfo *addrs;

      memset(&hints, '\0', sizeof(struct addrinfo));
      hints.ai_family = AF_UNSPEC;      /* AF_INET or AF_INET6 */
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = 0;
      if(!getaddrinfo(req.hostname, NULL, &hints, &addrs))
      {
        switch (addrs->ai_family)
        {
        case AF_INET:
          {
            struct sockaddr_in *sa4;
            sa4 = (struct sockaddr_in *) addrs->ai_addr;
            resp.length = sizeof(sa4->sin_addr);
            memcpy(resp.addr, &sa4->sin_addr, resp.length);
          }
          break;
        case AF_INET6:
          {
            struct sockaddr_in6 *sa6;
            sa6 = (struct sockaddr_in6 *) addrs->ai_addr;
            resp.length = sizeof(sa6->sin6_addr);
            memcpy(resp.addr, &sa6->sin6_addr, resp.length);
          }
          break;
        }
        resp.status = 0;
        resp.syserr = 0;
        resp.herr = 0;
        resp.family = addrs->ai_family;
        freeaddrinfo(addrs);
        ok = TRUE;
      }
    }
#else
    {
      struct hostent *host;
      if((host = gethostbyname(req.hostname)))
      {
        resp.status = 0;
        resp.syserr = 0;
        resp.herr = 0;
        resp.family = AF_INET;
        resp.length = host->h_length;
        memcpy(resp.addr, host->h_addr, host->h_length);
        ok = TRUE;
      }
    }
#endif
    if(!ok)
    {
      resp.status = -1;
      resp.syserr = errno;
      resp.herr = h_errno;
    }
    if(bufio_write(out, (char *) &resp, sizeof(resp)) != sizeof(resp))
    {
      perror("dns server - protocol error");
      exit(1);
    }
  }
}

int dns_serv_start(void)
{
  int cfd[2];
  int sfd[2];
  int i;

  for(i = 0; i < DNS_HASH_NUM; i++)
  {
    dns_htab[i].e = NULL;
  }

  if(pipe(cfd))
  {
    xperror("pipe");
    return -1;
  }

  if(pipe(sfd))
  {
    xperror("pipe");
    close(cfd[0]);
    close(cfd[1]);
    return -1;
  }

  if(!(dns_pid = fork()))
  {
    /* child */
    close(sfd[0]);
    close(cfd[1]);
    dns_serv_loop(bufio_fdopen(cfd[0]), bufio_fdopen(sfd[1]));
    exit(0);
  }

  if(dns_pid < 0)
  {
    xperror("fork");
    close(cfd[0]);
    close(cfd[1]);
    close(sfd[0]);
    close(sfd[1]);
    return -1;
  }

  setpgid(0, dns_pid);

  close(sfd[1]);
  close(cfd[0]);

  dns_w = bufio_fdopen(cfd[1]);
  dns_r = bufio_fdopen(sfd[0]);
  dns_pid = dns_pid;

  return 0;
}

void dns_server_kill(void)
{
  if(dns_pid)
  {
    kill(dns_pid, 15);
    waitpid(dns_pid, NULL, 0);
    bufio_close(dns_w);
    bufio_close(dns_r);
    dns_pid = 0;
    dns_w = NULL;
    dns_r = NULL;
  }
}

static int dns_cli_gethostbyname(char *host, int *len, char *addr,
  int *family)
{
  static int serno = 0;
  dns_request_rec_t req;
  dns_response_rec_t resp;

  serno++;

  req.serial = serno;
  memcpy(req.hostname, host, TL_MIN(strlen(host) + 1,
      sizeof(req.hostname) - 1));
  req.hostname[sizeof(req.hostname) - 1] = '\0';

  if(bufio_write(dns_w, (char *) &req, sizeof(req)) != sizeof(req))
  {
    xperror("dns client protocol error");
    return -1;
  }

  do
  {
    if(cfg.rbreak)
    {
      errno = EINTR;
      return -1;
    }

    if(bufio_read(dns_r, (char *) &resp, sizeof(resp)) != sizeof(resp))
    {
      xperror("dns client protocol error");
      return -1;
    }
  }
  while(serno > resp.serial);

  if(resp.status)
  {
    h_errno = resp.herr;
    errno = resp.syserr;
    return -1;
  }
  else
  {
    h_errno = 0;
    memcpy(addr, resp.addr, resp.length);
    *len = resp.length;
    *family = resp.family;
    return 0;
  }
}
#endif /* I_FACE && !HAVE_MT */

int dns_gethostbyname(char *name, int *alen, char *addr, int *family)
{
#ifndef HAVE_INET6
  struct hostent *host;
#ifdef HAVE_GETHOSTBYNAME_R
  struct hostent shost;
#endif
#endif
  int ret;
  dns_entry *eptr;
  int i;
  unsigned int hpos;
#ifdef HAVE_INET6
  {
    struct in_addr ia4;
    struct in6_addr ia6;
    i = inet_pton(AF_INET, name, &ia4);
    if(i > 0)
    {
      *family = AF_INET;
      *alen = sizeof(ia4);
      memcpy(addr, &ia4, sizeof(ia4));
      return 0;
    }
    else if(i < 0 && errno == EAFNOSUPPORT)
    {
      if(inet_pton(AF_INET6, name, &ia6) > 0)
      {
        *family = AF_INET;
        *alen = sizeof(ia6);
        memcpy(addr, &ia6, sizeof(ia6));
        return 0;
      }
    }
  }
#else
  {
    struct in_addr ia4;
    if((ia4.s_addr = inet_addr(name)) != -1)
    {
      *family = AF_INET;
      *alen = sizeof(ia4);
      memcpy(addr, &ia4, sizeof(ia4));
      return 0;
    }
  }
#endif

  LOCK_DNS;
  hpos = hash_func(name, DNS_HASH_NUM);

  for(eptr = dns_htab[hpos].e; eptr != NULL; eptr = eptr->next)
  {
    /* Let's see if name is in the hash table */
    if(!strcasecmp(eptr->h_name, name))
    {
      /* name is in the table. Let's see what's up with it */
      if(eptr->h_length == 0)
      {
        /* if h_length is zero, it means that dns
         * resolution is in progress in a differrent
         * thread. Unlock and wait until done.
         */
#ifdef HAVE_MT
        if(pthread_cond_wait(&eptr->h_cond, &_mt_dns_lock))
        {
          xperror("pthread_cond_wait");
          UNLOCK_DNS;
          return -1;
        }
        /* _mt_dns_lock is locked again here */
#endif

      }
      /* Somebody else already did the lookup for us.
       * Check the results.
       *
       * The following check is the reason why h_length is
       * volatile. It was changed by a differrent thread
       * in a way invisible to complier while we were
       * waiting on the condition variable.
       */
      if(eptr->h_length == -1)
      {
        /* if h_length is -1, it means that the
         * previous attemt to resolve this name has
         * falied. We don't bother again.
         */
        UNLOCK_DNS;
        return -1;
      }
      else
      {
        /* If h_length is positive, it means that the
         * hash table entry is valid and was
         * successfully resolved.
         */
        *alen = eptr->h_length;
        *family = eptr->h_family;
        memcpy(addr, eptr->h_addres, eptr->h_length);
        UNLOCK_DNS;
        return 0;
      }
    }
  }
  if(eptr == NULL)
  {
    /* name was not in the hash table. It's up to us to do the
     * lookup. First we create the new entry for it, to signal
     * to other threads that we are already resolving name.
     */
    eptr = _malloc(sizeof(dns_entry));
#ifdef HAVE_MT
    if(pthread_cond_init(&eptr->h_cond, NULL))
    {
      free(eptr);
      xperror("pthread_cond_init");
      UNLOCK_DNS;
      return -1;
    }
#endif
    eptr->next = dns_htab[hpos].e;
    eptr->h_name = new_string(name);
    eptr->h_length = 0;
    eptr->h_family = AF_UNSPEC;
    eptr->h_addres = NULL;
    dns_htab[hpos].e = eptr;
    add_dnsinf_str_to_final_print(name);
  }
  UNLOCK_DNS;

  *family = AF_INET;

#if defined(I_FACE) && !defined(HAVE_MT)
  if(dns_pid)
  {
    ret = dns_cli_gethostbyname(name, alen, addr, family);
  }
  else
#endif
  {
#if defined(HAVE_INET6)
    {
      struct addrinfo hints;
      struct addrinfo *addrs;
      int err;

      memset(&hints, '\0', sizeof(struct addrinfo));
      hints.ai_family = AF_UNSPEC;      /* AF_INET or AF_INET6 */
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = 0;
      if((err = getaddrinfo(name, NULL, &hints, &addrs)))
      {
        _h_errno_ = err;
        ret = -1;
      }
      else
      {
        *family = addrs->ai_family;
        switch (addrs->ai_family)
        {
        case AF_INET:
          {
            struct sockaddr_in *sa4;
            sa4 = (struct sockaddr_in *) addrs->ai_addr;
            *alen = sizeof(sa4->sin_addr);
            memcpy(addr, &sa4->sin_addr, sizeof(sa4->sin_addr));
          }
          break;
        case AF_INET6:
          {
            struct sockaddr_in6 *sa6;
            sa6 = (struct sockaddr_in6 *) addrs->ai_addr;
            *alen = sizeof(sa6->sin6_addr);
            memcpy(addr, &sa6->sin6_addr, sizeof(sa6->sin6_addr));
          }
          break;
        }
        _h_errno_ = 0;
        ret = 0;
        freeaddrinfo(addrs);
      }
    }
#elif defined(HAVE_GETHOSTBYNAME_R) && !(defined(__OSF__) || defined (__osf__))
    {
      char sbuf[2048];
      int sherrno;
#if defined(__GLIBC__) && __GLIBC__ >= 2
      if(!gethostbyname_r(name, &shost, sbuf, sizeof(sbuf), &host, &sherrno)
        && host)
#elif defined (__OSF__) || defined (__osf__)
      struct hostent_data hdt;
      host = &shost;
      if(!gethostbyname_r(name, &shost, &hdt))
#else
      if((host = gethostbyname_r(name, &shost, sbuf, sizeof(sbuf), &sherrno)))
#endif
      {
        _h_errno_ = 0;
        *alen = host->h_length;
        memcpy(addr, host->h_addr, host->h_length);
        ret = 0;
      }
      else
      {
        _h_errno_ = sherrno;
        ret = -1;
      }
    }
#else
    LOCK_GHBN;
    if((host = gethostbyname(name)))
    {
      _h_errno_ = 0;
      *alen = host->h_length;
      memcpy(addr, host->h_addr, host->h_length);
      ret = 0;
    }
    else
    {
      _h_errno_ = h_errno;
      ret = -1;
    }
    UNLOCK_GHBN;
#endif
  }

  LOCK_DNS;
  /* although dns_htab[hpos].e might have been changed by a differrent
   * thread since we accessed it last time, we don't care since
   * eptr and *eptr weren't.
   */
  if(!ret)
  {
    eptr->h_length = *alen;
    eptr->h_addres = _malloc(*alen);
    eptr->h_family = *family;
    memcpy(eptr->h_addres, addr, *alen);
  }
  else
  {
    eptr->h_length = -1;
  }
#ifdef HAVE_MT
  pthread_cond_broadcast(&eptr->h_cond);
#endif
  UNLOCK_DNS;

  return ret;
}

static void dns_free_bucket(dns_entry * e)
{
  if(e != NULL)
  {
    dns_free_bucket(e->next);
    _free(e->next);
    _free(e->h_name);
    _free(e->h_addres);
#ifdef HAVE_MT
    if(pthread_cond_destroy(&e->h_cond))
    {
      xperror("pthread_cond_destroy");
    }
#endif
  }
}

void dns_free_tab(void)
{
  int i;

  LOCK_DNS;
  for(i = 0; i < DNS_HASH_NUM; i++)
  {
    dns_free_bucket(dns_htab[i].e);
  }
  UNLOCK_DNS;
}

struct sockaddr *dns_setup_sockaddr(abs_addr * addr, int port,
  struct sockaddr_storage *buf, int *sasize)
{
  struct sockaddr *sa = (struct sockaddr *) buf;

  switch (addr->family)
  {
  case AF_INET:
    {
      struct sockaddr_in *sa4 = (struct sockaddr_in *) buf;
      memset(buf, '\0', sizeof(struct sockaddr_in));
      memcpy(&sa4->sin_addr, addr->addr, sizeof(sa4->sin_addr));
      sa4->sin_family = AF_INET;
      sa4->sin_port = htons(port);
      *sasize = sizeof(struct sockaddr_in);
    }
    break;
#ifdef HAVE_INET6
  case AF_INET6:
    {
      struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *) buf;
      memset(buf, '\0', sizeof(struct sockaddr_in6));
      memcpy(&sa6->sin6_addr, addr->addr, sizeof(sa6->sin6_addr));
      sa6->sin6_family = AF_INET6;
      sa6->sin6_port = htons(port);
      *sasize = sizeof(struct sockaddr_in6);
    }
    break;
#endif
  }

  return sa;
}

char *dns_get_sockaddr_ip(struct sockaddr *sa)
{
#ifdef HAVE_INET6
  void *addr;
  char pom[128];

  switch (sa->sa_family)
  {
  case AF_INET:
    {
      struct sockaddr_in *sin;
      sin = (struct sockaddr_in *) sa;
      addr = &sin->sin_addr;
    }
    break;
  case AF_INET6:
    {
      struct sockaddr_in6 *sin;
      sin = (struct sockaddr_in6 *) sa;
      addr = &sin->sin6_addr;
    }
    break;
  default:
    return NULL;
  }
  if(inet_ntop(sa->sa_family, addr, pom, sizeof(pom)))
    return tl_strdup(pom);
  else
    return NULL;
#else
  char *p;
  struct sockaddr_in *sin;

  sin = (struct sockaddr_in *) sa;
  LOCK_INETNTOA;
  p = tl_strdup(inet_ntoa(sin->sin_addr));
  UNLOCK_INETNTOA;
  return p;
#endif
}

int dns_get_sockaddr_port(struct sockaddr *sa)
{
  int ret;

  switch (sa->sa_family)
  {
  case AF_INET:
    {
      struct sockaddr_in *sin;
      sin = (struct sockaddr_in *) sa;
      ret = ntohs(sin->sin_port);
    }
    break;
#ifdef HAVE_INET6
  case AF_INET6:
    {
      struct sockaddr_in6 *sin;
      sin = (struct sockaddr_in6 *) sa;
      ret = ntohs(sin->sin6_port);
    }
    break;
#endif
  default:
    ret = 0;
  }
  return ret;
}

char *dns_get_abs_addr_ip(abs_addr * haddr)
{
#ifdef HAVE_INET6
  void *addr;
  char pom[128];
  struct in_addr ia4;
  struct in6_addr *ia6;

  switch (haddr->family)
  {
  case AF_INET:
    {
      memcpy(&ia4, haddr->addr, sizeof(struct in_addr));
      addr = &ia4;
    }
    break;
  case AF_INET6:
    {
      memcpy(&ia6, haddr->addr, sizeof(struct in6_addr));
      addr = &ia6;
    }
    break;
  default:
    return NULL;
  }
  if(inet_ntop(haddr->family, addr, pom, sizeof(pom)))
    return tl_strdup(pom);
  else
    return NULL;
#else
  char *p;
  struct in_addr ia;

  memcpy(&ia, haddr->addr, sizeof(struct in_addr));
  LOCK_INETNTOA;
  p = tl_strdup(inet_ntoa(ia));
  UNLOCK_INETNTOA;
  return p;
#endif
}

int dns_getprotoid(char *name)
{
#if !defined(IPPROTO_TCP) || !defined(IPPROTO_TCP)
  struct protoent *pe;
  int rv = 0;

  LOCK_INETNTOA;
  pe = getprotobyname(name);
  if(pe)
    rv = pe->p_proto;
  UNLOCK_INETNTOA;

  return rv;
#else
  if(name[0] == 'u' && name[1] == 'd' && name[2] == 'p' && !name[3])
    return IPPROTO_UDP;
  else
    return IPPROTO_TCP;
#endif
}
