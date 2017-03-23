/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#if defined(USE_SSL) && defined(USE_SSL_IMPL_NSS)

#include <string.h>
#include <time.h>
#include <sys/ioctl.h>

#include <secutil.h>
#include <sslproto.h>

#include "myssl.h"
#include "http.h"
#include "bufio.h"
#include "errcode.h"
#include "tools.h"

#define PR_DESC_PAVUK_BUFIO 8

static PRInt32 PR_CALLBACK myssl_bufio_avail(PRFileDesc * fd)
{
  PRInt32 result;
  bufio *sock;

  sock = (bufio *) fd->secret;
  if(ioctl(sock->fd, FIONREAD, &result) < 0)
    result = -1;
  else
    result += sock->buf_end - sock->buf_start;

  return result;
}

static PRInt64 PR_CALLBACK myssl_bufio_avail64(PRFileDesc * fd)
{
  return myssl_bufio_avail(fd);
}

static PRStatus PR_CALLBACK myssl_bufio_close(PRFileDesc * fd)
{
  return PR_SUCCESS;
}

static PRStatus PR_CALLBACK myssl_bufio_sync(PRFileDesc * fd)
{
  return PR_SUCCESS;
}

static PRStatus PR_CALLBACK myssl_bufio_connect(PRFileDesc * fd,
  const PRNetAddr * addr, PRIntervalTime timeout)
{
  return PR_SUCCESS;
}

static PRInt32 PR_CALLBACK myssl_bufio_read(PRFileDesc * fd, void *buf,
  PRInt32 len)
{
  PRInt32 rv;
  bufio *sock;

  sock = (bufio *) fd->secret;
  rv = bufio_nbfread(sock, buf, len);

  return rv;
}

static PRInt32 PR_CALLBACK myssl_bufio_recv(PRFileDesc * fd, void *buf,
  PRInt32 len, PRIntn flags, PRIntervalTime timeout)
{
  return myssl_bufio_read(fd, buf, len);
}

static PRInt32 PR_CALLBACK myssl_bufio_write(PRFileDesc * fd, void *buf,
  PRInt32 len)
{
  PRInt32 rv;
  bufio *sock;

  sock = (bufio *) fd->secret;
  rv = bufio_write(sock, buf, len);

  return rv;
}

static PRInt32 PR_CALLBACK myssl_bufio_send(PRFileDesc * fd, void *buf,
  PRInt32 len, PRIntn flags, PRIntervalTime timeout)
{
  return myssl_bufio_write(fd, buf, len);
}

static PRInt16 PR_CALLBACK myssl_bufio_poll(PRFileDesc * fd, PRInt16 in_flags,
  PRInt16 * out_flags)
{
  *out_flags = 0;

  if((in_flags & PR_POLL_READ) && myssl_bufio_avail(fd) > 0)
    *out_flags = PR_POLL_READ;

  return in_flags;
}

static PRStatus PR_CALLBACK myssl_bufio_getpeername(PRFileDesc * fd,
  PRNetAddr * addr)
{
  bufio *sock;
  PRUint32 len;

  len = sizeof(PRNetAddr);
  sock = (bufio *) fd->secret;
  if(!getpeername(bufio_getfd(sock), (struct sockaddr *) addr, &len))
    return PR_SUCCESS;
  else
    return PR_FAILURE;
}

static PRStatus PR_CALLBACK myssl_bufio_getsockopt(PRFileDesc * fd,
  PRSocketOptionData * data)
{
  if(data->option == PR_SockOpt_Nonblocking)
  {
    data->value.non_blocking = FALSE;
    return PR_SUCCESS;
  }

  return PR_FAILURE;
}

/* FIXME: externs should be in an include file */
extern PRIntn _PR_InvalidInt(void);
extern PRInt64 _PR_InvalidInt64(void);
extern PRStatus _PR_InvalidStatus(void);
extern PRFileDesc *_PR_InvalidDesc(void);

