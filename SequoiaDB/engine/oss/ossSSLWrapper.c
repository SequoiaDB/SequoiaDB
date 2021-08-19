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

   Source File Name = ossSSLWrapper.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/1/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifdef SDB_SSL

#include "ossSSLWrapper.h"
#include "oss.h"
#include "ossUtil.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#ifdef _WINDOWS
#pragma comment(lib, "Ws2_32.lib")
#else
#include <errno.h>
#endif

SDB_EXTERN_C_START

struct SSLHandle
{
   SSL*     ssl;
   BIO*     bufferBIO;
   SOCKET   sock;
   INT32    error;
};

#define _SSLHandleInit(h) \
do { \
   (h)->ssl = NULL; \
   (h)->bufferBIO = NULL; \
   (h)->sock = -1; \
   (h)->error = SSL_ERROR_NONE; \
} while (0)

#define _SSL_BIO_BUFFER_SIZE  (8 * 1024)

static ossMutex* _ossSSLThreadLocks = NULL;

static unsigned long _ossSSLThreadIdCallback(void)
{
   unsigned long ret;

   ret = (unsigned long)ossGetCurrentThreadID();
   return ret;
}

static void _ossSSLThreadLockingCallback(int mode, int type,
                                          const char* file, int line)
{
   if (mode & CRYPTO_LOCK) {
      ossMutexLock(&_ossSSLThreadLocks[type]);
   } else {
      ossMutexUnlock(&_ossSSLThreadLocks[type]);
   }
}

static INT32 _ossSSLTheadInit()
{
   INT32 i;
   INT32 ret = SSL_OK;

   _ossSSLThreadLocks = (ossMutex*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(ossMutex));
   if (NULL == _ossSSLThreadLocks)
   {
      goto error;
   }

   for (i = 0; i < CRYPTO_num_locks(); i++)
   {
      ossMutexInit(&_ossSSLThreadLocks[i]);
   }

   /* set OpenSSL multithreading callbacks */
   CRYPTO_set_id_callback(_ossSSLThreadIdCallback);
   CRYPTO_set_locking_callback(_ossSSLThreadLockingCallback);

done:
   return ret;
error:
   ret = SSL_ERROR;
   goto done;
}

static void _ossSSLThreadFinalize()
{
   INT32 i;

   CRYPTO_set_locking_callback(NULL);

   for (i = 0; i < CRYPTO_num_locks(); i++)
   {
      ossMutexDestroy(&_ossSSLThreadLocks[i]);
   }
   OPENSSL_free(_ossSSLThreadLocks);
}

INT32 ossSSLInit()
{
   SSLContext* ctx = NULL;
   INT32 ret = SSL_OK;

   SSL_library_init();
   SSL_load_error_strings();
   ERR_load_crypto_strings();
   OpenSSL_add_all_algorithms();

   ret = _ossSSLTheadInit();
   if (SSL_OK != ret)
   {
      goto error;
   }

   /*
    * ensure init is ok
    */
   ctx = SSL_CTX_new(SSLv23_method());
   if (NULL == ctx)
   {
      _ossSSLThreadFinalize();
      goto error;
   }

   SSL_CTX_free(ctx);

done:
   return ret;
error:
   ret = SSL_ERROR;
   goto done;
}

void ossSSLFinalize()
{
   _ossSSLThreadFinalize();
   ERR_free_strings();
   EVP_cleanup();
}

