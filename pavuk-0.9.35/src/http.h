/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _http_h_
#define _http_h_

#include <time.h>
#include "bufio.h"
#include "doc.h"
#include "dllist.h"
#include "digest_auth.h"

#define DEFAULT_HTTP_PORT       80
#define DEFAULT_HTTP_PROXY_PORT 8080
#define DEFAULT_SSL_PORT        443
#define DEFAULT_SSL_PROXY_PORT  8080

typedef struct
{
  char *addr;                   /* proxy host                        */
  unsigned short port;          /* proxy port                        */
  int penault;                  /* penault to reject failing proxies */
  unsigned int fails;           /* number of fails for proxy         */
  int is_10;                    /* this is HTTP/1.0 proxy            */
  int ref;                      /* reference counting                */
} http_proxy;

typedef struct
{
  short ver_maj;                /*** major version of HTTP protocol ***/
  short ver_min;                /*** minor version of HTTP protocol ***/
  int ret_code;                 /*** HTTP response code             ***/
  char *text;                   /*** HTTP response code description ***/
} http_response;

typedef struct
{
  bool_t all;
  char *name;
  char *val;
} httphdr;

typedef struct
{
  protocol proto;                      /*** protocol ***/
  char *host;                          /*** hostname ***/
  unsigned short port;                 /*** port     ***/
  bufio *connection;                   /*** socket for persistent connection ***/
  http_digest_info *auth_digest;       /*** HTTP digest access authentification info ***/
  http_digest_info *auth_proxy_digest; /*** HTTP digest access proxy authentification info ***/
  char *http_proxy;                    /*** HTTP proxy address ***/
  unsigned short http_proxy_port;      /*** HTTP proxy port ***/
} http_connection;

typedef struct
{
  dllist *form_data;
} http_url_extension;

typedef enum
{
  HTTP_AUTH_NONE = 0,
  HTTP_AUTH_USER = 1,
  HTTP_AUTH_BASIC = 2,
  HTTP_AUTH_DIGEST = 3,
  HTTP_AUTH_NTLM = 4,
  HTTP_AUTH_LAST = 5
} http_auth_type_t;

typedef enum
{
  HTTP_REQ_UNKNOWN,
  HTTP_REQ_GET,
  HTTP_REQ_POST,
  HTTP_REQ_HEAD,
  HTTP_REQ_CONNECT
} http_request_type_t;

typedef struct
{
  char *name;
  http_auth_type_t id;
} http_auth_type_info_t;

extern char *_md5(unsigned char *);

extern int http_connect(char *, int);
extern int http_dumy_proxy_connect(doc *, char *, int, char *, int);
extern int http_dumy_proxy_connect_real(doc *, char *, int, char *, int);
extern bufio *http_get_data_socket(doc *);
extern int http_repeat_request(doc *);
extern int http_get_request(doc *);
extern int http_head_request(doc *);
extern int http_post_request(doc *);
extern int http_read_mime_header(doc *, char **, int *);
extern int http_throw_message_body(doc *);
extern void http_handle_redirect(doc *, int);

extern http_response *http_get_response_info(char *);
extern void http_response_free(http_response *);

extern http_auth_type_t http_get_authorization_type(char *);
extern int http_handle_site_auth_info(doc *);
extern int http_handle_proxy_auth_info(doc *);

extern httphdr *httphdr_parse(char *);
extern void httphdr_free(httphdr *);

extern int http_proxy_ref(http_proxy *);
extern int http_proxy_unref(http_proxy *);
extern void http_proxy_free(http_proxy *);
extern http_proxy *http_proxy_parse(char *);
extern http_proxy *http_proxy_get(void);
extern http_proxy *http_proxy_find(char *, int);
extern void http_proxy_check(http_proxy *, doc *);

extern const http_auth_type_info_t http_auths[];

#endif
