/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "fnmatch.h"
#endif

#include "lfname.h"
#include "url.h"
#include "tools.h"
#include "tr.h"
#include "dlhash_tools.h"
#include "http.h"
#include "form.h"
#include "mime.h"
#include "jsbind.h"

enum lfname_lsp_type
{
  LF_LSP_UNKNOWN, /*** unknown ***/
  LF_LSP_STR, /*** string variable ***/
  LF_LSP_NUM, /*** number variable ***/
  LF_LSP_MACRO, /*** macro variable ***/
  LF_LSP_SUB, /*** subpart from regex ***/
  LF_LSP_SC,  /*** strcat function ***/
  LF_LSP_SS,  /*** substr function ***/
  LF_LSP_HASH,  /*** hash function ***/
  LF_LSP_MD5, /*** md5 function ***/
  LF_LSP_LOWER, /*** lowerstr function ***/
  LF_LSP_UPPER, /*** upperstr function ***/
  LF_LSP_UENC,  /*** urlencode function ***/
  LF_LSP_UDEC,  /*** urldecode function ***/
  LF_LSP_DELCHR,  /*** delchr function ***/
  LF_LSP_TRCHR, /*** trchr function ***/
  LF_LSP_TRSTR, /*** trstr function ***/
  LF_LSP_STRSPN,  /*** strspn function ***/
  LF_LSP_STRCSPN, /*** strcspn function ***/
  LF_LSP_STRLEN,  /*** strlen function ***/
  LF_LSP_NRSTR, /*** nrtostr function ***/
  LF_LSP_LCHR,  /*** last character offset ***/
  LF_LSP_PLS, /*** plus ***/
  LF_LSP_MNS, /*** minus ***/
  LF_LSP_MOD, /*** mod ***/
  LF_LSP_MUL, /*** multiply ***/
  LF_LSP_DIV, /*** divide ***/
  LF_LSP_REMOVEPARAMETER, /*** removes a parameter from an url string */
  LF_LSP_GETVALUE,/*** reads a value from a parameter of an url string */
  LF_LSP_SIF, /*** if condition ***/
  LF_LSP_NOT, /*** logical not ***/
  LF_LSP_AND, /*** logical and ***/
  LF_LSP_OR,  /*** logical or ***/
  LF_LSP_GETEXT,  /*** get extension from path ***/
#ifdef HAVE_MOZJS
  LF_LSP_JSF, /*** result of JavaScript function ***/
#endif
  LF_LSP_SEQ  /*** string equal ***/
};

struct lfname_lsp_var
{
  enum lfname_lsp_type type;
  union
  {
    char *str;
    int num;
    char macro;
  } val;
  enum lfname_lsp_type rettype;
  union
  {
    char *str;
    int num;
  } ret_val;
  struct lfname_lsp_var *param1;
  struct lfname_lsp_var *param2;
  struct lfname_lsp_var *param3;
};


static char *lfname_lsp_get_by_url(struct lfname_lsp_interp *);
static struct lfname_lsp_var *lfname_lsp_analyze(const char **);
static void lfname_lsp_var_free(struct lfname_lsp_var *);

static char *_strfindnchr(char *str, int chr, int n)
{
  int cnt;
  char *p;

  for(p = str, cnt = 0; *p && cnt < n; p++)
  {
    if(*p == chr)
      cnt++;
  }
  if(cnt != n)
    return NULL;
  else
    return p - 1;
}

static char *_strrfindnchr(char *str, int chr, int n)
{
  int cnt;
  char *p;

  for(p = str + strlen(str) - 1, cnt = 0; p >= str && cnt < n; p--)
  {
    if(*p == chr)
      cnt++;
  }
  if(cnt != n)
    return NULL;
  else
    return p + 1;
}

#ifdef HAVE_REGEX
char *lfname_re_sub(lfname * lfnamep, const char *urlstr, int nr)
{
  char pom[4096];

  pom[0] = '\0';

  if(lfnamep->type != LFNAME_REGEX)
    return tl_strdup(pom);

#ifdef HAVE_POSIX_REGEX
  {
    regmatch_t *pmatch = lfnamep->pmatch;
    if(nr >= 0 && nr <= lfnamep->preg.re_nsub)
    {
      strncpy(pom, urlstr + pmatch[nr].rm_so,
        pmatch[nr].rm_eo - pmatch[nr].rm_so); /* FIXME: Security */
      pom[pmatch[nr].rm_eo - pmatch[nr].rm_so] = '\0';
    }
  }
#elif defined(HAVE_V8_REGEX)
#ifdef HAVE_V8_REGSUB
  {
    char ssect[10];
    if(nr)
      sprintf(ssect, "\\%d", nr);
    else
      strcpy(ssect, "&");
    regsub(lfnamep->preg, ssect, pom);
  }
#endif
#elif defined(HAVE_GNU_REGEX)
  if(nr >= 0 && nr < lfnamep->preg.re_nsub)
  {
    strncpy(pom, urlstr + lfnamep->pmatch.start[nr],
      lfnamep->pmatch.end[nr] - lfnamep->pmatch.start[nr]); /* FIXME: Security */
    pom[lfnamep->pmatch.end[nr] - lfnamep->pmatch.start[nr]] = '\0';
  }
#elif defined(HAVE_PCRE_REGEX)
  if(nr >= 0 && nr < lfnamep->pmatch_nr)
  {
    strncpy(pom, urlstr + lfnamep->pmatch[2 * nr],
      lfnamep->pmatch[2 * nr + 1] - lfnamep->pmatch[2 * nr]); /* FIXME: Security */
    pom[lfnamep->pmatch[2 * nr + 1] - lfnamep->pmatch[2 * nr]] = '\0';
  }
#endif
  return tl_strdup(pom);
}
#endif

