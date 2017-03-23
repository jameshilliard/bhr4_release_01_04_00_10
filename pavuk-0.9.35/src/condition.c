/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "fnmatch.h"
#endif

#include "url.h"
#include "condition.h"
#include "tools.h"
#include "uexit.h"
#include "re.h"
#include "dns.h"
#include "debugl.h"
#include "jsbind.h"
#include "tag_pattern.h"
#include "dlhash_tools.h"

static int cond_unsupported(url *, cond_info_t *);
static int cond_lmax(url *, cond_info_t *);
static int cond_dmax(url *, cond_info_t *);
static int cond_noftp(url *, cond_info_t *);
static int cond_nhttp(url *, cond_info_t *);
static int cond_nossl(url *, cond_info_t *);
static int cond_nogopher(url *, cond_info_t *);
static int cond_noftps(url *, cond_info_t *);
static int cond_nocgi(url *, cond_info_t *);
static int cond_asite(url *, cond_info_t *);
static int cond_dsite(url *, cond_info_t *);
static int cond_adomain(url *, cond_info_t *);
static int cond_ddomain(url *, cond_info_t *);
static int cond_aprefix(url *, cond_info_t *);
static int cond_dprefix(url *, cond_info_t *);
static int cond_asfx(url *, cond_info_t *);
static int cond_dsfx(url *, cond_info_t *);
static int cond_pattern(url *, cond_info_t *);
static int cond_rpattern(url *, cond_info_t *);
static int cond_skip_pattern(url *, cond_info_t *);
static int cond_skip_rpattern(url *, cond_info_t *);
static int cond_url_pattern(url *, cond_info_t *);
static int cond_url_rpattern(url *, cond_info_t *);
static int cond_skip_url_pattern(url *, cond_info_t *);
static int cond_skip_url_rpattern(url *, cond_info_t *);
static int cond_dont_leave_site(url *, cond_info_t *);
static int cond_dont_leave_dir(url *, cond_info_t *);
static int cond_user_condition(url *, cond_info_t *);
static int cond_aip_pattern(url *, cond_info_t *);
static int cond_dip_pattern(url *, cond_info_t *);
static int cond_site_level(url *, cond_info_t *);
static int cond_dont_leave_site_enter_dir(url *, cond_info_t *);
static int cond_leave_level(url *, cond_info_t *);
static int cond_aport(url *, cond_info_t *);
static int cond_dport(url *, cond_info_t *);
static int cond_max_size(url *, cond_info_t *);
static int cond_min_size(url *, cond_info_t *);
static int cond_amime_type(url *, cond_info_t *);
static int cond_dmime_type(url *, cond_info_t *);
static int cond_newer_than(url *, cond_info_t *);
static int cond_older_than(url *, cond_info_t *);
static int cond_tag_pattern(url *, cond_info_t *);
static int cond_tag_rpattern(url *, cond_info_t *);

#define CL0 (1 << 0)
#define CL1 (1 << 1)
#define CL2 (1 << 2)
#define CL3 (1 << 3)
#define CLALL (CL0 | CL1 | CL2 | CL3)

struct cond_type_info_t
{
  cond_type_t type;
  char *name;
  int (*cond_func) (url *, cond_info_t *);
  bool_t standard;
  int level;
};

