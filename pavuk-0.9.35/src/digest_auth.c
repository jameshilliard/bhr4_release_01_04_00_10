/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <string.h>
#include <stdio.h>

#include "config.h"

#include "url.h"
#include "http.h"
#include "base64.h"
#include "tools.h"
#include "errcode.h"
#include "authinfo.h"
#include "html.h"
#include "abstract.h"
#include "myssl.h"

#if defined(USE_SSL) && defined(USE_SSL_IMPL_OPENSSL)

#define _MD5_CTX  MD5_CTX
#define _MD5Init  MD5_Init
#define _MD5Update  MD5_Update
#define _MD5Final MD5_Final

#else

#include "md5c.h"

#endif

/* this function is stolen from apache md5_util.c by Jeff Hostetler */
char *_md5(unsigned char *data)
{
  _MD5_CTX md5ctx;
  unsigned char md5s[16];
  unsigned char result[33];
  unsigned char pom[3];
  int i;

  _MD5Init(&md5ctx);
  _MD5Update(&md5ctx, data, strlen(data));
  _MD5Final(md5s, &md5ctx);

  result[0] = '\0';
  for(i = 0; i < 16; i++)
  {
    sprintf(pom, "%02x", md5s[i]);
    strcat(result, pom);
  }

  return tl_strdup(result);
}

void http_digest_deep_free(http_digest_info * digest)
{
  _free(digest->nonce);
  _free(digest->opaque);
  _free(digest->realm);
  _free(digest->site);
  _free(digest);
}

http_digest_info *http_digest_parse(char *authtag)
{
  http_digest_info *retv = NULL;

  if(authtag)
  {
    if(!strncmp(authtag, "Digest ", 7))
    {
      retv = _malloc(sizeof(http_digest_info));
      retv->nonce = html_get_attrib_from_tag(authtag, "nonce");
      retv->opaque = html_get_attrib_from_tag(authtag, "opague");
      retv->realm = html_get_attrib_from_tag(authtag, "realm");
      retv->site = NULL;
      retv->port = 0;
    }
  }

  return retv;
}

char *http_get_digest_auth_str(http_digest_info * auth_digest, char *method,
  char *user, char *pass, url * urlp, char *buf, size_t size)
{
  char pom[1024];
  char *a1, *a2, *a3;
  char *d = url_encode_str(urlp->p.http.document, URL_PATH_UNSAFE);
  size_t used;

  snprintf(pom, sizeof(pom), "%s:%s:%s", user, auth_digest->realm, pass);
  a1 = _md5(pom);
  snprintf(pom, sizeof(pom), "%s:%s", method, d);
  a2 = _md5(pom);
  snprintf(pom, sizeof(pom), "%s:%s:%s", a1, auth_digest->nonce, a2);
  a3 = _md5(pom);

  snprintf(buf, size,
    "Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"",
    user, auth_digest->realm, auth_digest->nonce, d, a3);

  if(auth_digest->opaque)
  {
    used = strlen(buf);
    snprintf(buf + used, size - used, ", opaque=\"%s\"", auth_digest->opaque);
  }
  _free(d);
  _free(a1);
  _free(a2);
  _free(a3);

  return buf;
}

/*
 * -1 - failure before sending auth data - http_process_response() can
 *      continue safely
 *  0 - OK - http_process_response() must return immediatly
 *  1 - failure after sending auth data - http_process_response() must
 *  return immediatly
 */
int http_digest_do_auth(doc * docu, char *authtag)
{
  int rv = 0;
  http_digest_info *digest;

  /*** clean old digest info, we are sure   ***/
  /*** going to do negotiation of new nonce ***/
  if(docu->auth_digest)
  {
    http_digest_deep_free((http_digest_info *) docu->auth_digest);
    docu->auth_digest = NULL;
  }

  /*** to prevent looping here ***/
  if(docu->num_auth > (1 + (cfg.auth_reuse_nonce ? 1 : 0)))
    return -1;

  docu->num_auth++;

  xprintf(1, gettext("Trying to do HTTP Digest authorization\n"));

  digest = http_digest_parse(authtag);
  digest->site = tl_strdup(url_get_site(docu->doc_url));
  digest->port = url_get_port(docu->doc_url);

  docu->auth_digest = digest;

  /*** read body of the 401 response ***/
  rv = http_throw_message_body(docu);
  if(rv)
    docu->is_persistent = FALSE;

  /*** to cleanly support also HTTP/1.0 we need to   ***/
  /*** close connection which must not be persistant ***/
  abs_close_socket(docu, FALSE);
  _free(docu->mime);
  _free(docu->type_str);

  if(rv)
    return 1;

  /*** send new request with new authenthication info ***/
  if(http_repeat_request(docu))
    rv = 1;

  return rv;
}

int http_digest_do_proxy_auth(doc * docu, char *authtag)
{
  int rv = 0;
  http_digest_info *digest;

  if(docu->auth_proxy_digest)
  {
    http_digest_deep_free((http_digest_info *) docu->auth_proxy_digest);
    docu->auth_proxy_digest = NULL;
  }

  if(docu->num_proxy_auth > (1 + (cfg.auth_reuse_proxy_nonce ? 1 : 0)))
    return -1;

  docu->num_proxy_auth++;

  xprintf(1, gettext("Trying to do HTTP proxy Digest authorization\n"));

  digest = http_digest_parse(authtag);

  docu->auth_proxy_digest = digest;

  rv = http_throw_message_body(docu);
  if(rv)
    docu->is_persistent = FALSE;

  abs_close_socket(docu, FALSE);

  if(rv)
    return 1;

  _free(docu->mime);
  _free(docu->type_str);

  if(http_repeat_request(docu))
    rv = 1;

  return rv;
}