/* $x - x-th match section  */
/* %i - protocol id   */
/* %p - password    */
/* %u - user name   */
/* %h - host name   */
/* %m - domain name   */
/* %r - port number   */
/* %d - doc path    */
/* %n - doc name    */
/* %b - base name of document */
/* %e - extension   */
/* %s - search string   */
/* %q - POST query string */
/* %M - mime type               */
/* %E - extension by mime type  */
/* %o - default doc name        */
/* %-x - x-th dirname from end  */
/* %x - x-th dirname from start */

char *lfname_get_by_url(url * urlp, const char *urlstr, const char *mime_type,
  lfname * lfnamep)
{
  char *ps, *pd, *pp, *p1, *p2;
  char pom[4096];
  char pstr[4096];
  int nr;
  char *n, *d, *t, *e, *b, *m, *q, *o;
  const char *mimeext;
  char *retv = NULL;

  p1 = url_get_path(urlp);
  if(urlp->type == URLT_GOPHER)
  {
    if(urlp->p.gopher.selector[0] == '1')
      snprintf(pstr, sizeof(pstr), "/%s/%s", urlp->p.gopher.selector, priv_cfg.index_name);
    else
      snprintf(pstr, sizeof(pstr), "/%s", urlp->p.gopher.selector);

  }
  else if(tl_is_dirname(p1) ||
    ((urlp->type == URLT_FTP || urlp->type == URLT_FTPS) && urlp->p.ftp.dir))
  {
    snprintf(pstr, sizeof(pstr), "%s/%s", p1, priv_cfg.index_name);
  }
  else
  {
    strncpy(pstr, p1, sizeof(pstr));
    pstr[sizeof(pstr) - 1] = '\0';
  }

  t = get_abs_file_path(pstr);

  strncpy(pstr, t, sizeof(pstr));
  pstr[sizeof(pstr) - 1] = '\0';

  p1 = strrchr(pstr, '/');

  d = p1 ? tl_strndup(pstr, p1 - pstr) : tl_strdup("");

  n = p1 ? tl_strdup(p1 + 1) : tl_strdup(pstr);

  e = tl_strdup(tl_get_extension(pstr));

  p1 = strrchr(n, '.');

  if(p1)
    b = tl_strndup(n, p1 - n);
  else
    b = tl_strdup(n);

  m = url_get_site(urlp);
  p1 = strchr(m, '.');
  if(p1)
    m = p1 + 1;

  q = NULL;
  if(urlp->status & URL_FORM_ACTION)
  {
    form_info *fi = (form_info *) urlp->extension;

    p1 = form_encode_urlencoded(fi->infos);
    if(p1)
    {
      strncpy(pstr, p1, sizeof(pstr) - 1);
      pstr[sizeof(pstr) - 1] = '\0';
      q = tl_strdup(pstr);
    }
    _free(p1);
  }
  if(!q)
    q = tl_strdup("");

  o = url_get_default_local_name(urlp);
  mimeext = mime_get_type_ext(mime_type);

  pom[0] = '\0';

  if(lfnamep->transstr[0] == '(')
  {
    struct lfname_lsp_interp interp;
    char port[10];

    interp.urlp = urlp;
    interp.urlstr = urlstr;
    interp.scheme = prottable[urlp->type].dirname;
    interp.passwd = url_get_pass(urlp, NULL) ? url_get_pass(urlp, NULL) : "";
    interp.user = url_get_user(urlp, NULL) ? url_get_user(urlp, NULL) : "";
    interp.host = url_get_site(urlp) ? url_get_site(urlp) : "";
    interp.domain = m;
    sprintf(port, "%d", url_get_port(urlp));
    interp.port = port;
    interp.path = d;
    interp.name = n;
    interp.basename = b;
    interp.extension = e;
    interp.query = url_get_search_str(urlp) ? url_get_search_str(urlp) : "";
    interp.post_query = q;
    interp.deflt = o;
    interp.mime_type = mime_type;
    interp.mime_type_ext = mimeext;
    interp.orig = lfnamep;

    retv = lfname_lsp_get_by_url(&interp);
  }
  else
  {
    for(ps = lfnamep->transstr, pd = pom; *ps; ps++)
    {
      if(!*(ps + 1))
      {
        *pd = *ps;
        pd++;
        *pd = '\0';
        continue;
      }
      switch (*ps)
      {
      case '\\':
        ps++;
        *pd = *ps;
        pd++;
        *pd = '\0';
        break;
#ifdef HAVE_REGEX
      case '$':
        ps++;
        nr = strtol(ps, &pp, 10);
        p1 = lfname_re_sub(lfnamep, urlstr, nr);
        strncpy(pd, p1, sizeof(pom)-(pd-pom));
        pd[sizeof(pom) - (pd-pom) - 1] = '\0';
        _free(p1);
        while(*pd)
          pd++;
        ps = pp - 1;
        break;
#endif
      case '%':
        ps++;
        pstr[0] = '\0';
        switch (*ps)
        {
        case 'i':
          strncpy(pstr, prottable[urlp->type].dirname, sizeof(pstr));
          break;
        case 'p':
          strncpy(pstr, url_get_pass(urlp, NULL) ? url_get_pass(urlp,
              NULL) : "", sizeof(pstr));
          break;
        case 'u':
          strncpy(pstr, url_get_user(urlp, NULL) ? url_get_user(urlp,
              NULL) : "", sizeof(pstr));
          break;
        case 'h':
          strncpy(pstr, url_get_site(urlp) ? url_get_site(urlp) : "",
              sizeof(pstr));
          break;
        case 'm':
          strncpy(pstr, m, sizeof(pstr));
          break;
        case 'r':
          sprintf(pstr, "%d", url_get_port(urlp));
          break;
        case 't':
          strncpy(pstr, t, sizeof(pstr));
          break;
        case 'd':
          strncpy(pstr, d, sizeof(pstr));
          break;
        case 'n':
          strncpy(pstr, n, sizeof(pstr));
          break;
        case 'b':
          strncpy(pstr, b, sizeof(pstr));
          break;
        case 'e':
          strncpy(pstr, e, sizeof(pstr));
          break;
        case 's':
          strncpy(pstr,
            url_get_search_str(urlp) ? url_get_search_str(urlp) : "",
            sizeof(pstr));
          break;
        case 'q':
          strncpy(pstr, q, sizeof(pstr));
          break;
        case 'M':
          strncpy(pstr, mime_type ? mime_type : "", sizeof(pstr));
          break;
        case 'E':
          strncpy(pstr, mimeext ? mimeext : "", sizeof(pstr));
          break;
        case 'o':
          strncpy(pstr, o, sizeof(pstr));
          break;
        case '-':
          nr = strtol(ps + 1, &pp, 10);
          p1 = _strrfindnchr(d, '/', nr);
          p2 = _strrfindnchr(d, '/', nr + 1);
          if(!p1)
            pstr[0] = '\0';
          else if(p2)
          {
            strncpy(pstr, p2 + 1, p1 - 1 - p2); /* FIXME: Security */
            *(pstr + (p1 - 1 - p2)) = '\0';
          }
          else
            pstr[0] = '\0';
          ps = pp - 1;
          break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          nr = strtol(ps, &pp, 10);
          p1 = _strfindnchr(d, '/', nr);
          p2 = _strfindnchr(d, '/', nr + 1);
          if(!p1)
            pstr[0] = '\0';
          else if(p2)
          {
            strncpy(pstr, p1 + 1, p2 - 1 - p1); /* FIXME: Security */
            *(pstr + (p2 - 1 - p1)) = '\0';
          }
          else
            strncpy(pstr, p1 + 1, sizeof(pstr));
          ps = pp - 1;
          break;
        default:
          pstr[0] = *(ps - 1);
          pstr[1] = *ps;
          pstr[2] = '\0';
        }
        pstr[sizeof(pstr) - 1] = '\0';
        strcat(pd, pstr); /* FIXME: Security */
        while(*pd)
          pd++;
        break;
      default:
        *pd = *ps;
        pd++;
        *pd = '\0';
      }
    }
    retv = tl_strdup(pom);
  }
  free(e);
  free(n);
  free(t);
  free(d);
  free(q);
  free(o);
  return retv;
}