static const struct cond_type_info_t cond_type_info[] = {
  {CONDT_UNSUP, "unsupported", cond_unsupported, TRUE, CLALL},
  {CONDT_NOFTP, "-noftp", cond_noftp, TRUE, CL0 | CL2},
  {CONDT_NOHTTP, "-nohttp", cond_nhttp, TRUE, CL0 | CL2},
  {CONDT_NOSSL, "-nossl", cond_nossl, TRUE, CL0 | CL2},
  {CONDT_NOGOPHER, "-nogopher", cond_nogopher, TRUE, CL0 | CL2},
  {CONDT_NOFTPS, "-noftps", cond_noftps, TRUE, CL0 | CL2},
  {CONDT_NOCGI, "-nocgi", cond_nocgi, TRUE, CL0 | CL2},
  {CONDT_LMAX, "-lmax", cond_lmax, TRUE, CL0 | CL2},
  {CONDT_DMAX, "-dmax", cond_dmax, TRUE, CL1 | CL2},
  {CONDT_ASITE, "-asite", cond_asite, TRUE, CL0 | CL2},
  {CONDT_DSITE, "-dsite", cond_dsite, TRUE, CL0 | CL2},
  {CONDT_ADOMAIN, "-adomain", cond_adomain, TRUE, CL0 | CL2},
  {CONDT_DDOMAIN, "-ddomain", cond_ddomain, TRUE, CL0 | CL2},
  {CONDT_APREFIX, "-aprefix", cond_aprefix, TRUE, CL0 | CL2},
  {CONDT_DPREFIX, "-dprefix", cond_dprefix, TRUE, CL0 | CL2},
  {CONDT_ASFX, "-asfx", cond_asfx, TRUE, CL0 | CL2},
  {CONDT_DSFX, "-dsfx", cond_dsfx, TRUE, CL0 | CL2},
  {CONDT_DONT_LEAVE_SITE, "-dont_leave_site", cond_dont_leave_site, TRUE,
      CL0 | CL2},
  {CONDT_DONT_LEAVE_DIR, "-dont_leave_dir", cond_dont_leave_dir, TRUE,
      CL0 | CL2},
  {CONDT_SITE_LEVEL, "-site_level", cond_site_level, TRUE, CL0 | CL2},
  {CONDT_LEAVE_LEVEL, "-leave_level", cond_leave_level, TRUE, CL0 | CL2},
  {CONDT_DONT_LEAVE_SITE_ENTER_DIR, "-dont_leave_site_enter_dir",
      cond_dont_leave_site_enter_dir, TRUE, CL0 | CL2},
  {CONDT_APORTS, "-aport", cond_aport, TRUE, CL0 | CL2},
  {CONDT_DPORTS, "-dport", cond_dport, TRUE, CL0 | CL2},
  {CONDT_MAX_SIZE, "-max_size", cond_max_size, TRUE, CL3},
  {CONDT_MIN_SIZE, "-min_size", cond_min_size, TRUE, CL3},
  {CONDT_AMIME_TYPE, "-amimet", cond_amime_type, TRUE, CL3},
  {CONDT_DMIME_TYPE, "-dmimet", cond_dmime_type, TRUE, CL3},
  {CONDT_NEWER_THAN, "-newer_than", cond_newer_than, TRUE, CL3},
  {CONDT_OLDER_THAN, "-older_than", cond_older_than, TRUE, CL3},
  {CONDT_AIP_PATTERN, "-aip_pattern", cond_aip_pattern, TRUE, CL0 | CL2},
  {CONDT_DIP_PATTERN, "-dip_pattern", cond_dip_pattern, TRUE, CL0 | CL2},
  {CONDT_PATTERN, "-pattern", cond_pattern, FALSE, CL0 | CL2},
  {CONDT_RPATTERN, "-rpattern", cond_rpattern, FALSE, CL0 | CL2},
  {CONDT_SKIP_PATTERN, "-skip_pattern", cond_skip_pattern, FALSE, CL0 | CL2},
  {CONDT_SKIP_RPATTERN, "-skip_rpattern", cond_skip_rpattern, FALSE,
      CL0 | CL2},
  {CONDT_URL_PATTERN, "-url_pattern", cond_url_pattern, FALSE, CL0 | CL2},
  {CONDT_URL_RPATTERN, "-url_rpattern", cond_url_rpattern, FALSE, CL0 | CL2},
  {CONDT_SKIP_URL_PATTERN, "-skip_url_pattern", cond_skip_url_pattern, FALSE,
      CL0 | CL2},
  {CONDT_SKIP_URL_RPATTERN, "-skip_url_rpattern", cond_skip_url_rpattern,
      FALSE, CL0 | CL2},
  {CONDT_TAG_PATTERN, "-tag_pattern", cond_tag_pattern, TRUE, CL0},
  {CONDT_TAG_RPATTERN, "-tag_rpattern", cond_tag_rpattern, TRUE, CL0},
  {CONDT_USER_CONDITION, "-user_condition", cond_user_condition, FALSE,
      CL1 | CL3},
};

static const struct cond_type_info_t *cond_type_info_find(char *name)
{
  int i;

  for(i = 0; i < NUM_ELEM(cond_type_info); i++)
  {
    if(!strcasecmp(cond_type_info[i].name, name))
      return &(cond_type_info[i]);
  }
  return NULL;
}

int url_append_one_condition(char *name, url * urlp, cond_info_t * condp)
{
  const struct cond_type_info_t *cond;

  if((cond = cond_type_info_find(name)))
  {
    condp->reason = cond->type;
    return (cond->cond_func(urlp, condp) == TRUE);
  }
  else
    return -1;
}

#define DLMSG(i) \
  { \
    char *urlstr = url_to_urlstr(urlp, FALSE); \
    DEBUG_LIMITS("Failed URL condition (%s) -> %s\n", cond_type_info[i].name, \
      urlstr); \
    _free(urlstr); \
    condp->reason = cond_type_info[i].type; \
  }

