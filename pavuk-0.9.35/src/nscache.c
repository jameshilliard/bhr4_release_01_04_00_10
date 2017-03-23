/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"

#ifdef HAVE_BDB_18x


#ifdef HAVE_DB1_H
#include <db1/db.h>
#elif defined(HAVE_DB185_H)
#include <db_185.h>
#elif defined(HAVE_DB2_DB185_H)
#include <db2/db_185.h>
#elif defined(HAVE_DB3_DB185_H)
#include <db3/db_185.h>
#elif defined(HAVE_DB4_DB185_H)
#include <db4/db_185.h>
#else
#include <db.h>
#endif

typedef int int32__t;
typedef unsigned int uint32__t;

#ifdef WORDS_BIGENDIAN
#define COPY_INT32(i1,i2) \
        ((char *)(i1))[0] = ((char *)(i2))[3];\
        ((char *)(i1))[1] = ((char *)(i2))[2];\
        ((char *)(i1))[2] = ((char *)(i2))[1];\
        ((char *)(i1))[3] = ((char *)(i2))[0];
#else
#define COPY_INT32(i1,i2)  *((int32__t *) i1) = *((int32__t *) i2);
#endif

static char *ns_cache_filename = NULL;
static DB *ns_cache_db = NULL;
static time_t ns_cache_db_modtime = 0L;

static void ns_cache_open_db(char *dirname)
{
  bool_t reopen = FALSE;
  HASHINFO hash_info = {
    4 * 1024,                   /* bucket size */
    0,                          /* fill factor */
    0,                          /* number of elements */
    96 * 1024,                  /* bytes to cache */
    0,                          /* hash function */
    0
  };                            /* byte order */
  char pom[PATH_MAX];


  LOCK_NSCACHE;
  if(dirname)
  {
    struct stat estat;

    snprintf(pom, sizeof(pom), "%s/index.db", dirname);

    if(!ns_cache_filename || strcmp(pom, ns_cache_filename))
      reopen = TRUE;

    if(!reopen && !stat(pom, &estat) && estat.st_mtime != ns_cache_db_modtime)
      reopen = TRUE;
  }
  else
  {
    _free(ns_cache_filename);
    if(ns_cache_db)
      ns_cache_db->close(ns_cache_db);
    ns_cache_db = NULL;
    ns_cache_db_modtime = 0L;
  }

  if(reopen && ns_cache_db)
  {
    _free(ns_cache_filename);
    if(ns_cache_db)
      ns_cache_db->close(ns_cache_db);
    ns_cache_db = NULL;
    ns_cache_db_modtime = 0L;
  }

  if(reopen)
  {
    struct stat estat;

    ns_cache_db = dbopen(pom, O_RDONLY, 0600, DB_HASH, &hash_info);

    if(!ns_cache_db)
    {
      xprintf(0, gettext("Unable to open Netscape cache index - %s\n"), pom);
    }

    if(!stat(pom, &estat))
      ns_cache_db_modtime = estat.st_mtime;

    _free(ns_cache_filename);
    ns_cache_filename = tl_strdup(pom);
  }
  UNLOCK_NSCACHE;
}

static void ns_cache_gen_key(DBT *key, char *urlstr)
{
  int32__t size, len;
  char *tmp;

  size = sizeof(int32__t);      /* check sum */
  size += sizeof(int32__t);     /* size of urlstr */
  size += strlen(urlstr) + 1;   /* urlstr */
  size += sizeof(int32__t);     /* post data */

  key->size = size;
  key->data = (void *) malloc(size);

  tmp = key->data;

  COPY_INT32(tmp, &size);
  tmp += sizeof(int32__t);

  len = strlen(urlstr) + 1;
  COPY_INT32(tmp, &len);
  tmp += sizeof(int32__t);

  strcpy(tmp, urlstr);
  tmp += strlen(urlstr) + 1;

  len = 0;
  COPY_INT32(tmp, &len);
  tmp += sizeof(int32__t);
}

static char *ns_cache_get_filename(DBT *data)
{
  char *tmp = data->data;

  tmp += sizeof(int32__t)       /* size */
    + sizeof(int32__t)          /* version */
    + sizeof(time_t)            /* last_modified */
    + sizeof(time_t)            /* last_accessed */
    + sizeof(time_t)            /* expires */
    + sizeof(uint32__t)         /* content_length */
    + sizeof(bool_t)            /* is_netsite */
    + sizeof(time_t)            /* lock_date */
    + sizeof(int32__t)          /* size of filename */
    ;

  return tmp;
}

char *ns_cache_find_localname(char *urlstr)
{
  DBT key, data;
  char *retv = NULL;

  ns_cache_open_db(priv_cfg.ns_cache_dir);

  LOCK_NSCACHE;
  if(ns_cache_db)
  {
    ns_cache_gen_key(&key, urlstr);

    if(!ns_cache_db->get(ns_cache_db, &key, &data, 0))
    {
      retv = tl_str_concat(NULL, cfg.ns_cache_dir, "/",
        ns_cache_get_filename(&data), NULL);
    }
    _free(key.data);
  }
  UNLOCK_NSCACHE;

  return retv;
}

#endif