void lfname_free(lfname * lfnamep)
{
#ifdef HAVE_REGEX
  if(lfnamep->type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    regfree(&(lfnamep->preg));
    _free(lfnamep->pmatch);
#elif defined(HAVE_V8_REGEX)
    _free(lfnamep->preg);
#elif defined(HAVE_GNU_REGEX)
    regfree(&lfnamep->preg);
    _free(lfnamep->pmatch.start);
    _free(lfnamep->pmatch.end);
#elif defined(HAVE_PCRE_REGEX)
    _free(lfnamep->preg);
    _free(lfnamep->preg_extra);
    _free(lfnamep->pmatch);
#endif
  }
#endif

  _free(lfnamep->matchstr);
  _free(lfnamep->transstr);
  _free(lfnamep);
}

lfname *lfname_new(lfname_type type, const char *mpt, const char *str)
{
  lfname *rv;
  const char *p;

  rv = _malloc(sizeof(lfname));
  rv->type = type;
  rv->matchstr = NULL;
  rv->transstr = NULL;
#ifdef HAVE_REGEX
  if(type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    int ec;
    if((ec = regcomp(&(rv->preg), mpt, REG_EXTENDED)))
    {
      char pom[PATH_MAX];
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      regerror(ec, &(rv->preg), pom, sizeof(pom));
      xprintf(0, "%s\n", pom);
      regfree(&(rv->preg));
      free(rv);
      return NULL;
    }
    rv->pmatch = _malloc((rv->preg.re_nsub + 1) * sizeof(regmatch_t));
#elif defined(HAVE_V8_REGEX)
    if(!(rv->preg = regcomp(mpt)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      free(rv->preg);
      free(rv);
      return NULL;
    }
#elif defined(HAVE_BSD_REGEX)
    if((p = re_comp(mpt)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      xprintf(0, "%s", p);
      free(rv);
      return NULL;
    }
#elif defined(HAVE_GNU_REGEX)
    rv->preg.allocated = 0;
    rv->preg.buffer = NULL;
    rv->preg.fastmap = NULL;
    re_set_syntax(r_2phase_star);
    if((p = re_compile_pattern(mpt, strlen(mpt), &rv->preg)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      xprintf(0, "%s\n", p);
      regfree(&(rv->preg));
      free(rv);
      return NULL;
    }
    rv->pmatch.start =
      _malloc((rv->preg.re_nsub + 1) * sizeof(*rv->pmatch.start));
    rv->pmatch.end =
      _malloc((rv->preg.re_nsub + 1) * sizeof(*rv->pmatch.end));
    rv->pmatch.num_regs = rv->preg.re_nsub + 1;
    rv->preg.regs_allocated = REGS_FIXED;
#elif defined(HAVE_PCRE_REGEX)
    int errcode = 0;

    if((rv->preg = pcre_compile(mpt, 0, (const char **) &p, &errcode, NULL)))
    {
      rv->preg_extra = pcre_study(rv->preg, 0, (const char **) &p);
      pcre_fullinfo(rv->preg, rv->preg_extra, PCRE_INFO_CAPTURECOUNT,
        &rv->pmatch_nr);
      rv->pmatch_nr++;
      rv->pmatch = (int *) _malloc(rv->pmatch_nr * 3 * sizeof(int));
    }
    else
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), mpt);
      xprintf(0, "%s\n", p);
      _free(rv);
      return NULL;
    }
#endif
  }
#endif
  if(str[0] == '(')
  {
    struct lfname_lsp_var *variant;
    p = str;
    if((variant = lfname_lsp_analyze(&p)))
    {
      lfname_lsp_var_free(variant);
      if(*p)
      {
        xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), p);
        lfname_free(rv);
        return NULL;
      }
      else
      {
        rv->transstr = tl_strdup(str);
      }
    }
    else
    {
      lfname_free(rv);
      return NULL;
    }
  }
  else
    rv->transstr = tl_strdup(str);
  rv->matchstr = tl_strdup(mpt);
  return rv;
}

