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

   Source File Name = utilCipher.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossMem.h"
#include "ossUtil.h"
#include "msgDef.h"
#include "openssl/des.h"
#include "openssl/sha.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BYTES_PER_TIME               8
#define KEY_BYTE_LENGTH              8
#define EIGHT_BYTE_ALIGNED_BITS      8

#define RANDOM_ARRAY_MAX_LENGTH      16
#define ARRAY_PIECE_LENGTH           RANDOM_ARRAY_MAX_LENGTH
#define INSERTABLE_MAX_LENGTH        234

#define UINT8_MAX_NUMBER             65536

#define TOKEN_MAX_LENGTH             256
#define CIPHER_STRING_MAX_LENGTH     TOKEN_MAX_LENGTH + RANDOM_ARRAY_MAX_LENGTH

#ifdef _LINUX
#define UTIL_USER_DIRECTORY            "HOME"
#else
#define UTIL_USER_DIRECTORY            "USERPROFILE"
#endif

// When reorganizing the ciphertext, 6 extra auxiliary fields are used to record
// the offset of each key in the entire ciphertext and the length of each key
#define EXTRA_AUXILIARY_FIELDS_COUNT 6

#define INVALID_HEX_VALUE   0xFFFF

static BOOLEAN _sdbIsSrand = FALSE ;

#if defined (_LINUX) || defined (_AIX)
static UINT32 _sdbRandSeed = 0 ;
#endif
static void _sdbSrand ()
{
   if ( !_sdbIsSrand )
   {
#if defined (_WINDOWS)
      srand ( (UINT32) time ( NULL ) ) ;
#elif defined (_LINUX) || defined (_AIX)
      _sdbRandSeed = time ( NULL ) ;
#endif
      _sdbIsSrand = TRUE ;
   }
}

static UINT32 _sdbRand ()
{
   UINT32 randVal = 0 ;
   if ( !_sdbIsSrand )
   {
      _sdbSrand () ;
   }
#if defined (_WINDOWS)
   rand_s ( &randVal ) ;
#elif defined (_LINUX) || defined (_AIX)
   randVal = rand_r ( &_sdbRandSeed ) ;
#endif
   return randVal ;
}

#ifdef _DEBUG
   #include <assert.h>
   #define SDB_ASSERT(cond,str)  assert(cond)
#else
   #define SDB_ASSERT(cond,str)  do{ if( !(cond)) {} } while ( 0 )
#endif // _DEBUG

static INT16 _hexChar2dec( CHAR c )
{
   INT16 i = 0 ;
   if ( c >= '0' && c <= '9' )
   {
      i = c - '0' ;
   }
   else if ( c >= 'a' && c <= 'f' )
   {
      i = 10 + c - 'a' ;
   }
   else if ( c >= 'A' && c <= 'F' )
   {
      i = 10 + c - 'A' ;
   }
   else
   {
      return INVALID_HEX_VALUE ;
   }
   return i ;
}

static void _hexToByte( const CHAR *hex, CHAR *bytes, UINT32 *byteLength )
{
  UINT32 len = ossStrlen( hex ) ;
  const CHAR* base = hex ;
  UINT32 i = 0 ;
  UINT32 pos = 0 ;

  for ( ; i < len; i += 2, pos++ )
  {
	 const CHAR* c = base + i ;
	 bytes[pos] =  ( CHAR )( ( _hexChar2dec( c[0] ) << 4 ) |
							         _hexChar2dec( c[1] ) ) ;
    ( *byteLength )++ ;
  }
}

static void _hashToKey( CHAR *cipherString, UINT32 cipherStringSize,
                        UINT8 *cipherKey, UINT32 desiredLength )
{
   UINT8 hash[SHA256_DIGEST_LENGTH] ;
   SHA256_CTX sha256 ;

   SHA256_Init( &sha256 ) ;
   SHA256_Update( &sha256, cipherString, cipherStringSize ) ;
   SHA256_Final( hash, &sha256 ) ;

   ossMemcpy( cipherKey, hash, desiredLength < SHA256_DIGEST_LENGTH ?
                               desiredLength : SHA256_DIGEST_LENGTH ) ;
}

