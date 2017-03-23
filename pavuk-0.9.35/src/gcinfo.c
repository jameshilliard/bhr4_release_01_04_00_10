/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "gcinfo.h"
#include "url.h"
#include "doc.h"
#include "tools.h"
#include "http.h"
#include "ftp.h"
#include "myssl.h"

void save_global_connection_data(global_connection_info *infop, doc *docp)
{
  /*** preserve FTP control connection ***/
  if(docp->ftp_control)
  {
    infop->ftp_con.proto = docp->doc_url->type;
    infop->ftp_con.control = docp->ftp_control;
    infop->ftp_con.host = new_string(url_get_site(docp->doc_url));
    infop->ftp_con.port = url_get_port(docp->doc_url);
    infop->ftp_con.user = new_string(url_get_user(docp->doc_url, NULL));
    infop->ftp_con.passwd = new_string(url_get_pass(docp->doc_url, NULL));
  }
  else
  {
    infop->ftp_con.control = NULL;
    infop->ftp_con.port = 0;
    _free(infop->ftp_con.host);
    _free(infop->ftp_con.user);
    _free(infop->ftp_con.passwd);
  }

  /*** preserve SSL connection ***/
#if defined(USE_SSL) && defined(USE_SSL_IMPL_OPENSSL)
  if(docp->ssl_data_con.ssl_con)
  {
    infop->ssl_con.ssl_con = docp->ssl_data_con.ssl_con;
    infop->ssl_con.ssl_ctx = docp->ssl_data_con.ssl_ctx;
  }
  else
  {
    infop->ssl_con.ssl_con = NULL;
    infop->ssl_con.ssl_ctx = NULL;
  }
#endif
  /*** preserve HTTP connection infos ***/
  if(cfg.auth_reuse_nonce)
    infop->http_con.auth_digest = (http_digest_info *) docp->auth_digest;
  else
    infop->http_con.auth_digest = NULL;

  if(cfg.auth_reuse_proxy_nonce)
    infop->http_con.auth_proxy_digest =
      (http_digest_info *) docp->auth_proxy_digest;
  else
    infop->http_con.auth_proxy_digest = NULL;

  if(docp->datasock)
  {
    infop->http_con.connection = docp->datasock;
    infop->http_con.proto = docp->doc_url->type;
    infop->http_con.port = url_get_port(docp->doc_url);
    infop->http_con.host = new_string(url_get_site(docp->doc_url));

    if(docp->http_proxy)
    {
      infop->http_con.http_proxy_port = docp->http_proxy_port;
      infop->http_con.http_proxy = docp->http_proxy;
      docp->http_proxy = NULL;
    }
    else
    {
      infop->http_con.http_proxy = NULL;
      infop->http_con.http_proxy_port = 0;
    }
  }
  else
  {
    infop->http_con.connection = NULL;
    infop->http_con.proto = docp->doc_url->type;
    infop->http_con.port = 0;
    _free(infop->http_con.host);
    _free(infop->http_con.http_proxy);
  }
  _free(docp->http_proxy);
}