INT32 ossSSLNewContext(SSLContext** context, SSLCertificate* cert, SSLKey* key)
{
   SSLContext* ctx = NULL;
   INT32 ret = SDB_OK;

   SSL_ASSERT(NULL != context);
   SSL_ASSERT(NULL != key);
   SSL_ASSERT(NULL != cert);

   ctx = SSL_CTX_new(SSLv23_method ());
   if (NULL == ctx)
   {
      goto error;
   }

   /* SSL_OP_ALL - Activate all bug workaround options, to support buggy client SSL's.
    * SSL_OP_NO_SSLv2 - Disable SSL v2 support
    */
   SSL_CTX_set_options(ctx, (SSL_OP_ALL | SSL_OP_NO_SSLv2));

   /* HIGH - Enable strong ciphers
    * !EXPORT - Disable export ciphers (40/56 bit)
    * !aNULL - Disable anonymous auth ciphers
    * @STRENGTH - Sort ciphers based on strength
    */
   SSL_CTX_set_cipher_list(ctx, "HIGH:!EXPORT:!aNULL@STRENGTH");

   /* If renegotiation is needed, don't return from recv() or send() until it's successful.
    * NOTE: this is for blocking sockets only.
    */
   SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

   /* disable session caching */
   SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

   ret = SSL_CTX_use_certificate(ctx, cert);
   if (!ret)
   {
      goto error;
   }

   ret = SSL_CTX_use_PrivateKey(ctx, key);
   if (!ret)
   {
      goto error;
   }

   ret = SSL_CTX_check_private_key(ctx);
   if (!ret)
   {
      goto error;
   }

   *context = ctx;
   ret = SSL_OK;

done:
   return ret;
error:
   if (NULL != ctx)
   {
      SSL_CTX_free(ctx);
   }
   ret = SSL_ERROR;
   goto done;
}

void ossSSLFreeContext(SSLContext** context)
{
   SSL_ASSERT(NULL != context);

   if (NULL != *context)
   {
      SSL_CTX_free(*context);
      *context = NULL;
   }
}

/* Return value:
 * SSL_OK: the SSL handle is created
 * SSL_ERROR: failed, call ossSSLERRGetError() & ossSSLERRGetErrorMessage() for reason
 */
INT32 ossSSLNewHandle(SSLHandle** handle, SSLContext* ctx, SOCKET sock,
                             const char* initialBytes, INT32 len)
{
   SSLHandle* h = NULL;
   SSL* ssl = NULL;
   BIO* bufferBIO = NULL;
   BIO* socketBIO = NULL;
   INT32 ret = SSL_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != ctx);
   SSL_ASSERT(len >= 0);

   h = (SSLHandle*)OPENSSL_malloc(sizeof(SSLHandle));
   if (NULL == h)
   {
      goto error;
   }
   _SSLHandleInit(h);
   h->sock = sock;

   ssl = SSL_new(ctx);
   if (NULL == ssl)
   {
      goto error;
   }
   h->ssl = ssl;

   if (0 == len)
   {
      /* there is no initial bytes, so we just set the socket to SSL */
      ret = SSL_set_fd(ssl, sock);
      if (!ret)
      {
         goto error;
      }
   }
   else /* len > 0 */
   {
      SSL_ASSERT(NULL != initialBytes);

      /*
       * There are initial SSL bytes, so we should give these bytes to SSL by some way.
       * Here we create a buffer BIO, and put these bytes to it.
       * Then we create a socket BIO, and set a BIO chain to link 
       * the buffer and socket by BIO_push().
       * Finally, we set the buffer to SSL instead of the socket.
       *
       * NOTE: when do SSL operations, it should explicitly flush the buffer.
       */

      bufferBIO = BIO_new(BIO_f_buffer());
      if (NULL == bufferBIO)
      {
         goto error;
      }

      ret = BIO_set_buffer_read_data(bufferBIO, (void*)initialBytes, len);
      if (!ret)
      {
         goto error;
      }

      socketBIO = BIO_new_socket(sock, BIO_NOCLOSE);
      if (NULL == socketBIO)
      {
         goto error;
      }

      /* link socket to the buffer */
      if (NULL == BIO_push(bufferBIO, socketBIO))
      {
         goto error;
      }

      /* SSL_free() will also free bufferBIO,
       * so it's no need to free bufferBIO later when free the SSL handle.
       */
      SSL_set_bio(ssl, bufferBIO, bufferBIO);

      /* hold the bufferBIO pointer so we can flush it when do SSL operations */
      h->bufferBIO = bufferBIO;
   }

   *handle = h;
   ret = SDB_OK;