static const PRIOMethods myssl_bufio_methods = {
  PR_DESC_PAVUK_BUFIO,
  myssl_bufio_close,
  myssl_bufio_read,
  myssl_bufio_write,
  myssl_bufio_avail,
  myssl_bufio_avail64,
  myssl_bufio_sync,
  (PRSeekFN) _PR_InvalidInt,
  (PRSeek64FN) _PR_InvalidInt64,
  (PRFileInfoFN) _PR_InvalidStatus,
  (PRFileInfo64FN) _PR_InvalidStatus,
  (PRWritevFN) _PR_InvalidInt,
  myssl_bufio_connect,
  (PRAcceptFN) _PR_InvalidDesc,
  (PRBindFN) _PR_InvalidStatus,
  (PRListenFN) _PR_InvalidStatus,
  (PRShutdownFN) _PR_InvalidStatus,
  myssl_bufio_recv,
  myssl_bufio_send,
  (PRRecvfromFN) _PR_InvalidInt,
  (PRSendtoFN) _PR_InvalidInt,
  (PRPollFN) myssl_bufio_poll,
  (PRAcceptreadFN) _PR_InvalidInt,
  (PRTransmitfileFN) _PR_InvalidInt,
  (PRGetsocknameFN) _PR_InvalidStatus,
  myssl_bufio_getpeername,
  (PRReservedFN) _PR_InvalidInt,
  (PRReservedFN) _PR_InvalidInt,
  myssl_bufio_getsockopt,
  (PRSetsocketoptionFN) _PR_InvalidStatus,
  (PRSendfileFN) _PR_InvalidInt,
  (PRReservedFN) _PR_InvalidInt,
  (PRReservedFN) _PR_InvalidInt,
  (PRReservedFN) _PR_InvalidInt,
  (PRReservedFN) _PR_InvalidInt,
  (PRReservedFN) _PR_InvalidInt,
};

static void myssl_bufio_init_io_layer(void)
{
  static int inited = FALSE;
  if(!inited)
  {
    myssl_bufio_methods.file_type = PR_GetUniqueIdentity("Pavuk_bufio");
    inited = FALSE;
  }
  inited = TRUE;
}

static PRFileDesc *myssl_bufio_get_io_layer(void)
{
  myssl_bufio_init_io_layer();

  return PR_CreateIOLayerStub(myssl_bufio_methods.file_type,
    &myssl_bufio_methods);
}

static PRFileDesc *myssl_bufio_new_socket(bufio * sock)
{
  PRFileDesc *fd;

  fd = myssl_bufio_get_io_layer();
  fd->secret = (PRFilePrivate *) sock;

  return fd;
}


static char *my_ssl_passwd_callback(PK11SlotInfo * slot, PRBool retry,
  void *arg)
{
  char *passwd = NULL;
  if(!retry && arg)
    passwd = PL_strdup((char *) arg);
  return passwd;
}

static SECStatus my_ssl_verify_callback(void *arg, PRFileDesc * fd,
  PRBool check_sig, PRBool is_server)
{
  SECStatus rv;
  CERTCertificate *cert;

  cert = SSL_PeerCertificate(fd);

  DEBUG_SSL("Subject: %s\n", cert->subjectName);
  DEBUG_SSL("Issuer: %s\n", cert->issuerName);

  rv = SSL_AuthCertificate(arg, fd, check_sig, is_server);

  if(rv == SECSuccess)
    DEBUG_SSL("SSL certificate valid.\n");

  CERT_DestroyCertificate(cert);

  return rv;
}

static SECStatus my_ssl_verify_failed_callback(void *arg, PRFileDesc * fd)
{
  int err = PR_GetError();

  DEBUG_SSL("SSL certificate invalid (%d).\n", err);
  DEBUG_SSL("%s\n", SECU_Strerror(err));

  return cfg.nss_accept_unknown_cert ? SECSuccess : SECFailure;
}

void my_ssl_init(void)
{
}

void my_ssl_init_once(void)
{
  PR_Init(PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);
}

