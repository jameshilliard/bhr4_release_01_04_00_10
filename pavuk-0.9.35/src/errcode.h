/***************************************************************************/
/*    This code is part of WWW grabber called pavuk                        */
/*    Copyright (c) 1997 - 2001 Stefan Ondrejicka                          */
/*    Distributed under GPL 2 or later                                     */
/***************************************************************************/

#ifndef _errcode_h_
#define _errcode_h_

#include "doc.h"

#define ERR_NOERROR             0

#define ERR_STORE_DOC           1       /*** error string document to local file ***/
#define ERR_FILE_OPEN           2       /*** unable to open local file ***/
#define ERR_DIR_URL             3       /*** FILE URL points to directory ***/
#define ERR_UNKNOWN             5
#define ERR_LOCKED              6       /*** document is locked by another pavuk instance ***/
#define ERR_READ                7       /*** error while reading from socket/file ***/
#define ERR_BIGGER              8       /*** file is bigger than max. allowed ***/
#define ERR_NOMIMET             9       /*** file MIME type is not allowed ***/
#define ERR_PROXY_CONNECT       10      /*** error while connecting to proxy server ***/
#define ERR_BREAK               11      /*** user break ***/
#define ERR_OUTTIME             12      /*** file modification time doesn't fit in allowed time interval ***/
#define ERR_SCRIPT_DISABLED     13      /*** disabled by uexit script ***/
#define ERR_SMALLER             14      /*** file is smaller than minimal allowed ***/
#define ERR_ZERO_SIZE           15      /*** if file have zero size ***/
#define ERR_PROCESSED           16      /*** document was allready processed ***/
#define ERR_UDISABLED           17      /*** user disables manualy the processing of this URL ***/
#define ERR_RDISABLED           18      /*** this document is disabled because of rules ***/
#define ERR_LOW_TRANSFER_RATE   19      /*** transfer rate lower than minimal requested ***/
#define ERR_QUOTA_FILE          20      /*** rich maximal allowed size of file ***/
#define ERR_QUOTA_TRANS         21      /*** quota for transfer exceeded ***/
#define ERR_QUOTA_FS            22      /*** low space on filesystem ***/
#define ERR_QUOTA_TIME          23      /*** maximal allowed running time exceeded ***/

#define ERR_FTP_UNKNOWN         1000    /*** unknown error ***/
#define ERR_FTP_NOREGET         1001    /*** server doesm't support reget ***/
#define ERR_FTP_BDIR            1002    /*** directory list error ***/
#define ERR_FTP_CONNECT         1003    /*** error conecting ***/
#define ERR_FTP_BUSER           1004    /*** USER error ***/
#define ERR_FTP_BPASS           1005    /*** PASS error ***/
#define ERR_FTP_DATACON         1006    /*** error seting up data connection ***/
#define ERR_FTP_GET             1007    /*** error when trying to transfer file or dir ***/
#define ERR_FTP_NODIR           1008    /*** directory doesn't exist or is not accesible ***/
#define ERR_FTP_TRUNC           1009    /*** truncated file ?? ***/
#define ERR_FTP_ACTUAL          1010    /*** ftp file is actual (no transfer) ***/
#define ERR_FTP_NOTRANSFER      1011    /*** nothing to transfer ***/
#define ERR_FTP_NOMDTM          1012    /*** server doesn't suport MDTM command ***/
#define ERR_FTP_DIRNO           1013    /*** directory , but not allowed ***/
#define ERR_FTP_BPROXYUSER      1014    /*** proxy USER error ***/
#define ERR_FTP_BPROXYPASS      1015    /*** proxy PASS error ***/
#define ERR_FTP_LOGIN_HANDSHAKE 1016    /*** failed custom login handshake ***/

