/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <utime.h>

#include "tools.h"
#include "url.h"
#include "doc.h"
#include "html.h"
#include "gui_api.h"
#include "update_links.h"

static void rewrite_links(char *);

void update_links(char *dirname)
{
  DIR *dir;
  struct dirent *dent;
  char next_dir[PATH_MAX];
  struct stat estat;

  xprintf(1, gettext("Entering directory %s\n"), dirname);
  if(!(dir = opendir(dirname)))
  {
    xperror(dirname);
    return;
  }

  while((dent = readdir(dir)))
  {
    _Xt_Serve;

    sprintf(next_dir, "%s/%s", dirname, dent->d_name);
    if(!strcmp(dent->d_name, "."))
      continue;
    if(!strcmp(dent->d_name, ".."))
      continue;
    if(stat(next_dir, &estat))
    {
      xperror(next_dir);
      continue;
    }

    if(S_ISDIR(estat.st_mode))
    {
      if(!strcmp(dent->d_name, ".pavuk_info") && cfg.enable_info)
        continue;

      update_links(next_dir);
    }
    else if(file_is_html(next_dir))
    {
      xprintf(1, gettext("Relocating %s\n"), next_dir);
      rewrite_links(next_dir);
    }
    else
      xprintf(1, gettext("Omitting %s\n"), next_dir);
#ifdef I_FACE
    if(cfg.xi_face)
    {
      if(cfg.rbreak || cfg.stop)
      {
        closedir(dir);
        return;
      }
    }
#endif
  }

  closedir(dir);
  xprintf(1, gettext("Leaving directory %s\n"), dirname);
}

static void rewrite_links(char *fn)
{
  char pom[2048];
  char *savetmp, *p;
  int sock;
  doc pdoc;
  struct stat estat;
  struct utimbuf ut;
  url dum;

  if(stat(fn, &estat) == 0)
  {
    if(S_ISDIR(estat.st_mode))
    {
      xprintf(1, gettext("Can't open directory %s\n"), fn);
      return;
    }
  }

  ut.actime = estat.st_atime;
  ut.modtime = estat.st_mtime;

  memset(&dum, '\0', sizeof(url));
  dum.type = URLT_FILE;
  dum.p.file.filename = fn;
  dum.local_name = fn;
  if(!strcasecmp(tl_get_extension(fn), "css"))
    dum.status = URL_STYLE;
  else
    dum.status = 0;

  doc_init(&pdoc, &dum);

  doc_download(&pdoc, 1, TRUE);

  _free(pdoc.mime);

  html_process_parent_document(&pdoc, NULL, NULL);

  strcpy(pom, fn);
  p = strrchr(pom, '/');
  sprintf(p + 1, "._lnkupd%d", (int) getpid());
  savetmp = tl_strdup(pom);
  rename(fn, savetmp);

  if((sock = open(fn, O_BINARY | O_TRUNC | O_CREAT | O_WRONLY, 0644)) < 0)
  {
    xperror(fn);
    rename(savetmp, fn);
    free(savetmp);
    free(pdoc.contents);
    doc_remove_lock(&pdoc);
    return;
  }
  if(write(sock, pdoc.contents, pdoc.size) != pdoc.size)
  {
    xperror(fn);
    close(sock);
    rename(savetmp, fn);
    free(savetmp);
    free(pdoc.contents);
    doc_remove_lock(&pdoc);
    return;
  }
  close(sock);
  utime(fn, &ut);
  unlink(savetmp);
  free(savetmp);
  free(pdoc.contents);
  doc_remove_lock(&pdoc);
}
