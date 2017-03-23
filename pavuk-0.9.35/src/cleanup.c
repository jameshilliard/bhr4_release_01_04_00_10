/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/


#include "config.h"

#include <unistd.h>
#include <stdio.h>

#include "dns.h"
#include "htmlparser.h"
#include "robots.h"
#include "ainterface.h"
#include "jsbind.h"
#include "myssl.h"

void pavuk_do_at_exit(void)
{
#if defined(I_FACE) && !defined(HAVE_MT)
  dns_server_kill();
#endif
#ifdef HAVE_MOZJS
  pjs_destroy();
#endif
#ifdef USE_SSL
  my_ssl_cleanup();
#endif
  robots_do_cleanup();
  html_parser_do_cleanup();

#if defined(HAVE_MT) && defined(I_FACE)
  if(!cfg.xi_face)
#endif
    free_all();

  if(cfg.url_hash_tbl)
  {
    dlhash_free(cfg.url_hash_tbl);
    cfg.url_hash_tbl = NULL;
  }

  if(cfg.fn_hash_tbl)
  {
    dlhash_free(cfg.fn_hash_tbl);
    cfg.fn_hash_tbl = NULL;
  }

  dns_free_tab();
  cfg_free_params();
  while(cfg.request)
  {
    url_info_free((url_info *) cfg.request->data);
    cfg.request = dllist_remove_entry(cfg.request, cfg.request);
  }
  _free(cfg.time);
  _free(cfg.local_host);
  _free(cfg.path_to_home);
  _free(cfg.install_path);

#ifdef __CYGWIN__
  if(isatty(0) && cfg.wait_on_exit)
  {
    printf(gettext("press any key to exit\n"));
    getc(stdin);
  }
#endif
}
