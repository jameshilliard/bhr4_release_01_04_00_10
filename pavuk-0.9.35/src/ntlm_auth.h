/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _ntlm_auth_h_
#define _ntlm_auth_h_

/****************************************************************/
/* based on document "NTLM Athentication Scheme for HTTP"       */
/* http://www.innovation.ch/java/ntlm.html                      */
/* by Ronald Tschaler <ronald@innovation.ch>                    */
/****************************************************************/

typedef unsigned char byte_t;

#ifdef __GNUC__
#define _PACKED         __attribute__((packed))
#else
#define _PACKED
#endif

#define NTLM_MSG1_SIZE 32+128
#define NTLM_MSG2_SIZE 40
#define NTLM_MSG3_SIZE 64+512

typedef struct
{
  byte_t protocol[8];           /* "NTLMSSP\0"                          */
  byte_t type;                  /* 0x01                                 */
  byte_t zero[3];               /* \0\0\0                               */
  unsigned short flags;         /* 0xb203                               */
  byte_t zero2[2];              /* \0\0                                 */

  short dom_len;                /* length of domainname                 */
  short dom_len2;               /* length of domainname                 */
  short dom_off;                /* offset of domainname in structure    */
  byte_t zero3[2];              /* \0\0                                 */

  short host_len;               /* length of hostname                   */
  short host_len2;              /* length of hostname                   */
  short host_off;               /* offset of hostname in structure      */
  byte_t zero4[2];              /* \0\0                                 */

  byte_t padding[128];          /* space for hostname and domainname    */
#if 0
  byte_t host[*];               /* hostname                             */
  byte_t dom[*];                /* domainname                           */
#endif
} _PACKED ntlm_type_1_msg_t;

typedef struct
{
  byte_t protocol[8];           /* "NTLMSSP\0"                          */
  byte_t type;                  /* 0x02                                 */
  byte_t zero[7];               /* \0\0\0\0\0\0\0                       */
  short msg_len;                /* 0x28                                 */
  byte_t zero2[2];              /* \0\0                                 */
  unsigned short flags;         /* 0x8201                               */
  byte_t zero3[2];              /* \0\0                                 */
  byte_t nonce[8];              /* server nonce                         */
  byte_t zero4[8];              /* \0\0\0\0\0\0\0\0                     */
} _PACKED ntlm_type_2_msg_t;

typedef struct
{
  byte_t protocol[8];           /* "NTLMSSP\0"                          */
  byte_t type;                  /* 0x03                                 */
  byte_t zero[3];               /* \0\0\0                               */

  short lm_resp_len;            /* LM response length (0x18)            */
  short lm_resp_len2;           /* LM response length (0x18)            */
  short lm_resp_off;            /* offset of LM response in structure   */
  byte_t zero2[2];              /* \0\0                                 */

  short nt_resp_len;            /* NT response length (0x18)            */
  short nt_resp_len2;           /* NT response length (0x18)            */
  short nt_resp_off;            /* offset of NT response in structure   */
  byte_t zero3[2];              /* \0\0                                 */

  short dom_len;                /* length of domainname                 */
  short dom_len2;               /* length of domainname                 */
  short dom_off;                /* offset of domainname in structure    */
  byte_t zero4[2];              /* \0\0                                 */

  short user_len;               /* length of username                   */
  short user_len2;              /* length of username                   */
  short user_off;               /* offset of username in structure      */
  byte_t zero5[2];              /* \0\0                                 */

  short host_len;               /* length of hostname                   */
  short host_len2;              /* length of hostname                   */
  short host_off;               /* offset of hostname in structure      */
  byte_t zero6[6];              /* \0\0\0\0\0\0                         */

  short msg_len;                /* message length                       */
  byte_t zero7[2];              /* \0\0                                 */

  unsigned short flags;         /* 0x8201                               */
  byte_t zero8[2];              /* \0\0                                 */

  byte_t padding[512];          /* space for hostname and domainname    */
#if 0
  byte_t host[*];               /* hostname                             */
  byte_t user[*];               /* username                             */
  byte_t dom[*];                /* domainname                           */
  byte_t lm_resp[*];            /* LM response                          */
  byte_t nt_resp[*];            /* NT response                          */
#endif
} _PACKED ntlm_type_3_msg_t;

extern char *ntlm_get_t1msg_str(char *, char *);
extern char *ntlm_get_nonce(char *, unsigned short *);
extern char *ntlm_get_t3msg_str(char *, char *, char *, char *, char *,
  unsigned short);

#ifndef ____ACCONFIG_TEST___
extern int ntlm_negotiate_connection(doc *, char *);
extern int ntlm_negotiate_proxy_connection(doc *, char *);
#endif

#endif
