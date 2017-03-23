/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

static double _default_maxrate = 0.0f;
static double _default_minrate = 0.0f;
static double _default_timeout = 0.0f;
static double _default_maxtime = 0.0f;


static cfg_param_t params[] = {
#ifdef GTK_FACE
/* internal gtk options which we must accept without complaining */
/* because we are going to parse commandline before gtk_init()   */
  {
      NULL,
      "-gtk-module",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-g-fatal-warnings",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 0,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-gtk-debug",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-gtk-no-debug",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-gdk-debug",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-gdk-no-debug",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-display",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-sync",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 0,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-no-xshm",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 0,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-name",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-class",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-gxid_host",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-gxid_port",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-xim-preedit",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-xim-status",
      NULL,
      PARAM_FUNC | PARAM_FOREIGN,
      (void *) 1,
      NULL,
      NULL,
      NULL,
      NULL
    },
#endif
  {
      "v",
      "-version",
      NULL,
      PARAM_FUNC,
      (void *) 0,
      cfg_version_info,
      NULL,
      NULL,
      gettext_nop(
        "\t-v                 - print version number\n")
    },
  {
      "h",
      "-help",
      NULL,
      PARAM_FUNC,
      (void *) 0,
      usage,
      NULL,
      NULL,
      gettext_nop(
        "\t-h                 - help\n")
    },
  {
      "X",
      "-with_gui",
      NULL,
#ifndef I_FACE
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.xi_face,
      NULL,
      NULL,
      gettext_nop(
        "\t-X                 - start GUI interface\n")
#endif
    },
  {
      "x",
      NULL,
      NULL,
#ifndef I_FACE
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.xi_face,
      NULL,
      NULL,
      NULL
#endif
    },

  {
      NULL,
      "-runX",
      "RunX:",
#ifndef I_FACE
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.run_iface,
      NULL,
      NULL,
      gettext_nop(
        "\t-runX              - after start of GUI interface, immediately\n"
        "\t                     start processing of entered URLs\n")
#endif
    },
  {
      NULL,
      "-prefs",
      "UsePreferences:",
#ifndef I_FACE
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.use_prefs,
      NULL,
      NULL,
      gettext_nop(
        "\t-prefs/-noprefs    - load preferences from ~/.pavuk_prefs file\n")
#endif
    },
  {
      NULL,
      "-noprefs",
      NULL,
#ifndef I_FACE
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.use_prefs,
      NULL,
      NULL,
      NULL
#endif
    },
#ifdef I_FACE
#ifdef GTK_FACE
  {
      NULL,
      NULL,
      "BtnConfigureIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_cfg,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnConfigureIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_cfg_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnLimitsIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_lim,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnLimitsIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_lim_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnGoBgIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_gobg,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnGoBgIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_gobg_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnRestartIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_rest,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnRestartIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_rest_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnContinueIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_cont,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnContinueIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_cont_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnStopIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_stop,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnStopIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_stop_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnBreakIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_brk,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnBreakIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_brk_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnExitIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_exit,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnExitIcon_s:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_exit_s,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnMinimizeIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_mtb,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      NULL,
      "BtnMaximizeIcon:",
      PARAM_STR,
      (void *) NULL,
      &cfg.bt_icon_mtb_s,
      NULL,
      NULL,
      NULL
    },
#endif
#endif
  {
      NULL,
      "-progress",
      "ShowProgress:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.progres,
      NULL,
      NULL,
      gettext_nop(
        "\t-progress/-noprogress\n"
        "\t                   - show retrieving progress while running on terminal\n")
    },
  {
      NULL,
      "-noprogress",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.progres,
      NULL,
      NULL,
      NULL
    },
  {
      "l",
      "-lmax",
      "MaxLevel:",
      PARAM_NUM,
      (void *) 0,
      &cfg.condition.max_levels,
      NULL,
      NULL,
      gettext_nop(
        "\t-lmax $nr          - allowed depth of tree\n")
    },
  {
      NULL,
      "-dmax",
      "MaxDocs:",
      PARAM_NUM,
      (void *) 0,
      &cfg.condition.max_documents,
      NULL,
      NULL,
      gettext_nop(
        "\t-dmax $nr          - maximal number of downloaded documents\n")
    },
  {
      NULL,
      "-sleep",
      "SleepBetween:",
      PARAM_NUM,
      (void *) 0,
      &cfg.sleep,
      NULL,
      NULL,
      gettext_nop(
        "\t-sleep $nr         - sleep for $nr seconds between transfers,\n"
        "\t                     default 0 seconds\n")
    },
  {
      NULL,
      "-rsleep",
      "RandomizeSleepPeriod:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.rsleep,
      NULL,
      NULL,
      gettext_nop(
        "\t-rsleep/-norsleep  - randomize sleeping time between transfers\n"
        "\t                     from 0 to -sleep time\n")
    },
  {
      NULL,
      "-norsleep",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.rsleep,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-retry",
      "MaxRetry:",
      PARAM_NUM,
      (void *) 2,
      &cfg.nretry,
      NULL,
      NULL,
      gettext_nop(
        "\t-retry $nr         - number of retries if anything failed\n")
    },
  {
      NULL,
      "-nregets",
      "MaxRegets:",
      PARAM_NUM,
      (void *) 2,
      &cfg.nreget,
      NULL,
      NULL,
      gettext_nop(
        "\t-nregets $nr       - max number of regets on single file, default 2\n")
    },
  {
      NULL,
      "-nredirs",
      "MaxRedirections:",
      PARAM_NUM,
      (void *) 5,
      &cfg.nredir,
      NULL,
      NULL,
      gettext_nop(
        "\t-nredirs $nr       - max number of followed HTTP redirections, default 5\n")
    },
  {
      NULL,
      "-timeout",
      "CommTimeout:",
      PARAM_DOUBLE,
      (void *) &_default_timeout,
      &cfg.ctimeout,
      NULL,
      NULL,
      gettext_nop(
        "\t-timeout $nr       - timeout for network communications (sec).\n"
        "\t                     0 == no timeout, default = 0\n")
    },
  {
      NULL,
      "-rollback",
      "RegetRollbackAmount:",
      PARAM_NUM,
      (void *) 0,
      &cfg.rollback,
      NULL,
      NULL,
      gettext_nop(
        "\t-rollback $nr      - number of bytes to discard (counted from end\n"
        "\t                     of file) if regetting, default 0\n")

    },
  {
      NULL,
      "-ddays",
      "DocExpiration:",
      PARAM_NUM,
      (void *) 0,
      &cfg.ddays,
      NULL,
      NULL,
      gettext_nop(
        "\t-ddays $nr         - number of days since last access when document is\n"
        "\t                     checked in sync mode\n")
    },
  {
      NULL,
      "-nocache",
      "UseCache:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.cache,
      NULL,
      NULL,
      gettext_nop(
        "\t-nocache/-cache    - disallow caching of HTTP documents (on proxy cache)\n")
    },
  {
      NULL,
      "-cache",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.cache,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noRobots",
      "UseRobots:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.allow_robots,
      NULL,
      NULL,
      gettext_nop(
        "\t-noRobots/-Robots  - care about \"robots.txt\" file ?\n")
    },
  {
      NULL,
      "-Robots",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.allow_robots,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noFTP",
      "AllowFTP:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.ftp,
      NULL,
      NULL,
      gettext_nop(
        "\t-noFTP/-FTP        - don't download FTP files\n")
    },
  {
      NULL,
      "-FTP",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.ftp,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noHTTP",
      "AllowHTTP:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.http,
      NULL,
      NULL,
      gettext_nop(
        "\t-noHTTP/-HTTP      - don't download HTTP files\n")
    },
  {
      NULL,
      "-HTTP",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.http,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noSSL",
      "AllowSSL:",
#ifndef USE_SSL
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.https,
      NULL,
      NULL,
      gettext_nop(
        "\t-noSSL/-SSL        - don't download (HTTPS) SSL files\n")
#endif
    },
  {
      NULL,
      "-SSL",
      NULL,
#ifndef USE_SSL
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.https,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-verify",
      "Verify CERT:",
#ifndef USE_SSL
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.verify,
      NULL,
      NULL,
      gettext_nop(
        "\t-noverify/-verify        - don't verify SSL certificates\n")
#endif
    },
  {
      NULL,
      "-noverify",
      "Verify CERT:",
#ifndef USE_SSL
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.verify,
      NULL,
      NULL,
      gettext_nop(
        "\t-noverify/-verify        - don't verify SSL certificates\n")
#endif
    },
  {
      NULL,
      "-noFTPS",
      "AllowFTPS:",
#ifndef USE_SSL
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.ftps,
      NULL,
      NULL,
      gettext_nop(
        "\t-noFTPS/-FTPS      - don't download FTPS files\n")
#endif
    },
  {
      NULL,
      "-FTPS",
      NULL,
#ifndef USE_SSL
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.ftps,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-noGopher",
      "AllowGopher:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.gopher,
      NULL,
      NULL,
      gettext_nop(
        "\t-noGopher/-Gopher  - download Gopher files ?\n")
    },
  {
      NULL,
      "-Gopher",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.gopher,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noCGI",
      "AllowCGI:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.cgi,
      NULL,
      NULL,
      gettext_nop(
        "\t-noCGI/-CGI        - download parametric CGI pages ?\n")
    },
  {
      NULL,
      "-CGI",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.cgi,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noEnc",
      "AllowGZEncoding:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.use_enc,
      NULL,
      NULL,
      gettext_nop(
        "\t-noEnc/-Enc        - allow transfer of encoded files ?\n")
    },
  {
      NULL,
      "-Enc",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.use_enc,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-noRelocate",
      "AllowRelocation:",
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.rewrite_links,
      NULL,
      NULL,
      gettext_nop(
        "\t-noRelocate/-Relocate\n"
        "\t                   - don't rewrite links\n")
    },
  {
      NULL,
      "-Relocate",
      NULL,
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.rewrite_links,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-FTPhtml",
      "FTPhtml:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.ftp_html,
      NULL,
      NULL,
      gettext_nop(
        "\t-FTPhtml/-noFTPhtml\n"
        "\t                   - process HTML files downloaded over FTP\n")
    },
  {
      NULL,
      "-noFTPhtml",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.ftp_html,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-FTPlist",
      "FTPListCMD:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.ftp_list,
      NULL,
      NULL,
      gettext_nop(
        "\t-FTPlist/-noFTPlist\n"
        "\t                   - use wide FTP directory listing\n")
    },
  {
      NULL,
      "-noFTPlist",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.ftp_list,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-FTPdir",
      "AllowFTPRecursion:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.condition.ftpdir,
      NULL,
      NULL,
      gettext_nop(
        "\t-FTPdir/-noFTPdir  - recurse FTP directory\n")
    },
  {
      NULL,
      "-noFTPdir",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.condition.ftpdir,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-store_index",
      "StoreDirIndexFile:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.store_index,
      NULL,
      NULL,
      gettext_nop(
        "\t-store_index/-nostore_index\n"
        "\t                   - store directory URLs as index files\n")
    },
  {
      NULL,
      "-nostore_index",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.store_index,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-force_reget",
      "ForceReget:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.freget,
      NULL,
      NULL,
      gettext_nop(
        "\t-force_reget/-noforce_reget\n"
        "\t                   - force reget of whole file when server doesn't\n"
        "\t                     support reget\n")
    },
  {
      NULL,
      "-noforce_reget",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.freget,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-debug",
      "Debug:",
#ifndef DEBUG
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.debug,
      NULL,
      NULL,
      gettext_nop(
        "\t-debug             - turn on debug mode\n")
#endif
    },
  {
      NULL,
      "-nodebug",
      NULL,
#ifndef DEBUG
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.debug,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-debug_level",
      "DebugLevel:",
#ifndef DEBUG
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_DEBUGL,
      (void *) 0,
      &cfg.debug_level,
      NULL,
      NULL,
      gettext_nop(
        "\t-debug_level $l    - debug level number, see manual for $l description\n")
#endif
    },
  {
      NULL,
      "-asite",
      "AllowedSites:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.sites,
      (void *) TRUE,
      &cfg.condition.allow_site,
      gettext_nop(
        "\t-asite $list       - comma-separated list of allowed sites\n")
    },
  {
      NULL,
      "-dsite",
      "DisallowedSites:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.sites,
      (void *) FALSE,
      &cfg.condition.allow_site,
      gettext_nop(
        "\t-dsite $list       - comma-separated list of disallowed sites\n")
    },
  {
      NULL,
      "-adomain",
      "AllowedDomains:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.domains,
      (void *) TRUE,
      &cfg.condition.allow_domain,
      gettext_nop(
        "\t-adomain $list     - list of allowed domains\n")
    },
  {
      NULL,
      "-ddomain",
      "DisallowedDomains:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.domains,
      (void *) FALSE,
      &cfg.condition.allow_domain,
      gettext_nop(
        "\t-ddomain $list     - list of disallowed domains\n")
    },
  {
      NULL,
      "-aprefix",
      "AllowedPrefixes:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.dir_prefix,
      (void *) TRUE,
      &cfg.condition.allow_prefix,
      gettext_nop(
        "\t-aprefix $list     - list of allowed directory/file prefixes\n")
    },
  {
      NULL,
      "-dprefix",
      "DisallowedPrefixes:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.dir_prefix,
      (void *) FALSE,
      &cfg.condition.allow_prefix,
      gettext_nop(
        "\t-dprefix $list     - list of disallowed directory/file prefixes\n")
    },
  {
      NULL,
      "-asfx",
      "AllowedSuffixes:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.sufix,
      (void *) TRUE,
      &cfg.condition.allow_sufix,
      gettext_nop(
        "\t-asfx $list        - list of allowed suffixes\n")
    },
  {
      NULL,
      "-dsfx",
      "DisallowedSuffixes:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.sufix,
      (void *) FALSE,
      &cfg.condition.allow_sufix,
      gettext_nop(
        "\t-dsfx $list        - list of disallowed suffixes\n")
    },
  {
      NULL,
      "-amimet",
      "AllowedMIMETypes:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.mime,
      (void *) TRUE,
      &cfg.condition.allow_mime,
      gettext_nop(
        "\t-amimet $list      - list of alloved MIME types\n")
    },
  {
      NULL,
      "-dmimet",
      "DisallowedMIMETypes:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.mime,
      (void *) FALSE,
      &cfg.condition.allow_mime,
      gettext_nop(
        "\t-dmimet $list      - list of disallowed MIME types\n")
    },
  {
      NULL,
      "-alang",
      "PreferredLanguages:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.accept_lang,
      NULL,
      NULL,
      gettext_nop(
        "\t-alang $list       - list of preferred languages (only via HTTP)\n")
    },
  {
      NULL,
      "-acharset",
      "PreferredCharset:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.accept_chars,
      NULL,
      NULL,
      gettext_nop(
        "\t-acharset $list    - list of preferred character sets (only via HTTP)\n")
    },
  {
      NULL,
      "-scnDir",
      "ScenarioDirectory:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.scndir,
      NULL,
      NULL,
      gettext_nop(
        "\t-scndir $dir       - directory ,where are stored your scenarios\n"
        "\t                     (config files)\n")
    },
  {
      NULL,
      "-cdir",
      "WorkingDir:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.cache_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-cdir $dir         - directory for storing documents\n")
    },
  {
      NULL,
      "-subdir",
      "WorkingSubDir:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.subdir,
      NULL,
      NULL,
      gettext_nop(
        "\t-subdir $dir       - subdirectory of cdir to operate on\n")
    },
  {
      NULL,
      "-scenario",
      NULL,
      PARAM_FUNC,
      (void *) 1,
      cfg_load_scenario,
      NULL,
      NULL,
      gettext_nop(
        "\t-scenario $str     - name of scenario from scenario directory to load\n"
        "\t                     and or run\n")
    },
  {
      NULL,
      "-auth_scheme",
      "HTTPAuthorizationScheme:",
      PARAM_AUTHSCH,
      (void *) 2,
      &cfg.auth_scheme,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_scheme 1/2/3 - HTTP authorization scheme 1-user 2-Basic 3-Digest,\n"
        "\t                     default 2\n")
    },
  {
      NULL,
      "-auth_name",
      "HTTPAuthorizationName:",
      PARAM_STR,
      (void *) NULL,
      &cfg.name_auth,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_name $str    - name for Authorization (only via HTTP)\n")
    },
  {
      NULL,
      "-auth_passwd",
      "HTTPAuthorizationPassword:",
      PARAM_PASS,
      (void *) NULL,
      &cfg.passwd_auth,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_passwd $str  - password for Authorization (only via HTTP)\n")
    },
  {
      NULL,
      "-auth_reuse_nonce",
      "AuthReuseDigestNonce:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.auth_reuse_nonce,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_reuse_nonce/-noauth_reuse_nonce\n"
        "\t                   - reuse one HTTP digest access authorization\n"
        "\t                     nonce for more requests\n")
    },
  {
      NULL,
      "-noauth_reuse_nonce",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.auth_reuse_nonce,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-auth_reuse_proxy_nonce",
      "AuthReuseProxyDigestNonce:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.auth_reuse_proxy_nonce,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_reuse_proxy_nonce/-noauth_reuse_proxy_nonce\n"
        "\t                   - reuse one HTTP proxy digest access\n"
        "\t                     authorization nonce for more requests\n")
    },
  {
      NULL,
      "-noauth_reuse_proxy_nonce",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.auth_reuse_proxy_nonce,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-ssl_cert_passwd",
      "SSLCertPassword:",
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_OPENSSL)
      PARAM_PASS | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PASS,
      (void *) NULL,
      &cfg.ssl_cert_passwd,
      NULL,
      NULL,
      gettext_nop(
        "\t-ssl_cert_passwd $str\n"
        "\t                   - password for SSL certification file\n")
