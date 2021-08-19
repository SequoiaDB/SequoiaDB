/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ossSSLWrapper.h

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/1/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SSL_WRAPPER_H_
#define OSS_SSL_WRAPPER_H_

#ifdef SDB_SSL

#include "core.h"

#define SSL_OK       (0)
#define SSL_ERROR    (-1)
#define SSL_AGAIN    (-2)
#define SSL_TIMEOUT  (-3)

#ifdef _DEBUG
#include <assert.h>
#define SSL_ASSERT(e) assert(e)
#else
#define SSL_ASSERT(e) 
#endif

SDB_EXTERN_C_START

typedef struct evp_pkey_st SSLKey;
typedef struct x509_st     SSLCertificate;
typedef struct ssl_ctx_st  SSLContext;
typedef struct SSLHandle   SSLHandle;

/* ossSSLCertificate.c */
INT32 ossSSLNewKey(SSLKey** key);
void  ossSSLFreeKey(SSLKey** key);
INT32 ossSSLNewCertificate(SSLCertificate** cert, SSLKey* key);
void  ossSSLFreeCertificate(SSLCertificate** cert);

/* ossSSLWrapper.c */
INT32 ossSSLInit();
void  ossSSLFinalize();
INT32 ossSSLNewContext(SSLContext** context, SSLCertificate* cert, SSLKey* key);
void  ossSSLFreeContext(SSLContext** context);
INT32 ossSSLNewHandle(SSLHandle** handle, SSLContext* ctx, SOCKET sock, const char* initialBytes, INT32 len);
void  ossSSLFreeHandle(SSLHandle** handle);
INT32 ossSSLConnect(SSLHandle* handle);
INT32 ossSSLAccept(SSLHandle* handle);
INT32 ossSSLRead(SSLHandle* handle, void* buf, INT32 num);
INT32 ossSSLPeek(SSLHandle* handle, void* buf, INT32 num);
INT32 ossSSLWrite(SSLHandle* handle, const void* buf, INT32 num);
INT32 ossSSLShutdown(SSLHandle* handle);
INT32 ossSSLGetError(SSLHandle* handle);
char* ossSSLGetErrorMessage(INT32 error);
INT32 ossSSLERRGetError();
char* ossSSLERRGetErrorMessage(INT32 error);

/* ossSSLContext.c */
SSLContext* ossSSLGetContext();

SDB_EXTERN_C_END

#else
#error "include \"ossSSLWrapper.h\" without SDB_SSL"
#endif /* SDB_SSL */

#endif /* OSS_SSL_WRAPPER_H_ */

