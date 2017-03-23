/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#if defined(USE_SSL) && defined(USE_SSL_IMPL_OPENSSL)

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#include "bufio.h"
#include "errcode.h"
#include "http.h"
#include "myssl.h"
#include "tools.h"

/* Let's try to cache session ids! */
/* Very unlikely to have more than 4 session ids active,
 * so use cheap slow lookup */

typedef struct session_entry
{
  int len;
  int family;
  struct sockaddr_in addr;
  SSL_SESSION *session;
} session_entry;

#define N_SESSIONS 10

static session_entry *session_map[N_SESSIONS];
static int session_map_init = -1;

static int cmp_sockaddr(struct sockaddr_in *a, struct sockaddr_in *b)
{
  return (a->sin_addr.s_addr != b->sin_addr.s_addr)
    || (a->sin_port != b->sin_port);
}

static void dump_session_id(SSL_SESSION * s)
{
  int i;
  if(!s)
  {
    DEBUG_SSL("No session ID to dump?\n");
    return;
  }
  DEBUG_SSL("ssl_session_id(%d) = [", s->session_id_length);
  for(i = 0; i < s->session_id_length; i++)
  {
    DEBUG_SSL(" %02x", s->session_id[i]);
  }
  DEBUG_SSL("]\n");
  return;
}

static void map_init(void)
{
  int i;
  if(session_map_init != -1)
    return;
  LOCK_SSL_MAP;
  if(session_map_init == -1)
  {
    for(i = 0; i < N_SESSIONS; i++)
      session_map[i] = 0;
    session_map_init = 1;
  }
  UNLOCK_SSL_MAP;
}

static SSL_SESSION *cache_locate_session_by_addr(struct sockaddr_in *addr,
  int dolock)
{
  int i;

  map_init();

  if(!addr)
  {
    DEBUG_SSL("No addr to locate session id by?\n");
    return 0;
  }

  DEBUG_SSL("Trying to locate session id for ip %s:%d\n",
    inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

  if(dolock)
  {
    LOCK_SSL_MAP;
  }
  for(i = 0; i < N_SESSIONS; i++)
  {
    if(session_map[i] && !cmp_sockaddr(addr, &session_map[i]->addr))
    {
      if(dolock)
      {
        UNLOCK_SSL_MAP;
      }
      DEBUG_SSL("returning slot %d id for %s:%d\n", i,
        inet_ntoa(session_map[i]->addr.sin_addr),
        ntohs(session_map[i]->addr.sin_port));

      dump_session_id(session_map[i]->session);
      return session_map[i]->session;
    }
  }

  if(dolock)
  {
    UNLOCK_SSL_MAP;
  }
  DEBUG_SSL("not cached\n");

  return 0;
}

static void cache_session_by_addr(struct sockaddr_in *addr, SSL * con)
{
  session_entry *p;
  int i;
  SSL_SESSION *s;

  map_init();

  LOCK_SSL_MAP;
  /* see if this session is already in the map */
  s = cache_locate_session_by_addr(addr, 0);
  if(s)
  {
    /* Yes, so simply return */
    UNLOCK_SSL_MAP;
    DEBUG_SSL("session id for %s:%d already cached\n",
      inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    dump_session_id(s);
    return;
  }

  /* Install session in empty entry in map */
  assert(con);
  s = SSL_get1_session(con);

  if(s == 0)
  {
    DEBUG_SSL("(cache session) No session id to cache?\n");
    UNLOCK_SSL_MAP;
    return;
  }

  for(i = 0; i < N_SESSIONS; i++)
  {
    if(session_map[i] == 0)
    {
      p = malloc(sizeof(session_entry));
      p->addr = *addr;
      p->session = s;
      session_map[i] = p;
      UNLOCK_SSL_MAP;
      DEBUG_SSL("(cache_session) session for %s:%d cached in slot %d\n",
        inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), i);
      dump_session_id(p->session);
      return;
    }
  }

  DEBUG_SSL("(cache_session) You don't have a big enough map\n");
  /* FIXME: Replace oldest entry */

  UNLOCK_SSL_MAP;
  return;
}

static void cache_remove_session(SSL_SESSION * sess)
{
  DEBUG_SSL("Turn your head and cough\n");
  /* FIXME: WRITE ME */
}

static int SSL_FD(SSL * ssl)
{
  if(ssl == NULL)
  {
    return -1;
  }

  return BIO_get_fd(SSL_get_rbio(ssl), NULL);

}

static int ssl_new_session_cb(SSL * ssl, SSL_SESSION * sess)
{
  int sock = SSL_FD(ssl);
  struct sockaddr_in sa;
  struct sockaddr_in *sp;
  socklen_t alen = sizeof(sa);
  SSL_SESSION *cached;

  assert(sock != -1);

  DEBUG_SSL("ssl_new_session_cb() called\n");

  if(getpeername(sock, (struct sockaddr *) &sa, &alen))
    return 1;

  DEBUG_SSL("ssl_new_session_cb(%s:%d)\n",
    inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));

  /*
   * Search in cache.
   */
  cached = cache_locate_session_by_addr(&sa, 1);

  if(cached)
  {
    DEBUG_SSL("SSL session for %s:%d already cached.\n",
      inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
    return 1;
  }

  sp = malloc(sizeof(struct sockaddr_in));
  if(sp == NULL)
    return 1;

  *sp = sa;

  cached = SSL_get1_session(ssl);
  assert(cached == sess);

  cache_session_by_addr(sp, ssl);

  DEBUG_SSL("SSL session for %s:%d now cached\n",
    inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));

  return 1;

}

