/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _config_h_
#define _config_h_

#include "../ac-config.h"
typedef DLLISTTYPE dllist_t;

#define NeedFunctionPrototypes 1

#ifdef NEED_DECLARE_H_ERRNO
extern int h_errno;
#endif

#if !defined(HAVE_LONG_FILE_NAMES) && defined(__GNUC__)
#warning "This program can't run successfuly on machine without long filenames support"
#endif

#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <sys/time.h>
#include <unistd.h>

#include "mode.h"

#if defined(I_FACE) && !defined(HAVE_MT)
#define _Xt_Serve       gui_loop_serve()
#define _Xt_EscLoop     gui_loop_escape()
#define _Xt_ServeLoop   gui_loop_do()
#else
#define _Xt_Serve
#define _Xt_EscLoop
#define _Xt_ServeLoop
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include <limits.h>
#ifndef PATH_MAX
#ifdef FILENAME_MAX
#define PATH_MAX FILENAME_MAX
#else
#define PATH_MAX        2048
#endif
#endif

#ifndef NAME_MAX
#ifdef MAXNAMLEN
#define NAME_MAX MAXNAMLEN
#else
#define NAME_MAX 256
#endif
#endif

/********************************************************/
/* the folowing is to workaround systems which defines  */
/* unreal limits on filenames                           */
/********************************************************/
#if defined(HAVE_LONG_FILE_NAMES) && NAME_MAX < 16
#undef NAME_MAX
#define NAME_MAX 256
#endif

#if defined(HAVE_LONG_FILE_NAMES) && NAME_MAX < 1024
#undef PATH_MAX
#define PATH_MAX 2048
#endif

#ifndef INT_MAX
#define INT_MAX         2147483647
#endif
#ifndef USHRT_MAX
#define USHRT_MAX       65535
#endif

