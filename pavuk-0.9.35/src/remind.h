/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _remind_h_
#define _remind_h_

#include "url.h"
#include "time.h"

typedef struct
{
  url *urlp;
  time_t mdtm;
  int status;
} remind_entry;

#define REMIND_ERROR            (1 << 0)
#define REMIND_MODIFIED         (1 << 1)
#define REMIND_PROCESSED        (1 << 2)

extern void remind_load_db(void);
extern void remind_save_db(void);
extern void remind_do(void);
extern void remind_start_add(void);
extern void remind_send_result(void);

#endif
