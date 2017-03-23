/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include "gui.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef I_FACE

#include "authinfo.h"
#include "cookie.h"
#include "ftp.h"
#include "html.h"
#include "http.h"
#include "jstrans.h"
#include "lfname.h"
#include "log.h"
#include "net.h"
#include "re.h"
#include "times.h"
#include "tools.h"
#include "uconfig.h"

#ifdef GTK_FACE

/*****************************************************************************/
/*      GTK GUI abstracion macros          */
/*****************************************************************************/

#define SET_FTPHSLIST(w, l)\
{\
  dllist *ptr;\
  gtk_clist_freeze(GTK_CLIST(w));\
  gtk_clist_clear(GTK_CLIST(w));\
  for (ptr = l; ptr; ptr = ptr->next)\
  {\
    char *pp[2];\
    char pom[512];\
    ftp_handshake_info *fhi = (ftp_handshake_info *) ptr->data;\
    if (*fhi->host) sprintf(pom, "%s:%hu", fhi->host, fhi->port);\
    else pom[0] = '\0';\
    pp[0] = pom;\
    pp[1] = ftp_handshake_info_data_dump(fhi);\
    gtk_clist_append(GTK_CLIST(w), pp);\
    _free(pp[1]);\
  }\
  gtk_clist_thaw(GTK_CLIST(w));\
}

#define SET_URLLIST(w, l)\
{\
  dllist *p = l;\
  gtk_clist_freeze(GTK_CLIST(w));\
  gtk_clist_clear(GTK_CLIST(w));\
  while(p)\
  {\
    url_info *ui = (url_info *)p->data;\
    int row;\
    char *pp[2];\
    pp[0] = ui->urlstr;\
    pp[1] = ui->localname;\
    row = gtk_clist_append(GTK_CLIST(w), pp);\
    if (ui->type == URLI_FORM || ui->localname)\
    {\
      ui = url_info_duplicate(ui);\
      gtk_clist_set_row_data_full(GTK_CLIST(w), row, ui,\
        (GtkDestroyNotify)url_info_free);\
    }\
      p = p->next;\
  }\
  gtk_clist_thaw(GTK_CLIST(w));\
}

#define SET_PROXYLIST(w, l)\
{\
  dllist *p = l;\
  char pm[1024];\
  char *xp = pm;\
  gtk_clist_freeze(GTK_CLIST(w));\
  gtk_clist_clear(GTK_CLIST(w));\
  while(p)\
  {\
    int row;\
    http_proxy *pr = (http_proxy *)p->data;\
    sprintf(pm, "%s:%hu", pr->addr, pr->port);\
    row = gtk_clist_append(GTK_CLIST(w), &xp);\
    http_proxy_ref(pr);\
    gtk_clist_set_row_data_full(GTK_CLIST(w), row, pr,\
      (GtkDestroyNotify) http_proxy_unref);\
      p = p->next;\
  }\
  gtk_clist_thaw(GTK_CLIST(w));\
}

#define SET_LIST(w,l) \
{\
  char **p = l;\
  gtk_clist_freeze(GTK_CLIST(w));\
  gtk_clist_clear(GTK_CLIST(w));\
        if (p) while(*p)\
        {\
    gtk_clist_append(GTK_CLIST(w), p);\
      p++;\
  }\
  gtk_clist_thaw(GTK_CLIST(w));\
}

#define SET_DLLIST(w,l) \
{\
  dllist *p = l;\
  gtk_clist_freeze(GTK_CLIST(w));\
  gtk_clist_clear(GTK_CLIST(w));\
        while(p)\
        {\
    gtk_clist_append(GTK_CLIST(w), (char **)&p->data);\
      p = p->next;\
  }\
  gtk_clist_thaw(GTK_CLIST(w));\
}

#define SET_DOUBLE SET_NUM

#define SET_NUM(w,v) \
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), (gfloat)v);\
  gtk_signal_emit_by_name(GTK_OBJECT(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(w))), "value_changed");

#define SET_TEXT(w,t) \
  if (t) gtk_entry_set_text(GTK_ENTRY(w), t);\
  else gtk_entry_set_text(GTK_ENTRY(w), "");

#ifdef NO__CYGWIN__
#define SET_PATH(w,t) \
  if (t) \
  { \
    char *p = cvt_unix_to_win32_path(t); \
    gtk_entry_set_text(GTK_ENTRY(w), p); \
    _free(p); \
  } \
  else gtk_entry_set_text(GTK_ENTRY(w), "");
#else
#define SET_PATH  SET_TEXT
#endif

#define SET_BOOL(w,v) \
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(w), v);


#define SET_HTML_TAGS \
{\
  GList *chlist;\
  int *statpointer;\
  \
  for(chlist = GTK_BOX(gui_cfg.html_tags)->children ; chlist ; chlist = chlist->next)\
  {\
    statpointer = gtk_object_get_user_data(\
        GTK_OBJECT(((struct _GtkBoxChild *)chlist->data)->widget));\
  \
    SET_BOOL(((struct _GtkBoxChild *)chlist->data)->widget ,\
      !(*statpointer & LINK_DISABLED));\
  }\
}

#define SET_COMBO_IDX(w, i) \
    gtk_option_menu_set_history(GTK_OPTION_MENU(w), i);

#define SET_ENUM(w, v) \
  {\
    GList *ch;\
    int i;\
    for (i = 0, ch = GTK_MENU_SHELL(GTK_OPTION_MENU(w)->menu)->children; ch; i++, ch = ch->next)\
      if (v == (int)gtk_object_get_user_data(GTK_OBJECT(ch->data)))\
      {\
        gtk_option_menu_set_history(GTK_OPTION_MENU(w), i);\
        break;\
      }\
  }

#define SET_CALENDAR(w, ltime) \
  gtk_calendar_select_month(GTK_CALENDAR(w), \
    ltime->tm_mon, ltime->tm_year);\
  gtk_calendar_select_day(GTK_CALENDAR(w), \
    ltime->tm_mday);

#define SET_C_EXTRAS \
  SET_BOOL(gui_cfg.ftpmodegr[(int)cfg.ftp_activec], TRUE);

#ifdef USE_SSL
#define SET_SSL_VER \
  if (gui_cfg.ssl_version[0]) SET_BOOL(gui_cfg.ssl_version[cfg.ssl_version - 1], TRUE);
#else
#define SET_SSL_VER
#endif

#ifdef HAVE_REGEX
#define SET_JST(w, l) \
  { \
    dllist *ptr; \
    gtk_clist_clear(GTK_CLIST(w));\
    for (ptr = l; ptr; ptr = ptr->next) \
    { \
      char *pp[5]; \
      js_transform_t *jt = (js_transform_t *) ptr->data; \
      pp[0] = jt->re->pattern; \
      pp[1] = jt->transform; \
      pp[2] = jt->tag; \
      pp[3] = jt->attrib; \
      pp[4] = jt->type ? "X" : " "; \
      gtk_clist_append(GTK_CLIST(w), pp); \
    } \
  }
#else
#define SET_JST(w, l)
#endif