static SSL_SESSION *ssl_get_session_cb(SSL * ssl, unsigned char *data,
  int len, int *copy)
{
  DEBUG_SSL("ssl_get_session_cb()\n");
  return NULL;
}

static void ssl_remove_session_cb(SSL_CTX * ssl, SSL_SESSION * sess)
{
  cache_remove_session(sess);
}

static void ssl_assign_cached_session(SSL * ssl, struct sockaddr_in *peer)
{
  SSL_SESSION *sess;

  DEBUG_SSL("ssl_assign_cached_session() called\n");

  if(peer == NULL)
    return;

  DEBUG_SSL("ssl_assign_cached_session(%s:%d)\n",
    inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));

  sess = cache_locate_session_by_addr(peer, 1);

  if(sess == NULL)
  {
    DEBUG_SSL("SSL session for %s:%d not cached yet\n",
      inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
    return;
  }

  DEBUG_SSL("Setting SSL session for %s:%d\n",
    inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
  dump_session_id(sess);

  if(SSL_set_session(ssl, sess) == 0)
  {
    DEBUG_SSL("Failed to assign cached SSL session for %s:%d\n",
      inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
    return;
  }

  DEBUG_SSL("SSL session for %s:%d found in cache\n",
    inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
}

static void ssl_assign_cached_session_by_doc(SSL * ssl, doc * docp)
{
  char *hp;
  int alen;
  struct sockaddr_in addr;
  int family;

  map_init();

  DEBUG_SSL("assign_cached_session_by_doc()\n");

  if(!docp)
  {
    DEBUG_SSL("(assign cached session by doc) No doc?\n");
    return;
  }

  if(!docp->doc_url)
  {
    DEBUG_SSL("(assign cached session by doc) Doc has no url?\n");
    return;
  }

  hp = docp->doc_url->p.http.host;

  if(dns_gethostbyname(hp, &alen, (char *) &addr.sin_addr, &family) < 0)
  {
    DEBUG_SSL("(assign cached session by doc) Unable to get dns for %s\n",
      hp);
    return;
  }

  assert(family == AF_INET);
  assert(alen == sizeof(addr.sin_addr));
  addr.sin_port = htons(docp->doc_url->p.http.port);

  ssl_assign_cached_session(ssl, &addr);
}

/* Most of this code is inspired by example programs from SSLeay package */
static int my_ssl_passwd_callback(char *buf, int num, int verify)
{
  if(verify)
    xprintf(1, "%s\n", buf);
  else
  {
    if(num > strlen(priv_cfg.ssl_cert_passwd))
    {
      strcpy(buf, priv_cfg.ssl_cert_passwd);
      return strlen(buf);
    }
  }
  return 0;
}

static void my_ssl_info_callback(const SSL * con, int where, int ret)
{
  if(where & SSL_CB_LOOP)
  {
    DEBUG_SSL("SSL_CB_LOOP: (%s) %s\n", SSL_state_string(con),
      SSL_state_string_long(con));
  }
  else if(where & SSL_CB_ALERT)
  {
    DEBUG_SSL("SSL_CB_ALERT: %s:%s\n",
      SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
  }
  else if((where & SSL_CB_EXIT) && (ret <= 0))
  {
    /* If we print 'unknown' errors, we end up spewing the same */
    /* message dozens of times. This avoids that */
    if(SSL_alert_type_string(ret)[0] == 'U')
      return;
    DEBUG_SSL("SSL_CB_EXIT: [%s:%s] (%s) %s\n",
      SSL_alert_type_string_long(ret),
      SSL_alert_desc_string_long(ret),
      SSL_state_string(con), SSL_state_string_long(con));
  }
}

static int my_ssl_verify_callback(int ok, X509_STORE_CTX * ctx)
{
  char buf[256];
  X509 *err_cert;
  int err, depth;

  err_cert = X509_STORE_CTX_get_current_cert(ctx);
  err = X509_STORE_CTX_get_error(ctx);
  depth = X509_STORE_CTX_get_error_depth(ctx);

  X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
  DEBUG_SSL("Depth: %d\n", depth);
  DEBUG_SSL("Subject: %s\n", buf);
  if(!ok)
  {
    DEBUG_SSL("verify error:num=%d:%s\n", err,
      X509_verify_cert_error_string(err));
    if(0 >= depth)
      ok = 1;
    else
      ok = 0;
  }
  switch (ctx->error)
  {
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
    DEBUG_SSL("Issuer: %s\n", buf);
    break;
  case X509_V_ERR_CERT_NOT_YET_VALID:
  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    DEBUG_SSL("Error: SSL certificate not yet valid!\n");
    break;
  case X509_V_ERR_CERT_HAS_EXPIRED:
  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    DEBUG_SSL("Error: SSL certificate has expired!\n");
    break;
  }
  DEBUG_SSL("Verify status:%d\n", ok);

  return ok;
}

void my_ssl_init_once(void)
{
}

void my_ssl_init_start(void)
{
}

void my_ssl_init(void)
{
  unsigned char rnd_stack_data[64], *p = NULL;
  int rv = 0;

#ifdef HAVE_RAND_EGD
  if(!RAND_status())
  {
    char random_dev[PATH_MAX + 1];
    const char *p;

    if(priv_cfg.egd_socket)
    {
      strncpy(random_dev, priv_cfg.egd_socket, PATH_MAX);
      random_dev[PATH_MAX] = '\0';
    }
    else
    {
#ifdef EGD_SOCKET_NAME
      strncpy(random_dev, EGD_SOCKET_NAME, PATH_MAX);
      random_dev[PATH_MAX] = '\0';
#else
      /* we do not have /dev/random so need to seed
         entropy pool from an external source */
      p = RAND_file_name(random_dev, PATH_MAX + 1);
      if(!p)
      {
        xprintf(1, gettext("Failed obtaining entropy pathname\n"));
        rv = -1;
      }
#endif
    }
    if(rv != -1)
    {
      rv = RAND_egd(random_dev);
      if(rv == -1)
      {
        xprintf(1,
          gettext
          ("Failed to initialize random seed for OpenSSL via EGD daemon\n"));
      }
    }
  }
#endif

  if(rv == -1)
  {
    p = rnd_stack_data;
    time((time_t *) p);
    RAND_seed(rnd_stack_data, sizeof(rnd_stack_data));
    xprintf(1, gettext("Seeding entropy pool INSECURELY!\n"));
  }
  SSL_load_error_strings();
  SSLeay_add_ssl_algorithms();
}

void my_ssl_cleanup(void)
{
}

void my_ssl_connection_close(ssl_connection * con)
{
  if(con->ssl_con)
  {
    SSL_set_shutdown(con->ssl_con, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
    SSL_free(con->ssl_con);
  }

  if(con->ssl_ctx)
    SSL_CTX_free(con->ssl_ctx);
  if(con->socket)
    bufio_close(con->socket);

  con->ssl_con = NULL;
  con->ssl_ctx = NULL;
  con->socket = NULL;
}

void my_ssl_connection_destroy(ssl_connection * con)
{
  my_ssl_connection_close(con);
  _free(con);
}

static ssl_connection *my_ssl_connection_new(void)
{
  ssl_connection *con;

  con = _malloc(sizeof(ssl_connection));
  con->ssl_con = NULL;
  con->ssl_ctx = NULL;
  con->socket = NULL;

  return con;
}

bufio *my_ssl_do_connect(doc * docp, bufio * socket,
  ssl_connection * parent_ssl_con)
{
  SSL_METHOD *method;
  BIO *bio;
  ssl_connection *con;
  int rv;

  my_ssl_init();

  if(priv_cfg.ssl_proxy)
  {
    if(http_dumy_proxy_connect(docp,
        url_get_site(docp->doc_url), url_get_port(docp->doc_url),
        priv_cfg.ssl_proxy, cfg.ssl_proxy_port))
    {
      docp->errcode = ERR_PROXY_CONNECT;
      return FALSE;
    }
  }

  con = my_ssl_connection_new();

  switch (cfg.ssl_version)
  {
  case 3:
    method = SSLv3_client_method();
    break;
#ifdef WITH_SSL_TLS1
  case 4:
    method = TLSv1_client_method();
    break;
#endif
  default:
    method = SSLv23_client_method();
    break;
  }

  con->ssl_ctx = SSL_CTX_new(method);

  /* Bugs compatibility */
  SSL_CTX_set_options(con->ssl_ctx, SSL_OP_ALL);

  /* Modes of SSL_send() */
  SSL_CTX_set_mode(con->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
  SSL_CTX_set_mode(con->ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);


  SSL_CTX_set_timeout(con->ssl_ctx, 3600);

  if(cfg.unique_sslid)
  {
    /* Cache sessions */
    DEBUG_SSL("Going to cache session ids!\n");
    SSL_CTX_set_session_cache_mode(con->ssl_ctx, SSL_SESS_CACHE_CLIENT);

    SSL_CTX_sess_set_cache_size(con->ssl_ctx, 16);
    /*
     * Session cache management.
     */
    SSL_CTX_sess_set_new_cb(con->ssl_ctx, ssl_new_session_cb);
    SSL_CTX_sess_set_get_cb(con->ssl_ctx, ssl_get_session_cb);
    SSL_CTX_sess_set_remove_cb(con->ssl_ctx, ssl_remove_session_cb);
  }
  if(cfg.condition.verify)
  {
    SSL_CTX_set_info_callback(con->ssl_ctx, my_ssl_info_callback);
    SSL_CTX_set_verify(con->ssl_ctx, SSL_VERIFY_NONE, my_ssl_verify_callback);
    SSL_CTX_set_verify_depth(con->ssl_ctx, 10);
  }

  if(priv_cfg.ssl_cipher_list)
    SSL_CTX_set_cipher_list(con->ssl_ctx, priv_cfg.ssl_cipher_list);

/* SSL Certification stuff */
  if(priv_cfg.ssl_cert_file != NULL)
  {
    SSL *ssl;
    X509 *x509;
    DEBUG_SSL("Setting cert file to %s\n", priv_cfg.ssl_cert_file);
    if(priv_cfg.ssl_cert_passwd)
#ifdef OPENSSL
      SSL_CTX_set_default_passwd_cb(con->ssl_ctx,
        (pem_password_cb *) my_ssl_passwd_callback);
#else
      SSL_CTX_set_default_passwd_cb(con->ssl_ctx, my_ssl_passwd_callback);
#endif

    if(SSL_CTX_use_certificate_file(con->ssl_ctx,
        priv_cfg.ssl_cert_file, SSL_FILETYPE_PEM) <= 0)
    {
      xprintf(1,
        gettext("Unable to set certificate file (wrong password?)\n"));
      my_ssl_connection_destroy(con);
      return FALSE;
    }

    if(priv_cfg.ssl_key_file == NULL)
      priv_cfg.ssl_key_file = tl_strdup(priv_cfg.ssl_cert_file);

    if(SSL_CTX_use_PrivateKey_file(con->ssl_ctx,
        priv_cfg.ssl_key_file, SSL_FILETYPE_PEM) <= 0)
    {
      xprintf(1, gettext("Unable to set public key file\n"));
      my_ssl_connection_destroy(con);
      return NULL;
    }

    ssl = SSL_new(con->ssl_ctx);

    x509 = SSL_get_certificate(ssl);

    if(x509 != NULL)
      EVP_PKEY_copy_parameters(X509_get_pubkey(x509),
        SSL_get_privatekey(ssl));

    SSL_free(ssl);

    if(!SSL_CTX_check_private_key(con->ssl_ctx))
    {
      xprintf(1,
        gettext("Private key does not match the certificate public key\n"));
      my_ssl_connection_destroy(con);
      return NULL;
    }
  }
/* End SSL Certification stuff */

  con->ssl_con = SSL_new(con->ssl_ctx);
  bio = BIO_new_socket(bufio_getfd(socket), BIO_NOCLOSE);
  SSL_set_bio(con->ssl_con, bio, bio);

  if(cfg.unique_sslid)
  {
    DEBUG_SSL("assigning caching SSL session ID\n");
    ssl_assign_cached_session_by_doc(con->ssl_con, docp);
  }

  DEBUG_SSL("Connecting\n");
  SSL_connect(con->ssl_con);
  DEBUG_SSL("Connecting done\n");

  while((rv = SSL_in_init(con->ssl_con)) &&
    SSL_get_error(con->ssl_con, rv) == SSL_ERROR_WANT_CONNECT)
  {
    tl_msleep(2);
  }

  if(rv < 0)
  {
    SSL_get_error(con->ssl_con, -1);
    ERR_print_errors_fp(stderr);
    my_ssl_connection_destroy(con);
    return NULL;
  }

  con->socket = socket;

  return bufio_new_sslcon(socket, con);
}

int my_ssl_read(ssl_connection * ssl_con, char *buf, size_t len)
{
  int rv = -1;
  bool_t stopread = FALSE;

  while(!stopread)
  {
    rv = SSL_read(ssl_con->ssl_con, buf, (int) len);

    if(rv == 0)                 /* Nothing read */
    {
      stopread = TRUE;
      break;
    }

    switch (SSL_get_error(ssl_con->ssl_con, rv))
    {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      tl_msleep(10);
      break;
    case SSL_ERROR_SSL:
      ERR_print_errors_fp(stdout);
      stopread = TRUE;
      rv = -1;
      break;
    case SSL_ERROR_SYSCALL:
      if((errno != EWOULDBLOCK) && (errno != EAGAIN))
      {
        len = 0;
        stopread = TRUE;
        rv = -1;
      }
      else
        tl_msleep(10);
      break;
    default:
      stopread = TRUE;
      break;
    }
  }

  return rv;
}

int my_ssl_write(ssl_connection * ssl_con, char *buf, size_t len)
{
  int rv = 0;

  while(len)
  {
    rv = SSL_write(ssl_con->ssl_con, buf, (int) len);
    switch (SSL_get_error(ssl_con->ssl_con, rv))
    {
    case SSL_ERROR_NONE:
      len -= rv;
      buf += rv;
      if(rv <= 0)
        len = 0;
      break;
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
      tl_msleep(10);
      break;
    case SSL_ERROR_SSL:
      ERR_print_errors_fp(stdout);
      /* MJF: HACK -- ssl doesn't set errno, but because
         of the -1 return, our caller will try to print
         it, so we set errno to something "reasonable".
       */
      errno = EIO;
      rv = -1;
      len = 0;
      break;
    case SSL_ERROR_SYSCALL:
      if((errno != EWOULDBLOCK) && (errno != EAGAIN))
      {
        len = 0;
        rv = -1;
      }
      else
        tl_msleep(10);
      break;
    default:
      len = 0;
      break;
    }
  }

  return rv;
}

int my_ssl_data_pending(ssl_connection * ssl_con)
{
  return SSL_pending(ssl_con->ssl_con);
}

void my_ssl_print_last_error(void)
{
  ERR_print_errors_fp(stdout);
}

#endif