#ifdef HAVE_REGEX
static int url_append_condition_patterns_default(url * urlp,
  cond_info_t * condp)
{
  int pm1, pm2, pm3, pm4;

  pm1 = cond_pattern(urlp, condp);
  pm3 = cond_rpattern(urlp, condp);

  if(priv_cfg.condition.pattern && priv_cfg.condition.rpattern)
  {
    if(!pm1 && !pm3)
    {
      if(!pm1)
      {
        DLMSG(CONDT_PATTERN);
      }
      else
      {
        DLMSG(CONDT_RPATTERN);
      }
      return FALSE;
    }
  }
  else if(priv_cfg.condition.pattern)
  {
    if(!pm1)
    {
      DLMSG(CONDT_PATTERN);
      return FALSE;
    }
  }
  else if(priv_cfg.condition.rpattern)
  {
    if(!pm3)
    {
      DLMSG(CONDT_RPATTERN);
      return FALSE;
    }
  }

  pm2 = cond_skip_pattern(urlp, condp);
  pm4 = cond_skip_rpattern(urlp, condp);

  if(priv_cfg.condition.skip_pattern && priv_cfg.condition.rskip_pattern)
  {
    if(!pm2 && !pm4)
    {
      if(!pm2)
      {
        DLMSG(CONDT_SKIP_PATTERN);
      }
      else
      {
        DLMSG(CONDT_SKIP_RPATTERN);
      }
      return FALSE;
    }
  }
  else if(priv_cfg.condition.skip_pattern)
  {
    if(!pm2)
    {
      DLMSG(CONDT_SKIP_PATTERN);
      return FALSE;
    }
  }
  else if(priv_cfg.condition.rskip_pattern)
  {
    if(!pm4)
    {
      DLMSG(CONDT_SKIP_RPATTERN);
      return FALSE;
    }
  }

  pm1 = cond_url_pattern(urlp, condp);
  pm3 = cond_url_rpattern(urlp, condp);

  if(priv_cfg.condition.url_pattern && priv_cfg.condition.rurl_pattern)
  {
    if(!pm1 && !pm3)
    {
      if(!pm1)
      {
        DLMSG(CONDT_URL_PATTERN);
      }
      else
      {
        DLMSG(CONDT_URL_RPATTERN);
      }
      return FALSE;
    }
  }
  else if(priv_cfg.condition.url_pattern)
  {
    if(!pm1)
    {
      DLMSG(CONDT_URL_PATTERN);
      return FALSE;
    }
  }
  else if(priv_cfg.condition.rurl_pattern)
  {
    if(!pm3)
    {
      DLMSG(CONDT_URL_RPATTERN);
      return FALSE;
    }
  }

  pm2 = cond_skip_url_pattern(urlp, condp);
  pm4 = cond_skip_url_rpattern(urlp, condp);

  if(priv_cfg.condition.skip_url_pattern &&
    priv_cfg.condition.rskip_url_pattern)
  {
    if(!pm2 && !pm4)
    {
      if(!pm2)
      {
        DLMSG(CONDT_SKIP_URL_PATTERN);
      }
      else
      {
        DLMSG(CONDT_SKIP_URL_RPATTERN);
      }
      return FALSE;
    }
  }
  else if(priv_cfg.condition.skip_url_pattern)
  {
    if(!pm2)
    {
      DLMSG(CONDT_SKIP_URL_PATTERN);
      return FALSE;
    }
  }
  else if(priv_cfg.condition.rskip_url_pattern)
  {
    if(!pm4)
    {
      DLMSG(CONDT_SKIP_URL_RPATTERN);
      return FALSE;
    }
  }

  return TRUE;
}
#else
static int url_append_condition_patterns_default(url * urlp,
  cond_info_t * condp)
{
  int pm1;

  pm1 = cond_pattern(urlp, condp);
  if(cfg.condition.pattern && !pm1)
  {
    DLMSG(CONDT_PATTERN);
    return FALSE;
  }

  pm1 = cond_skip_pattern(urlp, condp);
  if(cfg.condition.skip_pattern && !pm1)
  {
    DLMSG(CONDT_SKIP_PATTERN);
    return FALSE;
  }

  pm1 = cond_url_pattern(urlp, condp);
  if(cfg.condition.url_pattern && !pm1)
  {
    DLMSG(CONDT_URL_PATTERN);
    return FALSE;
  }

  pm1 = cond_skip_url_pattern(urlp, condp);
  if(cfg.condition.skip_url_pattern && !pm1)
  {
    DLMSG(CONDT_SKIP_URL_PATTERN);
    return FALSE;
  }

  return TRUE;
}
#endif

/********************************************************/
/* check wheter URL match given limiting conditions */
/* following default pavuk rules      */
/********************************************************/
static int url_append_condition_default(url * urlp, cond_info_t * condp)
{
  int i;
  int level;

  level = (1 << condp->level);

  if(!cfg.condition.limit_inlines && (urlp->status & URL_INLINE_OBJ))
    return TRUE;

  for(i = 0; i < NUM_ELEM(cond_type_info); i++)
  {
    if(cond_type_info[i].standard &&
      (cond_type_info[i].level & level) &&
      !cond_type_info[i].cond_func(urlp, condp))
    {
      DLMSG(i);
      return FALSE;
    }
  }

  if((cond_type_info[CONDT_PATTERN].level & level))
  {
    if(!url_append_condition_patterns_default(urlp, condp))
      return FALSE;
  }

  if((cond_type_info[CONDT_USER_CONDITION].level & level))
  {
    if(!cond_user_condition(urlp, condp))
    {
      DLMSG(CONDT_USER_CONDITION);
      return FALSE;
    }
  }

  return TRUE;
}

/********************************************************/
/* check wheter URL match given limiting conditions */
/********************************************************/
int url_append_condition(url * urlp, cond_info_t * condp)
{
#ifdef HAVE_MOZJS
  int rv;

  if(!cond_unsupported(urlp, condp))
    return FALSE;

  rv = pjs_run_cond_check_func(urlp, condp);

  return (rv < 0) ? url_append_condition_default(urlp, condp) : rv;
#else
  return url_append_condition_default(urlp, condp);
#endif
}

/********************************************************/
/* check wheter site is from one of domains from aray */
/********************************************************/
static bool_t domain_condition(char *site, char **l)
{
  char **p = l;
  int sl = strlen(site);

  while(*p)
  {
    int dl = strlen(*p);

    if(dl <= sl)
    {
      if(!strcasecmp(*p, site + sl - dl))
        return TRUE;
    }
    p++;
  }
  return FALSE;
}

/********************************************************/
/* check wheter up have suffix sfx      */
/********************************************************/
static bool_t cmp_sfx(char *up, char *sfx)
{
  char *pom = NULL;
  int nlen, slen;
  int rv;

  slen = strlen(sfx);
  nlen = strlen(up);

  if(nlen < slen)
    return FALSE;

  rv = (!strcmp(sfx, up + nlen - slen));

  _free(pom);
  return rv;
}

/********************************************************/
/* check wheter url have match one of sufixes from aray */
/********************************************************/
static bool_t sfx_condition(url * urlr, char **l)
{
  char **pp = l;
  char *p = url_get_full_path(urlr);
  int rv = FALSE;

  while(*pp)
  {
    if(cmp_sfx(p, *pp))
    {
      rv = TRUE;
      break;
    }
    pp++;
  }

  _free(p);

  return rv;
}