#define SET_NUMLIST(w, l)\
{ \
  dllist *ptr; \
  char *p = NULL; \
  char pom[16]; \
  for (ptr = l; ptr; ptr = ptr->next) \
  { \
    if (p) sprintf(pom, ",%d", (int)ptr->data); \
    else sprintf(pom, "%d", (int)ptr->data); \
    p = tl_str_concat(p, pom, NULL); \
  } \
  gtk_entry_set_text(GTK_ENTRY(w), p ? p : ""); \
  _free(p); \
}

#define SET_PAT(w,l)\
{\
  char pom[4096];\
  char **pp;\
  pom[0] = '\0';\
  for (pp = l ; pp && *pp ; pp++)\
  {\
    if (pp != l)\
    {\
      strcat(pom, ",");\
      strcat(pom, *pp);\
    }\
    else strcpy(pom, *pp);\
  }\
  SET_TEXT(w, pom);\
}

#define SET_REPAT(w,l)\
{\
  dllist *ptr;\
  char pom2[PATH_MAX];\
  pom[0] = '\0';\
  for (ptr = l ; ptr ; ptr = ptr->next)\
  {\
    char *p = escape_str(((re_entry *)ptr->data)->pattern, "\\\"");\
    sprintf(pom2, "\"%s\" ", p);\
    strcat(pom, pom2);\
    _free(p);\
  }\
  SET_TEXT (w, pom);\
}


#define GETFTPHS_LIST(w,l) \
{\
  int n;\
  for (; l; l = dllist_remove_entry(l,l))\
    ftp_handshake_info_free((ftp_handshake_info *) l->data);\
  l = NULL;\
  for (n = 0; n < GTK_CLIST(w)->rows ; n++)\
  {\
    char *pp[2];\
    ftp_handshake_info *fhi;\
    gtk_clist_get_text(GTK_CLIST(w), n, 0, &pp[0]);\
    gtk_clist_get_text(GTK_CLIST(w), n, 1, &pp[1]);\
    fhi = ftp_handshake_info_parse(pp[0], pp[1]);\
    if (fhi)\
      l = dllist_append(l, (dllist_t) fhi);\
  }\
}

#define GET_LIST(w,l) \
{\
  char **p=l;\
  int n;\
  \
  while(l && *p) _free(*p);\
  _free(l);\
  l = NULL;\
  for (n = 0 ; n < GTK_CLIST(w)->rows ; n++)\
  {\
    char *xp;\
    l = (char **)_realloc(l, (n + 2) * sizeof(char *));\
    gtk_clist_get_text(GTK_CLIST(w), n, 0, &xp);\
    l[n] = tl_strdup(xp);\
    l[n+1] = NULL;\
  }\
}

#define GET_URLLIST(w,l) \
{\
  int n;\
  \
  while(l) {url_info_free((url_info *)l->data) ; l = dllist_remove_entry(l, l);}\
  for (n = 0 ; n < GTK_CLIST(w)->rows ; n++)\
  {\
    char *xp;\
    url_info *ui = (url_info *)gtk_clist_get_row_data(GTK_CLIST(w), n);\
    gtk_clist_get_text(GTK_CLIST(w), n, 0, &xp);\
    if (ui) ui = url_info_duplicate(ui);\
    else ui = url_info_new(xp);\
    l = dllist_append(l, (dllist_t) ui);\
  }\
}

#define GET_PROXYLIST(w,ol) \
{\
  int n;\
  dllist *l = NULL;\
  \
  cfg.last_used_proxy_node = NULL;\
  for (n = 0 ; n < GTK_CLIST(w)->rows ; n++)\
  {\
    char *xp;\
    http_proxy *pr;\
    http_proxy *pro;\
    gtk_clist_get_text(GTK_CLIST(w), n, 0, &xp);\
    pro = (http_proxy *)gtk_clist_get_row_data(GTK_CLIST(w), n);\
    pr = http_proxy_parse(xp);\
    if (pro)\
    {\
      pr->penault = pro->penault;\
      pr->fails = pro->fails;\
      pr->is_10 = pro->is_10;\
    }\
    l = dllist_append(l, (dllist_t) pr);\
  }\
  LOCK_PROXY;\
  while (ol) {http_proxy_free((http_proxy *)ol->data);\
    ol = dllist_remove_entry(ol, ol);}\
  ol = l;\
  UNLOCK_PROXY;\
}

#define GET_DLLIST(w,l) \
{\
  int n;\
  \
  while(l) {if(l->data) free((void *)l->data); l = dllist_remove_entry(l, l);}\
  for (n = 0 ; n < GTK_CLIST(w)->rows ; n++)\
  {\
    char *xp;\
    gtk_clist_get_text(GTK_CLIST(w), n, 0, &xp);\
    l = dllist_append(l, (dllist_t) tl_strdup(xp));\
  }\
}

#define GET_BOOL(w,v) \
    (v = GTK_TOGGLE_BUTTON(w)->active)

#define GET_NUM(w,v) \
    (v = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)))

#define GET_DOUBLE(w,v) \
    (v = (double) gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(w)))

#define GET_STR(w,s) \
  _free(s);\
  s = tl_strdup(gtk_entry_get_text (GTK_ENTRY(w)));\
  if (!(s && *s))\
  {\
    _free(s);\
  }

#ifdef NO__CYGWIN__
#define GET_PATH(w,s) \
  _free(s);\
  s = cvt_win32_to_unix_path(gtk_entry_get_text (GTK_ENTRY(w)));\
  if (!(s && *s))\
  {\
    _free(s);\
  }
#else
#define GET_PATH  GET_STR
#endif


#define GET_S_STR(w,s) \
  s = tl_strdup(gtk_entry_get_text (GTK_ENTRY(w)));\
  if (!(s && *s))\
  {\
    _free(s);\
  }

#ifdef NO__CYGWIN__
#define GET_S_PATH(w,s) \
  s = cvt_win32_to_unix_path(gtk_entry_get_text (GTK_ENTRY(w)));\
  if (!(s && *s))\
  {\
    _free(s);\
  }
#else
#define GET_S_PATH  GET_S_STR
#endif

#define GET_HTML_TAGS \
{\
  GList *chlist;\
  int *statpointer;\
  \
  for(chlist = GTK_BOX(gui_cfg.html_tags)->children ; chlist ; chlist = chlist->next)\
  {\
    statpointer = gtk_object_get_user_data(\
      GTK_OBJECT(((struct _GtkBoxChild *)chlist->data)->widget));\
    \
    if (GET_BOOL(((struct _GtkBoxChild *)chlist->data)->widget, b))\
      *statpointer &= ~LINK_DISABLED;\
    else\
      *statpointer |= LINK_DISABLED;\
  }\
}

#define GET_COMBO_IDX(w, i) \
  i = GTK_OPTION_MENU(w)->menu_item ? \
   (int) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(w)->menu_item)) : 0;

#define GET_ENUM(w, v) \
  v = (gint)(GTK_OPTION_MENU(w)->menu_item ? \
    (gint) gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(w)->menu_item)) : 0);

#define GET_DATE(w, ftm) \
  gtk_calendar_get_date(GTK_CALENDAR(w) ,\
    &ftm.tm_year, &ftm.tm_mon, &ftm.tm_mday);\
  ftm.tm_year -= 1900;

