/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

/****************************************************************/
/* based on the document "NTLM Athentication Scheme for HTTP"   */
/* http://www.innovation.ch/java/ntlm.html                      */
/* by Ronald Tschaler <ronald@innovation.ch>                    */
/****************************************************************/
/* and http://www.opengroup.org/comsource/techref2/NCH1222X.HTM */
/****************************************************************/

#ifdef ENABLE_NTLM

#include <string.h>

#include "base64.h"
#include "tools.h"
#include "mime.h"
#include "doc.h"
#include "http.h"
#include "tools.h"
#include "abstract.h"
#include "authinfo.h"
#include "errcode.h"
#include "ntlm_auth.h"

#ifdef HAVE_MCRYPT
#include <mcrypt.h>
#elif defined(HAVE_GCRYPT)
#include <gcrypt.h>
#else
#ifdef OPENSSL
#include <openssl/des.h>
#else
#include <des.h>
#endif
#endif

#ifdef HAVE_OPENSSL_MD4
#include <openssl/md4.h>

#define _MD4Init  MD4_Init
#define _MD4Update  MD4_Update
#define _MD4Final MD4_Final

#else
#include "md4c.h"

#define _MD4Init  MD4Init
#define _MD4Update  MD4Update
#define _MD4Final MD4Final
#endif

#ifdef WORDS_BIGENDIAN
static unsigned short ntlm_do_litle_endian_short(short v)
{
  short sh;

  ((char *) &sh)[0] = v && 0xFF;
  ((char *) &sh)[1] = v >> 8;

  return sh;
}

static unsigned short ntlm_get_litle_endian_short(short v)
{
  short sh;

  sh = ((char *) &v)[0] + (((char *) &v)[1] << 8);

  return sh;
}
#else
#define ntlm_do_litle_endian_short(v) ((unsigned short)(v))
#define ntlm_get_litle_endian_short(v) ((unsigned short)(v))
#endif

#define _LB(nr) ((nr) & 0xff)
#define _HB(nr) (((nr) & 0xff00) >> 8)

char *ntlm_get_t1msg_str(char *domain, char *host)
{
  int msgtk_size;
  int dlen, hlen;
#ifndef NTLM_UNPACKED_STRUCT
  ntlm_type_1_msg_t msg;
#else
  unsigned char msg[NTLM_MSG1_SIZE];
#endif

  dlen = strlen(domain);
  hlen = strlen(host);

  memset(&msg, '\0', sizeof(msg));

#ifndef NTLM_UNPACKED_STRUCT
  strcpy(msg.protocol, "NTLMSSP");
  msg.type = 0x01;
  msg.flags = ntlm_do_litle_endian_short(0xb203);
  msg.dom_len2 = msg.dom_len = ntlm_do_litle_endian_short(dlen);
  msg.host_len2 = msg.host_len = ntlm_do_litle_endian_short(hlen);
  msg.host_off = ntlm_do_litle_endian_short(32);
  msg.dom_off = ntlm_do_litle_endian_short(32 + hlen);
#else
  strcpy(msg, "NTLMSSP");
  msg[8] = 0x01;
  msg[12] = 0x03;
  msg[13] = 0xb2;
  msg[16] = msg[18] = _LB(dlen);
  msg[17] = msg[19] = _HB(dlen);
  msg[20] = _LB(32 + hlen);
  msg[21] = _HB(32 + hlen);
  msg[24] = msg[26] = _LB(hlen);
  msg[25] = msg[27] = _HB(hlen);
  msg[28] = 32;
  msg[29] = 0x0;
#endif

  strcpy(((char *) &msg) + 32, host);
  upperstr(((char *) &msg) + 32);
  strcpy(((char *) &msg) + 32 + hlen, domain);
  upperstr(((char *) &msg) + 32 + hlen);

  msgtk_size = 32 + hlen + dlen;

  return base64_encode_data((char *) &msg, msgtk_size);
}