/********************************************************/
/* check wheter url path have one of prefixes from  */
/* priv_cfg.condition.dir_prefix      */
/********************************************************/
static bool_t prefix_condition(url * urlr, char **l)
{
  char **pp = l;
  char *p = url_get_full_path(urlr);
  int rv = FALSE;

  while(*pp)
  {
    if(!strncmp(*pp, p, strlen(*pp)))
    {
      rv = TRUE;
      break;
    }
    pp++;
  }

  _free(p);

  return rv;
}

/********************************************************/
/* check wheter string is mattached by at least one */
/* wildcard pattern from list       */
/********************************************************/
static bool_t cmp_pattern(char *str, char **pattern)
{
  char **pp;

  for(pp = pattern; pp && *pp; pp++)
  {
    if(!fnmatch(*pp, str, 0))
      return FALSE;
  }

  return (pattern != NULL);
}

static bool_t cmp_dlpattern(char *str, dllist * pattern)
{
  for(; pattern; pattern = pattern->next)
  {
    if(!fnmatch((char *) pattern->data, str, 0))
      return FALSE;
  }

  return (pattern != NULL);
}


static char **str_list_to_array(dllist * sl)
{
  char **rv;
  int i;

  rv = _malloc(sizeof(char *) * (dllist_count(sl) + 1));

  for(i = 0; sl; i++, sl = sl->next)
    rv[i] = (char *)sl->data;
  rv[i] = NULL;

  return rv;
}

#ifdef HAVE_REGEX
/********************************************************/
/* check wheter string is mattached by at least one */
/* regular pattern from list        */
/********************************************************/
static bool_t cmp_rpattern(char *str, dllist * pattern)
{
  dllist *pp;

  for(pp = pattern; pp; pp = pp->next)
  {
    if(re_pmatch((re_entry *) pp->data, str))
    {
      return FALSE;
    }
  }

  return (pattern != NULL);
}

static dllist *str_list_to_re_list(dllist * sl)
{
  dllist *rv = NULL;

  for(; sl; sl = sl->next)
  {
    re_entry *re = re_make((char *) sl->data);

    if(re)
      rv = dllist_append(rv, (dllist_t)re);
  }

  return rv;
}

/********************************************************/
/* check wheter site of URL matches one of listed IP  */
/* adress regular patterns        */
/*              */
/* TRUE  = match          */
/* FALSE = don't match          */
/* -1    = error resolving hostname     */
/********************************************************/
static int check_ip_list(url * urlp, dllist * iplist)
{
  int retv = -1;
  char *site = url_get_site(urlp);

  if(site && iplist)
  {
    char *ip = NULL;
    int rv, f;
    int is_valid = TRUE;
    char raddr[64];

    _h_errno_ = 0;
    memset(&raddr, '\0', sizeof(raddr));

    if(dns_gethostbyname(site, &rv, raddr, &f))
      is_valid = FALSE;

#ifdef HAVE_INET6
    if(is_valid)
    {
      char buf[64];
      inet_ntop(f, raddr, buf, sizeof(buf));
      ip = tl_strdup(buf);
    }
#else
    if(is_valid)
    {
      struct in_addr ia;
      memcpy(&ia, raddr, TL_MIN(rv, sizeof(ia)));
      LOCK_INETNTOA;
      ip = tl_strdup(inet_ntoa(ia));
      UNLOCK_INETNTOA;
    }
#endif

    if(is_valid)
      retv = !cmp_rpattern(ip, iplist);
    _free(ip);
  }
  return retv;
}
#endif


/********************************************************/
/* below are functions for implementing particular  */
/* limiting options         */
/********************************************************/

static int cond_unsupported(url * urlp, cond_info_t * condp)
{
  /*
     0 - file    x
     1 - directory   x
     2 - CSO index   x
     3 - error
     4 - macbinhex   x
     5 - dosbin    x
     6 - uuencoded   x
     7 - index
     8 - telnet
     9 - bin     x
     + - redundant server
     T - t3270
     g - GIF     x
     I - image   x
     h - HTML    x
     i - info
     w - WWW address
     s - sound   x
     : - image   x
     ; - movie   x
     < - sound   x
   */
  if((urlp->type == URLT_GOPHER) &&
    strchr("0124569gIhs:;<", urlp->p.gopher.selector[0]))
    return TRUE;

  return prottable[urlp->type].supported;
}

static int cond_lmax(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    int n = _atoi((char *) condp->params->data);

    return (urlp->level - ((urlp->status & URL_INLINE_OBJ) ? 1 : 0)) <= n;
  }
  else if(cfg.condition.max_levels)
    return (urlp->level - ((urlp->status & URL_INLINE_OBJ) ? 1 : 0))
      <= cfg.condition.max_levels;
  else
    return TRUE;
}

static int cond_dmax(url * urlp, cond_info_t * condp)
{
  int n;

  if(condp->params)
    n = _atoi((char *) condp->params->data);
  else
    n = cfg.condition.max_documents;

  if(n)
    return !((!condp->urlnr &&
        (cfg.total_cnt + 1) > n) || (condp->urlnr && condp->urlnr > n));
  else
    return TRUE;
}

static int cond_noftp(url * urlp, cond_info_t * condp)
{
  return cfg.condition.ftp ? TRUE : (urlp->type != URLT_FTP);
}

