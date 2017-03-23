/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>

#ifdef __CYGWIN__
#include <windows.h>
#endif

#include "recurse.h"
#include "http.h"
#include "ftp.h"
#include "update_links.h"
#include "mode.h"
#include "url.h"
#include "dns.h"
#include "ainterface.h"
#include "times.h"
#include "schedule.h"
#include "dlhash_tools.h"
#include "nscache.h"
#include "log.h"
#include "authinfo.h"
#include "cookie.h"
#include "net.h"
#include "gui_api.h"
#include "gui.h"
#include "myssl.h"

#define STDOUT  1

_config_struct_t cfg;

#ifdef WIN32
/*
 * read string value from HKEY_LOCAL_MACHINE
 */
char *read_lmachine_registry_val(char *path, char *var)
{
  HKEY hKey;
  char rpath[2048];
  DWORD sz = sizeof(rpath);
  DWORD type;
  char *rv = NULL;

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ,
      &hKey) != ERROR_SUCCESS)
    return NULL;

  if(RegQueryValueEx(hKey, var, NULL, &type, rpath, &sz) == ERROR_SUCCESS)
    rv = tl_strdup(rpath);

  RegCloseKey(hKey);
  return rv;
}

#endif

static char *pavuk_get_install_path(void)
{
  char *rv = NULL;
#ifdef WIN32
  rv = read_lmachine_registry_val("Software\\Stefan Ondrejicka\\Pavuk",
    "Install Path");

  if(rv)
  {
    char *p = rv;
    rv = cvt_win32_to_unix_path(p);
    _free(p);
  }
  else
    rv = tl_strdup("/cygdrive/c");
#else
#ifdef INSTALL_PREFIX
  rv = tl_strdup(INSTALL_PREFIX);
#else
  rv = tl_strdup("/usr/local");
#endif
#endif

  return rv;
}

static void pavuk_quit(int signum)
{
  printf(gettext("QUIT signal catched\n"));
  cfg.xi_face = FALSE;
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
}

static void pavuk_alarm(int signum)
{
  printf(gettext("Program has been forcefully terminated\n"));
  exit(10);
}

static void pavuk_end(int signum)
{
  signal(SIGALRM, pavuk_alarm);
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  printf(gettext("TERM signal catched\n"));
  cfg.xi_face = FALSE;
  cfg.rbreak = TRUE;
  cfg.stop = TRUE;
  alarm(1);
}