#endif
    },
  {
      NULL,
      "-ssl_cert_file",
      "SSLCertFile:",
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_OPENSSL)
      PARAM_PATH | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PATH,
      (void *) NULL,
      &cfg.ssl_cert_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-ssl_cert_file $str\n"
        "\t                   - SSL certification file\n")
#endif
    },
  {
      NULL,
      "-ssl_key_file",
      "SSLKeyFile:",
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_OPENSSL)
      PARAM_PATH | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PATH,
      (void *) NULL,
      &cfg.ssl_key_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-ssl_key_file $str - SSL certification key file\n")
#endif
    },
  {
      NULL,
      "-ssl_cipher_list",
      "SSLCipherList:",
#ifndef USE_SSL
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.ssl_cipher_list,
      NULL,
      NULL,
      gettext_nop(
        "\t-ssl_cipher_list $str\n"
        "\t                   - list of prefered SSL ciphers in SSL communication\n")
#endif
    },
  {
      NULL,
      "-egd_socket",
      "EGDSocket:",
#ifndef HAVE_RAND_EGD
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.egd_socket,
      NULL,
      NULL,
      gettext_nop(
        "\t-egd_socket $file  - path to EGD listening socket\n")
#endif
    },
  {
      NULL,
      "-from",
      "EmailAddress:",
      PARAM_STR,
      (void *) NULL,
      &cfg.from,
      NULL,
      NULL,
      gettext_nop(
        "\t-from $str         - e-mail address used for user identification\n")
    },
  {
      NULL,
      "-send_from",
      "SendFromHeader:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.send_from,
      NULL,
      NULL,
      gettext_nop(
        "\t-send_from/-nosend_from\n"
        "\t                   - send From: header in HTTP request with your\n"
        "\t                     e-mail address\n")
    },
  {
      NULL,
      "-nosend_from",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.send_from,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-identity",
      "Identity:",
      PARAM_STR,
      (void *) NULL,
      &cfg.identity,
      NULL,
      NULL,
      gettext_nop(
        "\t-identity $str     - contents of User-agent: field in HTTP request\n")
    },
  {
      NULL,
      "-pattern",
      "MatchPattern:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-pattern $list     - list of wildcard patterns for files\n")
    },
  {
      NULL,
      "-rpattern",
      "REMatchPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.condition.rpattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-rpattern $re      - RE matching pattern for files\n")