INT32 _byteToHex( const CHAR *in, UINT32 inLen, CHAR *out, UINT32 outLen )
{
   INT32  rc = SDB_OK ;
   UINT32 i  = 0 ;
   static const char hexchars[] = "0123456789ABCDEF" ;

   SDB_ASSERT( NULL != in, "clearText can't be NULL" ) ;
   SDB_ASSERT( NULL != out, "cipherText can't be NULL" ) ;

   if ( inLen * 2 + 1 > outLen )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   for ( i = 0 ; i < inLen ; ++i )
   {
     CHAR c = in[i] ;
     CHAR high = hexchars[( c & 0xF0 ) >> 4] ;
     CHAR low = hexchars[( c & 0x0F )] ;

     out[i * 2] += high ;
     out[i * 2 + 1] += low ;
   }

done:
   return rc ;
error:
   goto done ;
}

static void _arrayRemoveElement( CHAR *array, UINT32 arrayLen, UINT32 index,
                                 UINT32 count, UINT32 *arrayNewLen )
{
   UINT32 removeLength = count > arrayLen - index ? arrayLen - index : count ;
   UINT32 copyLength = arrayLen - ( index + removeLength ) ;

   SDB_ASSERT( NULL != array, "array can't be NULL" ) ;
   SDB_ASSERT( arrayLen >= index, "index must be equal to or less than arrayLen" ) ;

   ossMemmove( array + index, array + index + removeLength, copyLength ) ;
   if ( NULL != arrayNewLen )
   {
      *arrayNewLen = arrayLen - removeLength ;
   }
}

/*
* append the 'index' started with 'count' length array in the 'appendArray'
* into the end of 'array'
*/
static void _arrayAppendElement( CHAR *array, UINT32 arrayLen,
                                 CHAR *appendArray, UINT32 index, UINT32 count,
                                 UINT32 *arrayNewLen )
{
   SDB_ASSERT( NULL != array && NULL != appendArray,
               "array & appendArray can't be NULL" ) ;

   ossMemcpy( array + arrayLen, appendArray + index, count ) ;

   if ( NULL != arrayNewLen )
   {
      *arrayNewLen = arrayLen + count ;
   }
}

static void _arrayInsertElement( CHAR *array, UINT32 arrayLen, UINT32 startPos,
                                 CHAR *appendArray, UINT32 index, UINT32 count,
                                 UINT32 *arrayNewLen )
{
   SDB_ASSERT( NULL != array && NULL != appendArray,
               "array & appendArray can't be NULL" ) ;
   SDB_ASSERT( arrayLen - startPos > 0,
               "arrayLen - startPos must be greater than 0" ) ;

   ossMemmove( array + startPos + count, array + startPos, arrayLen - startPos ) ;

   ossMemmove( array + startPos, appendArray + index, count ) ;

   if ( NULL != arrayNewLen )
   {
      *arrayNewLen += count ;
   }
}

// generate a random number betweeen begin and end(included)
static INT32 _randBetween( INT32 begin, INT32 end )
{
   return begin + ( _sdbRand() % ( end - begin + 1 ) ) ;
}

void utilCipherGenerateRandomArray( CHAR* array, UINT32 arrayLen )
{
   UINT32 i = 0 ;

   SDB_ASSERT( NULL != array, "array can't be NULL" ) ;

   for ( i = 0; i < arrayLen; i++ )
   {
     array[i] = ( CHAR )_randBetween( 1, UINT8_MAX_NUMBER ) ;
   }
}

