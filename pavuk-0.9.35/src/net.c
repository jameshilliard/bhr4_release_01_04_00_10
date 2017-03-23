/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
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
#include <sys/time.h>
#include <ctype.h>

#include "dns.h"
#include "gui_api.h"
#include "absio.h"
#include "net.h"
#include "doc.h"
#include "http.h"

int net_host_to_in_addr(char *hostname, abs_addr *haddr)
{
  int len;

  if(dns_gethostbyname(hostname, &len, haddr->addr, &haddr->family))
    return -1;

  return 0;
}

/*****************************************************************/
/* otvori spojenie na dany port servera  s (IP, DNS) adresou     */
/* FIXME: Translate me!                                          */
/*****************************************************************/
static int net_connect_helper(char *hostname, int port_no, doc * docp)
{
  struct sockaddr *addr;
  struct sockaddr_storage saddr;
  abs_addr haddr;
  int sock;
  int rv, l, sas, tcp;

  int actual_port = port_no;
  char *actual_hostname = hostname;
  http_proxy *hp;

  if(docp->doc_url->type == URLT_HTTPS && cfg.transparent_ssl_proxy)
    hp = cfg.transparent_ssl_proxy;
  else
    hp = cfg.transparent_proxy;

  /*
   * Check for transparent redirect
   *
   */
  if(hp)
  {
    actual_hostname = hp->addr;
    actual_port = hp->port;
  }

  _h_errno_ = 0;

  if(net_host_to_in_addr(actual_hostname, &haddr))
    return -2;

  gettimeofday(&docp->dns_time, NULL);

  _Xt_Serve;

  tcp = dns_getprotoid("tcp");

  if((sock = socket(haddr.family, SOCK_STREAM, tcp)) == -1)
    return -1;

  if(fcntl(sock, F_SETFL, O_NONBLOCK))
  {
    xperror("fcntl() - F_SETFL");
    close(sock);
    return -1;
  }

  if(cfg.local_ip)
  {
    struct sockaddr_storage sladdr;
    struct sockaddr *laddr;

    laddr = dns_setup_sockaddr(&cfg.local_ip_addr, 0, &sladdr, &sas);

    if(bind(sock, laddr, sas) == -1)
    {
      close(sock);
      return -1;
    }
  }

  addr = dns_setup_sockaddr(&haddr, actual_port, &saddr, &sas);

  rv = connect(sock, addr, sas);
  if(rv &&
    (errno != EINPROGRESS) &&
    (errno != EISCONN) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
  {
    close(sock);
    return -1;
  }


#ifndef HAVE_MT
  if(cfg.xi_face)
  {
    if(rv && (errno == EINPROGRESS))
    {
      rv = gui_wait_io(sock, FALSE);

      if(!rv)
      {
        DEBUG_NET("Async connect - connected\n");
      }

      if(cfg.rbreak || cfg.stop || rv)
      {
        close(sock);
        return -1;
      }

    }
  }
  else
#endif /* !HAVE_MT */
  {
    if(rv &&
      (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK))
    {
      while((rv = tl_selectw(sock, (int) (cfg.ctimeout))) == -1
        && errno == EINTR);
      if(rv <= 0)
      {
        if(rv == 0)
          errno = ETIMEDOUT;
        return -1;
      }
    }
  }

#if defined __QNX__ || defined __BEOS__
  if(connect(sock, addr, sas) && (errno != EISCONN))
  {
    close(sock);
    return -1;
  }
#else
  l = sizeof(rv);
  if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *) &rv, &l) || rv)
  {
    close(sock);
    errno = rv;
    return -1;
  }
#endif

  DEBUG_NET("Successfully connected to %s,%d\n", hostname, port_no);

  return (sock);
}

int net_connect(char *hostname, int port_no, doc *docp)
{
  int rc = net_connect_helper(hostname, port_no, docp);
  /* if net_connect_helper() returned -2, this was dns
   * failure. If -1, it was connect() failure
   */
  if(rc == -2)
  {
    rc = -1;
  }
  else
  {
    gettimeofday(&docp->connect_time, NULL);
  }
  return rc;
}

