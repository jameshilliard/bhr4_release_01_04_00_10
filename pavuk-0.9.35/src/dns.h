/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _dns_h_
#define _dns_h_

#define DNS_MAX_ADDR    64

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct _abs_addr
{
  int family;
  char addr[DNS_MAX_ADDR];
} abs_addr;

#ifndef HAVE_SOCKADDR_STORAGE
struct sockaddr_storage
{
  int ss_family;
  char padding[132];
};
#endif

extern int dns_gethostbyname(char *, int *, char *, int *);

extern char *dns_get_abs_addr_ip(abs_addr *);
extern struct sockaddr *dns_setup_sockaddr(abs_addr *, int,
  struct sockaddr_storage *, int *);
extern char *dns_get_sockaddr_ip(struct sockaddr *);
extern int dns_get_sockaddr_port(struct sockaddr *);


extern int dns_getprotoid(char *);

extern int dns_serv_start(void);
extern void dns_server_kill(void);
extern void dns_free_tab(void);
extern void print_all_dns_infos(void);
#endif
