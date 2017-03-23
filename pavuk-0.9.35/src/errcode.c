/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <stdio.h>

#include "config.h"
#include "errcode.h"


void report_error(doc * docp, char *str)
{
  if(cfg.rbreak)
  {
    xprintf(1, gettext("%s: user break\n"), str);
    return;
  }

  switch (docp->errcode)
  {
  case ERR_NOERROR:
    xprintf(1, gettext("%s: OK\n"), str);
    break;
  case ERR_STORE_DOC:
    xprintf(1, gettext("%s: ERROR: storing document\n"), str);
    break;
  case ERR_FILE_OPEN:
    xprintf(1, gettext("%s: ERROR: opening file\n"), str);
    break;
  case ERR_DIR_URL:
    xprintf(1,
      gettext
      ("%s: ERROR: URL pointing to local directory is not supported\n"), str);
    break;
  case ERR_UNKNOWN:
    xprintf(1, gettext("%s: ERROR: unknown\n"), str);
    break;
  case ERR_LOCKED:
    xprintf(1, gettext("%s: ERROR: document is locked\n"), str);
    break;
  case ERR_READ:
    xprintf(1, gettext("%s: ERROR: reading socket\n"), str);
    break;
  case ERR_BIGGER:
    xprintf(1, gettext("%s: MESSAGE: bigger than maximal allowed size\n"),
      str);
    break;
  case ERR_SCRIPT_DISABLED:
    xprintf(1,
      gettext("%s: MESSAGE: disabled by user-exit script condition\n"), str);
    break;
  case ERR_SMALLER:
    xprintf(1, gettext("%s: MESSAGE: smaller than minimal allowed size\n"),
      str);
    break;
  case ERR_NOMIMET:
    xprintf(1, gettext("%s: MESSAGE: this mime type is not allowed (%s)\n"),
      str, docp->type_str ? docp->type_str : "unknown");
    break;
  case ERR_PROXY_CONNECT:
    xprintf(1, gettext("%s: ERROR: error in proxy connect\n"), str);
    break;
  case ERR_BREAK:
    xprintf(1, gettext("%s: ERROR: transfer broken by user\n"), str);
    break;
  case ERR_OUTTIME:
    xprintf(1,
      gettext
      ("%s: MESSAGE: file modification time doesn't fit to specified interval\n"),
      str);
    break;
  case ERR_ZERO_SIZE:
    xprintf(1, gettext("%s: ERROR: file has zero size - possible error\n"),
      str);
    break;
  case ERR_PROCESSED:
    xprintf(1, gettext("%s: MESSAGE: document was already processed\n"), str);
    break;
  case ERR_UDISABLED:
    xprintf(1, gettext("%s: MESSAGE: document was disabled by user\n"), str);
    break;
  case ERR_RDISABLED:
    xprintf(1,
      gettext("%s: MESSAGE: document was disabled by limiting rules\n"), str);
    break;
  case ERR_LOW_TRANSFER_RATE:
    xprintf(1,
      gettext("%s: WARNING: transfer rate lower than minimal allowed\n"),
      str);
    break;
  case ERR_QUOTA_FILE:
    xprintf(1,
      gettext
      ("%s: WARNING: file size quota exceeded, rest will be truncated\n"),
      str);
    break;
  case ERR_QUOTA_TRANS:
    xprintf(1,
      gettext("%s: WARNING: transfer quota exceeded, breaking download\n"),
      str);
    break;
  case ERR_QUOTA_FS:
    xprintf(1,
      gettext("%s: ERROR: low free space on filesystem, breaking transfer\n"),
      str);
    break;
  case ERR_QUOTA_TIME:
    xprintf(1,
      gettext
      ("%s: WARNING: maximal allowed running time exceeded, downloading will break\n"),
      str);
    break;
  case ERR_FTP_UNKNOWN:
    xprintf(1, gettext("%s: ERROR: unnown FTP error\n"), str);
    break;
  case ERR_FTP_NOREGET:
    xprintf(1,
      gettext("%s: ERROR: FTP server doesn't support REST command\n"), str);
    break;
  case ERR_FTP_BDIR:
  case ERR_FTP_NODIR:
    xprintf(1, gettext("%s: ERROR: unable to list directory content\n"), str);
    break;
  case ERR_FTP_CONNECT:
    xprintf(1, gettext("%s: ERROR: unable to connect to FTP server\n"), str);
    break;
  case ERR_FTP_BUSER:
    xprintf(1, gettext("%s: ERROR: FTP authentification - bad username\n"),
      str);
    break;
  case ERR_FTP_BPASS:
    xprintf(1, gettext("%s: ERROR: FTP authentification - bad password\n"),
      str);
    break;
  case ERR_FTP_BPROXYUSER:
    xprintf(1,
      gettext("%s: ERROR: FTP proxy authentification - bad username\n"), str);
    break;
  case ERR_FTP_BPROXYPASS:
    xprintf(1,
      gettext("%s: ERROR: FTP proxy authentification - bad password\n"), str);
    break;
  case ERR_FTP_DATACON:
    xprintf(1, gettext("%s: ERROR: unable to open FTP data connection\n"),
      str);
    break;
  case ERR_FTP_GET:
    xprintf(1, gettext("%s: ERROR: unable to get file from FTP server\n"),
      str);
    break;
  case ERR_FTP_NOMDTM:
    xprintf(1,
      gettext("%s: ERROR: FTP server doesn't support MDTM command\n"), str);
    break;
  case ERR_FTP_TRUNC:
    xprintf(1, gettext("%s: ERROR: file from FTP server is truncated\n"),
      str);
    break;
  case ERR_HTTP_ACTUAL:
  case ERR_FTP_ACTUAL:
    xprintf(1, gettext("%s: MESSAGE: reget unneeded - file is up to date\n"),
      str);
    break;
  case ERR_FTP_NOTRANSFER:
    xprintf(1,
      gettext("%s: MESSAGE: FTP transfer not allowed because of rules\n"),
      str);
    break;
  case ERR_FTP_DIRNO:
    xprintf(1,
      gettext
      ("%s: WARNING: FTP directory URL, but FTP directory not allowed (-FTPdir)\n"),
      str);
    break;
  case ERR_FTP_LOGIN_HANDSHAKE:
    xprintf(1, gettext("%s: WARNING: FTP login_handshake failed\n"), str);
    break;
  case ERR_HTTP_UNKNOWN:
    xprintf(1, gettext("%s: ERROR: unknown HTTP error\n"), str);
    break;
  case ERR_HTTP_CONNECT:
    xprintf(1, gettext("%s: ERROR: unable to connect to HTTP server\n"), str);
    break;
  case ERR_HTTP_NOREGET:
    xprintf(1,
      gettext
      ("%s: ERROR: HTTP server doesn't support partial content retrieving\n"),
      str);
    break;
  case ERR_HTTP_SNDREQ:
    xprintf(1, gettext("%s: ERROR: broken HTTP request send\n"), str);
    break;
  case ERR_HTTP_RCVRESP:
    xprintf(1, gettext("%s: ERROR: failed to read HTTP response\n"), str);
    break;
  case ERR_HTTP_FAILREGET:
    xprintf(1,
      gettext
      ("%s: ERROR: unexpected HTTP response code after trying to reget\n"),
      str);
    break;

  case ERR_HTTP_SNDREQDATA:
    xprintf(1, gettext("%s: ERROR: unable to send HTTP request data\n"), str);
    break;
  case ERR_HTTP_REDIR:
    xprintf(1, gettext("%s: MESSAGE: redirecting to another location\n"),
      str);
    break;
  case ERR_HTTP_TRUNC:
    xprintf(1, gettext("%s: ERROR: HTTP document is truncated\n"), str);
    break;
  case ERR_HTTP_CYCLIC:
    xprintf(1, gettext("%s: ERROR: cyclic redirection!\n"), str);
    break;
  case ERR_HTTP_UNSUPREDIR:
    xprintf(1, gettext("%s: ERROR: redirecting to unsupported URL\n"), str);
    break;
  case ERR_HTTP_PROXY_CONN:
    xprintf(1, gettext("%s: ERROR: unable to connect to proxy server\n"),
      str);
    break;
  case ERR_HTTP_BADREDIRECT:
    xprintf(1,
      gettext("%s: ERROR: received bad redirect response from server\n"),
      str);
    break;
  case ERR_HTTP_AUTH_NTLM:
    xprintf(1, gettext("%s: ERROR: unable to do NTLM authorization\n"), str);
    break;
  case ERR_HTTP_AUTH_DIGEST:
    xprintf(1, gettext("%s: ERROR: unable to do HTTP Digest authorization\n"),
      str);
    break;
  case ERR_HTTP_PROAUTH_NTLM:
    xprintf(1, gettext("%s: ERROR: unable to do NTLM proxy authorization\n"),
      str);
    break;
  case ERR_HTTP_PROAUTH_DIGEST:
    xprintf(1,
      gettext("%s: ERROR: unable to do HTTP proxy Digest authorization\n"),
      str);
    break;
  case ERR_HTTP_BADRQ:
    xprintf(1, gettext("%s: ERROR: HTTP client sends bad request\n"), str);
    break;
  case ERR_HTTP_AUTH:
    xprintf(1, gettext("%s: ERROR: HTTP authentication is required\n"), str);
    break;
  case ERR_HTTP_PROXY_AUTH:
    xprintf(1, gettext("%s: ERROR: HTTP proxy authentication is required\n"),
      str);
    break;
  case ERR_HTTP_PAY:
    xprintf(1, gettext("%s: ERROR: HTTP payment required\n"), str);
    break;
  case ERR_HTTP_FORB:
    xprintf(1, gettext("%s: ERROR: forbidden HTTP request\n"), str);
    break;
  case ERR_HTTP_NFOUND:
    xprintf(1, gettext("%s: ERROR: HTTP document not found\n"), str);
    break;
  case ERR_HTTP_SERV:
    xprintf(1, gettext("%s: ERROR: HTTP remote server error\n"), str);
    break;
  case ERR_HTTP_TIMEOUT:
    xprintf(1,
      gettext
      ("%s: ERROR: HTTP server replied with connection timeout response\n"),
      str);
    break;
  case ERR_HTTP_CONFLICT:
    xprintf(1,
      gettext("%s: ERROR: HTTP server replied with conflict response\n"),
      str);
    break;
  case ERR_HTTP_GONE:
    xprintf(1, gettext("%s: ERROR: document was removed from HTTP server\n"),
      str);
    break;
  case ERR_HTTP_USE_PROXY:
    xprintf(1, gettext("%s: ERROR: you must use proxy to access this URL\n"),
      str);
    break;
  case ERR_HTTP_306:
    xprintf(1, gettext("%s: ERROR: 306\n"), str);
    break;
  case ERR_HTTP_NALLOW:
    xprintf(1,
      gettext
      ("%s: ERROR: used HTTP method is not supported or not allowed for this URL\n"),
      str);
    break;
  case ERR_HTTP_NACCEPT:
    xprintf(1,
      gettext
      ("%s: ERROR: client doesn't accept MIME type of requested URL\n"), str);
    break;
  case ERR_HTTP_MISSLEN:
    xprintf(1,
      gettext
      ("%s: ERROR: in request header is missing Content-Length: header\n"),
      str);
    break;
  case ERR_HTTP_PREC_FAIL:
    xprintf(1,
      gettext
      ("%s: ERROR: preconditions in request failed for requested URL\n"),
      str);
    break;
  case ERR_HTTP_TOO_LARGE:
    xprintf(1, gettext("%s: ERROR: request body too large\n"), str);
    break;
  case ERR_HTTP_LONG_URL:
    xprintf(1, gettext("%s: ERROR: request URL too long\n"), str);
    break;
  case ERR_HTTP_UNSUP_MEDIA:
    xprintf(1,
      gettext("%s: ERROR: resource in format unsupported for this request\n"),
      str);
    break;
  case ERR_HTTP_BAD_RANGE:
    xprintf(1, gettext("%s: ERROR: requested bad range of document\n"), str);
    break;
  case ERR_HTTP_EXPECT_FAIL:
    xprintf(1,
      gettext("%s: ERROR: failed to fulfill expectation from request\n"),
      str);
    break;
  case ERR_HTTP_NOT_IMPL:
    xprintf(1,
      gettext
      ("%s: ERROR: requested HTTP method not implemented on server side\n"),
      str);
    break;
  case ERR_HTTP_BAD_GW:
    xprintf(1, gettext("%s: ERROR: gatewaying failed for this URL\n"), str);
    break;
  case ERR_HTTP_SERV_UNAVAIL:
    xprintf(1,
      gettext
      ("%s: ERROR: HTTP service currently not available, check later\n"),
      str);
    break;
  case ERR_HTTP_GW_TIMEOUT:
    xprintf(1,
      gettext
      ("%s: ERROR: timeout occured in communication between gateway and remote server\n"),
      str);
    break;
  case ERR_HTTP_VER_UNSUP:
    xprintf(1,
      gettext
      ("%s: ERROR: HTTP version used in request is unsupported by remote server\n"),
      str);
    break;
  case ERR_GOPHER_CONNECT:
    xprintf(1, gettext("%s: ERROR: unable to connect to GOPHER server\n"),
      str);
    break;
  case ERR_GOPHER_UNKNOWN:
    xprintf(1, gettext("%s: ERROR: unknown GOPHER error\n"), str);
    break;
  case ERR_HTTPS_CONNECT:
    xprintf(1, gettext("%s: ERROR: unable to connect to HTTPS server\n"),
      str);
    break;
  case ERR_FTPS_CONNECT:
    xprintf(1,
      gettext
      ("%s: ERROR: unable to establish SSL control connection to FTPS server\n"),
      str);
    break;
  case ERR_FTPS_UNSUPORTED:
    xprintf(1,
      gettext("%s: ERROR: this FTP server doesn't support SSL connections\n"),
      str);
    break;
  case ERR_FTPS_DATASSLCONNECT:
    xprintf(1,
      gettext
      ("%s: ERROR: unable to establish SSL data connection to FTPS server\n"),
      str);
    break;
  default:
    xprintf(1, gettext("%s: ERROR: unknown errcode : %d\n"), str,
      docp->errcode);
  }
}
