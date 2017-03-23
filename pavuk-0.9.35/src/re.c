/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#ifdef HAVE_REGEX

#include "re.h"
#include "tools.h"

void re_free(re_entry * ree)
{
#ifdef HAVE_POSIX_REGEX
  regfree(&(ree->preg));
#endif
#ifdef HAVE_V8_REGEX
  _free(ree->preg);
#endif
#ifdef HAVE_GNU_REGEX
  regfree(&(ree->preg));
#endif
#ifdef HAVE_PCRE_REGEX
  _free(ree->preg);
  _free(ree->preg_extra);
#endif
  _free(ree->pattern);
  _free(ree);
}

re_entry *re_make(const char *str)
{
  int ec;
  re_entry *rv;
#if defined(HAVE_BSD_REGEX) || defined(HAVE_GNU_REGEX) || defined(HAVE_PCRE_REGEX)
  char *p;
#endif

  rv = _malloc(sizeof(re_entry));

#ifdef HAVE_POSIX_REGEX
  if((ec = regcomp(&(rv->preg), str, REG_EXTENDED)))
  {
    char pom[PATH_MAX];

    xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
    regerror(ec, &(rv->preg), pom, sizeof(pom));
    xprintf(0, "%s\n", pom);
    regfree(&(rv->preg));
    _free(rv);
  }
#endif
#ifdef HAVE_V8_REGEX
  if(!(rv->preg = regcomp(str)))
  {
    xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
    _free(rv);
  }
#endif
#ifdef HAVE_BSD_REGEX
  if((p = re_comp(str)))
  {
    xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
    xprintf(0, "%s", p);
    _free(rv);
  }
#endif
#ifdef HAVE_GNU_REGEX
  rv->preg.allocated = 0;
  rv->preg.buffer = NULL;
  rv->preg.fastmap = NULL;

  if((p = re_compile_pattern(str, strlen(str), &rv->preg)))
  {
    xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
    xprintf(0, "%s\n", p);
    regfree(&(rv->preg));
    _free(rv);
  }

#endif
#ifdef HAVE_PCRE_REGEX
  if((rv->preg = pcre_compile(str, 0, (const char **) &p, &ec, NULL)))
  {
    rv->preg_extra = pcre_study(rv->preg, 0, (const char **) &p);
  }

  if(!rv->preg)
  {
    xprintf(0, gettext("Error compiling regular expression : %s\n"), str);
    xprintf(0, "%s\n", p);
    _free(rv);
  }
#endif
  else
    rv->pattern = new_string(str);

  return rv;
}

int re_pmatch(re_entry * ree, char *str)
{
#ifdef HAVE_POSIX_REGEX
  return !regexec(&(ree->preg), str, 0, NULL, 0);
#endif
#ifdef HAVE_V8_REGEX
  return regexec(ree->preg, str);
#endif
#ifdef HAVE_BSD_REGEX
  re_comp(ree->pattern);
  return re_exec(str);
#endif
#ifdef HAVE_GNU_REGEX
  return re_match(&(ree->preg), str, strlen(str), 0, NULL) >= 0;
#endif
#ifdef HAVE_PCRE_REGEX
  return pcre_exec(ree->preg, ree->preg_extra, str, strlen(str), 0, 0, NULL,
    0) >= 0;
#endif
}