#define GET_C_EXTRAS \
  GET_BOOL(gui_cfg.ftpmodegr[1], cfg.ftp_activec);

#ifdef USE_SSL
#define GET_SSL_VER \
  if (gui_cfg.ssl_version[0])\
  {\
    if (GET_BOOL(gui_cfg.ssl_version[0], cfg.ssl_version))\
      cfg.ssl_version = 1;\
    else if (GET_BOOL(gui_cfg.ssl_version[1], cfg.ssl_version))\
      cfg.ssl_version = 2;\
    else if (GET_BOOL(gui_cfg.ssl_version[2], cfg.ssl_version))\
      cfg.ssl_version = 3;\
    else if (GET_BOOL(gui_cfg.ssl_version[3], cfg.ssl_version))\
      cfg.ssl_version = 4;\
  }
#else
#define GET_SSL_VER
#endif

#ifdef HAVE_REGEX
#define GET_JST(w, l) \
  { \
    js_transform_t *jt; \
    int i,j; \
    char *pp[5]; \
    while (l) {js_transform_free((js_transform_t *) l->data); \
      l = dllist_remove_entry(l,l);} \
    for (i = 0; i < GTK_CLIST(w)->rows; i++) \
    { \
      for (j = 0; j < 5; j++) \
        gtk_clist_get_text(GTK_CLIST(w), i, j, &pp[j]); \
      jt = js_transform_new(pp[0], pp[1], pp[2], pp[3], pp[4][0] == 'X'); \
      if (jt) \
        l = dllist_append(l, (dllist_t) jt); \
    } \
  }
#else
#define GET_JST(w, l)
#endif

#define GET_NUMLIST(w, l) \
{ \
  char *p; \
  l = dllist_remove_entry(l, l); \
  GET_S_STR(w, p); \
  if (p) l = tl_numlist_split(p, ","); \
  _free(p); \
}

#define GET_PAT(w,l)\
{ \
        char *p = NULL;\
  int i;\
        GET_STR(w, p);\
        for(i = 0; l && l[i] ; i++)\
                _free(l[i]);\
        _free(l);\
        l = tl_str_split(p, ",");\
  _free(p);\
}

#define GET_REPAT(w,l)\
{\
  dllist *ptr;\
  char *p;\
        p = NULL;\
        GET_STR(w, p);\
  ptr = NULL;\
  if (p && *p)\
  {\
    if (*p == '\"')\
    {\
      ptr = make_re_list(p);\
      if (!ptr)\
      {\
        _free(p);\
        _MT_CFGSTAMP;\
        UNLOCK_GCFG;\
        return -1;\
      }\
    }\
    else\
    {\
      re_entry *ree = re_make(p);\
      if (ree)\
      {\
        ptr = dllist_append(ptr, (dllist_t) ree);\
      }\
      else\
      {\
        _free(p);\
        _MT_CFGSTAMP;\
        UNLOCK_GCFG;\
        return -1;\
      }\
    }\
  }\
  while(l)\
  {\
    re_free((re_entry *)l->data);\
    l = dllist_remove_entry(l, l);\
  }\
        l = ptr;\
  _free(p);\
}

#endif /* GTK_FACE */

void xset_cfg_values_lim(void)
{
  char pom[2048];
  struct tm *ltime;
  time_t t;


  SET_HTML_TAGS;

  SET_BOOL(gui_cfg.ftp_html, cfg.ftp_html);

  SET_BOOL(gui_cfg.leaved_sw, cfg.condition.dont_leave_dir);

  SET_BOOL(gui_cfg.leaves_sw, cfg.condition.dont_leave_site);

  SET_BOOL(gui_cfg.dont_leave_site_dir, cfg.condition.dont_leave_site_dir);

  SET_BOOL(gui_cfg.singlepage, cfg.singlepage);

  SET_BOOL(gui_cfg.limit_inlines, cfg.condition.limit_inlines);

  SET_BOOL(gui_cfg.ftpd_sw, cfg.condition.ftpdir);

  SET_BOOL(gui_cfg.cgi_sw, cfg.condition.cgi);

  SET_BOOL(gui_cfg.allow_ports, cfg.condition.allow_ports);

  SET_BOOL(gui_cfg.hosts_sw, cfg.condition.allow_site);

  SET_BOOL(gui_cfg.domain_sw, cfg.condition.allow_domain);

  SET_BOOL(gui_cfg.mime_sw, cfg.condition.allow_mime);

  SET_BOOL(gui_cfg.prefix_sw, cfg.condition.allow_prefix);

  SET_BOOL(gui_cfg.sufix_sw, cfg.condition.allow_sufix);

  SET_BOOL(gui_cfg.robots_sw, cfg.condition.allow_robots);

  SET_NUM(gui_cfg.maxlev_label, cfg.condition.max_levels);

  SET_NUM(gui_cfg.maxdoc_label, cfg.condition.max_documents);

  SET_NUM(gui_cfg.leave_level, cfg.condition.leave_level);

  SET_NUM(gui_cfg.site_level, cfg.condition.site_level);

  SET_NUM(gui_cfg.maxsize_label, cfg.condition.max_size);
  SET_NUM(gui_cfg.min_size, cfg.condition.min_size);

  SET_NUMLIST(gui_cfg.ports, cfg.condition.ports);

  SET_PAT(gui_cfg.pattern_label, cfg.condition.pattern);
  SET_PAT(gui_cfg.skip_pattern, cfg.condition.skip_pattern);
  SET_PAT(gui_cfg.url_pattern_label, cfg.condition.url_pattern);
  SET_PAT(gui_cfg.skip_url_pattern, cfg.condition.skip_url_pattern);

#ifdef HAVE_REGEX
  SET_REPAT(gui_cfg.rpattern, cfg.condition.rpattern);
  SET_REPAT(gui_cfg.skip_rpattern, cfg.condition.rskip_pattern);
  SET_REPAT(gui_cfg.url_rpattern, cfg.condition.rurl_pattern);
  SET_REPAT(gui_cfg.url_skip_rpattern, cfg.condition.rskip_url_pattern);
#endif


  SET_TEXT(gui_cfg.en_uexit, cfg.condition.uexit);
  SET_TEXT(gui_cfg.follow_cmd, cfg.condition.follow_cmd);

  SET_LIST(gui_cfg.hosts_list, cfg.condition.sites);

  SET_LIST(gui_cfg.domain_list, cfg.condition.domains);

  SET_LIST(gui_cfg.sufixlist, cfg.condition.sufix);

  SET_LIST(gui_cfg.prefixlist, cfg.condition.dir_prefix);

  SET_LIST(gui_cfg.amimelist, cfg.condition.mime);

  SET_DOUBLE(gui_cfg.max_time, cfg.max_time);

  SET_BOOL(gui_cfg.btime_sw, !(cfg.condition.btime == 0));
  t = time(NULL);

  LOCK_TIME;
  if(cfg.condition.btime)
    ltime = new_tm(localtime(&cfg.condition.btime));
  else
    ltime = new_tm(localtime(&t));
  UNLOCK_TIME;

  ltime->tm_year += 1900;

  SET_CALENDAR(gui_cfg.btime_cal, ltime);

  SET_NUM(gui_cfg.btime_h_entry, ltime->tm_hour);
  SET_NUM(gui_cfg.btime_min_entry, ltime->tm_min);
  _free(ltime);

  SET_BOOL(gui_cfg.etime_sw, !(cfg.condition.etime == 0));
  LOCK_TIME;
  if(cfg.condition.etime)
    ltime = new_tm(localtime(&cfg.condition.etime));
  else
    ltime = new_tm(localtime(&t));
  UNLOCK_TIME;

  ltime->tm_year += 1900;

  SET_CALENDAR(gui_cfg.etime_cal, ltime);

  SET_NUM(gui_cfg.etime_h_entry, ltime->tm_hour);
  SET_NUM(gui_cfg.etime_min_entry, ltime->tm_min);
  _free(ltime);

  SET_PATH(gui_cfg.subdir_label, cfg.subdir);

}

