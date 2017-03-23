/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "dlhash.h"
#include "dlhash_tools.h"
#include "mozcache.h"
#include "tools.h"

typedef int int32__t;
typedef unsigned int uint32__t;

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

#ifdef WORDS_BIGENDIAN
#define COPY_INT32(i1,i2) \
  ((char *)(i1))[0] = ((char *)(i2))[3]; \
  ((char *)(i1))[1] = ((char *)(i2))[2]; \
  ((char *)(i1))[2] = ((char *)(i2))[1]; \
  ((char *)(i1))[3] = ((char *)(i2))[0];
#else
#define COPY_INT32(i1,i2)  *((int32__t *) i1) = *((int32__t *) i2);
#endif
#endif /* HAVE_BDB_18x */

static char *moz_cache_filename = NULL;
static time_t moz_cache_db_modtime = 0L;
static dlhash *moz_hash = NULL;

typedef struct
{
  char *urlstr;
  char *filename;
} moz_pair_t;

static moz_pair_t *moz_pair_new(char *f, char *u)
{
  moz_pair_t *rv = NULL;

  if(f && u)
  {
    rv = _malloc(sizeof(moz_pair_t));
    rv->filename = tl_strdup(f);
    rv->urlstr = tl_strdup(u);
  }

  return rv;
}

static void moz_pair_free(dllist_t dl)
{
  moz_pair_t * mp = (moz_pair_t *)dl;
  _free(mp->filename);
  _free(mp->urlstr);
  _free(mp);
}

static dllist_t moz_pair_get_urlstr(dllist_t mp)
{
  return (dllist_t) ((moz_pair_t *)mp)->urlstr;
}

#ifdef HAVE_BDB_18x
static char *moz_old_cache_get_filename(DBT * data)
{
  char *tmp = data->data;
  uint32__t size;

  COPY_INT32(&size, tmp);
  if(size != data->size || data->size < 20)
    return NULL;

  tmp += sizeof(uint32__t)      /* size */
    + sizeof(int32__t);         /* recordid */

  COPY_INT32(&size, tmp);
  tmp += sizeof(uint32__t) + size;      /* key_len + key */

  COPY_INT32(&size, tmp);
  tmp += sizeof(uint32__t) + size;      /* meta_len + meta */

  tmp += sizeof(uint32__t);     /* filename_len */

  return tmp;
}

static char *moz_old_cache_get_urlstr(DBT * data)
{
  char *tmp = data->data;
  uint32__t size;

  COPY_INT32(&size, tmp);
  if(size != data->size || data->size < 20)
    return NULL;

  tmp += sizeof(uint32__t)      /* size */
    + sizeof(int32__t);         /* recordid */

  COPY_INT32(&size, tmp);
  tmp += sizeof(uint32__t);     /* key_len */

  return tmp;
}

static void moz_old_cache_read_db(char *dirname)
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

  if(dirname)
  {
    struct stat estat;

    snprintf(pom, sizeof(pom), "%s/cache.db", dirname);

    if(!moz_cache_filename || strcmp(pom, moz_cache_filename))
      reopen = TRUE;

    if(!reopen && !stat(pom, &estat) &&
      estat.st_mtime != moz_cache_db_modtime)
      reopen = TRUE;
  }

  if(reopen || !dirname)
  {
    _free(moz_cache_filename);
    moz_cache_db_modtime = 0L;
    dlhash_empty(moz_hash);
  }

  if(reopen)
  {
    struct stat estat;
    DB *moz_cache_db;
    DBT key, data;

    moz_cache_db = dbopen(pom, O_RDONLY, 0600, DB_HASH, &hash_info);

    if(!moz_cache_db)
    {
      xprintf(0, gettext("Unable to open Mozilla cache index - %s\n"), pom);
    }
    else
    {
      if(!stat(pom, &estat))
        moz_cache_db_modtime = estat.st_mtime;

      _free(moz_cache_filename);
      moz_cache_filename = tl_strdup(pom);

      while(!moz_cache_db->seq(moz_cache_db, &key, &data, R_NEXT))
      {
        moz_pair_t *mp;

        mp = moz_pair_new(moz_old_cache_get_filename(&data),
          moz_old_cache_get_urlstr(&data));

        if(mp)
          dlhash_insert(moz_hash, mp);
      }

      moz_cache_db->close(moz_cache_db);
    }
  }
}
#endif /* HAVE_BDB_18x */

#define MOZ2_REC_PER_BUCKET 256
#define MOZ2_REC_SIZE   16
#define MOZ2_BITMAP_SIZE  4096

typedef struct
{
  uint32__t version;
  int32__t size;
  int32__t nentries;
  uint32__t dirty;
} moz2_cache_map_head;

