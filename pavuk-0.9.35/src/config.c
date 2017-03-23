/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#include "config.h"
#include "tools.h"
#include "ftp.h"
#include "http.h"
#include "gopher.h"
#include "authinfo.h"
#include "times.h"
#include "html.h"
#include "lfname.h"
#include "re.h"
#include "mopt.h"
#include "jstrans.h"
#include "tag_pattern.h"

static void cfg_version_info(void);
static int cfg_load_scenario(const char *);

#include "options.h"

struct strategie_mapt
{
  strategie id;
  char *name;
  char *label;
};

static struct strategie_mapt strategie_map[] = {
  {SSTRAT_DO_SIRKY, "level", gettext_nop("Level order")},
  {SSTRAT_DO_SIRKY_I, "leveli", gettext_nop("Level order, inline first")},
  {SSTRAT_DO_HLBKY, "pre", gettext_nop("Pre order")},
  {SSTRAT_DO_HLBKY_I, "prei", gettext_nop("Pre order, inline first")},
};

static strategie get_strategie_by_str(char *str)
{
  int i;

  for(i = 0; i < SSTRAT_LAST; i++)
  {
    if(!strcasecmp(str, strategie_map[i].name))
      return strategie_map[i].id;
  }

  return SSTRAT_LAST;
}

static char *get_strategie_str(strategie id)
{
  return strategie_map[id].name;
}

char *get_strategie_label(strategie id)
{
  return gettext(strategie_map[id].label);
}

static const struct
{
  char *name;
  int id;
} _ssl_versions[] =
{
  {"ssl23", 1},
  {"ssl2",  2},
  {"ssl3",  3},
  {"tls1",  4}
};

static strategie get_ssl_version_by_str(char *str)
{
  int i;

  for(i = 0; i < NUM_ELEM(_ssl_versions); i++)
  {
    if(!strcasecmp(str, _ssl_versions[i].name))
      return _ssl_versions[i].id;
  }

  return -1;
}

static char *get_ssl_version_str(strategie id)
{
  return _ssl_versions[id - 1].name;
}

/**********************************/
/* show program usage information */
/**********************************/
void usage(void)
{
  int i;

  cfg.bgmode = FALSE;

  xprintf(0,
    gettext("Usage:  %s  [options]  [any number of URLS]\npavuk-%s %s\n"),
    cfg.prg_path, VERSION, HOSTTYPE);

  for(i = 0; i < NUM_ELEM(params); i++)
  {
    if(params[i].help)
      xprintf(0, gettext(params[i].help));
  }

  fflush(stdout);
  exit(PAVUK_EXIT_OK);
}

void usage_short(void)
{
  xprintf(0, "%s %s %s %s\n", PACKAGE, VERSION, REVISION, HOSTTYPE);
  xprintf(0, gettext("Type \"%s --help\" for long help\n"), cfg.prg_path);
  exit(PAVUK_EXIT_CFG_ERR);
}

static void cfg_version_info(void)
{
  xprintf(0, "%s %s %s %s\n", PACKAGE, VERSION, REVISION, HOSTTYPE);
  xprintf(0, gettext("Optional features available :\n"));
#ifdef DEBUG
  xprintf(0, gettext(" - Debug mode\n"));
#endif

#ifdef GETTEXT_NLS
  xprintf(0, gettext(" - GNU gettext internationalization of messages\n"));
#endif

#ifdef HAVE_FLOCK
  xprintf(0, gettext(" - flock() document locking\n"));
#endif

#ifdef HAVE_FCNTL_LOCK
  xprintf(0, gettext(" - fcntl() document locking\n"));
#endif

#ifdef I_FACE
#ifdef GTK_FACE
  xprintf(0, gettext(" - Gtk GUI interface\n"));
#endif
#ifdef WITH_TREE
  xprintf(0, gettext(" - URL tree preview\n"));
#endif
#endif

#ifdef USE_SSL
  xprintf(0, gettext(" - HTTP and FTP over SSL\n"));

#if defined(USE_SSL_IMPL_OPENSSL) && defined(OPENSSL)
#define __SSLIMP "OpenSSL"
#elif defined(USE_SSL_IMPL_OPENSSL)
#define __SSLIMP "SSLeay"
#elif defined(USE_SSL_IMPL_NSS)
#define __SSLIMP "NSS3"
#else
#define __SSLIMP "unknown"
#endif
  xprintf(0, gettext(" - SSL layer implemented with %s library\n"), __SSLIMP);
#endif

#ifdef SOCKS
  xprintf(0, gettext(" - Socks proxy support\n"));
#endif
#ifdef HAVE_FSTATFS
  xprintf(0, gettext(" - filesystem free space checking\n"));
#endif

#ifdef HAVE_REGEX
  xprintf(0,
    gettext
    (" - optional regex patterns in -fnrules and -*rpattern options\n"));
#endif

#ifdef HAVE_POSIX_REGEX
  xprintf(0, gettext(" - POSIX regexp\n"));
#endif

#ifdef HAVE_V8_REGEX
  xprintf(0, gettext(" - Bell V8 regexp\n"));
#endif

#ifdef HAVE_BSD_REGEX
  xprintf(0, gettext(" - BSD regexp\n"));
#endif

#ifdef HAVE_GNU_REGEX
  xprintf(0, gettext(" - GNU regexp\n"));
#endif

#ifdef HAVE_PCRE_REGEX
  xprintf(0, gettext(" - PCRE regexp\n"));
#endif

#ifdef HAVE_BDB_18x
  xprintf(0,
    gettext(" - support for loading files from Netscape browser cache\n"));
#endif

#ifdef HAVE_TERMIOS
  xprintf(0,
    gettext
    (" - support for detecting whether pavuk is running as background job\n"));
#endif

#ifdef HAVE_MT
  xprintf(0, gettext(" - multithreading support\n"));
#endif

#ifdef ENABLE_NTLM
  xprintf(0, gettext(" - NTLM authorization support\n"));
#endif

#ifdef HAVE_MOZJS
  xprintf(0, gettext(" - JavaScript bindings\n"));
#endif

#ifdef HAVE_INET6
  xprintf(0, gettext(" - IPv6 support\n"));
#endif

  exit(PAVUK_EXIT_OK);
}

static int htmltag_set_disabled(char *tagstr, int disable)
{
  int i, j;
  bool_t tfound, afound;
  char *tag;
  char *attrib;
  char *pom, *strtokbuf;

  if(!strcasecmp(tagstr, "all"))
  {
    for(i = 0; i < html_link_tags_num(); i++)
    {
      for(j = 0; html_link_tags[i].attribs[j].attrib; j++)
      {
        if(disable)
          html_link_tags[i].attribs[j].stat |= LINK_DISABLED;
        else
          html_link_tags[i].attribs[j].stat &= ~LINK_DISABLED;
      }
    }

    return -1;
  }

  pom = tl_strdup(tagstr);
  tag = strtokc_r(pom, ',', &strtokbuf);
  attrib = strtokc_r(NULL, ';', &strtokbuf);

  while(tag)
  {
    tfound = FALSE;
    afound = FALSE;
    for(i = 0; i < html_link_tags_num(); i++)
    {
      if(!strcasecmp(html_link_tags[i].tag, tag))
      {
        tfound = TRUE;
        for(j = 0; html_link_tags[i].attribs[j].attrib; j++)
        {
          if(attrib && *attrib)
          {
            if(!strcasecmp(html_link_tags[i].attribs[j].attrib, attrib))
            {
              afound = TRUE;
              if(disable)
                html_link_tags[i].attribs[j].stat |= LINK_DISABLED;
              else
                html_link_tags[i].attribs[j].stat &= ~LINK_DISABLED;
              break;
            }
          }
          else
          {
            afound = TRUE;
            if(disable)
              html_link_tags[i].attribs[j].stat |= LINK_DISABLED;
            else
              html_link_tags[i].attribs[j].stat &= ~LINK_DISABLED;
          }
        }
        break;
      }
    }
    if(!(tfound && afound))
    {
      xprintf(0, gettext("HTML tag not supported : %s.%s\n"), tag,
        attrib ? attrib : "(null)");
    }

    tag = strtokc_r(NULL, ',', &strtokbuf);
    attrib = strtokc_r(NULL, ';', &strtokbuf);
  }

  _free(pom);
  return -1;
}