void xset_cfg_values_comm(void)
{
  dllist *ptr;
  char pom[PATH_MAX];

  SET_PATH(gui_cfg.cdir_label, cfg.cache_dir);
  SET_TEXT(gui_cfg.default_prefix, cfg.default_prefix);
  SET_PATH(gui_cfg.info_dir, cfg.info_dir);

#ifdef HAVE_BDB_18x
  SET_PATH(gui_cfg.ns_cache_dir, cfg.ns_cache_dir);
  SET_PATH(gui_cfg.moz_cache_dir, cfg.moz_cache_dir);
#endif

  SET_PATH(gui_cfg.en_cookie_file, cfg.cookie_file);

  SET_TEXT(gui_cfg.index_name, cfg.index_name);

  SET_TEXT(gui_cfg.store_name, cfg.store_name);

  SET_BOOL(gui_cfg.check_size, cfg.check_size);
  SET_BOOL(gui_cfg.store_index, cfg.store_index);

  SET_BOOL(gui_cfg.unique_doc, cfg.unique_doc);
  SET_BOOL(gui_cfg.del_after, cfg.del_after);

  SET_BOOL(gui_cfg.preserve_perm, cfg.preserve_perm);
  SET_BOOL(gui_cfg.preserve_links, cfg.preserve_links);
  SET_BOOL(gui_cfg.retrieve_slink, cfg.retrieve_slink);
  SET_BOOL(gui_cfg.ftp_list, cfg.ftp_list);
  SET_BOOL(gui_cfg.fix_wuftpd, cfg.fix_wuftpd);
  SET_BOOL(gui_cfg.use_http11, cfg.use_http11);

  SET_FTPHSLIST(gui_cfg.ftp_login_hs, cfg.ftp_login_hs);

  SET_BOOL(gui_cfg.sw_cookie_send, cfg.send_cookies);
  SET_BOOL(gui_cfg.sw_cookie_recv, cfg.recv_cookies);
  SET_BOOL(gui_cfg.sw_cookie_update, cfg.update_cookies);
  SET_BOOL(gui_cfg.cookie_check_domain, cfg.cookie_check_domain);

  SET_BOOL(gui_cfg.ptime, cfg.preserve_time);

  SET_BOOL(gui_cfg.oldrm_sw, cfg.remove_old);

  SET_BOOL(gui_cfg.cache_sw, cfg.cache);

  SET_BOOL(gui_cfg.ftp_sw, cfg.condition.ftp);

  SET_BOOL(gui_cfg.http_sw, cfg.condition.http);

#ifdef USE_SSL
  SET_BOOL(gui_cfg.https_sw, cfg.condition.https);
  SET_BOOL(gui_cfg.ftps_sw, cfg.condition.ftps);
#endif
  SET_BOOL(gui_cfg.gopher_sw, cfg.condition.gopher);

  SET_BOOL(gui_cfg.gopher_httpgw, cfg.gopher_via_http);

  SET_BOOL(gui_cfg.ftp_httpgw, cfg.ftp_via_http);

  SET_BOOL(gui_cfg.ftp_dirtyp, cfg.ftp_dirtyp);

  SET_BOOL(gui_cfg.freget_sw, cfg.freget);

  SET_BOOL(gui_cfg.noreloc_sw, cfg.rewrite_links);

  SET_BOOL(gui_cfg.url_to_local, TRUE);

  SET_BOOL(gui_cfg.all_to_local, cfg.all_to_local);
  SET_BOOL(gui_cfg.all_to_remote, cfg.all_to_remote);
  SET_BOOL(gui_cfg.sel_to_local, cfg.sel_to_local);
  SET_BOOL(gui_cfg.post_update, cfg.post_update);

  SET_BOOL(gui_cfg.read_css, cfg.read_css);

  SET_BOOL(gui_cfg.enable_js, cfg.enable_js);

  SET_BOOL(gui_cfg.enable_info, cfg.enable_info);

  SET_BOOL(gui_cfg.auto_referer, cfg.auto_referer);
  SET_BOOL(gui_cfg.referer, cfg.referer);

  SET_BOOL(gui_cfg.enc_sw, cfg.use_enc);

  SET_BOOL(gui_cfg.auth_reuse_nonce, cfg.auth_reuse_nonce);
  SET_BOOL(gui_cfg.auth_reuse_proxy_nonce, cfg.auth_reuse_proxy_nonce);

  SET_BOOL(gui_cfg.send_if_range, cfg.send_if_range);

  SET_BOOL(gui_cfg.show_time, cfg.show_time);

  SET_ENUM(gui_cfg.http_auth_scheme, cfg.auth_scheme);
  SET_ENUM(gui_cfg.http_proxy_auth_scheme, cfg.proxy_auth_scheme);

#ifdef ENABLE_NTLM
  SET_TEXT(gui_cfg.auth_ntlm_domain, cfg.auth_ntlm_domain);
  SET_TEXT(gui_cfg.auth_proxy_ntlm_domain, cfg.auth_proxy_ntlm_domain);
#endif

  SET_C_EXTRAS;
  SET_SSL_VER;

#ifdef WITH_TREE
  SET_TEXT(gui_cfg.browser_label, cfg.browser);
#endif

  SET_TEXT(gui_cfg.remind_cmd, cfg.remind_cmd);
  SET_TEXT(gui_cfg.post_cmd, cfg.post_cmd);

  SET_TEXT(gui_cfg.identity, cfg.identity);

  SET_TEXT(gui_cfg.ftp_proxyh_label, cfg.ftp_proxy);

  SET_NUM(gui_cfg.ftp_proxyp_label, cfg.ftp_proxy_port);

  SET_TEXT(gui_cfg.gopher_proxyh_label, cfg.gopher_proxy);

  SET_NUM(gui_cfg.gopher_proxyp_label, cfg.gopher_proxy_port);

#ifdef USE_SSL
  SET_TEXT(gui_cfg.ssl_proxyh_label, cfg.ssl_proxy);

  SET_NUM(gui_cfg.ssl_proxyp_label, cfg.ssl_proxy_port);

  SET_TEXT(gui_cfg.ssl_cert_passwd_en, cfg.ssl_cert_passwd);
  SET_TEXT(gui_cfg.ssl_cipher_list, cfg.ssl_cipher_list);
  SET_BOOL(gui_cfg.unique_sslid, cfg.unique_sslid);
#ifdef USE_SSL_IMPL_OPENSSL
  SET_PATH(gui_cfg.ssl_cert_file_en, cfg.ssl_cert_file);
  SET_PATH(gui_cfg.ssl_key_file_en, cfg.ssl_key_file);
#ifdef HAVE_RAND_EGD
  SET_PATH(gui_cfg.egd_socket, cfg.egd_socket);
#endif
#endif

#ifdef USE_SSL_IMPL_NSS
  SET_PATH(gui_cfg.nss_cert_dir, cfg.nss_cert_dir);
  SET_BOOL(gui_cfg.nss_accept_unknown_cert, cfg.nss_accept_unknown_cert);
  SET_BOOL(gui_cfg.nss_domestic_policy, cfg.nss_domestic_policy);
#endif
#endif

  SET_TEXT(gui_cfg.local_ip, cfg.local_ip);
  memset(&cfg.local_ip_addr, '\0', sizeof(cfg.local_ip_addr));
  if(cfg.local_ip && net_host_to_in_addr(cfg.local_ip, &cfg.local_ip_addr))
  {
    xherror(cfg.local_ip);
  }


  ptr = cfg.http_headers;
  pom[0] = '\0';
  while(ptr)
  {
    httphdr *hdr = (httphdr *) ptr->data;
    char pom2[PATH_MAX];
    char *p1, *p2;

    p1 = escape_str(hdr->name, "\\\"");
    p2 = escape_str(hdr->val, "\\\"");
    sprintf(pom2, "\"%s%s %s\" ", hdr->all ? "+" : "", p1, p2);
    _free(p1);
    _free(p2);
    strcat(pom, pom2);
    ptr = ptr->next;
  }
  SET_TEXT(gui_cfg.http_headers, pom);

  SET_TEXT(gui_cfg.ftp_list_options, cfg.ftp_list_options);

  SET_NUM(gui_cfg.en_cookie_max, cfg.cookies_max);

  SET_NUM(gui_cfg.base_level_label, cfg.base_level);

  SET_DOUBLE(gui_cfg.timeout_label, cfg.ctimeout);

  SET_NUM(gui_cfg.retry_label, cfg.nretry);

  SET_NUM(gui_cfg.redir_label, cfg.nredir);

  SET_NUM(gui_cfg.reget_label, cfg.nreget);

  SET_NUM(gui_cfg.sleep_label, cfg.sleep);
  SET_BOOL(gui_cfg.rsleep, cfg.rsleep);

  SET_NUM(gui_cfg.xloglen_label, cfg.xlogsize);

  SET_NUM(gui_cfg.bufsize, cfg.bufsize);

#ifdef HAVE_MT
  SET_NUM(gui_cfg.nthr, cfg.nthr);
#endif

#ifdef HAVE_FSTATFS
  SET_NUM(gui_cfg.fs_quota, cfg.fs_quota);
#endif
  SET_NUM(gui_cfg.file_quota, cfg.file_quota);

  SET_NUM(gui_cfg.trans_quota, cfg.trans_quota);

  SET_DOUBLE(gui_cfg.maxrate, cfg.maxrate);

  SET_DOUBLE(gui_cfg.minrate, cfg.minrate);

  SET_BOOL(gui_cfg.gen_logname, cfg.gen_logname);

  SET_PATH(gui_cfg.log_label, cfg.logfile);

  SET_PATH(gui_cfg.slog_label, cfg.short_logfile);

  SET_TEXT(gui_cfg.auth_label, cfg.name_auth);

  SET_TEXT(gui_cfg.pass_label, cfg.passwd_auth);

  SET_TEXT(gui_cfg.proxy_auth_label, cfg.http_proxy_user);
  SET_TEXT(gui_cfg.proxy_pass_label, cfg.http_proxy_pass);

  SET_TEXT(gui_cfg.ftp_proxy_user, cfg.ftp_proxy_user);
  SET_TEXT(gui_cfg.ftp_proxy_pass, cfg.ftp_proxy_pass);

  SET_BOOL(gui_cfg.send_from, cfg.send_from);
  SET_TEXT(gui_cfg.from_label, cfg.from);

  SET_NUM(gui_cfg.rollback_label, cfg.rollback);

  SET_NUM(gui_cfg.ddays_label, cfg.ddays);

  SET_NUM(gui_cfg.hash_size, cfg.hash_size);

  SET_LIST(gui_cfg.alanglist, cfg.accept_lang);

  SET_LIST(gui_cfg.acharset_list, cfg.accept_chars);

  SET_LIST(gui_cfg.cookie_domain_list, cfg.cookies_disabled_domains);

  SET_TEXT(gui_cfg.tr_del_chr, cfg.tr_del_chr);
  SET_TEXT(gui_cfg.tr_str_s1, cfg.tr_str_s1);
  SET_TEXT(gui_cfg.tr_str_s2, cfg.tr_str_s2);
  SET_TEXT(gui_cfg.tr_chr_s1, cfg.tr_chr_s1);
  SET_TEXT(gui_cfg.tr_chr_s2, cfg.tr_chr_s2);

  SET_COMBO_IDX(gui_cfg.scheduling_strategie, cfg.scheduling_strategie);

  SET_PAT(gui_cfg.dont_touch_url_pattern, cfg.dont_touch_url_pattern);

#ifdef HAVE_REGEX
  SET_REPAT(gui_cfg.dont_touch_url_rpattern, cfg.dont_touch_url_rpattern);
  SET_REPAT(gui_cfg.dont_touch_tag_rpattern, cfg.dont_touch_tag_rpattern);
  SET_BOOL(gui_cfg.remove_adv, cfg.remove_adv);
  {
    dllist *ptr2 = cfg.advert_res;
    ptr = NULL;

    while(ptr2)
    {
      ptr = dllist_append(ptr, (dllist_t) ((re_entry *) ptr2->data)->pattern);
      ptr2 = ptr2->next;
    }

    SET_DLLIST(gui_cfg.advert_res, ptr);

    while(ptr)
      ptr = dllist_remove_entry(ptr, ptr);


    ptr2 = cfg.js_patterns;
    ptr = NULL;

    while(ptr2)
    {
      ptr = dllist_append(ptr, (dllist_t) ((re_entry *) ptr2->data)->pattern);
      ptr2 = ptr2->next;
    }

    SET_DLLIST(gui_cfg.js_patterns, ptr);

    while(ptr)
      ptr = dllist_remove_entry(ptr, ptr);
  }

#endif

#ifdef GTK_FACE
  {
    char *ld[3];

    ptr = cfg.lfnames;

    gtk_clist_freeze(GTK_CLIST(gui_cfg.rules_list));

    gtk_clist_clear(GTK_CLIST(gui_cfg.rules_list));

    while(ptr)
    {
      lfname *lf = (lfname *) ptr->data;

      ld[0] = (lf->type == LFNAME_FNMATCH) ? "F" : "R";
      ld[1] = lf->matchstr;
      ld[2] = lf->transstr;

      gtk_clist_append(GTK_CLIST(gui_cfg.rules_list), ld);
      ptr = ptr->next;
    }

    gtk_clist_thaw(GTK_CLIST(gui_cfg.rules_list));
  }
#endif
  SET_URLLIST(gui_cfg.url_list, cfg.request);

  SET_URLLIST(gui_cfg.formdata_list, cfg.formdata);

  SET_PROXYLIST(gui_cfg.http_proxy_list, cfg.http_proxy);

  SET_JST(gui_cfg.js_transform, cfg.js_transform);
}

