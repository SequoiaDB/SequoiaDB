/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilAuthSCRAMSHA.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          16/04/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilAuthSCRAMSHA.hpp"
#include "msgDef.h"
#include "utilString.hpp"

#define UITL_AUTH_SYMBOL_COLON               ":"
#define UITL_AUTH_SYMBOL_COMMA               ","
#define UITL_AUTH_SYMBOL_EQUAL               "="
#define UITL_AUTH_MSG_SYMBOL_USERNAME        "n"
#define UITL_AUTH_MSG_SYMBOL_RANDOM          "r"
#define UITL_AUTH_MSG_SYMBOL_SALT            "s"
#define UITL_AUTH_MSG_SYMBOL_ITERATIONCOUNT  "i"
#define UITL_AUTH_MSG_SYMBOL_IDENTIFY        "c"

/** \fn static INT32 buildAuthMsg( const CHAR* username,
                                   const CHAR* combineNonceBase64,
                                   const CHAR* saltBase64,
                                   UINT32 iterationCount,
                                   const CHAR* identify,
                                   string &authMsg )
    \brief Build auth message in the format of mongodb.
    \param [in] username User name.
    \param [in] combineNonceBase64 Commbine nonce in base64 format.
    \param [in] saltBase64 Random salt in base64 format.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] identify Session identifier. When the client is C++ driver, its
                value is "C++_Session". When the client is C driver, its
                value is "C_Session".
    \param [out] authMsg Auth msg we will build.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
static INT32 buildAuthMsg( const CHAR *username,
                           const CHAR *combineNonceBase64,
                           const CHAR *saltBase64,
                           UINT32 iterationCount,
                           const CHAR *identify,
                           string &authMsg )
{
   INT32 rc = SDB_OK ;
   engine::_utilString<> clientNonceBase64 ;
   INT32 len = 0 ;

   if ( NULL == username           ||
        NULL == combineNonceBase64 ||
        NULL == saltBase64         ||
        NULL == identify )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   len = ossStrlen( combineNonceBase64 ) ;
   if ( len <= UTIL_AUTH_SCRAMSHA_NONCE_BASE64_LEN )
   {
      // combineNonce = clientNonce(maybe 24 or 32 byte) + serverNonce(32byte)
      // So lenght of combineNonce must greater than 32
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   clientNonceBase64.append( combineNonceBase64,
                             len - UTIL_AUTH_SCRAMSHA_NONCE_BASE64_LEN ) ;

   // The format of authMsg is as follows:
   // "n=username,r=clientNonceBase64
   // ,r=clientNonceBase64+serverNonceBase64,s=saltBase64,i=iterationCount
   // ,c=identify,r=clientNonceBase64+serverNonceBase64"

   try
   {
      stringstream ss ;
      ss << UITL_AUTH_MSG_SYMBOL_USERNAME UITL_AUTH_SYMBOL_EQUAL << username
         << UITL_AUTH_SYMBOL_COMMA
         << UITL_AUTH_MSG_SYMBOL_RANDOM UITL_AUTH_SYMBOL_EQUAL << clientNonceBase64.str()
         << UITL_AUTH_SYMBOL_COMMA
         << UITL_AUTH_MSG_SYMBOL_RANDOM UITL_AUTH_SYMBOL_EQUAL << combineNonceBase64
         << UITL_AUTH_SYMBOL_COMMA
         << UITL_AUTH_MSG_SYMBOL_SALT UITL_AUTH_SYMBOL_EQUAL << saltBase64
         << UITL_AUTH_SYMBOL_COMMA
         << UITL_AUTH_MSG_SYMBOL_ITERATIONCOUNT UITL_AUTH_SYMBOL_EQUAL << iterationCount
         << UITL_AUTH_SYMBOL_COMMA
         << UITL_AUTH_MSG_SYMBOL_IDENTIFY  UITL_AUTH_SYMBOL_EQUAL << identify
         << UITL_AUTH_SYMBOL_COMMA
         << UITL_AUTH_MSG_SYMBOL_RANDOM UITL_AUTH_SYMBOL_EQUAL << combineNonceBase64 ;
      authMsg = ss.str() ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn static INT32 buildSdbAuthMsg( const CHAR* username,
                                      const CHAR* combineNonceBase64,
                                      const CHAR* saltBase64,
                                      UINT32 iterationCount,
                                      const CHAR* identify,
                                      string &authMsg )
    \brief Build auth message in the format of sequoiadb.
    \param [in] username User name.
    \param [in] combineNonceBase64 Commbine nonce in base64 format.
    \param [in] saltBase64 Random salt in base64 format.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] identify Session identifier. When the client is C++ driver, its
                value is "C++_Session". When the client is C driver, its
                value is "C_Session".
    \param [out] authMsg Auth msg we will build.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
static INT32 buildSdbAuthMsg( const CHAR *username,
                              const CHAR *combineNonceBase64,
                              const CHAR *saltBase64,
                              UINT32 iterationCount,
                              const CHAR *identify,
                              string &authMsg )
{
   INT32 rc = SDB_OK ;
   CHAR clientNonceBase64[ UTIL_AUTH_SCRAMSHA_NONCE_BASE64_LEN + 1 ] = { 0 } ;


   if ( NULL == username           ||
        NULL == combineNonceBase64 ||
        NULL == saltBase64         ||
        NULL == identify           ||
        0 == ossStrlen( combineNonceBase64 ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossStrncpy( clientNonceBase64,
               combineNonceBase64,
               UTIL_AUTH_SCRAMSHA_NONCE_BASE64_LEN ) ;

   // The format of authMsg is as follows:
   // "User:<username>,Nonce:<clientNonce>,
   // IterationCount:<iterationCount>,Salt:<salt>,Nonce:<commbineNonce>,
   // Identify:<identify>,Nonce:<commbineNonce>"

   try
   {
      stringstream ss ;
      ss << SDB_AUTH_USER           UITL_AUTH_SYMBOL_COLON << username
         << UITL_AUTH_SYMBOL_COMMA
         << SDB_AUTH_NONCE          UITL_AUTH_SYMBOL_COLON << clientNonceBase64
         << UITL_AUTH_SYMBOL_COMMA
         << SDB_AUTH_ITERATIONCOUNT UITL_AUTH_SYMBOL_COLON << iterationCount
         << UITL_AUTH_SYMBOL_COMMA
         << SDB_AUTH_SALT           UITL_AUTH_SYMBOL_COLON << saltBase64
         << UITL_AUTH_SYMBOL_COMMA
         << SDB_AUTH_NONCE          UITL_AUTH_SYMBOL_COLON << combineNonceBase64
         << UITL_AUTH_SYMBOL_COMMA
         << SDB_AUTH_IDENTIFY       UITL_AUTH_SYMBOL_COLON << identify
         << UITL_AUTH_SYMBOL_COMMA
         << SDB_AUTH_NONCE          UITL_AUTH_SYMBOL_COLON << combineNonceBase64 ;
      authMsg = ss.str() ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}


/** \fn INT32 utilAuthCaculateKey( const CHAR *originalPassword,
                                   const BYTE *salt, INT32 saltLen,
                                   UINT32 iterationCount,
                                   string &storedKeyBase64,
                                   string &serverKeyBase64,
                                   string &clientKeyBase64 )
    \brief Caculate StoredKey, ServerKey and ClientKey by SCRAM-SHA-256.
    \param [in] originalPassword Original password. It may be
                clear text password or the MD5 of clear text password.
    \param [in] salt The random salt.
    \param [in] saltLen Length of the random salt.
    \param [in] iterationCount Number of encryption iterations.
    \param [out] storedKeyBase64 StoredKey in base64 we will caculate.
    \param [out] serverKeyBase64 ServerKey in base64 we will caculate.
    \param [out] clientKeyBase64 ClientKey in base64 we will caculate.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthCaculateKey( const CHAR *originalPassword,
                           const BYTE *salt, INT32 saltLen,
                           UINT32 iterationCount,
                           string &storedKeyBase64,
                           string &serverKeyBase64,
                           string &clientKeyBase64 )
{
   INT32 rc = SDB_OK ;
   BYTE startKeyNew[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]     = { 0 } ;
   BYTE saltPassword[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]    = { 0 } ;
   BYTE saltPasswordTmp[UTIL_AUTH_SCRAMSHA256_HASH_SIZE] = { 0 } ;
   BYTE startKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]        = { 0 } ;
   BYTE clientKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]       = { 0 } ;
   BYTE storedKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]       = { 0 } ;
   BYTE serverKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]       = { 0 } ;

   if ( NULL == originalPassword ||
        NULL == salt ||
        UTIL_AUTH_SCRAMSHA256_SALT_LEN != saltLen )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossMemcpy( startKey, salt, saltLen ) ;

   startKey[ saltLen ] = 0 ;
   startKey[ saltLen + 1 ] = 0 ;
   startKey[ saltLen + 2 ] = 0 ;
   startKey[ saltLen + 3 ] = 1 ;

   try
   {
      HMAC( EVP_sha256(),
            originalPassword, ossStrlen( originalPassword ),
            startKey, sizeof(startKey),
            saltPasswordTmp, NULL ) ;

      ossMemcpy( saltPassword, saltPasswordTmp, sizeof(saltPasswordTmp) ) ;
      ossMemcpy( startKeyNew, saltPasswordTmp, sizeof(saltPasswordTmp) ) ;

      for ( UINT32 i = 1; i < iterationCount; i++ )
      {
         HMAC( EVP_sha256(),
               originalPassword, ossStrlen( originalPassword ),
               startKeyNew, sizeof(startKeyNew),
               saltPasswordTmp, NULL ) ;

         for ( UINT32 j = 0; j < UTIL_AUTH_SCRAMSHA256_HASH_SIZE; j++ )
         {
            saltPassword[j] ^= saltPasswordTmp[j] ;
         }

         ossMemcpy( startKeyNew, saltPasswordTmp,
                    UTIL_AUTH_SCRAMSHA256_HASH_SIZE ) ;
      }

      // ClientKey = HMAC( saltedPassword, "Client Key" )
      HMAC( EVP_sha256(),
            saltPassword, sizeof(saltPassword),
            (BYTE*)UTIL_AUTH_CLIENT_KEY, strlen( UTIL_AUTH_CLIENT_KEY ),
            clientKey, NULL ) ;

      // ServerKey = HMAC( saltedPassword, "Server Key" )
      HMAC( EVP_sha256(),
            saltPassword, sizeof(saltPassword),
            (BYTE*)UTIL_AUTH_SERVER_KEY, strlen( UTIL_AUTH_SERVER_KEY ),
            serverKey, NULL ) ;

      // StoredKey = SHA256( ClientKey )
      SHA256( clientKey, sizeof(clientKey), storedKey ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      serverKeyBase64 = base64::encode( (CHAR*)serverKey, sizeof(serverKey) ) ;
      storedKeyBase64 = base64::encode( (CHAR*)storedKey, sizeof(storedKey) ) ;
      clientKeyBase64 = base64::encode( (CHAR*)clientKey, sizeof(clientKey) ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthCaculateKey1( const CHAR *originalPassword,
                                    const BYTE *salt, INT32 saltLen,
                                    UINT32 iterationCount,
                                    string &storedKeyBase64,
                                    string &serverKeyBase64,
                                    string &clientKeyBase64 )
    \brief Caculate StoredKey, ServerKey and ClientKey by SCRAM-SHA-1.
    \param [in] originalPassword Original password. It may be
                clear text password or the MD5 of clear text password.
    \param [in] salt The random salt.
    \param [in] saltLen Length of the random salt.
    \param [in] iterationCount Number of encryption iterations.
    \param [out] storedKeyBase64 StoredKey in base64 we will caculate.
    \param [out] serverKeyBase64 ServerKey in base64 we will caculate.
    \param [out] clientKeyBase64 ClientKey in base64 we will caculate.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthCaculateKey1( const CHAR *originalPassword,
                            const BYTE *salt, INT32 saltLen,
                            UINT32 iterationCount,
                            string &storedKeyBase64,
                            string &serverKeyBase64,
                            string &clientKeyBase64 )
{
   INT32 rc = SDB_OK ;
   BYTE startKeyNew[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]     = { 0 } ;
   BYTE saltPassword[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]    = { 0 } ;
   BYTE saltPasswordTmp[UTIL_AUTH_SCRAMSHA1_HASH_SIZE] = { 0 } ;
   BYTE startKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]        = { 0 } ;
   BYTE clientKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]       = { 0 } ;
   BYTE storedKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]       = { 0 } ;
   BYTE serverKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]       = { 0 } ;

   if ( NULL == originalPassword || NULL == salt ||
        UTIL_AUTH_SCRAMSHA1_SALT_LEN != saltLen )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossMemcpy( startKey, salt, saltLen ) ;

   startKey[ saltLen ] = 0 ;
   startKey[ saltLen + 1 ] = 0 ;
   startKey[ saltLen + 2 ] = 0 ;
   startKey[ saltLen + 3 ] = 1 ;

   try
   {
      HMAC( EVP_sha1(),
            originalPassword, ossStrlen( originalPassword ),
            startKey, sizeof(startKey),
            saltPasswordTmp, NULL ) ;

      ossMemcpy( saltPassword, saltPasswordTmp, sizeof(saltPasswordTmp) ) ;
      ossMemcpy( startKeyNew, saltPasswordTmp, sizeof(saltPasswordTmp) ) ;

      for ( UINT32 i = 1; i < iterationCount; i++ )
      {
         HMAC( EVP_sha1(),
               originalPassword, ossStrlen( originalPassword ),
               startKeyNew, sizeof(startKeyNew),
               saltPasswordTmp, NULL ) ;

         for ( UINT32 j = 0; j < UTIL_AUTH_SCRAMSHA1_HASH_SIZE; j++ )
         {
            saltPassword[j] ^= saltPasswordTmp[j] ;
         }

         ossMemcpy( startKeyNew, saltPasswordTmp,
                    UTIL_AUTH_SCRAMSHA1_HASH_SIZE ) ;
      }

      // ClientKey = HMAC( saltedPassword, "Client Key" )
      HMAC( EVP_sha1(),
            saltPassword, sizeof(saltPassword),
            (BYTE*)UTIL_AUTH_CLIENT_KEY, strlen( UTIL_AUTH_CLIENT_KEY ),
            clientKey, NULL ) ;

      // ServerKey = HMAC( saltedPassword, "Server Key" )
      HMAC( EVP_sha1(),
            saltPassword, sizeof(saltPassword),
            (BYTE*)UTIL_AUTH_SERVER_KEY, strlen( UTIL_AUTH_SERVER_KEY ),
            serverKey, NULL ) ;

      // StoredKey = SHA256( ClientKey )
      SHA1( clientKey, sizeof(clientKey), storedKey ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      serverKeyBase64 = base64::encode( (CHAR*)serverKey, sizeof(serverKey) ) ;
      storedKeyBase64 = base64::encode( (CHAR*)storedKey, sizeof(storedKey) ) ;
      clientKeyBase64 = base64::encode( (CHAR*)clientKey, sizeof(clientKey) ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthCaculateClientProof( const CHAR *originalPassword,
                                           const CHAR *username,
                                           UINT32 iterationCount,
                                           const CHAR *saltBase64,
                                           const CHAR *combineNonceBase64,
                                           const CHAR *identify,
                                           BOOLEAN fromSdb,
                                           string &clientProofBase64 )
    \brief Caculate client proof by SCRAM-SHA-256.
    \param [in] originalPassword The original password. It may be
                clear text password or the MD5 of clear text password.
    \param [in] username User name.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] saltBase64 Random salt in base64.
    \param [in] combineNonceBase64 Combine nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [in] fromSdb Build sdb auth message or mongodb auth message.
    \param [out] clientProofBase64 Client proof in base64 we will caculate.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthCaculateClientProof( const CHAR *originalPassword,
                                   const CHAR *username,
                                   UINT32 iterationCount,
                                   const CHAR *saltBase64,
                                   const CHAR *combineNonceBase64,
                                   const CHAR *identify,
                                   BOOLEAN fromSdb,
                                   string &clientProofBase64 )
{
   INT32 rc = SDB_OK ;
   BYTE clientProof[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]     = { 0 } ;
   BYTE clientSignature[UTIL_AUTH_SCRAMSHA256_HASH_SIZE] = { 0 } ;
   BYTE storedKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]       = { 0 } ;
   BYTE clientKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]       = { 0 } ;
   BYTE salt[UTIL_AUTH_SCRAMSHA256_SALT_LEN]             = { 0 } ;
   string decodeStr ;
   string storedKeyBase64 ;
   string serverKeyBase64 ;
   string clientKeyBase64 ;
   string authMsg ;

   if ( NULL == originalPassword ||
        NULL == username ||
        NULL == saltBase64 ||
        NULL == combineNonceBase64 ||
        NULL == identify ||
        UTIL_AUTH_SCRAMSHA256_SALT_BASE64_LEN != ossStrlen(saltBase64) ||
        0 == ossStrlen(combineNonceBase64) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( string(saltBase64) ) ;
      ossMemcpy( salt, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   rc = utilAuthCaculateKey( originalPassword, salt, sizeof(salt),
                             iterationCount, storedKeyBase64, serverKeyBase64,
                             clientKeyBase64 ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( storedKeyBase64 ) ;
      ossMemcpy( storedKey, decodeStr.c_str(), decodeStr.length() ) ;
      decodeStr = base64::decode( clientKeyBase64 ) ;
      ossMemcpy( clientKey, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( fromSdb )
   {
      rc = buildSdbAuthMsg( username, combineNonceBase64,
                            saltBase64, iterationCount, identify, authMsg ) ;
   }
   else
   {
      rc = buildAuthMsg( username, combineNonceBase64,
                         saltBase64, iterationCount, identify, authMsg ) ;
   }
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // ClientSignature = HMAC( StoredKey, AuthMsg )
      HMAC( EVP_sha256(),
            storedKey, sizeof(storedKey),
            (BYTE*)(authMsg.c_str()), authMsg.length(),
            clientSignature, NULL ) ;

      // ClientProof = ClientSignature ^ ClientKey
      for ( UINT32 i = 0; i < UTIL_AUTH_SCRAMSHA256_HASH_SIZE; i++ )
      {
         clientProof[i] = clientSignature[i] ^ clientKey[i] ;
      }
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      clientProofBase64 = base64::encode( (CHAR*)clientProof,
                                          sizeof(clientProof) ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthCaculateServerProof( const CHAR *username,
                                           UINT32 iterationCount,
                                           const CHAR *saltBase64,
                                           const CHAR *combineNonceBase64,
                                           const CHAR *identify,
                                           const CHAR *serverKeyBase64,
                                           BOOLEAN fromSdb,
                                           string &serverProofBase64 )
    \brief Caculate server proof by SCRAM-SHA-256.
    \param [in] username User name.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] saltBase64 Random salt in base64.
    \param [in] combineNonceBase64 Combine nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [in] serverKeyBase64 ServerKey in base64.
    \param [in] fromSdb Build sdb auth message or mongodb auth message.
    \param [out] serverProofBase64 Server proof in base64 we will caculate.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthCaculateServerProof( const CHAR *username,
                                   UINT32 iterationCount,
                                   const CHAR *saltBase64,
                                   const CHAR *combineNonceBase64,
                                   const CHAR *identify,
                                   const CHAR *serverKeyBase64,
                                   BOOLEAN fromSdb,
                                   string &serverProofBase64 )
{
   INT32 rc = SDB_OK ;
   BYTE serverKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]   = { 0 } ;
   BYTE serverProof[UTIL_AUTH_SCRAMSHA256_HASH_SIZE] = { 0 } ;
   string decodeStr ;
   string authMsg ;

   if ( NULL == username ||
        NULL == saltBase64 ||
        NULL == combineNonceBase64 ||
        NULL == identify ||
        NULL == serverKeyBase64 ||
        UTIL_AUTH_SCRAMSHA256_SALT_BASE64_LEN != ossStrlen(saltBase64) ||
        0 == ossStrlen(combineNonceBase64) ||
        UTIL_AUTH_SCRAMSHA256_HASH_BASE64_SIZE != ossStrlen(serverKeyBase64) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( serverKeyBase64 ) ;
      ossMemcpy( serverKey, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( fromSdb )
   {
      rc = buildSdbAuthMsg( username, combineNonceBase64,
                            saltBase64, iterationCount, identify, authMsg ) ;
   }
   else
   {
      rc = buildAuthMsg( username, combineNonceBase64,
                         saltBase64, iterationCount, identify, authMsg ) ;
   }
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // ServerProof = HMAC( ServerKey, AuthMsg )
      HMAC( EVP_sha256(),
            serverKey, sizeof(serverKey),
            (BYTE*)(authMsg.c_str()), authMsg.length(),
            serverProof, NULL ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      serverProofBase64 = base64::encode( (CHAR*)serverProof,
                                          sizeof(serverProof) ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthCaculateServerProof1( const CHAR *username,
                                            UINT32 iterationCount,
                                            const CHAR *saltBase64,
                                            const CHAR *combineNonceBase64,
                                            const CHAR *identify,
                                            const CHAR *serverKeyBase64,
                                            string &serverProofBase64 )
    \brief Caculate server proof by SCRAM-SHA-1.
    \param [in] username User name.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] saltBase64 Random salt in base64.
    \param [in] combineNonceBase64 Combine nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [in] serverKeyBase64 ServerKey in base64.
    \param [out] serverProofBase64 Server proof in base64 we will caculate.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthCaculateServerProof1( const CHAR *username,
                                    UINT32 iterationCount,
                                    const CHAR *saltBase64,
                                    const CHAR *combineNonceBase64,
                                    const CHAR *identify,
                                    const CHAR *serverKeyBase64,
                                    string &serverProofBase64 )
{
   INT32 rc = SDB_OK ;
   BYTE serverKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]   = { 0 } ;
   BYTE serverProof[UTIL_AUTH_SCRAMSHA1_HASH_SIZE] = { 0 } ;
   string decodeStr ;
   string authMsg ;

   if ( NULL == username ||
        NULL == saltBase64 ||
        NULL == combineNonceBase64 ||
        NULL == identify ||
        NULL == serverKeyBase64 ||
        UTIL_AUTH_SCRAMSHA1_SALT_BASE64_LEN  != ossStrlen(saltBase64) ||
        UTIL_AUTH_SCRAMSHA1_HASH_BASE64_SIZE != ossStrlen(serverKeyBase64) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( serverKeyBase64 ) ;
      ossMemcpy( serverKey, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   rc = buildAuthMsg( username, combineNonceBase64,
                      saltBase64, iterationCount, identify, authMsg ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // ServerProof = HMAC( ServerKey, AuthMsg )
      HMAC( EVP_sha1(),
            serverKey, sizeof(serverKey),
            (BYTE*)(authMsg.c_str()), authMsg.length(),
            serverProof, NULL ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      serverProofBase64 = base64::encode( (CHAR*)serverProof,
                                          sizeof(serverProof) ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthGenerateNonce( BYTE *nonce, UINT32 nonceLen )
    \brief Generate a random BYTE string of the specified length .
    \param [in] nonceLen Length of the random UINT8 string.
    \param [out] nonce Random BYTE string.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthGenerateNonce( BYTE *nonce, UINT32 nonceLen )
{
   INT32 rc = SDB_OK ;
   UINT64 n = 0 ;

   if ( NULL == nonce || nonceLen <= 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      for ( UINT32 i = 0; i < nonceLen; i++ )
      {
         #if defined(_WIN32)
           unsigned int a=0, b=0;
           rand_s(&a);
           rand_s(&b);
           n = (((unsigned long long)a)<<32) | b;
         #else
           n = (((unsigned long long)random())<<32) | random();
         #endif
         nonce[i] = n ;
      }
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthVerifyClientProof( const CHAR *clientProofBase64,
                                         const CHAR *username,
                                         UINT32 iterationCount,
                                         const CHAR *saltBase64,
                                         const CHAR *combineNonceBase64,
                                         const CHAR *identify,
                                         const CHAR *storedKeyBase64,
                                         BOOLEAN fromSdb,
                                         BOOLEAN &isValid )
    \brief Check if client proof is valid by SCRAM-SHA-256.
    \param [in] clientProofBase64 Client proof in base64.
    \param [in] username User name.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] saltBase64 Random salt in base64.
    \param [in] combineNonceBase64 Combine nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [in] storedKeyBase64 StoredKey that stored in
                SYSAUTH.SYSUSRS in base64.
    \param [in] fromSdb Build sdb auth message or mongodb auth message.
    \param [out] isValid Whether client proof is valid.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthVerifyClientProof( const CHAR *clientProofBase64,
                                 const CHAR *username,
                                 UINT32 iterationCount,
                                 const CHAR *saltBase64,
                                 const CHAR *combineNonceBase64,
                                 const CHAR *identify,
                                 const CHAR *storedKeyBase64,
                                 BOOLEAN fromSdb,
                                 BOOLEAN &isValid )
{
   INT32 rc = SDB_OK ;
   // StoredKey caculated by client proof
   BYTE externStoredKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE] = { 0 } ;
   BYTE clientSignature[UTIL_AUTH_SCRAMSHA256_HASH_SIZE] = { 0 } ;
   BYTE clientKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]       = { 0 } ;
   BYTE clientProof[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]     = { 0 } ;
   BYTE innerStoredKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]  = { 0 } ;
   string decodeStr ;
   string authMsg ;
   isValid = TRUE ;

   if ( NULL == clientProofBase64 ||
        NULL == username ||
        NULL == saltBase64 ||
        NULL == combineNonceBase64 ||
        NULL == identify ||
        NULL == storedKeyBase64 ||
        UTIL_AUTH_SCRAMSHA256_HASH_BASE64_SIZE != ossStrlen(clientProofBase64) ||
        UTIL_AUTH_SCRAMSHA256_SALT_BASE64_LEN != ossStrlen(saltBase64) ||
        0 == ossStrlen(combineNonceBase64) ||
        UTIL_AUTH_SCRAMSHA256_HASH_BASE64_SIZE != ossStrlen(storedKeyBase64) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( string(clientProofBase64) ) ;
      ossMemcpy( clientProof, decodeStr.c_str(), decodeStr.length() ) ;
      decodeStr = base64::decode( string(storedKeyBase64) ) ;
      ossMemcpy( innerStoredKey, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( fromSdb )
   {
      rc = buildSdbAuthMsg( username, combineNonceBase64,
                            saltBase64, iterationCount, identify, authMsg ) ;
   }
   else
   {
      rc = buildAuthMsg( username, combineNonceBase64,
                         saltBase64, iterationCount, identify, authMsg ) ;
   }
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // ClientSignature = HMAC( StoredKey, AuthMsg )
      HMAC( EVP_sha256(),
            innerStoredKey, sizeof(innerStoredKey),
            (BYTE*)(authMsg.c_str()), authMsg.length(),
            clientSignature, NULL ) ;

      // ClientKey = ClientProof XOR ClientSignature
      for ( UINT32 i = 0; i < UTIL_AUTH_SCRAMSHA256_HASH_SIZE; i++)
      {
         clientKey[i] = ( clientProof[i] ^ clientSignature[i] ) ;
      }

      // StoredKey = H( ClientKey )
      SHA256( clientKey, sizeof(clientKey), externStoredKey ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   for ( UINT32 i = 0; i < UTIL_AUTH_SCRAMSHA256_HASH_SIZE; i++ )
   {
      if ( externStoredKey[i] != innerStoredKey[i] )
      {
         isValid = FALSE ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthVerifyClientProof1( const CHAR *clientProofBase64,
                                          const CHAR *username,
                                          UINT32 iterationCount,
                                          const CHAR *saltBase64,
                                          const CHAR *combineNonceBase64,
                                          const CHAR *identify,
                                          const CHAR *storedKeyBase64,
                                          BOOLEAN &isValid )
    \brief Check if client proof is valid by SCRAM-SHA-1.
    \param [in] clientProofBase64 Client proof in base64.
    \param [in] username User name.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] saltBase64 Random salt in base64.
    \param [in] combineNonceBase64 Combine nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [in] storedKeyBase64 StoredKey that stored in
                SYSAUTH.SYSUSRS in base64.
    \param [out] isValid Whether client proof is valid.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthVerifyClientProof1( const CHAR *clientProofBase64,
                                  const CHAR *username,
                                  UINT32 iterationCount,
                                  const CHAR *saltBase64,
                                  const CHAR *combineNonceBase64,
                                  const CHAR *identify,
                                  const CHAR *storedKeyBase64,
                                  BOOLEAN &isValid )
{
   INT32 rc = SDB_OK ;
   // StoredKey caculated by client proof
   BYTE externStoredKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE] = { 0 } ;
   BYTE clientSignature[UTIL_AUTH_SCRAMSHA1_HASH_SIZE] = { 0 } ;
   BYTE clientKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]       = { 0 } ;
   BYTE clientProof[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]     = { 0 } ;
   BYTE innerStoredKey[UTIL_AUTH_SCRAMSHA1_HASH_SIZE]  = { 0 } ;
   string decodeStr ;
   string authMsg ;
   isValid = TRUE ;

   if ( NULL == clientProofBase64 ||
        NULL == username ||
        NULL == saltBase64 ||
        NULL == combineNonceBase64 ||
        NULL == identify ||
        NULL == storedKeyBase64 ||
        UTIL_AUTH_SCRAMSHA1_HASH_BASE64_SIZE != ossStrlen(clientProofBase64) ||
        UTIL_AUTH_SCRAMSHA1_SALT_BASE64_LEN  != ossStrlen(saltBase64) ||
        UTIL_AUTH_SCRAMSHA1_HASH_BASE64_SIZE != ossStrlen(storedKeyBase64) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( string(clientProofBase64) ) ;
      ossMemcpy( clientProof, decodeStr.c_str(), decodeStr.length() ) ;
      decodeStr = base64::decode( string(storedKeyBase64) ) ;
      ossMemcpy( innerStoredKey, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   rc = buildAuthMsg( username, combineNonceBase64,
                      saltBase64, iterationCount, identify, authMsg ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // ClientSignature = HMAC( StoredKey, AuthMsg )
      HMAC( EVP_sha1(),
            innerStoredKey, sizeof(innerStoredKey),
            (BYTE*)(authMsg.c_str()), authMsg.length(),
            clientSignature, NULL ) ;

      // ClientKey = ClientProof XOR ClientSignature
      for ( UINT32 i = 0; i < UTIL_AUTH_SCRAMSHA1_HASH_SIZE; i++)
      {
         clientKey[i] = ( clientProof[i] ^ clientSignature[i] ) ;
      }

      // StoredKey = H( ClientKey )
      SHA1( clientKey, sizeof(clientKey), externStoredKey ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   for ( UINT32 i = 0; i < UTIL_AUTH_SCRAMSHA1_HASH_SIZE; i++ )
   {
      if ( externStoredKey[i] != innerStoredKey[i] )
      {
         isValid = FALSE ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 utilAuthVerifyServerProof( const CHAR *serverProofBase64,
                                         const CHAR *username,
                                         UINT32 iterationCount,
                                         const CHAR *saltBase64,
                                         const CHAR *combineNonceBase64,
                                         const CHAR *identify,
                                         const CHAR *serverKeyBase64,
                                         BOOLEAN fromSdb,
                                         BOOLEAN &isValid )
    \brief Check if server proof is valid by SCRAM-SHA-256.
    \param [in] serverProofBase64 Server proof in base64 that caculated
                by server.
    \param [in] username User name.
    \param [in] iterationCount Number of encryption iterations.
    \param [in] saltBase64 Random salt in base64.
    \param [in] combineNonceBase64 Combine nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [in] serverKeyBase64 Serverkey that stored in
                SYSAUTH.SYSUSRS in base64.
    \param [in] fromSdb Build sdb auth message or mongodb auth message.
    \param [out] isValid Whether server proof is valid.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 utilAuthVerifyServerProof( const CHAR *serverProofBase64,
                                 const CHAR *username,
                                 UINT32 iterationCount,
                                 const CHAR *saltBase64,
                                 const CHAR *combineNonceBase64,
                                 const CHAR *identify,
                                 const CHAR *serverKeyBase64,
                                 BOOLEAN fromSdb,
                                 BOOLEAN &isValid )
{
   INT32 rc = SDB_OK ;
   // The server proof caculated by inner ServerKey
   BYTE innerServerProof[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]  = { 0 } ;
   BYTE externServerProof[UTIL_AUTH_SCRAMSHA256_HASH_SIZE] = { 0 } ;
   BYTE innerServerKey[UTIL_AUTH_SCRAMSHA256_HASH_SIZE]    = { 0 } ;
   string decodeStr ;
   string authMsg ;
   isValid = TRUE ;

   if ( NULL == serverProofBase64 ||
        NULL == username ||
        NULL == saltBase64 ||
        NULL == combineNonceBase64 ||
        NULL == identify ||
        NULL == serverKeyBase64 ||
        UTIL_AUTH_SCRAMSHA256_HASH_BASE64_SIZE != ossStrlen(serverProofBase64) ||
        UTIL_AUTH_SCRAMSHA256_SALT_BASE64_LEN != ossStrlen(saltBase64) ||
        0 == ossStrlen(combineNonceBase64) ||
        UTIL_AUTH_SCRAMSHA256_HASH_BASE64_SIZE != ossStrlen(serverKeyBase64) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   try
   {
      decodeStr = base64::decode( string(serverProofBase64) ) ;
      ossMemcpy( externServerProof, decodeStr.c_str(), decodeStr.length() ) ;
      decodeStr = base64::decode( string(serverKeyBase64) ) ;
      ossMemcpy( innerServerKey, decodeStr.c_str(), decodeStr.length() ) ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( fromSdb )
   {
      rc = buildSdbAuthMsg( username, combineNonceBase64,
                            saltBase64, iterationCount, identify, authMsg ) ;
   }
   else
   {
      rc = buildAuthMsg( username, combineNonceBase64,
                         saltBase64, iterationCount, identify, authMsg ) ;
   }
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // ServerProof = HMAC( ServerKey, AuthMsg )
      HMAC( EVP_sha256(),
            innerServerKey, sizeof(innerServerKey),
            (BYTE*)(authMsg.c_str()), authMsg.length(),
            innerServerProof, NULL ) ;
   }
   catch( std::exception )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   for ( UINT32 i = 0; i < UTIL_AUTH_SCRAMSHA256_HASH_SIZE; i++ )
   {
      if ( externServerProof[i] != innerServerProof[i] )
      {
         isValid = FALSE ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}