#endif
    },
  {
      NULL,
      "-skip_pattern",
      "SkipMatchPattern:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.skip_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-skip_pattern $list\n"
        "\t                   - list of skip wildcard patterns for files\n")
    },
  {
      NULL,
      "-skip_rpattern",
      "SkipREMatchPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.condition.rskip_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-skip_rpattern $re\n"
        "\t                   - skip RE matching pattern for files\n")
#endif
    },
  {
      NULL,
      "-url_pattern",
      "URLMatchPattern:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.url_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-url_pattern $list - list of wildcard patterns for urls\n")
    },
  {
      NULL,
      "-url_rpattern",
      "URLREMatchPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.condition.rurl_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-url_rpattern $re  - RE matching pattern for urls\n")
#endif
    },
  {
      NULL,
      "-skip_url_pattern",
      "SkipURLMatchPattern:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.condition.skip_url_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-skip_url_pattern $list\n"
        "\t                   - list of wildcard patterns for urls\n")
    },
  {
      NULL,
      "-skip_url_rpattern",
      "SkipURLREMatchPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL,
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.condition.rskip_url_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-skip_url_rpattern $re\n"
        "\t                   - RE matching patterns for urls\n")
#endif
    },
  {
      NULL,
      "-mode",
      "DefaultMode:",
      PARAM_MODE,
      (void *) MODE_NORMAL,
      &cfg.mode,
      NULL,
      NULL,
      gettext_nop(
        "\t-mode $mode        - set operation mode\n"
        "\t                         normal - recurse throught WWW (default)\n"
        "\t                         linkupdate - update remote links in local tree\n"
        "\t                         sync - synchronize local tree with remote WWW\n"
        "\t                                servers\n"
        "\t                         singlepage - single page with inline objects\n"
        "\t                         singlereget - reget file until not whole\n"
        "\t                         resumeregets - reget all files which are broken\n"
        "\t                         dontstore - transfer documents, but don't store\n"
        "\t                         reminder - checks URLs if they are changed\n"
        "\t                         ftpdir - list content of FTP directory\n"
        "\t                         mirror - make exact copy of remote site\n")
    },
  {
      NULL,
      "-ftp_proxy",
      "FTPProxy:",
      PARAM_CONN,
      (void *) NULL,
      &cfg.ftp_proxy,
      (void *) DEFAULT_FTP_PROXY_PORT,
      &cfg.ftp_proxy_port,
      gettext_nop(
        "\t-ftp_proxy $site[:$port]\n"
        "\t                   - ftp proxy/cache server\n")
    },
  {
      NULL,
      "-http_proxy",
      "HTTPProxy:",
      PARAM_PROXY,
      (void *) NULL,
      &cfg.http_proxy,
      NULL,
      NULL,
      gettext_nop(
        "\t-http_proxy $site[:$port]\n"
        "\t                   - http proxy/cache server\n")
    },
  {
      NULL,
      "-gopher_proxy",
      "GopherProxy:",
      PARAM_CONN,
      (void *) NULL,
      &cfg.gopher_proxy,
      (void *) DEFAULT_GOPHER_PROXY_PORT,
      &cfg.gopher_proxy_port,
      gettext_nop(
        "\t-gopher_proxy $site[:$port]\n"
        "\t                   - gopher proxy/cache server\n")
    },
  {
      NULL,
      "-ssl_proxy",
      "SSLProxy:",
#ifndef USE_SSL
      PARAM_CONN | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_CONN,
      (void *) NULL,
      &cfg.ssl_proxy,
      (void *) DEFAULT_SSL_PROXY_PORT,
      &cfg.ssl_proxy_port,
      gettext_nop(
        "\t-ssl_proxy $site[:$port]\n"
        "\t                   - ssl proxy server\n")
#endif
    },
  {
      NULL,
      "-gopher_httpgw",
      "GopherViaHTTPProxy:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.gopher_via_http,
      NULL,
      NULL,
      gettext_nop(
        "\t-gopher_httpgw/-nogopher_httpgw\n"
        "\t                   - specified gopher proxy is HTTP gateway for Gopher\n")

    },
  {
      NULL,
      "-nogopher_httpgw",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.gopher_via_http,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-ftp_httpgw",
      "FTPViaHTTPProxy:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.ftp_via_http,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_httpgw/-noftp_httpgw\n"
        "\t                   - specified ftp proxy is HTTP gateway for FTP\n")

    },
  {
      NULL,
      "-noftp_httpgw",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.ftp_via_http,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-ftp_dirtyproxy",
      "FTPDirtyProxy:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.ftp_dirtyp,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_dirtyproxy/-noftp_dirtyproxy\n"
        "\t                   - use CONNECT request to HTTP proxy for FTP\n"
        "\t                     connections\n")
    },
  {
      NULL,
      "-noftp_dirtyproxy",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.ftp_dirtyp,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-browser",
      "Browser:",
#if !defined(I_FACE) || !defined(WITH_TREE)
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.browser,
      NULL,
      NULL,
      gettext_nop(
        "\t-browser $str      - your preferred browser programm\n")
#endif
    },
  {
      NULL,
      "-xmaxlog",
      "XMaxLogSize:",
#ifndef I_FACE
      PARAM_NUM | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NUM,
      (void *) NULL,
      &cfg.xlogsize,
      NULL,
      NULL,
      gettext_nop(
        "\t-xmaxlog           - maximal length of log window\n")
#endif
    },
  {
      NULL,
      "-dumpscn",
      NULL,
      PARAM_STR,
      (void *) NULL,
      &cfg.save_scn,
      NULL,
      NULL,
      gettext_nop(
        "\t-dumpscn $str      - save scenario in scndir with name $str\n")
    },
  {
      NULL,
      "-maxsize",
      "MaxSize:",
      PARAM_NUM,
      (void *) 0,
      &cfg.condition.max_size,
      NULL,
      NULL,
      gettext_nop(
        "\t-maxsize $nr       - maximal allowed size of document in bytes\n")
    },
  {
      NULL,
      "-minsize",
      "MinSize:",
      PARAM_NUM,
      (void *) 0,
      &cfg.condition.min_size,
      NULL,
      NULL,
      gettext_nop(
        "\t-minsize $nr       - minimal allowed size of document in bytes\n")
    },
  {
      NULL,
      "-http_proxy_pass",
      "HTTPProxyPass:",
      PARAM_PASS,
      (void *) NULL,
      &cfg.http_proxy_pass,
      NULL,
      NULL,
      gettext_nop(
        "\t-http_proxy_pass $str\n"
        "\t                   - password for HTTP proxy authorization\n")
    },
  {
      NULL,
      "-http_proxy_user",
      "HTTPProxyUser:",
      PARAM_STR,
      (void *) NULL,
      &cfg.http_proxy_user,
      NULL,
      NULL,
      gettext_nop(
        "\t-http_proxy_user $str\n"
        "\t                   - user name for HTTP proxy authorization\n")
    },
  {
      NULL,
      "-http_proxy_auth",
      "HTTPProxyAuth:",
      PARAM_AUTHSCH,
      (void *) 2,
      &cfg.proxy_auth_scheme,
      NULL,
      NULL,
      gettext_nop(
        "\t-http_proxy_auth 1/2/3\n"
        "\t                   - authorization scheme for HTTP proxy authorization\n")
    },
  {
      NULL,
      "-logfile",
      "LogFile:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.logfile,
      NULL,
      NULL,
      gettext_nop(
        "\t-logfile $file     - name of file where mesages are stored\n")
    },
  {
      NULL,
      "-slogfile",
      "SLogFile:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.short_logfile,
      NULL,
      NULL,
      gettext_nop(
        "\t-slogfile $file    - name of file where short log will be stored\n")
    },
  {
      NULL,
      "-tlogfile",
      "TLogFile:",
      PARAM_STR,
      (void *) NULL,
      &cfg.time_logfile,
      NULL,
      NULL,
      gettext_nop(
        "\t-tlogfile $file    - name of file where time log will be stored\n")
    },
  {
      NULL,
      "-trelative",
      "Relative:",
      PARAM_STR,
      (void *) NULL,
      &cfg.time_relative,
      NULL,
      NULL,
      gettext_nop(
        "\t-trelative $str (object/program)\n"
        "\t                   - what timings are relative to\n")
    },
  {
      "-tp",
      "-transparent_proxy",
      "TransparentProxy:",
      PARAM_TRANSPARENT,
      (void *) NULL,
      &cfg.transparent_proxy,
      NULL,
      NULL,
      gettext_nop(
        "\t-transparent_proxy $site[:$port]\n"
        "\t                   - transparent proxy\n")
    },
  {
      "-tsp",
      "-transparent_ssl_proxy",
      "TransparentSSLProxy:",
      PARAM_TRANSPARENT,
      (void *) NULL,
      &cfg.transparent_ssl_proxy,
      NULL,
      NULL,
      gettext_nop(
        "\t-transparent_ssl_proxy $site[:$port]\n"
        "\t                   - transparent SSL proxy\n")
    },
  {
      NULL,
      "-sdemo",
      "SdemoMode:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.sdemo_mode,
      NULL,
      NULL,
      gettext_nop(
        "\t-sdemo_mode/-notsdemo_mode\n"
        "\t                   - sdemo compatible output\n")
    },
  {
      NULL,
      "-noencode",
      "NoEncode:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.noencode,
      NULL,
      NULL,
      gettext_nop(
        "\t-noencode          - do not perform rfc 2396 character encoding\n")
    },
  {
      NULL,
      "-stime",
      "ShowDownloadTime:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.show_time,
      NULL,
      NULL,
      gettext_nop(
        "\t-stime/-nostime    - write starting and ending time of transfer\n")
    },
  {
      NULL,
      "-nostime",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.show_time,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-remove_old",
      "RemoveOldDocuments:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.remove_old,
      NULL,
      NULL,
      gettext_nop(
        "\t-remove_old/-noremove_old\n"
        "\t                   - remove improper files or directories while running\n"
        "\t                     in sync mode\n")

    },
  {
      NULL,
      "-noremove_old",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.remove_old,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-remove_before_store",
      "RemoveBeforeStore:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.remove_before_store,
      NULL,
      NULL,
      gettext_nop(
        "\t-remove_before_store/-noremove_before_store\n"
        "\t                   - remove file before storing new content\n")
    },
  {
      NULL,
      "-noremove_before_store",
      "RemoveBeforeStore:",
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.remove_before_store,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-always_mdtm",
      "AlwaysMDTM:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.always_mdtm,
      NULL,
      NULL,
      gettext_nop(
        "\t-always_mdtm/-noalways_mdtm\n"
        "\t                   - always use MDTM to determine modifictaion time\n"
        "\t                     of remote file; never use values from file listing\n")
    },
  {
      NULL,
      "-noalways_mdtm",
      "AlwaysMDTM:",
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.always_mdtm,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-auth_file",
      "AuthFile:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.auth_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_file $file   - file where you have stored your auth infos\n"
        "\t                     see manual for format description\n")
    },
  {
      NULL,
      "-base_level",
      "BaseLevel:",
      PARAM_NUM,
      (void *) 0,
      &cfg.base_level,
      NULL,
      NULL,
      gettext_nop(
        "\t-base_level $nr    - number of levels of directory tree to omit from top\n")
    },
  {
      NULL,
      "-ftp_active",
      "ActiveFTPData:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.ftp_activec,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_active        - select active FTP data connection\n")
    },
  {
      NULL,
      "-ftp_passive",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.ftp_activec,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_passive       - select passive FTP data connection\n")
    },
  {
      NULL,
      "-active_ftp_port_range",
      "ActiveFTPPortRange:",
      PARAM_PORT_RANGE,
      (void *) -1,
      &cfg.active_ftp_min_port,
      (void *) -1,
      &cfg.active_ftp_max_port,
      gettext_nop(
        "\t-active_ftp_port_range $min:$max\n"
        "\t                   - range of FTP data connection ports used\n"
        "\t                     for active ftp\n")
    },
  {
      NULL,
      "-msgcat",
      "NLSMessageCatalogDir:",
#ifndef GETTEXT_NLS
      PARAM_PATH | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PATH,
      (void *) NULL,
      &cfg.msgcatd,
      NULL,
      NULL,
      gettext_nop(
        "\t-msgcat $dir       - directory where message catalogs are stored\n")
#endif
    },
  {
      NULL,
      "-language",
      "Language:",
#ifndef GETTEXT_NLS
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.language,
      NULL,
      NULL,
      gettext_nop(
        "\t-language $str     - set language for messages\n")
#endif
    },
  {
      NULL,
      "-quiet",
      "Quiet:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.quiet,
      NULL,
      NULL,
      gettext_nop(
        "\t-quiet             - don't show output messages\n")
     },
  {
      NULL,
      "-verbose",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.quiet,
      NULL,
      NULL,
      gettext_nop(
        "\t-verbose           - show output messages (default)\n")
    },
  {
      NULL,
      "-newer_than",
      "NewerThan:",
      PARAM_TIME,
      (void *) 0,
      &cfg.condition.btime,
      NULL,
      NULL,
      gettext_nop(
        "\t-newer_than $time  - download only documents newer than $time\n"
        "\t                     format of $time: YYYY.MM.DD.hh:mm\n")
    },
  {
      NULL,
      "-older_than",
      "OlderThan:",
      PARAM_TIME,
      (void *) 0,
      &cfg.condition.etime,
      NULL,
      NULL,
      gettext_nop(
        "\t-older_than $time  - download only documents older than $time\n"
        "\t                     format of $time: YYYY.MM.DD.hh:mm\n")
    },
  {
      NULL,
      "-schedule",
      NULL,
      PARAM_TIME,
      (void *) 0,
      &cfg.schtime,
      NULL,
      NULL,
      gettext_nop(
        "\t-schedule $time    - schedule pavuk start at $time\n"
        "\t                     format of $time: YYYY.MM.DD.hh:mm\n")
    },
  {
      NULL,
      "-reschedule",
      "Reschedule:",
      PARAM_NUM,
      (void *) 0,
      &cfg.reschedh,
      NULL,
      NULL,
      gettext_nop(
        "\t-reschedule $nr    - number of hours between two runs\n"
        "\t                     for cyclic scheduling\n")
    },
  {
      NULL,
      "-dont_leave_site",
      "DontLeaveSite:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.condition.dont_leave_site,
      NULL,
      NULL,
      gettext_nop(
        "\t-dont_leave_site/-leave_site\n"
        "\t                   - don't/leave site of starting URL\n")
    },
  {
      NULL,
      "-leave_site",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.condition.dont_leave_site,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-dont_leave_dir",
      "DontLeaveDir:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.condition.dont_leave_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-dont_leave_dir/-leave_dir\n"
        "\t                   - don't/leave directory of starting URL\n")
    },
  {
      NULL,
      "-leave_dir",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.condition.dont_leave_dir,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-preserve_time",
      "PreserveTime:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.preserve_time,
      NULL,
      NULL,
      gettext_nop(
        "\t-preserve_time/-nopreserve_time\n"
        "\t                   - preserve document modification time\n")
    },
  {
      NULL,
      "-nopreserve_time",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.preserve_time,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-preserve_perm",
      "PreservePermisions:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.preserve_perm,
      NULL,
      NULL,
      gettext_nop(
        "\t-preserve_perm/-nopreserve_perm\n"
        "\t                   - preserve document permissions\n")
    },
  {
      NULL,
      "-nopreserve_perm",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.preserve_perm,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-preserve_slinks",
      "PreserveAbsoluteSymlinks:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.preserve_links,
      NULL,
      NULL,
      gettext_nop(
        "\t-preserve_slinks/-nopreserve_slinks\n"
        "\t                   - preserve absolute symlinks\n")
    },
  {
      NULL,
      "-nopreserve_slinks",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.preserve_links,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-leave_level",
      "LeaveLevel:",
      PARAM_NUM,
      (void *) 0,
      &cfg.condition.leave_level,
      NULL,
      NULL,
      gettext_nop(
        "\t-leave_level $nr   - how many tree levels leave from starting\n"
        "\t                     site , (0 == don't care) default 0\n")
    },
  {
      NULL,
      "-cookie_file",
      "CookieFile:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.cookie_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-cookie_file $file - file where are stored cookie infos\n")
    },
  {
      NULL,
      "-cookie_send",
      "CookieSend:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.send_cookies,
      NULL,
      NULL,
      gettext_nop(
        "\t-cookie_send/-nocookie_send\n"
        "\t                   - send cookie info in HTTP request\n")
    },
  {
      NULL,
      "-nocookie_send",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.send_cookies,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-cookie_recv",
      "CookieRecv:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.recv_cookies,
      NULL,
      NULL,
      gettext_nop(
        "\t-cookie_recv/-nocookie_recv\n"
        "\t                   - accept cookies from HTTP response\n")
    },
  {
      NULL,
      "-nocookie_recv",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.recv_cookies,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-cookie_update",
      "CookieUpdate:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.update_cookies,
      NULL,
      NULL,
      gettext_nop(
        "\t-cookie_update/-nocookie_update\n"
        "\t                   - update cookies in cookies file\n")
    },
  {
      NULL,
      "-nocookie_update",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.update_cookies,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-cookie_check",
      "CookieCheckDomain:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.cookie_check_domain,
      NULL,
      NULL,
      gettext_nop(
        "\t-cookie_check/-nocookie_check\n"
        "\t                   - check if cookies are set for source domain\n")
    },
  {
      NULL,
      "-nocookie_check",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.cookie_check_domain,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-cookies_max",
      "CookiesMax:",
      PARAM_NUM,
      (void *) 0,
      &cfg.cookies_max,
      NULL,
      NULL,
      gettext_nop(
        "\t-cookies_max $nr   - maximal number of cookies in cookie cache\n")
    },
  {
      NULL,
      "-disabled_cookie_domains",
      "DisabledCookieDomains:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.cookies_disabled_domains,
      (void *) NULL,
      NULL,
      gettext_nop(
        "\t-disabled_cookie_domains $list\n"
        "\t                   - comma-separated list of disabled cookie domains\n")
    },
  {
      NULL,
      "-disable_html_tag",
      "DisableHTMLTag:",
      PARAM_HTMLTAG,
      (void *) TRUE,
      NULL,
      NULL,
      NULL,
      gettext_nop(
        "\t-disable_html_tag $TAG,[$ATTRIB][;...]\n"
        "\t                   - disable processing of URLs from\n"
        "\t                     attribute $ATTRIB of HTML tag $TAG\n")
    },
  {
      NULL,
      "-enable_html_tag",
      "EnableHTMLTag:",
      PARAM_HTMLTAG,
      (void *) FALSE,
      NULL,
      NULL,
      NULL,
      gettext_nop(
        "\t-enable_html_tag $TAG,[$ATTRIB][;...]\n"
        "\t                   - enable proccessing of URLs from\n"
        "\t                     attribute $ATTRIB of HTML tag $TAG\n")
    },
  {
      NULL,
      "-gui_font",
      "GUIFont:",
#ifndef I_FACE
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.fontname,
      NULL,
      NULL,
      gettext_nop(
        "\t-gui_font $font    - font name used in GUI interface\n")
#endif
    },
  {
      NULL,
      "-user_condition",
      "UserCondition:",
      PARAM_STR,
      (void *) NULL,
      &cfg.condition.uexit,
      NULL,
      NULL,
      gettext_nop(
        "\t-user_condition $str\n"
        "\t                   - user exit script used to restrict some URLs\n"
        "\t                     for more informations see manpage\n")
    },
  {
      NULL,
      "-tr_del_chr",
      "TrDeleteChar:",
      PARAM_STR,
      (void *) NULL,
      &cfg.tr_del_chr,
      NULL,
      NULL,
      gettext_nop(
        "\t-tr_del_chr $str   - characters that will be deleted from\n"
        "\t                     local filename\n")
    },
  {
      NULL,
      "-tr_str_str",
      "TrStrToStr:",
      PARAM_TWO_QSTR,
      (void *) NULL,
      &cfg.tr_str_s1,
      (void *) NULL,
      &cfg.tr_str_s2,
      gettext_nop(
        "\t-tr_str_str $str1 $str2\n"
        "\t                   - translate $str1 to $str2 in local filename\n")
    },
  {
      NULL,
      "-tr_chr_chr",
      "TrChrToChr:",
      PARAM_TWO_QSTR,
      (void *) NULL,
      &cfg.tr_chr_s1,
      (void *) NULL,
      &cfg.tr_chr_s2,
      gettext_nop(
        "\t-tr_chr_chr $chrset1 $chrset2\n"
        "\t                   - translate $chrset1 to $chrset2 in local filename\n")
    },
  {
      NULL,
      "-index_name",
      "IndexName:",
      PARAM_STR,
      (void *) NULL,
      &cfg.index_name,
      NULL,
      NULL,
      gettext_nop(
        "\t-index_name $str   - name of directory index instead of _._.html\n")
    },
  {
      NULL,
      "-store_name",
      "StoreName:",
      PARAM_STR,
      (void *) NULL,
      &cfg.store_name,
      NULL,
      NULL,
      gettext_nop(
        "\t-store_name $str   - filename for first downloaded document\n")
    },
  {
      NULL,
      "-htDig",
      NULL,
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.htdig,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-check_size",
      "CheckSize:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.check_size,
      NULL,
      NULL,
      gettext_nop(
        "\t-check_size/-nocheck_size\n"
        "\t                   - compare received size of file with\n"
        "\t                     that provided by remote server\n")
    },
  {
      NULL,
      "-nocheck_size",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.check_size,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-urls_file",
      "URLsFile:",
      PARAM_STR,
      (void *) NULL,
      &cfg.urls_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-urls_file $file   - URLs should be read from file, until\n"
        "\t                     line with single \".\" occurs in input\n")
    },
  {
      NULL,
      "-bg",
      "BgMode:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.bgmode,
      NULL,
      NULL,
      gettext_nop(
        "\t-bg/-nobg          - detach pavuk proces from terminal and\n"
        "\t                     don't output any messages on screen\n")
    },
  {
      NULL,
      "-nobg",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.bgmode,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-maxrate",
      "MaxRate:",
      PARAM_DOUBLE,
      (void *) &_default_maxrate,
      &cfg.maxrate,
      NULL,
      NULL,
      gettext_nop(
        "\t-maxrate $nr       - limit to maximal speed of transfer (kB/s)\n")
    },
  {
      NULL,
      "-minrate",
      "MinRate:",
      PARAM_DOUBLE,
      (void *) &_default_minrate,
      &cfg.minrate,
      NULL,
      NULL,
      gettext_nop(
        "\t-minrate $nr       - limit to minimal speed of transfer (kB/s)\n")
    },
  {
      NULL,
      "-bufsize",
      "ReadBufferSize:",
      PARAM_NUM,
      (void *) 32,
      &cfg.bufsize,
      NULL,
      NULL,
      gettext_nop(
        "\t-bufsize $nr       - size of read buffer (kB)\n")
    },
  {
      NULL,
      "-file_quota",
      "FileSizeQuota:",
      PARAM_NUM,
      (void *) 0,
      &cfg.file_quota,
      NULL,
      NULL,
      gettext_nop(
        "\t-file_quota $nr    - maximal size of file to transfer (kB)\n")
    },
  {
      NULL,
      "-trans_quota",
      "TransferQuota:",
      PARAM_NUM,
      (void *) 0,
      &cfg.trans_quota,
      NULL,
      NULL,
      gettext_nop(
        "\t-trans_quota $nr   - maximal amount of transfer per session (kB)\n")
    },
  {
      NULL,
      "-fs_quota",
      "FSQuota:",
      PARAM_NUM,
      (void *) 0,
      &cfg.fs_quota,
      NULL,
      NULL,
      gettext_nop(
        "\t-fs_quota $nr      - disk free space quota (kB)\n")
    },
  {
      NULL,
      "-enable_js",
      "EnableJS:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.enable_js,
      NULL,
      NULL,
      gettext_nop(
        "\t-enable_js/-disable_js\n"
        "\t                   - enable downloading of javascript source files\n")
    },
  {
      NULL,
      "-disable_js",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.enable_js,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-fnrules",
      "FnameRules:",
      PARAM_LFNAME,
      (void *) NULL,
      &cfg.lfnames,
      NULL,
      NULL,
      gettext_nop(
        "\t-fnrules $t $m $r  - local filename construction rules\n"
        "\t                     (for better description see manual)\n")
    },
  {
      NULL,
      "-store_info",
      "StoreDocInfoFiles:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.enable_info,
      NULL,
      NULL,
      gettext_nop(
        "\t-store_info/-nostore_info\n"
        "\t                   - store document info files with each document\n")
    },
  {
      NULL,
      "-nostore_info",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.enable_info,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-all_to_local",
      "AllLinksToLocal:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.all_to_local,
      NULL,
      NULL,
      gettext_nop(
        "\t-all_to_local/-noall_to_local\n"
        "\t                   - change all links inside HTML document to\n"
        "\t                     local immediately after download\n")
    },
  {
      NULL,
      "-noall_to_local",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.all_to_local,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-sel_to_local",
      "SelectedLinksToLocal:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.sel_to_local,
      NULL,
      NULL,
      gettext_nop(
        "\t-sel_to_local/-nosel_to_local\n"
        "\t                   - change all links inside HTML document\n"
        "\t                     which acomplish the limits, to\n"
        "\t                     local immediately after download\n")
    },
  {
      NULL,
      "-nosel_to_local",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.sel_to_local,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-all_to_remote",
      "AllLinksToRemote:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.all_to_remote,
      NULL,
      NULL,
      gettext_nop(
        "\t-all_to_remote/-noall_to_remote\n"
        "\t                   - change all links inside HTML document to\n"
        "\t                     remote immediately after download and\n"
        "\t                     don't do any further changes to it\n")
    },
  {
      NULL,
      "-noall_to_remote",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.all_to_remote,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-remind_cmd",
      "ReminderCMD:",
      PARAM_STR,
      (void *) NULL,
      &cfg.remind_cmd,
      NULL,
      NULL,
      gettext_nop(
        "\t-remind_cmd $str   - command which sends result from reminder mode\n")
    },
  {
      NULL,
      "-auto_referer",
      "AutoReferer:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.auto_referer,
      NULL,
      NULL,
      gettext_nop(
        "\t-auto_referer/-noauto_referer\n"
        "\t                   - in HTTP request send for each starting URL\n"
        "\t                     Referer: field which contains its own URL\n")
    },
  {
      NULL,
      "-noauto_referer",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.auto_referer,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-referer",
      "Referer:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.referer,
      NULL,
      NULL,
      gettext_nop(
        "\t-referer/-noreferer\n"
        "\t                   - send Referer: field in HTTP request\n")
    },
  {
      NULL,
      "-noreferer",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.referer,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-url_strategy",
      "UrlSchedulingStrategy:",
      PARAM_USTRAT,
      (void *) SSTRAT_DO_SIRKY,
      &cfg.scheduling_strategie,
      NULL,
      NULL,
      gettext_nop(
        "\t-url_strategy $strategy\n"
        "\t                   - scheduling strategy for URLs\n"
        "\t                     (this means order how URLs will be downloaded)\n"
        "\t                     $strategy is one of :\n"
        "\t                        level  - level order in URL tree\n"
        "\t                        leveli - level order in URL tree,\n"
        "\t                                 but inline objects go first\n"
        "\t                        pre    - pre-order in URL tree\n"
        "\t                        prei   - pre-order in URL tree,\n"
        "\t                                 but inline objects go first\n")
    },
  {
      NULL,
      "-nscache_dir",
      "NetscapeCacheDir:",
#ifndef HAVE_BDB_18x
      PARAM_PATH | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PATH,
      (void *) NULL,
      &cfg.ns_cache_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-nscache_dir $dir  - path to Netcape browser cache directory\n")
#endif
    },
  {
      NULL,
      "-ie_cache",
      "UseMSIECache:",
#ifndef __CYGWIN__
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.ie_cache,
      NULL,
      NULL,
      gettext_nop(
        "\t-ie_cache          - allow loading of files from MSIE browser cache\n")
#endif
    },
  {
      NULL,
      "-noie_cache",
      NULL,
#ifndef __CYGWIN__
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.ie_cache,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-remove_adv",
      "RemoveAdvertisement:",
#ifndef HAVE_REGEX
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.remove_adv,
      NULL,
      NULL,
      gettext_nop(
        "\t-remove_adv/-noremove_adv\n"
        "\t                   - enable removing of advertisement banners\n"
        "\t                     assumes you have setup regular expresions\n"
        "\t                     for matching adv banners with -adv_re option\n")
#endif
    },
  {
      NULL,
      "-noremove_adv",
      NULL,
#ifndef HAVE_REGEX
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.remove_adv,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-adv_re",
      "AdvBannerRE:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.advert_res,
      NULL,
      NULL,
      gettext_nop(
        "\t-adv_re $RE        - regular expressions for matching advertisement\n"
        "\t                     banner URLs\n")
#endif
    },
  {
      NULL,
      "-check_bg",
      "CheckIfRunnigAtBackground:",
#ifndef HAVE_TERMIOS
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.tccheck,
      NULL,
      NULL,
      gettext_nop(
        "\t-check_bg/-nocheck_bg\n"
        "\t                   - if running at background, don't write any\n"
        "\t                     messages to screen\n")
#endif
    },
  {
      NULL,
      "-nocheck_bg",
      NULL,
#ifndef HAVE_TERMIOS
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.tccheck,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-send_if_range",
      "SendIfRange:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.send_if_range,
      NULL,
      NULL,
      gettext_nop(
        "\t-send_if_range/-nosend_if_range\n"
        "\t                   - send If-Range header in HTTP request\n"
        "\t                     turn this of when server support reget, but\n"
        "\t                     generates different Etags for two requests\n")
    },
  {
      NULL,
      "-nosend_if_range",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.send_if_range,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-sched_cmd",
      "SchedulingCommand:",
      PARAM_STR,
      (void *) NULL,
      &cfg.sched_cmd,
      NULL,
      NULL,
      gettext_nop(
        "\t-sched_cmd $str    - command for scheduling\n")
    },
  {
      NULL,
      "-unique_log",
      "UniqueLogName:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.gen_logname,
      NULL,
      NULL,
      gettext_nop(
        "\t-unique_log/-nounique_log\n"
        "\t                   - when original log file locked try to find other\n"
        "\t                     with numbering extension which is not locked\n")
    },
  {
      NULL,
      "-nounique_log",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.gen_logname,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-read_css",
      "ReadCSS:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.read_css,
      NULL,
      NULL,
      gettext_nop(
        "\t-read_css/-noread_css\n"
        "\t                   - fetch objects mentioned in style sheets\n")
    },
  {
      NULL,
      "-noread_css",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.read_css,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-post_cmd",
      "PostCommand:",
      PARAM_STR,
      (void *) NULL,
      &cfg.post_cmd,
      NULL,
      NULL,
      gettext_nop(
        "\t-post_cmd $str     - postprocessing command, which will be invoked\n"
        "\t                     after document will be successfully downloaded\n")
    },
  {
      NULL,
      "-ssl_version",
      "SSLVersion:",
#ifndef USE_SSL
      PARAM_SSLVER | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_SSLVER,
      (void *) 1,
      &cfg.ssl_version,
      NULL,
      NULL,
#if defined(WITH_SSL_TLS1) || defined(USE_SSL_IMPL_NSS)
      gettext_nop(
        "\t-ssl_version $v    - version of SSL protocol used for communication,\n"
        "\t                     valid values: ssl23,ssl2,ssl3,tls1\n")
#else
      gettext_nop(
        "\t-ssl_version $v    - version of SSL protocol used for communication,\n"
        "\t                     valid values: ssl23,ssl2,ssl3\n")
#endif
#endif
    },
  {
      NULL,
      "-unique_sslid",
      "UniqueSSLID:",
#if !defined(USE_SSL)
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.unique_sslid,
      NULL,
      NULL,
      gettext_nop(
        "\t-unique_sslid/-nounique_sslid\n"
        "\t                   - use unique SSL ID with each SSL session\n")
#endif
    },
  {
      NULL,
      "-nounique_sslid",
      NULL,
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_OPENSSL)
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.unique_sslid,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-httpad",
      "AddHTTPHeader:",
      PARAM_HTTPHDR,
      (void *) NULL,
      &cfg.http_headers,
      NULL,
      NULL,
      gettext_nop(
        "\t-httpad $hdr       - additional HTTP header\n")
    },
  {
      NULL,
      "-statfile",
      "StatisticsFile:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.stats_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-statfile $str     - statistical report from downloading progress\n")
    },
  {
      NULL,
      "-ewait",
      "WaitOnExit:",
#ifndef __CYGWIN__
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.wait_on_exit,
      NULL,
      NULL,
      gettext_nop(
        "\t-ewait             - wait to finish program after downloading progress\n"
        "\t                     is done\n")
#endif
    },
  {
      NULL,
      "-noewait",
      NULL,
#ifndef __CYGWIN__
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.wait_on_exit,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-aip_pattern",
      "AllowedIPAdrressPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.condition.aip,
      NULL,
      NULL,
      gettext_nop(
        "\t-aip_pattern $re   - pattern for allowed IP addresses of servers\n")
#endif
    },
  {
      NULL,
      "-dip_pattern",
      "DisallowedIPAdrressPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.condition.skipip,
      NULL,
      NULL,
      gettext_nop(
        "\t-dip_pattern $re   - pattern for disallowed IP addresses of servers\n")
#endif
    },
  {
      NULL,
      "-site_level",
      "SiteLevel:",
      PARAM_NUM,
      (void *) 0,
      &cfg.condition.site_level,
      NULL,
      NULL,
      gettext_nop(
        "\t-site_level $nr    - maximum allowed number of sites to leave from\n"
        "\t                     starting site\n")
    },
  {
      NULL,
      "-use_http11",
      "UseHTTP11:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.use_http11,
      NULL,
      NULL,
      gettext_nop(
        "\t-use_http11/-nouse_http11\n"
        "\t                   - enable or disable using of HTTP/1.1 protocol\n"
        "\t                     features, default is off now\n")
    },
  {
      NULL,
      "-nouse_http11",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.use_http11,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-max_time",
      "MaxRunTime:",
      PARAM_DOUBLE,
      (void *) &_default_maxtime,
      &cfg.max_time,
      NULL,
      NULL,
      gettext_nop(
        "\t-max_time $nr      - set maximal amount of time for program run\n"
        "\t                     in minutes, default 0 == no limit\n"
        "\t                     you can use floating point numbers, to\n"
        "\t                     specify subminute times\n")

    },
  {
      NULL,
      "-local_ip",
      "LocalIP:",
      PARAM_STR,
      NULL,
      &cfg.local_ip,
      NULL,
      NULL,
      gettext_nop(
        "\t-local_ip $str     - use this local ip address of network interface.\n"
        "\t                     This option is for multihomed machines.\n")
    },
  {
      NULL,
      "-request",
      "RequestInfo:",
      PARAM_REQUEST,
      NULL,
      &cfg.request,
      NULL,
      NULL,
      gettext_nop(
        "\t-request $req      - extended request informations, used to specify\n"
        "\t                     information for GET or POST requests.\n"
        "\t                     format of $req :\n"
        "\t                     \"URL:$url METHOD:[GET|POST] ENCODING:[m|u]\n"
        "\t                       FIELD:$variable=$value\n"
        "\t                       FILE:$variable=$filename\n"
        "\t                       LNAME:$local_filename\"\n"
        "\t                     (for more informations see manual page)\n")
    },
  {
      NULL,
      "-hash_size",
      "HashSize:",
      PARAM_NUM,
      (void *) HASH_TBL_SIZE,
      &cfg.hash_size,
      NULL,
      NULL,
      gettext_nop(
        "\t-hash_size $nr     - size of internal hash tables for performance tuning\n")
    },
  {
      NULL,
      "-nthreads",
      "NumberOfThreads:",
#ifndef HAVE_MT
      PARAM_NUM | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NUM,
      (void *) 3,
      &cfg.nthr,
      NULL,
      NULL,
      gettext_nop(
        "\t-nthreads $nr      - set number of concurent downloading threads\n")
#endif
    },
  {
      NULL,
      "-immesg",
      "ImmediateMessages:",
#ifndef HAVE_MT
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.immessages,
      NULL,
      NULL,
      gettext_nop(
        "\t-immesg/-noimmesg  - write messages immediately after produced, not\n"
        "\t                     just when safe (from MT point of view)\n")
#endif
    },
  {
      NULL,
      "-noimmesg",
      NULL,
#ifndef HAVE_MT
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL,
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.immessages,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-formdata",
      "HTMLFormData:",
      PARAM_REQUEST,
      NULL,
      &cfg.formdata,
      NULL,
      NULL,
      gettext_nop(
        "\t-formdata $data    - used to specify HTML form fields, for HTML\n"
        "\t                     forms found during traversing document tree.\n"
        "\t                     Only forms mentiond in this option will be\n"
        "\t                     processed automaticaly.\n"
        "\t                     \"URL:$url\n"
        "\t                       FIELD:$variable=$value\n"
        "\t                       FILE:$variable=$filename\"\n"
        "\t                     (for more informations see manual page)\n")
    },
  {
      NULL,
      "-dumpfd",
      "DumpFD:",
      PARAM_NUM,
      (void *) -1,
      &cfg.dumpfd,
      NULL,
      NULL,
      gettext_nop(
        "\t-dumpfd $nr        - number of filedescriptor where to output\n"
        "\t                     document contents instead to file\n")
    },
  {
      NULL,
      "-dump_urlfd",
      "DumpUrlFD:",
      PARAM_NUM,
      (void *) -1,
      &cfg.dump_urlfd,
      NULL,
      NULL,
      gettext_nop(
        "\t-dump_urlfd $nr    - number of filedescriptor where to output\n"
        "\t                     all URLs found in documents\n")
    },
  {
      NULL,
      "-del_after",
      "DeleteAfterTransfer:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.del_after,
      NULL,
      NULL,
      gettext_nop(
        "\t-del_after/-nodel_after\n"
        "\t                   - delete FTP document after succesfull transfer\n")
    },
  {
      NULL,
      "-nodel_after",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.del_after,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-unique_name",
      "UniqueDocName:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.unique_doc,
      NULL,
      NULL,
      gettext_nop(
        "\t-unique_name/-nounique_name\n"
        "\t                   - always generate unique name for each document\n")
    },
  {
      NULL,
      "-nounique_name",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.unique_doc,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-leave_site_enter_dir",
      "LeaveSiteEnterDirectory:",
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.condition.dont_leave_site_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-leave_site_enter_dir/-dont_leave_site_enter_dir\n"
        "\t                   - leave directory which we first entered on site\n")
    },
  {
      NULL,
      "-dont_leave_site_enter_dir",
      NULL,
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.condition.dont_leave_site_dir,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-singlepage",
      "SinglePage:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.singlepage,
      NULL,
      NULL,
      gettext_nop(
        "\t-singlepage/-nosinglepage\n"
        "\t                   - donwload single HTML page with all inline objects\n")
    },
  {
      NULL,
      "-nosinglepage",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.singlepage,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-dump_after",
      NULL,
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.dump_after,
      NULL,
      NULL,
      gettext_nop(
        "\t-dump_after/-nodump_after\n"
        "\t                   - when used with option -dumpfd, dump document after\n"
        "\t                     successful download and processing of HTML\n"
        "\t                     documents\n")
    },
  {
      NULL,
      "-nodump_after",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.dump_after,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-dump_response",
      NULL,
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.dump_resp,
      NULL,
      NULL,
      gettext_nop(
        "\t-dump_response/-nodump_response\n"
        "\t                   - when used with option -dumpfd, also responses\n"
        "\t                     from HTTP servers will be dumped\n")
    },
  {
      NULL,
      "-nodump_response",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.dump_resp,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-auth_ntlm_domain",
      "NTLMAuthorizationDomain:",
#ifndef ENABLE_NTLM
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.auth_ntlm_domain,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_ntlm_domain $str\n"
        "\t                   - SMB domain name for HTTP NTLM authorization\n")