static void init_values(int argc, char **argv)
{
  char *d;
  uid_t uid;
  time_t __time = time(NULL);
  char pom[PATH_MAX];
  struct hostent *hp = NULL;
  struct passwd *pwstruct;
#if defined(GETTEXT_NLS) || defined(I_FACE)
  int i;
#endif
  memset(&cfg, '\0', sizeof(cfg));
#ifdef I_FACE
  memset(&gui_cfg, '\0', sizeof(gui_cfg));
#endif

  cfg.prg_path = argv[0];
  cfg.install_path = pavuk_get_install_path();

#ifdef HAVE_MT
  mt_init();
#endif

  init_locale_env();

#ifdef GTK_FACE
  cfg.path_to_home = tl_strdup(g_get_home_dir());
#endif
  if(!cfg.path_to_home)
    cfg.path_to_home = tl_strdup(getenv("HOME"));
  if(!cfg.path_to_home)
    cfg.path_to_home = tl_strdup("/tmp/");

#ifdef HAVE_TZSET
  tzset();
#endif

#ifdef I_FACE
  cfg.done = FALSE;
#endif
  cfg.urlstack = NULL;
  cfg.urls_in_dir = NULL;
  cfg.total_cnt = 0;
  cfg.time = new_tm(localtime(&__time));
  cfg.time->tm_year += 1900;
  cfg.fail_cnt = 0;
  cfg.docnr = 0;

  cfg.url_hash_tbl = NULL;
  cfg.fn_hash_tbl = NULL;
  cfg.last_used_proxy_node = NULL;

  cfg_setup_default();

#ifdef GETTEXT_NLS
#ifdef GETTEXT_DEFAULT_CATALOG_DIR
  cfg.msgcatd = tl_strdup(GETTEXT_DEFAULT_CATALOG_DIR);
#endif
#ifdef __CYGWIN__
  _free(cfg.msgcatd);
  cfg.msgcatd = tl_str_concat(NULL, cfg.install_path, "/share/locale", NULL);
#endif
#else
  cfg.language = "C";
#endif

  _INIT_NLS;

#ifdef SOCKS
  SOCKSinit(argv[0]);
#endif

  if(!(d = getenv("USER")))
  {
    uid = getuid();
    if((pwstruct = getpwuid(uid)))
      d = tl_strdup(pwstruct->pw_name);
  }
  else
    d = tl_strdup(d);

  if(gethostname(pom, sizeof(pom)))
  {
    perror("gethostname");
  }
  else
  {
    cfg.local_host = tl_strdup(pom);
    hp = gethostbyname(pom);
  }

  if(hp)
  {
    if(d)
      snprintf(pom, sizeof(pom), "%s@%s", d, hp->h_name);
    else
      snprintf(pom, sizeof(pom), "pavuk@%s", hp->h_name);
  }
  else
  {
    if(d)
      snprintf(pom, sizeof(pom), "%s@unknown.sk", d);
    else
      strcpy(pom, "pavuk@unknown.sk");
  }

  _free(d);
  cfg.from = tl_strdup(pom);

  cfg_load_setup();

  cfg.xi_face = FALSE;

#ifdef I_FACE
  cfg.stop = FALSE;
  cfg.rbreak = FALSE;
  for(i = 1; i < argc; i++)
  {
    /*** load preferences ***/
    if(!strcasecmp(argv[i], "-prefs"))
      cfg.use_prefs = TRUE;
    else if(!strcasecmp(argv[i], "-noprefs"))
      cfg.use_prefs = FALSE;
    /**** we want to run GUI ****/
    else if(!strcasecmp(argv[i], "-X"))
      cfg.xi_face = TRUE;
  }
  if(cfg.use_prefs && cfg.xi_face)
    cfg_load_pref();
#endif

  _INIT_NLS;

#ifdef GETTEXT_NLS
/*** these parameters have to be resolved before each other ***/
  for(i = 1; i < argc; i++)
  {
    if(!strcasecmp(argv[i], "-msgcat"))
    {
      i++;
      if(i < argc)
      {
        cfg.msgcatd = tl_strdup(argv[i]);
      }
      else
      {
        xprintf(0, gettext("Not enough number of parameters \"-msgcat\"\n"));
        usage();
      }

    }
    if(!strcasecmp(argv[i], "-language"))
    {
      i++;
      if(i < argc)
      {
        cfg.language = tl_strdup(argv[i]);
      }
      else
      {
        xprintf(0,
          gettext("Not enough number of parameters \"-scenario\"\n")); /* FIXME: Wrong string? */
        usage();
      }
    }
  }
#endif

  _INIT_NLS;

  cfg_setup_cmdln(argc, argv);

  /**** if requested, create GUI ****/
  if(cfg.xi_face)
  {
#if defined I_FACE && !defined HAVE_MT
    dns_serv_start();
#endif
    gui_start(&argc, argv);
  }


#ifdef GETTEXT_NLS
  cfg.language = tl_strdup(getenv("LC_MESSAGES"));
#endif

  if(cfg.dumpfd >= 0)
  {
    if((fcntl(cfg.dumpfd, F_GETFD) < 0) && (errno == EBADF))
    {
      xprintf(0,
        gettext("Error: Supplied bad file descriptor in -dumpfd option\n"),
        cfg.dumpfd);
      exit(PAVUK_EXIT_CFG_ERR);
    }
  }

  if(cfg.dump_urlfd >= 0)
  {
    if((fcntl(cfg.dump_urlfd, F_GETFD) < 0) && (errno == EBADF))
    {
      xprintf(0,
        gettext
        ("Error: Supplied bad file descriptor in -dump_urlfd option\n"),
        cfg.dumpfd);
      exit(PAVUK_EXIT_CFG_ERR);
    }
  }

  cfg.url_hash_tbl =
    dlhash_new(cfg.hash_size, url_key_func, url_hash_func,
    dllist_url_compare);
  dlhash_set_free_func(cfg.url_hash_tbl, url_free_func, NULL);
  cfg.fn_hash_tbl =
    dlhash_new(cfg.hash_size, fn_key_func, str_hash_func, str_comp_func);
  dlhash_set_free_func(cfg.fn_hash_tbl, NULL, NULL);

  memset(&cfg.local_ip_addr, '\0', sizeof(cfg.local_ip_addr));
  if(cfg.local_ip && net_host_to_in_addr(cfg.local_ip, &cfg.local_ip_addr))
  {
    xherror(cfg.local_ip);
  }

  if(cfg.cache_dir)
  {
    d = cfg.cache_dir;
    cfg.cache_dir = get_abs_file_path_oss(cfg.cache_dir);
    free(d);
  }
  else
  {
    getcwd(pom, sizeof(pom));
    cfg.cache_dir = tl_strdup(pom);
  }

  if(cfg.subdir)
  {
    d = cfg.subdir;
    cfg.subdir = get_abs_file_path_oss(cfg.subdir);
    free(d);
    if(tl_is_dirname(cfg.subdir))
      *(cfg.subdir + strlen(cfg.subdir) - 1) = '\0';
  }

  cfg.prev_mode = cfg.mode;

  if(cfg.save_scn)
  {
    if(cfg.scndir)
    {
      snprintf(pom, sizeof(pom), "%s/%s", cfg.scndir, cfg.save_scn);
      cfg_dump(pom);
    }
    else
    {
      xprintf(0,
        gettext
        ("WARNING: scndir not specified - saving to current directory\n"));
      cfg_dump(cfg.save_scn);
    }
    exit(PAVUK_EXIT_OK);
  }

  if(cfg.cookie_file)
    cookie_read_file(cfg.cookie_file);

  if(cfg.auth_file)
    authinfo_load(cfg.auth_file);

  log_start(cfg.logfile);

  if(!cfg.sched_cmd)
    cfg.sched_cmd = tl_strdup(AT_CMD);

  if(cfg.schtime)
  {
    _free(cfg.time);
    cfg.time = new_tm(localtime(&cfg.schtime));
  }

  if(!cfg.index_name)
    cfg.index_name = tl_strdup("_._.html");

#ifdef USE_SSL
  my_ssl_init_once();
#endif

  if(cfg.bgmode)
  {
    pid_t ppid;

    ppid = fork();
    if(ppid < 0)
    {
      xperror("fork");
      xprintf(1,
        gettext
        ("Unable to fork pavuk to background - running in foreground\n"));
    }
    else if(ppid != 0)
    {
      xprintf(0, gettext("Pavuk will run at backround as PID %d\n"),
        (int) ppid);
      exit(PAVUK_EXIT_CFG_ERR);
    }
#ifdef __CYGWIN__
    FreeConsole();
#endif
  }
}

