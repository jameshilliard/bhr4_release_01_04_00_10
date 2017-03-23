/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fcntl.h>

#include "config.h"
#include "stdio.h"
#include "file.h"
#include "url.h"
#include "errcode.h"

/********************************************************/
/* otvory subor pre dane FILE URL                       */
/* FIXME: Translate me!                                 */
/********************************************************/
bufio *get_file_data_socket(doc *docp)
{
  struct stat estat;

  if(stat(docp->doc_url->p.file.filename, &estat) == 0)
  {
    if(S_ISDIR(estat.st_mode))
    {
      xprintf(1, gettext("Can't open directory\n"));
      docp->errcode = ERR_DIR_URL;
      return NULL;
    }
    docp->totsz = estat.st_size;
  }
  if(!(docp->datasock =
      bufio_open(docp->doc_url->p.file.filename, O_BINARY | O_RDONLY)))
  {
    docp->errcode = ERR_FILE_OPEN;
    xperror(docp->doc_url->p.file.filename);
  }
  return docp->datasock;
}