/*
* divide randArray into 3 pieces & insert into 3 random positon in destArray
*/
void utilCipherInsertRandomArray( CHAR *destArray, UINT32 destArrayLen,
                                  CHAR *randArray, UINT32 randArrayLen,
                                  UINT32 *destArrayNewLen )
{
   UINT32       insertLowPos  = 0 ;
   UINT32       insertMidPos  = 0 ;
   UINT32       insertHighPos = 0 ;
   UINT32       insertLen     = 0 ;
   UINT32       destArrayEndIndex = destArrayLen > INSERTABLE_MAX_LENGTH ?
                                    INSERTABLE_MAX_LENGTH - 1 : destArrayLen - 1 ;
   CHAR         randArrayPiece1[RANDOM_ARRAY_MAX_LENGTH] = { '\0' } ;
   CHAR         randArrayPiece2[RANDOM_ARRAY_MAX_LENGTH] = { '\0' } ;
   CHAR         randArrayPiece3[RANDOM_ARRAY_MAX_LENGTH] = { '\0' } ;
   UINT32       randArrayEndIndex = randArrayLen - 1 ;
   UINT32       piece1Len = 0 ;
   UINT32       piece2Len = 0 ;
   UINT32       piece3Len = 0 ;
   UINT32       splitPos1 = 0 ;
   UINT32       splitPos2 = 0 ;

   SDB_ASSERT( NULL != destArray, "destArray can't be NULL" ) ;
   SDB_ASSERT( NULL != randArray, "randArray can't be NULL" ) ;
   SDB_ASSERT( 3 <= destArrayLen, "destArray should be at least 3" ) ;

   // calculate insertion positions in destArray, leave space(s) for mid and high.
   insertLowPos  = _randBetween( 0, destArrayEndIndex - 2 ) ;
   insertMidPos  = _randBetween( insertLowPos + 1, destArrayEndIndex - 1 ) ;
   insertHighPos = _randBetween( insertMidPos + 1, destArrayEndIndex ) ;

   // divide random array into 3 pieces(low, mid, high) for insertion.
   splitPos1     = _randBetween( 1, randArrayEndIndex - 2 ) ;
   splitPos2     = _randBetween( splitPos1 + 1, randArrayEndIndex - 1 ) ;

   // construct a randArray piece: randArray length + piece of randArray itself
   // + offset to next randArray length
   randArrayPiece1[piece1Len++] = splitPos1 - 0 ;
   // randArray piece
   _arrayAppendElement( randArrayPiece1, piece1Len,
                        randArray, 0, randArrayPiece1[0], &piece1Len ) ;
   // insert position should move along with growing destArray
   insertMidPos += piece1Len + 1 ;
   insertHighPos += piece1Len + 1 ;
   // offset to next piece
   randArrayPiece1[piece1Len++] = insertMidPos ;

   randArrayPiece2[piece2Len++] = splitPos2 - splitPos1 ;
   _arrayAppendElement( randArrayPiece2, piece2Len,
                        randArray, splitPos1, randArrayPiece2[0], &piece2Len ) ;
   insertHighPos += piece2Len + 1 ;
   randArrayPiece2[piece2Len++] = insertHighPos ;

   randArrayPiece3[piece3Len++] = randArrayEndIndex - splitPos2 + 1 ;
   _arrayAppendElement( randArrayPiece3, piece3Len,
                        randArray, splitPos2, randArrayPiece3[0], &piece3Len ) ;

   // insert into positions
   _arrayInsertElement( destArray, destArrayLen + insertLen, insertLowPos,
                        randArrayPiece1, 0, piece1Len,
                        &insertLen ) ;
   _arrayInsertElement( destArray, destArrayLen + insertLen, insertMidPos,
                        randArrayPiece2, 0, piece2Len,
                        &insertLen ) ;
   _arrayInsertElement( destArray, destArrayLen + insertLen, insertHighPos,
                        randArrayPiece3, 0, piece3Len,
                        &insertLen ) ;
   _arrayInsertElement( destArray, destArrayLen + insertLen, 0,
                        (CHAR *)&insertLowPos, 0, 1,
                        &insertLen ) ;

   *destArrayNewLen = destArrayLen + insertLen ;
}

