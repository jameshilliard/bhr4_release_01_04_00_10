/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "mopt.h"
#include "dlhash.h"
#include "tools.h"

static unsigned int mopt_long_hash_func(unsigned int size, dllist_t key)
{
  unsigned char *p = (unsigned char *) key;
  unsigned int retv = 0;

  while(*p)
  {
    retv = (retv + tl_ascii_tolower(*p)) % size;
    p++;
  }

  return retv;
}

static unsigned int mopt_short_hash_func(unsigned int size, dllist_t key)
{
  unsigned char *p = (unsigned char *) key;
  unsigned int retv = 0;

  while(*p)
  {
    retv = (retv + *p) % size;
    p++;
  }

  return retv;
}

static int mopt_long_comp_func(dllist_t key1, dllist_t key2)
{
  return (!strcasecmp((void *) key1, (void *) key2));
}

static int mopt_short_comp_func(dllist_t key1, dllist_t key2)
{
  return (!strcmp((void *) key1, (void *) key2));
}

static dllist_t mopt_short_key_func(dllist_t data)
{
  return (dllist_t) ((cfg_param_t *) data)->short_cmd;
}

static dllist_t mopt_long_key_func(dllist_t data)
{
  return (dllist_t) ((cfg_param_t *) data)->long_cmd;
}

void mopt_init(mopt_t * mopt, int nparams, cfg_param_t * params, int argc,
  char **argv)
{
  int s_size, l_size;
  int i;

  l_size = 42;
  s_size = 10;
  memset(mopt, '\0', sizeof(mopt_t));

  mopt->argc = argc;
  mopt->argv = argv;
  mopt->nparams = nparams;
  mopt->params = params;
  mopt->option_type = MOPT_OPT_NONE;

  mopt->short_hash = dlhash_new(s_size, mopt_short_key_func,
    mopt_short_hash_func, mopt_short_comp_func);

  mopt->long_hash = dlhash_new(l_size, mopt_long_key_func,
    mopt_long_hash_func, mopt_long_comp_func);

  for(i = 0; i < nparams; i++)
  {
    if(params[i].short_cmd)
      dlhash_insert(mopt->short_hash, (dllist_t) &params[i]);
    if(params[i].long_cmd)
      dlhash_insert(mopt->long_hash, (dllist_t) &params[i]);
  }
  mopt->short_offset = 0;
  mopt->current = 0;
}

void mopt_destroy(mopt_t * mopt)
{
  dlhash_free(mopt->short_hash);
  dlhash_free(mopt->long_hash);
  memset(mopt, '\0', sizeof(mopt_t));
}

/*
   on success     -  0
   on end     -  1
   on no option param   -  2
   on bad number of params  - -1
   on unknown param   - -2
   on wrong formated param  - -3
 */
int mopt_get_next_param(mopt_t * mopt, cfg_param_t ** pparam)
{
  cfg_param_t *param = NULL;
  char *cpar;
  int i, np;

  if(!mopt->current)
  {
    if(mopt->argc == 1)
      return MOPT_END;
    else
      mopt->current = 1;
  }

  if(mopt->argc <= mopt->current)
    return MOPT_END;

  cpar = mopt->argv[mopt->current];

  if(mopt->short_offset)
  {
    mopt->option_type = MOPT_OPT_SHORT;

    cpar += mopt->short_offset;

    if(mopt->short_offset == 1)
    {
      param = (cfg_param_t *) dlhash_find_by_key(mopt->short_hash,
      (dllist_t) cpar);
    }

    if(param)
    {
      mopt->short_offset = 0;
      mopt->current++;
      cpar = NULL;
    }
    else
    {
      char pom[2];

      pom[0] = *cpar;
      pom[1] = '\0';

      param = (cfg_param_t *) dlhash_find_by_key(mopt->short_hash,
      (dllist_t) pom);

      if(param)
      {
        mopt->short_offset++;
        cpar++;
        if(!*cpar)
        {
          cpar = NULL;
          mopt->short_offset = 0;
          mopt->current++;
        }
      }
    }

    if(!param)
    {
      if(mopt->short_offset == 1)
        return MOPT_UNKNOWN;
      else
        return MOPT_BAD;
    }

    np = cfg_get_num_params(param);

    *pparam = param;

    if(np + mopt->current > mopt->argc)
      return MOPT_MISSINGP;

    if(np)
    {
      if(cpar)
      {
        mopt->args[0] = cpar;
        mopt->short_offset = 0;
        mopt->current++;
      }

      for(i = (cpar ? 1 : 0); i < np; i++)
        mopt->args[i] = mopt->argv[mopt->current + i];

      mopt->current += np - (cpar ? 1 : 0);
    }
    return MOPT_OK;
  }
  else
  {
    param = (cfg_param_t *) dlhash_find_by_key(mopt->long_hash, (dllist_t)
    (cpar + ((cpar[0] == '-' && cpar[1] == '-') ? 1 : 0)));

    if(param)
    {
      mopt->option_type = MOPT_OPT_LONG;

      np = cfg_get_num_params(param);

      mopt->current++;

      *pparam = param;

      if(np + mopt->current > mopt->argc)
        return MOPT_MISSINGP;

      for(i = 0; i < np; i++)
        mopt->args[i] = mopt->argv[mopt->current + i];

      mopt->current += np;


      return MOPT_OK;
    }
    else if(cpar[0] == '-' && cpar[1] == '-' && strchr(cpar, '='))
    {
      char *tpar;
      char *p;

      mopt->option_type = MOPT_OPT_LONG;

      p = strchr(cpar, '=');

      tpar = tl_strndup(cpar, p - cpar);

      param = (cfg_param_t *) dlhash_find_by_key(mopt->long_hash,
      (dllist_t) (tpar + 1));

      _free(tpar);

      if(param)
      {
        cpar = p + 1;

        np = cfg_get_num_params(param);

        *pparam = param;

        if(np + mopt->current - 1 > mopt->argc)
          return MOPT_MISSINGP;

        if(!np)
          return MOPT_BAD;
        else
        {
          mopt->args[0] = cpar;
          mopt->short_offset = 0;
          mopt->current++;

          for(i = 1; i < np; i++)
            mopt->args[i] = mopt->argv[mopt->current + i];

          mopt->current += np - 1;
        }

        return MOPT_OK;

      }
    }

    if(!param)
    {
      if(cpar[0] == '-' && cpar[1] == '-')
      {
        return MOPT_UNKNOWN;
      }
      if(*cpar == '-')
      {
        mopt->short_offset = 1;
        return mopt_get_next_param(mopt, pparam);
      }
      else
      {
        mopt->args[0] = cpar;
        mopt->current++;
        return MOPT_PARAM;
      }
    }
  }
  return MOPT_ERR;
}
