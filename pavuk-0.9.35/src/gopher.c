/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "abstract.h"
#include "doc.h"
#include "errcode.h"
#include "gopher.h"
#include "http.h"
#include "net.h"
#include "url.h"

/********************************************************/
/* otvori spojenie na gopher server pre dane gopher URL */
/* FIXME: Translate me!                                 */
/********************************************************/
bufio *gopher_get_data_socket(doc * docp)
{
  char pom[2048];
  char *host;
  int port;

  if(priv_cfg.gopher_proxy)
  {
    host = priv_cfg.gopher_proxy;
    port = cfg.gopher_proxy_port;
  }
  else
  {
    host = docp->doc_url->p.gopher.host;
    port = docp->doc_url->p.gopher.port;
  }

  docp->datasock = bufio_sock_fdopen(net_connect(host, port, docp));

  if(docp->datasock)
  {
    if(priv_cfg.gopher_proxy)
    {
      if(http_dumy_proxy_connect(docp,
          url_get_site(docp->doc_url),
          url_get_port(docp->doc_url), host, port))
      {
        docp->errcode = ERR_PROXY_CONNECT;
        bufio_close(docp->datasock);
        docp->datasock = NULL;
        return NULL;
      }
    }

    snprintf(pom, sizeof(pom), "%s\r\n", docp->doc_url->p.gopher.selector + 1);
    abs_write(docp->datasock, pom, strlen(pom));
    DEBUG_PROTOC(gettext
      ("********************* Gopher request **************\n"));
    DEBUG_PROTOC("%s", pom);
    DEBUG_PROTOC("***************************************************\n");
  }
  else
  {
    docp->errcode = ERR_GOPHER_CONNECT;
    if(_h_errno_)
      xherror(host);
    else
      xperror("net_connect");
  }


  return docp->datasock;
}

/********************************************************/
/* z gopher adresara urobi HTML dokument                */
/* FIXME: Translate me!                                 */
/********************************************************/
void gopher_dir_to_html(doc * docp)
{
  char *p, *res = NULL;
  char pom[8192];
  int tsize;
  char tp;
  char *title;
  char *host;
  char *sel;
  int port;
  int ilen;
  bool_t last = 1;

  if(docp->doc_url->p.gopher.selector[0] != '1')
    return;

  snprintf(pom, sizeof(pom),
    gettext("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
      "<HTML>\n<TITLE>\nDirectory of gopher://%s:%hu/%s\n</TITLE>\n"
      "<BODY>\n<H1 ALIGN=CENTER><B>"
      "Directory of gopher://%s:%hu/%s </H1><BR><BR><UL>"),
    docp->doc_url->p.gopher.host, docp->doc_url->p.gopher.port,
    docp->doc_url->p.gopher.selector, docp->doc_url->p.gopher.host,
    docp->doc_url->p.gopher.port, docp->doc_url->p.gopher.selector);

  res = new_string(pom);
  tsize = strlen(pom);

  p = docp->contents;

  while(*p)
  {
    char **fields;

    ilen = strcspn(p, "\r\n");
    if(*(p + ilen))
      *(p + ilen) = '\0';
    else
      last = 0;

    if(*p == '.')
      break;

    fields = tl_str_split(p, "\t");

    if(tl_strv_length(fields) > 3)
    {
      title = fields[0];
      tp = *title;
      title++;

      sel = url_encode_str(fields[1], URL_PATH_UNSAFE);
      host = fields[2];
      port = _atoi(fields[3]);

      snprintf(pom, sizeof(pom), "<LI><A HREF=\"gopher://%s:%c%d/%s\">"
        "&quot;%s&quot;</A></LI>\n", host, port, tp, sel, title);

      _free(sel);

      tsize += strlen(pom);
      res = _realloc(res, tsize + 1);
      strcat(res, pom);
    }
    else
    {
      xprintf(1, gettext("Failed to parse Gopher directory entry:\n%s\n"), p);
    }

    p += ilen + last;
    p += strspn(p, "\r\n");

    tl_strv_free(fields);
  }

  tsize += 22;
  res = _realloc(res, tsize + 1);

  strcat(res, "</UL>\n</BODY>\n</HTML>\n");

  _free(docp->contents);

  docp->contents = res;
  docp->size = tsize;
}