static int cond_nhttp(url * urlp, cond_info_t * condp)
{
  return cfg.condition.http ? TRUE : (urlp->type != URLT_HTTP);
}

static int cond_nossl(url * urlp, cond_info_t * condp)
{
#ifdef USE_SSL
  return cfg.condition.https ? TRUE : (urlp->type != URLT_HTTPS);
#else
  return TRUE;
#endif
}

static int cond_nogopher(url * urlp, cond_info_t * condp)
{
  return cfg.condition.gopher ? TRUE : (urlp->type != URLT_GOPHER);
}

static int cond_noftps(url * urlp, cond_info_t * condp)
{
#ifdef USE_SSL
  return cfg.condition.ftps ? TRUE : (urlp->type != URLT_FTPS);
#else
  return TRUE;
#endif
}

static int cond_nocgi(url * urlp, cond_info_t * condp)
{
  if((urlp->type == URLT_HTTP || urlp->type == URLT_HTTPS) &&
    !cfg.condition.cgi)
    return (urlp->p.http.searchstr == NULL);
  else
    return TRUE;
}

static int cond_asite(url * urlp, cond_info_t * condp)
{
  char *site = url_get_site(urlp);

  if(!site)
    return TRUE;

  if(condp->params)
    return is_in_dllist(site, condp->params);
  else if(priv_cfg.condition.sites && priv_cfg.condition.sites[0] &&
    site && priv_cfg.condition.allow_site)
    return is_in_list(site, priv_cfg.condition.sites);
  else
    return TRUE;

}

static int cond_dsite(url * urlp, cond_info_t * condp)
{
  char *site = url_get_site(urlp);

  if(!site)
    return TRUE;

  if(condp->params)
    return !is_in_dllist(site, condp->params);
  if(priv_cfg.condition.sites && priv_cfg.condition.sites[0] && site &&
    !priv_cfg.condition.allow_site)
    return !is_in_list(site, priv_cfg.condition.sites);
  else
    return TRUE;
}

static int cond_adomain(url * urlp, cond_info_t * condp)
{
  char *site = url_get_site(urlp);

  if(!site)
    return TRUE;

  if(condp->params)
  {
    char **sa = str_list_to_array(condp->params);
    int rv = domain_condition(site, sa);
    _free(sa);
    return rv;
  }
  else if(priv_cfg.condition.domains && priv_cfg.condition.domains[0] &&
    site && priv_cfg.condition.allow_domain)
    return domain_condition(site, priv_cfg.condition.domains);
  else
    return TRUE;
}

static int cond_ddomain(url * urlp, cond_info_t * condp)
{
  char *site = url_get_site(urlp);

  if(!site)
    return TRUE;

  if(condp->params)
  {
    char **sa = str_list_to_array(condp->params);
    int rv = !domain_condition(site, sa);
    _free(sa);
    return rv;
  }
  else if(priv_cfg.condition.domains && priv_cfg.condition.domains[0] &&
    site && !priv_cfg.condition.allow_domain)
    return !domain_condition(site, priv_cfg.condition.domains);
  else
    return TRUE;
}

static int cond_aprefix(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    char **sa = str_list_to_array(condp->params);
    int rv = prefix_condition(urlp, sa);
    _free(sa);
    return rv;
  }
  else if(priv_cfg.condition.dir_prefix &&
    priv_cfg.condition.dir_prefix[0] && (urlp->type != URLT_FILE) &&
    priv_cfg.condition.allow_prefix)
    return prefix_condition(urlp, priv_cfg.condition.dir_prefix);
  else
    return TRUE;
}

static int cond_dprefix(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    char **sa = str_list_to_array(condp->params);
    int rv = !prefix_condition(urlp, sa);
    _free(sa);
    return rv;
  }
  else if(priv_cfg.condition.dir_prefix &&
    priv_cfg.condition.dir_prefix[0] && (urlp->type != URLT_FILE) &&
    !priv_cfg.condition.allow_prefix)
    return !prefix_condition(urlp, priv_cfg.condition.dir_prefix);
  else
    return TRUE;
}

static int cond_asfx(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    char **sa = str_list_to_array(condp->params);
    int rv = sfx_condition(urlp, sa);
    _free(sa);
    return rv;
  }
  else if(priv_cfg.condition.sufix && priv_cfg.condition.sufix[0] &&
    (urlp->type != URLT_FILE) && priv_cfg.condition.allow_sufix)
    return sfx_condition(urlp, priv_cfg.condition.sufix);
  else
    return TRUE;
}

static int cond_dsfx(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    char **sa = str_list_to_array(condp->params);
    int rv = !sfx_condition(urlp, sa);
    _free(sa);
    return rv;
  }
  else if(priv_cfg.condition.sufix && priv_cfg.condition.sufix[0] &&
    (urlp->type != URLT_FILE) && !priv_cfg.condition.allow_sufix)
    return !sfx_condition(urlp, priv_cfg.condition.sufix);
  else
    return TRUE;
}

static int cond_pattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;

  if(condp->params)
  {
    char *p = url_get_full_path(urlp);
    rv = !cmp_dlpattern(p, condp->params);
    _free(p);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.pattern)
  {
    char *p = url_get_full_path(urlp);
    rv = !cmp_pattern(p, priv_cfg.condition.pattern);
    _free(p);
  }
  return rv;
}