int lfname_match(lfname * lfnamep, const char *urlstr)
{
#ifdef HAVE_REGEX
  if(lfnamep->type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    return !regexec(&(lfnamep->preg), urlstr, lfnamep->preg.re_nsub + 1,
      lfnamep->pmatch, 0);
#elif defined(HAVE_V8_REGEX)
    return regexec(lfnamep->preg, urlstr);
#elif defined(HAVE_BSD_REGEX)
    re_comp(lfnamep->matchstr);
    return re_exec(urlstr);
#elif defined(HAVE_GNU_REGEX)
    return re_match(&(lfnamep->preg), urlstr, strlen(urlstr), 0,
      &lfnamep->pmatch) >= 0;
#elif defined(HAVE_PCRE_REGEX)
    return pcre_exec(lfnamep->preg, lfnamep->preg_extra, urlstr,
      strlen(urlstr), 0, 0, lfnamep->pmatch, 3 * lfnamep->pmatch_nr) >= 0;
#endif
  }
  else
#endif
    return !fnmatch(lfnamep->matchstr, urlstr, 0);
}

int lfname_check_rule(const char *str)
{
  if(str[0] == '(')
  {
    const char *p = str;
    struct lfname_lsp_var *variant;

    if((variant = lfname_lsp_analyze(&p)))
    {
      lfname_lsp_var_free(variant);
      if(*p)
      {
        xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), p);
        return FALSE;
      }
      else
        return TRUE;
    }
    else
      return FALSE;
  }
  return TRUE;
}

int lfname_check_pattern(lfname_type type, const char *str)
{
#ifdef HAVE_REGEX
  if(type == LFNAME_REGEX)
  {
#ifdef HAVE_POSIX_REGEX
    int ec;
    char pom[PATH_MAX];
    regex_t preg;

    ec = regcomp(&preg, str, REG_EXTENDED);

    if(ec)
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
      regerror(ec, &preg, pom, sizeof(pom));
      xprintf(0, "%s\n", pom);
    }
    regfree(&preg);
    return !ec;
#elif defined(HAVE_V8_REGEX)
    regexp *preg;

    preg = regcomp(str);

    if(!preg)
      xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
    else
      free(preg);
    return preg != NULL;
#elif defined(HAVE_BSD_REGEX)
    char *p;

    p = re_comp(str);

    if(p)
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
      xprintf(0, "%s", p);
    }
    return p == NULL;