static void cfg_set_to_default(cfg_param_t * cpar)
{
  char **p;
  int x, j;

  if(cpar->type & PARAM_UNSUPPORTED)
    return;

  if(cpar->type & PARAM_FOREIGN)
    return;

  switch (cpar->type)
  {
  case PARAM_NUM:
    *((long *) cpar->val_adr) = (long) cpar->default_val;
    break;
  case PARAM_PBOOL:
    *((bool_t *) cpar->val_adr) = (bool_t) (long) cpar->default_val;
    break;
  case PARAM_NBOOL:
    *((bool_t *) cpar->val_adr) = (bool_t) (long) cpar->default_val;
    break;
  case PARAM_PORT_RANGE:
    *((long *) cpar->val_adr) = (long) cpar->default_val;
    *((long *) cpar->mval_adr) = (long) cpar->mdefault_val;
    break;
  case PARAM_PATH:
  case PARAM_STR:
  case PARAM_PASS:
    _free(*((char **) cpar->val_adr));
    *((char **) cpar->val_adr) = (char *) cpar->default_val;
    break;
  case PARAM_STRLIST:
    for(p = *((char ***) cpar->val_adr); p && *p; p++)
      _free(*p);
    _free(*(char ***) cpar->val_adr);

    *((char ***) cpar->val_adr) = (char **) cpar->default_val;
    if(cpar->mval_adr)
      *((bool_t *) cpar->mval_adr) = (bool_t) (long) cpar->mdefault_val;
    break;
  case PARAM_CONN:
    _free(*((char **) cpar->val_adr));
    *((char **) cpar->val_adr) = (char *) cpar->default_val;
    if(cpar->mval_adr)
      *((long *) cpar->mval_adr) = (long) cpar->mdefault_val;
    break;
  case PARAM_AUTHSCH:
    *((long *) cpar->val_adr) = (long) cpar->default_val;
    break;
  case PARAM_MODE:
    *((long *) cpar->val_adr) = (long) cpar->default_val;
    break;
  case PARAM_TIME:
    *((time_t *) cpar->val_adr) = (time_t) 0;
    break;
  case PARAM_HTMLTAG:
    for(x = 0; x < html_link_tags_num(); x++)
      for(j = 0; html_link_tags[x].attribs[j].attrib; j++)
        html_link_tags[x].attribs[j].stat &= ~LINK_DISABLED;
    break;
  case PARAM_TWO_QSTR:
    *((char **) cpar->val_adr) = (char *) cpar->default_val;
    *((char **) cpar->mval_adr) = (char *) cpar->mdefault_val;
    break;
  case PARAM_DOUBLE:
    *((double *) cpar->val_adr) = *(double *) cpar->default_val;
    break;
  case PARAM_LFNAME:
    while(cfg.lfnames)
    {
      lfname_free((lfname *) cfg.lfnames->data);
      cfg.lfnames = dllist_remove_entry(cfg.lfnames, cfg.lfnames);
    }
    break;
  case PARAM_RE:
#ifdef HAVE_REGEX
    {
      dllist *ptr = *((dllist **) cpar->val_adr);

      *((dllist **) cpar->val_adr) = NULL;

      while(ptr)
      {
        re_free((re_entry *) ptr->data);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
#endif
    break;
  case PARAM_USTRAT:
    *((strategie *) cpar->val_adr) = (strategie) cpar->default_val;
    break;
  case PARAM_SSLVER:
    *((long *) cpar->val_adr) = (long) cpar->default_val;
    break;
  case PARAM_HTTPHDR:
    {
      dllist *ptr = *(dllist **) cpar->val_adr;
      *(dllist **) cpar->val_adr = NULL;

      while(ptr)
      {
        httphdr_free((httphdr *)ptr->data);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
    break;
  case PARAM_DEBUGL:
    *((long *) cpar->val_adr) = (long) cpar->default_val;
    break;
  case PARAM_REQUEST:
    {
      dllist *ptr = *(dllist **) cpar->val_adr;
      *(dllist **) cpar->val_adr = NULL;
      while(ptr)
      {
        url_info_free((url_info *) ptr->data);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
    break;
  case PARAM_TRANSPARENT:
    {
      if(cpar->val_adr)
      {
        http_proxy *pr = *((http_proxy **) cpar->val_adr);
        if(pr)
          http_proxy_free(pr);
      }
    }
    break;
  case PARAM_PROXY:
    {
      dllist *ptr = *((dllist **) cpar->val_adr);
      *((dllist **) cpar->val_adr) = NULL;

      while(ptr)
      {
        http_proxy *pr = (http_proxy *) ptr->data;
        http_proxy_unref(pr);
        ptr = dllist_remove_entry(ptr, ptr);
      }
    }
    break;
  case PARAM_FUNC:
    break;
  case PARAM_JSTRANS:
#ifdef HAVE_REGEX
    while(cfg.js_transform)
    {
      js_transform_free((js_transform_t *) cfg.js_transform->data);
      cfg.js_transform =
        dllist_remove_entry(cfg.js_transform, cfg.js_transform);
    }
#endif
    break;
  case PARAM_NUMLIST:
    {
      dllist *ptr = *((dllist **) cpar->val_adr);
      *((dllist **) cpar->val_adr) = NULL;
      while(ptr)
        ptr = dllist_remove_entry(ptr, ptr);
      if(cpar->mval_adr)
        *((bool_t *) cpar->mval_adr) = (bool_t) (long) cpar->mdefault_val;
    }
    break;
  case PARAM_FTPHS:
    {
      dllist *ptr = *((dllist **) cpar->val_adr);
      *((dllist **) cpar->val_adr) = NULL;
      for(; ptr; ptr = dllist_remove_entry(ptr, ptr))
        ftp_handshake_info_free((ftp_handshake_info *)ptr->data);
    }
    break;
  case PARAM_TAGPAT:
    {
      dllist *ptr = *((dllist **) cpar->val_adr);
      *((dllist **) cpar->val_adr) = NULL;
      for(; ptr; ptr = dllist_remove_entry(ptr, ptr))
        tag_pattern_free((tag_pattern_t *)ptr->data);
    }
    break;
  }
}

void cfg_set_all_to_default(void)
{
  int i;

  for(i = 0; i < NUM_ELEM(params); i++)
    cfg_set_to_default(&(params[i]));
}

void cfg_setup_default(void)
{
  int i, x, j;

  for(i = 0; i < NUM_ELEM(params); i++)
  {
    if(params[i].type & PARAM_UNSUPPORTED)
      continue;

    if(params[i].type & PARAM_FOREIGN)
      continue;

    switch (params[i].type)
    {
    case PARAM_NUM:
      *((long *) params[i].val_adr) = (long) params[i].default_val;
      break;
    case PARAM_PBOOL:
      *((bool_t *) params[i].val_adr) = (bool_t) (long) params[i].default_val;
      break;
    case PARAM_NBOOL:
      *((bool_t *) params[i].val_adr) = (bool_t) (long) params[i].default_val;
      break;
    case PARAM_PORT_RANGE:
      *((long *) params[i].val_adr) = (long) params[i].default_val;
      *((long *) params[i].mval_adr) = (long) params[i].mdefault_val;
      break;
    case PARAM_PATH:
    case PARAM_STR:
    case PARAM_PASS:
      *((char **) params[i].val_adr) = (char *) params[i].default_val;
      break;
    case PARAM_STRLIST:
      *((char ***) params[i].val_adr) = (char **) params[i].default_val;
      if(params[i].mval_adr)
        *((bool_t *) params[i].mval_adr) =
          (bool_t) (long) params[i].mdefault_val;
      break;
    case PARAM_CONN:
      *((char **) params[i].val_adr) = (char *) params[i].default_val;
      if(params[i].mval_adr)
        *((long *) params[i].mval_adr) = (long) params[i].mdefault_val;
      break;
    case PARAM_AUTHSCH:
      *((long *) params[i].val_adr) = (long) params[i].default_val;
      break;
    case PARAM_MODE:
      *((long *) params[i].val_adr) = (long) params[i].default_val;
      break;
    case PARAM_TIME:
      *((time_t *) params[i].val_adr) = (time_t) 0;
      break;
    case PARAM_HTMLTAG:
      for(x = 0; x < html_link_tags_num(); x++)
        for(j = 0; html_link_tags[x].attribs[j].attrib; j++)
          html_link_tags[x].attribs[j].stat &= ~LINK_DISABLED;
      break;
    case PARAM_TWO_QSTR:
      *((char **) params[i].val_adr) = (char *) params[i].default_val;
      *((char **) params[i].mval_adr) = (char *) params[i].mdefault_val;
      break;
    case PARAM_DOUBLE:
      *((double *) params[i].val_adr) = *(double *) params[i].default_val;
      break;
    case PARAM_LFNAME:
      cfg.lfnames = NULL;
      break;
    case PARAM_RE:
#ifdef HAVE_REGEX
      *((dllist **) params[i].val_adr) = NULL;
#endif
      break;
    case PARAM_USTRAT:
      *((strategie *) params[i].val_adr) = (strategie) params[i].default_val;
      break;
    case PARAM_SSLVER:
      *((long *) params[i].val_adr) = (long) params[i].default_val;
      break;
    case PARAM_HTTPHDR:
      *((dllist **) params[i].val_adr) = NULL;
      break;
    case PARAM_DEBUGL:
      *((long *) params[i].val_adr) = (long) params[i].default_val;
      break;
    case PARAM_REQUEST:
      *((dllist **) params[i].val_adr) = NULL;
      break;
    case PARAM_TRANSPARENT:
      *((http_proxy **) params[i].val_adr) = NULL;
      break;
    case PARAM_PROXY:
      *((dllist **) params[i].val_adr) = NULL;
      break;
    case PARAM_FUNC:
      break;
    case PARAM_JSTRANS:
#ifdef HAVE_REGEX
      cfg.js_transform = NULL;
#endif
      break;
    case PARAM_NUMLIST:
      *((dllist **) params[i].val_adr) = (dllist *) params[i].default_val;
      if(params[i].mval_adr)
        *((bool_t *) params[i].mval_adr) =
          (bool_t) (long) params[i].mdefault_val;
      break;
    case PARAM_FTPHS:
      *((dllist **) params[i].val_adr) = (dllist *) params[i].default_val;
      break;
    case PARAM_TAGPAT:
      *((dllist **) params[i].val_adr) = (dllist *) params[i].default_val;
      break;
    }
  }
}

int cfg_get_num_params(cfg_param_t * cpar)
{
  long num;

  static const struct
  {
    par_type_t type;
    int num_params;
  } tab[] =
  {
    {PARAM_NUM, 1},
    {PARAM_PBOOL, 0},
    {PARAM_NBOOL, 0},
    {PARAM_STR, 1},
    {PARAM_PASS, 1},
    {PARAM_STRLIST, 1},
    {PARAM_CONN, 1},
    {PARAM_AUTHSCH, 1},
    {PARAM_MODE, 1},
    {PARAM_PATH, 1},
    {PARAM_TIME, 1},
    {PARAM_HTMLTAG, 1},
    {PARAM_TWO_QSTR, 2},
    {PARAM_DOUBLE, 1},
    {PARAM_LFNAME, 3},
    {PARAM_RE, 1},
    {PARAM_USTRAT, 1},
    {PARAM_SSLVER, 1},
    {PARAM_HTTPHDR, 1},
    {PARAM_DEBUGL, 1},
    {PARAM_REQUEST, 1},
    {PARAM_PROXY, 1},
    {PARAM_TRANSPARENT, 1},
    {PARAM_FUNC, 0},
    {PARAM_JSTRANS, 4},
    {PARAM_NUMLIST, 1},
    {PARAM_FTPHS, 2},
    {PARAM_TAGPAT, 3},
    {PARAM_PORT_RANGE, 1}
  };

  num = tab[cpar->type].num_params;

  if(cpar->type == PARAM_FUNC)
    num = (long) cpar->default_val;

  return num;
}

static char *cfg_get_option_string(cfg_param_t * param, int type)
{
  switch (type)
  {
  case MOPT_OPT_SHORT:
    return param->short_cmd;
  case MOPT_OPT_LONG:
    return param->long_cmd;
  case MOPT_OPT_COMPAT:
    return param->long_cmd;
  default:
    return "";
  }
}

void cfg_setup_cmdln(int argc, char **argv)
{
  int nr = 0;
  double dnr = 0.0;
  char *p = NULL;
  char **pl = NULL;
  mopt_t mopt;
  cfg_param_t *cpar;
  int moptrv;

  mopt_init(&mopt, NUM_ELEM(params), params, argc, argv);

  for(;;)
  {
    moptrv = mopt_get_next_param(&mopt, &cpar);

    if(moptrv == MOPT_END)
      break;

    if(moptrv == MOPT_ERR)
    {
      xprintf(0, gettext("Error parsing commandline\n"));
      usage_short();
      break;
    }

    if(moptrv == MOPT_MISSINGP)
    {
      xprintf(0,
        gettext("Not enough number of parameters for \"-%s\" option\n"),
        cfg_get_option_string(cpar, mopt.option_type));
      usage_short();
      break;
    }

    if(moptrv == MOPT_UNKNOWN)
    {
      xprintf(0, gettext("Unknown option %s\n"), argv[mopt.current]);
      usage_short();
      break;
    }

    if(moptrv == MOPT_BAD)
    {
      xprintf(0, gettext("Wrong format of option %s\n"), argv[mopt.current]);
      usage_short();
      break;
    }

    if(moptrv == MOPT_OK)
    {
      if(cpar->type & PARAM_UNSUPPORTED)
      {
        xprintf(0,
          gettext
          ("WARNING: option \"-%s\" not supported in current configuration!\n"),
          cfg_get_option_string(cpar, mopt.option_type));
        continue;
      }

      if(cpar->type & PARAM_FOREIGN)
        continue;

      switch (cpar->type)
      {
      case PARAM_NUM:
        nr = _atoi(mopt.args[0]);
        if(errno == ERANGE)
        {
          xprintf(0,
            gettext("Please specify number with parameter \"-%s\"\n"),
            cfg_get_option_string(cpar, mopt.option_type));
          usage_short();
        }
        *((int *) cpar->val_adr) = nr;
        break;
      case PARAM_PBOOL:
        *((bool_t *) cpar->val_adr) = TRUE;
        break;
      case PARAM_NBOOL:
        *((bool_t *) cpar->val_adr) = FALSE;
        break;
      case PARAM_PORT_RANGE:
        if(sscanf(mopt.args[0], "%ld:%ld",
            (long *) cpar->val_adr,
            (long *) cpar->mval_adr) != 2
          || *((long *) cpar->val_adr) <= 1023 ||
          *((long *) cpar->mval_adr) > 65535 ||
          *((long *) cpar->val_adr) >= *((long *) cpar->mval_adr))
        {
          xprintf(0, gettext("Invalid port range \"%s\"\n"), mopt.args[0]);
          usage_short();
        }
        break;
      case PARAM_PASS:
      case PARAM_STR:
        cfg_set_to_default(cpar);
        if(mopt.args[0][0])
          p = tl_strdup(mopt.args[0]);
        else
          p = NULL;
        *((char **) cpar->val_adr) = p;
        if(cpar->type == PARAM_PASS)
        {
          if(mopt.args[0][0])
          {
            memset(mopt.args[0], ' ', strlen(mopt.args[0]));
            strcpy(mopt.args[0], "*");
          }
        }
        break;
      case PARAM_PATH:
        cfg_set_to_default(cpar);
        if(mopt.args[0][0])
        {
#ifdef __CYGWIN__
          p = cvt_win32_to_unix_path(mopt.args[0]);
#else
          p = get_abs_file_path_oss(mopt.args[0]);
#endif
        }
        *((char **) cpar->val_adr) = p;
        break;
      case PARAM_STRLIST:
        cfg_set_to_default(cpar);
        pl = tl_str_split(mopt.args[0], ",");
        if(cpar->mval_adr)
          *((bool_t *) cpar->mval_adr) = (bool_t) (long) cpar->mdefault_val;
        *((char ***) cpar->val_adr) = pl;
        break;
      case PARAM_CONN:
        cfg_set_to_default(cpar);
        if(mopt.args[0][0])
        {
          p = strchr(mopt.args[0], ':');
          if(p)
          {
            nr = _atoi(p + 1);
            if(errno == ERANGE)
            {
              struct servent *se;

              if((se = getservbyname(p + 1, "tcp")))
              {
                nr = ntohs(se->s_port);
              }
              else
              {
                xprintf(0, gettext("Unknown port \"%s\"\n"), p + 1);
              }
            }
            if(cpar->mval_adr)
              *((int *) cpar->mval_adr) = (int) nr;
          }
        }
        else
          p = NULL;

        *((char **) cpar->val_adr) =
          p ? tl_strndup(mopt.args[0],
          p - mopt.args[0]) : tl_strdup(mopt.args[0]);
        break;
      case PARAM_AUTHSCH:
        nr = authinfo_get_type(mopt.args[0]);
        if(nr == HTTP_AUTH_NONE)
        {
          xprintf(0, gettext("Bad auth scheme \"%s\"\n"), mopt.args[0]);
          usage_short();
        }
        else
          *((int *) cpar->val_adr) = nr;
        break;
      case PARAM_MODE:
        cfg.mode = mode_get_by_str(mopt.args[0]);
        if(cfg.mode == MODE_UNKNOWN)
        {
          xprintf(0, gettext("Unknow operation mode \"%s\"\n"), mopt.args[0]);
          usage_short();
        }
        break;
      case PARAM_TIME:
        *((time_t *) cpar->val_adr) = time_scn_cmd(mopt.args[0]);
        break;
      case PARAM_HTMLTAG:
        htmltag_set_disabled(mopt.args[0], (long) cpar->default_val);
        break;
      case PARAM_TWO_QSTR:
        cfg_set_to_default(cpar);
        p = tl_strdup(mopt.args[0]);
        *((char **) cpar->val_adr) = p;

        p = tl_strdup(mopt.args[1]);
        *((char **) cpar->mval_adr) = p;
        break;
      case PARAM_DOUBLE:
        dnr = _atof(mopt.args[0]);
        if(errno == ERANGE)
        {
          xprintf(0,
            gettext
            ("Please specify floating number with parameter \"-%s\"\n"),
            cfg_get_option_string(cpar, mopt.option_type));
          usage_short();
        }
        *((double *) cpar->val_adr) = dnr;
        break;
      case PARAM_LFNAME:
        {
          lfname_type t;
          lfname *lfnm;

          if(!strcasecmp(mopt.args[0], "F"))
            t = LFNAME_FNMATCH;
#ifdef HAVE_REGEX
          else if(!strcasecmp(mopt.args[0], "R"))
            t = LFNAME_REGEX;
#endif
          else
          {
            t = LFNAME_UNKNOWN;
#ifdef HAVE_REGEX
#define __CONDITIONS "F or R"
#else
#define __CONDITIONS "F"
#endif
            xprintf(0,
              gettext("Please specify proper condition type for -%s (%s)\n"),
              cfg_get_option_string(cpar, mopt.option_type), __CONDITIONS);
#undef __CONDITIONS
            usage_short();
          }
          lfnm = lfname_new(t, mopt.args[1], mopt.args[2]);
          if(!lfnm)
            usage_short();

          cfg.lfnames = dllist_append(cfg.lfnames, (dllist_t) lfnm);
        }
        break;
      case PARAM_RE:
#ifdef HAVE_REGEX
        {
          re_entry *ree = NULL;

          if(!(ree = re_make(mopt.args[0])))
          {
            xprintf(0,
              gettext("Please specify valid RE with parameter \"-%s\"\n"),
              cfg_get_option_string(cpar, mopt.option_type));
            usage_short();
          }
          *((dllist **) cpar->val_adr) =
            dllist_append(*((dllist **) cpar->val_adr), (dllist_t) ree);
        }
#endif
        break;
      case PARAM_USTRAT:
        *(strategie *) cpar->val_adr = get_strategie_by_str(mopt.args[0]);
        if(*(strategie *) cpar->val_adr == SSTRAT_LAST)
        {
          xprintf(0, gettext("Unknown URL scheduling strategy - \"%s\"\n"),
            mopt.args[0]);
          usage_short();
        }
        break;
      case PARAM_SSLVER:
        *(int *) cpar->val_adr = get_ssl_version_by_str(mopt.args[0]);
        if(*(int *) cpar->val_adr == -1)
        {
          xprintf(0, gettext("Unknown SSL version - \"%s\"\n"), mopt.args[0]);
          usage_short();
        }
        break;
      case PARAM_HTTPHDR:
        {
          httphdr *hr = httphdr_parse(mopt.args[0]);
          if(!hr)
          {
            xprintf(0, gettext("Invalid additional HTTP header - \"%s\"\n"),
              mopt.args[0]);
            usage_short();
          }
          *((dllist **) cpar->val_adr) =
            dllist_append(*((dllist **) cpar->val_adr), (dllist_t) hr);
        }
        break;
      case PARAM_DEBUGL:
#ifdef DEBUG
        {
          int dl = debug_level_parse(mopt.args[0]);
          if(dl == -1)
          {
            usage_short();
          }
          *((int *) cpar->val_adr) = dl;
        }
#endif
        break;
      case PARAM_REQUEST:
        {
          url_info *ui = url_info_parse(mopt.args[0]);
          if(!ui)
          {
            xprintf(0, gettext("Invalid request specification - \"%s\"\n"),
              mopt.args[0]);
            usage_short();
          }
          *((dllist **) cpar->val_adr) =
            dllist_append(*((dllist **) cpar->val_adr), (dllist_t) ui);
        }
        break;
      case PARAM_TRANSPARENT:
        if(mopt.args[0][0])
        {
          http_proxy *pr = http_proxy_parse(mopt.args[0]);

          if(!pr)
            usage_short();
          else
            *((http_proxy **) cpar->val_adr) = pr;
        }
        else
        {
          cfg_set_to_default(cpar);
        }
        break;
      case PARAM_PROXY:
        if(mopt.args[0][0])
        {
          http_proxy *pr = http_proxy_parse(mopt.args[0]);

          if(!pr)
            usage_short();
          else
            *((dllist **) cpar->val_adr) =
              dllist_append(*((dllist **) cpar->val_adr), (dllist_t) pr);
        }
        else
        {
          cfg_set_to_default(cpar);
        }
        break;
      case PARAM_FUNC:
        {
          int (*_cfg_func) (char *, char *, char *, char *);

          _cfg_func = cpar->val_adr;

          if(_cfg_func)
          {
            if(_cfg_func(mopt.args[0], mopt.args[1], mopt.args[2],
                mopt.args[3]))
            {
              usage_short();
            }
          }
        }
        break;
      case PARAM_JSTRANS:
#ifdef HAVE_REGEX
        {
          js_transform_t *jt;

          jt = js_transform_new(mopt.args[0], mopt.args[1], mopt.args[2],
          mopt.args[3], (long) cpar->mdefault_val);

          if(!jt)
          {
            xprintf(0, gettext("Invalid parameters for \"-%s\" option\n"),
              cfg_get_option_string(cpar, mopt.option_type));
            usage_short();
          }
          else
          {
            cfg.js_transform = dllist_append(cfg.js_transform, (dllist_t) jt);
          }
        }
#endif
        break;
      case PARAM_NUMLIST:
        {
          dllist *ptr = tl_numlist_split(mopt.args[0], ",");

          if(!ptr && mopt.args[0][0])
          {
            xprintf(0,
              gettext("Invalid number list \"%s\" for option \"-%s\"\n"),
              mopt.args[0], cfg_get_option_string(cpar, mopt.option_type));
            usage_short();
          }
          cfg_set_to_default(cpar);
          if(cpar->mval_adr)
            *((bool_t *) cpar->mval_adr) = (bool_t) (long) cpar->mdefault_val;
          *((dllist **) cpar->val_adr) = ptr;
        }
        break;
      case PARAM_FTPHS:
        {
          ftp_handshake_info *fhi;
          fhi = ftp_handshake_info_parse(mopt.args[0], mopt.args[1]);

          if(!fhi)
          {
            xprintf(0,
              gettext
              ("Invalid FTP login handshake string \"%s\" for option \"-%s\"\n"),
              mopt.args[1], cfg_get_option_string(cpar, mopt.option_type));
            usage_short();
          }
          *((dllist **) cpar->val_adr) =
            dllist_append(*((dllist **) cpar->val_adr), (dllist_t) fhi);
        }
        break;
      case PARAM_TAGPAT:
        {
          tag_pattern_t *tp;
          tp = tag_pattern_new((long) cpar->mdefault_val,
            mopt.args[0], mopt.args[1], mopt.args[2]);
          if(!tp)
            usage_short();
          *((dllist **) cpar->val_adr) =
            dllist_append(*((dllist **) cpar->val_adr), (dllist_t) tp);
        }
        break;
      }
    }

    if(moptrv == MOPT_PARAM)
    {
      url_info *ui;
      ui = url_info_new(mopt.args[0]);
      cfg.request = dllist_append(cfg.request, (dllist_t) ui);
      cfg.total_cnt++;
    }
  }
  mopt_destroy(&mopt);
}

static int cfg_load_fd(bufio * fd)
{
  int i;
  bool_t found;
  int nr;
  double dnr;
  char *p;
  char lnbuf[4096];
  char *lns;
  pavuk_mode temp_mode;
  int rv = 0;

  while(bufio_readln(fd, lnbuf, sizeof(lnbuf)) > 0)
  {
    strip_nl(lnbuf);
    for(lns = lnbuf; *lns && tl_ascii_isspace(*lns); lns++);
    if(*lns == '#' || !*lns)
      continue;

    found = FALSE;

    for(i = 0; i < NUM_ELEM(params); i++)
    {
      if(!params[i].par_entry)
        continue;
      if(!strncasecmp(lns, params[i].par_entry, strlen(params[i].par_entry)))
      {
        if(params[i].type & PARAM_UNSUPPORTED)
        {
          xprintf(0,
            gettext
            ("WARNING: option \"-%s\" not supported in current configuration!\n"),
            params[i].par_entry);
          continue;
        }

        if(params[i].type & PARAM_FOREIGN)
          continue;

        lns += strlen(params[i].par_entry);
        for(; *lns && tl_ascii_isspace(*lns); lns++);
        for(p = lns + strlen(lns) - 1; p >= lns && tl_ascii_isspace(*p); p--)
          *p = '\0';

        if(!*lns)
        {
          cfg_set_to_default(&(params[i]));
          continue;
        }

        found = TRUE;
        switch (params[i].type)
        {
        case PARAM_NUM:
          nr = _atoi(lns);
          if(errno == ERANGE)
          {
            xprintf(0, gettext("Please specify number \"%s\"\n"),
              params[i].par_entry);
            rv = 1;
          }
          else
            *((int *) params[i].val_adr) = nr;
          break;
        case PARAM_PBOOL:
        case PARAM_NBOOL:
          if(!strcasecmp(lns, "false"))
          {
            *((bool_t *) params[i].val_adr) = FALSE;
          }
          else if(!strcasecmp(lns, "true"))
          {
            *((bool_t *) params[i].val_adr) = TRUE;
          }
          else
          {
            xprintf(0,
              gettext("Only \"true\" & \"false\" is allowed : \"%s\"\n"),
              lns);
            rv = 1;
          }
          break;
        case PARAM_PORT_RANGE:
          if(sscanf(lns, "%ld:%ld",
              (long *) params[i].val_adr,
              (long *) params[i].mval_adr) != 2
            || *((long *) params[i].val_adr) <= 1023 ||
            *((long *) params[i].mval_adr) > 65535 ||
            *((long *) params[i].val_adr) >= *((long *) params[i].mval_adr))
          {
            xprintf(0, gettext("Invalid port range \"%s\"\n"), lns);
            rv = 1;
          }
          break;
        case PARAM_STR:
        case PARAM_PASS:
          cfg_set_to_default(&params[i]);
          *((char **) params[i].val_adr) = *lns ? tl_strdup(lns) : NULL;
          break;
        case PARAM_PATH:
          cfg_set_to_default(&params[i]);
#ifdef __CYGWIN__
          *((char **) params[i].val_adr) =
            *lns ? cvt_win32_to_unix_path(lns) : NULL;
#else
          *((char **) params[i].val_adr) =
            *lns ? get_abs_file_path_oss(lns) : NULL;
#endif
          break;
        case PARAM_STRLIST:
          cfg_set_to_default(&params[i]);
          if(params[i].mval_adr)
            *((bool_t *) params[i].mval_adr) =
              (bool_t) (long) params[i].mdefault_val;
          if(*lns)
            *((char ***) params[i].val_adr) = tl_str_split(lns, ",");
          break;
        case PARAM_CONN:
          cfg_set_to_default(&params[i]);
          p = strchr(lns, ':');
          if(p)
          {
            nr = _atoi(p + 1);
            if(errno == ERANGE)
            {
              struct servent *se;

              if((se = getservbyname(p + 1, "tcp")))
              {
                nr = ntohs(se->s_port);
              }
              else
              {
                xprintf(0, gettext("Unknown port \"%s\"\n"), p + 1);
                rv = 1;
              }
            }
            if(params[i].mval_adr)
              *((int *) params[i].mval_adr) = (int) nr;
          }
          *((char **) params[i].val_adr) =
            p ? tl_strndup(lns, p - lns) : tl_strdup(lns);
          break;
        case PARAM_AUTHSCH:
          nr = authinfo_get_type(lns);
          if(nr == HTTP_AUTH_NONE)
          {
            xprintf(0, gettext("Bad auth scheme \"%s\"\n"), lns);
            rv = 1;
          }
          else
            *((int *) params[i].val_adr) = nr;
          break;
        case PARAM_MODE:
          temp_mode = mode_get_by_str(lns);
          if(temp_mode == MODE_UNKNOWN)
          {
            xprintf(0, gettext("Unknow operation mode \"%s\"\n"), lns);
            rv = 1;
          }
          else
            *((pavuk_mode *) params[i].val_adr) = temp_mode;
          break;
        case PARAM_TIME:
          {
            time_t ttm = time_scn_cmd(lns);
            if(!params[i].val_adr)
            {
              xprintf(0, gettext("Bad time parameter \"%s\"\n"), lns);
              rv = 1;
            }
            else
              *(time_t *) params[i].val_adr = ttm;
          }
          break;
        case PARAM_HTMLTAG:
          htmltag_set_disabled(lns, (long) params[i].default_val);
          break;
        case PARAM_TWO_QSTR:
          cfg_set_to_default(&params[i]);
          if(lns && *lns)
          {
            char *xp = tl_strdup(lns);

            *(char **) params[i].val_adr = tl_strdup(get_1qstr(xp));
            *(char **) params[i].mval_adr = tl_strdup(get_1qstr(NULL));

            _free(xp);
          }
          else
          {
            *(char **) params[i].val_adr = NULL;
            *(char **) params[i].mval_adr = NULL;
          }
          break;
        case PARAM_DOUBLE:
          dnr = _atof(lns);
          if(errno == ERANGE)
          {
            xprintf(0, gettext("Please specify floating number \"%s\"\n"),
              params[i].par_entry);
            rv = 1;
          }
          else
            *(double *) params[i].val_adr = dnr;
          break;
        case PARAM_LFNAME:
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            char *ps3 = tl_strdup(get_1qstr(NULL));
            lfname_type t;
            lfname *lfnm;

            if(!ps1 || !ps2 || !ps3)
            {
              t = LFNAME_UNKNOWN;
              xprintf(0, gettext("Please specify proper arguments for %s\n"),
                params[i].par_entry);
              rv = 1;
            }
            else if(!strcasecmp(ps1, "F"))
              t = LFNAME_FNMATCH;
#ifdef HAVE_REGEX
            else if(!strcasecmp(ps1, "R"))
              t = LFNAME_REGEX;
#endif
            else
            {
              t = LFNAME_UNKNOWN;
              xprintf(0,
                gettext("Please specify proper condition type for %s (%s)\n"),
                params[i].par_entry,
#ifdef HAVE_REGEX
                "F or R"
#else
                "F"
#endif
                );
              rv = 1;
            }
            if(t != LFNAME_UNKNOWN)
            {
              lfnm = lfname_new(t, ps2, ps3);
              if(lfnm)
                cfg.lfnames = dllist_append(cfg.lfnames, (dllist_t) lfnm);
              else
                rv = 1;
            }
            _free(ps1);
            _free(ps2);
            _free(ps3);
          }
          break;
        case PARAM_RE:
#ifdef HAVE_REGEX
          {
            re_entry *ree;
            if(!(ree = re_make(lns)))
            {
              xprintf(0, gettext("Please specify valid RE \"%s\"\n"),
                params[i].par_entry);
              rv = 1;
            }
            else
              *(dllist **) params[i].val_adr =
                dllist_append(*((dllist **) params[i].val_adr), (dllist_t) ree);
          }
#endif
          break;
        case PARAM_USTRAT:
          {
            strategie strtg = get_strategie_by_str(lns);
            if(strtg == SSTRAT_LAST)
            {
              xprintf(0,
                gettext("Unknown URL scheduling strategy - \"%s\"\n"), lns);
              rv = 1;
            }
            else
              *(strategie *) params[i].val_adr = strtg;
          }
          break;
        case PARAM_SSLVER:
          {
            int sslv = get_ssl_version_by_str(lns);
            if(sslv == -1)
            {
              xprintf(0, gettext("Unknown SSL version - \"%s\"\n"), lns);
              rv = 1;
            }
            else
              *(int *) params[i].val_adr = sslv;
          }
          break;
        case PARAM_HTTPHDR:
          {
            httphdr *hr = httphdr_parse(lns);
            if(!hr)
            {
              xprintf(0, gettext("Invalid additional HTTP header - \"%s\"\n"),
                lns);
              rv = 1;
            }
            else
              *((dllist **) params[i].val_adr) =
                dllist_append(*((dllist **) params[i].val_adr), (dllist_t) hr);
          }
          break;
        case PARAM_DEBUGL:
#ifdef DEBUG
          {
            int dl = debug_level_parse(lns);
            if(dl != -1)
              *((int *) params[i].val_adr) = dl;
            else
              rv = 1;
          }
#endif
          break;
        case PARAM_REQUEST:
          {
            url_info *ui = url_info_parse(lns);
            if(!ui)
            {
              xprintf(0, gettext("Invalid request specification - \"%s\"\n"),
                lns);
              rv = 1;
            }
            else
              *((dllist **) params[i].val_adr) =
                dllist_append(*((dllist **) params[i].val_adr), (dllist_t) ui);
          }
          break;
        case PARAM_TRANSPARENT:
          if(lns)
          {
            http_proxy *pr = http_proxy_parse(lns);
            if(pr)
              *((http_proxy **) params[i].val_adr) = pr;
            else
              rv = 1;
          }
          break;
        case PARAM_PROXY:
          if(lns)
          {
            http_proxy *pr = http_proxy_parse(lns);

            if(pr)
              *((dllist **) params[i].val_adr) =
                dllist_append(*((dllist **) params[i].val_adr), (dllist_t) pr);
            else
              rv = 1;
          }
          break;
        case PARAM_FUNC:
          break;
        case PARAM_JSTRANS:
#ifdef HAVE_REGEX
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            char *ps3 = tl_strdup(get_1qstr(NULL));
            char *ps4 = tl_strdup(get_1qstr(NULL));

            js_transform_t *jt;

            jt = js_transform_new(ps1, ps2, ps3, ps4,
            (long) params[i].mdefault_val);

            if(!jt)
            {
              xprintf(0,
                gettext("Invalid js_transform specification - \"%s\"\n"),
                lns);
              rv = 1;
            }
            else
              cfg.js_transform = dllist_append(cfg.js_transform, (dllist_t) jt);
            _free(ps1);
            _free(ps2);
            _free(ps3);
            _free(ps4);
          }
#endif
          break;
        case PARAM_NUMLIST:
          cfg_set_to_default(&params[i]);
          if(params[i].mval_adr)
            *((bool_t *) params[i].mval_adr) =
              (bool_t) (long) params[i].mdefault_val;
          if(*lns)
            *((dllist **) params[i].val_adr) = tl_numlist_split(lns, ",");
          if(*lns && !*((dllist **) params[i].val_adr))
          {
            xprintf(0,
              gettext("Invalid number list specification - \"%s\"\n"), lns);
            rv = 1;
          }
          break;
        case PARAM_FTPHS:
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            ftp_handshake_info *fhi = NULL;

            if(ps1 && ps2)
              fhi = ftp_handshake_info_parse(ps1, ps2);

            if(!fhi)
            {
              xprintf(0,
                gettext("Invalid FTP login handshake string \"%s\".\n"), lns);
              rv = 1;
            }
            *((dllist **) params[i].val_adr) =
              dllist_append(*((dllist **) params[i].val_adr), (dllist_t) fhi);
            _free(ps1);
            _free(ps2);
          }
          break;
        case PARAM_TAGPAT:
          {
            char *ps1 = tl_strdup(get_1qstr(lns));
            char *ps2 = tl_strdup(get_1qstr(NULL));
            char *ps3 = tl_strdup(get_1qstr(NULL));
            tag_pattern_t *tp = NULL;

            if(ps1 && ps2 && ps3)
	    {
              tp = tag_pattern_new((long) params[i].mdefault_val,
	      ps1, ps2, ps3);
            }
            if(!tp)
              rv = 1;
            *((dllist **) params[i].val_adr) =
              dllist_append(*((dllist **) params[i].val_adr), (dllist_t) tp);
            _free(ps1);
            _free(ps2);
            _free(ps3);
          }
          break;
        }
      }
    }
    if(!found && !strncasecmp(lns, "URL:", 4))
    {
      url_info *ui;

      lns += 4;
      for(; *lns && tl_ascii_isspace(*lns); lns++);
      for(p = lns + strlen(lns); *p && tl_ascii_isspace(*p); p--)
        *p = '\0';
      if(!*lns)
        continue;

      ui = url_info_new(lns);
      cfg.request = dllist_append(cfg.request, (dllist_t) ui);
      cfg.total_cnt++;
    }
    else if(!found)
    {
      xprintf(0, gettext("Unable to parse \"%s\"\n"), lns);
      rv = 1;
    }
  }

  return rv;
}

int cfg_load(const char *filename)
{
  int rv;
  bufio *fd;

  if(!(fd = bufio_open(filename, O_BINARY | O_RDONLY)))
  {
    xperror(filename);
    return -1;
  }

  rv = cfg_load_fd(fd);

  bufio_close(fd);

  return rv;
}

static int cfg_load_scenario(const char *filename)
{
  char *fn;
  int rv;

  _free(cfg.scenario);

  if(strchr(filename, '/') || !cfg.scndir)
    fn = tl_strdup(filename);
  else
    fn = tl_str_concat(tl_strdup(cfg.scndir), "/", filename, NULL);

  if((rv = cfg_load(fn)))
  {
    xprintf(0, gettext("ERROR: Scenario loading failed (%s)\n"), fn);
    exit(PAVUK_EXIT_CFG_ERR);
  }

  cfg.scenario = fn;

  return rv;
}

void cfg_load_setup(void)
{
  char pom[PATH_MAX];
  char *p;

#ifdef DEFAULTRC
  if(!access(DEFAULTRC, R_OK))
    cfg_load(DEFAULTRC);
#endif
  p = getenv("PAVUKRC_FILE");
  if(!p)
  {
    snprintf(pom, sizeof(pom), "%s/%s", cfg.path_to_home, ".pavukrc");
    p = pom;
  }
  if(!access(p, R_OK))
    cfg_load(p);
}

static int cfg_dump_fd(int fd)
{
  int i, j;
  char pom[8192];
  char pom2[20];
  char **pl;

  if(cfg.request)
  {
    dllist *dptr;

    for(dptr = cfg.request; dptr; dptr = dptr->next)
    {
      url_info *ui = (url_info *) dptr->data;

      if(ui->type == URLI_NORMAL && !ui->localname)
      {
        write(fd, "URL: ", 5);
        write(fd, ui->urlstr, strlen(ui->urlstr));
        write(fd, "\n", 1);
      }
    }
  }

  for(i = 0; i < NUM_ELEM(params); i++)
  {
    if(params[i].type & PARAM_UNSUPPORTED)
      continue;
    if(params[i].type & PARAM_FOREIGN)
      continue;
    if(!params[i].par_entry)
      continue;

    switch (params[i].type)
    {
    case PARAM_NUM:
      snprintf(pom, sizeof(pom), "%s %ld\n", params[i].par_entry,
        *((long *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_NBOOL:
    case PARAM_PBOOL:
      if(*((bool_t *) params[i].val_adr))
        snprintf(pom, sizeof(pom), "%s true\n", params[i].par_entry);
      else
        snprintf(pom, sizeof(pom), "%s false\n", params[i].par_entry);
      write(fd, pom, strlen(pom));
      break;
    case PARAM_PORT_RANGE:
      if(*((long *) params[i].val_adr) >= 0)
      {
        snprintf(pom, sizeof(pom), "%s %ld:%ld\n", params[i].par_entry,
          *((long *) params[i].val_adr), *((long *) params[i].mval_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_PATH:
    case PARAM_STR:
    case PARAM_PASS:
      if(*((char **) params[i].val_adr))
      {
        snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry,
          *((char **) params[i].val_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_STRLIST:
      if(!params[i].mval_adr || (params[i].mval_adr &&
          (*((bool_t *) params[i].mval_adr) ==
            (bool_t) (long) params[i].mdefault_val)))
      {
        pl = *((char ***) params[i].val_adr);
        if(pl && pl[0])
        {
          snprintf(pom, sizeof(pom), "%s %s", params[i].par_entry, pl[0]);
          write(fd, pom, strlen(pom));

          j = 1;
          while(pl[j])
          {
            write(fd, ",", 1);
            write(fd, pl[j], strlen(pl[j]));
            j++;
          }
          write(fd, "\n", 1);
        }
      }
      break;
    case PARAM_CONN:
      if(*((char **) params[i].val_adr))
      {
        snprintf(pom, sizeof(pom), "%s %s:%d\n", params[i].par_entry,
          *((char **) params[i].val_adr), *((int *) params[i].mval_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_AUTHSCH:
      snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry,
        http_auths[*((long *) params[i].val_adr)].name);
      write(fd, pom, strlen(pom));
      break;
    case PARAM_MODE:
      snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry,
        mode_get_str(cfg.mode));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_TIME:
      if(*((time_t *) params[i].val_adr))
      {
        LOCK_TIME;
        strftime(pom2, sizeof(pom2), "%Y.%m.%d.%H:%M",
          localtime((time_t *) params[i].val_adr));
        UNLOCK_TIME;
        snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry, pom2);
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_HTMLTAG:
      {
        int x, y;
        bool_t first = TRUE;

        snprintf(pom, sizeof(pom), "%s ", params[i].par_entry);
        for(x = 0; x < html_link_tags_num(); x++)
        {
          for(y = 0; html_link_tags[x].attribs[y].attrib; y++)
          {
            if(!(html_link_tags[x].attribs[y].stat & LINK_DISABLED) ==
              !(params[i].default_val))
            {
              if(!first)
              {
                strncat(pom, ";", sizeof(pom) - strlen(pom));
                pom[sizeof(pom) - 1] = '\0';
              }
              strncat(pom, html_link_tags[x].tag, sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, ",", sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, html_link_tags[x].attribs[y].attrib,
                sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              first = FALSE;
            }
          }
        }
        strncat(pom, "\n", sizeof(pom) - strlen(pom));
        pom[sizeof(pom) - 1] = '\0';
        if(!first)
          write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_TWO_QSTR:
      if(*((char **) params[i].val_adr))
      {
        char *p1, *p2;
        p1 = escape_str(*((char **) params[i].val_adr), "\\\"");
        p2 = escape_str(*((char **) params[i].mval_adr), "\\\"");
        snprintf(pom, sizeof(pom), "%s \"%s\" \"%s\"\n", params[i].par_entry,
          p1, p2);
        _free(p1);
        _free(p2);
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_DOUBLE:
      snprintf(pom, sizeof(pom), "%s %.3f\n", params[i].par_entry,
        *((double *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_LFNAME:
      {
        dllist *pdl = cfg.lfnames;
        while(pdl)
        {
          lfname *lfnm = (lfname *) pdl->data;
          char *p1, *p2;

          p1 = escape_str(lfnm->matchstr, "\\\"");
          p2 = escape_str(lfnm->transstr, "\\\"");

          snprintf(pom, sizeof(pom), "%s \"%s\" \"%s\" \"%s\"\n",
            params[i].par_entry, (lfnm->type == LFNAME_FNMATCH) ? "F" : "R",
            p1, p2);
          _free(p1);
          _free(p2);
          write(fd, pom, strlen(pom));

          pdl = pdl->next;
        }
      }
      break;
    case PARAM_RE:
#ifdef HAVE_REGEX
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          re_entry *ree = (re_entry *) ptr->data;

          snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry,
            ree->pattern);
          write(fd, pom, strlen(pom));

          ptr = ptr->next;
        }
      }
#endif
      break;
    case PARAM_USTRAT:
      snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry,
        get_strategie_str(*(strategie *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_SSLVER:
      snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry,
        get_ssl_version_str(*(int *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_HTTPHDR:
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          httphdr *hr = (httphdr *) ptr->data;

          snprintf(pom, sizeof(pom), "%s %s%s %s\n", params[i].par_entry,
            hr->all ? "+" : "", hr->name, hr->val);
          write(fd, pom, strlen(pom));

          ptr = ptr->next;
        }
      }
      break;
    case PARAM_DEBUGL:
#ifdef DEBUG
      {
        char strbuf[1024];
        debug_level_construct(*((int *) params[i].val_adr), strbuf);
        snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry, strbuf);
        write(fd, pom, strlen(pom));
      }
#endif
      break;
    case PARAM_REQUEST:
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          url_info *ui = (url_info *) ptr->data;

          if(ui->type != URLI_NORMAL || ui->localname)
          {
            char *p = url_info_dump(ui);
            snprintf(pom, sizeof(pom), "%s %s\n", params[i].par_entry, p);
            _free(p);
            write(fd, pom, strlen(pom));
          }

          ptr = ptr->next;
        }
      }
      break;
    case PARAM_TRANSPARENT:
      {
        http_proxy *pr = *((http_proxy **) params[i].val_adr);
        if(pr)
        {
          snprintf(pom, sizeof(pom), "%s %s:%d\n", params[i].par_entry,
            pr->addr, pr->port);
          write(fd, pom, strlen(pom));
        }
      }
      break;
    case PARAM_PROXY:
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          http_proxy *pr = (http_proxy *) ptr->data;
          snprintf(pom, sizeof(pom), "%s %s:%hu\n", params[i].par_entry,
            pr->addr, pr->port);
          write(fd, pom, strlen(pom));

          ptr = ptr->next;
        }
      }
      break;
    case PARAM_FUNC:
      break;
    case PARAM_JSTRANS:
#ifdef HAVE_REGEX
      {
        dllist *ptr;
        for(ptr = cfg.js_transform; ptr; ptr = ptr->next)
        {
          js_transform_t *jt = (js_transform_t *) ptr->data;
          char *p[4];

          if(jt->type != (long) params[i].mdefault_val)
            continue;

          p[0] = escape_str(jt->re->pattern, "\\\"");
          p[1] = escape_str(jt->transform, "\\\"");
          p[2] = escape_str(jt->tag, "\\\"");
          p[3] = escape_str(jt->attrib, "\\\"");

          snprintf(pom, sizeof(pom), "%s \"%s\" \"%s\" \"%s\" \"%s\"\n",
            params[i].par_entry, p[0], p[1], p[2], p[3]);
          _free(p[0]);
          _free(p[1]);
          _free(p[2]);
          _free(p[3]);
          write(fd, pom, strlen(pom));
        }
      }
#endif
      break;
    case PARAM_NUMLIST:
      if(!params[i].mval_adr || (params[i].mval_adr &&
          (*((bool_t *) params[i].mval_adr) ==
            (bool_t) (long) params[i].mdefault_val)))
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        if(ptr)
        {
          snprintf(pom, sizeof(pom), "%s %ld", params[i].par_entry,
            (long) ptr->data);
          write(fd, pom, strlen(pom));

          j = 1;
          for(; ptr; ptr = ptr->next)
          {
            snprintf(pom, sizeof(pom), ",%ld", (long) ptr->data);
            write(fd, pom, strlen(pom));
          }
          write(fd, "\n", 1);
        }
      }
      break;
    case PARAM_FTPHS:
      {
        dllist *ptr;
        for(ptr = *((dllist **) params[i].val_adr); ptr; ptr = ptr->next)
        {
          char *p, *p2;
          ftp_handshake_info *fhi = (ftp_handshake_info *)ptr->data;

          p = ftp_handshake_info_data_dump(fhi);
          p2 = escape_str(p, "\\\"");
          _free(p);
          p = p2;
          if(*fhi->host)
            snprintf(pom, sizeof(pom),
              "%s \"%s:%hu\" \"%s\"\n",
              params[i].par_entry, fhi->host, fhi->port, p);
          else
            snprintf(pom, sizeof(pom),
              "%s \"\" \"%s\"\n", params[i].par_entry, p);

          _free(p);
          write(fd, pom, strlen(pom));
        }
      }
      break;
    case PARAM_TAGPAT:
      {
        dllist *ptr;
        for(ptr = *((dllist **) params[i].val_adr); ptr; ptr = ptr->next)
        {
          char *t, *a, *u;
          tag_pattern_t *tp = (tag_pattern_t *) ptr->data;

          t = escape_str(tp->tag, "\\\"");
          a = escape_str(tp->attrib, "\\\"");
          u = escape_str(tp->urlp, "\\\"");

          snprintf(pom, sizeof(pom),
            "%s \"%s\" \"%s\" \"%s\"\n", params[i].par_entry, t, a, u);

          _free(t);
          _free(a);
          _free(u);

          write(fd, pom, strlen(pom));
        }
      }
      break;
    }
  }
  return 0;
}

int cfg_dump(const char *filename)
{
  int fd;

  if((fd = open(filename, O_BINARY | O_CREAT | O_WRONLY, 0666)) < 0)
  {
    xperror(filename);
    return -1;
  }

  ftruncate(fd, 0);

  cfg_dump_fd(fd);

  close(fd);
  return 0;
}

int cfg_load_pref(void)
{
  bufio *fd;
  char filename[PATH_MAX];

  snprintf(filename, sizeof(filename), "%s/.pavuk_prefs", cfg.path_to_home);

  if(!(fd = bufio_open(filename, O_BINARY | O_RDONLY)))
  {
    return -1;
  }

  cfg_set_all_to_default();

  cfg_load_fd(fd);

  bufio_close(fd);

  return 0;
}

int cfg_dump_pref(void)
{
  int fd;
  char filename[PATH_MAX];

  snprintf(filename, sizeof(filename), "%s/.pavuk_prefs", cfg.path_to_home);

  if((fd = open(filename, O_BINARY | O_CREAT | O_WRONLY, 0666)) < 0)
  {
    xperror(filename);
    return -1;
  }

  ftruncate(fd, 0);

  cfg_dump_fd(fd);

  close(fd);
  return 0;
}

int cfg_dump_cmd(const char *filename)
{
  int fd;
  int rv;

  if((fd = open(filename, O_BINARY | O_CREAT | O_WRONLY, 0666)) < 0)
  {
    xperror(filename);
    return -1;
  }

  rv = cfg_dump_cmd_fd(fd);

  close(fd);

  return rv;
}

int cfg_dump_cmd_fd(int fd)
{
  int i, j;
  char pom[8192];
  char pom2[20];
  char **pl;

  ftruncate(fd, 0);

  write(fd, cfg.prg_path, strlen(cfg.prg_path));

  write(fd, " ", 1);

  if(cfg.request)
  {
    dllist *dptr;

    for(dptr = cfg.request; dptr; dptr = dptr->next)
    {
      url_info *ui = (url_info *) dptr->data;

      if(ui->type == URLI_NORMAL && !ui->localname)
      {
        write(fd, " '", 2);
        write(fd, ui->urlstr, strlen(ui->urlstr));
        write(fd, "' ", 2);
      }
    }
  }

  for(i = 0; i < NUM_ELEM(params); i++)
  {
    if(params[i].type & PARAM_UNSUPPORTED)
      continue;
    if(params[i].type & PARAM_FOREIGN)
      continue;
    if(!params[i].long_cmd)
      continue;

    if(!params[i].par_entry &&
      (params[i].type != PARAM_PBOOL) && (params[i].type != PARAM_NBOOL))
      continue;

    switch (params[i].type)
    {
    case PARAM_NUM:
      snprintf(pom, sizeof(pom), " -%s=%ld ", params[i].long_cmd,
        *((long *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_NBOOL:
      if(!*((bool_t *) params[i].val_adr))
      {
        write(fd, " -", 2);
        write(fd, params[i].long_cmd, strlen(params[i].long_cmd));
        write(fd, " ", 1);
      }
      break;
    case PARAM_PBOOL:
      if(*((bool_t *) params[i].val_adr))
      {
        write(fd, " -", 2);
        write(fd, params[i].long_cmd, strlen(params[i].long_cmd));
        write(fd, " ", 1);
      }
      break;
    case PARAM_PORT_RANGE:
      if(*((long *) params[i].val_adr) >= 0)
      {
        snprintf(pom, sizeof(pom), " -%s=%ld:%ld ", params[i].long_cmd,
          *((long *) params[i].val_adr), *((long *) params[i].mval_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_PATH:
    case PARAM_STR:
    case PARAM_PASS:
      if(*((char **) params[i].val_adr))
      {
        snprintf(pom, sizeof(pom), " -%s '%s' ", params[i].long_cmd,
          *((char **) params[i].val_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_STRLIST:
      if(!params[i].mval_adr || (params[i].mval_adr &&
          (*((bool_t *) params[i].mval_adr) ==
            (bool_t) (long) params[i].mdefault_val)))
      {
        pl = *((char ***) params[i].val_adr);
        if(pl && pl[0])
        {
          snprintf(pom, sizeof(pom), " -%s '%s", params[i].long_cmd, pl[0]);
          write(fd, pom, strlen(pom));

          j = 1;
          while(pl[j])
          {
            write(fd, ",", 1);
            write(fd, pl[j], strlen(pl[j]));
            j++;
          }
          write(fd, "' ", 2);
        }
      }
      break;
    case PARAM_CONN:
      if(*((char **) params[i].val_adr))
      {
        snprintf(pom, sizeof(pom), "-%s %s:%hu ", params[i].long_cmd,
          *((char **) params[i].val_adr), *((int *) params[i].mval_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_AUTHSCH:
      snprintf(pom, sizeof(pom), " -%s %s ", params[i].long_cmd,
        http_auths[*((long *) params[i].val_adr)].name);
      write(fd, pom, strlen(pom));
      break;
    case PARAM_MODE:
      snprintf(pom, sizeof(pom), " -%s %s ", params[i].long_cmd,
        mode_get_str(cfg.mode));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_TIME:
      if(*((time_t *) params[i].val_adr))
      {
        LOCK_TIME;
        strftime(pom2, sizeof(pom2), "%Y.%m.%d.%H:%M",
          localtime((time_t *) params[i].val_adr));
        UNLOCK_TIME;
        snprintf(pom, sizeof(pom), " -%s %s ", params[i].long_cmd, pom2);
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_HTMLTAG:
      {
        int x, y;
        bool_t first = TRUE;

        snprintf(pom, sizeof(pom), " -%s '", params[i].long_cmd);
        for(x = 0; x < html_link_tags_num(); x++)
        {
          for(y = 0; html_link_tags[x].attribs[y].attrib; y++)
          {
            if(!(html_link_tags[x].attribs[y].stat & LINK_DISABLED) ==
              !(params[i].default_val))
            {
              if(!first)
              {
                strncat(pom, ";", sizeof(pom) - strlen(pom));
                pom[sizeof(pom) - 1] = '\0';
              }
              strncat(pom, html_link_tags[x].tag, sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, ",", sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              strncat(pom, html_link_tags[x].attribs[y].attrib,
                sizeof(pom) - strlen(pom));
              pom[sizeof(pom) - 1] = '\0';
              first = FALSE;
            }
          }
        }
        strncat(pom, "' ", sizeof(pom) - strlen(pom));
        pom[sizeof(pom) - 1] = '\0';
        if(!first)
          write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_TWO_QSTR:
      if(*((char **) params[i].val_adr))
      {
        snprintf(pom, sizeof(pom), " -%s '%s' '%s' ", params[i].long_cmd,
          *((char **) params[i].val_adr), *((char **) params[i].mval_adr));
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_DOUBLE:
      snprintf(pom, sizeof(pom), " -%s=%.3f ", params[i].long_cmd,
        *((double *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_LFNAME:
      {
        dllist *pdl = cfg.lfnames;
        while(pdl)
        {
          lfname *lfnm = (lfname *)pdl->data;

          snprintf(pom, sizeof(pom), " -%s \'%s\' \'%s\' \'%s\' ",
            params[i].long_cmd, (lfnm->type == LFNAME_FNMATCH) ? "F" : "R",
            lfnm->matchstr, lfnm->transstr);
          write(fd, pom, strlen(pom));

          pdl = pdl->next;
        }
      }
      break;
    case PARAM_RE:
#ifdef HAVE_REGEX
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          re_entry *ree = (re_entry *)ptr->data;

          snprintf(pom, sizeof(pom), " -%s \'%s\' ", params[i].long_cmd,
            ree->pattern);
          write(fd, pom, strlen(pom));

          ptr = ptr->next;
        }
      }
#endif
      break;
    case PARAM_USTRAT:
      snprintf(pom, sizeof(pom), " -%s=%s ", params[i].long_cmd,
        get_strategie_str(*(strategie *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_SSLVER:
      snprintf(pom, sizeof(pom), " -%s=%s ", params[i].long_cmd,
        get_ssl_version_str(*(int *) params[i].val_adr));
      write(fd, pom, strlen(pom));
      break;
    case PARAM_HTTPHDR:
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          httphdr *hr = (httphdr *) ptr->data;

          snprintf(pom, sizeof(pom), " -%s \"%s%s %s\" ", params[i].long_cmd,
            hr->all ? "+" : "", hr->name, hr->val);
          write(fd, pom, strlen(pom));

          ptr = ptr->next;
        }
      }
      break;
    case PARAM_DEBUGL:
#ifdef DEBUG
      {
        char strbuf[1024];
        debug_level_construct(*((int *) params[i].val_adr), strbuf);
        snprintf(pom, sizeof(pom), " -%s \'%s\' ", params[i].long_cmd,
          strbuf);
        write(fd, pom, strlen(pom));
      }
#endif
      break;
    case PARAM_REQUEST:
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          url_info *ui = (url_info *) ptr->data;

          if(ui->type != URLI_NORMAL || ui->localname)
          {
            char *p = url_info_dump(ui);
            snprintf(pom, sizeof(pom), " -%s \'%s\' ", params[i].long_cmd, p);
            _free(p);
            write(fd, pom, strlen(pom));
          }

          ptr = ptr->next;
        }
      }
      break;
    case PARAM_TRANSPARENT:
      {
        http_proxy *pr = *((http_proxy **) params[i].val_adr);
        snprintf(pom, sizeof(pom), " -%s=%s:%d ", params[i].long_cmd,
          pr->addr, pr->port);
        write(fd, pom, strlen(pom));
      }
      break;
    case PARAM_PROXY:
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        while(ptr)
        {
          http_proxy *pr = (http_proxy *) ptr->data;
          snprintf(pom, sizeof(pom), " -%s=%s:%hu ", params[i].long_cmd,
            pr->addr, pr->port);
          write(fd, pom, strlen(pom));

          ptr = ptr->next;
        }
      }
      break;
    case PARAM_FUNC:
      break;
    case PARAM_JSTRANS:
#ifdef HAVE_REGEX
      {
        dllist *ptr;
        for(ptr = cfg.js_transform; ptr; ptr = ptr->next)
        {
          js_transform_t *jt = (js_transform_t *) ptr->data;
          if(jt->type != (long) params[i].mdefault_val)
            continue;

          snprintf(pom, sizeof(pom), " -%s \'%s\' \'%s\' \'%s\' \'%s\' ",
            params[i].long_cmd, jt->re->pattern, jt->transform, jt->tag,
            jt->attrib);
          write(fd, pom, strlen(pom));
        }
      }
#endif
      break;
    case PARAM_NUMLIST:
      if(!params[i].mval_adr || (params[i].mval_adr &&
          (*((bool_t *) params[i].mval_adr) ==
            (bool_t) (long) params[i].mdefault_val)))
      {
        dllist *ptr = *((dllist **) params[i].val_adr);
        if(ptr)
        {
          snprintf(pom, sizeof(pom), "-%s %ld", params[i].long_cmd,
            (long) ptr->data);
          write(fd, pom, strlen(pom));

          j = 1;
          for(; ptr; ptr = ptr->next)
          {
            snprintf(pom, sizeof(pom), ",%ld", (long) ptr->data);
            write(fd, pom, strlen(pom));
          }
          write(fd, " ", 1);
        }
      }
      break;
    case PARAM_FTPHS:
      {
        dllist *ptr;
        for(ptr = *((dllist **) params[i].val_adr); ptr; ptr = ptr->next)
        {
          char *p;
          ftp_handshake_info *fhi = (ftp_handshake_info *) ptr->data;

          p = ftp_handshake_info_data_dump(fhi);

          if(*fhi->host)
            snprintf(pom, sizeof(pom),
              "-%s \"%s:%hu\" \"%s\" ",
              params[i].long_cmd, fhi->host, fhi->port, p);
          else
            snprintf(pom, sizeof(pom),
              "-%s \"\" \"%s\" ", params[i].long_cmd, p);
          _free(p);
          write(fd, pom, strlen(pom));
        }
      }
      break;
    case PARAM_TAGPAT:
      {
        dllist *ptr;
        for(ptr = *((dllist **) params[i].val_adr); ptr; ptr = ptr->next)
        {
          tag_pattern_t *tp = (tag_pattern_t *) ptr->data;

          snprintf(pom, sizeof(pom),
            "-%s \"%s\" \"%s\" \"%s\" ",
            params[i].long_cmd, tp->tag, tp->attrib, tp->urlp);

          write(fd, pom, strlen(pom));
        }
      }
      break;
    }
  }

  return 0;
}

void cfg_free_params(void)
{
  int i;

  for(i = 0; i < NUM_ELEM(params); i++)
    cfg_set_to_default(&(params[i]));
}

#if defined(HAVE_MT) && defined(I_FACE)

static char **_copy_strnar(char **orig)
{
  int n, i;
  char **rv;

  if(!orig)
    return NULL;

  for(n = 0; orig[n]; n++);
  n++;
  rv = (char **) _malloc(n * sizeof(char **));

  for(i = 0; i < n; i++)
    rv[i] = tl_strdup(orig[i]);

  return rv;
}

#ifdef HAVE_REGEX
static dllist *_copy_relist(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    rv = dllist_append(rv, re_make(((re_entry *) ptr->data)->pattern));
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_jstrans(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    js_transform_t *jt = ptr->data;

    rv = dllist_append(rv, js_transform_new(jt->re->pattern,
        jt->transform, jt->tag, jt->attrib, jt->type));
    ptr = ptr->next;
  }

  return rv;
}
#endif

static dllist *_copy_lfnames(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    lfname *ln = (lfname *) ptr->data;
    rv = dllist_append(rv, lfname_new(ln->type, ln->matchstr, ln->transstr));
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_httphdr(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    httphdr *ov, *nv;

    ov = (httphdr *) ptr->data;
    nv = _malloc(sizeof(httphdr));

    nv->all = ov->all;
    nv->name = tl_strdup(ov->name);
    nv->val = tl_strdup(ov->val);

    rv = dllist_append(rv, nv);
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_urlinfo(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return NULL;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    url_info *ui;

    ui = url_info_duplicate((url_info *) ptr->data);

    rv = dllist_append(rv, ui);
    ptr = ptr->next;
  }

  return rv;
}

static dllist *_copy_numlist(dllist * orig)
{
  dllist *rv = NULL, *ptr;

  for(ptr = orig; ptr; ptr = ptr->next)
    rv = dllist_append(rv, ptr->data);

  return rv;
}

static dllist *_copy_ftphs(dllist * orig)
{
  dllist *rv = NULL;

  for(; orig; orig = orig->next)
    rv = dllist_append(rv, ftp_handshake_info_dup(orig->data));

  return rv;
}

static dllist *_copy_tagpat(dllist * orig)
{
  dllist *rv = NULL;

  for(; orig; orig = orig->next)
  {
    tag_pattern_t *tp = orig->data;

    rv = dllist_append(rv, tag_pattern_new(tp->type, tp->tag,
        tp->attrib, tp->urlp));
  }

  return rv;
}

void privcfg_make_copy(_config_struct_priv_t * pcfg)
{
  LOCK_GCFG;

  memset(pcfg, '\0', sizeof(_config_struct_priv_t));

  pcfg->timestamp = time(NULL);

  pcfg->default_prefix = tl_strdup(cfg.default_prefix);
  pcfg->info_dir = tl_strdup(cfg.info_dir);
  pcfg->subdir = tl_strdup(cfg.subdir);
  pcfg->cache_dir = tl_strdup(cfg.cache_dir);
  pcfg->post_cmd = tl_strdup(cfg.post_cmd);
  pcfg->http_proxy_pass = tl_strdup(cfg.http_proxy_pass);
  pcfg->http_proxy_user = tl_strdup(cfg.http_proxy_user);
  pcfg->ftp_proxy_pass = tl_strdup(cfg.ftp_proxy_pass);
  pcfg->ftp_proxy_user = tl_strdup(cfg.ftp_proxy_user);
  pcfg->ftp_proxy = tl_strdup(cfg.ftp_proxy);
  pcfg->ftp_proxy_port = cfg.ftp_proxy_port;
  pcfg->gopher_proxy = tl_strdup(cfg.gopher_proxy);
  pcfg->gopher_proxy_port = cfg.gopher_proxy_port;
  pcfg->name_auth = tl_strdup(cfg.name_auth);
  pcfg->passwd_auth = tl_strdup(cfg.passwd_auth);
  pcfg->index_name = tl_strdup(cfg.index_name);
  pcfg->store_name = tl_strdup(cfg.store_name);
  pcfg->from = tl_strdup(cfg.from);
  pcfg->identity = tl_strdup(cfg.identity);
  pcfg->auth_ntlm_domain = tl_strdup(cfg.auth_ntlm_domain);
  pcfg->ftp_list_options = tl_strdup(cfg.ftp_list_options);

  pcfg->accept_lang = _copy_strnar(cfg.accept_lang);
  pcfg->accept_chars = _copy_strnar(cfg.accept_chars);
  pcfg->cookies_disabled_domains = _copy_strnar(cfg.cookies_disabled_domains);
  pcfg->dont_touch_url_pattern = _copy_strnar(cfg.dont_touch_url_pattern);

  pcfg->lfnames = _copy_lfnames(cfg.lfnames);
  pcfg->http_headers = _copy_httphdr(cfg.http_headers);
  pcfg->formdata = _copy_urlinfo(cfg.formdata);
  pcfg->ftp_login_hs = _copy_ftphs(cfg.ftp_login_hs);
  pcfg->condition.tag_patterns = _copy_tagpat(cfg.condition.tag_patterns);

  pcfg->condition.ports = _copy_numlist(cfg.condition.ports);

  pcfg->condition.sites = _copy_strnar(cfg.condition.sites);
  pcfg->condition.allow_site = cfg.condition.allow_site;
  pcfg->condition.sufix = _copy_strnar(cfg.condition.sufix);
  pcfg->condition.allow_sufix = cfg.condition.allow_sufix;
  pcfg->condition.dir_prefix = _copy_strnar(cfg.condition.dir_prefix);
  pcfg->condition.allow_prefix = cfg.condition.allow_prefix;
  pcfg->condition.domains = _copy_strnar(cfg.condition.domains);
  pcfg->condition.allow_domain = cfg.condition.allow_domain;
  pcfg->condition.mime = _copy_strnar(cfg.condition.mime);
  pcfg->condition.allow_mime = cfg.condition.allow_mime;

  pcfg->condition.pattern = _copy_strnar(cfg.condition.pattern);
  pcfg->condition.url_pattern = _copy_strnar(cfg.condition.url_pattern);
  pcfg->condition.skip_pattern = _copy_strnar(cfg.condition.skip_pattern);
  pcfg->condition.skip_url_pattern =
    _copy_strnar(cfg.condition.skip_url_pattern);

  pcfg->condition.uexit = tl_strdup(cfg.condition.uexit);
  pcfg->condition.follow_cmd = tl_strdup(cfg.condition.follow_cmd);

  pcfg->tr_del_chr = tl_strdup(cfg.tr_del_chr);
  pcfg->tr_str_s1 = tl_strdup(cfg.tr_str_s1);
  pcfg->tr_str_s2 = tl_strdup(cfg.tr_str_s2);
  pcfg->tr_chr_s1 = tl_strdup(cfg.tr_chr_s1);
  pcfg->tr_chr_s2 = tl_strdup(cfg.tr_chr_s2);

#ifdef HAVE_BDB_18x
  pcfg->ns_cache_dir = tl_strdup(cfg.ns_cache_dir);
  pcfg->moz_cache_dir = tl_strdup(cfg.moz_cache_dir);
#endif


#ifdef HAVE_REGEX
  pcfg->advert_res = _copy_relist(cfg.advert_res);
  pcfg->js_patterns = _copy_relist(cfg.js_patterns);
  pcfg->dont_touch_url_rpattern = _copy_relist(cfg.dont_touch_url_rpattern);
  pcfg->dont_touch_tag_rpattern = _copy_relist(cfg.dont_touch_tag_rpattern);

  pcfg->js_transform = _copy_jstrans(cfg.js_transform);

  pcfg->condition.rpattern = _copy_relist(cfg.condition.rpattern);
  pcfg->condition.rskip_pattern = _copy_relist(cfg.condition.rskip_pattern);
  pcfg->condition.rurl_pattern = _copy_relist(cfg.condition.rurl_pattern);
  pcfg->condition.rskip_url_pattern =
    _copy_relist(cfg.condition.rskip_url_pattern);

  pcfg->condition.aip = _copy_relist(cfg.condition.aip);
  pcfg->condition.skipip = _copy_relist(cfg.condition.skipip);
#endif

#ifdef USE_SSL
  pcfg->ssl_proxy = tl_strdup(cfg.ssl_proxy);
  pcfg->ssl_proxy_port = cfg.ssl_proxy_port;
  pcfg->ssl_cipher_list = tl_strdup(cfg.ssl_cipher_list);
  pcfg->ssl_cert_passwd = tl_strdup(cfg.ssl_cert_passwd);

#ifdef USE_SSL_IMPL_OPENSSL
  pcfg->ssl_cert_file = tl_strdup(cfg.ssl_cert_file);
  pcfg->ssl_key_file = tl_strdup(cfg.ssl_key_file);
  pcfg->egd_socket = tl_strdup(cfg.egd_socket);
#endif
#ifdef USE_SSL_IMPL_NSS
  pcfg->nss_cert_dir = tl_strdup(cfg.nss_cert_dir);
#endif
#endif
  UNLOCK_GCFG;

}

#define __free_strnar(orig) _free_strnar(orig);orig = NULL;

static void _free_strnar(char **orig)
{
  int n;

  if(!orig)
    return;

  for(n = 0; orig[n]; n++)
    _free(orig[n]);
  _free(orig);
}

#ifdef HAVE_REGEX
#define __free_relist(orig) _free_relist(orig);orig = NULL;

static void _free_relist(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    re_free((re_entry *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_jstrans(orig) _free_jstrans(orig);orig = NULL;

static void _free_jstrans(dllist * orig)
{
  dllist *rv, *ptr;

  if(!orig)
    return;

  rv = NULL;
  ptr = orig;

  while(ptr)
  {
    js_transform_free((js_transform_t *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#endif

#define __free_lfnames(orig) _free_lfnames(orig);orig = NULL;

static void _free_lfnames(dllist * orig)
{
  dllist *ptr;

  if(!orig)
    return;

  ptr = orig;

  while(ptr)
  {
    lfname_free((lfname *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_httphdr(orig) _free_httphdr(orig);orig = NULL;

static void _free_httphdr(dllist * orig)
{
  dllist *ptr;

  if(!orig)
    return;

  ptr = orig;

  while(ptr)
  {
    httphdr *ov = (httphdr *) ptr->data;

    _free(ov->name);
    _free(ov->val);

    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_urlinfo(orig) _free_urlinfo(orig);orig = NULL;

static void _free_urlinfo(dllist * orig)
{
  dllist *ptr;

  if(!orig)
    return;

  ptr = orig;

  while(ptr)
  {
    url_info_free((url_info *) ptr->data);
    ptr = dllist_remove_entry(ptr, ptr);
  }
}

#define __free_numlist(orig) _free_numlist(orig);orig = NULL;

static void _free_numlist(dllist * orig)
{
  while(orig)
    orig = dllist_remove_entry(orig, orig);
}

#define __free_ftphs(orig) _free_ftphs(orig);orig = NULL;

static void _free_ftphs(dllist * orig)
{
  for(; orig; orig = dllist_remove_entry(orig, orig))
    ftp_handshake_info_free(orig->data);
}

#define __free_tagpat(orig) _free_tagpat(orig);orig = NULL;

static void _free_tagpat(dllist * orig)
{
  for(; orig; orig = dllist_remove_entry(orig, orig))
    tag_pattern_free(orig->data);
}

void privcfg_free(_config_struct_priv_t * pcfg)
{
  _free(pcfg->default_prefix);
  _free(pcfg->info_dir);
  _free(pcfg->subdir);
  _free(pcfg->cache_dir);
  _free(pcfg->post_cmd);
  _free(pcfg->http_proxy_pass);
  _free(pcfg->http_proxy_user);
  _free(pcfg->ftp_proxy_pass);
  _free(pcfg->ftp_proxy_user);
  _free(pcfg->ftp_proxy);
  _free(pcfg->gopher_proxy);
  _free(pcfg->name_auth);
  _free(pcfg->passwd_auth);
  _free(pcfg->index_name);
  _free(pcfg->store_name);
  _free(pcfg->from);
  _free(pcfg->identity);
  _free(pcfg->auth_ntlm_domain);
  _free(pcfg->auth_proxy_ntlm_domain);
  _free(pcfg->ftp_list_options);

  __free_strnar(pcfg->accept_lang);
  __free_strnar(pcfg->accept_chars);
  __free_strnar(pcfg->cookies_disabled_domains);
  __free_strnar(pcfg->dont_touch_url_pattern);

  __free_lfnames(pcfg->lfnames);
  __free_httphdr(pcfg->http_headers);
  __free_urlinfo(pcfg->formdata);
  __free_ftphs(pcfg->ftp_login_hs);
  __free_tagpat(pcfg->condition.tag_patterns);

  __free_numlist(pcfg->condition.ports);

  __free_strnar(pcfg->condition.sites);
  __free_strnar(pcfg->condition.sufix);
  __free_strnar(pcfg->condition.dir_prefix);
  __free_strnar(pcfg->condition.domains);
  __free_strnar(pcfg->condition.mime);

  __free_strnar(pcfg->condition.pattern);
  __free_strnar(pcfg->condition.url_pattern);
  __free_strnar(pcfg->condition.skip_pattern);
  __free_strnar(pcfg->condition.skip_url_pattern);

  _free(pcfg->condition.uexit);
  _free(pcfg->condition.follow_cmd);

  _free(pcfg->tr_del_chr);
  _free(pcfg->tr_str_s1);
  _free(pcfg->tr_str_s2);
  _free(pcfg->tr_chr_s1);
  _free(pcfg->tr_chr_s2);

#ifdef HAVE_BDB_18x
  _free(pcfg->ns_cache_dir);
  _free(pcfg->moz_cache_dir);
#endif

#ifdef HAVE_REGEX
  __free_relist(pcfg->advert_res);
  __free_relist(pcfg->dont_touch_url_rpattern);
  __free_relist(pcfg->dont_touch_tag_rpattern);
  __free_relist(pcfg->js_patterns);
  __free_jstrans(pcfg->js_transform);

  __free_relist(pcfg->condition.rpattern);
  __free_relist(pcfg->condition.rskip_pattern);
  __free_relist(pcfg->condition.rurl_pattern);
  __free_relist(pcfg->condition.rskip_url_pattern);

  __free_relist(pcfg->condition.aip);
  __free_relist(pcfg->condition.skipip);
#endif

#ifdef USE_SSL
  _free(pcfg->ssl_proxy);
  _free(pcfg->ssl_cipher_list);
  _free(pcfg->ssl_cert_passwd);

#ifdef USE_SSL_IMPL_OPENSSL
  _free(pcfg->egd_socket);
  _free(pcfg->ssl_cert_file);
  _free(pcfg->ssl_key_file);
#endif
#ifdef USE_SSL_IMPL_NSS
  _free(pcf->nss_cert_dir);
#endif
#endif
  memset(pcfg, '\0', sizeof(_config_struct_priv_t));
}

#endif /* HAVE_MT && I_FACE */