typedef struct
{
  uint32__t hashnr;
  uint32__t erank;
  uint32__t data_loc;
  uint32__t meta_loc;
} moz2_cache_map_record;

typedef struct
{
  uint32__t version;
  uint32__t meta_loc;
  uint32__t fetch_count;
  uint32__t last_fetched;
  uint32__t last_modified;
  uint32__t expiration;
  uint32__t data_size;
  uint32__t key_size;
  uint32__t meta_size;
} moz2_cache_record_header;

static char *moz2_db_dir = NULL;

static struct
{
  enum
  {
    MOZ2_MAP_FILE,
    MOZ2_B1_FILE,
    MOZ2_B2_FILE,
    MOZ2_B3_FILE,
    MOZ2_LAST
  } id;
  int fd;
  int block_size;
  char *name;
} moz2_files[] =
{
  {MOZ2_MAP_FILE, -1, 0, "_CACHE_MAP_"},
  {MOZ2_B1_FILE, -1, 256, "_CACHE_001_"},
  {MOZ2_B2_FILE, -1, 1024, "_CACHE_002_"},
  {MOZ2_B3_FILE, -1, 4096, "_CACHE_003_"}
};

#define MOZ2_LOC_INITED(v)     ((v) & 0x80000000)
#define MOZ2_LOC_FILE(v)      (((v) & 0x30000000) >> 28)
#define MOZ2_LOC_BLOCKS(v)    (((v) & 0x03000000) >> 24)
#define MOZ2_LOC_BLOCKNR(v)    ((v) & 0x00FFFFFF)
#define MOZ2_LOC_GENERATION(v) ((v) & 0xFF)

static int moz2_open_db(char *name)
{
  char *p;
  char pom[PATH_MAX];
  int i, len, rv = 0;

  if(!name)
    return -1;

  strncpy(pom, name, PATH_MAX);
  pom[PATH_MAX - 1] = '\0';
  p = pom + strlen(pom);
  len = PATH_MAX - strlen(pom);

  for(i = 0; i < MOZ2_LAST; i++)
  {
    strncpy(p, moz2_files[i].name, len);
    pom[PATH_MAX - 1] = '\0';

    moz2_files[i].fd = open(pom, O_RDWR);
    if(moz2_files[i].fd < 0)
      moz2_files[i].fd = open(pom, O_RDONLY);

    if(moz2_files[i].fd < 0)
    {
      xprintf(1, gettext("Error opening cache index file %s\n"));
      xperror(pom);
      rv = -1;
      break;
    }
  }

  if(!rv)
  {
    char bucket[MOZ2_REC_SIZE * MOZ2_REC_PER_BUCKET];
    moz2_cache_map_head *cmh;

    cmh = (moz2_cache_map_head *) bucket;

    if((read(moz2_files[MOZ2_MAP_FILE].fd, bucket, sizeof(bucket))
        != sizeof(bucket)) || (ntohl(cmh->version) != 0x00010003))
    {
      xprintf(1, gettext("Mozilla new cache map file format not recognized"));
      rv = -1;
    }
  }

  if(!rv)
  {
    *p = '\0';
    moz2_db_dir = tl_strdup(pom);
  }

  return rv;
}

static void moz2_close_db(void)
{
  int i;
  for(i = 0; i < MOZ2_LAST; i++)
  {
    if(moz2_files[i].fd >= 0)
    {
      close(moz2_files[i].fd);
      moz2_files[i].fd = -1;
    }
  }
}

static char *moz2_get_filename(int type, uint32__t hashnr,
  uint32__t generation)
{
  char pom[PATH_MAX];

  snprintf(pom, sizeof(pom), "%s%08X%c%02X", moz2_db_dir, hashnr, (char) type, generation);

  return tl_strdup(pom);
}

static char *load_blocks(int id, uint32__t blocknr, uint32__t blocks)
{
  int ofs, sz;
  char *rv;

  ofs = MOZ2_BITMAP_SIZE + moz2_files[id].block_size * blocknr;

  if(lseek(moz2_files[id].fd, ofs, SEEK_SET) != ofs)
  {
    xprintf(1, gettext("Corrupted Mozilla cache blockfile!"));
    return NULL;
  }

  sz = moz2_files[id].block_size * (blocks + 1);

  rv = _malloc(sz);

  if(read(moz2_files[id].fd, rv, sz) != sz)
  {
    xprintf(1, gettext("Corrupted Mozilla cache blockfile!"));
    _free(rv);
    return NULL;
  }

  return rv;
}