INT32 utilCipherExtractRandomArray( CHAR *cipherText, UINT32 cipherTextLen,
                                    CHAR *array, UINT32 *cipherTextNewLen,
                                    UINT32 *arrayNewLen )
{
   INT32    rc            = SDB_OK ;
   UINT32   insertLowPos  = 0 ;
   UINT32   insertMidPos  = 0 ;
   UINT32   insertHighPos = 0 ;
   CHAR     randArrayPiece1[ARRAY_PIECE_LENGTH] = { '\0' } ;
   CHAR     randArrayPiece2[ARRAY_PIECE_LENGTH] = { '\0' } ;
   CHAR     randArrayPiece3[ARRAY_PIECE_LENGTH] = { '\0' } ;
   UINT8    randArrayPiece1Len = 0 ;
   UINT8    randArrayPiece2Len = 0 ;
   UINT8    randArrayPiece3Len = 0 ;

   SDB_ASSERT( NULL != cipherText && NULL != array,
               "cipherText & array can't be NULL" ) ;

   // calculate random array position in cipherText, then extract
   insertLowPos = (UINT8)cipherText[0] ;
   if ( cipherTextLen <= insertLowPos )
   {
      goto error ;
   }
   _arrayRemoveElement( cipherText, cipherTextLen, 0, 1, cipherTextNewLen ) ;

   randArrayPiece1Len = cipherText[insertLowPos] + 2;
   ossMemcpy( randArrayPiece1, cipherText + insertLowPos, randArrayPiece1Len ) ;

   insertMidPos = (UINT8)randArrayPiece1[randArrayPiece1Len - 1] ;
   if ( cipherTextLen <= insertMidPos )
   {
      goto error ;
   }
   randArrayPiece2Len = cipherText[insertMidPos] + 2 ;
   ossMemcpy( randArrayPiece2, cipherText + insertMidPos, randArrayPiece2Len ) ;

   insertHighPos = (UINT8)randArrayPiece2[randArrayPiece2Len - 1] ;
   if ( cipherTextLen <= insertHighPos )
   {
      goto error ;
   }
   randArrayPiece3Len = cipherText[insertHighPos] + 1;
   ossMemcpy( randArrayPiece3, cipherText + insertHighPos, randArrayPiece3Len ) ;

   // erase random array from cipherText.
   _arrayRemoveElement( cipherText, *cipherTextNewLen,
                        insertHighPos, randArrayPiece3Len, cipherTextNewLen ) ;
   _arrayRemoveElement( cipherText, *cipherTextNewLen,
                        insertMidPos, randArrayPiece2Len, cipherTextNewLen ) ;
   _arrayRemoveElement( cipherText, *cipherTextNewLen,
                        insertLowPos, randArrayPiece1Len, cipherTextNewLen ) ;
   cipherText[(*cipherTextNewLen)] = '\0' ;

   *arrayNewLen = 0 ;
   _arrayAppendElement( array, *arrayNewLen, randArrayPiece1, 1,
                        randArrayPiece1Len - 2, arrayNewLen ) ;
   _arrayAppendElement( array, *arrayNewLen, randArrayPiece2, 1,
                        randArrayPiece2Len - 2, arrayNewLen ) ;
   _arrayAppendElement( array, *arrayNewLen, randArrayPiece3, 1,
                        randArrayPiece3Len - 1, arrayNewLen ) ;

done:
   return rc ;
error:
   rc = SDB_SYS ;
   goto done ;
}