int net_bindport(abs_addr *haddr, int port_min, int port_max)
{
  int sock;
  int port;
  int n, tcp;
  struct sockaddr *addr;
  struct sockaddr_storage saddr;
  char pom[512];

  addr = (struct sockaddr *) &saddr;

  tcp = dns_getprotoid("tcp");

  if((sock = socket(haddr->family, SOCK_STREAM, tcp)) == -1)
  {
    perror("socket");
    return -1;
  }

  n = 1;
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n)))
    xperror("net_bind: setsockopt - SO_REUSEADDR");

  if(port_min < 0 || port_max < 0)
  {
    port = 0;

    addr = dns_setup_sockaddr(haddr, port, &saddr, &n);

    if(bind(sock, addr, n) == -1)
    {
      snprintf(pom, sizeof(pom), "bind to port(%d)", port);
      xperror(pom);
      close(sock);
      return -1;
    }
  }
  else
  {
    /*
       pro: take a port out of the specified range
     */
    int count = 50;
    double port_range = port_max - port_min + 1;
    int success = 0;

    DEBUG_NET("trying ports in range %d - %d\n", port_min, port_max);
    while(count >= 0)
    {
      if(count == 0)
      {
        port = 0;
        DEBUG_NET("trying system assigned port\n");
      }
      else
      {
        port = port_min + (int) (port_range * rand() / (RAND_MAX + 1.0));
        assert(port >= port_min && port <= port_max);
      }

      addr = dns_setup_sockaddr(haddr, port, &saddr, &n);

      if(bind(sock, addr, n) == 0)
      {
        success = 1;
        break;
      }

      DEBUG_NET("trying new port : port %d failed\n", port);

      count--;
    }

    if(!success)
    {
      snprintf(pom, sizeof(pom), "bind to ports (%d-%d)", port_min, port_max);
      xperror(pom);
      close(sock);
      return -1;
    }
  }

  if(getsockname(sock, addr, &n))
  {
    xperror("getsockname");
    close(sock);
    return -1;
  }

  DEBUG_NET("BINDING to port : %d\n", dns_get_sockaddr_port(addr));

  if(listen(sock, 1) == -1)
  {
    xperror("listen");
    close(sock);
    return -1;
  }

  return sock;
}

int net_accept(int sock)
{
  struct sockaddr *caller;
  struct sockaddr_storage scaller;
  int p, rsock = -1;
  int rv;


  if(fcntl(sock, F_SETFL, O_NONBLOCK))
  {
    xperror("fcntl() - F_SETFL");
    close(sock);
    return -1;
  }

  caller = (struct sockaddr *) &scaller;
  p = sizeof(scaller);

  rsock = accept(sock, caller, &p);
  if((rsock < 0) && (errno != EWOULDBLOCK) && (errno != EAGAIN))
  {
    close(sock);
    return -1;
  }

#ifndef HAVE_MT
  if(cfg.xi_face)
  {
    if((rsock < 0) && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
      rv = gui_wait_io(sock, TRUE);
      if(rv || cfg.rbreak || cfg.stop)
      {
        close(sock);
        return -1;
      }
    }
  }
  else
#endif /* !HAVE_MT */
  {
    if((rsock < 0) && (errno == EWOULDBLOCK || errno == EAGAIN))
    {
      while((rv = tl_selectr(sock, (int) (cfg.ctimeout))) == -1
        && errno == EINTR);
      if(rv <= 0)
      {
        if(rv == 0)
          errno = ETIMEDOUT;
        close(sock);
        return -1;
      }
    }
  }

  if((rsock < 0) && ((rsock = accept(sock, caller, &p)) == -1))
  {
    close(sock);
    return -1;
  }

#ifdef DEBUG
  if(cfg.debug)
  {
    char *ip = dns_get_sockaddr_ip(caller);
    DEBUG_NET("ACCEPTING connection from: %s:%d\n", ip,
      dns_get_sockaddr_port(caller));
    _free(ip);
  }
#endif

  return rsock;
}
