/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _gui_h_
#define _gui_h_

#ifdef GTK_FACE
#include <gtk/gtk.h>

#if !defined(HAVE_MT)
#undef GDK_THREADS_ENTER
#define GDK_THREADS_ENTER()
#undef GDK_THREADS_LEAVE
#define GDK_THREADS_LEAVE()
#endif

typedef struct
{
  GdkPixmap *pixmap;
  GdkBitmap *shape;
} Icon;

typedef struct
{
  GtkWidget *list;
  GtkWidget *entry;
} listanfo;

#include "icons.h"
#include "mode.h"

#define GUI_TREE_RTYPE GtkCTreeNode*

#define PAVUK_ABOUT     0
#define PAVUK_CFGCOMM   1
#define PAVUK_CFGSCH    2
#define PAVUK_CFGLIM    3
#define PAVUK_TREE      4
#define PAVUK_SCNLD     5
#define PAVUK_SCNSV     6
#define PAVUK_SCNADD    7
#define PAVUK_JSCONS    8

#define GUI_SET_TOOLTIP(w,t) \
        gtk_tooltips_set_tip(gui_cfg.help_tooltips, w, t, NULL);

typedef struct
{
  GtkWidget *tag_patterns;
  GtkWidget *rsleep;
  GtkWidget *ftp_login_hs;
  GtkWidget *ftp_login_hs_host;
  GtkWidget *ftp_login_hs_handshake;
  GtkWidget *default_prefix;
  GtkWidget *ports;
  GtkWidget *allow_ports;
  GtkTooltips *help_tooltips;
  GtkWidget *info_dir;
  GtkWidget *post_update;
  GtkWidget *fix_wuftpd;
  GtkWidget *ftp_list_options;
  GtkWidget *limit_inlines;
  GtkWidget *js_transform;
  GtkWidget *jst_rewrite;
  GtkWidget *jst_pattern;
  GtkWidget *jst_rule;
  GtkWidget *jst_tag;
  GtkWidget *jst_attrib;
  GtkWidget *ftp_proxy_user;
  GtkWidget *ftp_proxy_pass;
  GtkWidget *follow_cmd;
  GtkWidget *retrieve_slink;
  GtkWidget *js_patterns;
  GtkWidget *auth_ntlm_domain;
  GtkWidget *auth_proxy_ntlm_domain;
  GtkWidget *http_auth_scheme;
  GtkWidget *http_proxy_auth_scheme;
  GtkWidget *singlepage;
  GtkWidget *dont_leave_site_dir;
  GtkWidget *unique_doc;
  GtkWidget *del_after;
  GtkWidget *mini_toolbar;
  GtkWidget *main_window_hide;
  GtkWidget *watch_download;
  GtkWidget *hash_size;
  GtkWidget *local_ip;
  GtkWidget *max_time;
  GtkWidget *use_http11;
  GtkWidget *site_level;
  GtkWidget *debug_level_mi;
  GtkWidget *debug_level_m;
  GtkWidget *http_headers;
/* for fnrules */
  GtkWidget *rules_list;
  GtkWidget *mpt_entry;
  GtkWidget *rule_entry;
  GtkWidget *ptrn_fnmatch;
  GtkWidget *ptrn_regex;
  GtkWidget *dont_touch_url_pattern;
/* end for fnrules */
#ifdef HAVE_REGEX
  GtkWidget *aip;
  GtkWidget *skipip;
  GtkWidget *rpattern;
  GtkWidget *skip_rpattern;
  GtkWidget *url_rpattern;
  GtkWidget *url_skip_rpattern;
  GtkWidget *dont_touch_url_rpattern;
  GtkWidget *dont_touch_tag_rpattern;
#endif
#ifdef HAVE_BDB_18x
  GtkWidget *ns_cache_dir;
  GtkWidget *moz_cache_dir;
#endif
#ifdef HAVE_MOZJS
  GtkWidget *pjs_console_shell;
#endif
  GtkWidget *show_time;
  GtkWidget *post_cmd;
  GtkWidget *gen_logname;
  GtkWidget *sched_cmd;
  GtkWidget *send_if_range;
  GtkWidget *remove_adv;
  GtkWidget *advert_res;
  GtkWidget *scheduling_strategie;
  GtkWidget *auto_referer;
  GtkWidget *referer;
  GtkWidget *cookie_check_domain;
  GtkWidget *remind_cmd;
  GtkWidget *sel_to_local;
  GtkWidget *all_to_local;
  GtkWidget *url_to_local;
  GtkWidget *all_to_remote;
  GtkWidget *enable_info;
  GtkWidget *enable_js;
  GtkWidget *read_css;
  GtkWidget *fs_quota;
  GtkWidget *file_quota;
  GtkWidget *trans_quota;
  GtkWidget *minrate;
  GtkWidget *maxrate;
  GtkWidget *bufsize;
  GtkWidget *use_prefs;
  GtkWidget *acharset_list;
  GtkWidget *identity;
  GtkWidget *store_index;
  GtkWidget *send_from;
  GtkWidget *check_size;
  GtkWidget *min_size;
  GtkWidget *skip_pattern;
  GtkWidget *skip_url_pattern;
  GtkWidget *ftp_html;
  GtkWidget *preserve_links;
  GtkWidget *preserve_perm;
  GtkWidget *ftp_list;
  GtkWidget *index_name;
  GtkWidget *store_name;
  GtkAccelGroup *accel_group;
  GSList *menu_gaccels;
  GtkWidget *tr_del_chr;
  GtkWidget *tr_str_s1;
  GtkWidget *tr_str_s2;
  GtkWidget *tr_chr_s1;
  GtkWidget *tr_chr_s2;
  GtkWidget *en_uexit;
  GtkWidget *html_tags;
  GtkWidget *cookie_domain_list;
  GtkWidget *cookie_domain_entry;
  GtkWidget *en_cookie_max;
  GtkWidget *sw_cookie_update;
  GtkWidget *sw_cookie_recv;
  GtkWidget *sw_cookie_send;
  GtkWidget *en_cookie_file;
  GtkWidget *cb_comcfg;
  GtkWidget *leave_level;
  GtkWidget *cfg_menu;
  GtkWidget *ptime;
  GtkWidget *leaves_sw;
  GtkWidget *leaved_sw;
  GtkWidget *resched;
  GtkWidget *btime_h_entry;
  GtkWidget *etime_h_entry;
  GtkWidget *btime_min_entry;
  GtkWidget *etime_min_entry;
  GtkWidget *btime_yentry;
  GtkWidget *etime_yentry;
  GtkWidget *btime_mon;
  GtkWidget *etime_mon;
  GtkWidget *btime_cal;
  GtkWidget *etime_cal;
  GtkWidget *btime_sw;
  GtkWidget *etime_sw;
  GtkWidget *selected_node;                      /*** selected tree node ***/
  GUI_TREE_RTYPE root;                          /*** tree root item ***/
  int endloop;                                  /*** end procesing _Xt_Serve ***/
  GtkWidget *modegr[NUM_MODES];                         /*** radio group for mode selection ***/
  GtkWidget *ftpmodegr[2];
  GtkWidget *toplevel;                          /*** jednotlive widgety ktore potrebujeme adresovat ***/
  GtkWidget *logw;
  GtkWidget *logw_menu;
  GtkWidget *logw_copy_me;
  GtkWidget *logw_swin;
  GtkAdjustment *logvadj;
  GtkWidget *scn_load_shell;
  GtkWidget *scn_add_shell;
  GtkWidget *scn_save_shell;
  GtkWidget *log_label;
  GtkWidget *slog_label;
  GtkWidget *base_level_label;
  GtkWidget *xloglen_label;
  GtkWidget *maxdoc_label;
  GtkWidget *maxlev_label;
  GtkWidget *maxsize_label;
  GtkWidget *browser_label;
  GtkWidget *ddays_label;
  GtkWidget *rollback_label;
  GtkWidget *oldrm_sw;
  GtkWidget *mime_sw;
  GtkWidget *freget_sw;
  GtkWidget *noreloc_sw;
  GtkWidget *gopher_sw;
  GtkWidget *ftp_sw;
  GtkWidget *ftpd_sw;
  GtkWidget *ftp_data_sw;
  GtkWidget *http_sw;
  GtkWidget *cache_sw;
  GtkWidget *cgi_sw;
  GtkWidget *robots_sw;
  GtkWidget *enc_sw;
  GtkWidget *auth_sw;
  GtkWidget *url_entry;
  GtkWidget *url_list;
  GtkWidget *formdata_entry;
  GtkWidget *formdata_list;
  GtkWidget *cdir_label;
  GtkWidget *http_proxyh_label;
  GtkWidget *http_proxyp_label;
  GtkWidget *http_proxy_list;
#ifdef USE_SSL
  GtkWidget *ftps_sw;
  GtkWidget *https_sw;
  GtkWidget *ssl_proxyh_label;
  GtkWidget *ssl_proxyp_label;
  GtkWidget *ssl_cipher_list;
  GtkWidget *ssl_version[4];
  GtkWidget *ssl_cert_passwd_en;
  GtkWidget *unique_sslid;
#ifdef USE_SSL_IMPL_OPENSSL
  GtkWidget *ssl_key_file_en;
  GtkWidget *ssl_cert_file_en;
#ifdef HAVE_RAND_EGD
  GtkWidget *egd_socket;
#endif
#endif
#ifdef USE_SSL_IMPL_NSS
  GtkWidget *nss_cert_dir;
  GtkWidget *nss_accept_unknown_cert;
  GtkWidget *nss_domestic_policy;
#endif
#endif
  GtkWidget *ftp_proxyh_label;
  GtkWidget *ftp_proxyp_label;
  GtkWidget *ftp_httpgw;
  GtkWidget *ftp_dirtyp;
  GtkWidget *gopher_proxyh_label;
  GtkWidget *gopher_proxyp_label;
  GtkWidget *gopher_httpgw;
  GtkWidget *retry_label;
  GtkWidget *redir_label;
  GtkWidget *reget_label;
  GtkWidget *hour_label;
  GtkWidget *min_label;
  GtkWidget *year_label;
  GtkWidget *month_combo;
  GtkWidget *auth_label;
  GtkWidget *pass_label;
  GtkWidget *auth_reuse_nonce;
  GtkWidget *proxy_auth_label;
  GtkWidget *proxy_pass_label;
  GtkWidget *auth_reuse_proxy_nonce;
  GtkWidget *proxy_auth_sw;
  GtkWidget *from_label;
  GtkWidget *subdir_label;
  GtkWidget *timeout_label;
  GtkWidget *config_shell;
  GtkWidget *about_shell;
  GtkWidget *calendar;
  GtkWidget *alanglist;
  GtkWidget *mimelist;
  GtkWidget *amimelist;
  GtkWidget *mimet_entry;
  GtkWidget *prefixlist;
  GtkWidget *prefix_label;
  GtkWidget *prefix_sw;
  GtkWidget *sufixlist;
  GtkWidget *sufix_label;
  GtkWidget *sufix_sw;
  GtkWidget *domain_sw;
  GtkWidget *domain_list;
  GtkWidget *domain_entry;
  GtkWidget *hosts_sw;
  GtkWidget *hosts_list;
  GtkWidget *hosts_entry;
  GtkWidget *pattern_label;
  GtkWidget *url_pattern_label;
  GtkWidget *sleep_label;
  GtkWidget *cfg_sch;
  GtkWidget *cfg_limits;
  GtkWidget *mbb_cfg;
  GtkWidget *mbb_mode;
  GtkWidget *bt_cfg;
  GtkWidget *bt_lim;
  GtkWidget *bt_bg;
  GtkWidget *bt_rest;
  GtkWidget *bt_start;
  GtkWidget *bt_stop;
  GtkWidget *bt_break;
  GtkWidget *bt_exit;
  GtkWidget *mea_rest;
  GtkWidget *mea_start;
  GtkWidget *mea_stop;
  GtkWidget *mea_break;
  GtkWidget *mtb_rest;
  GtkWidget *mtb_start;
  GtkWidget *mtb_stop;
  GtkWidget *mtb_break;
  GtkWidget *toolbar;
  GtkWidget *minitb_label;
  GtkWidget *me_debug;
  GtkWidget *me_quiet;
#ifdef WITH_TREE
  GtkWidget *tree_widget;
  GtkWidget *tree_shell;
  GtkWidget *tree_help;
  GtkWidget *tmenu;
  GtkWidget *me_disable_url;
  GtkWidget *me_enable_url;
  GtkWidget *me_download_url;
  GtkWidget *me_browse_url;
  GtkWidget *me_prop_url;
#endif
#ifdef WITH_TREE
  urltype_icon icon;                            /*** icons in URL tree preview ***/
#endif

#ifdef HAVE_MT
  GtkWidget *immessages;
  GtkWidget *nthr;
  GtkWidget *status_list;
#else
  GtkWidget *status_size;
  GtkWidget *status_rate;
  GtkWidget *status_et;
  GtkWidget *status_rt;
#endif

  GtkWidget *status_done;
  GtkWidget *status_queue;
  GtkWidget *status_fail;
  GtkWidget *status_rej;
  GtkWidget *status_msg;

  bool_t _go_bg;
} Gtk_nfo;