static void read_urls(char *filename)
{
  bufio *fd;
  char lnbuf[4096];
  int n;
  bool_t isstdin;


  isstdin = !strcmp(cfg.urls_file, "-");

  DEBUG_MISC(gettext("reading URLs from file - %s\n"), filename);
  if(isstdin)
    fd = bufio_fdopen(0);
  else
    fd = bufio_open(filename, O_BINARY | O_RDONLY);

  if(!fd)
  {
    xperror(filename);
    return;
  }

  while((n = bufio_readln(fd, lnbuf, sizeof(lnbuf))) > 0)
  {
    strip_nl(lnbuf);
    if(!strcmp(lnbuf, "."))
      break;

    if(lnbuf[0])
    {
      url_info *ui;

      ui = url_info_new(lnbuf);
      cfg.request = dllist_append(cfg.request, (dllist_t) ui);
    }
  }

  if(n < 0)
    xperror("reading stdin");

  if(isstdin)
    bufio_free(fd);
  else
    bufio_close(fd);
}

int main(int argc, char **argv)
{
  time_t __time = time(NULL);

  init_values(argc, argv);
  atexit(pavuk_do_at_exit);

  /*
     pro: We do not fully trust pavuk to stop working after the timeout
     expired, so we order the OS to send us an ALARM signal one minute
     after the timeout expires. If the program still runs at that time
     it will be aborted once the signal strikes home.
   */
  if(cfg.max_time > 0)
  {
    alarm((int) ((cfg.max_time + 1) * 60));
  }

  if(cfg.urls_file)
  {
    read_urls(cfg.urls_file);
    _free(cfg.urls_file);
  }

  /*
     pro: Set seed for random generator; needed to find a port for
     active ftp.
   */
  srand(time(NULL) ^ getpid());

  signal(SIGINT, pavuk_end);
  signal(SIGTERM, pavuk_end);
  signal(SIGALRM, pavuk_end);
  signal(SIGPIPE, SIG_IGN);
/**** spustenie algoritmu alebo rozhrania ****/
/**** FIXME: Translate me!                ****/
  if(cfg.xi_face)
  {
    cfg.prev_mode = cfg.mode;
    cfg.mode_started = FALSE;

    gui_main();
  }
  else
  {
    signal(SIGQUIT, pavuk_quit);

    if(cfg.schtime)
    {
      cfg.schtime = (time_t) 0;
      if(at_schedule())
      {
        xprintf(0, gettext("Error scheduling\n"));
      }
    }
    else
    {
      if(cfg.reschedh)
      {
        __time += 3600 * cfg.reschedh;
        _free(cfg.time);
        cfg.time = new_tm(localtime(&__time));
        at_schedule();
      }
      absi_restart();
    }
  }

  log_start(NULL);

  print_all_url_infos();
  print_all_dns_infos();
  
  return cfg.fail_cnt ? PAVUK_EXIT_DOC_ERR : PAVUK_EXIT_OK;
}