done:
   return ret;
error:
   if (NULL != bufferBIO)
   {
      BIO_free(bufferBIO);
   }
   if (NULL != socketBIO)
   {
      BIO_free(socketBIO);
   }
   if (NULL != ssl)
   {
      SSL_free(ssl);
   }
   if (NULL != h)
   {
      OPENSSL_free(h);
   }
   ret = SSL_ERROR;
   goto done;
}

void ossSSLFreeHandle(SSLHandle** handle)
{
   SSL_ASSERT(NULL != handle);

   if (NULL != *handle)
   {
      SSLHandle* h = *handle;

      /* h->bufferBIO will be freed by SSL_free() */
      if (NULL != h->ssl)
      {
         SSL_free(h->ssl);
      }
      OPENSSL_free(h);
      *handle = NULL;
   }
}

static INT32 _ossSSLCheckStatus(SSLHandle* handle, INT32 status)
{
   INT32 err;
   INT32 ret = SDB_OK;

   SSL_ASSERT(NULL != handle);

   err = SSL_get_error(handle->ssl, status);
   handle->error = err;
   switch(err)
   {
   case SSL_ERROR_WANT_WRITE:
   case SSL_ERROR_WANT_READ:
      ret = SSL_AGAIN;
      /* pass through */
   case SSL_ERROR_NONE:
      if (NULL != handle->bufferBIO)
      {
         if (!BIO_flush(handle->bufferBIO))
         {
            handle->error = SSL_ERROR_SYSCALL;
            ret = SSL_ERROR;
         }
      }
      break;
   default:
#ifdef _WINDOWS
      {
         /* OPENSSL considers WSAETIMEDOUT as an error */
         INT32 lastError = WSAGetLastError();
         if (WSAETIMEDOUT == lastError)
         {
            ret = SSL_TIMEOUT;
         }
      }
#else
      ret = SSL_ERROR;
#endif
   }

   return ret;
}

/* Return value:
 * SSL_OK: the SSL connection is created
 * SSL_ERROR: failed, call ossSSLGetError() & ossSSLGetErrorMessage() for reason
 */
INT32 ossSSLConnect(SSLHandle* handle)
{
   INT32 status;
   INT32 ret = SSL_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != handle->ssl);

   do
   {
      status = SSL_connect(handle->ssl);
      ret = _ossSSLCheckStatus(handle, status);
   } while (SSL_AGAIN == ret);

   return ret;
}

/* Return value:
 * SSL_OK: the SSL connection is accepted
 * SSL_ERROR: failed, call ossSSLGetError() & ossSSLGetErrorMessage() for reason
 */
INT32 ossSSLAccept(SSLHandle* handle)
{
   INT32 status;
   INT32 ret = SDB_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != handle->ssl);

   do
   {
      status = SSL_accept(handle->ssl);
      ret = _ossSSLCheckStatus(handle, status);
   } while (SSL_AGAIN == ret);

   return ret;
}

/* Return value:
 * >0: the number of bytes actually read from SSL connection
 * SSL_AGAIN: need to read again
 * SSL_ERROR: failed, call ossSSLGetError() & ossSSLGetErrorMessage() for reason
 * SSL_TIMEOUT: only in windows
 */
INT32 ossSSLRead(SSLHandle* handle, void* buf, INT32 num)
{
   INT32 status;
   INT32 ret = SSL_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != handle->ssl);
   SSL_ASSERT(NULL != buf);

   status = SSL_read(handle->ssl, buf, num);
   ret = _ossSSLCheckStatus(handle, status);

   if (SSL_OK != ret)
   {
      status = ret;
   }

   return status;
}

