/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include "config.h"
#include <string.h>

#ifdef I_FACE
#include "gui.h"
#include "gui_api.h"
#include "tools.h"

#ifdef WITH_TREE
#include "xpm/audio.xpm"
#include "xpm/binary.xpm"
#include "xpm/html.xpm"
#include "xpm/image.xpm"
#include "xpm/video.xpm"
#include "xpm/text.xpm"
#include "xpm/gopherdir.xpm"
#include "xpm/ftpdir.xpm"
#include "xpm/broken.xpm"
#include "xpm/redirected.xpm"
#include "xpm/rejected.xpm"
#include "xpm/notprocessed.xpm"
#include "xpm/cantaccess.xpm"
#include "xpm/incomplete.xpm"
#include "xpm/local.xpm"
#include "xpm/compressed.xpm"
#endif

void icons_set_for_doc(doc * docp)
{
#ifdef WITH_TREE
  Icon *icn;
  int i;

  icn = gui_cfg.icon.binary;

  if(docp->doc_url->status & URL_MOVED)
    icn = gui_cfg.icon.redirected;
  else if(docp->doc_url->status & URL_TRUNCATED)
    icn = gui_cfg.icon.incomplete;
  else if(docp->doc_url->status & URL_REJECTED)
    icn = gui_cfg.icon.rejected;
  else if((docp->doc_url->status & URL_NOT_FOUND) ||
    (docp->doc_url->status & URL_ERR_UNREC))
    icn = gui_cfg.icon.broken;
  else if(docp->doc_url->status & URL_ERR_REC)
    icn = gui_cfg.icon.cantaccess;
  else if((docp->doc_url->type == URLT_FTP
      || docp->doc_url->type == URLT_FTPS) && docp->doc_url->p.ftp.dir)
    icn = gui_cfg.icon.ftpdir;
  else if((docp->doc_url->type == URLT_GOPHER) &&
    (docp->doc_url->p.gopher.selector[0] == 'I'))
    icn = gui_cfg.icon.image;
  else if((docp->doc_url->type == URLT_GOPHER) &&
    (docp->doc_url->p.gopher.selector[0] == '0'))
    icn = gui_cfg.icon.text;
  else if((docp->doc_url->type == URLT_GOPHER) &&
    (docp->doc_url->p.gopher.selector[0] == '1'))
    icn = gui_cfg.icon.gopherdir;
  else if(docp->type_str)
  {
    if(!strncasecmp(docp->type_str, "text/html", 9))
      icn = gui_cfg.icon.html;
    else if(!strncasecmp(docp->type_str, "text/", 5))
      icn = gui_cfg.icon.text;
    else if(!strncasecmp(docp->type_str, "audio/", 6))
      icn = gui_cfg.icon.audio;
    else if(!strncasecmp(docp->type_str, "image/", 6))
      icn = gui_cfg.icon.image;
    else if(!strncasecmp(docp->type_str, "video/", 6))
      icn = gui_cfg.icon.video;
    else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
        "gz", "arj", "zip", "lha", "pak", "lzh", "bz", "bz2", "rar", "uc2",
        "ha", "tgz", NULL))
      icn = gui_cfg.icon.compressed;
    else
      icn = gui_cfg.icon.binary;
  }
  else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
      "html", "htm", "shtml", "phtml", NULL))
    icn = gui_cfg.icon.html;
  else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
      "txt", NULL))
    icn = gui_cfg.icon.text;
  else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
      "mov", "avi", "mpg", "mpeg", "fli", "flc", NULL))
    icn = gui_cfg.icon.video;
  else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
      "jpg", "jpeg", "png", "gif", "bmp", "pic", "ppm", "pbm", "pgm", "pnm",
      "xbm ", "xpm", NULL))
    icn = gui_cfg.icon.image;
  else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
      "voc", "au", "wav", "midi", "mp3", NULL))
    icn = gui_cfg.icon.audio;
  else if(str_is_in_list(0, tl_get_extension(url_get_path(docp->doc_url)),
      "gz", "arj", "zip", "lha", "pak", "lzh", "bz", "bz2", "rar", "uc2",
      "ha", "tgz", NULL))
    icn = gui_cfg.icon.compressed;
  else if((docp->doc_url->type == URLT_FILE) ||
    (docp->doc_url->status & URL_REDIRECT))
    icn = gui_cfg.icon.local;

#ifdef GTK_FACE
  LOCK_URL(docp->doc_url);
  for(i = 0; i < docp->doc_url->ref_cnt; i++)
  {
    char *p = url_to_urlstr(docp->doc_url, FALSE);

    LOCK_GTKTREE;
    GDK_THREADS_ENTER();
    gtk_ctree_set_node_info(GTK_CTREE(gui_cfg.tree_widget),
      docp->doc_url->tree_nfo[i], p, 8,
      icn->pixmap, icn->shape, icn->pixmap, icn->shape, FALSE, TRUE);
    GDK_THREADS_LEAVE();
    UNLOCK_GTKTREE;

    _free(p);
  }
  UNLOCK_URL(docp->doc_url);
#endif
  _Xt_Serve;
#endif
}

void icons_load(void)
{
#ifdef WITH_TREE
  gui_cfg.icon.audio = guitl_load_pixmap(audio_xpm);
  gui_cfg.icon.binary = guitl_load_pixmap(binary_xpm);
  gui_cfg.icon.html = guitl_load_pixmap(html_xpm);
  gui_cfg.icon.image = guitl_load_pixmap(image_xpm);
  gui_cfg.icon.video = guitl_load_pixmap(video_xpm);
  gui_cfg.icon.text = guitl_load_pixmap(text_xpm);
  gui_cfg.icon.gopherdir = guitl_load_pixmap(gopherdir_xpm);
  gui_cfg.icon.ftpdir = guitl_load_pixmap(ftpdir_xpm);
  gui_cfg.icon.broken = guitl_load_pixmap(broken_xpm);
  gui_cfg.icon.redirected = guitl_load_pixmap(redirected_xpm);
  gui_cfg.icon.rejected = guitl_load_pixmap(rejected_xpm);
  gui_cfg.icon.notprocessed = guitl_load_pixmap(notprocessed_xpm);
  gui_cfg.icon.cantaccess = guitl_load_pixmap(cantaccess_xpm);
  gui_cfg.icon.incomplete = guitl_load_pixmap(incomplete_xpm);
  gui_cfg.icon.local = guitl_load_pixmap(local_xpm);
  gui_cfg.icon.compressed = guitl_load_pixmap(compressed_xpm);
#endif
}

#endif