void my_ssl_init_start(void)
{
  SECStatus rv;
  char *certDir = priv_cfg.nss_cert_dir;
  static bool_t called = FALSE;

  if(called)
    NSS_Shutdown();
  else
    called = TRUE;

  if(!certDir)
  {
    /* Look in $SSL_DIR */
    certDir = SECU_DefaultSSLDir();
    /* call even if it's NULL */
    certDir = SECU_ConfigDirectory(certDir);
  }
  DEBUG_SSL("myssl_nss: config dir %s\n", certDir ? certDir : "(null)");

  if(priv_cfg.ssl_cert_passwd)
    PK11_SetPasswordFunc(my_ssl_passwd_callback);
  else
    PK11_SetPasswordFunc(SECU_GetModulePassword);

  rv = NSS_Init(certDir);
  if(rv != SECSuccess)
    xprintf(1, gettext("NSS_Init: Unable to open cert database"));

  if(cfg.nss_domestic_policy)
    NSS_SetDomesticPolicy();
  else
    NSS_SetExportPolicy();
}

void my_ssl_cleanup(void)
{
  NSS_Shutdown();
  PR_Cleanup();
}

void my_ssl_connection_close(ssl_connection * con)
{
  if(con->fd)
    PR_Close(con->fd);
  if(con->socket)
    bufio_close(con->socket);

  con->fd = NULL;
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
  con->fd = NULL;
  con->socket = NULL;

  return con;
}

static const struct
{
  int cipher;
  char *name;
} my_ssl_cipher_tab[] =
{
  {SSL_EN_RC4_128_WITH_MD5, "EN_RC4_128_WITH_MD5"},
  {SSL_EN_RC4_128_EXPORT40_WITH_MD5, "EN_RC4_128_EXPORT40_WITH_MD5"},
  {SSL_EN_RC2_128_CBC_WITH_MD5, "EN_RC2_128_CBC_WITH_MD5"},
  {SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5, "EN_RC2_128_CBC_EXPORT40_WITH_MD5"},
  {SSL_EN_DES_64_CBC_WITH_MD5, "EN_DES_64_CBC_WITH_MD5"},
  {SSL_EN_DES_192_EDE3_CBC_WITH_MD5, "EN_DES_192_EDE3_CBC_WITH_MD5"},
  {SSL_NULL_WITH_NULL_NULL, "NULL_WITH_NULL_NULL"},
  {SSL_RSA_WITH_NULL_MD5, "RSA_WITH_NULL_MD5"},
  {SSL_RSA_WITH_NULL_SHA, "RSA_WITH_NULL_SHA"},
  {SSL_RSA_EXPORT_WITH_RC4_40_MD5, "RSA_EXPORT_WITH_RC4_40_MD5"},
  {SSL_RSA_WITH_RC4_128_MD5, "RSA_WITH_RC4_128_MD5"},
  {SSL_RSA_WITH_RC4_128_SHA, "RSA_WITH_RC4_128_SHA"},
  {SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5, "RSA_EXPORT_WITH_RC2_CBC_40_MD5"},
  {SSL_RSA_WITH_IDEA_CBC_SHA, "RSA_WITH_IDEA_CBC_SHA"},
  {SSL_RSA_EXPORT_WITH_DES40_CBC_SHA, "RSA_EXPORT_WITH_DES40_CBC_SHA"},
  {SSL_RSA_WITH_DES_CBC_SHA, "RSA_WITH_DES_CBC_SHA"},
  {SSL_RSA_WITH_3DES_EDE_CBC_SHA, "RSA_WITH_3DES_EDE_CBC_SHA"},
  {SSL_DH_DSS_EXPORT_WITH_DES40_CBC_SHA, "DH_DSS_EXPORT_WITH_DES40_CBC_SHA"},
  {SSL_DH_DSS_WITH_DES_CBC_SHA, "DH_DSS_WITH_DES_CBC_SHA"},
  {SSL_DH_DSS_WITH_3DES_EDE_CBC_SHA, "DH_DSS_WITH_3DES_EDE_CBC_SHA"},
  {SSL_DH_RSA_EXPORT_WITH_DES40_CBC_SHA, "DH_RSA_EXPORT_WITH_DES40_CBC_SHA"},
  {SSL_DH_RSA_WITH_DES_CBC_SHA, "DH_RSA_WITH_DES_CBC_SHA"},
  {SSL_DH_RSA_WITH_3DES_EDE_CBC_SHA, "DH_RSA_WITH_3DES_EDE_CBC_SHA"},
  {SSL_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA, "DHE_DSS_EXPORT_WITH_DES40_CBC_SHA"},
  {SSL_DHE_DSS_WITH_DES_CBC_SHA, "DHE_DSS_WITH_DES_CBC_SHA"},
  {SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA, "DHE_DSS_WITH_3DES_EDE_CBC_SHA"},
  {SSL_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA, "DHE_RSA_EXPORT_WITH_DES40_CBC_SHA"},
  {SSL_DHE_RSA_WITH_DES_CBC_SHA, "DHE_RSA_WITH_DES_CBC_SHA"},
  {SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA, "DHE_RSA_WITH_3DES_EDE_CBC_SHA"},
  {SSL_DH_ANON_EXPORT_WITH_RC4_40_MD5, "DH_ANON_EXPORT_WITH_RC4_40_MD5"},
  {SSL_DH_ANON_WITH_RC4_128_MD5, "DH_ANON_WITH_RC4_128_MD5"},
  {SSL_DH_ANON_EXPORT_WITH_DES40_CBC_SHA, "DH_ANON_EXPORT_WITH_DES40_CBC_SHA"},
  {SSL_DH_ANON_WITH_DES_CBC_SHA, "DH_ANON_WITH_DES_CBC_SHA"},
  {SSL_DH_ANON_WITH_3DES_EDE_CBC_SHA, "DH_ANON_WITH_3DES_EDE_CBC_SHA"},
  {SSL_FORTEZZA_DMS_WITH_NULL_SHA, "FORTEZZA_DMS_WITH_NULL_SHA"},
  {SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA,
      "FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA"},
  {SSL_FORTEZZA_DMS_WITH_RC4_128_SHA, "FORTEZZA_DMS_WITH_RC4_128_SHA"},
  {TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA, "RSA_EXPORT1024_WITH_DES_CBC_SHA"},
  {TLS_RSA_EXPORT1024_WITH_RC4_56_SHA, "RSA_EXPORT1024_WITH_RC4_56_SHA"},
  {TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA,
      "DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA"},
  {TLS_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA,
      "DHE_DSS_EXPORT1024_WITH_RC4_56_SHA"},
  {TLS_DHE_DSS_WITH_RC4_128_SHA, "DHE_DSS_WITH_RC4_128_SHA"},
  {SSL_RSA_OLDFIPS_WITH_3DES_EDE_CBC_SHA, "RSA_OLDFIPS_WITH_3DES_EDE_CBC_SHA"},
  {SSL_RSA_OLDFIPS_WITH_DES_CBC_SHA, "RSA_OLDFIPS_WITH_DES_CBC_SHA"},
  {SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA, "RSA_FIPS_WITH_3DES_EDE_CBC_SHA"},
  {SSL_RSA_FIPS_WITH_DES_CBC_SHA, "RSA_FIPS_WITH_DES_CBC_SHA"}
};