static int cond_rpattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;
#ifdef HAVE_REGEX
  if(condp->params)
  {
    dllist *pl = str_list_to_re_list(condp->params);

    if(pl)
    {
      char *p = url_get_full_path(urlp);
      rv = !cmp_rpattern(p, pl);
      _free(p);
    }

    for(; pl; pl = dllist_remove_entry(pl, pl))
      re_free((re_entry *) pl->data);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.rpattern)
  {
    char *p = url_get_full_path(urlp);
    rv = !cmp_rpattern(p, priv_cfg.condition.rpattern);
    _free(p);
  }
#endif
  return rv;
}

static int cond_skip_pattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;

  if(condp->params)
  {
    char *p = url_get_full_path(urlp);
    rv = cmp_dlpattern(p, condp->params);
    _free(p);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.skip_pattern)
  {
    char *p = url_get_full_path(urlp);
    rv = cmp_pattern(p, priv_cfg.condition.skip_pattern);
    _free(p);
  }
  return rv;
}

static int cond_skip_rpattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;
#ifdef HAVE_REGEX
  if(condp->params)
  {
    dllist *pl = str_list_to_re_list(condp->params);

    if(pl)
    {
      char *p = url_get_full_path(urlp);
      rv = cmp_rpattern(p, pl);
      _free(p);
    }

    for(; pl; pl = dllist_remove_entry(pl, pl))
      re_free((re_entry *) pl->data);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.rskip_pattern)
  {
    char *p = url_get_full_path(urlp);
    rv = cmp_rpattern(p, priv_cfg.condition.rskip_pattern);
    _free(p);
  }
#endif
  return rv;
}

static int cond_url_pattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;

  if(condp->params)
  {
    char *p = url_to_urlstr(urlp, FALSE);
    rv = !cmp_dlpattern(p, condp->params);
    _free(p);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.url_pattern)
  {
    char *p = url_to_urlstr(urlp, FALSE);
    rv = !cmp_pattern(p, priv_cfg.condition.url_pattern);
    _free(p);
  }
  return rv;
}

static int cond_url_rpattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;
#ifdef HAVE_REGEX
  if(condp->params)
  {
    dllist *pl = str_list_to_re_list(condp->params);

    if(pl)
    {
      char *p = url_to_urlstr(urlp, FALSE);
      rv = !cmp_rpattern(p, pl);
      _free(p);
    }

    for(; pl; pl = dllist_remove_entry(pl, pl))
      re_free((re_entry *) pl->data);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.rurl_pattern)
  {
    char *p = url_to_urlstr(urlp, FALSE);
    rv = !cmp_rpattern(p, priv_cfg.condition.rurl_pattern);
    _free(p);
  }
#endif
  return rv;
}

static int cond_skip_url_pattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;

  if(condp->params)
  {
    char *p = url_to_urlstr(urlp, FALSE);
    rv = cmp_dlpattern(p, condp->params);
    _free(p);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.skip_url_pattern)
  {
    char *p = url_to_urlstr(urlp, FALSE);
    rv = cmp_pattern(p, priv_cfg.condition.skip_url_pattern);
    _free(p);
  }
  return rv;
}

static int cond_skip_url_rpattern(url * urlp, cond_info_t * condp)
{
  int rv = TRUE;
#ifdef HAVE_REGEX
  if(condp->params)
  {
    dllist *pl = str_list_to_re_list(condp->params);

    if(pl)
    {
      char *p = url_to_urlstr(urlp, FALSE);
      rv = cmp_rpattern(p, pl);
      _free(p);
    }

    for(; pl; pl = dllist_remove_entry(pl, pl))
      re_free((re_entry *) pl->data);
  }
  else if(urlp->type != URLT_FILE && priv_cfg.condition.rskip_url_pattern)
  {
    char *p = url_to_urlstr(urlp, FALSE);
    rv = cmp_rpattern(p, priv_cfg.condition.rskip_url_pattern);
    _free(p);
  }
#endif
  return rv;
}

static int cond_dont_leave_site(url * urlp, cond_info_t * condp)
{
  url *gparent = urlp;

  if(urlp->type != URLT_FILE && cfg.condition.dont_leave_site)
  {
    bool_t isgp = FALSE;

    while(!isgp)
    {
#ifdef HAVE_MT
      url *ogp = gparent;
#endif
      if(gparent->status & URL_ISSTARTING)
      {
        isgp = TRUE;
        break;
      }

      LOCK_URL(ogp);
      if(gparent->parent_url)
        gparent = (url *) gparent->parent_url->data;
      else
        isgp = TRUE;
      UNLOCK_URL(ogp);
    }

    return url_is_same_site(urlp, gparent);
  }
  else
    return TRUE;
}

static int cond_dont_leave_dir(url * urlp, cond_info_t * condp)
{
  if(urlp->type != URLT_FILE && cfg.condition.dont_leave_dir)
  {
    url *gparent = urlp;
    char *p1, *p2, *p;
    int len = 0;
    bool_t isgp = FALSE;

    while(!isgp)
    {
#ifdef HAVE_MT
      url *ogp = gparent;
#endif
      if(gparent->status & URL_ISSTARTING)
      {
        isgp = TRUE;
        break;
      }

      LOCK_URL(ogp);
      if(gparent->parent_url)
        gparent = (url *) gparent->parent_url->data;
      else
        isgp = TRUE;
      UNLOCK_URL(ogp);
    }

    p1 = url_get_path(urlp);
    p2 = url_get_path(gparent);

    p = strrchr(p2, '/');
    if(p)
      len = p - p2;

    return url_is_same_site(urlp, gparent) && !strncmp(p1, p2, len);
  }
  else
    return TRUE;
}