char *ntlm_get_nonce(char *t2msg, unsigned short *t2flags)
{
  char *rv;
  int st;
#ifndef NTLM_UNPACKED_STRUCT
  ntlm_type_2_msg_t *msg;
#else
  char *msg;
#endif

  msg = NULL;
  st = base64_decode_data(t2msg, (char **) &msg);

#ifndef NTLM_UNPACKED_STRUCT
  if(st < 0 || st < sizeof(ntlm_type_2_msg_t) ||
    strcmp(msg->protocol, "NTLMSSP") || msg->type != 0x02)
  {
    rv = NULL;
  }
  else
  {
    rv = _malloc(9);
    memcpy(rv, msg->nonce, 8);
    rv[8] = '\0';
    *t2flags = ntlm_do_litle_endian_short(msg->flags);
  }
#else
  if(st < 0 || st < NTLM_MSG2_SIZE ||
    strcmp(msg, "NTLMSSP") || msg[8] != 0x02)
  {
    rv = NULL;
  }
  else
  {
    rv = _malloc(9);
    memcpy(rv, msg + 24, 8);
    rv[8] = '\0';
    *t2flags =
      ntlm_do_litle_endian_short(*((unsigned short *) (((char *) msg) + 20)));
  }
#endif
  _free(msg);

  return rv;
}

static void ntlm_copy_str_to_wcstr(char *wcstr, char *str)
{
  while(*str)
  {
    wcstr[0] = *str;
    wcstr[1] = '\0';

    wcstr += 2;
    str++;
  }
}

static void ntlm_copy_str_to_uwcstr(char *wcstr, char *str)
{
  while(*str)
  {
    wcstr[0] = tl_ascii_toupper(*str);
    wcstr[1] = '\0';

    wcstr += 2;
    str++;
  }
}

/* turns a 56 bit key into the 64 bit */
static void ntlm_setup_des_key(unsigned char *key_56, unsigned char *key_64)
{
  key_64[0] = key_56[0];
  key_64[1] = ((key_56[0] << 7) & 0xFF) | (key_56[1] >> 1);
  key_64[2] = ((key_56[1] << 6) & 0xFF) | (key_56[2] >> 2);
  key_64[3] = ((key_56[2] << 5) & 0xFF) | (key_56[3] >> 3);
  key_64[4] = ((key_56[3] << 4) & 0xFF) | (key_56[4] >> 4);
  key_64[5] = ((key_56[4] << 3) & 0xFF) | (key_56[5] >> 5);
  key_64[6] = ((key_56[5] << 2) & 0xFF) | (key_56[6] >> 6);
  key_64[7] = (key_56[6] << 1) & 0xFF;
}

#ifdef HAVE_MCRYPT
static void ntlm_des_enc(unsigned char *key, unsigned char *plaintext,
  unsigned char *result)
{
  MCRYPT ctx;
  unsigned char res[8];
  unsigned char key64[8];

  ntlm_setup_des_key(key, key64);
  memcpy(res, plaintext, 8);
  ctx = mcrypt_module_open(MCRYPT_DES, NULL, MCRYPT_ECB, NULL);
  mcrypt_generic_init(ctx, key64, 8, NULL);
  mcrypt_generic(ctx, res, 8);
  memcpy(result, res, 8);
  mcrypt_generic_end(ctx);
  mcrypt_module_close(ctx);
}