#elif defined(HAVE_GNU_REGEX)
    char *p;
    struct re_pattern_buffer preg;

    preg.allocated = 0;
    preg.buffer = NULL;
    preg.fastmap = NULL;

    if((p = re_compile_pattern(str, strlen(str), &preg)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
      xprintf(0, "%s\n", p);
    }
    regfree(&preg);
    return p == NULL;
#elif defined(HAVE_PCRE_REGEX)
    pcre *re;
    const char *errmsg = NULL;
    int errcode = 0;

    if(!(re = pcre_compile(str, 0, &errmsg, &errcode, NULL)))
    {
      xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
      xprintf(0, "%s\n", errmsg);
      return -1;
    }
    else
      free(re);

    return re != NULL;
#endif
  }
  else
#endif
    return TRUE;
}

const char *lfname_interp_get_macro(struct lfname_lsp_interp *interp,
int macro)
{
  switch (macro)
  {
  case 'i':
    return interp->scheme;
  case 'p':
    return interp->passwd;
  case 'u':
    return interp->user;
  case 'h':
    return interp->host;
  case 'm':
    return interp->domain;
  case 'r':
    return interp->port;
  case 'd':
    return interp->path;
  case 'n':
    return interp->name;
  case 'b':
    return interp->basename;
  case 'e':
    return interp->extension;
  case 's':
    return interp->query;
  case 'q':
    return interp->post_query;
  case 'U':
    return interp->urlstr;
  case 'o':
    return interp->deflt;
  case 'M':
    return interp->mime_type;
  case 'E':
    return interp->mime_type_ext;
  }
  return NULL;
}

int lfname_check_macro(int macro)
{
  return strchr("ipuhmrdnbesUoqEM", macro) != NULL;
}