#ifdef HAVE_SOCKS_H
#include <socks.h>
#elif defined(SOCKS)
int SOCKSinit (char *);
#define connect         Rconnect
int Rconnect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
#define bind            Rbind
int Rbind(int  sockfd, struct sockaddr *my_addr, socklen_t addrlen);
#define accept          Raccept
int Raccept(int s, struct sockaddr *addr, socklen_t *addrlen);
#define listen          Rlisten
int Rlisten(int s, int backlog);
#define select          Rselect
int  Rselect(int  n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
#define gethostbyname   Rgethostbyname
struct hostent *Rgethostbyname(const char *name);
#define getsockname     Rgetsockname
int Rgetsockname(int s, struct sockaddr  *name, socklen_t *namelen);
#endif /* SOCKS */

#include "mt.h"
#include "debugl.h"
#include "condition.h"
#include "nls.h"
#include "dllist.h"
#include "dlhash.h"
#include "http.h"
#include "dns.h"

#define HASH_TBL_SIZE   233

typedef enum
{
  SSTRAT_DO_SIRKY,
  SSTRAT_DO_SIRKY_I,
  SSTRAT_DO_HLBKY,
  SSTRAT_DO_HLBKY_I,
  SSTRAT_LAST
} strategie;

typedef struct
{
/*** CMDline parameters ***/
  char *default_prefix;         /*** default prefix used in filename_to_url when nondefault tree layout is used ***/
  char *info_dir;               /*** directory where are info files stored instead of regulary to -cdir ***/
  char *urls_file;              /*** urls will be read from this file ***/
  char *cookie_file;            /*** cookie file in NS format ***/
  char *auth_file;              /*** file for authentification informations ***/
  char *save_scn;               /*** name for scenario saving ***/
  char *scndir;                 /*** directory where scenarios are stored ***/
  char *scenario;               /*** scenario file to load ***/
  char *subdir;                 /*** subdirectory of cache_dir to focus  ***/
  char *cache_dir;              /*** dir where your local tree is located ***/
  char *logfile;                /*** logging file ***/
  char *short_logfile;          /*** newstyle log file name ***/
  char *time_logfile;           /*** time log file name ***/
  bool_t sdemo_mode;            /*** sdemo compaible output ***/
  bool_t noencode;              /*** don't do RFC 2396 character escaping ***/
  char *time_relative;          /*** what timings are relative to ***/
  http_proxy *transparent_proxy;     /*** list of transparent proxy servers ***/
  http_proxy *transparent_ssl_proxy; /*** list of transparent SSL proxy servers ***/
  char *remind_cmd;             /*** command for reminding changed URLS ***/
  char *sched_cmd;              /*** scheduling command ***/
  char *post_cmd;               /*** command for post downloading processing of files ***/
  char *stats_file;             /*** status file ***/

  long dumpfd;                  /*** number of filedescriptor for dumping documents ***/
  long dump_urlfd;              /*** number of filedescriptor for dumping all URLs ***/
  long hash_size;               /*** hash tables size ***/
  long trans_quota;             /*** transfer quota ***/
  long file_quota;              /*** file size quota ***/
  long fs_quota;                /*** filesystem quota ***/
  long bufsize;                 /*** size of read buffer ***/
  long base_level;              /*** base tree level from which is built local file tree ***/
  long nretry;                  /*** number of retries on error ***/
  long nreget;                  /*** number of regets ***/
  long nredir;                  /*** max number of redirections ***/
  long ddays;                   /*** delay in days for document reload ***/
  long rollback;                /*** size to go back when reget-ing ***/
  long sleep;                   /*** sleeptime between transfers ***/
  long cookies_max;             /*** maximal number of cookies ***/
  long reschedh;                /*** number of hours for rescheduling ***/
  double max_time;              /*** maximal time for run ***/
  pavuk_mode mode;              /*** working mode ***/
  struct tm *time;              /*** when to schedule execution time ***/
  time_t schtime;               /*** cmdln param for scheduling time ***/
  strategie scheduling_strategie;
                                /*** url downloading scheduling strategie ***/

  char *ftp_proxy_pass;         /*** password for access to FTP proxy ***/
  char *ftp_proxy_user;         /*** username for access to FTP proxy ***/
  char *http_proxy_pass;        /*** password for access to HTTP proxy ***/
  char *http_proxy_user;        /*** username for access to HTTP proxy ***/
  long proxy_auth_scheme;       /*** authorization scheme for proxy ***/
  char *ftp_proxy;              /*** FTP proxy server ***/
  long ftp_proxy_port;
  bool_t ftp_via_http;          /*** gatewaying FTP through HTTP proxy ***/
  bool_t ftp_dirtyp;            /*** dirty FTP proxying via CONNECT request to HTTP proxy ***/
  long active_ftp_min_port;     /*** minimum port for active ftp ***/
  long active_ftp_max_port;     /*** minimum port for active ftp ***/

  char *gopher_proxy;           /*** Gopher proxy ***/
  long gopher_proxy_port;
  bool_t gopher_via_http;       /*** gatewaying Gopher trough HTTP proxy ***/
  char *name_auth;              /*** meno pouzivatela pri HTTP autentifikacii ***/
  char *passwd_auth;            /*** password for HTTP authorization ***/
  long auth_scheme;              /*** authorization scheme 1- user, 2- Basic, 3- Digest ***/

  bool_t rsleep;                /*** randomize sleep period up to -sleep ***/
  bool_t hack_add_index;        /*** add also directories of all files to queue ***/
  bool_t post_update;           /*** update in parent documents only curently downloaded URLs ***/
  bool_t fix_wuftpd;            /*** use STAT -d to test existence of directory
                                     when using LIST, because wuftpd does not raise
                                     error when listing non existing directory ***/
  bool_t retrieve_slink;        /*** retrieve symliks like regular files ***/
  bool_t dump_resp;             /*** when -dumpfd is used, dump also HTTP response header ***/
  bool_t dump_after;            /*** when -dumpfd is used, dump document just
                                     after successfull download and after
                                     processing of HTML documents ***/
  bool_t xi_face;               /*** requested GUI interface ***/
  bool_t singlepage;            /*** download single HTML page with all inline objecs ***/
  bool_t unique_doc;            /*** always try to generate unique names for documents ***/
  bool_t del_after;             /*** delete transfered files after successfull transfer ***/
  bool_t use_http11;            /*** enable using of HTTP/1.1 protocol version ***/
  bool_t gen_logname;           /*** generate nummbered log names when original locked ***/
  bool_t send_if_range;         /*** send If-Range: header in HTTP request ***/
  bool_t auto_referer;          /*** send own URL as referer with starting URLs ***/
  bool_t referer;               /*** referer field for requests ***/
  bool_t all_to_remote;         /*** change all links to remote and don't do any further changes to it ***/
  bool_t sel_to_local;          /*** change links, which acomplish limits, to local immediately ***/
  bool_t all_to_local;          /*** change all links to local immediately ***/
  bool_t enable_info;           /*** enable using and creating info files ***/
  bool_t enable_js;             /*** enable javascript ***/
  bool_t bgmode;                /*** run at background ***/
  bool_t store_index;           /*** store directory URLS as index files ***/
  bool_t send_from;             /*** send From: header in request ***/
  bool_t check_size;            /*** some broken HTTP servers send wrong Content-Length: header ***/
  bool_t htdig;                 /*** to dump HTTP response - used by htDig ***/
  bool_t send_cookies;          /*** send available cookies in HTTP request ***/
  bool_t recv_cookies;          /*** receive cookie infos ***/
  bool_t update_cookies;        /*** update cookie file ***/
  bool_t cookie_check_domain;   /*** check if server sets cookie for own domain ***/
  bool_t preserve_time;         /*** preserve time of remote document ***/
  bool_t preserve_perm;         /*** preserve permisions of remote document ***/
  bool_t preserve_links;        /*** preserve absolute symlinks ***/
  bool_t quiet;                 /*** ? output messages ***/
  bool_t auth_reuse_nonce;      /*** reuse HTTP Digest authorization nonce ***/
  bool_t auth_reuse_proxy_nonce;/*** reuse HTTP proxy Digest authorization nonce ***/
  bool_t show_time;             /*** show start and end time of download ? */
  bool_t remove_old;            /*** remove old documents (when not occurrs on remote site) ***/
  bool_t remove_before_store;   /*** pro: remove document before storing it ***/
  bool_t always_mdtm;           /*** always use MDTM: no cached values ***/
  bool_t progres;               /*** show retrieving progres , when on console ***/
  bool_t ftp_activec;           /*** use active FTP data connection instead of passive ***/
  bool_t ftp_list;              /*** retrieve FTP directory with LIST cmd insted of NLST ***/
  bool_t ftp_html;              /*** process HTML files downloaded over FTP protocol ***/
  bool_t cache;                 /*** disallow caching of HTTP documents (on proxy cache) ***/
  bool_t rewrite_links;         /*** indikacia ci ma maju byt odkazy v HTML dokumentoch prepisovane ***/
  bool_t freget;                /*** force reget whole file when server doesn't support reget ***/
  bool_t use_enc;               /*** indikacia ci sa ma pouzivat gzip/compress kodovanie pri prenose ***/
  bool_t read_css;              /*** fetch objects refed in css ***/
  char *ftp_list_options;       /*** aditional options to FTP LIST/NLST commands ***/
  char *auth_ntlm_domain;       /*** domain name for NTLM authorization ***/
  char *auth_proxy_ntlm_domain; /*** domain name for NTLM proxy authorization ***/
  char *local_ip;               /*** address for local network interface ***/
  char *index_name;             /*** name of directory index file instead of _._.html ***/
  char *store_name;             /*** filename of document transfered with -mode singlepage ***/
  char *from;                   /*** HTTP request field From: or anonymous FTP password ***/
  char *identity;               /*** User-agent: field contents ***/
  char **accept_lang;           /*** list of preffered languages ***/
  char **accept_chars;          /*** list of preffered character sets ***/
  char **cookies_disabled_domains;
                                /*** domains from which cookies are not acceptable ***/
  char **dont_touch_url_pattern;/*** to allow preserve some URLs in the original form ***/

  cond condition;               /*** structure which contains all limiting conditions ***/
  dllist *request;              /*** list of urls entered by user ***/
  dllist *formdata;             /*** data for forms found during document tree traversing ***/
  dllist *lfnames;              /*** list of filename conversion rules ***/
  dllist *http_headers;         /*** list of additional HTTP headers ***/
  dllist *http_proxy;           /*** list of HTTP proxy servers ***/
  dllist *ftp_login_hs;         /*** list for -ftp_login_handshake ***/

  char *tr_del_chr;             /*** set of characters to delete while doing name transformation ***/
  char *tr_str_s1;              /*** strfrom in transformation ***/
  char *tr_str_s2;              /*** strto in transformation ***/
  char *tr_chr_s1;              /*** setfrom in transformation ***/
  char *tr_chr_s2;              /*** setto in transformation ***/

  double maxrate;               /*** maximal transfer rate ***/
  double minrate;               /*** minimal transfer rate ***/
  double ctimeout;              /*** timeout for network communication ***/

#ifdef HAVE_MOZJS
  char *js_script_file;         /*** file which contains JavaScript script with functions declarations ***/
#endif

#ifdef HAVE_BDB_18x
  char *ns_cache_dir;           /*** directory for Netscape cache ***/
#endif
  char *moz_cache_dir;          /*** directory for Mozilla cache ***/

#ifdef HAVE_MT
  long nthr;                    /*** configured number of running threads ***/
  bool_t immessages;            /*** print messages immediatly when produced not just when it is safe ***/
#endif

#ifdef __CYGWIN__
  bool_t ie_cache;              /*** possibily load files from MSIE cache directory */
  bool_t wait_on_exit;          /*** this option is for WIN32 CLI version ***/
#endif

#ifdef HAVE_TERMIOS
  bool_t tccheck;               /*** checking of we are at foreground ***/
#endif

#ifdef HAVE_REGEX
  dllist *js_patterns;          /*** matching patterns for JS URLs ***/
  dllist *js_transform;         /*** matching patterns for JS with transform **/
  dllist *advert_res;           /*** list of RE-s for advertisement banners ***/
  bool_t remove_adv;            /*** enable / disable advertisement banners ***/

  dllist *dont_touch_url_rpattern;
  dllist *dont_touch_tag_rpattern;
                                /*** to allow preserve some URLs in the original form ***/
#endif

#ifdef DEBUG
  bool_t debug;                 /*** debug mode on/off ***/
  long debug_level;             /*** debug level ***/
#endif

#ifdef USE_SSL
  long ssl_version;             /*** ssl2/ssl3/ssl23/tls1 version of ssl_client_method() ***/
  char *ssl_proxy;              /*** SSL tuneling proxy ***/
  long ssl_proxy_port;
  char *ssl_cipher_list;
  char *ssl_cert_passwd;
  bool_t unique_sslid;          /*** use unique SSL IDs with each SSL connection ***/
#ifdef USE_SSL_IMPL_OPENSSL
  char *ssl_cert_file;
  char *ssl_key_file;
  char *egd_socket;             /*** path to EGD socket ***/
#endif
#ifdef USE_SSL_IMPL_NSS
  char *nss_cert_dir;           /*** certDir for Netscape NSS ***/
  bool_t nss_accept_unknown_cert;        /*** don't care much about certificates ***/
  bool_t nss_domestic_policy;
#endif
#endif

  char *language;               /*** language for LC_MESSAGES ***/

#ifdef GETTEXT_NLS
  char *msgcatd;                /*** explicit message catalog directory ***/
#endif

#ifdef I_FACE
  char *fontname;               /*** default font used in interface ***/
  long xlogsize;                /*** max number of lines in LOG widget ***/
  bool_t log_autoscroll;        /*** autoscroll of log window ***/
  bool_t run_iface;             /*** if immediately run download after start of pavuk in GUI interface ***/
  bool_t use_prefs;             /*** store & load prefernces from ~/.pavuk_prefs file ***/

                                /*** alternative icons for GUI ***/
  char *bt_icon_cfg;
  char *bt_icon_cfg_s;
  char *bt_icon_lim;
  char *bt_icon_lim_s;
  char *bt_icon_gobg;
  char *bt_icon_gobg_s;
  char *bt_icon_rest;
  char *bt_icon_rest_s;
  char *bt_icon_cont;
  char *bt_icon_cont_s;
  char *bt_icon_stop;
  char *bt_icon_stop_s;
  char *bt_icon_brk;
  char *bt_icon_brk_s;
  char *bt_icon_exit;
  char *bt_icon_exit_s;
  char *bt_icon_mtb;
  char *bt_icon_mtb_s;

#ifdef WITH_TREE
  char *browser;                /*** command to execute your preffered browser ***/
#endif
#endif

/*** GLOBALdata ***/
  abs_addr local_ip_addr;       /*** numeric address for local network interface ***/
  time_t start_time;            /*** start time of downloading ***/
  struct timeval hr_start_time; /*** high-resolution start time of downloading ***/
  long trans_size;              /*** transfered size in session ***/
  char *path_to_home;
  char *local_host;             /*** hostname of local machine ***/
  long fail_cnt;                /*** counter for failed transfers ---> return code of pavuk ***/
  char *prg_path;               /*** path to pavuk executable == argv[0] ***/
  char *install_path;           /*** pavuk install path especialy used in win32 version ***/
  long total_cnt;               /*** total number of URLs in queue  ***/
  long process_cnt;             /*** number of already processed documents ***/
  long reject_cnt;              /*** number of rejected URLs ***/
  pavuk_mode prev_mode;         /*** previous active mode ***/
  bool_t mode_started;          /*** mode startup finisched ***/
  bool_t rbreak;                /*** immediately stop transfer ***/
  bool_t stop;                  /*** stop after this document will be processed ***/

  dllist *urlstack;             /*** list of URLs in processing queue ***/
  dllist *urls_in_dir;          /*** list of URLs extracted from mirroring
                                     directory, for checking for nonexistent
                                     document removal ***/
  dlhash *url_hash_tbl;         /*** hash table for better performance URL lookup ***/
  dlhash *fn_hash_tbl;          /*** hash table for better performance filename lookup ***/
  dllist *last_used_proxy_node; /*** pointer to last used proxy node ***/
  long docnr;                   /*** current number of document ***/

#ifdef HAVE_MT
  time_t timestamp;
  time_t cfg_changed;           /*** timestamp for cfg struct last change ***/
  pthread_key_t currdoc_key;
  pthread_key_t herrno_key;
  pthread_key_t thrnr_key;
  pthread_key_t privcfg_key;
  mt_semaphore nrunning_sem;
  mt_semaphore urlstack_sem;
  pthread_t mainthread;
  pthread_t *allthreads;
  long allthreadsnr;
#endif

#ifdef I_FACE
  bool_t done;                  /*** was done startup ? ***/
  bool_t processing;            /*** some URL is actualy in processing ***/
#endif
} _config_struct_t;

extern _config_struct_t cfg;

#if defined(HAVE_MT) && defined(I_FACE)

#if 0
/********************************************************************/
/* this structure contains corresponding field form _config_struct  */
/* structure. when I don't want to use mutex(es) for locking of     */
/* config structure when running multiple downloading threads, I    */
/* I have to make copy of dynamicaly created config parameters to   */
/* prevent segfaults when changing configuration from GUI           */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/* !!!!!!!!!! Not realy used, only to know which fields !!!!!!!!!!! */
/* !!!!!!!!!! are used from private copy                !!!!!!!!!!! */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/********************************************************************/
typedef struct
{
  time_t timestamp;
  char *default_prefix;
  char *info_dir;
  char *subdir;
  char *cache_dir;
  char *post_cmd;
  char *http_proxy_pass;
  char *http_proxy_user;
  char *ftp_proxy_pass;
  char *ftp_proxy_user;
  char *ftp_proxy;
  char *gopher_proxy;
  char *name_auth;
  char *passwd_auth;
  char *index_name;
  char *store_name;
  char *from;
  char *identity;
  char *auth_ntlm_domain;
  char *auth_proxy_ntlm_domain;
  char *ftp_list_options;

  char **accept_lang;
  char **accept_chars;
  char **cookies_disabled_domains;
  char **dont_touch_url_pattern;

  cond condition;
  dllist *formdata;
  dllist *lfnames;
  dllist *http_headers;
  dllist *js_patterns;
  dllist *js_transform;
  dllist *ftp_login_hs;

  char *tr_del_chr;
  char *tr_str_s1;
  char *tr_str_s2;
  char *tr_chr_s1;
  char *tr_chr_s2;

#ifdef HAVE_REGEX
  dllist *advert_res;
  dllist *js_patterns;
  dllist *js_transform;
  dllist *dont_touch_url_rpattern;
  dllist *dont_touch_tag_rpattern;
#endif

#ifdef HAVE_MOZJS
  char *js_script_file;
#endif

#ifdef HAVE_BDB_18x
  char *ns_cache_dir;
  char *moz_cache_dir;
#endif

#ifdef USE_SSL
  char *ssl_proxy;
  char *ssl_cipher_list;
  char *ssl_cert_file;
  char *ssl_key_file;
  char *ssl_cert_passwd;
  char *egd_socket;
#endif
} _config_struct_priv_t;
#endif /* 0 */

#define _config_struct_priv_t _config_struct_t

extern void privcfg_make_copy(_config_struct_priv_t *);
extern void privcfg_free(_config_struct_priv_t *);

#define priv_cfg (*((_config_struct_priv_t *)pthread_getspecific(cfg.privcfg_key)))
#define _MT_CFGSTAMP    cfg.cfg_changed = time(NULL)
#else
#define priv_cfg cfg
#define _MT_CFGSTAMP
#endif

typedef enum
{
  PARAM_NUM,                    /* integer number                       */
  PARAM_PBOOL,                  /* positive bool_tean                   */
  PARAM_NBOOL,                  /* negative bool_tean                   */
  PARAM_STR,                    /* single string                        */
  PARAM_PASS,                   /* password string                      */
  PARAM_STRLIST,                /* comma separated list of strings      */
  PARAM_CONN,                   /* connection - host[:port]             */
  PARAM_AUTHSCH,                /* authorization scheme - 1/2/3         */
  PARAM_MODE,                   /* operation mode - mode.c              */
  PARAM_PATH,                   /* file/dir path                        */
  PARAM_TIME,                   /* time string - YYYY.MM.DD.HH:mm       */
  PARAM_HTMLTAG,                /* HTML tags specification              */
  PARAM_TWO_QSTR,               /* two quoted strings                   */
  PARAM_DOUBLE,                 /* double number                        */
  PARAM_LFNAME,                 /* for -fnrules option                  */
  PARAM_RE,                     /* list of regular expressions          */
  PARAM_USTRAT,                 /* url strategie - -strategie           */
  PARAM_SSLVER,                 /* ssl version - ssl23/ssl2/ssl3/tls1   */
  PARAM_HTTPHDR,                /* additional HTTP header               */
  PARAM_DEBUGL,                 /* debug level - debugl.c               */
  PARAM_REQUEST,                /* extended request specification       */
  PARAM_PROXY,                  /* proxy specification - host:port      */
  PARAM_TRANSPARENT,            /* proxy specification - host:port      */
  PARAM_FUNC,                   /* exec function for this param type    */
  PARAM_JSTRANS,                /* for -js_transform option             */
  PARAM_NUMLIST,                /* list of integer numbers -[ad]port    */
  PARAM_FTPHS,                  /* for FTP -ftp_login_handshake         */
  PARAM_TAGPAT,                 /* for HTML tag patterns                */
  PARAM_PORT_RANGE              /* for TCP/IP port ranges               */
} par_type_t;

/* this is to support parameters of foreign libraries (like gtk) */
#define PARAM_FOREIGN           (1 << 29)

/* this is for marking option as unsupported in current compile time    */
/* configuration. This will allow to accept unsupported option on       */
/* commandline just throwing warning insted of trowing error and exit.  */
#define PARAM_UNSUPPORTED       (1 << 30)


typedef struct _cfg_param
{
  char *short_cmd;
  char *long_cmd;
  char *par_entry;
  par_type_t type;
  void *default_val;
  void *val_adr;
  void *mdefault_val;
  void *mval_adr;
  char *help;
} cfg_param_t;

extern char *get_strategie_label(strategie);
extern void usage(void);
extern void usage_short(void);
extern void cfg_setup_default(void);
extern void cfg_set_all_to_default(void);
extern void cfg_setup_cmdln(int, char **);
extern int cfg_load(const char *);
extern void cfg_load_setup(void);
extern int cfg_dump(const char *);
extern int cfg_dump_cmd(const char *);
extern int cfg_dump_cmd_fd(int);
extern void cfg_free_params(void);
extern int cfg_dump_pref(void);
extern int cfg_load_pref(void);
extern int cfg_get_num_params(cfg_param_t *);

extern void pavuk_do_at_exit(void);

#define PAVUK_EXIT_OK           0       /* everything goes as expected  */
#define PAVUK_EXIT_CFG_ERR      1       /* configuration error          */
#define PAVUK_EXIT_DOC_ERR      2       /* some of documents failed     */

#endif