/*** gui_tools.c ***/
extern GtkWidget *guitl_timesel_new(GtkWidget **, GtkWidget **, GtkWidget **,
  GtkWidget **, GtkWidget **);
extern GtkWidget *guitl_tab_add_entry(GtkWidget *, char *, guint, guint,
  guint);
extern GtkWidget *guitl_tab_add_numentry(GtkWidget *, char *, guint, guint,
  guint);
extern GtkWidget *guitl_tab_add_doubleentry(GtkWidget *, char *, guint, guint,
  guint, guint);
extern GtkWidget *guitl_tab_add_enum(GtkWidget *, char *, guint, guint,
  const char **, guint);
extern GtkWidget *guitl_tab_add_path_entry_full(GtkWidget *, char *, guint,
  guint, int, char *);
extern GtkWidget *guitl_tab_add_path_entry(GtkWidget *, char *, guint, guint,
  int);
extern GtkWidget *guitl_new_edit_list(GtkWidget **, GtkWidget **, char *,
  GtkWidget **, GtkWidget **, GtkWidget **, GtkWidget **, gboolean,
  const char **);
extern GtkWidget *guitl_pixmap_button(char **, char *, char *);
extern Icon *guitl_load_pixmap(char **);
extern void guitl_ListDeleteSelected(GtkObject *, gpointer);
extern void guitl_ListInsertEntry(GtkObject *, gpointer);
extern void guitl_ListModifyEntry(GtkObject *, gpointer);
extern void guitl_ListClear(GtkObject *, gpointer);
extern void guitl_ListCopyToEntry(GtkObject *, int, int, GdkEvent *,
  gpointer);