/* Return value:
 * >0: the number of bytes actually read from SSL connection
 * SSL_AGAIN: need to read again
 * SSL_ERROR: failed, call ossSSLGetError() & ossSSLGetErrorMessage() for reason
 * SSL_TIMEOUT: only in windows
 *
 * NOTE: in constrast to the ossSSLRead(), the data in the SSL buffer is unmodified
 * after the ossSSLPeek() opertaion
 */
INT32 ossSSLPeek(SSLHandle* handle, void* buf, INT32 num)
{
   INT32 status;
   INT32 ret = SSL_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != handle->ssl);
   SSL_ASSERT(NULL != buf);

   status = SSL_peek(handle->ssl, buf, num);
   ret = _ossSSLCheckStatus(handle, status);

   if (SSL_OK != ret)
   {
      status = ret;
   }

   return status;
}

/* Return value:
 * >0: the number of bytes actually write to SSL connection
 * SSL_AGAIN: need to write again
 * SSL_ERROR: failed, call ossSSLGetError() & ossSSLGetErrorMessage() for reason
 * SSL_TIMEOUT: only in windows
 */
INT32 ossSSLWrite(SSLHandle* handle, const void* buf, INT32 num)
{
   INT32 status;
   INT32 ret = SSL_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != handle->ssl);
   SSL_ASSERT(NULL != buf);

   status = SSL_write(handle->ssl, buf, num);
   ret = _ossSSLCheckStatus(handle, status);

   if (SSL_OK != ret)
   {
      status = ret;
   }

   return status;
}

/* Return value:
 * SSL_OK: the SSL connection is accepted
 * SSL_ERROR: failed, call ossSSLGetError() & ossSSLGetErrorMessage() for reason
 */
INT32 ossSSLShutdown(SSLHandle* handle)
{
   INT32 status;
   INT32 ret = SSL_OK;

   SSL_ASSERT(NULL != handle);
   SSL_ASSERT(NULL != handle->ssl);

   do
   {
      status = SSL_shutdown(handle->ssl);
      ret = _ossSSLCheckStatus(handle, status);
   } while (SSL_AGAIN == ret);

   return ret;
}

INT32 ossSSLGetError(SSLHandle* handle)
{
   SSL_ASSERT(NULL != handle);

   return handle->error;
}

char* ossSSLGetErrorMessage(INT32 error)
{
#define _MSG_LEN 256
   static OSS_THREAD_LOCAL char buf[_MSG_LEN];
   char* errorMsg;
   INT32 err = ERR_get_error();

   switch(error)
   {
   case SSL_ERROR_WANT_READ:
   case SSL_ERROR_WANT_WRITE:
      errorMsg = "possibly timed out during connect";
      break;
   case SSL_ERROR_ZERO_RETURN:
      errorMsg = "SSL network connection closed";
      break;
   case SSL_ERROR_SYSCALL:
      if (0 != err)
      {
         ERR_error_string_n(err, buf, _MSG_LEN);
         errorMsg = buf;
      }
      else
      {
         INT32 lastError;
#ifdef _WINDOWS
         lastError = WSAGetLastError();
#else
         lastError = errno;
#endif
         ossSnprintf(buf, _MSG_LEN, "the SSL BIO reported an I/O error, "
            "system errno: %d", lastError);
         errorMsg = buf;
      }
      break;
   case SSL_ERROR_SSL:
      ERR_error_string_n(err, buf, _MSG_LEN);
      errorMsg = buf;
      break;
   default:
      errorMsg = "unrecognized SSL error";
   }

   return errorMsg;
}

INT32 ossSSLERRGetError()
{
   return ERR_get_error();
}

char* ossSSLERRGetErrorMessage(INT32 error)
{
#define _MSG_LEN 256
   static OSS_THREAD_LOCAL char buf[_MSG_LEN];

   ERR_error_string_n(error, buf, _MSG_LEN);

   return buf;
}

SDB_EXTERN_C_END

#endif /* SDB_SSL */

