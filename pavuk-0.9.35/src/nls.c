/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include "tools.h"
#include "gui.h"

#ifdef GETTEXT_NLS
static const struct
{
  const char *name;
  const char *id;
} nls_langcat_tab[] =
{
  {gettext_nop("Czech"), "cs"},
  {gettext_nop("German"), "de"},
  {gettext_nop("Spanish"), "es"},
  {gettext_nop("French"), "fr"},
  {gettext_nop("Italian"), "it"},
  {gettext_nop("Japanese"), "ja"},
  {gettext_nop("Polish"), "pl"},
  {gettext_nop("Slovak"), "sk"},
  {gettext_nop("Ukrainian"), "uk"},
  {gettext_nop("English"), "en"},
  {NULL, NULL}
};

const char *nls_langcat_name(const char *lang)
{
  int i;
  for(i = 0; nls_langcat_tab[i].name; i++)
  {
    if(!strncmp(lang, nls_langcat_tab[i].id, 2))
      break;
  }
  return gettext(nls_langcat_tab[i].name);
}
#endif

void init_locale_env(void)
{
  char *lang;
  char *languages;
  char *lc_messages;
  char *lc_all;
  char *lc_ctype;
  char *l = NULL;

  lang = tl_strdup(getenv("LANG"));
  languages = tl_strdup(getenv("LANGUAGE"));
  lc_all = tl_strdup(getenv("LC_ALL"));
  lc_messages = tl_strdup(getenv("LC_MESSAGES"));
  lc_ctype = tl_strdup(getenv("LC_CTYPE"));

  if(lc_all)
    unsetenv("LC_ALL");
  if(lang)
    unsetenv("LANG");
  if(languages)
    unsetenv("LANGUAGE");
  if(lc_messages)
    unsetenv("LC_MESSAGES");
  unsetenv("LC_TIME");
  unsetenv("LC_NUMERIC");

  if(lc_messages)
    l = lc_messages;
  else if(lang)
    l = lang;
  else if(lc_all)
    l = lc_all;

  if(l)
    setenv("LC_MESSAGES", l, TRUE);

  if(!lc_ctype)
  {
    if(lc_all)
      setenv("LC_CTYPE", lc_all, TRUE);
    else if(lang)
      setenv("LC_CTYPE", lang, TRUE);
  }

  setlocale(LC_CTYPE, "");

#ifdef LC_MESSAGES
  setlocale(LC_MESSAGES, "");
#endif

#ifdef GTK_FACE
  gdk_set_locale();
#endif

  _free(lang);
  _free(lc_messages);
  _free(lc_all);
  _free(lc_ctype);
}
