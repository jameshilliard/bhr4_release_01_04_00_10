/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bufio.h"
#include "dinfo.h"
#include "doc.h"
#include "form.h"
#include "mime.h"
#include "tools.h"
#include "url.h"

static char *dinfo_get_filename_by_filename(char *fname)
{
  char *p;
  char *pom;

  if(priv_cfg.info_dir)
  {
    /*
       pro: if our file does NOT reside inside the cache dir we
       just return a bogus file name;
       The file name could e.g. be inside the current directory
       of the running pavuk process.
       See htmlparser.c for details (search for "'#'")
       Without this fix the program will CRASH in such cases
       (provided that the file names have a certain minimum length).

       The crash is caused by the fact that the routine makes the
       resulting filename start with "info_dir" instead of
       "cache_dir" even if the prefix of the given file name does
       not start with "cache_dir".
       This means that the routine makes the resulting filename
       start with "info_dir" and then takes the string starting
       after offset strlen("cache_dir") of the given filename.
       If strlen("cache_dir") is greater than the length of the
       given filename the system will take an undefined string of
       undefined length out of the heap and the resulting string
       might be much longer than the pre-calculated string len.

       => happy crashing
     */
    if(strncmp(fname, priv_cfg.cache_dir, strlen(priv_cfg.cache_dir)) != 0)
    {
      char bogus[] = "/bogus_dir";
      pom = _malloc(strlen(bogus) + 1);
      strcpy(pom, bogus);
      DEBUG_HTML("Returning bogus filename for file: '%s'\n", fname);
      return pom;
    }
    pom = _malloc(strlen(fname) + 30 + strlen(priv_cfg.info_dir) -
      strlen(priv_cfg.cache_dir));
    strcpy(pom, priv_cfg.info_dir);
    p = fname + strlen(priv_cfg.cache_dir);
    if(*p != '/')
      p--;
    strcat(pom, p);
  }
  else
  {
    pom = _malloc(strlen(fname) + 30);
    strcpy(pom, fname);
  }

  p = strrchr(pom, '/');
  if(!p)
    p = pom;
  else
    p++;
  memmove(p + 12, p, strlen(p) + 1);
  strncpy(p, ".pavuk_info/", 12);

  return pom;
}

static char *dinfo_get_filename_by_url(url * urlp)
{
  return dinfo_get_filename_by_filename(url_to_filename(urlp, TRUE));
}

static int dinfo_save_real(char *fnm, url * urlp, char *mime)
{
  int fd;
  char *p;

  if(makealldirs(fnm))
    xperror(fnm);

  if((fd = open(fnm, O_BINARY | O_CREAT | O_TRUNC | O_RDWR,
        S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR)) < 0)
  {
    xperror(fnm);
    return -1;
  }

  write(fd, "Original_URL: ", 14);
  p = url_to_urlstr(urlp, FALSE);
  write(fd, p, strlen(p));
  _free(p);
  write(fd, "\n", 1);

  if(urlp->status & URL_FORM_ACTION)
  {
    form_info *fi;
    dllist *ptr;

    fi = (form_info *) urlp->extension;

    write(fd, "XPavuk-FormMethod: ", 19);
    if(fi->method == FORM_M_GET)
      write(fd, "GET\n", 4);
    else if(fi->method == FORM_M_POST)
      write(fd, "POST\n", 5);

    write(fd, "XPavuk-FormEncoding: ", 21);
    if(fi->encoding == FORM_E_URLENCODED)
      write(fd, "application/x-www-form-urlencoded\n", 34);
    else if(fi->encoding == FORM_E_MULTIPART)
      write(fd, "multipart/form-data\n", 20);

    ptr = fi->infos;
    while(ptr)
    {
      form_field *ff;
      char *n, *v;

      ff = (form_field *) ptr->data;

      n = form_encode_urlencoded_str(ff->name);
      v = form_encode_urlencoded_str(ff->value);

      if(ff->type == FORM_T_FILE)
        write(fd, "XPavuk-FormFile: ", 17);
      else
        write(fd, "XPavuk-FormField: ", 18);

      write(fd, n, strlen(n));
      write(fd, "=", 1);
      write(fd, v, strlen(v));
      write(fd, "\n", 1);

      _free(v);
      _free(n);

      ptr = ptr->next;
    }
  }

  if(mime)
    write(fd, mime, strlen(mime));

  close(fd);

  return 0;
}