#ifdef HAVE_REGEX
static dllist *make_re_list(char *str)
{
  dllist *retv = NULL;
  char *p;
  bool_t err = FALSE;

  for(p = get_1qstr(str); p; p = get_1qstr(NULL))
  {
    re_entry *ree;

    ree = re_make(p);

    if(!ree)
    {
      err = TRUE;
      break;
    }
    else
    {
      retv = dllist_append(retv, (dllist_t) ree);
    }
  }

  if(err)
  {
    while(retv)
    {
      re_free((re_entry *) retv->data);
      retv = dllist_remove_entry(retv, retv);
    }
  }

  return retv;
}
#endif

int xget_cfg_values_lim(void)
{
  struct tm ftm;
  char *p = NULL;
  bool_t b;
  struct stat estat;

  LOCK_GCFG;
  GET_HTML_TAGS;

  GET_PATH(gui_cfg.subdir_label, p);
  if(p && !((stat(p, &estat) >= 0) && S_ISDIR(estat.st_mode)))
  {
    _free(p);
    xprintf(0, gettext("Bad parameter - Working subdirectory"));
    _MT_CFGSTAMP;
    UNLOCK_GCFG;
    return -1;
  }
  else
  {
    _free(cfg.subdir);
    if(p)
      cfg.subdir = get_abs_file_path_oss(p);
    _free(p);
  }

  GET_BOOL(gui_cfg.ftp_html, cfg.ftp_html);

  GET_BOOL(gui_cfg.leaves_sw, cfg.condition.dont_leave_site);

  GET_BOOL(gui_cfg.leaved_sw, cfg.condition.dont_leave_dir);

  GET_BOOL(gui_cfg.dont_leave_site_dir, cfg.condition.dont_leave_site_dir);

  GET_BOOL(gui_cfg.singlepage, cfg.singlepage);

  GET_BOOL(gui_cfg.limit_inlines, cfg.condition.limit_inlines);

  GET_BOOL(gui_cfg.cgi_sw, cfg.condition.cgi);

  GET_BOOL(gui_cfg.ftpd_sw, cfg.condition.ftpdir);

  GET_BOOL(gui_cfg.allow_ports, cfg.condition.allow_ports);

  GET_BOOL(gui_cfg.hosts_sw, cfg.condition.allow_site);

  GET_BOOL(gui_cfg.domain_sw, cfg.condition.allow_domain);

  GET_BOOL(gui_cfg.mime_sw, cfg.condition.allow_mime);

  GET_BOOL(gui_cfg.prefix_sw, cfg.condition.allow_prefix);

  GET_BOOL(gui_cfg.sufix_sw, cfg.condition.allow_sufix);

  GET_BOOL(gui_cfg.robots_sw, cfg.condition.allow_robots);

  GET_NUMLIST(gui_cfg.ports, cfg.condition.ports);

  GET_PAT(gui_cfg.pattern_label, cfg.condition.pattern);
  GET_PAT(gui_cfg.skip_pattern, cfg.condition.skip_pattern);
  GET_PAT(gui_cfg.url_pattern_label, cfg.condition.url_pattern);
  GET_PAT(gui_cfg.skip_url_pattern, cfg.condition.skip_url_pattern);

#ifdef HAVE_REGEX
  GET_REPAT(gui_cfg.rpattern, cfg.condition.rpattern);
  GET_REPAT(gui_cfg.skip_rpattern, cfg.condition.rskip_pattern);
  GET_REPAT(gui_cfg.url_rpattern, cfg.condition.rurl_pattern);
  GET_REPAT(gui_cfg.url_skip_rpattern, cfg.condition.rskip_url_pattern);
  GET_REPAT(gui_cfg.aip, cfg.condition.aip);
  GET_REPAT(gui_cfg.skipip, cfg.condition.skipip);

#endif

  GET_STR(gui_cfg.en_uexit, cfg.condition.uexit);
  GET_STR(gui_cfg.follow_cmd, cfg.condition.follow_cmd);

  GET_LIST(gui_cfg.hosts_list, cfg.condition.sites);

  GET_LIST(gui_cfg.amimelist, cfg.condition.mime);

  GET_LIST(gui_cfg.domain_list, cfg.condition.domains);

  GET_NUM(gui_cfg.maxlev_label, cfg.condition.max_levels);

  GET_NUM(gui_cfg.leave_level, cfg.condition.leave_level);

  GET_NUM(gui_cfg.site_level, cfg.condition.site_level);

  GET_NUM(gui_cfg.maxdoc_label, cfg.condition.max_documents);

  GET_NUM(gui_cfg.maxsize_label, cfg.condition.max_size);
  GET_NUM(gui_cfg.min_size, cfg.condition.min_size);

  GET_LIST(gui_cfg.sufixlist, cfg.condition.sufix);

  GET_LIST(gui_cfg.prefixlist, cfg.condition.dir_prefix);

  GET_DOUBLE(gui_cfg.max_time, cfg.max_time);

  GET_BOOL(gui_cfg.btime_sw, b);
  if(b)
  {
    GET_DATE(gui_cfg.btime_cal, ftm);
    ftm.tm_sec = 0;
    ftm.tm_isdst = -1;
    GET_NUM(gui_cfg.btime_h_entry, ftm.tm_hour);
    GET_NUM(gui_cfg.btime_min_entry, ftm.tm_min);
    cfg.condition.btime = mktime(&ftm);
  }

  else
    cfg.condition.btime = (time_t) 0;

  GET_BOOL(gui_cfg.etime_sw, b);
  if(b)
  {
    GET_DATE(gui_cfg.etime_cal, ftm);
    ftm.tm_sec = 0;
    ftm.tm_isdst = -1;
    GET_NUM(gui_cfg.etime_h_entry, ftm.tm_hour);
    GET_NUM(gui_cfg.etime_min_entry, ftm.tm_min);
    cfg.condition.etime = mktime(&ftm);
  }
  else
    cfg.condition.etime = (time_t) 0;

  _MT_CFGSTAMP;
  UNLOCK_GCFG;

  return 0;
}

