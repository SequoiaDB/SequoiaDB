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

   Source File Name = ossSSLContext.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/2/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifdef SDB_SSL

#include "ossSSLWrapper.h"
#include "oss.h"

SDB_EXTERN_C_START

typedef struct SSLGlobalContext
{
   SSLContext*       ctx;
   SSLCertificate*   cert;
   SSLKey*           key;
} SSLGlobalContext;

static SSLGlobalContext sslGlobalContext = {NULL};

static void _ossSSLInitGlobalContext( void )
{
   SSLKey* key = NULL;
   SSLCertificate* cert = NULL;
   SSLContext* ctx = NULL;
   INT32 ret;

   static BOOLEAN inited = FALSE;
   SSL_ASSERT(!inited);

   ret = ossSSLInit();
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = ossSSLNewKey(&key);
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = ossSSLNewCertificate(&cert, key);
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = ossSSLNewContext(&ctx, cert, key);
   if (SSL_OK != ret)
   {
      goto error;
   }

   sslGlobalContext.ctx = ctx;
   sslGlobalContext.cert = cert;
   sslGlobalContext.key = key;

   inited = TRUE;

done:
   return;
error:
   if (NULL != ctx)
   {
      ossSSLFreeContext(&ctx);
   }
   if (NULL != cert)
   {
      ossSSLFreeCertificate(&cert);
   }
   if (NULL != key)
   {
      ossSSLFreeKey(&key);
   }
   goto done;
}

SSLContext* ossSSLGetContext()
{
   static ossOnce initOnce = OSS_ONCE_INIT;

   ossOnceRun(&initOnce, _ossSSLInitGlobalContext);

   return sslGlobalContext.ctx;
}

SDB_EXTERN_C_END

#endif /* SDB_SSL */