int re_pmatch_sub(re_entry * ree, char *str, int nr, int *start, int *end)
{
  int rv = FALSE;

  if(rv < 0)
    return FALSE;

#ifdef HAVE_POSIX_REGEX
  if(nr < (ree->preg.re_nsub + 1))
  {
    regmatch_t *pmatch;

    pmatch = _malloc((ree->preg.re_nsub + 1) * sizeof(regmatch_t));

    rv = !regexec(&(ree->preg), str, ree->preg.re_nsub + 1, pmatch, 0);

    if(rv)
    {
      *start = pmatch[nr].rm_so;
      *end = pmatch[nr].rm_eo;
    }

    _free(pmatch);
  }
#endif
#ifdef HAVE_V8_REGEX
  /* not supported ! */
  rv = FALSE;
#endif
#ifdef HAVE_BSD_REGEX
  /* not supported ! */
  rv = FALSE;
#endif
#ifdef HAVE_GNU_REGEX
  if(nr < lfnamep->preg.re_nsub)
  {
    struct re_registers pmatch;
    int sra;

    pmatch.start = _malloc((ree->preg.re_nsub + 1) * sizeof(*pmatch.start));
    pmatch.end = _malloc((ree->preg.re_nsub + 1) * sizeof(*pmatch.end));
    ree->pmatch.num_regs = rv->preg.re_nsub + 1;
    sra = ree->preg.regs_allocated;
    ree->preg.regs_allocated = REGS_FIXED;

    rv = re_match(&(ree->preg), str, strlen(str), 0, &pmatch) >= 0;

    if(rv)
    {
      *start = pmatch.start[nr];
      *end = pmatch.end[nr];
    }
    ree->preg.regs_allocated = sra;
    _free(pmatch.start);
    _free(pmatch.end);
  }
#endif
#ifdef HAVE_PCRE_REGEX
  {
    int *pmatch;
    int pmatch_nr;

    pcre_fullinfo(ree->preg, ree->preg_extra, PCRE_INFO_CAPTURECOUNT,
      &pmatch_nr);

    if(pmatch_nr >= nr)
    {
      pmatch = (int *) _malloc((pmatch_nr + 1) * 3 * sizeof(int));
      rv =
        pcre_exec(ree->preg, ree->preg_extra, str, strlen(str), 0, 0, pmatch,
        (pmatch_nr + 1) * 3) >= 0;

      if(rv)
      {
        *start = pmatch[2 * nr];
        *end = pmatch[2 * nr + 1];
      }
      _free(pmatch);
    }
  }
#endif

  return rv;
}

int re_pmatch_subs(re_entry * ree, char *str, int *nr, int **subs)
{
  int rv = FALSE;

  if(rv < 0)
    return FALSE;

#ifdef HAVE_POSIX_REGEX
  {
    regmatch_t *pmatch;

    pmatch = _malloc((ree->preg.re_nsub + 1) * sizeof(regmatch_t));

    rv = !regexec(&(ree->preg), str, ree->preg.re_nsub + 1, pmatch, 0);

    if(rv)
    {
      *subs = (int *) _malloc((ree->preg.re_nsub + 1) * 2 * sizeof(int));
      for(*nr = 0; *nr < (ree->preg.re_nsub + 1); (*nr)++)
      {
        (*subs)[2 * (*nr)] = pmatch[*nr].rm_so;
        (*subs)[2 * (*nr) + 1] = pmatch[*nr].rm_eo;
      }
    }

    _free(pmatch);
  }
#endif
#ifdef HAVE_V8_REGEX
  /* not supported ! */
  rv = FALSE;
#endif
#ifdef HAVE_BSD_REGEX
  /* not supported ! */
  rv = FALSE;
#endif
#ifdef HAVE_GNU_REGEX
  {
    struct re_registers pmatch;
    int sra;

    pmatch.start = _malloc((ree->preg.re_nsub + 1) * sizeof(*pmatch.start));
    pmatch.end = _malloc((ree->preg.re_nsub + 1) * sizeof(*pmatch.end));
    ree->pmatch.num_regs = rv->preg.re_nsub + 1;
    sra = ree->preg.regs_allocated;
    ree->preg.regs_allocated = REGS_FIXED;

    rv = re_match(&(ree->preg), str, strlen(str), 0, &pmatch) >= 0;

    if(rv)
    {
      *subs = (int *) _malloc((ree->preg.re_nsub + 1) * 2 * sizeof(int));
      for(*nr = 0; *nr < (ree->preg.re_nsub + 1); (*nr)++)
      {
        (*subs)[2 * (*nr)] = pmatch.start[*nr];
        (*subs)[2 * (*nr) + 1] = pmatch.end[*nr];
      }
    }

    ree->preg.regs_allocated = sra;
    _free(pmatch.start);
    _free(pmatch.end);
  }
#endif
#ifdef HAVE_PCRE_REGEX
  {
    int *pmatch;
    int pmatch_nr;

    pcre_fullinfo(ree->preg, ree->preg_extra, PCRE_INFO_CAPTURECOUNT,
      &pmatch_nr);
    pmatch = (int *) _malloc((pmatch_nr + 1) * 3 * sizeof(int));
    rv =
      pcre_exec(ree->preg, ree->preg_extra, str, strlen(str), 0, 0, pmatch,
      (pmatch_nr + 1) * 3) >= 0;

    if(rv)
    {
      *subs = (int *) _malloc((pmatch_nr + 1) * 2 * sizeof(int));
      for(*nr = 0; *nr < (pmatch_nr + 1); (*nr)++)
      {
        (*subs)[2 * (*nr)] = pmatch[2 * (*nr)];
        (*subs)[2 * (*nr) + 1] = pmatch[2 * (*nr) + 1];
      }
    }

    _free(pmatch);
  }
#endif

  return rv;
}

#endif