int dinfo_save(doc * docp)
{
  char *p;
  int rv;

  if(!cfg.enable_info)
    return 0;

  p = dinfo_get_filename_by_url(docp->doc_url);
  rv = dinfo_save_real(p, docp->doc_url, docp->mime);

  _free(p);
  return rv;
}

char *dinfo_load(char *fname)
{
  char *p = NULL;
  char pom[1024];
  int l, tl = 0;
  bufio *fd;

  if(!(fd = bufio_open(fname, O_BINARY | O_RDONLY)))
  {
    /*xperror(fname); */
    return NULL;
  }

  while((l = bufio_readln(fd, pom, sizeof(pom))) > 0)
  {
    p = _realloc(p, tl + l + 1);
    memcpy(p + tl, pom, l);
    tl += l;
  }

  if(p)
    *(p + tl) = '\0';

  bufio_close(fd);

  return p;
}

char *dinfo_get_unique_name(url * urlp, char *pname, int lockfn)
{
  char *dinfos;
  char *p, *us;
  int i;
  char *pom = NULL;
  char *pom2 = NULL;
  char xidx[20];
  char *idir, *ofn, *odir;
  int fd = -1;
  bool_t exist = FALSE;

  pom2 = dinfo_get_filename_by_filename(pname);
  pom = _malloc(strlen(pname) + 14);

  p = strrchr(pname, '/');
  if(p)
  {
    ofn = tl_strdup(p + 1);
    odir = tl_strndup(pname, p - pname);
  }
  else
  {
    ofn = tl_strdup("");
    odir = tl_strdup(pname);
  }

  p = strrchr(pom2, '/');
  if(p)
    idir = tl_strndup(pom2, p - pom2);
  else
    idir = tl_strdup(pom2);


  /* !!!!! lock !!!!! */
  if(lockfn)
  {
    pom2[strlen(idir) - 11] = '\0';
    strcat(pom2, "._lock");
    if(makealldirs(pom2))
      xperror(pom2);
    fd = open(pom2, O_BINARY | O_WRONLY | O_CREAT, 0644);
    if(fd < 0)
    {
      xperror(pom2);
      _free(ofn);
      _free(idir);
      _free(odir);
      _free(pom);
      _free(pom2);
      return NULL;
    }
    if(_flock(fd, pom2, O_BINARY | O_WRONLY | O_CREAT, TRUE))
    {
      xperror(pom2);
      _free(ofn);
      _free(idir);
      _free(odir);
      close(fd);
      _free(pom);
      _free(pom2);
      return NULL;
    }
    strcpy(pom2, idir);
    strcat(pom2, "/");
    if(makealldirs(pom2))
      xperror(pom2);
  }

  us = url_to_urlstr(urlp, FALSE);

  i = 1;
  xidx[0] = '\0';
  while(i > 0)
  {
    sprintf(pom, "%s/%s%s", odir, xidx, ofn);
    sprintf(pom2, "%s/%s%s", idir, xidx, ofn);
    if(access(pom, F_OK) && access(pom2, F_OK))
      break;
    if((dinfos = dinfo_load(pom2)))
    {
      p = get_mime_param_val_str("Original_URL:", dinfos);
      _free(dinfos);
      if(p && !strcmp(p, us))
      {
        _free(p);
        exist = TRUE;
        break;
      }
      _free(p);
    }
    else
    {
      /*
         We don't have info file, but have regular file
         thus we can assume that this file belongs to
         current URL. Usable in case when files were
         before downloaded without -store_info option.
       */
      break;
    }

    sprintf(xidx, "%03d_", i);
    i++;
  }

  /* create info file to know, that this filename is reserved */
  if(lockfn)
  {
    if(!exist)
      dinfo_save_real(pom2, urlp, NULL);

    /* !!!! unlock !!!! */
    _funlock(fd);
    close(fd);
  }

  _free(us);
  _free(idir);
  _free(odir);
  _free(ofn);
  _free(pom2);
  return pom;
}