static int cond_user_condition(url * urlp, cond_info_t * condp)
{
  return priv_cfg.condition.uexit ? uexit_condition(urlp, NULL, 0L) : TRUE;
}

static int cond_aip_pattern(url * urlp, cond_info_t * condp)
{
#ifdef HAVE_REGEX
  if(condp->params)
  {
    int rv = TRUE;
    dllist *p = str_list_to_re_list(condp->params);

    if(p)
      rv = check_ip_list(urlp, p);

    for(; p; p = dllist_remove_entry(p, p))
      re_free((re_entry *) p->data);

    return rv;
  }
  else if(priv_cfg.condition.aip)
    return check_ip_list(urlp, priv_cfg.condition.aip);
#endif
  return TRUE;
}

static int cond_dip_pattern(url * urlp, cond_info_t * condp)
{
#ifdef HAVE_REGEX
  if(condp->params)
  {
    int rv = TRUE;
    dllist *p = str_list_to_re_list(condp->params);

    if(p)
    {
      rv = check_ip_list(urlp, p);

      if(rv == 0 || rv == -1)
        rv = TRUE;
      else
        rv = FALSE;
    }

    for(; p; p = dllist_remove_entry(p, p))
      re_free((re_entry *) p->data);

    return rv;
  }
  else if(priv_cfg.condition.skipip)
  {
    int rv = check_ip_list(urlp, priv_cfg.condition.skipip);

    if(rv == 0 || rv == -1)
      return TRUE;
    else
      return FALSE;
  }
#endif
  return TRUE;
}

static int cond_site_level(url * urlp, cond_info_t * condp)
{
  int lvl;

  if(condp->params)
    lvl = _atoi((char *) condp->params->data);
  else
    lvl = cfg.condition.site_level;

  if(urlp->type != URLT_FILE && lvl)
  {
    url *curl = urlp;
    url *parent;
    int level = 0;
    int slevel = 0;

    LOCK_URL(urlp);
    if(urlp->parent_url)
      parent = (url *) urlp->parent_url->data;
    else
      parent = NULL;
    UNLOCK_URL(urlp);

    while(parent)
    {
      if(parent->status & URL_ISSTARTING)
        break;

      if((curl->type != parent->type) ||
        (url_get_port(curl) != url_get_port(parent)) ||
        strcmp(url_get_site(curl), url_get_site(parent)))
      {
        if(!curl->moved_to || slevel)
          level++;
        slevel = 0;
      }
      else if(!curl->moved_to)
        slevel++;
      curl = parent;
      LOCK_URL(curl);
      if(urlp->parent_url)
        parent = (url *) parent->parent_url->data;
      else
        parent = NULL;
      UNLOCK_URL(curl);
    }

    return level <= lvl;
  }
  else
    return TRUE;
}

static int cond_dont_leave_site_enter_dir(url * urlp, cond_info_t * condp)
{
  if(urlp->type != URLT_FILE && cfg.condition.dont_leave_site_dir)
  {
    url *gparent = urlp;
    char *p1, *p2, *p;
    int len = 0;
    bool_t isgp = FALSE;

    while(!isgp)
    {
      url *ogp = gparent;

      if(gparent->status & URL_ISSTARTING)
      {
        isgp = TRUE;
      }
      else
      {
        LOCK_URL(ogp);
        if(gparent->parent_url)
          gparent = (url *) gparent->parent_url->data;
        else
          isgp = TRUE;
        UNLOCK_URL(ogp);
      }

      if((ogp->type != gparent->type) ||
        (url_get_port(urlp) != url_get_port(gparent)) ||
        strcmp(url_get_site(urlp), url_get_site(gparent)))
      {
        gparent = ogp;
        break;
      }
    }

    while(gparent->moved_to)
    {
      if(gparent == urlp)
        break;
      gparent = gparent->moved_to;
    }

    if(!isgp)
    {
      p1 = url_get_path(urlp);
      p2 = url_get_path(gparent);

      p = strrchr(p2, '/');
      if(p)
        len = p - p2;

      return url_is_same_site(urlp, gparent) && !strncmp(p1, p2, len);
    }
  }

  return TRUE;
}

static int cond_leave_level(url * urlp, cond_info_t * condp)
{
  int lvl;

  if(condp->params)
    lvl = _atoi((char *) condp->params->data);
  else
    lvl = cfg.condition.leave_level;

  if(urlp->type != URLT_FILE && lvl)
  {
    url *gparent = urlp;
    url *pomurl = urlp;
    int level = -1;
    bool_t isgp = FALSE;

    while(!isgp)
    {
#ifdef HAVE_MT
      url *ogp = gparent;
#endif
      if(gparent->status & URL_ISSTARTING)
      {
        isgp = TRUE;
        break;
      }

      LOCK_URL(ogp);
      if(gparent->parent_url)
        gparent = (url *) gparent->parent_url->data;
      else
        isgp = TRUE;
      UNLOCK_URL(ogp);
    }

    while(pomurl)
    {
#ifdef HAVE_MT
      url *tempurl = pomurl;
#endif
      if((pomurl->type == gparent->type) &&
        (url_get_port(pomurl) == url_get_port(gparent)) &&
        !strcmp(url_get_site(pomurl), url_get_site(gparent)))
      {
        break;
      }

      if(!pomurl->moved_to)
        level++;

      if((level - ((urlp->status & URL_INLINE_OBJ) ? 1 : 0)) >= lvl)
      {
        return FALSE;
      }

      if(pomurl->status & URL_ISSTARTING)
      {
        pomurl = NULL;
      }
      else
      {
        LOCK_URL(tempurl);
        if(pomurl->parent_url)
          pomurl = (url *) pomurl->parent_url->data;
        else
          pomurl = NULL;
        UNLOCK_URL(tempurl);
      }
    }
  }

  return TRUE;
}