#elif defined(HAVE_GCRYPT)
static void ntlm_des_enc(unsigned char *key, unsigned char *plaintext,
  unsigned char *result)
{
  gcry_cipher_hd_t ctx;
  unsigned char key64[8];

  ntlm_setup_des_key(key, key64);
  gcry_cipher_open(&ctx, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
  gcry_cipher_setkey(ctx, key64, 8);
  gcry_cipher_encrypt(ctx, result, 8, plaintext, 8);
  gcry_cipher_close(ctx);
}
#else
static void ntlm_des_enc(unsigned char *key, unsigned char *plaintext,
  unsigned char *result)
{
  des_key_schedule ks;
#ifdef OPENSSL
  const_des_cblock _plaintext;
#else
  const des_cblock _plaintext;
#endif
  des_cblock _result;
  des_cblock key64;
  unsigned char skey[8];

  memcpy(&_plaintext, plaintext, sizeof(_plaintext));

  ntlm_setup_des_key(key, skey);
  memcpy(key64, skey, 8);

#ifdef OPENSSL
  des_set_odd_parity(&key64);
  des_set_key(&key64, ks);

  des_ecb_encrypt(&_plaintext, &_result, ks, DES_ENCRYPT);
#else
  des_set_odd_parity(key64);
  des_set_key(key64, ks);

  des_ecb_encrypt(_plaintext, _result, ks, DES_ENCRYPT);
#endif
  memcpy(result, &_result, sizeof(_result));
}
#endif

/*
 * takes a 21 byte array and treats it as 3 56-bit DES keys. The
 * 8 byte plaintext is encrypted with each key and the resulting 24
 * bytes are stored in the results array.
 */
static void ntlm_calc_resp(unsigned char *keys, unsigned char *plaintext,
  unsigned char *results)
{
  ntlm_des_enc(keys, plaintext, results);
  ntlm_des_enc(keys + 7, plaintext, results + 8);
  ntlm_des_enc(keys + 14, plaintext, results + 16);
}

static void ntlm_gen_lm_resp(char *pass, char *nonce, char *buf)
{
  unsigned char magic[] = { 0x4B, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25 };
  unsigned char lm_hpw[21];
  unsigned char lm_pw[14];

  memset(lm_pw, '\0', sizeof(lm_pw));
  strncpy(lm_pw, pass, 14);
  lm_pw[13] = '\0';
  upperstr(lm_pw);

  ntlm_des_enc(lm_pw, magic, lm_hpw);
  ntlm_des_enc(lm_pw + 7, magic, lm_hpw + 8);
  memset(lm_hpw + 16, '\0', 5);

  ntlm_calc_resp(lm_hpw, nonce, buf);
}

static void ntlm_gen_nt_resp(char *pass, char *nonce, char *buf)
{
  MD4_CTX md4ctx;
  char *nt_pw;
  unsigned char nt_hpw[21];
  int len = strlen(pass);

  nt_pw = _malloc(2 * len);
  ntlm_copy_str_to_wcstr(nt_pw, pass);

  _MD4Init(&md4ctx);
  _MD4Update(&md4ctx, nt_pw, 2 * len);
  _MD4Final(nt_hpw, &md4ctx);

  memset(nt_hpw + 16, '\0', 5);

  ntlm_calc_resp(nt_hpw, nonce, buf);

  _free(nt_pw);
}

static char *ntlm_get_t3msg_wcstr(char *domain, char *host, char *user,
  char *pass, char *nonce)
{
  int dlen, hlen, ulen, lm_len, nt_len;
  int dofs, hofs, uofs, lm_ofs, nt_ofs;
  int msgtk_size;
#ifndef NTLM_UNPACKED_STRUCT
  ntlm_type_3_msg_t msg;
#else
  char msg[NTLM_MSG3_SIZE];
#endif

  dlen = strlen(domain);
  hlen = strlen(host);
  ulen = strlen(user);
  lm_len = 0x18;
  nt_len = 0x18;

  dofs = 64;
  uofs = dofs + 2 * dlen;
  hofs = uofs + 2 * ulen;
  lm_ofs = hofs + 2 * hlen;
  nt_ofs = lm_ofs + lm_len;

  msgtk_size = 64 + 2 * dlen + 2 * ulen + 2 * hlen + lm_len + nt_len;

  memset(&msg, '\0', sizeof(msg));

#ifndef NTLM_UNPACKED_STRUCT
  strcpy(msg.protocol, "NTLMSSP");
  msg.type = 0x03;
  msg.flags = 0x8201;

  msg.dom_len = msg.dom_len2 = ntlm_do_litle_endian_short(2 * dlen);
  msg.dom_off = ntlm_do_litle_endian_short(dofs);
  msg.user_len = msg.user_len2 = ntlm_do_litle_endian_short(2 * ulen);
  msg.user_off = ntlm_do_litle_endian_short(uofs);
  msg.host_len = msg.host_len2 = ntlm_do_litle_endian_short(2 * hlen);
  msg.host_off = ntlm_do_litle_endian_short(hofs);

  msg.lm_resp_len = msg.lm_resp_len2 = ntlm_do_litle_endian_short(lm_len);
  msg.lm_resp_off = ntlm_do_litle_endian_short(lm_ofs);
  msg.nt_resp_len = msg.nt_resp_len2 = ntlm_do_litle_endian_short(nt_len);
  msg.nt_resp_off = ntlm_do_litle_endian_short(nt_ofs);

  msg.msg_len = ntlm_do_litle_endian_short(msgtk_size);
#else
  strcpy(msg, "NTLMSSP");
  msg[8] = 0x3;
  msg[12] = msg[14] = _LB(lm_len);
  msg[13] = msg[15] = _HB(lm_len);
  msg[16] = _LB(lm_ofs);
  msg[17] = _HB(lm_ofs);
  msg[20] = msg[22] = _LB(nt_len);
  msg[21] = msg[23] = _HB(nt_len);
  msg[24] = _LB(nt_ofs);
  msg[25] = _HB(nt_ofs);
  msg[28] = msg[30] = _LB(2 * dlen);
  msg[29] = msg[31] = _HB(2 * dlen);
  msg[32] = _LB(dofs);
  msg[33] = _HB(dofs);
  msg[36] = msg[38] = _LB(2 * ulen);
  msg[37] = msg[39] = _HB(2 * ulen);
  msg[40] = _LB(uofs);
  msg[41] = _HB(uofs);
  msg[44] = msg[46] = _LB(2 * hlen);
  msg[45] = msg[47] = _HB(2 * hlen);
  msg[48] = _LB(hofs);
  msg[49] = _HB(hofs);
  msg[56] = _LB(msgtk_size);
  msg[57] = _HB(msgtk_size);
  msg[60] = 0x1;
  msg[61] = 0x82;
#endif

  ntlm_copy_str_to_uwcstr(((char *) &msg) + dofs, domain);
  ntlm_copy_str_to_wcstr(((char *) &msg) + uofs, user);
  ntlm_copy_str_to_uwcstr(((char *) &msg) + hofs, host);

  ntlm_gen_lm_resp(pass, nonce, ((char *) &msg) + lm_ofs);
  ntlm_gen_nt_resp(pass, nonce, ((char *) &msg) + nt_ofs);

  return base64_encode_data((char *) &msg, msgtk_size);
}

static char *ntlm_get_t3msg_asciistr(char *domain, char *host, char *user,
  char *pass, char *nonce)
{
  int dlen, hlen, ulen, lm_len, nt_len;
  int dofs, hofs, uofs, lm_ofs, nt_ofs;
  int msgtk_size;
#ifndef NTLM_UNPACKED_STRUCT
  ntlm_type_3_msg_t msg;
#else
  char msg[NTLM_MSG3_SIZE];
#endif

  dlen = strlen(domain);
  hlen = strlen(host);
  ulen = strlen(user);
  lm_len = 0x18;
  nt_len = 0x18;

  dofs = 64;
  uofs = dofs + dlen;
  hofs = uofs + ulen;
  lm_ofs = hofs + hlen;
  nt_ofs = lm_ofs + lm_len;

  msgtk_size = 64 + dlen + ulen + hlen + lm_len + nt_len;

  memset(&msg, '\0', sizeof(msg));

#ifndef NTLM_UNPACKED_STRUCT
  strcpy(msg.protocol, "NTLMSSP");
  msg.type = 0x03;
  msg.flags = 0x8201;

  msg.dom_len = msg.dom_len2 = ntlm_do_litle_endian_short(dlen);
  msg.dom_off = ntlm_do_litle_endian_short(dofs);
  msg.user_len = msg.user_len2 = ntlm_do_litle_endian_short(ulen);
  msg.user_off = ntlm_do_litle_endian_short(uofs);
  msg.host_len = msg.host_len2 = ntlm_do_litle_endian_short(hlen);
  msg.host_off = ntlm_do_litle_endian_short(hofs);

  msg.lm_resp_len = msg.lm_resp_len2 = ntlm_do_litle_endian_short(lm_len);
  msg.lm_resp_off = ntlm_do_litle_endian_short(lm_ofs);
  msg.nt_resp_len = msg.nt_resp_len2 = ntlm_do_litle_endian_short(nt_len);
  msg.nt_resp_off = ntlm_do_litle_endian_short(nt_ofs);

  msg.msg_len = ntlm_do_litle_endian_short(msgtk_size);
#else
  strcpy(msg, "NTLMSSP");
  msg[8] = 0x3;
  msg[12] = msg[14] = _LB(lm_len);
  msg[13] = msg[15] = _HB(lm_len);
  msg[16] = _LB(lm_ofs);
  msg[17] = _HB(lm_ofs);
  msg[20] = msg[22] = _LB(nt_len);
  msg[21] = msg[23] = _HB(nt_len);
  msg[24] = _LB(nt_ofs);
  msg[25] = _HB(nt_ofs);
  msg[28] = msg[30] = _LB(dlen);
  msg[29] = msg[31] = _HB(dlen);
  msg[32] = _LB(dofs);
  msg[33] = _HB(dofs);
  msg[36] = msg[38] = _LB(ulen);
  msg[37] = msg[39] = _HB(ulen);
  msg[40] = _LB(uofs);
  msg[41] = _HB(uofs);
  msg[44] = msg[46] = _LB(hlen);
  msg[45] = msg[47] = _HB(hlen);
  msg[48] = _LB(hofs);
  msg[49] = _HB(hofs);
  msg[56] = _LB(msgtk_size);
  msg[57] = _HB(msgtk_size);
  msg[60] = 0x1;
  msg[61] = 0x82;
#endif

  strcpy(((char *) &msg) + dofs, domain);
  upperstr(((char *) &msg) + dofs);
  strcpy(((char *) &msg) + uofs, user);
  strcpy(((char *) &msg) + hofs, host);
  upperstr(((char *) &msg) + hofs);

  ntlm_gen_lm_resp(pass, nonce, ((char *) &msg) + lm_ofs);
  ntlm_gen_nt_resp(pass, nonce, ((char *) &msg) + nt_ofs);

  return base64_encode_data((char *) &msg, msgtk_size);
}

char *ntlm_get_t3msg_str(char *domain, char *host, char *user, char *pass,
  char *nonce, unsigned short t2flags)
{
  return (t2flags & 0x01) ? ntlm_get_t3msg_wcstr(domain, host, user, pass,
    nonce) : ntlm_get_t3msg_asciistr(domain, host, user, pass, nonce);
}

/*
 * -1 - failure before sending auth data - http_process_response() can
 *      continue safely
 *  0 - OK - http_process_response() must return immediately
 *  1 - failure after sending auth data - http_process_response() must
 *      return immediately
 */
int ntlm_negotiate_connection(doc * docp, char *authtag)
{
  char *ntlm_msg, *nonce;
  unsigned short t2_flags;
  http_response *resp;
  char *user, *pass, *domain, *host;
  authinfo *ai;
  char shost[128];
  int l;

  if(docp->num_auth)
    return -1;

  docp->num_auth++;

  xprintf(1, gettext("Trying to do NTLM authorization\n"));

  /*** it seems we should allow the first response     ***/
  /*** to be sent via non persistent connection      ***/
#if 0
  /*** supported only when persistant connections are  ***/
  /*** supported by server and we are talking HTTP/1.1 ***/
  if(!docp->is_persistent)
  {
    xprintf(1,
      gettext
      ("NTLM authorization supported only on persistent connections!\n"));
    docp->errcode = ERR_HTTP_AUTH_NTLM;
    return -1;
  }
#endif

  /*** get authorization infos (all mandatory) ***/
  ai = authinfo_match_entry(docp->doc_url->type,
    url_get_site(docp->doc_url), url_get_port(docp->doc_url),
    url_get_path(docp->doc_url), NULL);
  domain = (ai && ai->ntlm_domain) ?
    ai->ntlm_domain : priv_cfg.auth_ntlm_domain;

  l = strcspn(cfg.local_host, ".");
  if (l > 127)
  {
    l = 127;
  }
  strncpy(shost, cfg.local_host, l);
  shost[l] = '\0';
  host = shost;

  user = url_get_user(docp->doc_url, NULL);
  pass = url_get_pass(docp->doc_url, NULL);

  if(!domain || !user || !pass || !host)
  {
    xprintf(1, gettext("Not enough data for NTLM authorization!\n"));
    xprintf(1, gettext("Missing:\n"));
    if(!domain)
      xprintf(1, gettext("       domain\n"));
    if(!user)
      xprintf(1, gettext("       username\n"));
    if(!pass)
      xprintf(1, gettext("       password\n"));
    if(!host)
      xprintf(1, gettext("       local hostname\n"));
    return -1;
  }

  /*** read body of 401 response ***/
  if(http_throw_message_body(docp))
  {
    docp->is_persistent = FALSE;
    abs_close_socket(docp, FALSE);
    return 1;
  }
  abs_close_socket(docp, FALSE);

  /*** prepare and send NTLM msg type 1 to ***/
  /*** start NTLM negotiation with server  ***/
  ntlm_msg = ntlm_get_t1msg_str(domain, host);

  docp->additional_headers = tl_str_concat(NULL, "Authorization: NTLM ",
    ntlm_msg, "\r\n", NULL);
  _free(ntlm_msg);
  _free(docp->mime);
  _free(docp->type_str);

  l = http_repeat_request(docp);

  _free(docp->additional_headers);

  if(l)
    return -1;

  /*** now we expect 401 server response with ***/
  /*** NTLM msg type 2 which contains nonce   ***/
  resp = http_get_response_info(docp->mime);

  if(!resp)
  {
    xprintf(1, gettext("Got unexpected response\n"));
    if(!docp->errcode)
      docp->errcode = ERR_HTTP_AUTH_NTLM;
    return -1;
  }

  if(resp->ret_code != 401)
  {
    xprintf(1, gettext("Got unexpected response\n"));
    docp->errcode = ERR_HTTP_AUTH_NTLM;
    http_response_free(resp);
    return -1;
  }
  http_response_free(resp);

  if(!docp->is_persistent)
  {
    xprintf(1,
      gettext
      ("NTLM authorization supported only on persistent connections!\n"));
    docp->errcode = ERR_HTTP_AUTH_NTLM;
    return -1;
  }

  if(http_throw_message_body(docp))
  {
    docp->is_persistent = FALSE;
    abs_close_socket(docp, FALSE);
    return 1;
  }

  authtag = get_mime_param_val_str("WWW-Authenticate: NTLM ", docp->mime);

  if(!authtag)
  {
    xprintf(1, gettext("Got unexpected response\n"));
    docp->errcode = ERR_HTTP_AUTH_NTLM;
    return -1;
  }

  nonce = ntlm_get_nonce(authtag, &t2_flags);
  _free(authtag);

  if(!nonce)
  {
    xprintf(1, gettext("Failed NTLM nonce negotiation\n"));
    docp->errcode = ERR_HTTP_AUTH_NTLM;
    return -1;
  }

  /*** final step of NTLM authorization - prepare    ***/
  /*** and send NTLM msg type 3 with all auth. infos ***/
  ntlm_msg = ntlm_get_t3msg_str(domain, host, user, pass, nonce, t2_flags);
  docp->additional_headers = tl_str_concat(NULL, "Authorization: NTLM ",
    ntlm_msg, "\r\n", NULL);
  _free(ntlm_msg);
  _free(nonce);
  _free(docp->mime);
  _free(docp->type_str);

  l = http_repeat_request(docp);

  _free(docp->additional_headers);

  if(l)
    return -1;

  return 0;
}

int ntlm_negotiate_proxy_connection(doc * docp, char *authtag)
{
  char *ntlm_msg, *nonce;
  unsigned short t2_flags;
  http_response *resp;
  char *user, *pass, *domain, *host;
  authinfo *ai;
  char shost[128];
  int l;

  if(docp->num_proxy_auth)
    return -1;

  if(!docp->http_proxy)
    return -1;

  xprintf(1, gettext("Trying to do proxy NTLM authorization\n"));

  docp->num_proxy_auth++;

  docp->errcode = ERR_HTTP_PROAUTH_NTLM;

  /*** it seems we should allow the first response     ***/
  /*** to be sent via non persistent connection      ***/
#if 0
  /*** supported only when persistant connections are  ***/
  /*** supported by server and we are talking HTTP/1.1 ***/
  if(!docp->is_persistent)
  {
    xprintf(1,
      gettext
      ("NTLM authorization supported only on persistent connections!\n"));
    return -1;
  }
#endif

  /*** get authorization infos (all mandatory) ***/
  ai = authinfo_match_entry(docp->doc_url->type,
    docp->http_proxy, docp->http_proxy_port, NULL, NULL);
  domain = (ai && ai->ntlm_domain) ?
    ai->ntlm_domain : priv_cfg.auth_proxy_ntlm_domain;

  l = strcspn(cfg.local_host, ".");
  if (l > 127)
  {
    l = 127;
  }
  strncpy(shost, cfg.local_host, l);
  shost[l] = '\0';
  host = shost;

  user = priv_cfg.http_proxy_user;
  pass = priv_cfg.http_proxy_pass;

  if(!domain || !user || !pass || !host)
  {
    xprintf(1, gettext("Not enough data for NTLM authorization!\n"));
    xprintf(1, gettext("Missing:\n"));
    if(!domain)
      xprintf(1, gettext("       domain\n"));
    if(!user)
      xprintf(1, gettext("       username\n"));
    if(!pass)
      xprintf(1, gettext("       password\n"));
    if(!host)
      xprintf(1, gettext("       local hostname\n"));
    return -1;
  }

  /*** read body of 407 response ***/
  if(http_throw_message_body(docp))
  {
    docp->is_persistent = FALSE;
    abs_close_socket(docp, FALSE);
    return 1;
  }
  abs_close_socket(docp, FALSE);

  /*** prepare and send NTLM msg type 1 to ***/
  /*** start NTLM negotiation with server  ***/
  ntlm_msg = ntlm_get_t1msg_str(domain, host);

  docp->additional_headers = tl_str_concat(NULL, "Proxy-Authorization: NTLM ",
    ntlm_msg, "\r\n", NULL);

  _free(ntlm_msg);
  _free(docp->mime);
  _free(docp->type_str);

  l = http_repeat_request(docp);
  _free(docp->additional_headers);

  if(l)
    return -1;

  /*** now we expect 407 server response with ***/
  /*** NTLM msg type 2 which contains nonce   ***/
  resp = http_get_response_info(docp->mime);

  if(!resp)
  {
    xprintf(1, gettext("Got unexpected response\n"));
    if(!docp->errcode)
      docp->errcode = ERR_HTTP_PROAUTH_NTLM;
    return -1;
  }

  if(resp->ret_code != 407)
  {
    xprintf(1, gettext("Got unexpected response\n"));
    docp->errcode = ERR_HTTP_PROAUTH_NTLM;
    http_response_free(resp);
    return -1;
  }
  http_response_free(resp);

  if(!docp->is_persistent)
  {
    xprintf(1,
      gettext
      ("NTLM authorization supported only on persistent connections!\n"));
    docp->errcode = ERR_HTTP_PROAUTH_NTLM;
    return -1;
  }

  if(http_throw_message_body(docp))
  {
    docp->is_persistent = FALSE;
    abs_close_socket(docp, FALSE);
    return 1;
  }

  authtag = get_mime_param_val_str("Proxy-Authenticate: NTLM ", docp->mime);

  if(!authtag)
  {
    xprintf(1, gettext("Got unexpected response\n"));
    docp->errcode = ERR_HTTP_PROAUTH_NTLM;
    return -1;
  }

  nonce = ntlm_get_nonce(authtag, &t2_flags);
  _free(authtag);

  if(!nonce)
  {
    xprintf(1, gettext("Failed NTLM proxy nonce negotiation\n"));
    docp->errcode = ERR_HTTP_PROAUTH_NTLM;
    return -1;
  }

  /*** final step of NTLM authorization - prepare    ***/
  /*** and send NTLM msg type 3 with all auth. infos ***/
  ntlm_msg = ntlm_get_t3msg_str(domain, host, user, pass, nonce, t2_flags);
  docp->additional_headers = tl_str_concat(NULL, "Proxy-Authorization: NTLM ",
    ntlm_msg, "\r\n", NULL);
  _free(ntlm_msg);
  _free(nonce);
  _free(docp->mime);
  _free(docp->type_str);

  l = http_repeat_request(docp);

  if(l)
    return -1;

  return 0;
}
#endif /* ENABLE_NTLM */