url *dinfo_get_url_for_filename(char *fn)
{
  char *p = dinfo_get_filename_by_filename(fn);
  char *dinfos;
  url *rv = NULL;

  dinfos = dinfo_load(p);
  _free(p);

  if(dinfos)
  {
    p = get_mime_param_val_str("Original_URL:", dinfos);
    if(p)
    {
      rv = url_parse(p);
      assert(rv->type != URLT_FROMPARENT);
      _free(p);
    }
    if(rv)
    {
      p = get_mime_param_val_str("XPavuk-FormMethod:", dinfos);

      if(p)
      {
        form_info *fi;
        int i;

        fi = _malloc(sizeof(form_info));

        fi->method = FORM_M_GET;
        fi->encoding = FORM_E_URLENCODED;
        fi->action = NULL;
        fi->text = NULL;
        fi->infos = NULL;

        if(!strcasecmp(p, "GET"))
          fi->method = FORM_M_GET;
        else if(!strcasecmp(p, "POST"))
          fi->method = FORM_M_POST;
        _free(p);

        p = get_mime_param_val_str("XPavuk-FormEncoding:", dinfos);

        if(p)
        {
          if(!strcasecmp(p, "multipart/form-data"))
            fi->encoding = FORM_E_MULTIPART;
          else if(strcasecmp(p, "application/x-www-form-urlencoded"))
            fi->encoding = FORM_E_URLENCODED;
          _free(p);
        }

        for(i = 0;
          (p = get_mime_n_param_val_str("XPavuk-FormFile:", dinfos, i)); i++)
        {
          char *tp = strchr(p, '=');

          if(tp)
          {
            form_field *ff = _malloc(sizeof(form_field));

            ff->type = FORM_T_FILE;
            ff->name = form_decode_urlencoded_str(p, tp - p);
            ff->value = form_decode_urlencoded_str(tp + 1, strlen(tp));

            fi->infos = dllist_append(fi->infos, (dllist_t) ff);
          }
          else
            xprintf(1, gettext("Error parsing .pavuk_info file field: %s\n"),
              p);
        _free(p)}

        for(i = 0;
          (p = get_mime_n_param_val_str("XPavuk-FormField:", dinfos, i)); i++)
        {
          char *tp = strchr(p, '=');

          if(tp)
          {
            form_field *ff = _malloc(sizeof(form_field));

            ff->type = FORM_T_TEXT;
            ff->name = form_decode_urlencoded_str(p, tp - p);
            ff->value = form_decode_urlencoded_str(tp + 1, strlen(tp));

            fi->infos = dllist_append(fi->infos, (dllist_t) ff);
          }
          else
            xprintf(1, gettext("Error parsing .pavuk_info file field: %s\n"),
              p);
          _free(p);
        }

        if(!fi->infos)
          _free(fi);

        if(fi)
        {
          rv->extension = fi;
          rv->status = URL_FORM_ACTION;
        }
      }
    }
    _free(dinfos);
  }

  return rv;
}

void dinfo_remove(char *fn)
{
  char *p, *pinf;

  pinf = dinfo_get_filename_by_filename(fn);

  if(!unlink(pinf))
  {
    char *pom;
    int fd;

    pom = tl_strdup(pinf);
    p = strrchr(pom, '/');
    if(p)
      *(p) = '\0';
    p = strrchr(pom, '/');
    if(p)
      *(p + 1) = '\0';
    strcat(pom, "._lock");      /* FIXME - security */

    fd = open(pom, O_BINARY | O_WRONLY | O_CREAT, 0644);
    if(fd < 0)
    {
      xperror(pom);
      _free(pom);
      return;
    }
    if(_flock(fd, pom, O_BINARY | O_WRONLY | O_CREAT, TRUE))
    {
      xperror(pom);
      close(fd);
      _free(pom);
      return;
    }

    p = strrchr(pom, '/');
    if(p)
      *(p + 1) = '\0';
    strcat(pom, ".pavuk_info"); /* FIXME - security */
    if(rmdir(pom) && errno != ENOTEMPTY && errno != EEXIST)
      xperror(pom);

    p = strrchr(pom, '/');
    if(p)
      *(p + 1) = '\0';
    strcat(pom, "._lock");      /* FIXME - security */
    if(unlink(pom) && errno != ENOENT)
      xperror(pom);
    _funlock(fd);
    close(fd);
    _free(pom);
  }
  _free(pinf);
}