#define ERR_HTTP_UNKNOWN        2000    /*** unknown error ***/
#define ERR_HTTP_CONNECT        2001    /*** connection error ***/
#define ERR_HTTP_NOREGET        2002    /*** reget not supported ***/
#define ERR_HTTP_SNDREQ         2003    /*** error sending request ***/
#define ERR_HTTP_REDIR          2004    /*** redirect ***/
#define ERR_HTTP_TRUNC          2005    /*** truncated ***/
#define ERR_HTTP_CYCLIC         2006    /*** cyclic redirection ***/
#define ERR_HTTP_UNSUPREDIR     2007    /*** unsupported URL in redirection ***/
#define ERR_HTTP_SNDREQDATA     2008    /*** error sending request data ***/
#define ERR_HTTP_PROXY_CONN     2009    /*** error connecting to HTTP proxy ***/
#define ERR_HTTP_BADREDIRECT    2010    /*** after redirect Loacation: is missing ***/
#define ERR_HTTP_AUTH_NTLM      2011    /*** error doing NTLM authorization ***/
#define ERR_HTTP_AUTH_DIGEST    2012    /*** error doing HTTP Digest authorization ***/
#define ERR_HTTP_PROAUTH_NTLM   2013    /*** error doing NTLM proxy authorization ***/
#define ERR_HTTP_PROAUTH_DIGEST 2014    /*** error doing HTTP proxy Digest authorization ***/
#define ERR_HTTP_CLOSURE        2015    /*** persistent connection was closed ***/
#define ERR_HTTP_RCVRESP        2016    /*** error reading response ***/
#define ERR_HTTP_FAILREGET      2017    /*** bad answer on reget request ***/
#define ERR_HTTP_ACTUAL         2304    /*** not modified ***/
#define ERR_HTTP_USE_PROXY      2305    /*** Use Proxy ***/
#define ERR_HTTP_306            2306    /*** 306 Unsupported by HTTP/1.1 ***/
#define ERR_HTTP_REDIR2         2307    /*** Temporary Redirect ***/
#define ERR_HTTP_BADRQ          2400    /*** bad request ***/
#define ERR_HTTP_AUTH           2401    /*** authorization error ***/
#define ERR_HTTP_PAY            2402    /*** payment required ***/
#define ERR_HTTP_FORB           2403    /*** forbiden request ***/
#define ERR_HTTP_NFOUND         2404    /*** not found ***/
#define ERR_HTTP_NALLOW         2405    /*** Method Not Allowed ***/
#define ERR_HTTP_NACCEPT        2406    /*** Not Acceptable ***/
#define ERR_HTTP_PROXY_AUTH     2407    /*** proxy authorization error ***/
#define ERR_HTTP_TIMEOUT        2408    /*** HTTP connection timeout ***/
#define ERR_HTTP_CONFLICT       2409    /*** conflict ***/
#define ERR_HTTP_GONE           2410    /*** gone ***/
#define ERR_HTTP_MISSLEN        2411    /*** Length Required ***/
#define ERR_HTTP_PREC_FAIL      2412    /*** Precondition Failed ***/
#define ERR_HTTP_TOO_LARGE      2413    /*** Request Entity Too Large ***/
#define ERR_HTTP_LONG_URL       2414    /*** Request-URI Too Long ***/
#define ERR_HTTP_UNSUP_MEDIA    2415    /*** unsupported media type ***/
#define ERR_HTTP_BAD_RANGE      2416    /*** Requested Range Not Satisfiable ***/
#define ERR_HTTP_EXPECT_FAIL    2417    /*** Expectation Failed ***/
#define ERR_HTTP_SERV           2500    /*** server error ***/
#define ERR_HTTP_NOT_IMPL       2501    /*** not implemented ***/
#define ERR_HTTP_BAD_GW         2502    /*** Bad Gateway ***/
#define ERR_HTTP_SERV_UNAVAIL   2503    /*** Service Unavailable ***/
#define ERR_HTTP_GW_TIMEOUT     2504    /*** Gateway Timeout ***/
#define ERR_HTTP_VER_UNSUP      2505    /*** HTTP Version Not Supported ***/

#define ERR_GOPHER_UNKNOWN      3000    /*** unknown error ***/
#define ERR_GOPHER_CONNECT      3001    /*** error connecting ***/

#define ERR_HTTPS_CONNECT       4001    /*** error connecting to HTTPS server ***/

#define ERR_FTPS_CONNECT        5001
#define ERR_FTPS_UNSUPORTED     5002
#define ERR_FTPS_DATASSLCONNECT 5003

extern void report_error(doc *, char *);

#endif