static int my_ssl_get_cipher_by_name(char *n)
{
  int i;

  for(i = 0; i < NUM_ELEM(my_ssl_cipher_tab); i++)
  {
    if(!strcasecmp(n, my_ssl_cipher_tab[i].name))
      return my_ssl_cipher_tab[i].cipher;
  }
  DEBUG_SSL("unknown NSS3 SSL cipher %s\n", n);
  return -1;
}

static void my_ssl_set_ciphers(PRFileDesc * fd, char *ciphers)
{
  char **c;
  int cipher, i;

  if(!strcasecmp(ciphers, "all"))
    return;

  c = tl_str_split(ciphers, ",");

  /* at first disable all ciphers */
  for(i = 0; i < NUM_ELEM(my_ssl_cipher_tab); i++)
  {
    cipher = my_ssl_cipher_tab[i].cipher;
    if(fd)
      SSL_CipherPrefSet(fd, cipher, SSL_NOT_ALLOWED);
  }

  for(i = 0; c && c[i]; i++)
  {
    cipher = my_ssl_get_cipher_by_name(c[i]);
    if(cipher != -1 && fd)
      SSL_CipherPrefSet(fd, cipher, SSL_ALLOWED);
  }

  tl_strv_free(c);
}

bufio *my_ssl_do_connect(doc * docp, bufio * socket,
  ssl_connection * parent_ssl_con)
{
  int rv;
  ssl_connection *con;
  PRFileDesc *model;
  CERTCertDBHandle *handle;

  my_ssl_init();

  if(priv_cfg.ssl_proxy)
  {
    if(http_dumy_proxy_connect(docp,
        url_get_site(docp->doc_url), url_get_port(docp->doc_url),
        priv_cfg.ssl_proxy, cfg.ssl_proxy_port))
    {
      docp->errcode = ERR_PROXY_CONNECT;
      return NULL;
    }
  }

  con = my_ssl_connection_new();

  model = myssl_bufio_new_socket(socket);
  /* model = PR_ImportTCPSocket(bufio_getfd(socket)); */

  con->fd = SSL_ImportFD(NULL, model);
  SSL_OptionSet(con->fd, SSL_SECURITY, PR_TRUE);
  SSL_OptionSet(con->fd, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE);

  switch (cfg.ssl_version)
  {
  case 1:
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL2, TRUE);
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL3, FALSE);
    SSL_OptionSet(con->fd, SSL_ENABLE_TLS, FALSE);
    SSL_OptionSet(con->fd, SSL_V2_COMPATIBLE_HELLO, TRUE);
    break;
  case 2:
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL2, TRUE);
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL3, TRUE);
    SSL_OptionSet(con->fd, SSL_ENABLE_TLS, FALSE);
    SSL_OptionSet(con->fd, SSL_V2_COMPATIBLE_HELLO, TRUE);
    break;
  case 3:
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL2, FALSE);
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL3, TRUE);
    SSL_OptionSet(con->fd, SSL_ENABLE_TLS, FALSE);
    SSL_OptionSet(con->fd, SSL_V2_COMPATIBLE_HELLO, FALSE);
    break;
  case 4:
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL2, FALSE);
    SSL_OptionSet(con->fd, SSL_ENABLE_SSL3, FALSE);
    SSL_OptionSet(con->fd, SSL_ENABLE_TLS, TRUE);
    SSL_OptionSet(con->fd, SSL_V2_COMPATIBLE_HELLO, FALSE);
    break;
  }
  SSL_OptionSet(con->fd, SSL_NO_CACHE, !cfg.unique_sslid);

  if(priv_cfg.ssl_cipher_list)
    my_ssl_set_ciphers(con->fd, priv_cfg.ssl_cipher_list);

  if(priv_cfg.ssl_cert_passwd)
    SSL_SetPKCS11PinArg(con->fd, priv_cfg.ssl_cert_passwd);

  handle = CERT_GetDefaultCertDB();

  SSL_AuthCertificateHook(con->fd, my_ssl_verify_callback, (void *) handle);
  SSL_GetClientAuthDataHook(con->fd, NSS_GetClientAuthData, (void *) NULL);
  SSL_BadCertHook(con->fd, my_ssl_verify_failed_callback, (void *) NULL);
  SSL_SetURL(con->fd, url_get_site(docp->doc_url));
  SSL_SetSockPeerID(con->fd, url_get_site(docp->doc_url));

  rv = SSL_ResetHandshake(con->fd, 0);
  rv = SSL_ForceHandshake(con->fd);

  if(rv != SECSuccess)
  {
    const char *errstr;
    errstr = SECU_Strerror(PR_GetError());
    xprintf(1, gettext("SSL connect failure - %s\n"), errstr);
    my_ssl_connection_destroy(con);

    return NULL;
  }

  con->socket = socket;
  return bufio_new_sslcon(socket, con);
}

int my_ssl_read(ssl_connection * ssl_con, char *buf, size_t len)
{
  return PR_Read(ssl_con->fd, buf, len);
}

int my_ssl_write(ssl_connection * ssl_con, char *buf, size_t len)
{
  return PR_Write(ssl_con->fd, buf, len);
}

int my_ssl_data_pending(ssl_connection * ssl_con)
{
  return SSL_DataPending(ssl_con->fd);
}

void my_ssl_print_last_error(void)
{
  char *errstr = SECU_Strerror(PR_GetError());
  xprintf(1, gettext("SSL error - %s\n"), errstr);
}

#endif