static const struct
{
  enum lfname_lsp_type type;
  enum lfname_lsp_type rettype;
  char *name;
  short params;
  enum lfname_lsp_type p1type;
  enum lfname_lsp_type p2type;
  enum lfname_lsp_type p3type;
} lfname_lsp_ftbl[] =
{
  {LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, NULL, 0,
      LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_STR, LF_LSP_STR, NULL, 0,
      LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_NUM, LF_LSP_NUM, NULL, 0,
      LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_MACRO, LF_LSP_STR, NULL, 0,
      LF_LSP_UNKNOWN, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_SUB, LF_LSP_STR, "sp ", 1,
      LF_LSP_NUM, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_SC, LF_LSP_STR, "sc ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_SS, LF_LSP_STR, "ss ", 3, LF_LSP_STR, LF_LSP_NUM, LF_LSP_NUM},
  {LF_LSP_HASH, LF_LSP_NUM, "hsh ", 2, LF_LSP_STR, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_MD5, LF_LSP_STR, "md5 ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_LOWER, LF_LSP_STR, "lo ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_UPPER, LF_LSP_STR, "up ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_UENC, LF_LSP_STR, "ue ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_UDEC, LF_LSP_STR, "ud ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_DELCHR, LF_LSP_STR, "dc ", 2,
      LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_TRCHR, LF_LSP_STR, "tc ", 3, LF_LSP_STR, LF_LSP_STR, LF_LSP_STR},
  {LF_LSP_TRSTR, LF_LSP_STR, "ts ", 3, LF_LSP_STR, LF_LSP_STR, LF_LSP_STR},
  {LF_LSP_STRSPN, LF_LSP_NUM, "spn ", 2,
      LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_STRCSPN, LF_LSP_NUM, "cspn ", 2,
      LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_STRLEN, LF_LSP_NUM, "sl ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_NRSTR, LF_LSP_STR, "ns ", 2, LF_LSP_STR, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_LCHR, LF_LSP_NUM, "lc ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_PLS, LF_LSP_NUM, "+ ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_MNS, LF_LSP_NUM, "- ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_MOD, LF_LSP_NUM, "% ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_MUL, LF_LSP_NUM, "* ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_DIV, LF_LSP_NUM, "/ ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_REMOVEPARAMETER, LF_LSP_STR, "rmpar ", 2,
      LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_GETVALUE, LF_LSP_STR, "getval ", 2,
      LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN},
  {LF_LSP_SIF, LF_LSP_STR, "sif ", 3, LF_LSP_NUM, LF_LSP_STR, LF_LSP_STR},
  {LF_LSP_NOT, LF_LSP_NUM, "! ", 1,
      LF_LSP_NUM, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
  {LF_LSP_AND, LF_LSP_NUM, "& ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_OR, LF_LSP_NUM, "| ", 2, LF_LSP_NUM, LF_LSP_NUM, LF_LSP_UNKNOWN},
  {LF_LSP_GETEXT, LF_LSP_STR, "getext ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
#ifdef HAVE_MOZJS
  {LF_LSP_JSF, LF_LSP_STR, "jsf ", 1,
      LF_LSP_STR, LF_LSP_UNKNOWN, LF_LSP_UNKNOWN},
#endif
  {LF_LSP_SEQ, LF_LSP_NUM, "seq ", 2, LF_LSP_STR, LF_LSP_STR, LF_LSP_UNKNOWN}
};

static enum lfname_lsp_type lfname_lsp_token_type(const char **pstr)
{
  const char *p = *pstr;
  enum lfname_lsp_type retv = LF_LSP_UNKNOWN;

  while(*p == ' ')
    p++;

  if(*p == '(')
  {
    int i;
    for(i = 0; i < NUM_ELEM(lfname_lsp_ftbl); i++)
    {
      if(lfname_lsp_ftbl[i].name &&
        !strncmp(p + 1, lfname_lsp_ftbl[i].name,
          strlen(lfname_lsp_ftbl[i].name)))
      {
        retv = lfname_lsp_ftbl[i].type;
        p += 1 + strlen(lfname_lsp_ftbl[i].name);
        break;
      }
    }
  }
  else if(*p == '\"')
  {
    retv = LF_LSP_STR;
    p++;
  }
  else if(*p == '%')
  {
    retv = LF_LSP_MACRO;
    p++;
  }
  else if(tl_ascii_isdigit(*p) || *p == '-')
    retv = LF_LSP_NUM;

  *pstr = p;

  return retv;
}

static struct lfname_lsp_var *lfname_lsp_var_new(enum lfname_lsp_type type)
{
  struct lfname_lsp_var *retv = NULL;

  retv = _malloc(sizeof(struct lfname_lsp_var));
  retv->type = type;
  retv->val.str = NULL;
  retv->rettype = lfname_lsp_ftbl[type].rettype;
  retv->ret_val.str = NULL;
  retv->param1 = NULL;
  retv->param2 = NULL;
  retv->param3 = NULL;

  return retv;
}

static void lfname_lsp_var_ret_free(struct lfname_lsp_var *var)
{
  if(!var)
    return;

  lfname_lsp_var_ret_free(var->param1);
  lfname_lsp_var_ret_free(var->param2);
  lfname_lsp_var_ret_free(var->param3);
  if(var->rettype == LF_LSP_STR)
    _free(var->ret_val.str);
}

static void lfname_lsp_var_free(struct lfname_lsp_var *var)
{
  if(!var)
    return;

  lfname_lsp_var_free(var->param1);
  lfname_lsp_var_free(var->param2);
  lfname_lsp_var_free(var->param3);
  if(var->type == LF_LSP_STR)
    _free(var->val.str);
  _free(var);
}

static struct lfname_lsp_var *lfname_lsp_analyze(const char **pstr)
{
  struct lfname_lsp_var *retv = NULL;
  enum lfname_lsp_type type;
  char *p;
  const char *cp;

  type = lfname_lsp_token_type(pstr);

  switch (type)
  {
  case LF_LSP_UNKNOWN:
    xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), *pstr);
    break;
  case LF_LSP_NUM:
    {
      int nval;

      errno = 0;
      nval = strtol(*pstr, &p, 0);

      if((errno == ERANGE) || (*p != '\0' && *p != ')' && *p != ' '))
      {
        xprintf(0, gettext("LSP analyze error: bad numeric value at - %s\n"),
          *pstr);
        break;
      }
      retv = lfname_lsp_var_new(type);
      retv->val.num = nval;
      *pstr = p;
    }
    break;
  case LF_LSP_MACRO:
    {
      if(!lfname_check_macro(**pstr) ||
        (*(*pstr + 1) != '\0' && *(*pstr + 1) != ')' && *(*pstr + 1) != ' '))
      {
        xprintf(0, gettext("LSP analyze error: bad macro at - %s\n"),
          *pstr - 1);
        break;
      }
      retv = lfname_lsp_var_new(type);
      retv->val.macro = **pstr;
      *pstr += 1;
    }
    break;
  case LF_LSP_STR:
    {
      char *tmp = _malloc(strlen(*pstr) + 1);
      char *tp;

      cp = *pstr;
      tp = tmp;

      while(*cp)
      {
        if(*cp == '\"')
          break;
        if(*cp == '\\')
          cp++;
        *tp = *cp;
        tp++;
        if(*cp)
          cp++;
      }
      *tp = '\0';

      if(*cp != '\"')
      {
        xprintf(0,
          gettext("LSP analyze error: unterminated string at - %s\n"),
          *pstr - 1);
        break;
      }
      retv = lfname_lsp_var_new(type);
      retv->val.str = tl_strdup(tmp);
      _free(tmp);
      *pstr = cp + 1;
    }
    break;
  default:
    {
      struct lfname_lsp_var *p1 = NULL;
      struct lfname_lsp_var *p2 = NULL;
      struct lfname_lsp_var *p3 = NULL;
      if(lfname_lsp_ftbl[type].params >= 1)
      {
        cp = *pstr;
        p1 = lfname_lsp_analyze(pstr);
        if(!p1)
          break;
        if(p1->rettype != lfname_lsp_ftbl[type].p1type)
        {
          xprintf(0,
            gettext("LSP analyze error: bad parameter type at - %s\n"), cp);
          lfname_lsp_var_free(p1);
          break;
        }
      }
      if(p1 && lfname_lsp_ftbl[type].params >= 2)
      {
        cp = *pstr;
        p2 = lfname_lsp_analyze(pstr);
        if(!p2)
        {
          lfname_lsp_var_free(p1);
          break;
        }
        if(p2->rettype != lfname_lsp_ftbl[type].p2type)
        {
          xprintf(0,
            gettext("LSP analyze error: bad parameter type at - %s\n"), cp);
          lfname_lsp_var_free(p1);
          lfname_lsp_var_free(p2);
          break;
        }
      }
      if(p2 && lfname_lsp_ftbl[type].params >= 3)
      {
        cp = *pstr;
        p3 = lfname_lsp_analyze(pstr);
        if(!p3)
        {
          lfname_lsp_var_free(p1);
          lfname_lsp_var_free(p2);
          break;
        }
        if(p3->rettype != lfname_lsp_ftbl[type].p3type)
        {
          xprintf(0,
            gettext("LSP analyze error: bad parameter type at - %s\n"), cp);
          lfname_lsp_var_free(p1);
          lfname_lsp_var_free(p2);
          lfname_lsp_var_free(p3);
          break;
        }
      }
      while(**pstr == ' ')
        (*pstr)++;
      if(**pstr != ')')
      {
        xprintf(0, gettext("LSP analyze error: bad token at - %s\n"), *pstr);
        if(p1)
          lfname_lsp_var_free(p1);
        if(p2)
          lfname_lsp_var_free(p2);
        if(p3)
          lfname_lsp_var_free(p3);
      }
      else
      {
        (*pstr)++;
        retv = lfname_lsp_var_new(type);
        retv->param1 = p1;
        retv->param2 = p2;
        retv->param3 = p3;
      }
    }
    break;
  }

  return retv;
}

/*
Removes a parameter from an URL-String.
e.g. removeparameter("myurl.php3?var=something","var") will convert the
URL to "myurl.php3?"
*/
static char *lfname_fn_removeparameter(char *urlstr, char *var)
{
  char *p, *found;
  int pos1;
  int parlen;

  /* &var= */
  p = tl_str_concat(NULL, "&", var, "=", NULL);
  parlen = strlen(p);

  found = strstr(urlstr, p);
  if(!found)
  {
    /* ?var= */
    *p = '?';
    found = strstr(urlstr, p);
    if(!found)
    {
      /* var= */
      if((parlen > 1) && !strncmp(urlstr, p + 1, parlen - 1))
      {
        found = urlstr;
      }
    }
  }
  _free(p);

  if(!found)
    return tl_strdup(urlstr);

  pos1 = found - urlstr + 1;

  found = strstr((urlstr + pos1 + 1), "&");
  if(!found)
  {
    return tl_strndup(urlstr, pos1 - 1);
  }

  urlstr = tl_strndup(urlstr, pos1);

  return tl_str_concat(urlstr, found + 1, NULL);
}

/*
reads a value from parameter of an URL-String.
e.g. lfname_fn_getvalue("myurl.php3?var=value","var") results in value
*/
static char *lfname_fn_getvalue(char *urlstr, char *var)
{
  char *p, *found;
  int parlen;

  /* &var= */
  p = tl_str_concat(NULL, "&", var, "=", NULL);
  parlen = strlen(p);

  found = strstr(urlstr, p);
  if(!found)
  {
    /* ?var= */
    *p = '?';
    found = strstr(urlstr, p);
    if(!found)
    {
      /* var= */
      if((parlen > 1) && !strncmp(urlstr, p + 1, parlen - 1))
      {
        parlen--;
        found = urlstr;
      }
    }
  }
  _free(p);

  if(!found)
    return tl_strdup("");

  return tl_strndup(found + parlen, strcspn(found + parlen, "&"));
}

static int lfname_lsp_eval(struct lfname_lsp_interp *interp,
  struct lfname_lsp_var *var)
{
  if(var->param1)
    lfname_lsp_eval(interp, var->param1);
  if(var->param2)
    lfname_lsp_eval(interp, var->param2);
  if(var->param3)
    lfname_lsp_eval(interp, var->param3);

  var->ret_val.str = NULL;

  switch (var->type)
  {
  case LF_LSP_UNKNOWN:
    break;
  case LF_LSP_STR:
    var->ret_val.str = tl_strdup(var->val.str);
    break;
  case LF_LSP_NUM:
    var->ret_val.num = var->val.num;
    break;
  case LF_LSP_MACRO:
    var->ret_val.str =
      tl_strdup(lfname_interp_get_macro(interp, var->val.macro));
    break;
  case LF_LSP_SUB:
#ifdef HAVE_REGEX
    var->ret_val.str = lfname_re_sub(interp->orig,
      interp->urlstr, var->param1->ret_val.num);
#endif
    break;
  case LF_LSP_SC:
    {
      char *p;
      p =
        _malloc(strlen(var->param1->ret_val.str) +
        strlen(var->param2->ret_val.str) + 1);
      strcpy(p, var->param1->ret_val.str);
      strcat(p, var->param2->ret_val.str);
      var->ret_val.str = p;
    }
    break;
  case LF_LSP_SS:
    {
      char *p;
      p = var->param1->ret_val.str;
      if(var->param2->ret_val.num > 0)
        p +=
          (strlen(p) >=
          var->param2->ret_val.num) ? var->param2->ret_val.num : strlen(p);
      if(var->param3->ret_val.num > 0)
        var->ret_val.str = tl_strndup(p, var->param3->ret_val.num);
      else
        var->ret_val.str = tl_strdup(p);
    }
    break;
  case LF_LSP_HASH:
    var->ret_val.num = str_hash_func(var->param2->ret_val.num,
      (dllist_t) var->param1->ret_val.str);
    break;
  case LF_LSP_MD5:
    var->ret_val.str = _md5(var->param1->ret_val.str);
    break;
  case LF_LSP_LOWER:
    var->ret_val.str = lowerstr(var->param1->ret_val.str);
    break;
  case LF_LSP_UPPER:
    var->ret_val.str = upperstr(var->param1->ret_val.str);
    break;
  case LF_LSP_UENC:
    var->ret_val.str = url_encode_str(var->param1->ret_val.str,
      var->param2->ret_val.str);
    break;
  case LF_LSP_UDEC:
    var->ret_val.str = url_decode_str(var->param1->ret_val.str,
      strlen(var->param1->ret_val.str));
    break;
  case LF_LSP_DELCHR:
    var->ret_val.str = tr_del_chr(var->param2->ret_val.str,
      var->param1->ret_val.str);
    break;
  case LF_LSP_TRCHR:
    var->ret_val.str = tr_chr_chr(var->param2->ret_val.str,
      var->param3->ret_val.str, var->param1->ret_val.str);
    break;
  case LF_LSP_TRSTR:
    var->ret_val.str = tr_str_str(var->param2->ret_val.str,
      var->param3->ret_val.str, var->param1->ret_val.str);
    break;
  case LF_LSP_STRSPN:
    var->ret_val.num = strspn(var->param1->ret_val.str,
      var->param2->ret_val.str);
    break;
  case LF_LSP_STRCSPN:
    var->ret_val.num = strcspn(var->param1->ret_val.str,
      var->param2->ret_val.str);
    break;
  case LF_LSP_STRLEN:
    var->ret_val.num = strlen(var->param1->ret_val.str);
    break;
  case LF_LSP_NRSTR:
    {
      char pom[1024];
      snprintf(pom, sizeof(pom), var->param1->ret_val.str, var->param2->ret_val.num);
      var->ret_val.str = tl_strdup(pom);
    }
    break;
  case LF_LSP_LCHR:
    {
      char *p;
      p = strrchr(var->param1->ret_val.str, *var->param2->ret_val.str);
      var->ret_val.num = p ? p - var->param1->ret_val.str : 0;
    }
    break;
  case LF_LSP_PLS:
    var->ret_val.num = var->param1->ret_val.num + var->param2->ret_val.num;
    break;
  case LF_LSP_MNS:
    var->ret_val.num = var->param1->ret_val.num - var->param2->ret_val.num;
    break;
  case LF_LSP_MOD:
    var->ret_val.num = var->param1->ret_val.num % var->param2->ret_val.num;
    break;
  case LF_LSP_MUL:
    var->ret_val.num = var->param1->ret_val.num * var->param2->ret_val.num;
    break;
  case LF_LSP_DIV:
    var->ret_val.num = var->param1->ret_val.num / var->param2->ret_val.num;
    break;
  case LF_LSP_REMOVEPARAMETER:
    var->ret_val.str =
      lfname_fn_removeparameter(var->param1->ret_val.str,
      var->param2->ret_val.str);
    break;
  case LF_LSP_GETVALUE:
    var->ret_val.str =
      lfname_fn_getvalue(var->param1->ret_val.str, var->param2->ret_val.str);
    break;
  case LF_LSP_SIF:
    var->ret_val.str = var->param1->ret_val.num ?
      tl_strdup(var->param2->ret_val.str) :
      tl_strdup(var->param3->ret_val.str);
    break;
  case LF_LSP_NOT:
    var->ret_val.num = !var->param1->ret_val.num;
    break;
  case LF_LSP_AND:
    var->ret_val.num = var->param1->ret_val.num && var->param2->ret_val.num;
    break;
  case LF_LSP_OR:
    var->ret_val.num = var->param1->ret_val.num || var->param2->ret_val.num;
    break;
  case LF_LSP_GETEXT:
    var->ret_val.str = tl_strdup(tl_get_extension(var->param1->ret_val.str));
    break;
  case LF_LSP_SEQ:
    var->ret_val.num = !strcmp(var->param1->ret_val.str,
      var->param2->ret_val.str);
    break;
#ifdef HAVE_MOZJS
  case LF_LSP_JSF:
    var->ret_val.str = pjs_run_fnrules_func(var->param1->ret_val.str, interp);
    break;
#endif
  }
  if(var->rettype == LF_LSP_STR && !var->ret_val.str)
    var->ret_val.str = tl_strdup("");

  return 0;
}

static char *lfname_lsp_get_by_url(struct lfname_lsp_interp *interp)
{
  char *retv = NULL;
  struct lfname_lsp_var *variant;
  const char *p;
  p = interp->orig->transstr;

  variant = lfname_lsp_analyze(&p);

  if(variant)
  {
    lfname_lsp_eval(interp, variant);
    if(variant->rettype == LF_LSP_NUM)
    {
      char nr[10];
      sprintf(nr, "%d", variant->ret_val.num);
      retv = tl_strdup(nr);
    }
    else
    {
      retv = tl_strdup(variant->ret_val.str);
    }
    lfname_lsp_var_free(variant);
  }

  return retv;
}
