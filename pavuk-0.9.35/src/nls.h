/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _nls_H_
#define _nls_H_

#ifdef GETTEXT_NLS
#include <locale.h>
#include <libintl.h>

#ifdef HAVE_CAT_CNTR
extern int _nl_msg_cat_cntr;
#define _NLS_CHANGE_CAT \
  _nl_msg_cat_cntr++;
#else
#define _NLS_CHANGE_CAT
#endif

#if defined(LC_MESSAGES) && defined(GTK_FACE)
#define __NLS_SL \
  setlocale(LC_MESSAGES, cfg.language ? cfg.language : ""); \
  gdk_set_locale();
#elif defined(LC_MESSAGES)
#define __NLS_SL \
  setlocale(LC_MESSAGES, cfg.language ? cfg.language : "");
#elif defined(GTK_FACE)
#define __NLS_SL \
  gdk_set_locale();
#else
#define __NLS_SL
#endif


#define _INIT_NLS \
{ \
  if(cfg.language) \
  { \
    setenv("LC_MESSAGES", cfg.language, 1); \
    setenv("LC_CTYPE", cfg.language, 1); \
  } \
  setlocale(LC_CTYPE, cfg.language ? cfg.language : ""); \
  __NLS_SL \
  if(cfg.msgcatd) \
    bindtextdomain(PACKAGE, cfg.msgcatd);\
  textdomain(PACKAGE); \
  _NLS_CHANGE_CAT\
}

extern const char *nls_langcat_name(const char *);
#else /* GETTEXT_NLS */

#define _INIT_NLS

#define gettext(str) str

#endif  /* GETTEXT_NLS */

#define gettext_nop(str) str

extern void init_locale_env(void);

#endif