void restore_global_connection_data(global_connection_info *infop, doc *docp)
{
#if defined(HAVE_MT) && defined(I_FACE)
  if(cfg.cfg_changed > cfg.timestamp)
  {
    privcfg_free(&priv_cfg);
    privcfg_make_copy(&priv_cfg);
  }
#endif
  if(infop->ftp_con.control)
  {
    char *pass = url_get_pass(docp->doc_url, NULL);
    char *user = url_get_user(docp->doc_url, NULL);
    char *host = url_get_site(docp->doc_url);
    unsigned short port = url_get_port(docp->doc_url);

    if((docp->doc_url->type == URLT_FTP ||
        docp->doc_url->type == URLT_FTPS) &&
      infop->ftp_con.proto == docp->doc_url->type &&
      infop->ftp_con.port == port &&
      !strcmp(infop->ftp_con.host, host) &&
      ((pass && infop->ftp_con.passwd &&
          !strcmp(pass, infop->ftp_con.passwd)) ||
        (!pass && !infop->ftp_con.passwd)) &&
      ((user && infop->ftp_con.user &&
          !strcmp(user, infop->ftp_con.user)) ||
        (!user && !infop->ftp_con.user)))
    {
      docp->ftp_control = infop->ftp_con.control;
    }
    else
    {
      bufio_write(infop->ftp_con.control, "QUIT\r\n", 6);
      bufio_close(infop->ftp_con.control);
    }
    _free(infop->ftp_con.host);
    _free(infop->ftp_con.user);
    _free(infop->ftp_con.passwd);
  }
  infop->ftp_con.control = NULL;
#if defined(USE_SSL) && defined(USE_SSL_IMPL_OPENSSL)
  if(infop->ssl_con.ssl_con)
  {
    docp->ssl_data_con.ssl_con = infop->ssl_con.ssl_con;
    docp->ssl_data_con.ssl_ctx = infop->ssl_con.ssl_ctx;
  }
  else
    memset(&docp->ssl_data_con, '\0', sizeof(ssl_connection));
#endif
  if(cfg.auth_reuse_nonce && infop->http_con.auth_digest &&
    (docp->doc_url->type == URLT_HTTP || docp->doc_url->type == URLT_HTTPS))
  {
    if(!strcmp(infop->http_con.auth_digest->site,
        url_get_site(docp->doc_url)) &&
      infop->http_con.auth_digest->port == url_get_port(docp->doc_url))
    {
      docp->auth_digest = infop->http_con.auth_digest;
    }
    else
    {
      http_digest_deep_free(infop->http_con.auth_digest);
    }
  }
  infop->http_con.auth_digest = NULL;

  if(cfg.auth_reuse_proxy_nonce)
    docp->auth_proxy_digest = infop->http_con.auth_proxy_digest;
  else
  {
    if(infop->http_con.auth_proxy_digest)
      http_digest_deep_free(infop->http_con.auth_proxy_digest);
  }
  infop->http_con.auth_proxy_digest = NULL;

  if(infop->http_con.connection)
  {
    char *host = url_get_site(docp->doc_url);
    unsigned short port = url_get_port(docp->doc_url);

    if(infop->http_con.proto == docp->doc_url->type &&
      infop->http_con.port == port && !strcmp(infop->http_con.host, host))
    {
      docp->datasock = infop->http_con.connection;
      docp->http_proxy = infop->http_con.http_proxy;
      docp->http_proxy_port = infop->http_con.http_proxy_port;
    }
    else
    {
      bufio_close(infop->http_con.connection);
      docp->datasock = NULL;
      _free(infop->http_con.http_proxy);
    }
    infop->http_con.connection = NULL;
    _free(infop->http_con.host);
  }
}

void kill_global_connection_data(global_connection_info *infop)
{
  /**** close FTP control connection ****/
  if(infop->ftp_con.control)
  {
    bufio_write(infop->ftp_con.control, "QUIT\r\n", 6);
    bufio_close(infop->ftp_con.control);
    infop->ftp_con.control = NULL;
    _free(infop->ftp_con.host);
    _free(infop->ftp_con.user);
    _free(infop->ftp_con.passwd);
  }
  /*** close preserved SSL connection ***/
#if defined(USE_SSL) && defined(USE_SSL_IMPL_OPENSSL)
  if(infop->ssl_con.ssl_con)
    my_ssl_connection_close(&infop->ssl_con);
#endif
  if(infop->http_con.auth_digest)
    http_digest_deep_free(infop->http_con.auth_digest);
  if(infop->http_con.auth_proxy_digest)
    http_digest_deep_free(infop->http_con.auth_proxy_digest);

  if(infop->http_con.connection)
  {
    bufio_close(infop->http_con.connection);
    _free(infop->http_con.host);
  }
  _free(infop->http_con.http_proxy);
}

void init_global_connection_data(global_connection_info *infop)
{
  infop->ftp_con.proto = URLT_FTP;
  infop->ftp_con.port = 0;
  infop->ftp_con.host = NULL;
  infop->ftp_con.user = NULL;
  infop->ftp_con.passwd = NULL;
  infop->ftp_con.control = NULL;
#ifdef USE_SSL
  memset(&infop->ssl_con, '\0', sizeof(ssl_connection));
#endif
  infop->http_con.auth_digest = NULL;
  infop->http_con.auth_proxy_digest = NULL;
  infop->http_con.connection = NULL;
  infop->http_con.host = NULL;
  infop->http_con.proto = URLT_HTTP;
  infop->http_con.port = 0;
  infop->http_con.http_proxy_port = 0;
  infop->http_con.http_proxy = NULL;
}