/*
* clearText, token are NULL terminated C strings.
* cipherText is a user provided buffer for cipherText string storage
*/
INT32 utilCipherEncrypt( const CHAR *clearText, const CHAR *token,
                         CHAR *cipherText, UINT32 cipherTextLen )
{
   INT32            rc = SDB_OK ;
   UINT32           clearTextSize = ossStrlen( clearText ) ;
   CHAR             randArray[RANDOM_ARRAY_MAX_LENGTH]     = { '\0' } ;
   CHAR             cipherString[CIPHER_STRING_MAX_LENGTH] = {'\0' } ;
   UINT32           cipherStringSize = 0 ;
   DES_cblock       keyEncrypt ;
   DES_key_schedule keySchedule ;
   const_DES_cblock inputText ;
   DES_cblock       outputText ;
   CHAR             *result        = NULL ;
   UINT32           resultLen      = 0 ;
   UINT32           resultSize     = 0 ;
   INT32            copiedTokenLen = 0 ;
   UINT32           extraBits      = 0 ; // 8-byte aligned supplementary bits
   UINT32           i              = 0 ;

   SDB_ASSERT( NULL != clearText, "clearText can't be NULL" ) ;
   SDB_ASSERT( NULL != cipherText, "cipherText can't be NULL" ) ;

   utilCipherGenerateRandomArray( randArray, RANDOM_ARRAY_MAX_LENGTH ) ;

   if ( NULL != token && 0 != ossStrlen( token ) )
   {
      INT32 tokenLen = ossStrlen( token ) ;
      copiedTokenLen = tokenLen > TOKEN_MAX_LENGTH ?
                       TOKEN_MAX_LENGTH : tokenLen ;
      ossStrncpy( cipherString, token, copiedTokenLen ) ;
      cipherStringSize += copiedTokenLen ;
   }
   _arrayAppendElement( cipherString, copiedTokenLen,
                        randArray, 0, RANDOM_ARRAY_MAX_LENGTH,
                        &cipherStringSize ) ;

   _hashToKey( cipherString, cipherStringSize,
               ( UINT8 * )&keyEncrypt, KEY_BYTE_LENGTH ) ;

   DES_set_odd_parity( &keyEncrypt ) ;
   if ( 0 > DES_set_key_checked( &keyEncrypt, &keySchedule ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( clearTextSize % BYTES_PER_TIME != 0 )
   {
      extraBits = EIGHT_BYTE_ALIGNED_BITS - ( clearTextSize % BYTES_PER_TIME ) ;
   }
   else
   {
      extraBits = 0 ;
   }
   resultLen = clearTextSize + extraBits + RANDOM_ARRAY_MAX_LENGTH +
               EXTRA_AUXILIARY_FIELDS_COUNT ;
   result = ( CHAR * )SDB_OSS_MALLOC( resultLen ) ;
   if ( NULL == result )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( result, 0, resultLen ) ;

   for ( i = 0; i < clearTextSize / BYTES_PER_TIME; i++ )
   {
      ossMemcpy( inputText, clearText + i * BYTES_PER_TIME, BYTES_PER_TIME ) ;
      DES_ecb_encrypt( &inputText, &outputText, &keySchedule, DES_ENCRYPT ) ;
      _arrayAppendElement( result, resultSize, (CHAR *)outputText, 0 ,
                           BYTES_PER_TIME, &resultSize ) ;
   }

   if ( clearTextSize % BYTES_PER_TIME != 0 )
   {
      INT32 remainTextIndex = ( clearTextSize / BYTES_PER_TIME ) *
                                BYTES_PER_TIME ;
      INT32 remainTextLen = EIGHT_BYTE_ALIGNED_BITS - extraBits ;

      // padding using 0s
      ossMemset( inputText, 0, BYTES_PER_TIME ) ;
      ossMemcpy( inputText, clearText + remainTextIndex, remainTextLen ) ;
      DES_ecb_encrypt( &inputText, &outputText, &keySchedule, DES_ENCRYPT ) ;
      _arrayAppendElement( result, resultSize, (CHAR *)outputText, 0 ,
                           BYTES_PER_TIME, &resultSize ) ;
   }

   utilCipherInsertRandomArray( result, resultSize, randArray,
                                RANDOM_ARRAY_MAX_LENGTH, &resultSize ) ;

   // serialize
   rc = _byteToHex( result, resultSize, cipherText, cipherTextLen ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   if ( result )
   {
      SDB_OSS_FREE( result ) ;
      result = NULL ;
   }
   return rc ;
error:
   goto done ;
}

INT32 utilCipherDecrypt( const CHAR *cipherText, const CHAR *token,
                         CHAR *clearText )
{
   INT32              rc = SDB_OK ;
   CHAR               randArray[RANDOM_ARRAY_MAX_LENGTH] = {'\0'} ;
   UINT32             randArraySize = 0 ;
   CHAR               *passwdEncrypted = NULL ;
   UINT32             passwdEncryptedSize = 0 ;
   UINT32             passwdLen = 0 ;
   CHAR               *passwd = NULL ;
   CHAR               cipherString[CIPHER_STRING_MAX_LENGTH] = {'\0'} ;
   UINT32             cipherStringSize = 0 ;
   DES_cblock         keyEncrypt ;
   DES_key_schedule   keySchedule ;
   const_DES_cblock   inputText ;
   DES_cblock         outputText ;
   UINT32             posInClearText = 0 ;
   UINT32             clearTextSize = 0 ;
   UINT32             i = 0 ;
   UINT32             j = 0 ;

   if ( NULL == cipherText )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   passwdEncrypted = ( CHAR * )SDB_OSS_MALLOC( ossStrlen( cipherText ) / 2 ) ;
   if ( NULL == passwdEncrypted )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _hexToByte( cipherText, passwdEncrypted, &passwdEncryptedSize ) ;

   rc = utilCipherExtractRandomArray( passwdEncrypted, passwdEncryptedSize,
                                      randArray, &passwdEncryptedSize,
                                      &randArraySize ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( NULL != token && 0 != ossStrlen( token ) )
   {
      ossStrncpy( cipherString, token,
                  CIPHER_STRING_MAX_LENGTH - randArraySize  ) ;
      cipherStringSize += ossStrlen( token ) ;
   }
   _arrayAppendElement( cipherString, NULL == token ? 0 : ossStrlen( token ),
                        randArray, 0, randArraySize, &cipherStringSize ) ;

   _hashToKey( cipherString, cipherStringSize,
               ( UINT8 * )&keyEncrypt, KEY_BYTE_LENGTH ) ;

   passwdLen = passwdEncryptedSize ;
   passwd = passwdEncrypted ;

   DES_set_odd_parity( &keyEncrypt ) ;
   if ( 0 > DES_set_key_checked( &keyEncrypt, &keySchedule ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   for ( i = 0; i < passwdLen / BYTES_PER_TIME; i++ )
   {

      ossMemcpy( inputText, passwd + i * BYTES_PER_TIME, BYTES_PER_TIME ) ;
      DES_ecb_encrypt( &inputText, &outputText, &keySchedule, DES_DECRYPT ) ;

      for ( j = 0; j < BYTES_PER_TIME; j++ )
      {
         _arrayAppendElement( clearText, clearTextSize,
                              ( CHAR * )&outputText[j], 0, 1,
                              &clearTextSize ) ;
      }
   }

   // remove trailing padding 0s
   posInClearText = clearTextSize - 1 ;
   while ( 0 == clearText[posInClearText] )
   {
      posInClearText-- ;
   }
   if ( ( clearTextSize - 1 ) != posInClearText )
   {
      clearText[posInClearText + 1] = '\0' ;
   }

done:
   if ( passwdEncrypted )
   {
      SDB_OSS_FREE( passwdEncrypted ) ;
      passwdEncrypted = NULL ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _readn( INT32 fd, void *vptr, size_t n )
{
   size_t  nleft ;
   INT32   nread ;
   CHAR   *ptr ;
   ptr = ( CHAR *)vptr ;
   nleft = n ;

   while ( nleft > 0 )
   {
#if defined (_LINUX)
      if ( ( nread = read( fd, ptr, nleft ) ) < 0 )
#elif defined (_WINDOWS)
      if ( ( nread = _read( fd, ptr, nleft ) ) < 0 )
#endif
      {
         if ( errno == EINTR )
            nread = 0 ;      /* and call read() again */
         else
            return -1 ;
      }
      else if ( nread == 0 )
         break ;              /* EOF */

      nleft -= nread ;
      ptr += nread ;
   }

   return ( n - nleft ) ;         /* return >= 0 */
}

INT32 _readFile( const CHAR *path, CHAR **fileContent, UINT32 *readLength )
{
   INT32  rc = SDB_OK ;
   off_t  fileSize = 0 ;
   INT32  fd ;
   UINT32 nleft = 0 ;
#if defined (_LINUX)
   struct stat stat  ;
#elif defined (_WINDOWS)
   struct _stat stat ;
#endif

#if defined (_LINUX)
   fd = open( path, O_RDONLY ) ;
   if ( -1 == fd )
   {
      goto error ;
   }

   if ( -1 == fstat( fd, &stat ) )
   {
      goto error ;
   }

#elif defined (_WINDOWS)
   fd = _open( path, O_RDONLY ) ;
   if ( -1 == fd )
   {
      goto error ;
   }

   if ( -1 == _fstat( fd, &stat ) )
   {
      goto error ;
   }
#endif

   fileSize = stat.st_size ;
   if( 0 == fileSize )
   {
      *readLength = 0;
      goto done;
   }

   *fileContent = ( CHAR * )SDB_OSS_MALLOC( fileSize ) ;
   if ( NULL == *fileContent )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   nleft = _readn( fd, ( void * )*fileContent, fileSize ) ;
   if ( 0 > nleft )
   {
      goto error ;
   }

   *readLength = nleft ;

done:
   if ( -1 != fd )
   {
#if defined (_LINUX)
      close( fd ) ;
#elif defined (_WINDOWS)
      _close( fd ) ;
#endif
   }
   return rc ;

error:
   if ( SDB_OK == rc )
   {
      switch ( errno )
      {
      case ENOENT:
         rc = SDB_FNE ;
         break ;
      case EACCES:
      case EPERM:
         rc = SDB_PERM ;
         break ;
      case ENOTDIR:
      case EINVAL:
         rc = SDB_INVALIDARG ;
         break ;
      case EIO:
         rc = SDB_IO ;
         break ;
      default :
         rc = SDB_SYS ;
         break ;
      }
   }
   goto done ;
}

void _extractUserName( const CHAR *user, CHAR *userName, CHAR *fullName )
{
   CHAR   *atPos = 0 ;

   SDB_ASSERT( NULL != user, "user can't be null" ) ;

   atPos = ossStrchr( user, '@' ) ;

   if ( NULL != atPos )
   {
      ossStrncpy( userName, user, atPos - user ) ;
   }
   else
   {
      ossStrncpy( userName, user, ossStrlen( user ) ) ;
   }

   ossStrncpy( fullName, user, ossStrlen( user ) ) ;
}

INT32 utilDecryptUserCipher( const CHAR *user, const CHAR *token,
                             const CHAR *path, CHAR *connectionUser,
                             CHAR *clearText )
{
   INT32  rc                 = SDB_OK ;
   INT32  foundFullNameCount = 0 ;
   INT32  foundHalfNameCount = 0 ;
   UINT32 fileLength         = 0 ;
   CHAR   *fileContent       = NULL ;
   CHAR   *foundNewline      = NULL ;
   CHAR   *atPos             = NULL ;
   CHAR   *colonPos          = NULL ;
   CHAR   *startPosition     = NULL ;
   CHAR   *matchedPosition   = NULL ;
   UINT32 matchedLength      = 0 ;
   UINT32 fileFullNameLen    = 0 ;
   UINT32 fileUserNameLen    = 0 ;
   UINT32 fullNameLen        = 0 ;
   UINT32 userNameLen        = 0 ;
   CHAR   filePath[OSS_MAX_PATHSIZE]        = { '\0' };
   CHAR   userName[SDB_MAX_USERNAME_LENGTH] = { '\0' } ;
   CHAR   fullName[SDB_MAX_USERNAME_LENGTH] = { '\0' } ;

   if ( NULL == user || NULL == path || NULL == clearText )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   _extractUserName( user, userName, fullName ) ;
   ossStrncpy( connectionUser, userName, SDB_MAX_USERNAME_LENGTH ) ;

   if ( '~' == *path && '/' == *(path + 1) )
   {
      ossStrncpy( filePath, getenv(UTIL_USER_DIRECTORY), OSS_MAX_PATHSIZE ) ;
      path++;
      ossStrncat( filePath, path, ossStrlen(path) );
   }
   else
   {
      ossStrncpy( filePath, path, OSS_MAX_PATHSIZE ) ;
   }

   rc = _readFile( filePath, &fileContent, &fileLength ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   fullNameLen = ossStrlen( fullName ) ;
   userNameLen = ossStrlen( userName ) ;

   startPosition = fileContent ;
   while ( fileLength > 0 )
   {
      foundNewline = ( CHAR * )memchr( startPosition, '\n', fileLength ) ;
      if ( NULL != foundNewline )
      {
         colonPos = ( CHAR * )memchr( startPosition, ':',
                                      foundNewline - startPosition ) ;
         atPos = ( CHAR * )memchr( startPosition, '@',
                                   foundNewline - startPosition ) ;
         if ( NULL == colonPos )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         fileFullNameLen = colonPos - startPosition ;
         if ( NULL != atPos )
         {
            fileUserNameLen = atPos - startPosition ;
         }

         if ( 0 == memcmp( startPosition, fullName,
                           fileFullNameLen > fullNameLen ?
                           fileFullNameLen : fullNameLen ) )
         {
            foundFullNameCount++ ;
            matchedPosition = colonPos + 1 ;
            matchedLength = foundNewline - matchedPosition ;
            break ;
         }
         else if ( NULL != atPos &&
                   0 == memcmp( startPosition, userName,
                                fileUserNameLen > userNameLen ?
                                fileUserNameLen : userNameLen ) )
         {
            foundHalfNameCount++ ;
            matchedPosition = colonPos + 1 ;
            matchedLength = foundNewline - matchedPosition ;
         }
      }
      else
      {
         break ;
      }

      fileLength -= ( foundNewline + 1 ) - startPosition ;
      startPosition = foundNewline + 1 ;
   }

   if ( 1 == foundFullNameCount || 1 == foundHalfNameCount )
   {
      matchedPosition[matchedLength] = '\0' ;

      rc = utilCipherDecrypt( matchedPosition, token, clearText ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else if ( 1 < foundHalfNameCount )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else
   {
      rc = SDB_AUTH_USER_NOT_EXIST ;
      goto error ;
   }

done:
   if ( fileContent )
   {
      SDB_OSS_FREE( fileContent ) ;
      fileContent = NULL ;
   }
   return rc ;
error:
   goto done ;
}