extern void guitl_ListInsertList(GtkObject *, gpointer);
extern void guitl_PopdownW(GtkObject *, gpointer func_data);
extern void guitl_menu_attach(GtkWidget *, GtkWidget *);
extern GtkWidget *guitl_menu_parent(GtkWidget *);
extern void guitl_set_clipboard_content(char *);
extern void guitl_clist_selection_to_clipboard(GtkWidget *, GtkWidget *);
extern GtkWidget *guitl_toolbar_button(GtkWidget *, char *, char *, char **,
  GtkSignalFunc, gpointer, char *);
extern GtkWidget *guitl_tab_add_edit_entry(GtkWidget *, char *, char *, guint,
  guint, guint);


/*** gui_tree.c ***/
extern void gui_build_tree_preview(int);
extern gint gui_tree_list_events(GtkWidget *, GdkEvent *);
extern void gui_SelectTreeNode(GtkObject *, gpointer);

/*** gui_sched.c ***/
extern void gui_build_scheduler(int);

/*** gui_scenario.c ***/
extern void gui_build_scenario_loader(int);
extern void gui_build_scenario_adder(int);
extern void gui_build_scenario_saver(int);

/*** gui_main.c ***/
extern void gui_set_debug_level_mi(void);
extern const GtkTargetEntry dragtypes[3];
extern void gui_window_drop_url(GtkWidget *, GdkDragContext *, gint, gint,
  GtkSelectionData *, guint, guint, gpointer);
extern void gui_PopdownWC(GtkObject *, gpointer);
extern void gui_PopupW(GtkObject *, gpointer);

/*** gui_addurl.c ***/
extern void gui_build_addurl(int);

/*** gui_common.c ***/
extern void gui_build_config_common(int);

/*** gui_limits.c ***/
extern void gui_build_config_limits(int);

/*** gui_about.c ***/
extern void gui_build_about(int);

/*** gui_jscons.c ***/
extern void gui_pjs_console(int);

/******************************************/
/* global GUI configuration structructure */
/******************************************/
extern Gtk_nfo gui_cfg;

#endif
#endif