static moz2_cache_record_header *moz2_load_record_data(moz2_cache_map_record *
  rec)
{
  char *p;

  if(!MOZ2_LOC_FILE(rec->meta_loc))
  {
    /* load from file */
    char *fn;

    fn = moz2_get_filename('m', rec->hashnr,
      MOZ2_LOC_GENERATION(rec->meta_loc));

    p = tl_load_text_file(fn);

    _free(fn);
  }
  else
  {
    p = load_blocks(moz2_files[MOZ2_LOC_FILE(rec->meta_loc)].id,
      MOZ2_LOC_BLOCKNR(rec->meta_loc), MOZ2_LOC_BLOCKS(rec->meta_loc));
  }

  return (moz2_cache_record_header *) p;
}

static moz_pair_t *moz2_compose_record(moz2_cache_map_record * rec,
  moz2_cache_record_header * meta)
{
  moz_pair_t *ret = NULL;
  char *u, *p;

  p = moz2_get_filename('d', rec->hashnr, MOZ2_LOC_GENERATION(rec->data_loc));

  u = (char *) meta;
  u += sizeof(moz2_cache_record_header);
  u = strchr(u, ':') + 1;

  ret = moz_pair_new(p, u);

  return ret;
}

static moz_pair_t *moz2_get_record_info(moz2_cache_map_record * rec)
{
  moz2_cache_record_header *meta;
  moz_pair_t *rv = NULL;

  if(!rec->hashnr)
    return NULL;

  if(!MOZ2_LOC_INITED(rec->data_loc) || !MOZ2_LOC_INITED(rec->meta_loc))
    return NULL;

  /* don't know how to handle data file in block files */
  if(MOZ2_LOC_FILE(rec->data_loc))
    return NULL;

  meta = moz2_load_record_data(rec);

  if(meta && (ntohl(meta->version) == 0x00010003))
    rv = moz2_compose_record(rec, meta);

  _free(meta);

  return rv;
}

static void moz2_read_db(char *dbname)
{
  int l, i;
  char bucket[MOZ2_REC_SIZE * MOZ2_REC_PER_BUCKET];
  char *p;

  if(moz2_open_db(dbname))
    return;

  lseek(moz2_files[MOZ2_MAP_FILE].fd, sizeof(bucket), SEEK_SET);

  for(;;)
  {
    l = read(moz2_files[MOZ2_MAP_FILE].fd, bucket, sizeof(bucket));

    if(!l)
      break;

    if(l != sizeof(bucket))
    {
      xprintf(1, gettext("Corrupted Mozilla cache map file!"));
      break;
    }
    p = bucket;
    for(i = 0; i < MOZ2_REC_PER_BUCKET; i++, p += MOZ2_REC_SIZE)
    {
      moz2_cache_map_record *cmr = (moz2_cache_map_record *) p;
      moz_pair_t *mp;

      mp = moz2_get_record_info(cmr);
      if(mp)
        dlhash_insert(moz_hash, (dllist_t) mp);
    }
  }

  moz2_close_db();
}

static void moz_new_cache_read_db(char *dirname)
{
  bool_t reopen = FALSE;
  char pom[PATH_MAX];

  if(dirname)
  {
    struct stat estat;

    if(!moz_cache_filename || strcmp(dirname, moz_cache_filename))
      reopen = TRUE;

    snprintf(pom, sizeof(pom), "%s/_CACHE_MAP_", dirname);

    if(!reopen && !stat(pom, &estat) &&
      estat.st_mtime != moz_cache_db_modtime)
    {
      reopen = TRUE;
      moz_cache_db_modtime = estat.st_mtime;
    }
  }

  if(reopen || !dirname)
  {
    _free(moz_cache_filename);
    moz_cache_db_modtime = 0L;
    dlhash_empty(moz_hash);
  }

  if(reopen)
  {
    moz2_read_db(dirname);
  }
}

static void moz_cache_read_db(char *dirname)
{
  char pom[PATH_MAX];

  if(!moz_hash)
  {
    moz_hash = dlhash_new(256, moz_pair_get_urlstr,
    str_hash_func, str_comp_func);
    dlhash_set_free_func(moz_hash, moz_pair_free, NULL);
  }
  snprintf(pom, sizeof(pom), "%s/_CACHE_MAP_", dirname);
#ifdef HAVE_BDB_18x
  if(access(pom, F_OK))
    moz_old_cache_read_db(dirname);
  else
#endif /* HAVE_BDB_18x */
    moz_new_cache_read_db(dirname);
}

char *moz_cache_find_localname(char *urlstr)
{
  char *retv = NULL;

  LOCK_NSCACHE;
  moz_cache_read_db(priv_cfg.moz_cache_dir);

  if(moz_hash)
  {
    moz_pair_t *mp;

    mp = (moz_pair_t *) dlhash_find_by_key(moz_hash, (dllist_t) urlstr);

    if(mp)
      retv = tl_strdup(mp->filename);
  }
  UNLOCK_NSCACHE;

  return retv;
}