static int cond_aport(url * urlp, cond_info_t * condp)
{
  long port = url_get_port(urlp);

  if(condp->params)
  {
    char pom[10];

    sprintf(pom, "%ld", port);
    return !dllist_find2(condp->params, (dllist_t)pom, str_comp_func);
  }
  else if(priv_cfg.condition.ports && port && priv_cfg.condition.allow_ports)
  {
    return (dllist_find(priv_cfg.condition.ports, (dllist_t) port)
    ? TRUE : FALSE);
  }
  else
    return TRUE;
}

static int cond_dport(url * urlp, cond_info_t * condp)
{
  long port = url_get_port(urlp);

  if(condp->params)
  {
    char pom[10];

    sprintf(pom, "%ld", port);
    return !dllist_find2(condp->params, (dllist_t)pom, str_comp_func);
  }
  else if(priv_cfg.condition.ports && port && !priv_cfg.condition.allow_ports)
  {
    return ((!dllist_find(priv_cfg.condition.ports, (dllist_t) port))
    ? TRUE : FALSE);
  }
  else
    return TRUE;
}

static int cond_max_size(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    int n = _atoi((char *) condp->params->data);

    if(n)
      return (n >= condp->size);
  }
  else if(cfg.condition.max_size && condp->size)
    return (cfg.condition.max_size >= condp->size);

  return TRUE;
}

static int cond_min_size(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    int n = _atoi((char *) condp->params->data);

    if(n)
      return (n <= condp->size);
  }
  else if(cfg.condition.min_size && condp->size)
    return (cfg.condition.min_size <= condp->size);

  return TRUE;
}

static int cond_amime_type(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    return !is_in_pattern_dllist(condp->mimet, condp->params);
  }
  else if(priv_cfg.condition.mime && priv_cfg.condition.allow_mime &&
    condp->mimet)
  {
    return is_in_pattern_list(condp->mimet, priv_cfg.condition.mime);
  }
  else
    return TRUE;
}

static int cond_dmime_type(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    return !is_in_pattern_dllist(condp->mimet, condp->params);
  }
  else if(priv_cfg.condition.mime && !priv_cfg.condition.allow_mime &&
    condp->mimet)
  {
    return !is_in_pattern_list(condp->mimet, priv_cfg.condition.mime);
  }
  else
    return TRUE;
}

static int cond_newer_than(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    time_t t = _atoi((char *) condp->params->data);

    if(t)
      return difftime(condp->time, t) <= 0;
  }
  else if(cfg.condition.etime && condp->time)
    return difftime(condp->time, cfg.condition.etime) <= 0;

  return TRUE;
}

static int cond_older_than(url * urlp, cond_info_t * condp)
{
  if(condp->params)
  {
    time_t t = _atoi((char *) condp->params->data);

    if(t)
      return difftime(condp->time, t) >= 0;
  }
  else if(cfg.condition.etime && condp->time)
    return difftime(condp->time, cfg.condition.btime) >= 0;

  return TRUE;
}

static int cond_tag_pattern(url * urlp, cond_info_t * condp)
{
  tag_pattern_t *tp;
  char *p = NULL;

  if(!condp->tag || !condp->attrib)
    return TRUE;

  if(condp->params)
  {
    if(dllist_count(condp->params) != 3)
      return FALSE;
    else
      tp = tag_pattern_new(TAGP_WC,
        (char *) dllist_nth(condp->params, 0),
        (char *) dllist_nth(condp->params, 1),
        (char *) dllist_nth(condp->params, 2));

    if(tp)
    {
      int r;

      p = url_to_urlstr(urlp, FALSE);
      r = tag_pattern_match(tp, condp->tag, condp->attrib, p);
      tag_pattern_free(tp);
      _free(p);

      return r;
    }
    else
      return FALSE;
  }
  else
  {
    dllist *ptr;

    if(priv_cfg.condition.tag_patterns)
      p = url_to_urlstr(urlp, FALSE);

    for(ptr = priv_cfg.condition.tag_patterns; ptr; ptr = ptr->next)
    {
      tp = (tag_pattern_t *) ptr->data;

      if(tag_pattern_match(tp, condp->tag, condp->attrib, p))
      {
        _free(p);
        return TRUE;
      }

    }
    _free(p);
    return priv_cfg.condition.tag_patterns == NULL;
  }
}

static int cond_tag_rpattern(url * urlp, cond_info_t * condp)
{
  if(!condp->tag || !condp->attrib)
    return TRUE;

  if(condp->params)
  {
    tag_pattern_t *tp;

    if(dllist_count(condp->params) != 3)
      return FALSE;
    else
      tp = tag_pattern_new(TAGP_RE,
        (char *) dllist_nth(condp->params, 0),
        (char *) dllist_nth(condp->params, 1),
        (char *) dllist_nth(condp->params, 2));

    if(tp)
    {
      int r;
      char *p;

      p = url_to_urlstr(urlp, FALSE);
      r = tag_pattern_match(tp, condp->tag, condp->attrib, p);
      tag_pattern_free(tp);
      _free(p);

      return r;
    }
    else
      return FALSE;
  }
  else
  {
    /* always return TRUE standard is handled by cond_tag_pattern */
    return TRUE;
  }
}