#endif
    },
  {
      NULL,
      "-auth_proxy_ntlm_domain",
      "NTLMProxyAuthorizationDomain:",
#ifndef ENABLE_NTLM
      PARAM_STR | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_STR,
      (void *) NULL,
      &cfg.auth_proxy_ntlm_domain,
      NULL,
      NULL,
      gettext_nop(
        "\t-auth_proxy_ntlm_domain $str\n"
        "\t                   - SMB domain name for HTTP proxy NTLM authorization\n")
#endif
    },
  {
      NULL,
      "-js_pattern",
      "JavascriptPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL,
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.js_patterns,
      NULL,
      NULL,
      gettext_nop(
        "\t-js_pattern $re    - additional RE patterns for matching URLs in DOM\n"
        "\t                     event attributes of HTML tags\n")
#endif
    },
  {
      NULL,
      "-follow_cmd",
      "FollowCommand:",
      PARAM_STR,
      (void *) NULL,
      &cfg.condition.follow_cmd,
      NULL,
      NULL,
      gettext_nop(
        "\t-follow_cmd $str   - user exit script used tell if you want to follow\n"
        "\t                     any link from current HTML document\n")
    },
  {
      NULL,
      "-retrieve_symlink",
      "RetrieveSymlinks:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.retrieve_slink,
      NULL,
      NULL,
      gettext_nop(
        "\t-retrieve_symlink/-noretrieve_symlink\n"
        "\t                   - retrieve symliks like regular files or directories\n")
    },
  {
      NULL,
      "-noretrieve_symlink",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.retrieve_slink,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-js_transform",
      "JSTransform:",
#ifndef HAVE_REGEX
      PARAM_JSTRANS | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_JSTRANS,
      (void *) NULL,
      &cfg.js_transform,
      (void *) 0,
      NULL,
      gettext_nop(
        "\t-js_transform $p $t $h $a\n"
        "\t                   - custom JS pattern with transformation\n"
        "\t                     $p - pattern\n"
        "\t                     $t - transfomation\n"
        "\t                     $h - HTML tag or * or \"\"\n"
        "\t                     $a - HTML attrib\n")
#endif
    },
  {
      NULL,
      "-js_transform2",
      "JSTransform2:",
#ifndef HAVE_REGEX
      PARAM_JSTRANS | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_JSTRANS,
      (void *) NULL,
      &cfg.js_transform,
      (void *) 1,
      NULL,
      gettext_nop(
        "\t-js_transform2 $p $t $h $a\n"
        "\t                   - custom JS pattern with transformation and\n"
        "\t                     rewriting of URLs in first subpattern\n"
        "\t                     $p - pattern\n"
        "\t                     $t - transfomation\n"
        "\t                     $h - HTML tag or * or \"\"\n"
        "\t                     $a - HTML attrib\n")
#endif
    },
  {
      NULL,
      "-ftp_proxy_user",
      "FTPProxyUser:",
      PARAM_STR,
      NULL,
      &cfg.ftp_proxy_user,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_proxy_user $str\n"
        "\t                   - username for access authorization to FTP proxy\n")
    },
  {
      NULL,
      "-ftp_proxy_pass",
      "FTPProxyPassword:",
      PARAM_PASS,
      NULL,
      &cfg.ftp_proxy_pass,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_proxy_pass $str\n"
        "\t                   - password for access authorization to FTP proxy\n")
    },
  {
      NULL,
      "-limit_inlines",
      "LimitInlineObjects:",
      PARAM_PBOOL,
      (void *) TRUE,
      &cfg.condition.limit_inlines,
      NULL,
      NULL,
      gettext_nop(
        "\t-limit_inlines/-dont_limit_inlines\n"
        "\t                   - apply limiting options on page inline objects?\n")
    },
  {
      NULL,
      "-dont_limit_inlines",
      NULL,
      PARAM_NBOOL,
      (void *) TRUE,
      &cfg.condition.limit_inlines,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-ftp_list_options",
      "FTPListOptions:",
      PARAM_STR,
      NULL,
      &cfg.ftp_list_options,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_list_options $str\n"
        "\t                   - additional options or parameters to FTP LIST or\n"
        "\t                     NLST commands\n")
    },
  {
      NULL,
      "-fix_wuftpd_list",
      "FixWuFTPDBrokenLISTcmd:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.fix_wuftpd,
      NULL,
      NULL,
      gettext_nop(
        "\t-fix_wuftpd_list/-nofix_wuftpd_list\n"
        "\t                   - do extensive checking of FTP directory presence to\n"
        "\t                     workaround WuFTPD broken responses when trying\n"
        "\t                     to list nonexisting directory\n")
    },
  {
      NULL,
      "-nofix_wuftpd_list",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.fix_wuftpd,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-post_update",
      "PostUpdate:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.post_update,
      NULL,
      NULL,
      gettext_nop(
        "\t-post_update/-nopost_update\n"
        "\t                   - update in parent documents only links pointing\n"
        "\t                     to current document\n")
    },
  {
      NULL,
      "-nopost_update",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.post_update,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-info_dir",
      "SeparateInfoDir:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.info_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-info_dir $dir     - separate directory where will stored info files\n"
        "\t                     instead of directly into document tree\n")
    },
  {
      NULL,
      "-mozcache_dir",
      "MozillaCacheDir:",
      PARAM_PATH,
      (void *) NULL,
      &cfg.moz_cache_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-mozcache_dir $dir - path to Mozilla browser cache directory\n")
    },
  {
      NULL,
      "-aport",
      "AllowedPorts:",
      PARAM_NUMLIST,
      (void *) NULL,
      &cfg.condition.ports,
      (void *) TRUE,
      &cfg.condition.allow_ports,
      gettext_nop(
        "\t-aport $list       - allow downloading of documents from servers\n"
        "\t                     siting on listed ports\n")
    },
  {
      NULL,
      "-dport",
      "DisallowedPorts:",
      PARAM_NUMLIST,
      (void *) NULL,
      &cfg.condition.ports,
      (void *) FALSE,
      &cfg.condition.allow_ports,
      gettext_nop(
        "\t-dport $list       - deny downloading of documents from servers\n"
        "\t                     siting on listed ports\n")
    },
  {
      NULL,
      "-hack_add_index",
      "HackAddIndex:",
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.hack_add_index,
      NULL,
      NULL,
      gettext_nop(
        "\t-hack_add_index/-nohack_add_index\n"
        "\t                   - this is hack to allow adding of also directories\n"
        "\t                     of all queued files\n")
    },
  {
      NULL,
      "-nohack_add_index",
      NULL,
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.hack_add_index,
      NULL,
      NULL,
      NULL
    },
  {
      NULL,
      "-default_prefix",
      "DefaultURLPrefix:",
      PARAM_STR,
      (void *) NULL,
      &cfg.default_prefix,
      NULL,
      NULL,
      gettext_nop(
        "\t-default_prefix $str\n"
        "\t                   - default URL prefix for subdirectory of\n"
        "\t                     mirrored files\n")
    },
  {
      NULL,
      "-js_script_file",
      "JavaScriptFile:",
#ifndef HAVE_MOZJS
      PARAM_PATH | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PATH,
      (void *) NULL,
      &cfg.js_script_file,
      NULL,
      NULL,
      gettext_nop(
        "\t-js_script_file $file\n"
        "\t                  - script file with JavaScript functions\n")
#endif
    },
  {
      NULL,
      "-ftp_login_handshake",
      "FtpLoginHandshake:",
      PARAM_FTPHS,
      (void *) NULL,
      &cfg.ftp_login_hs,
      NULL,
      NULL,
      gettext_nop(
        "\t-ftp_login_handshake $host $handshake\n"
        "\t                   - customize login handshake for FTP server\n")
    },
  {
      NULL,
      "-nss_cert_dir",
      "NSSCertDir:",
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_NSS)
      PARAM_PATH | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PATH,
      (void *) FALSE,
      &cfg.nss_cert_dir,
      NULL,
      NULL,
      gettext_nop(
        "\t-nss_cert_dir $dir - set certificate config directory for Netscape NSS\n")