int xget_cfg_values_comm(void)
{
  char *tp, *p = NULL;
  struct stat estat;
  dllist *ptr;

  LOCK_GCFG;
  GET_PATH(gui_cfg.cdir_label, p);
  if(p && !((stat(p, &estat) >= 0) && S_ISDIR(estat.st_mode)))
  {
    xprintf(0, gettext("Bad parameter - Cache directory"));
    _MT_CFGSTAMP;
    UNLOCK_GCFG;
    return -1;
  }
  else
  {
    _free(cfg.cache_dir);
    if(p)
      cfg.cache_dir = get_abs_file_path_oss(p);
    _free(p);
  }

  GET_PATH(gui_cfg.info_dir, cfg.info_dir);

#ifdef HAVE_BDB_18x
  GET_PATH(gui_cfg.ns_cache_dir, cfg.ns_cache_dir);
  GET_PATH(gui_cfg.moz_cache_dir, cfg.moz_cache_dir);
#endif

  p = cfg.cookie_file;
  GET_S_PATH(gui_cfg.en_cookie_file, cfg.cookie_file);
  if(!((cfg.cookie_file && p && !strcmp(p, cfg.cookie_file))
      || !cfg.cookie_file))
  {
    cookie_read_file(cfg.cookie_file);
  }
  _free(p);

  GET_STR(gui_cfg.default_prefix, cfg.default_prefix);

  GET_BOOL(gui_cfg.check_size, cfg.check_size);
  GET_BOOL(gui_cfg.store_index, cfg.store_index);

  GET_BOOL(gui_cfg.unique_doc, cfg.unique_doc);
  GET_BOOL(gui_cfg.del_after, cfg.del_after);

  GET_BOOL(gui_cfg.preserve_perm, cfg.preserve_perm);
  GET_BOOL(gui_cfg.preserve_links, cfg.preserve_links);
  GET_BOOL(gui_cfg.retrieve_slink, cfg.retrieve_slink);
  GET_BOOL(gui_cfg.ftp_list, cfg.ftp_list);
  GET_BOOL(gui_cfg.fix_wuftpd, cfg.fix_wuftpd);
  GET_BOOL(gui_cfg.use_http11, cfg.use_http11);

  GETFTPHS_LIST(gui_cfg.ftp_login_hs, cfg.ftp_login_hs);

  GET_STR(gui_cfg.index_name, cfg.index_name);
  GET_STR(gui_cfg.store_name, cfg.store_name);

  GET_BOOL(gui_cfg.sw_cookie_send, cfg.send_cookies);
  GET_BOOL(gui_cfg.sw_cookie_recv, cfg.recv_cookies);
  GET_BOOL(gui_cfg.sw_cookie_update, cfg.update_cookies);
  GET_BOOL(gui_cfg.cookie_check_domain, cfg.cookie_check_domain);

  GET_BOOL(gui_cfg.ptime, cfg.preserve_time);

  GET_BOOL(gui_cfg.cache_sw, cfg.cache);

  GET_BOOL(gui_cfg.oldrm_sw, cfg.remove_old);

  GET_BOOL(gui_cfg.ftp_sw, cfg.condition.ftp);


  GET_BOOL(gui_cfg.http_sw, cfg.condition.http);

#ifdef USE_SSL
  GET_BOOL(gui_cfg.https_sw, cfg.condition.https);
  GET_BOOL(gui_cfg.ftps_sw, cfg.condition.ftps);
#endif
  GET_BOOL(gui_cfg.gopher_sw, cfg.condition.gopher);

  GET_BOOL(gui_cfg.gopher_httpgw, cfg.gopher_via_http);

  GET_BOOL(gui_cfg.ftp_httpgw, cfg.ftp_via_http);

  GET_BOOL(gui_cfg.ftp_dirtyp, cfg.ftp_dirtyp);

  GET_BOOL(gui_cfg.freget_sw, cfg.freget);

  GET_BOOL(gui_cfg.noreloc_sw, cfg.rewrite_links);

  GET_BOOL(gui_cfg.all_to_local, cfg.all_to_local);
  GET_BOOL(gui_cfg.all_to_remote, cfg.all_to_remote);
  GET_BOOL(gui_cfg.sel_to_local, cfg.sel_to_local);
  GET_BOOL(gui_cfg.post_update, cfg.post_update);

  GET_BOOL(gui_cfg.enable_js, cfg.enable_js);

  GET_BOOL(gui_cfg.auto_referer, cfg.auto_referer);
  GET_BOOL(gui_cfg.referer, cfg.referer);

  GET_BOOL(gui_cfg.enable_info, cfg.enable_info);

  GET_BOOL(gui_cfg.enc_sw, cfg.use_enc);

  GET_BOOL(gui_cfg.auth_reuse_nonce, cfg.auth_reuse_nonce);
  GET_BOOL(gui_cfg.auth_reuse_proxy_nonce, cfg.auth_reuse_proxy_nonce);

  GET_BOOL(gui_cfg.send_if_range, cfg.send_if_range);

  GET_BOOL(gui_cfg.show_time, cfg.show_time);

  GET_ENUM(gui_cfg.http_auth_scheme, cfg.auth_scheme);
  GET_ENUM(gui_cfg.http_proxy_auth_scheme, cfg.proxy_auth_scheme);

#ifdef ENABLE_NTLM
  GET_STR(gui_cfg.auth_ntlm_domain, cfg.auth_ntlm_domain);
  GET_STR(gui_cfg.auth_proxy_ntlm_domain, cfg.auth_proxy_ntlm_domain);
#endif

  GET_C_EXTRAS;
  GET_SSL_VER;

#ifdef WITH_TREE
  GET_STR(gui_cfg.browser_label, cfg.browser);
#endif

  GET_STR(gui_cfg.remind_cmd, cfg.remind_cmd);
  GET_STR(gui_cfg.post_cmd, cfg.post_cmd);
  GET_STR(gui_cfg.identity, cfg.identity);

  GET_STR(gui_cfg.ftp_proxyh_label, cfg.ftp_proxy);
  GET_NUM(gui_cfg.ftp_proxyp_label, cfg.ftp_proxy_port);

  GET_STR(gui_cfg.gopher_proxyh_label, cfg.gopher_proxy);
  GET_NUM(gui_cfg.gopher_proxyp_label, cfg.gopher_proxy_port);

  GET_BOOL(gui_cfg.read_css, cfg.read_css);
#ifdef USE_SSL
  GET_STR(gui_cfg.ssl_proxyh_label, cfg.ssl_proxy);
  GET_NUM(gui_cfg.ssl_proxyp_label, cfg.ssl_proxy_port);
  GET_STR(gui_cfg.ssl_cert_passwd_en, cfg.ssl_cert_passwd);
  GET_STR(gui_cfg.ssl_cipher_list, cfg.ssl_cipher_list);
  GET_BOOL(gui_cfg.unique_sslid, cfg.unique_sslid);

#ifdef USE_SSL_IMPL_OPENSSL
  GET_PATH(gui_cfg.ssl_cert_file_en, cfg.ssl_cert_file);
  GET_PATH(gui_cfg.ssl_key_file_en, cfg.ssl_key_file);

#ifdef HAVE_RAND_EGD
  GET_PATH(gui_cfg.egd_socket, cfg.egd_socket);
#endif
#endif
#ifdef USE_SSL_IMPL_NSS
  GET_PATH(gui_cfg.nss_cert_dir, cfg.nss_cert_dir);
  GET_BOOL(gui_cfg.nss_accept_unknown_cert, cfg.nss_accept_unknown_cert);
  GET_BOOL(gui_cfg.nss_domestic_policy, cfg.nss_domestic_policy);
#endif
#endif

  GET_STR(gui_cfg.local_ip, cfg.local_ip);
  memset(&cfg.local_ip_addr, '\0', sizeof(cfg.local_ip_addr));
  if(cfg.local_ip && net_host_to_in_addr(cfg.local_ip, &cfg.local_ip_addr))
  {
    xherror(cfg.local_ip);
  }

  GET_S_STR(gui_cfg.http_headers, tp);
  p = tp;
  while(cfg.http_headers)
  {
    httphdr_free((httphdr *) cfg.http_headers->data);
    cfg.http_headers =
      dllist_remove_entry(cfg.http_headers, cfg.http_headers);
  }
  if(p && *p)
  {
    while((p = get_1qstr(p)))
    {
      httphdr *hdr = httphdr_parse(p);
      if(hdr)
        cfg.http_headers = dllist_append(cfg.http_headers, (dllist_t) hdr);
      else
        xprintf(0, "Bad HTTP headers entry - %s\n", p);
      p = NULL;
    }
  }
  _free(tp);

  GET_STR(gui_cfg.ftp_list_options, cfg.ftp_list_options);

  GET_NUM(gui_cfg.en_cookie_max, cfg.cookies_max);

  GET_NUM(gui_cfg.base_level_label, cfg.base_level);

  GET_DOUBLE(gui_cfg.timeout_label, cfg.ctimeout);

  GET_NUM(gui_cfg.retry_label, cfg.nretry);

  GET_NUM(gui_cfg.redir_label, cfg.nredir);

  GET_NUM(gui_cfg.reget_label, cfg.nreget);

  GET_NUM(gui_cfg.sleep_label, cfg.sleep);
  GET_BOOL(gui_cfg.rsleep, cfg.rsleep);

  GET_NUM(gui_cfg.xloglen_label, cfg.xlogsize);

  GET_NUM(gui_cfg.bufsize, cfg.bufsize);

#ifdef HAVE_MT
  GET_NUM(gui_cfg.nthr, cfg.nthr);
#endif

#ifdef HAVE_FSTATFS
  GET_NUM(gui_cfg.fs_quota, cfg.fs_quota);
#endif

  GET_NUM(gui_cfg.file_quota, cfg.file_quota);

  GET_NUM(gui_cfg.trans_quota, cfg.trans_quota);

  GET_DOUBLE(gui_cfg.maxrate, cfg.maxrate);

  GET_DOUBLE(gui_cfg.minrate, cfg.minrate);

  GET_PATH(gui_cfg.slog_label, cfg.short_logfile);

  GET_BOOL(gui_cfg.gen_logname, cfg.gen_logname);

  GET_PATH(gui_cfg.log_label, cfg.logfile);
  log_start(cfg.logfile);

  GET_STR(gui_cfg.auth_label, cfg.name_auth);

  GET_STR(gui_cfg.pass_label, cfg.passwd_auth);

  GET_STR(gui_cfg.proxy_auth_label, cfg.http_proxy_user);
  GET_STR(gui_cfg.proxy_pass_label, cfg.http_proxy_pass);

  GET_STR(gui_cfg.ftp_proxy_user, cfg.ftp_proxy_user);
  GET_STR(gui_cfg.ftp_proxy_pass, cfg.ftp_proxy_pass);

  GET_BOOL(gui_cfg.send_from, cfg.send_from);

  GET_STR(gui_cfg.from_label, cfg.from);

  GET_NUM(gui_cfg.rollback_label, cfg.rollback);

  GET_NUM(gui_cfg.ddays_label, cfg.ddays);

  GET_NUM(gui_cfg.hash_size, cfg.hash_size);
  dlhash_resize(cfg.url_hash_tbl, cfg.hash_size);
  dlhash_resize(cfg.fn_hash_tbl, cfg.hash_size);

  GET_LIST(gui_cfg.alanglist, cfg.accept_lang);
  GET_LIST(gui_cfg.acharset_list, cfg.accept_chars);

  GET_LIST(gui_cfg.cookie_domain_list, cfg.cookies_disabled_domains);

  GET_STR(gui_cfg.tr_del_chr, cfg.tr_del_chr);
  GET_STR(gui_cfg.tr_str_s1, cfg.tr_str_s1);
  GET_STR(gui_cfg.tr_str_s2, cfg.tr_str_s2);
  GET_STR(gui_cfg.tr_chr_s1, cfg.tr_chr_s1);
  GET_STR(gui_cfg.tr_chr_s2, cfg.tr_chr_s2);

  GET_COMBO_IDX(gui_cfg.scheduling_strategie, cfg.scheduling_strategie);

  GET_PAT(gui_cfg.dont_touch_url_pattern, cfg.dont_touch_url_pattern);

#ifdef HAVE_REGEX
  GET_REPAT(gui_cfg.dont_touch_url_rpattern, cfg.dont_touch_url_rpattern);
  GET_REPAT(gui_cfg.dont_touch_tag_rpattern, cfg.dont_touch_tag_rpattern);
  GET_BOOL(gui_cfg.remove_adv, cfg.remove_adv);

  ptr = cfg.advert_res;

  while(ptr)
  {
    re_free((re_entry *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }

  cfg.advert_res = NULL;

  GET_DLLIST(gui_cfg.advert_res, ptr);

  while(ptr)
  {
    re_entry *ree;

    ree = re_make((char *) ptr->data);
    if(ree)
      cfg.advert_res = dllist_append(cfg.advert_res, (dllist_t) ree);
    if(ptr->data) free((char *)ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }


  ptr = cfg.js_patterns;

  while(ptr)
  {
    re_free((re_entry *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }

  cfg.advert_res = NULL;

  GET_DLLIST(gui_cfg.js_patterns, ptr);

  while(ptr)
  {
    re_entry *ree;

    ree = re_make((char *) ptr->data);
    if(ree)
      cfg.js_patterns = dllist_append(cfg.js_patterns, (dllist_t) ree);
    if(ptr->data) free((char *)ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
#endif


#ifdef GTK_FACE
  {
    int i;

    while(cfg.lfnames)
    {
      lfname_free((lfname *) cfg.lfnames->data);
      cfg.lfnames = dllist_remove_entry(cfg.lfnames, cfg.lfnames);
    }

    for(i = 0; i < GTK_CLIST(gui_cfg.rules_list)->rows; i++)
    {
      lfname *lfn;
      lfname_type tp;
      char *ld[3];

      gtk_clist_get_text(GTK_CLIST(gui_cfg.rules_list), i, 0, &ld[0]);
      gtk_clist_get_text(GTK_CLIST(gui_cfg.rules_list), i, 1, &ld[1]);
      gtk_clist_get_text(GTK_CLIST(gui_cfg.rules_list), i, 2, &ld[2]);
#ifdef HAVE_REGEX
      tp = (*ld[0] == 'F') ? LFNAME_FNMATCH : LFNAME_REGEX;
#else
      tp = LFNAME_FNMATCH;
#endif
      if((lfn = lfname_new(tp, ld[1], ld[2])))
        cfg.lfnames = dllist_append(cfg.lfnames, (dllist_t) lfn);
      else
      {
        _MT_CFGSTAMP;
        UNLOCK_GCFG;
        return -1;
      }
    }
  }

#endif

  GET_URLLIST(gui_cfg.url_list, cfg.request);

  GET_URLLIST(gui_cfg.formdata_list, cfg.formdata);

  GET_PROXYLIST(gui_cfg.http_proxy_list, cfg.http_proxy);

  GET_JST(gui_cfg.js_transform, cfg.js_transform);

  _MT_CFGSTAMP;
  UNLOCK_GCFG;

  return 0;
}

#endif /* I_FACE */