#endif
    },
  {
      NULL,
      "-nss_accept_unknown_cert",
      "NSSAcceptUnknownCert:",
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_NSS)
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.nss_accept_unknown_cert,
      NULL,
      NULL,
      gettext_nop(
        "\t-nss_accept_unknown_cert/-nonss_accept_unknown_cert\n"
        "\t                   - accept SSL server certificates not listed in\n"
        "\t                     certificate database\n")
#endif
    },
  {
      NULL,
      "-nonss_accept_unknown_cert",
      NULL,
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_NSS)
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.nss_accept_unknown_cert,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-nss_domestic_policy",
      "NSSDomesticPolicy:",
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_NSS)
      PARAM_PBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_PBOOL,
      (void *) FALSE,
      &cfg.nss_domestic_policy,
      NULL,
      NULL,
      gettext_nop(
        "\t-nss_domestic_policy/-nss_export_policy\n"
        "\t                   - set policy for SSL ciphers in NSS\n")
#endif
    },
  {
      NULL,
      "-nss_export_policy",
      NULL,
#if !defined(USE_SSL) || !defined(USE_SSL_IMPL_NSS)
      PARAM_NBOOL | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_NBOOL,
      (void *) FALSE,
      &cfg.nss_domestic_policy,
      NULL,
      NULL,
      NULL
#endif
    },
  {
      NULL,
      "-dont_touch_url_rpattern",
      "DontTouchUrlREPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.dont_touch_url_rpattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-dont_touch_url_rpattern $re\n"
        "\t                   - RE pattern of URL which should not be touched\n"
        "\t                     by URL rewriting engine\n")
#endif
    },
  {
      NULL,
      "-dont_touch_url_pattern",
      "DontTouchUrlPattern:",
      PARAM_STRLIST,
      (void *) NULL,
      &cfg.dont_touch_url_pattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-dont_touch_url_pattern $list\n"
        "\t                   - wilcard patterns of URLs which should not\n"
        "\t                     be touched by URL rewriting engine\n")
    },
  {
      NULL,
      "-dont_touch_tag_rpattern",
      "DontTouchTagREPattern:",
#ifndef HAVE_REGEX
      PARAM_RE | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_RE,
      (void *) NULL,
      &cfg.dont_touch_tag_rpattern,
      NULL,
      NULL,
      gettext_nop(
        "\t-dont_touch_tag_rpattern $re\n"
        "\t                   - RE pattern of HTML tag in which URLs should\n"
        "\t                     not be touched by URL rewriting engine\n")
#endif
    },
  {
      NULL,
      "-tag_pattern",
      "HTMLTagPattern:",
      PARAM_TAGPAT,
      (void *) NULL,
      &cfg.condition.tag_patterns,
      (void *) TAGP_WC,
      NULL,
      gettext_nop(
        "\t-tag_pattern $tag $attrib $url\n"
        "\t                   - wilcard patterns for precise matching of URLs\n"
        "\t                     inside HTML tags\n")
    },
  {
      NULL,
      "-tag_rpattern",
      "HTMLTagREPattern:",
#ifndef HAVE_REGEX
      PARAM_TAGPAT | PARAM_UNSUPPORTED, NULL, NULL, NULL, NULL, NULL
#else
      PARAM_TAGPAT,
      (void *) NULL,
      &cfg.condition.tag_patterns,
      (void *) TAGP_RE,
      NULL,
      gettext_nop(
        "\t-tag_rpattern $tag $attrib $url\n"
        "\t                   - RE patterns for precise matching of URLs\n"
        "\t                     inside HTML tags\n")
#endif
    }
};
