/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = common.c

   Descriptive Name = C Common

   When/how to use: this program may be used on binary and text-formatted
   versions of C Client component. This file contains functions for
   client common functions ( including package build/extract ).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "common.h"
#include "ossMem.h"
#include "ossUtil.h"
#include "msgCatalogDef.h"
#include "oss.h"
#include "msg.h"
#include "../bson/lib/md5.h"

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif
#define CACHED_CHECK_TIME_INTERVAL 300
#define MAX_CACHE_SLOT_NUMBER      1000
#define CLINET_CS_NAME_SIZE        300

static const INT32 clientDefaultVersion = CATALOG_DEFAULT_VERSION ;
static const INT16 clientDefaultW = 0 ;
static const UINT64 clientDefaultRouteID = 0 ;
static const SINT32 clientDefaultFlags = 0 ;
static BOOLEAN cacheEnabled = TRUE ;
static UINT32  cachedTimeInterval = 300 ;   // default is 300 seconds
static UINT32  maxCachedSlotCount = 1000 ;

/*
   Local functions
*/
static INT32 clientCheckBuffer ( CHAR **ppBuffer,
                                 INT32 *bufferSize,
                                 INT32 packetLength )
{
   INT32 rc = SDB_OK ;

   if ( packetLength > *bufferSize )
   {
      CHAR *pOld = *ppBuffer ;
      INT32 newSize = ossRoundUpToMultipleX ( packetLength, SDB_PAGE_SIZE ) ;
      if ( newSize < 0 )
      {
         ossPrintf ( "new buffer overflow"OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      *ppBuffer = (CHAR*)SDB_OSS_REALLOC ( *ppBuffer, sizeof(CHAR)*(newSize)) ;
      if ( !*ppBuffer )
      {
         ossPrintf ( "Failed to allocate %d bytes send buffer"OSS_NEWLINE,
                     newSize ) ;
         rc = SDB_OOM ;
         *ppBuffer = pOld ;
         goto error ;
      }
      *bufferSize = newSize ;
   }
done :
   return rc ;
error :
   goto done ;
}

// "l2r" means "local to remote", when the bson is send to engine, l2r should
// be true, when we receive get a bson from engine, 12r should be false.
static BOOLEAN bson_endian_convert ( CHAR *data, off_t *off, BOOLEAN l2r )
{
   // object size after conversion
   INT32 objaftersize = 0 ;
   // object size before conversion
   INT32 objbeforesize = *(INT32*)&data[*off] ;
   // offset before conversion, this is used for sanity check by end of function
   off_t beginOff = *off ;
   INT32 objrealsize = 0 ;
   // convert from before size to after-size
   ossEndianConvert4 ( objbeforesize, objaftersize ) ;
   // assign size to after-size
   *(INT32*)&data[*off] = objaftersize ;
   // the real size
   objrealsize = l2r?objbeforesize:objaftersize ;
   // increase offset by 4
   *off += sizeof(INT32) ;
   // loop until hitting '\0; for end of bson
   while ( BSON_EOO != data[*off] )
   {
      // get bson element type
      CHAR type = data[*off] ;
      // move offset to next in order to skip type
      *off += sizeof(CHAR) ;
      // skip element name, note element name is a string ended up with '\0'
      *off += ossStrlen ( &data[*off] ) + 1 ;
      switch ( type )
      {
      case BSON_DOUBLE :
      {
         FLOAT64 value = 0 ;
         ossEndianConvert8 ( *(FLOAT64*)&data[*off], value ) ;
         *(FLOAT64*)&data[*off] = value ;
         *off += sizeof(FLOAT64) ;
         break ;
      }
      case BSON_STRING :
      case BSON_CODE :
      case BSON_SYMBOL :
      {
         // for those 3 types, there are 4 bytes length plus a string
         // the length is the length of string plus '\0'
         INT32 len = *(INT32*)&data[*off] ;
         INT32 newlen = 0 ;
         ossEndianConvert4 ( len, newlen ) ;
         *(INT32*)&data[*off] = newlen ;
         // 4 for length, then string with '\0'
         *off += sizeof(INT32) + (l2r?len:newlen) ;
         break ;
      }
      case BSON_OBJECT :
      case BSON_ARRAY :
      {
         BOOLEAN rc = bson_endian_convert ( data, off, l2r ) ;
         if ( !rc )
            return rc ;
         break ;
      }
      case BSON_BINDATA :
      {
         // for bindata, there are 4 bytes length, 1 byte subtype and data
         // note length is the real length of data
         INT32 len = *(INT32*)&data[*off] ;
         INT32 newlen ;
         ossEndianConvert4 ( len, newlen ) ;
         *(INT32*)&data[*off] = newlen ;
         // 4 for length, 1 for subtype, then data
         *off += sizeof(INT32) + sizeof(CHAR) + (l2r?len:newlen) ;
         break ;
      }
      case BSON_UNDEFINED :
      case BSON_NULL :
      case BSON_MINKEY :
      case BSON_MAXKEY :
      {
         // nothing in those types
         break ;
      }
      case BSON_OID :
      {
         // do we need to do conversion for this one? let's skip it first
         *off += 12 ;
         break ;
      }
      case BSON_BOOL :
      {
         // one byte for true or false
         *off += 1 ;
         break ;
      }
      case BSON_DATE :
      {
         INT64 date = *(INT64*)&data[*off] ;
         INT64 newdate = 0 ;
         ossEndianConvert8 ( date, newdate ) ;
         *(INT64*)&data[*off] = newdate ;
         *off += sizeof(INT64) ;
         break ;
      }
      case BSON_REGEX :
      {
         // two cstring, each with string
         // for regex
         *off += ossStrlen ( &data[*off] ) + 1 ;
         // for options
         *off += ossStrlen ( &data[*off] ) + 1 ;
         break ;
      }
      case BSON_DBREF :
      {
         // dbref is 4 bytes lenth + string + 12 bytes
         INT32 len = *(INT32*)&data[*off] ;
         INT32 newlen = 0 ;
         ossEndianConvert4 ( len, newlen ) ;
         *(INT32*)&data[*off] = newlen ;
         // 4 for length, then string, note len already included '\0' ;
         *off += sizeof(INT32) + (l2r?len:newlen) ;
         *off += 12 ;
         break ;
      }
      case BSON_CODEWSCOPE :
      {
         // 4 bytes and 4 bytes + string, then obj
         INT32 value = 0 ;
         INT32 len, newlen ;
         BOOLEAN rc ;
         ossEndianConvert4 ( *(INT32*)&data[*off], value ) ;
         *(INT32*)&data[*off] = value ;
         *off += sizeof(INT32) ;
         // then string
         len = *(INT32*)&data[*off] ;
         newlen = 0 ;
         ossEndianConvert4 ( len, newlen ) ;
         *(INT32*)&data[*off] = newlen ;
         *off += sizeof(INT32) + (l2r?len:newlen) ;
         // then object
         rc = bson_endian_convert ( &data[*off], off, l2r ) ;
         if ( !rc )
            return rc ;
         break ;
      }
      case BSON_INT :
      {
         INT32 value = 0 ;
         ossEndianConvert4 ( *(INT32*)&data[*off], value ) ;
         *(INT32*)&data[*off] = value ;
         *off += sizeof(INT32) ;
         break ;
      }
      case BSON_TIMESTAMP :
      {
         // timestamp is with 2 4-bytes
         INT32 value = 0 ;
         ossEndianConvert4 ( *(INT32*)&data[*off], value ) ;
         *(INT32*)&data[*off] = value ;
         *off += sizeof(INT32) ;
         ossEndianConvert4 ( *(INT32*)&data[*off], value ) ;
         *(INT32*)&data[*off] = value ;
         *off += sizeof(INT32) ;
         break ;
      }
      case BSON_LONG :
      {
         INT64 value = 0 ;
         ossEndianConvert8 ( *(INT64*)&data[*off], value ) ;
         *(INT64*)&data[*off] = value ;
         *off += sizeof(INT64) ;
         break ;
      }
      case BSON_DECIMAL :
      {
         INT32 size    = 0 ;
         INT32 newSize = 0 ;
         INT32 value4  = 0 ;
         INT16 value2  = 0 ;
         INT32 i       = 0 ;
         INT32 ndigits = 0 ;
         // size
         size = *(INT32*)&data[*off] ;
         ossEndianConvert4 ( size, newSize ) ;
         *(INT32*)&data[*off] = newSize ;
         *off += sizeof(INT32) ;

         // typemod
         ossEndianConvert4 ( *(INT32*)&data[*off], value4 ) ;
         *(INT32*)&data[*off] = value4 ;
         *off += sizeof(INT32) ;

         // scale
         ossEndianConvert2 ( *(INT16*)&data[*off], value2 ) ;
         *(INT16*)&data[*off] = value2 ;
         *off += sizeof(INT16) ;

         // weight
         ossEndianConvert2 ( *(INT16*)&data[*off], value2 ) ;
         *(INT16*)&data[*off] = value2 ;
         *off += sizeof(INT16) ;

         ndigits = ( ( l2r?size:newSize ) - SDB_DECIMAL_HEADER_SIZE ) /
                   ( sizeof( INT16 ) ) ;
         for ( i = 0 ; i < ndigits; i++ )
         {
            ossEndianConvert2 ( *(INT16*)&data[*off], value2 ) ;
            *(INT16*)&data[*off] = value2 ;
            *off += sizeof(INT16) ;
         }
      }
      } // switch
   } // while ( BSON_EOO != data[*off] )
   // jump off BSON_EOO at end of bson
   *off += sizeof(CHAR) ;
   if ( *off - beginOff != objrealsize )
      return FALSE ;
   return TRUE ;
}

INT32 hash_table_create_node( const CHAR *key, htbNode **node )
{
   INT32 rc = SDB_OK ;
   CHAR *ptr =  NULL ;
   *node = NULL ;
   if ( NULL == key )
   {
      goto done ;
   }

   ptr = ( CHAR * )SDB_OSS_MALLOC( sizeof( htbNode ) ) ;
   if ( NULL == ptr )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   *node = ( htbNode *)ptr ;
   ossMemset ( *node, 0, sizeof( htbNode ) ) ;
   ptr = NULL ;

   ptr = ( CHAR * )SDB_OSS_MALLOC( ossStrlen( key ) + 1 ) ;
   if ( NULL == ptr )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossStrcpy( ptr, key ) ;
   (*node)->name = ptr ;

done:
   return rc ;
error:
   if ( NULL != *node )
   {
      if ( NULL != (*node)->name )
      {
         SDB_OSS_FREE( (*node)->name ) ;
         (*node)->name = NULL ;
      }
      SDB_OSS_FREE( *node ) ;
      *node = NULL ;
   }
   goto done ;
}

INT32 hash_table_destroy_node( htbNode **node )
{
   if ( NULL == node || NULL == *node )
   {
      goto done ;
   }

   if ( NULL != (*node)->name )
   {
      SDB_OSS_FREE( (*node)->name ) ;
      (*node)->name = NULL ;
   }
   SDB_OSS_FREE ( *node ) ;
   *node = NULL ;

done:
   return SDB_OK ;
}

INT32 hash_table_insert( hashTable *tb, htbNode *node )
{
   UINT32 hashV = 0 ;
   UINT32 locate = 0 ;

   if ( NULL == tb || NULL == tb->node )
   {
      goto done ;
   }

   if ( NULL == node )
   {
      goto done ;
   }

   if ( NULL == node->name )
   {
      goto done ;
   }

   hashV = ossHash( node->name, ossStrlen( node->name ) ) ;
   locate = hashV % tb->capacity ;

   if ( NULL != tb->node[ locate ] )
   {
      htbNode *toFree = tb->node[ locate ] ;
      hash_table_destroy_node( &toFree ) ;
   }

   tb->node[ locate ] = node ;

done:
   return SDB_OK ;
}

INT32 hash_table_remove( hashTable *tb, const CHAR *key, BOOLEAN dropCS )
{
   UINT32 hashV = 0 ;
   UINT32 locate = 0 ;
   UINT32 index = 0 ;

   if ( NULL == tb || NULL == tb->node )
   {
      goto done ;
   }

   if ( NULL == key )
   {
      goto done ;
   }

   if ( !dropCS )
   {
      hashV = ossHash( key, strlen( key ) ) ;
      locate = hashV % tb->capacity ;

      if ( NULL != tb->node[ locate ] )
      {
         htbNode *toFree = tb->node[ locate ] ;
         if ( NULL == tb->node[ locate ]->name ||
              0 == ossStrcmp( toFree->name, key ) )
         {
            hash_table_destroy_node( &toFree ) ;
            tb->node[ locate ] = NULL ;
         }
      }
   }
   else
   {
      for ( ; index < tb->capacity ; ++index )
      {
         htbNode *toFree = tb->node[ index ] ;
         if ( NULL != toFree )
         {
            if ( NULL == tb->node[ index ]->name ||
                 0 == ossStrncmp( toFree->name, key, ossStrlen( key ) ) )
            {
               hash_table_destroy_node( &toFree ) ;
               tb->node[ index ] = NULL ;
            }
         }
      }
   }

done:
   return SDB_OK ;
}

INT32 hash_table_fetch( hashTable *tb, const CHAR *key, htbNode **node )
{
   UINT32 hashV = 0 ;
   UINT32 locate = 0 ;

   if ( NULL == tb || NULL == tb->node )
   {
      goto done ;
   }

   if ( NULL == key )
   {
      goto done ;
   }

   hashV = ossHash( key, strlen( key ) ) ;
   locate = hashV % tb->capacity ;

   if (   NULL != tb->node[ locate ] &&
        ( NULL != tb->node[ locate ]->name &&
          0 == ossStrcmp( tb->node[ locate ]->name, key ) ) )
   {
      *node = tb->node[ locate ] ;
   }
   else
   {
      *node = NULL ;
   }

done:
   return SDB_OK ;
}

INT32 hash_table_create( hashTable **tb, const UINT32 bucketSize )
{
   INT32 rc = SDB_OK ;
   CHAR *ptr = NULL ;

   if ( !cacheEnabled )
   {
      goto done ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   ptr = ( CHAR * )SDB_OSS_MALLOC( sizeof(hashTable) ) ;
   if ( NULL == ptr )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   *tb = ( hashTable * )ptr ;
   (*tb)->capacity = bucketSize ;
   (*tb)->node = NULL ;

   ptr = ( CHAR * )SDB_OSS_MALLOC( sizeof(htbNode *) * bucketSize ) ;
   if ( NULL == ptr )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( ptr, 0, sizeof(htbNode *) * bucketSize ) ;
   (*tb)->node = (htbNode **)ptr ;

done:
   return rc ;
error:
   if ( NULL != (*tb) )
   {
      if (NULL != (*tb)->node )
      {
         SDB_OSS_FREE( (*tb)->node ) ;
         (*tb)->node = NULL ;
      }

      SDB_OSS_FREE( *tb ) ;
      *tb = NULL ;
   }

   goto done ;
}

INT32 hash_table_destroy( hashTable **tb )
{
   UINT32 idx = 0 ;
   if ( !cacheEnabled )
   {
      goto done ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   if ( NULL == *tb )
   {
      goto done ;
   }

   for ( ; idx < (*tb)->capacity ; ++idx )
   {
      htbNode *node = ( htbNode * )( (*tb)->node[ idx ] ) ;
      if ( NULL != node )
      {
         hash_table_destroy_node( &node ) ;
      }
   }

   SDB_OSS_FREE( (*tb)->node ) ;
   SDB_OSS_FREE( *tb ) ;
   (*tb) = NULL ;

done:
   return SDB_OK ;
}

INT32 insertCachedObject( hashTable *tb, const CHAR *key )
{
   return insertCachedVersion( tb, key, clientDefaultVersion ) ;
}

INT32 insertCachedVersion( hashTable *tb, const CHAR *key, INT32 version )
{
   INT32 rc       = SDB_OK ;
   htbNode *node  = NULL ;
   UINT64 curTime = 0 ;

   if ( !cacheEnabled )
   {
      goto done ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   if ( NULL == key )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = hash_table_create_node( key, &node ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( NULL == node )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   curTime = (UINT64)time( NULL ) ;
   node->lastTime = curTime ;
   node->version  = version;

   rc = hash_table_insert( tb, node ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   if ( NULL == node )
   {
      hash_table_destroy_node( &node ) ;
   }
   goto done ;
}

INT32 removeCachedObject( hashTable *tb, const CHAR *key, BOOLEAN dropCS )
{
   INT32 rc = SDB_OK ;

   if ( !cacheEnabled )
   {
      goto done ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   if ( NULL == key )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = hash_table_remove( tb, key, dropCS ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

BOOLEAN fetchCachedObject ( hashTable *tb, const CHAR *key )
{
   INT32 version = CATALOG_INVALID_VERSION ;

   return fetchCachedVersion( tb,key,&version ) ;
}

BOOLEAN fetchCachedVersion( hashTable *tb, const CHAR *key,INT32* pVersion )
{
   INT32 rc       = SDB_OK ;
   htbNode *node  = NULL ;
   UINT64 curTime = 0 ;

   *pVersion  = CATALOG_INVALID_VERSION;

   if ( !cacheEnabled )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   if ( NULL == key )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = hash_table_fetch( tb, key, &node ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   curTime = (UINT64)time( NULL ) ;
   if ( NULL != node && curTime - node->lastTime < cachedTimeInterval )
   {
      *pVersion = node->version ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }

done:
   return ( SDB_OK == rc ) ;
error:
   goto done ;
}

INT32 updateCachedObject( const INT32 code, hashTable *tb, const CHAR *key )
{
   return updateCachedVersion( code, tb, key, clientDefaultVersion ) ;
}

INT32 updateCachedVersion( const INT32 code, hashTable *tb, const CHAR *key, INT32 version )
{
   INT32 rc       = SDB_OK ;
   htbNode *node  = NULL ;
   UINT64 curTime = 0 ;
   CHAR *pos      = NULL ;
   CHAR csName[ CLINET_CS_NAME_SIZE + 1 ] = { 0 } ;

   if ( !cacheEnabled )
   {
      goto done ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   if ( NULL == key )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( SDB_DMS_NOTEXIST == code )
   {
      removeCachedObject( tb, key, FALSE ) ;
   }
   else if ( SDB_DMS_CS_NOTEXIST == code )
   {
      pos = ossStrchr( key, '.' ) ;
      if ( NULL != pos )
      {
         UINT32 csLen = pos - key ;
         if ( csLen > CLINET_CS_NAME_SIZE )
         {
            csLen = CLINET_CS_NAME_SIZE ;
         }
         ossStrncpy( csName, key, csLen ) ;
      }
      else
      {
         ossStrncpy( csName, key, CLINET_CS_NAME_SIZE ) ;
      }
      removeCachedObject( tb, csName, TRUE ) ;
   }
   else if ( SDB_OK == code || SDB_CLIENT_CATA_VER_OLD == code )
   {
      pos = ossStrchr( key, '.' ) ;
      if ( NULL != pos )
      {
         // update collection space in cache
         UINT32 csLen = pos - key ;
         if ( csLen > CLINET_CS_NAME_SIZE )
         {
            csLen = CLINET_CS_NAME_SIZE ;
         }
         ossStrncpy( csName, key, csLen ) ;
         rc = hash_table_fetch( tb, csName, &node ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( NULL == node )
         {
            rc = insertCachedVersion( tb, csName, version ) ;
         }
         else
         {
            curTime = (UINT64)time( NULL ) ;
            node->lastTime = curTime ;
            node->version  = version ;
         }
      }
      // update collection in cache
      rc = hash_table_fetch( tb, key, &node ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL == node )
      {
         rc = insertCachedVersion( tb, key, version ) ;
      }
      else
      {
         curTime = (UINT64)time( NULL ) ;
         node->lastTime = curTime ;
         node->version  = version ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 initCacheStrategy( BOOLEAN enableCacheStrategy,
                         const UINT32 timeInterval )
{
   if ( enableCacheStrategy )
   {
      cacheEnabled = TRUE ;
      cachedTimeInterval = ( ( 0 != timeInterval ) ?
                             timeInterval : CACHED_CHECK_TIME_INTERVAL ) ;
   }
   else
   {
      cacheEnabled = FALSE ;
      cachedTimeInterval  = CACHED_CHECK_TIME_INTERVAL ;
   }

   return SDB_OK ;
}

INT32 initHashTable( hashTable **tb )
{
   INT32 rc = SDB_OK ;
   if ( !cacheEnabled )
   {
      goto done ;
   }

   if ( NULL == tb )
   {
      goto done ;
   }

   if ( NULL != *tb )
   {
      hash_table_destroy( tb ) ;
   }

   rc = hash_table_create( tb, maxCachedSlotCount ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 releaseHashTable( hashTable **tb )
{
   INT32 rc = SDB_OK ;

   if ( NULL == tb )
   {
      goto done ;
   }

   rc = hash_table_destroy( tb ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/*
   _flagMappingItem define
*/
struct _flagMappingItem
{
   UINT32 _original ;
   UINT32 _new ;
} ;
typedef struct _flagMappingItem flagMappingItem ;

/*
   regulateQueryFlags implement
*/
INT32 regulateQueryFlags( INT32 flags )
{
   INT32 newFlags = flags ;
   UINT32 i = 0 ;

   static const flagMappingItem __mapping[] = {
      // add mapping flags as below, if necessary:
      { 0, 0 }
   } ;
   static const UINT32 __mappingSize = sizeof( __mapping ) /
                                       sizeof( flagMappingItem ) ;

   for ( i = 0 ; i < __mappingSize ; i++ )
   {
      if ( ( __mapping[i]._new != __mapping[i]._original ) &&
           ( __mapping[i]._original & flags ))
      {
         newFlags &= ~( __mapping[i]._original ) ;
         newFlags |= __mapping[i]._new ;
      }
   }

   return newFlags ;
}

INT32 eraseSingleFlag( INT32 flags, INT32 erasedFlag )
{
    INT32 newFlags = flags ;
    if ( ( newFlags & erasedFlag ) != 0 )
    {
       newFlags &= ~erasedFlag ;
    }
    return newFlags ;
}

static void clientEndianConvertHeader ( MsgHeader *pHeader )
{
   MsgHeader newheader ;
   ossEndianConvert4 ( pHeader->messageLength, newheader.messageLength ) ;
   ossEndianConvert4 ( pHeader->opCode, newheader.opCode ) ;
   ossEndianConvert4 ( pHeader->TID, newheader.TID ) ;
   ossEndianConvert8 ( pHeader->routeID, newheader.routeID ) ;
   ossEndianConvert8 ( pHeader->requestID, newheader.requestID ) ;
   ossMemcpy ( pHeader, &newheader, sizeof(newheader) ) ;
}

INT32 clientCheckRetMsgHeader( const CHAR *pSendBuf, const CHAR *pRecvBuf,
                               BOOLEAN endianConvert )
{
   INT32 rc          = SDB_OK ;
   INT32 tmpOpCode   = 0 ;
   INT32 sendOpCode  = 0 ;
   INT32 recvOpCode = 0 ;
   if ( NULL == pSendBuf || NULL == pRecvBuf )
   {
      rc = SDB_INVALIDARG ;
      goto done ;
   }
   tmpOpCode = ((MsgHeader*)pSendBuf)->opCode ;
   ossEndianConvertIf ( tmpOpCode, sendOpCode, endianConvert ) ;
   recvOpCode = (((MsgHeader*)pRecvBuf)->opCode) ;
   if ( MAKE_REPLY_TYPE( sendOpCode ) != recvOpCode )
   {
      rc = SDB_UNEXPECTED_RESULT ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

/*
   Message functions
*/
INT32 clientBuildUpdateMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName, SINT32 flag,
                             UINT64 reqID,
                             bson *selector, bson *updator,
                             bson *hint, BOOLEAN endianConvert )
{
   bson emptyObj ;
   INT32 packetLength   = 0 ;
   INT32 rc             = SDB_OK ;
   MsgOpUpdate *pUpdate = NULL ;
   INT32 offset         = 0 ;
   INT32 nameLength     = 0 ;
   INT32 opCode         = MSG_BS_UPDATE_REQ ;
   UINT32 tid           = ossGetCurrentThreadID() ;

   bson_init ( &emptyObj ) ;
   bson_empty ( &emptyObj ) ;
   if ( !selector )
   {
      selector = &emptyObj ;
   }
   if ( !updator )
   {
      updator = &emptyObj ;
   }
   if ( !hint )
   {
      hint = &emptyObj ;
   }

   packetLength = ossRoundUpToMultipleX( offsetof(MsgOpUpdate, name) +
                                         ossStrlen ( CollectionName ) + 1,
                                         4 ) +
                  ossRoundUpToMultipleX( bson_size(selector), 4 ) +
                  ossRoundUpToMultipleX( bson_size(updator), 4 ) +
                  ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // now the buffer is large enough
   pUpdate = (MsgOpUpdate*)(*ppBuffer) ;
   ossEndianConvertIf ( flag, pUpdate->flags, endianConvert ) ;
   ossEndianConvertIf ( clientDefaultVersion, pUpdate->version,
                        endianConvert ) ;
   ossEndianConvertIf ( clientDefaultW, pUpdate->w, endianConvert ) ;
   // nameLength does NOT include '\0'
   nameLength = ossStrlen ( CollectionName ) ;
   ossEndianConvertIf ( nameLength, pUpdate->nameLength, endianConvert ) ;
   // header assignment
   pUpdate->header.requestID           = reqID ;
   pUpdate->header.opCode              = opCode ;
   pUpdate->header.messageLength       = packetLength ;
   pUpdate->header.eye                 = MSG_COMM_EYE_DEFAULT ;
   pUpdate->header.version             = SDB_PROTOCOL_VER_2 ;
   pUpdate->header.flags               = FLAG_RESULT_DETAIL ;
   pUpdate->header.routeID.value       = clientDefaultRouteID ;
   pUpdate->header.TID                 = tid ;
   ossMemset( &(pUpdate->header.globalID), 0, sizeof(pUpdate->header.globalID) ) ;
   ossMemset( pUpdate->header.reserve, 0, sizeof(pUpdate->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pUpdate->name, CollectionName, nameLength ) ;
   pUpdate->name[nameLength] = 0 ;
   // get the offset of the first bson obj
   offset = ossRoundUpToMultipleX( offsetof(MsgOpUpdate, name) +
                                   nameLength + 1,
                                   4 ) ;
   if ( !endianConvert )
   {
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(selector),
                                          bson_size(selector));
      offset += ossRoundUpToMultipleX( bson_size(selector), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(updator),
                                          bson_size(updator));
      offset += ossRoundUpToMultipleX( bson_size(updator), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(hint),
                                          bson_size(hint));
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   }
   else
   {
      bson newselector ;
      bson newupdator ;
      bson newhint ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &pUpdate->header ) ;
      bson_init ( &newselector ) ;
      bson_init ( &newupdator ) ;
      bson_init ( &newhint ) ;

      rc = bson_copy ( &newselector, selector ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      rc = bson_copy ( &newupdator, updator ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      rc = bson_copy ( &newhint, hint ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newselector), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newupdator), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newhint), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newselector),
                                          bson_size(selector));
      offset += ossRoundUpToMultipleX( bson_size(selector), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newupdator),
                                          bson_size(updator));
      offset += ossRoundUpToMultipleX( bson_size(updator), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newhint),
                                          bson_size(hint));
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;

endian_convert_done :
      bson_destroy ( &newselector ) ;
      bson_destroy ( &newupdator ) ;
      bson_destroy ( &newhint ) ;

      if ( rc )
      {
         goto error ;
      }
   }
   // sanity test
   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

static INT32 _clientAppendObj2Buff( CHAR *pBuffer, INT32 buffLen,
                                    INT32 offset, const bson *object,
                                    BOOLEAN endianConvert, INT32 *nextOffset )
{
   INT32 rc = SDB_OK ;
   bson newObj ;
   BOOLEAN bsonInit = FALSE ;
   INT32 appendSize = 0 ;
   INT32 objSize = bson_size( object ) ;

   if ( offset + objSize > buffLen )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( endianConvert )
   {
      off_t off = 0 ;
      bson_init( &newObj ) ;
      bsonInit = TRUE ;
      rc = bson_copy( &newObj, object ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !bson_endian_convert ( (char*)bson_data(&newObj), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemcpy ( &( pBuffer[ offset ] ), bson_data( &newObj ),
                  bson_size( &newObj ) );
      appendSize = ossRoundUpToMultipleX( bson_size(&newObj), 4 ) ;
   }
   else
   {
      ossMemcpy( &( pBuffer[ offset ] ), bson_data( object ), objSize ) ;
      appendSize = ossRoundUpToMultipleX( objSize, 4 ) ;
   }

   if ( nextOffset )
   {
      *nextOffset = offset + appendSize ;
   }

done:
   if ( bsonInit )
   {
      bson_destroy( &newObj ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 clientAppendInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              bson *insertor, BOOLEAN endianConvert )
{
   INT32 rc             = SDB_OK ;
   MsgOpInsert *pInsert = (MsgOpInsert*)(*ppBuffer) ;
   INT32 offset         = 0 ;
   INT32 packetLength   = 0 ;
   INT32 nextOffset     = 0 ;
   ossEndianConvertIf ( pInsert->header.messageLength, offset,
                        endianConvert ) ;
   packetLength   = ossRoundUpToMultipleX( offset, 4 ) +
                    ossRoundUpToMultipleX( bson_size(insertor), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   /* now the buffer is large enough */
   pInsert = (MsgOpInsert*)(*ppBuffer) ;
   ossEndianConvertIf ( packetLength, pInsert->header.messageLength,
                        endianConvert ) ;

   rc = _clientAppendObj2Buff( *ppBuffer, *bufferSize, offset,
                               insertor, endianConvert, &nextOffset ) ;
   if ( rc )
   {
      ossPrintf( "Failed to append insertor to buffer, rc = %d"OSS_NEWLINE,
                 rc ) ;
      goto error ;
   }

   if ( nextOffset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 clientAppendHint2InsertMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                  bson *hint, BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK;
   INT32 packetLength = 0 ;
   INT32 offset = 0 ;
   INT32 nextOffset = 0 ;
   INT32 hintSize = bson_size( hint ) ;
   MsgOpInsert *pInsert = (MsgOpInsert *)(*ppBuffer) ;

   ossEndianConvertIf( pInsert->header.messageLength, offset, endianConvert ) ;
   offset = ossRoundUpToMultipleX( offset, 4 ) ;
   packetLength = offset + MSG_HINT_MARK_LEN +
                  ossRoundUpToMultipleX( hintSize, 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientCheckBuffer( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   pInsert = (MsgOpInsert *)(*ppBuffer) ;
   ossEndianConvertIf( packetLength, pInsert->header.messageLength,
                       endianConvert ) ;

   // Append prefix marking for insert: 4 bytes of value 0.
   ossMemset( &(*ppBuffer)[offset], 0, MSG_HINT_MARK_LEN ) ;
   offset += MSG_HINT_MARK_LEN ;

   rc = _clientAppendObj2Buff( *ppBuffer, packetLength, offset, hint,
                               endianConvert, &nextOffset ) ;
   if ( rc )
   {
      ossPrintf( "Failed to append hint to buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   if ( nextOffset != packetLength )
   {
      ossPrintf( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName, SINT32 flag,
                             UINT64 reqID,
                             bson *insertor,
                             bson *hint,
                             BOOLEAN endianConvert )
{
   INT32 rc             = SDB_OK ;
   MsgOpInsert *pInsert = NULL ;
   INT32 offset         = 0 ;
   INT32 nextOffset     = 0 ;
   INT32 nameLength     = 0 ;
   INT32 packetLength = ossRoundUpToMultipleX(offsetof(MsgOpInsert, name) +
                                              ossStrlen ( CollectionName ) + 1,
                                              4 ) +
                        ossRoundUpToMultipleX( bson_size(insertor), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // now the buffer is large enough
   pInsert                       = (MsgOpInsert*)(*ppBuffer) ;
   ossEndianConvertIf ( clientDefaultVersion, pInsert->version,
                                              endianConvert ) ;
   ossEndianConvertIf ( clientDefaultW, pInsert->w, endianConvert ) ;
   ossEndianConvertIf ( flag, pInsert->flags, endianConvert ) ;
   // nameLength does NOT include '\0'
   nameLength = ossStrlen ( CollectionName ) ;
   ossEndianConvertIf( nameLength, pInsert->nameLength, endianConvert ) ;
   pInsert->header.requestID     = reqID ;
   pInsert->header.opCode        = MSG_BS_INSERT_REQ ;
   pInsert->header.messageLength = packetLength ;
   pInsert->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pInsert->header.version       = SDB_PROTOCOL_VER_2 ;
   pInsert->header.flags         = FLAG_RESULT_DETAIL ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pInsert->header.globalID), 0, sizeof(pInsert->header.globalID) ) ;
   ossMemset( pInsert->header.reserve, 0, sizeof(pInsert->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pInsert->name, CollectionName, nameLength ) ;
   pInsert->name[nameLength] = 0 ;
   // get the offset of the first bson obj
   offset = ossRoundUpToMultipleX( offsetof(MsgOpInsert, name) +
                                   nameLength + 1,
                                   4 ) ;
   if ( endianConvert )
   {
      clientEndianConvertHeader ( &pInsert->header ) ;
   }

   rc = _clientAppendObj2Buff( *ppBuffer, *bufferSize, offset, insertor,
                               endianConvert, &nextOffset ) ;
   if ( rc )
   {
      goto error ;
   }

   offset = nextOffset ;

   if ( hint )
   {
      rc = clientAppendHint2InsertMsg( ppBuffer, bufferSize,hint,
                                       endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      // sanity test. If hint is specified, the check will be done when
      // appending the hint.
      if ( offset != packetLength )
      {
         ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildQueryMsg  ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName, SINT32 flag,
                             UINT64 reqID,
                             SINT64 numToSkip, SINT64 numToReturn,
                             const bson *query, const bson *fieldSelector,
                             const bson *orderBy, const bson *hint,
                             BOOLEAN endianConvert )
{
   bson emptyObj ;
   INT32 packetLength   = 0 ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   INT32 offset         = 0 ;
   INT32 nameLength     = 0 ;

   bson_init ( &emptyObj ) ;
   bson_empty ( &emptyObj ) ;

   if ( !query )
   {
      query = &emptyObj ;
   }
   if ( !fieldSelector )
   {
      fieldSelector = &emptyObj ;
   }
   if ( !orderBy )
   {
      orderBy = &emptyObj ;
   }
   if ( !hint )
   {
      hint = &emptyObj ;
   }

   packetLength = ossRoundUpToMultipleX(offsetof(MsgOpQuery, name) +
                                        ossStrlen ( CollectionName ) + 1,
                                        4 ) +
                  ossRoundUpToMultipleX( bson_size(query), 4 ) +
                  ossRoundUpToMultipleX( bson_size(fieldSelector), 4) +
                  ossRoundUpToMultipleX( bson_size(orderBy), 4 ) +
                  ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   nameLength = ossStrlen ( CollectionName ) ;
   // now the buffer is large enough
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   ossEndianConvertIf ( clientDefaultVersion, pQuery->version,
                        endianConvert ) ;
   ossEndianConvertIf ( clientDefaultW, pQuery->w, endianConvert ) ;
   ossEndianConvertIf ( flag, pQuery->flags, endianConvert ) ;
   // nameLength does NOT include '\0'
   ossEndianConvertIf ( nameLength, pQuery->nameLength, endianConvert ) ;
   ossEndianConvertIf ( numToSkip, pQuery->numToSkip, endianConvert ) ;
   ossEndianConvertIf ( numToReturn, pQuery->numToReturn, endianConvert ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.eye            = MSG_COMM_EYE_DEFAULT ;
   pQuery->header.version        = SDB_PROTOCOL_VER_2 ;
   pQuery->header.flags          = 0 ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossMemset( &(pQuery->header.globalID), 0, sizeof(pQuery->header.globalID) ) ;
   ossMemset( pQuery->header.reserve, 0, sizeof(pQuery->header.reserve) ) ;

   // copy collection name
   ossStrncpy ( pQuery->name, CollectionName, nameLength ) ;
   pQuery->name[nameLength]=0 ;
   // get the offset of the first bson obj
   offset = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                                   nameLength + 1,
                                   4 ) ;

   if ( !endianConvert )
   {
      // write query condition
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(query), bson_size(query)) ;
      offset += ossRoundUpToMultipleX( bson_size(query), 4 ) ;
      // write field select
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(fieldSelector),
                  bson_size(fieldSelector) ) ;
      offset += ossRoundUpToMultipleX( bson_size(fieldSelector), 4 ) ;
      // write order by clause
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(orderBy),
                  bson_size(orderBy) ) ;
      offset += ossRoundUpToMultipleX( bson_size(orderBy), 4 ) ;
      // write optimizer hint
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(hint),
                  bson_size(hint) ) ;
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   }
   else
   {
      bson newquery ;
      bson newselector ;
      bson neworderby ;
      bson newhint ;
      off_t off = 0 ;

      clientEndianConvertHeader ( &pQuery->header ) ;
      bson_init ( &newquery ) ;
      bson_init ( &newselector ) ;
      bson_init ( &neworderby ) ;
      bson_init ( &newhint ) ;

      rc = bson_copy ( &newquery, query ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      rc = bson_copy ( &newselector, fieldSelector ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      rc = bson_copy ( &neworderby, orderBy ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      rc = bson_copy ( &newhint, hint ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newquery), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newselector), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&neworderby), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newhint), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newquery),
                  bson_size(query));
      offset += ossRoundUpToMultipleX( bson_size(query), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newselector),
                  bson_size(fieldSelector));
      offset += ossRoundUpToMultipleX( bson_size(fieldSelector), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&neworderby),
                  bson_size(orderBy));
      offset += ossRoundUpToMultipleX( bson_size(orderBy), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newhint),
                  bson_size(hint));
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;

endian_convert_done :
      bson_destroy ( &newquery ) ;
      bson_destroy ( &newselector ) ;
      bson_destroy ( &neworderby ) ;
      bson_destroy ( &newhint ) ;

      if ( rc )
      {
         goto error ;
      }
   }
   // sanity test
   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildAdvanceMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             INT64 contextID, UINT64 reqID,
                             const bson *option,
                             const CHAR *pBackData,
                             INT32 backDataSize,
                             BOOLEAN endianConvert )
{
   bson emptyObj ;
   INT32 packetLength   = 0 ;
   INT32 rc             = SDB_OK ;
   MsgOpAdvance *pAdvance = NULL ;
   INT32 offset         = 0 ;

   bson_init ( &emptyObj ) ;
   bson_empty ( &emptyObj ) ;

   if ( !option )
   {
      option = &emptyObj ;
   }

   packetLength = ossRoundUpToMultipleX( sizeof( MsgOpAdvance ), 4 ) +
                  ossRoundUpToMultipleX( bson_size(option), 4 ) +
                  ossRoundUpToMultipleX( backDataSize, 4 ) ;

   if ( backDataSize < 0 )
   {
      ossPrintf ( "Back data size invalid"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // now the buffer is large enough
   pAdvance                      = (MsgOpAdvance*)(*ppBuffer) ;
   ossEndianConvertIf ( contextID, pAdvance->contextID, endianConvert ) ;
   ossEndianConvertIf ( backDataSize, pAdvance->backDataSize, endianConvert ) ;
   ossMemset( pAdvance->padding, 0, sizeof(pAdvance->padding) ) ;

   pAdvance->header.requestID    = reqID ;
   pAdvance->header.opCode       = MSG_BS_ADVANCE_REQ ;
   pAdvance->header.messageLength= packetLength ;
   pAdvance->header.routeID.value= 0 ;
   pAdvance->header.TID          = ossGetCurrentThreadID() ;
   ossMemset( &(pAdvance->header.globalID), 0, sizeof(pAdvance->header.globalID) ) ;

   // get the offset of the bson obj
   offset = ossRoundUpToMultipleX( sizeof( MsgOpAdvance ), 4 ) ;

   if( !endianConvert )
   {
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(option),
                                          bson_size(option) );
      offset += ossRoundUpToMultipleX( bson_size(option), 4 ) ;

      /// copy the back data
      if ( backDataSize > 0 )
      {
         ossMemcpy( &((*ppBuffer)[offset]), pBackData, backDataSize ) ;
         offset += ossRoundUpToMultipleX( backDataSize, 4 ) ;
      }
   }
   else
   {
      bson newoption ;
      bson backdata ;
      INT32 backdataSz = 0 ;
      INT32 backDataOffset = 0 ;

      off_t off = 0 ;
      clientEndianConvertHeader ( &pAdvance->header ) ;
      bson_init ( &newoption ) ;
      bson_init ( &backdata ) ;

      rc = bson_copy ( &newoption, option ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newoption), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newoption),
                  bson_size(&newoption));
      offset += ossRoundUpToMultipleX( bson_size(option), 4 ) ;

      /// copy the back data
      if ( backDataSize > 0 )
      {
         while ( backDataOffset < backDataSize )
         {
            bson_init_finished_data ( &backdata, &pBackData[backDataOffset] ) ;
            off = 0 ;
            backdataSz = bson_size( &backdata ) ;

            if ( !bson_endian_convert( (char*)bson_data(&backdata), &off, TRUE ) )
            {
               rc = SDB_INVALIDARG ;
               goto endian_convert_done ;
            }

            backDataOffset += ossRoundUpToMultipleX( backdataSz, 4 ) ;
         }

         if ( backDataOffset != ossRoundUpToMultipleX( backDataSize, 4 ) )
         {
            ossPrintf ( "Back data length is invalid"OSS_NEWLINE ) ;
            rc = SDB_INVALIDARG ;
            goto endian_convert_done ;
         }

         ossMemcpy( &((*ppBuffer)[offset]), pBackData, backDataSize ) ;
         offset += ossRoundUpToMultipleX( backDataSize, 4 ) ;
      }


endian_convert_done :
      bson_destroy ( &newoption ) ;
      bson_destroy ( &backdata ) ;

      if ( rc )
      {
         goto error ;
      }
   }
   // sanity test
   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildDeleteMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName,
                             SINT32 flag, UINT64 reqID,
                             bson *deletor,
                             bson *hint,
                             BOOLEAN endianConvert )
{
   bson emptyObj ;
   INT32 packetLength   = 0 ;
   INT32 rc             = SDB_OK ;
   MsgOpDelete *pDelete = NULL ;
   INT32 offset         = 0 ;
   INT32 nameLength     = 0 ;

   bson_init ( &emptyObj ) ;
   bson_empty ( &emptyObj ) ;

   if ( !deletor )
   {
      deletor = &emptyObj ;
   }
   if ( !hint )
   {
      hint = &emptyObj ;
   }
   packetLength = ossRoundUpToMultipleX(offsetof(MsgOpDelete, name) +
                                        ossStrlen ( CollectionName ) + 1,
                                        4 ) +
                  ossRoundUpToMultipleX( bson_size(deletor), 4 ) +
                  ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   nameLength = ossStrlen ( CollectionName ) ;
   // now the buffer is large enough
   pDelete                       = (MsgOpDelete*)(*ppBuffer) ;
   ossEndianConvertIf ( clientDefaultVersion, pDelete->version, endianConvert ) ;
   ossEndianConvertIf ( clientDefaultW, pDelete->w, endianConvert ) ;
   ossEndianConvertIf ( flag, pDelete->flags, endianConvert ) ;
   // nameLength does NOT include '\0'
   ossEndianConvertIf ( nameLength, pDelete->nameLength, endianConvert ) ;
   pDelete->header.requestID     = reqID ;
   pDelete->header.opCode        = MSG_BS_DELETE_REQ ;
   pDelete->header.messageLength = packetLength ;
   pDelete->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pDelete->header.version       = SDB_PROTOCOL_VER_2 ;
   pDelete->header.flags         = 0 ;
   pDelete->header.routeID.value = 0 ;
   pDelete->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pDelete->header.globalID), 0, sizeof(pDelete->header.globalID) ) ;
   ossMemset( pDelete->header.reserve, 0, sizeof(pDelete->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pDelete->name, CollectionName, nameLength ) ;
   pDelete->name[nameLength]=0 ;
   // get the offset of the first bson obj
   offset = ossRoundUpToMultipleX( offsetof(MsgOpDelete, name) +
                                   nameLength + 1,
                                   4 ) ;
   if( !endianConvert )
   {
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(deletor),
                                          bson_size(deletor) );
      offset += ossRoundUpToMultipleX( bson_size(deletor), 4 ) ;

      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(hint), bson_size(hint) );
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   }
   else
   {
      bson newdeletor ;
      bson newhint ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &pDelete->header ) ;
      bson_init ( &newdeletor ) ;
      bson_init ( &newhint ) ;

      rc = bson_copy ( &newdeletor, deletor ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      rc = bson_copy ( &newhint, hint ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newdeletor), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newhint), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newdeletor),
                  bson_size(deletor));
      offset += ossRoundUpToMultipleX( bson_size(deletor), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newhint),
                  bson_size(hint));
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;

endian_convert_done :
      bson_destroy ( &newdeletor ) ;
      bson_destroy ( &newhint ) ;

      if ( rc )
      {
         goto error ;
      }
   }
   // sanity test
   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientAppendOID ( bson *obj, bson_iterator *ret )
{
   INT32 rc = SDB_OK ;
   bson_iterator it ;
   rc = bson_find ( &it, obj, CLIENT_RECORD_ID_FIELD ) ;
   if ( BSON_EOO == rc )
   {
      /* if there's no "_id" exists in the object */
      bson_oid_t oid ;
      CHAR *data = NULL ;
      INT32 len  = 0 ;
      bson_oid_gen ( &oid ) ;
      /* 2 means type and '\0' for key */
      len = bson_size ( obj ) + CLIENT_RECORD_ID_FIELD_STRLEN +
            sizeof ( oid ) + 2 ;
      data = ( CHAR * ) malloc ( len ) ;
      if ( data )
      {
         CHAR *cur = data ;
         /* append size for whole object */
         *(INT32*)data = len ;
         data += sizeof(INT32) ;
         /* append type */
         *data = BSON_OID ;
         /* current position is where bson_oid starts, let's return it */
         if ( ret )
         {
            ret->cur = data ;
            ret->first = 0 ;
         }
         ++data ;
         /* append "_id" */
         data[0] = '_' ;
         data[1] = 'i' ;
         data[2] = 'd' ;
         data[3] = '\0' ;
         data += CLIENT_RECORD_ID_FIELD_STRLEN + 1 ;
         ossMemcpy ( data, oid.bytes, sizeof(oid ) ) ;
         data += sizeof ( oid ) ;
         ossMemcpy ( data, obj->data + sizeof ( INT32 ),
                     bson_size ( obj ) - sizeof ( INT32 ) ) ;
         if ( obj->ownmem )
         {
            free ( obj->data ) ;
         }
         obj->data = cur ;
         obj->ownmem = 1 ;
         obj->dataSize = len ;
         obj->cur = cur + len ;
      }
      else
      {
         // if we failed to allocate memory, let's return OOM
         rc = SDB_OOM ;
         goto error ;
      }
   }
   else if ( ret )
   {
      ossMemcpy ( ret, &it, sizeof(bson_iterator) ) ;
   }
   rc = SDB_OK ;
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildAggrRequest1( CHAR **ppBuffer, INT32 *bufferSize,
                               const CHAR *CollectionName, bson **objs,
                               SINT32 num, BOOLEAN endianConvert )
{
   INT32 rc                = SDB_OK ;
   MsgOpAggregate *pAggr   = NULL ;
   INT32 nameLength        = 0;
   INT32 packetLength      = 0;
   INT32 offset            = 0;
   SINT32 i                = 0;

   if ( NULL == objs || num <= 0 )
   {
      rc = SDB_INVALIDARG;
      ossPrintf( "param can't be empty!"OSS_NEWLINE );
      goto error;
   }
   nameLength = ossStrlen( CollectionName );
   packetLength = ossRoundUpToMultipleX( offsetof(MsgOpAggregate, name ) +
                                         nameLength + 1,
                                         4 ) ;
   for ( i = 0 ; i < num ; i++ )
   {
      packetLength += ossRoundUpToMultipleX( bson_size(objs[i]), 4 );
      if ( packetLength <= 0 )
      {
         rc = SDB_INVALIDARG;
         ossPrintf( "packet size overflow!"OSS_NEWLINE );
         goto error;
      }
   }
   rc = clientCheckBuffer( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf( "Failed to check buffer, rc=%d"OSS_NEWLINE, rc );
      goto error;
   }
   pAggr = (MsgOpAggregate *)(*ppBuffer);
   ossEndianConvertIf( clientDefaultVersion, pAggr->version, endianConvert );
   ossEndianConvertIf( clientDefaultW, pAggr->w, endianConvert );
   ossEndianConvertIf(clientDefaultFlags, pAggr->flags, endianConvert );
   ossEndianConvertIf( nameLength, pAggr->nameLength, endianConvert );
   pAggr->header.messageLength = packetLength;
   pAggr->header.eye = MSG_COMM_EYE_DEFAULT ;
   pAggr->header.version = SDB_PROTOCOL_VER_2 ;
   pAggr->header.flags = FLAG_RESULT_DETAIL ;
   pAggr->header.opCode = MSG_BS_AGGREGATE_REQ ;
   pAggr->header.routeID.value = 0;
   pAggr->header.requestID = 0;
   pAggr->header.TID = ossGetCurrentThreadID();
   ossMemset( &(pAggr->header.globalID), 0, sizeof(pAggr->header.globalID) ) ;
   ossMemset( pAggr->header.reserve, 0, sizeof(pAggr->header.reserve) ) ;
   ossStrncpy( pAggr->name, CollectionName, nameLength );
   pAggr->name[nameLength] = 0;

   offset = ossRoundUpToMultipleX( offsetof(MsgOpAggregate, name) +
                                   nameLength + 1,
                                   4 );
   if ( !endianConvert )
   {
      for ( i = 0 ; i < num ; i++ )
      {
         ossMemcpy( &((*ppBuffer)[offset]), bson_data(objs[i]),
                    bson_size(objs[i]));
         offset += ossRoundUpToMultipleX( bson_size(objs[i]), 4 );
      }
   }
   else
   {
      clientEndianConvertHeader( &pAggr->header );
      for ( i = 0 ; i < num ; i++ )
      {
         off_t off = 0;
         bson newObj ;

         bson_init( &newObj );
         rc = bson_copy( &newObj, objs[i] );
         if ( rc )
         {
            rc = SDB_INVALIDARG ;
            goto endian_convert_done ;
         }
         if ( !bson_endian_convert((CHAR *)bson_data(&newObj), &off, TRUE ) )
         {
            rc = SDB_INVALIDARG ;
            goto endian_convert_done;
         }

         ossMemcpy( &((*ppBuffer)[offset]), bson_data(&newObj),
                    bson_size(objs[i])) ;
         offset += ossRoundUpToMultipleX( bson_size(objs[i]), 4 );

endian_convert_done:
         bson_destroy( &newObj ) ;

         if ( rc )
         {
            goto error;
         }
      }
   }
   if ( offset != packetLength )
   {
      ossPrintf( "Invalid packet length"OSS_NEWLINE );
      rc = SDB_SYS;
      goto error;
   }
done:
   return rc;
error:
   goto done;
}

INT32 clientBuildAggrRequest( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *CollectionName, bson *obj,
                              BOOLEAN endianConvert )
{
   INT32 rc                = SDB_OK ;
   MsgOpAggregate *pAggr   = NULL ;
   INT32 offset            = 0 ;
   INT32 nameLength        = ossStrlen ( CollectionName ) ;
   INT32 packetLength = ossRoundUpToMultipleX(offsetof(MsgOpInsert, name) +
                                              nameLength + 1,
                                              4 ) +
                        ossRoundUpToMultipleX( bson_size(obj), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // now the buffer is large enough
   pAggr = (MsgOpAggregate*)(*ppBuffer) ;
   ossEndianConvertIf ( clientDefaultVersion, pAggr->version,
                                              endianConvert ) ;
   ossEndianConvertIf ( clientDefaultW, pAggr->w, endianConvert ) ;
   ossEndianConvertIf ( clientDefaultFlags, pAggr->flags, endianConvert ) ;
   // nameLength does NOT include '\0'
   //nameLength = ossStrlen ( CollectionName ) ;
   ossEndianConvertIf( nameLength, pAggr->nameLength, endianConvert ) ;
   pAggr->header.requestID     = 0 ;
   pAggr->header.opCode        = MSG_BS_AGGREGATE_REQ ;
   pAggr->header.messageLength = packetLength ;
   pAggr->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pAggr->header.version       = SDB_PROTOCOL_VER_2 ;
   pAggr->header.flags         = 0 ;
   pAggr->header.routeID.value = 0 ;
   pAggr->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pAggr->header.globalID), 0, sizeof(pAggr->header.globalID) ) ;
   ossMemset( pAggr->header.reserve, 0, sizeof(pAggr->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pAggr->name, CollectionName, nameLength ) ;
   pAggr->name[nameLength]=0 ;
   // get the offset of the first bson obj
   offset = ossRoundUpToMultipleX( offsetof(MsgOpInsert, name) +
                                   nameLength + 1,
                                   4 ) ;
   if( !endianConvert )
   {
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(obj),
                                          bson_size(obj));
      offset += ossRoundUpToMultipleX( bson_size(obj), 4 ) ;
   }
   else
   {
      bson newinsertor ;
      off_t off = 0 ;

      bson_init ( &newinsertor ) ;
      rc = bson_copy ( &newinsertor, obj ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      clientEndianConvertHeader ( &pAggr->header ) ;
      if ( !bson_endian_convert( (CHAR*)bson_data(&newinsertor), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newinsertor),
                                          bson_size(obj));
      offset += ossRoundUpToMultipleX( bson_size(obj), 4 ) ;

endian_convert_done :
      bson_destroy ( &newinsertor ) ;

      if ( rc )
      {
         goto error ;
      }
   }
   // sanity test
   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientAppendAggrRequest ( CHAR **ppBuffer, INT32 *bufferSize,
                                bson *obj, BOOLEAN endianConvert )
{
   INT32 rc                = SDB_OK ;
   MsgOpAggregate *pAggr   = (MsgOpAggregate*)(*ppBuffer) ;
   INT32 offset            = 0 ;
   INT32 packetLength      = 0 ;

   ossEndianConvertIf ( pAggr->header.messageLength, offset,
                        endianConvert ) ;
   packetLength   = offset +
                    ossRoundUpToMultipleX( bson_size(obj), 4 ) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   /* now the buffer is large enough */
   pAggr = (MsgOpAggregate*)(*ppBuffer) ;
   ossEndianConvertIf ( packetLength, pAggr->header.messageLength,
                        endianConvert ) ;
   if( !endianConvert )
   {
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(obj),
                                          bson_size(obj));
      offset += ossRoundUpToMultipleX ( bson_size(obj), 4 ) ;
   }
   else
   {
      bson newinsertor ;
      off_t off = 0 ;

      bson_init ( &newinsertor ) ;
      rc = bson_copy ( &newinsertor, obj ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      if ( !bson_endian_convert ( (char*)bson_data(&newinsertor), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newinsertor),
                  bson_size(obj));
      offset += ossRoundUpToMultipleX( bson_size(obj), 4 ) ;

endian_convert_done :
      bson_destroy ( &newinsertor ) ;

      if ( rc )
      {
         goto error ;
      }
   }

   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                         INT32 msgType, const bson *meta,
                         SINT32 flags, SINT16 w, SINT64 contextID,
                         UINT64 reqID, const SINT64 *lobOffset,
                         const UINT32 *len, const CHAR *data,
                         BOOLEAN endianConvert )
{
   INT32 rc             = SDB_OK ;
   SINT32 packetLength  = 0 ;
   MsgOpLob *msg        = NULL ;
   SINT32 offset        = 0 ;
   SINT16 padding       = 0 ;
   UINT32 bsonLen       = 0 ;
   MsgLobTuple *tuple   = NULL ;

   packetLength = sizeof( MsgOpLob ) ;

   if ( NULL != meta )
   {
      bsonLen = bson_size( meta ) ;
      packetLength += ossRoundUpToMultipleX( bsonLen, 4 ) ;
   }

   if ( NULL != lobOffset && NULL != len )
   {
      packetLength += sizeof( MsgLobTuple ) ;
   }

   if ( NULL != data )
   {
      packetLength += ossRoundUpToMultipleX( *len, 4 ) ;
   }

   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   msg = ( MsgOpLob * )( *ppBuffer ) ;
   ossEndianConvertIf ( flags, msg->flags, endianConvert ) ;
   ossEndianConvertIf ( clientDefaultVersion, msg->version, endianConvert ) ;
   ossEndianConvertIf ( w, msg->w, endianConvert ) ;
   ossEndianConvertIf ( padding, msg->padding, endianConvert ) ;
   ossEndianConvertIf ( contextID, msg->contextID, endianConvert ) ;
   ossEndianConvertIf ( bsonLen, msg->bsonLen, endianConvert ) ;

   msg->header.requestID = reqID ;
   msg->header.opCode = msgType ;
   msg->header.messageLength = packetLength ;
   msg->header.eye = MSG_COMM_EYE_DEFAULT ;
   msg->header.version = SDB_PROTOCOL_VER_2 ;
   msg->header.flags = 0 ;
   msg->header.routeID.value = clientDefaultRouteID ;
   msg->header.TID = ossGetCurrentThreadID() ;
   ossMemset( &(msg->header.globalID), 0, sizeof(msg->header.globalID) ) ;
   ossMemset( msg->header.reserve, 0, sizeof(msg->header.reserve) ) ;

   offset = sizeof( MsgOpLob ) ;

   if ( !endianConvert )
   {
      if ( NULL != meta )
      {
         ossMemcpy( *ppBuffer + offset, bson_data( meta ), bsonLen ) ;
         offset += ossRoundUpToMultipleX( bsonLen, 4 ) ;
      }

      if ( NULL != lobOffset && NULL != len )
      {
         tuple = ( MsgLobTuple * )(*ppBuffer + offset) ;
         tuple->columns.len = *len ;
         tuple->columns.offset = *lobOffset ;
         offset += sizeof( MsgLobTuple ) ;
      }

      if ( NULL != data )
      {
         ossMemcpy( *ppBuffer + offset, data, *len ) ;
         offset += ossRoundUpToMultipleX( *len, 4 ) ;
      }
   }
   else
   {
      clientEndianConvertHeader ( &( msg->header ) ) ;
      if ( NULL != meta )
      {
         off_t off = 0 ;
         bson newObj ;

         bson_init( &newObj ) ;
         rc = bson_copy( &newObj, meta ) ;
         if ( rc )
         {
            rc = SDB_INVALIDARG ;
            bson_destroy( &newObj ) ;
            goto error ;
         }

         if ( !bson_endian_convert ( (char*)bson_data(&newObj), &off, TRUE ) )
         {
            ossMemcpy( *ppBuffer + offset, bson_data( &newObj ),
                       bson_size( &newObj ) ) ;
            offset += ossRoundUpToMultipleX( bson_size( &newObj ), 4 ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }

         bson_destroy( &newObj ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( NULL != lobOffset && NULL != len )
      {
         tuple = ( MsgLobTuple * )( *ppBuffer + offset ) ;
         ossEndianConvertIf( *len, tuple->columns.len, endianConvert ) ;
         ossEndianConvertIf( *lobOffset, tuple->columns.offset, endianConvert ) ;
         offset += sizeof( MsgLobTuple ) ;
      }

      if ( NULL != data )
      {
         ossMemcpy( *ppBuffer + offset, data, *len ) ;
         offset += ossRoundUpToMultipleX( *len, 4 ) ;
      }
   }

   if ( offset != packetLength )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildOpenLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const bson *meta, SINT32 flags, SINT16 w,
                             UINT64 reqID,
                             BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_OPEN_REQ, meta,
                           flags, w, -1, reqID, NULL,
                           NULL, NULL, endianConvert ) ;

   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}


INT32 clientBuildCreateLobIDMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const bson *meta, SINT32 flags, SINT16 w,
                             UINT64 reqID,
                             BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_CREATELOBID_REQ, meta,
                           flags, w, -1, reqID, NULL,
                           NULL, NULL, endianConvert ) ;

   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}


INT32 clientBuildRemoveLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                               const bson *meta,
                               SINT32 flags, SINT16 w,
                               UINT64 reqID,
                               BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_REMOVE_REQ, meta,
                           flags, w, -1, reqID,
                           NULL, NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildTruncateLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                 const bson *meta,
                                 SINT32 flags, SINT16 w,
                                 UINT64 reqID,
                                 BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_TRUNCATE_REQ, meta,
                           flags, w, -1, reqID,
                           NULL, NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildAuthCrtMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *pUsrName,
                             const CHAR *clearTextPasswd,
                             const CHAR *pPasswd,
                             const bson *options,
                             UINT64 reqID, BOOLEAN endianConvert,
                             INT32 authVersion )
{
   INT32 rc = SDB_OK ;
   INT32 msgLen = 0 ;
   bson obj ;
   INT32 bsonSize = 0 ;
   MsgAuthCrtUsr *msg ;

   if ( NULL == pUsrName || NULL == pPasswd || NULL == clearTextPasswd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bson_init( &obj ) ;
   rc = bson_append_string( &obj, SDB_AUTH_USER, pUsrName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_PASSWD, pPasswd ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   if ( authVersion >= AUTH_SCRAM_SHA256 )
   {
      rc = bson_append_string( &obj, SDB_AUTH_TEXTPASSWD, clearTextPasswd ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   if ( options )
   {
      rc = bson_append_bson( &obj, FIELD_NAME_OPTIONS, options ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   bsonSize = bson_size( &obj ) ;
   msgLen = sizeof( MsgAuthCrtUsr ) +
            ossRoundUpToMultipleX( bsonSize, sizeof(ossValuePtr ) ) ;
   if ( msgLen < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, msgLen ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   msg                       = ( MsgAuthCrtUsr * )(*ppBuffer) ;
   msg->header.requestID     = reqID ;
   msg->header.opCode        = MSG_AUTH_CRTUSR_REQ ;
   msg->header.messageLength = sizeof( MsgAuthCrtUsr ) + bsonSize ;
   msg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   msg->header.version       = SDB_PROTOCOL_VER_2 ;
   msg->header.flags         = 0 ;
   msg->header.routeID.value = 0 ;
   msg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(msg->header.globalID), 0, sizeof(msg->header.globalID) ) ;
   ossMemset( msg->header.reserve, 0, sizeof(msg->header.reserve) ) ;

   if ( !endianConvert )
   {
      ossMemcpy( *ppBuffer + sizeof(MsgAuthCrtUsr),
                 bson_data( &obj ), bsonSize ) ;
   }
   else
   {
      bson newobj ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &msg->header ) ;
      bson_init ( &newobj ) ;
      rc = bson_copy ( &newobj, &obj ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      if ( !bson_endian_convert ( (char*)bson_data(&newobj), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      ossMemcpy( *ppBuffer + sizeof(MsgAuthCrtUsr),
                 bson_data( &newobj ), bsonSize ) ;
endian_convert_done :
      bson_destroy ( &newobj ) ;

      if ( rc )
      {
         goto error ;
      }
   }
done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

INT32 clientBuildSeqFetchMsg( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *seqName, INT32 fetchNum,
                              UINT64 reqID, BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   INT32 opCode = MSG_BS_SEQUENCE_FETCH_REQ ;
   bson obj ;
   MsgOpQuery *pQuery = NULL ;
   bson_init( &obj ) ;

   rc = bson_append_string( &obj, FIELD_NAME_NAME, seqName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_int( &obj, FIELD_NAME_FETCH_NUM, fetchNum ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = clientBuildQueryMsg( ppBuffer, bufferSize,
                             "", 0, reqID, 0, -1,
                             &obj, NULL, NULL, NULL,
                             endianConvert ) ;
   pQuery = (MsgOpQuery*)(*ppBuffer) ;
   ossEndianConvertIf( opCode, pQuery->header.opCode, endianConvert ) ;
done:
   bson_destroy ( &obj ) ;
   return rc ;
error:
   goto done ;
}

/*****************************************************************
    +++++  CPP Message functions begin  ++++
******************************************************************/
INT32 clientBuildUpdateMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                const CHAR*selector,
                                const CHAR*updator,
                                const CHAR*hint,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bs ;
   bson bu ;
   bson bh ;
   bson_init ( &bs ) ;
   bson_init ( &bu ) ;
   bson_init ( &bh ) ;
   if ( selector )
   {
      bson_init_finished_data ( &bs, selector ) ;
   }
   if ( updator )
   {
      bson_init_finished_data ( &bu, updator ) ;
   }
   if ( hint )
   {
      bson_init_finished_data ( &bh, hint ) ;
   }
   rc = clientBuildUpdateMsg ( ppBuffer, bufferSize,
                               CollectionName, flag, reqID,
                               selector?&bs:NULL,
                               updator?&bu:NULL,
                               hint?&bh:NULL,
                               endianConvert ) ;
   bson_destroy ( &bs ) ;
   bson_destroy ( &bu ) ;
   bson_destroy ( &bh ) ;
   return rc ;
}

INT32 clientAppendInsertMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                 const CHAR *insertor,
                                 BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bi ;
   bson_init ( &bi ) ;
   bson_init_finished_data ( &bi, insertor ) ;
   rc = clientAppendInsertMsg ( ppBuffer, bufferSize,
                                &bi, endianConvert ) ;
   bson_destroy ( &bi ) ;
   return rc ;
}

INT32 clientAppendHint2InsertMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                     const CHAR *hint,
                                     BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bi ;
   bson_init ( &bi ) ;
   bson_init_finished_data ( &bi, hint ) ;
   rc = clientAppendHint2InsertMsg( ppBuffer, bufferSize, &bi, endianConvert ) ;
   bson_destroy( &bi ) ;
   return rc ;
}

INT32 clientBuildInsertMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                const CHAR *insertor,
                                const CHAR *hint,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bi ;
   bson bh ;
   bson_init ( &bi ) ;
   bson_init ( &bh ) ;
   bson_init_finished_data ( &bi, insertor ) ;

   if ( hint )
   {
      bson_init_finished_data ( &bh, hint ) ;
   }
   rc = clientBuildInsertMsg ( ppBuffer, bufferSize, CollectionName, flag,
                               reqID, &bi, hint?&bh:NULL, endianConvert ) ;
   bson_destroy ( &bi ) ;
   bson_destroy ( &bh ) ;
   return rc ;
}

INT32 clientBuildTransactionCommitMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                          UINT64 reqID,
                                          const CHAR *hint,
                                          BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bh ;
   bson_init ( &bh ) ;
   if ( hint )
   {
      bson_init_finished_data ( &bh, hint ) ;
   }
   rc = clientBuildTransactionCommitMsg ( ppBuffer, bufferSize,
                                          reqID, hint?&bh:NULL, endianConvert ) ;
   bson_destroy ( &bh ) ;
   return rc ;
}

INT32 clientBuildQueryMsgCpp  ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                SINT64 numToSkip,
                                SINT64 numToReturn,
                                const CHAR *query,
                                const CHAR *fieldSelector,
                                const CHAR *orderBy,
                                const CHAR *hint,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bq ;
   bson bf ;
   bson bo ;
   bson bh ;
   bson_init ( &bq ) ;
   bson_init ( &bf ) ;
   bson_init ( &bo ) ;
   bson_init ( &bh ) ;
   if ( query )
   {
      bson_init_finished_data ( &bq, query ) ;
   }
   if ( fieldSelector )
   {
      bson_init_finished_data ( &bf, fieldSelector ) ;
   }
   if ( orderBy )
   {
      bson_init_finished_data ( &bo, orderBy ) ;
   }
   if ( hint )
   {
      bson_init_finished_data ( &bh, hint ) ;
   }
   rc = clientBuildQueryMsg ( ppBuffer, bufferSize,
                              CollectionName, flag, reqID,
                              numToSkip, numToReturn,
                              query?&bq:NULL,
                              fieldSelector?&bf:NULL,
                              orderBy?&bo:NULL,
                              hint?&bh:NULL,
                              endianConvert ) ;
   bson_destroy ( &bq ) ;
   bson_destroy ( &bf ) ;
   bson_destroy ( &bo ) ;
   bson_destroy ( &bh ) ;
   return rc ;
}

INT32 clientBuildAdvanceMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                INT64 contextID, UINT64 reqID,
                                const CHAR *option,
                                const CHAR *pBackData,
                                INT32 backDataSize,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bo ;
   bson_init ( &bo ) ;
   if ( option )
   {
      bson_init_finished_data ( &bo, option ) ;
   }

   rc = clientBuildAdvanceMsg( ppBuffer, bufferSize, contextID, reqID,
                               &bo, pBackData, backDataSize,
                               endianConvert ) ;
   bson_destroy ( &bo ) ;

   return rc ;
}

INT32 clientBuildDeleteMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                const CHAR *deletor,
                                const CHAR *hint,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bd ;
   bson bh ;
   bson_init ( &bd ) ;
   bson_init ( &bh ) ;
   if ( deletor )
   {
      bson_init_finished_data ( &bd, deletor ) ;
   }
   if ( hint )
   {
      bson_init_finished_data ( &bh, hint ) ;
   }
   rc = clientBuildDeleteMsg ( ppBuffer, bufferSize,
                               CollectionName, flag, reqID,
                               deletor?&bd:NULL,
                               hint?&bh:NULL,
                               endianConvert ) ;
   bson_destroy ( &bd ) ;
   bson_destroy ( &bh ) ;
   return rc ;
}

INT32 clientBuildAggrRequestCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                 const CHAR *CollectionName,
                                 const CHAR *obj,
                                 BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bi ;
   bson_init ( &bi ) ;
   bson_init_finished_data ( &bi, obj ) ;
   rc = clientBuildAggrRequest( ppBuffer, bufferSize,
                                CollectionName, &bi, endianConvert ) ;
   bson_destroy ( &bi ) ;
   return rc ;
}

INT32 clientAppendAggrRequestCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                   const CHAR *obj,
                                   BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bi ;
   bson_init ( &bi ) ;
   bson_init_finished_data ( &bi, obj ) ;
   clientAppendAggrRequest ( ppBuffer, bufferSize,
                             &bi, endianConvert ) ;
   bson_destroy ( &bi ) ;
   return rc ;
}

INT32 clientBuildLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                            INT32 msgType, const CHAR *pMeta,
                            SINT32 flags, SINT16 w,
                            SINT64 contextID, UINT64 reqID,
                            const SINT64 *lobOffset,
                            const UINT32 *len,
                            const CHAR *data,
                            BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson bi ;
   bson_init ( &bi ) ;
   bson_init_finished_data ( &bi, pMeta ) ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           msgType, &bi,
                           flags, w, contextID,
                           reqID, lobOffset,
                           len, data,
                           endianConvert ) ;
   bson_destroy ( &bi ) ;
   return rc ;
}

INT32 clientBuildOpenLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *pMeta,
                                SINT32 flags, SINT16 w,
                                UINT64 reqID,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsgCpp( ppBuffer, bufferSize,
                              MSG_BS_LOB_OPEN_REQ, pMeta,
                              flags, w, -1, reqID, NULL,
                              NULL, NULL, endianConvert ) ;

   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildCreateLobIDMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                    const CHAR *pMeta, SINT32 flags, SINT16 w,
                                    UINT64 reqID, BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsgCpp( ppBuffer, bufferSize,
                              MSG_BS_LOB_CREATELOBID_REQ, pMeta,
                              flags, w, -1, reqID,
                              NULL, NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildRemoveLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                  const CHAR *pMeta,
                                  SINT32 flags, SINT16 w,
                                  UINT64 reqID,
                                  BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsgCpp( ppBuffer, bufferSize,
                              MSG_BS_LOB_REMOVE_REQ, pMeta,
                              flags, w, -1, reqID,
                              NULL, NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildTruncateLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                    const CHAR *pMeta,
                                    SINT32 flags, SINT16 w,
                                    UINT64 reqID,
                                    BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsgCpp( ppBuffer, bufferSize,
                              MSG_BS_LOB_TRUNCATE_REQ, pMeta,
                              flags, w, -1, reqID,
                              NULL, NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildAuthCrtMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *pUsrName,
                                const CHAR *clearTextPasswd,
                                const CHAR *pPasswd,
                                const CHAR *pOptions,
                                UINT64 reqID,
                                BOOLEAN endianConvert,
                                INT32 authVersion )
{
   INT32 rc = SDB_OK ;
   bson options ;

   bson_init ( &options ) ;

   if ( pOptions )
   {
      rc = bson_init_finished_data ( &options, pOptions ) ;
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   rc = clientBuildAuthCrtMsg( ppBuffer, bufferSize,
                               pUsrName, clearTextPasswd, pPasswd,
                               pOptions ? &options : NULL, reqID,
                               endianConvert, authVersion ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   bson_destroy ( &options ) ;
   return rc ;
error:
   goto done ;
}

INT32 clientBuildSeqFetchMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                 const CHAR *seqName, INT32 fetchNum,
                                 UINT64 reqID, BOOLEAN endianConvert )
{
   return clientBuildSeqFetchMsg( ppBuffer, bufferSize, seqName, fetchNum,
                                  reqID, endianConvert ) ;
}

/*****************************************************************
    ----  CPP Message functions end  ----
******************************************************************/

INT32 clientBuildGetMoreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              SINT32 numToReturn,
                              SINT64 contextID, UINT64 reqID,
                              BOOLEAN endianConvert )
{
   INT32 rc               = SDB_OK ;
   MsgOpGetMore *pGetMore = NULL ;
   INT32 packetLength = sizeof(MsgOpGetMore);
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // now the buffer is large enough
   pGetMore                       = (MsgOpGetMore*)(*ppBuffer) ;
   // nameLength does NOT include '\0'
   ossEndianConvertIf ( numToReturn, pGetMore->numToReturn, endianConvert ) ;
   ossEndianConvertIf ( contextID, pGetMore->contextID, endianConvert ) ;
   pGetMore->header.requestID     = reqID ;
   pGetMore->header.opCode        = MSG_BS_GETMORE_REQ ;
   pGetMore->header.messageLength = packetLength ;
   pGetMore->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pGetMore->header.version       = SDB_PROTOCOL_VER_2 ;
   pGetMore->header.flags         = 0 ;
   pGetMore->header.routeID.value = 0 ;
   pGetMore->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pGetMore->header.globalID), 0, sizeof(pGetMore->header.globalID) ) ;
   ossMemset( pGetMore->header.reserve, 0, sizeof(pGetMore->header.reserve) ) ;
   if ( endianConvert )
   {
      clientEndianConvertHeader ( &pGetMore->header ) ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildKillContextsMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                   UINT64 reqID,
                                   SINT32 numContexts,
                                   const SINT64 *pContextIDs,
                                   BOOLEAN endianConvert )
{
   INT32 rc               = SDB_OK ;
   MsgOpKillContexts *pKC = NULL ;
   SINT32 i               = 0 ;
   // aligned by 8 since contextIDs are 64 bits
   // so we don't need to manually align it, it must be 8 bytes aligned already
   INT32 packetLength = offsetof(MsgOpKillContexts, contextIDs) +
                        sizeof ( SINT64 ) * (numContexts) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // now the buffer is large enough
   pKC                       = (MsgOpKillContexts*)(*ppBuffer) ;
   pKC->header.requestID     = reqID ;
   pKC->header.opCode        = MSG_BS_KILL_CONTEXT_REQ ;
   pKC->header.messageLength = packetLength ;
   pKC->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pKC->header.version       = SDB_PROTOCOL_VER_2 ;
   pKC->header.flags         = 0 ;
   pKC->header.routeID.value = 0 ;
   pKC->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pKC->header.globalID), 0, sizeof(pKC->header.globalID) ) ;
   ossMemset( pKC->header.reserve, 0, sizeof(pKC->header.reserve) ) ;
   ossEndianConvertIf ( numContexts, pKC->numContexts, endianConvert ) ;
   if( endianConvert )
   {
      clientEndianConvertHeader ( &pKC->header ) ;
      for( ; i < numContexts ; i++ )
      {
         ossEndianConvertIf ( pContextIDs[i], pKC->contextIDs[i],
                              endianConvert ) ;
      }
   }
   else
   {
       // copy collection name
       ossMemcpy ( (CHAR*)(&pKC->contextIDs[0]), (CHAR*)pContextIDs,
                    sizeof(SINT64)*pKC->numContexts ) ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildInterruptMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                UINT64 reqID, BOOLEAN isSelf,
                                BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   MsgOpKillAllContexts *killAllContexts = NULL ;
   INT32 len = sizeof( MsgOpKillAllContexts ) +
               ossRoundUpToMultipleX( 0, sizeof(ossValuePtr) ) ;
   if ( len < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, len ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   killAllContexts                       = (MsgOpKillAllContexts *)
                                           ( *ppBuffer ) ;
   killAllContexts->header.messageLength = len ;
   killAllContexts->header.eye           = MSG_COMM_EYE_DEFAULT ;
   killAllContexts->header.version       = SDB_PROTOCOL_VER_2 ;
   killAllContexts->header.flags         = 0 ;
   killAllContexts->header.opCode = isSelf ? MSG_BS_INTERRUPTE_SELF :
                                    MSG_BS_INTERRUPTE ;
   killAllContexts->header.TID           = ossGetCurrentThreadID() ;
   killAllContexts->header.routeID.value = 0 ;
   killAllContexts->header.requestID     = reqID ;
   ossMemset( &(killAllContexts->header.globalID), 0,
              sizeof(killAllContexts->header.globalID) ) ;
   ossMemset( killAllContexts->header.reserve, 0,
              sizeof(killAllContexts->header.reserve) ) ;
   if ( endianConvert )
   {
      clientEndianConvertHeader ( &killAllContexts->header ) ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientExtractReply ( CHAR *pBuffer, SINT32 *flag, SINT64 *contextID,
                           SINT32 *startFrom, SINT32 *numReturned,
                           BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   MsgOpReply *pReply = (MsgOpReply*)pBuffer ;
   ossEndianConvertIf ( pReply->flags, *flag, endianConvert ) ;
   ossEndianConvertIf ( pReply->contextID, *contextID, endianConvert ) ;
   ossEndianConvertIf ( pReply->startFrom, *startFrom, endianConvert ) ;
   ossEndianConvertIf ( pReply->numReturned, *numReturned, endianConvert ) ;
   if ( endianConvert )
   {
      INT32 offset = 0, count = 0 ;
      clientEndianConvertHeader ( &pReply->header ) ;
      // convert variables endianess
      pReply->flags       = *flag ;
      pReply->contextID   = *contextID ;
      pReply->startFrom   = *startFrom ;
      pReply->numReturned = *numReturned ;
      // we need to take care of all records in the reply message
      offset = ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ;
      count = 0 ;
      while ( count < *numReturned && offset < pReply->header.messageLength )
      {
         off_t off = 0 ;
         if ( !bson_endian_convert ( &pBuffer[offset], &off, FALSE ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         offset += *(INT32*)&pBuffer[offset] ;
         offset = ossRoundUpToMultipleX ( offset, 4 ) ;
         ++count ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildDisconnectMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                 UINT64 reqID, BOOLEAN endianConvert )
{
   INT32 rc                     = SDB_OK ;
   MsgOpDisconnect *pDisconnect = NULL ;
   INT32 packetLength = ossRoundUpToMultipleX ( sizeof( MsgOpDisconnect ), 4) ;
   if ( packetLength < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, packetLength ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   pDisconnect                       = (MsgOpDisconnect*)(*ppBuffer) ;
   pDisconnect->header.requestID     = reqID ;
   pDisconnect->header.opCode        = MSG_BS_DISCONNECT ;
   pDisconnect->header.messageLength = packetLength ;
   pDisconnect->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pDisconnect->header.version       = SDB_PROTOCOL_VER_2 ;
   pDisconnect->header.flags         = 0 ;
   pDisconnect->header.routeID.value = 0 ;
   pDisconnect->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pDisconnect->header.globalID), 0,
              sizeof(pDisconnect->header.globalID) ) ;
   ossMemset( pDisconnect->header.reserve, 0,
              sizeof(pDisconnect->header.reserve) ) ;
   if( endianConvert )
   {
      clientEndianConvertHeader ( &pDisconnect->header ) ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientBuildSqlMsg( CHAR **ppBuffer, INT32 *bufferSize,
                         const CHAR *sql, UINT64 reqID,
                         BOOLEAN endianConvert )
{
   INT32 rc             = SDB_OK ;
   INT32 sqlLen         = 0 ;
   MsgOpSql *sqlMsg     = NULL ;
   INT32 len            = 0 ;

   if ( sql )
   {
      sqlLen = ossStrlen( sql ) + 1 ;
      len = sizeof( MsgOpSql ) +
            ossRoundUpToMultipleX( sqlLen, sizeof(ossValuePtr) ) ;
      if ( len < 0 )
      {
         ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = clientCheckBuffer ( ppBuffer, bufferSize, len ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

      sqlMsg                       = ( MsgOpSql *)( *ppBuffer ) ;
      sqlMsg->header.requestID     = reqID ;
      sqlMsg->header.opCode        = MSG_BS_SQL_REQ ;
      sqlMsg->header.messageLength = sizeof( MsgOpSql ) + sqlLen ;
      sqlMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
      sqlMsg->header.version       = SDB_PROTOCOL_VER_2 ;
      sqlMsg->header.flags         = 0 ;
      sqlMsg->header.routeID.value = 0 ;
      sqlMsg->header.TID           = ossGetCurrentThreadID() ;
      ossMemset( &(sqlMsg->header.globalID), 0, sizeof(sqlMsg->header.globalID) ) ;
      ossMemset( sqlMsg->header.reserve, 0, sizeof(sqlMsg->header.reserve) ) ;
      ossMemcpy( *ppBuffer + sizeof( MsgOpSql ),
                 sql, sqlLen ) ;
      if( endianConvert )
      {
         clientEndianConvertHeader ( &sqlMsg->header ) ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/** \fn INT32 clientBuildAuthVer0Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                      const CHAR *pUsrName,
                                      const CHAR *pPasswd,
                                      UINT64 reqID,
                                      BOOLEAN endianConvert )
    \brief Build auth msg when we use MD5 authentication.
    \param [in] bufferSize Size of msg buffer.
    \param [in] pUsrName User name.
    \param [in] pPasswd User password.
    \param [in] reqID Request ID.
    \param [in] endianConvert If true, it's big endian. And if false, it's
                little endian.
    \param [out] ppBuffer Msg buffer.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 clientBuildAuthVer0Msg( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *pUsrName,
                              const CHAR *pPasswd,
                              UINT64 reqID,
                              BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   INT32 msgLen = 0 ;
   bson obj ;
   INT32 bsonSize = 0 ;
   MsgAuthentication *msg = NULL ;

   if ( NULL == pUsrName || NULL == pPasswd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bson_init( &obj ) ;

   rc = bson_append_string( &obj, SDB_AUTH_USER, pUsrName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_PASSWD, pPasswd ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   bsonSize = bson_size( &obj ) ;
   msgLen = sizeof( MsgAuthentication ) +
            ossRoundUpToMultipleX( bsonSize, 4 ) ;
   if ( msgLen < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, msgLen ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   msg                       = ( MsgAuthentication * )(*ppBuffer) ;
   msg->header.requestID     = reqID ;
   msg->header.opCode        = MSG_AUTH_VERIFY_REQ ;
   msg->header.messageLength = sizeof( MsgAuthentication ) + bsonSize ;
   msg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   msg->header.version       = SDB_PROTOCOL_VER_2 ;
   msg->header.flags         = FLAG_RESULT_DETAIL ;
   msg->header.routeID.value = 0 ;
   msg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(msg->header.globalID), 0, sizeof( msg->header.globalID ) ) ;
   ossMemset( msg->header.reserve, 0, sizeof( msg->header.reserve ) ) ;
   if ( !endianConvert )
   {
      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 bson_data( &obj ), bsonSize ) ;
   }
   else
   {
      bson newobj ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &msg->header ) ;
      bson_init ( &newobj ) ;
      rc = bson_copy ( &newobj, &obj ) ;
      if ( SDB_OK != rc )
      {
         bson_destroy ( &newobj ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !bson_endian_convert ( (CHAR*)bson_data ( &newobj ) , &off, TRUE ) )
      {
         bson_destroy ( &newobj ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 bson_data( &newobj ), bsonSize ) ;
      bson_destroy ( &newobj ) ;
   }
done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

/** \fn INT32 clientBuildAuthVer1Step1Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                           const CHAR *pUsrName,
                                           UINT64 reqID,
                                           BOOLEAN endianConvert,
                                           const CHAR *clientNonceBase64 )
    \brief Build the first certification msg when we use SCRAM-SHA256
           authentication.
    \param [in] bufferSize Size of msg buffer.
    \param [in] pUsrName User name.
    \param [in] reqID Request ID.
    \param [in] endianConvert If true, it's big endian. And if false, it's
                little endian.
    \param [in] clientNonceBase64 Client nonce in base64.
    \param [out] ppBuffer Msg buffer.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 clientBuildAuthVer1Step1Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                   const CHAR *pUsrName,
                                   UINT64 reqID,
                                   BOOLEAN endianConvert,
                                   const CHAR *clientNonceBase64 )
{
   INT32 rc = SDB_OK ;
   INT32 msgLen = 0 ;
   bson obj ;
   INT32 bsonSize = 0 ;
   MsgAuthentication *msg = NULL ;

   bson_init( &obj ) ;

   if ( NULL == pUsrName || NULL == clientNonceBase64 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_int( &obj, SDB_AUTH_STEP, SDB_AUTH_STEP_1 ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_USER, pUsrName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_NONCE, clientNonceBase64 ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_int( &obj, SDB_AUTH_TYPE, SDB_AUTH_TYPE_MD5_PWD ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   bsonSize = bson_size( &obj ) ;
   msgLen = sizeof( MsgAuthentication ) +
            ossRoundUpToMultipleX( bsonSize, 4 ) ;
   if ( msgLen < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, msgLen ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   msg                       = ( MsgAuthentication * )(*ppBuffer) ;
   msg->header.requestID     = reqID ;
   msg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   msg->header.version       = SDB_PROTOCOL_VER_2 ;
   msg->header.flags         = 0 ;
   msg->header.opCode        = MSG_AUTH_VERIFY1_REQ ;
   msg->header.messageLength = sizeof( MsgAuthentication ) + bsonSize ;
   msg->header.routeID.value = 0 ;
   msg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(msg->header.globalID), 0, sizeof( msg->header.globalID ) ) ;
   if ( !endianConvert )
   {
      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 bson_data( &obj ), bsonSize ) ;
   }
   else
   {
      bson newobj ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &msg->header ) ;
      bson_init ( &newobj ) ;
      rc = bson_copy ( &newobj, &obj ) ;
      if ( SDB_OK != rc )
      {
         bson_destroy ( &newobj ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !bson_endian_convert ( (CHAR*)bson_data ( &newobj ) , &off, TRUE ) )
      {
         bson_destroy ( &newobj ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 bson_data( &newobj ), bsonSize ) ;
      bson_destroy ( &newobj ) ;
   }
done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

/** \fn INT32 clientBuildAuthVer1Step2Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                           const CHAR *pUsrName,
                                           UINT64 reqID,
                                           BOOLEAN endianConvert,
                                           const CHAR *combineNonceBase64,
                                           const CHAR *clientProofBase64,
                                           const CHAR *identify )
    \brief Build the first certification msg when we use SCRAM-SHA256
           authentication.
    \param [in] bufferSize Size of msg buffer.
    \param [in] pUsrName User name.
    \param [in] reqID Request ID.
    \param [in] endianConvert If true, it's big endian. And if false, it's
                little endian.
    \param [in] combineNonceBase64 Combine string for clien nonce in base64 and
                server nonce in base64.
    \param [in] clientNonceBase64 Client nonce in base64.
    \param [in] identify Session identifier. If the client is C++ driver,
                its value is "C++_Session". If the client is C driver, its
                value is "C_Session".
    \param [out] ppBuffer Msg buffer.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 clientBuildAuthVer1Step2Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                   const CHAR *pUsrName,
                                   UINT64 reqID,
                                   BOOLEAN endianConvert,
                                   const CHAR *combineNonceBase64,
                                   const CHAR *clientProofBase64,
                                   const CHAR *identify )
{
   INT32 rc = SDB_OK ;
   INT32 msgLen = 0 ;
   bson obj ;
   INT32 bsonSize = 0 ;
   MsgAuthentication *msg = NULL ;

   bson_init( &obj ) ;

   if ( NULL == pUsrName ||
        NULL == combineNonceBase64 ||
        NULL == clientProofBase64 ||
        NULL == identify )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_int( &obj, SDB_AUTH_STEP, SDB_AUTH_STEP_2 ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_USER, pUsrName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_NONCE, combineNonceBase64 ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_IDENTIFY, identify ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_PROOF, clientProofBase64 ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_int( &obj, SDB_AUTH_TYPE, SDB_AUTH_TYPE_MD5_PWD ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   bsonSize = bson_size( &obj ) ;
   msgLen = sizeof( MsgAuthentication ) +
            ossRoundUpToMultipleX( bsonSize, 4 ) ;
   if ( msgLen < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, msgLen ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   msg                       = ( MsgAuthentication * )(*ppBuffer) ;
   msg->header.requestID     = reqID ;
   msg->header.opCode        = MSG_AUTH_VERIFY1_REQ ;
   msg->header.messageLength = sizeof( MsgAuthentication ) + bsonSize ;
   msg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   msg->header.version       = SDB_PROTOCOL_VER_2 ;
   msg->header.flags         = 0 ;
   msg->header.routeID.value = 0 ;
   msg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(msg->header.globalID), 0, sizeof( msg->header.globalID ) ) ;
   ossMemset( msg->header.reserve, 0,
              sizeof( msg->header.reserve ) ) ;
   if ( !endianConvert )
   {
      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 bson_data( &obj ), bsonSize ) ;
   }
   else
   {
      bson newobj ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &msg->header ) ;
      bson_init ( &newobj ) ;
      rc = bson_copy ( &newobj, &obj ) ;
      if ( SDB_OK != rc )
      {
         bson_destroy ( &newobj ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !bson_endian_convert ( (CHAR*)bson_data ( &newobj ) , &off, TRUE ) )
      {
         bson_destroy ( &newobj ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 bson_data( &newobj ), bsonSize ) ;
      bson_destroy ( &newobj ) ;
   }
done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

INT32 clientBuildAuthDelMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *pUsrName,
                             const CHAR *pPasswd,
                             UINT64 reqID, BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   INT32 msgLen = 0 ;
   bson obj ;
   INT32 bsonSize = 0 ;
   MsgAuthDelUsr *msg = NULL ;
   if ( NULL == pUsrName || NULL == pPasswd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bson_init( &obj ) ;
   rc = bson_append_string( &obj, SDB_AUTH_USER, pUsrName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_string( &obj, SDB_AUTH_PASSWD, pPasswd ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   bsonSize = bson_size( &obj ) ;
   msgLen = sizeof( MsgAuthDelUsr ) +
            ossRoundUpToMultipleX( bsonSize, sizeof(ossValuePtr ) ) ;
   if ( msgLen < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer ( ppBuffer, bufferSize, msgLen ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   msg                       = ( MsgAuthDelUsr * )(*ppBuffer) ;
   msg->header.requestID     = reqID ;
   msg->header.opCode        = MSG_AUTH_DELUSR_REQ ;
   msg->header.routeID.value = 0 ;
   msg->header.TID           = ossGetCurrentThreadID() ;
   msg->header.messageLength = sizeof( MsgAuthDelUsr ) + bsonSize ;
   msg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   msg->header.version       = SDB_PROTOCOL_VER_2 ;
   msg->header.flags         = 0 ;
   ossMemset( &(msg->header.globalID), 0, sizeof(msg->header.globalID) ) ;
   ossMemset( msg->header.reserve, 0, sizeof(msg->header.reserve) ) ;

  if ( !endianConvert )
   {
      ossMemcpy( *ppBuffer + sizeof(MsgAuthDelUsr),
                 bson_data( &obj ), bsonSize ) ;
   }
   else
   {
      bson newobj ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &msg->header ) ;
      bson_init ( &newobj ) ;
      rc = bson_copy ( &newobj, &obj ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      if ( !bson_endian_convert ( (char*)bson_data(&newobj), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }
      ossMemcpy( *ppBuffer + sizeof(MsgAuthDelUsr),
                 bson_data( &newobj ), bsonSize ) ;

endian_convert_done :
      bson_destroy ( &newobj ) ;

      if ( rc )
      {
         goto error ;
      }
   }
done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

INT32 clientBuildTransactionBegMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                    UINT64 reqID, BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   MsgOpTransBegin *transBeginMsg = NULL ;
   INT32 len = sizeof( MsgOpTransBegin ) +
               ossRoundUpToMultipleX( 0, sizeof(ossValuePtr) ) ;
   if ( len < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientCheckBuffer ( ppBuffer, bufferSize, len ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   transBeginMsg                       = ( MsgOpTransBegin *)( *ppBuffer ) ;
   transBeginMsg->header.requestID     = reqID ;
   transBeginMsg->header.opCode        = MSG_BS_TRANS_BEGIN_REQ ;
   transBeginMsg->header.messageLength = sizeof( MsgOpTransBegin ) + 0 ;
   transBeginMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   transBeginMsg->header.version       = SDB_PROTOCOL_VER_2 ;
   transBeginMsg->header.flags         = 0 ;
   transBeginMsg->header.routeID.value = 0 ;
   transBeginMsg->header.TID           = ossGetCurrentThreadID() ;
   transBeginMsg->transID              = 0 ;
   ossMemset( &(transBeginMsg->header.globalID), 0, sizeof( transBeginMsg->header.globalID ) ) ;
   ossMemset( transBeginMsg->reserved, 0, sizeof( transBeginMsg->reserved ) ) ;
   ossMemset( transBeginMsg->header.reserve, 0,
              sizeof(transBeginMsg->header.reserve) ) ;
   if( endianConvert )
   {
      clientEndianConvertHeader ( &transBeginMsg->header ) ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildTransactionCommitMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                       UINT64 reqID,
                                       bson *hint,
                                       BOOLEAN endianConvert )
{
   bson emptyObj ;
   INT32 rc = SDB_OK ;
   SINT32 offset = 0 ;
   INT32 len = 0 ;
   MsgOpTransCommit *transCommitMsg = NULL ;
   bson_init ( &emptyObj ) ;
   bson_empty ( &emptyObj ) ;
   if ( !hint )
   {
      hint = &emptyObj ;
   }

   len = ossRoundUpToMultipleX( sizeof( MsgOpTransCommit ), 4 ) +
         ossRoundUpToMultipleX( bson_size(hint), 4 ) ;

   if ( len < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientCheckBuffer ( ppBuffer, bufferSize, len ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   transCommitMsg                       = ( MsgOpTransCommit *)( *ppBuffer ) ;
   transCommitMsg->header.requestID     = reqID ;
   transCommitMsg->header.opCode        = MSG_BS_TRANS_COMMIT_REQ ;
   transCommitMsg->header.messageLength = len ;
   transCommitMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   transCommitMsg->header.version       = SDB_PROTOCOL_VER_2 ;
   transCommitMsg->header.flags         = 0 ;
   transCommitMsg->header.routeID.value = 0 ;
   transCommitMsg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(transCommitMsg->header.globalID), 0,
              sizeof(transCommitMsg->header.globalID) ) ;
   ossMemset( transCommitMsg->header.reserve, 0,
              sizeof(transCommitMsg->header.reserve) ) ;

   offset = ossRoundUpToMultipleX( sizeof( MsgOpTransCommit ), 4 ) ;

   if ( !endianConvert )
   {
      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(hint),
                                          bson_size(hint));
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;
   }
   else
   {
      bson newhint ;
      off_t off = 0 ;
      clientEndianConvertHeader ( &transCommitMsg->header ) ;
      bson_init ( &newhint ) ;

      rc = bson_copy ( &newhint, hint ) ;
      if ( rc )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      off = 0 ;
      if ( !bson_endian_convert ( (char*)bson_data(&newhint), &off, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         goto endian_convert_done ;
      }

      ossMemcpy ( &((*ppBuffer)[offset]), bson_data(&newhint),
                                          bson_size(hint));
      offset += ossRoundUpToMultipleX( bson_size(hint), 4 ) ;

endian_convert_done :
      bson_destroy ( &newhint ) ;

      if ( rc )
      {
         goto error ;
      }
   }
   // sanity test
   if ( offset != len )
   {
      ossPrintf ( "Invalid packet length"OSS_NEWLINE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildTransactionRollbackMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                         UINT64 reqID,
                                         BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   MsgOpTransRollback *transRollbackMsg = NULL ;
   INT32 len = sizeof( MsgOpTransRollback ) +
               ossRoundUpToMultipleX( 0, sizeof(ossValuePtr) ) ;
   if ( len < 0 )
   {
      ossPrintf ( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientCheckBuffer ( ppBuffer, bufferSize, len ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   transRollbackMsg                       = ( MsgOpTransRollback *)( *ppBuffer ) ;
   transRollbackMsg->header.requestID     = reqID ;
   transRollbackMsg->header.opCode        = MSG_BS_TRANS_ROLLBACK_REQ ;
   transRollbackMsg->header.messageLength = sizeof( MsgOpTransBegin ) + 0 ;
   transRollbackMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   transRollbackMsg->header.version       = SDB_PROTOCOL_VER_2 ;
   transRollbackMsg->header.flags         = 0 ;
   transRollbackMsg->header.routeID.value = 0 ;
   transRollbackMsg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(transRollbackMsg->header.globalID), 0,
              sizeof(transRollbackMsg->header.globalID) ) ;
   ossMemset( transRollbackMsg->header.reserve, 0,
              sizeof(transRollbackMsg->header.reserve) ) ;
   if( endianConvert )
   {
      clientEndianConvertHeader ( &transRollbackMsg->header ) ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildSysInfoRequest ( CHAR **ppBuffer, INT32 *pBufferSize )
{
   INT32 rc = SDB_OK ;
   MsgSysInfoRequest *request = NULL ;
   rc = clientCheckBuffer ( ppBuffer, pBufferSize, sizeof(MsgSysInfoRequest) ) ;
   if ( rc )
   {
      goto error ;
   }
   request                                   = (MsgSysInfoRequest*)(*ppBuffer) ;
   request->header.specialSysInfoLen         = MSG_SYSTEM_INFO_LEN ;
   request->header.eyeCatcher                = MSG_SYSTEM_INFO_EYECATCHER ;
   request->header.realMessageLength         = sizeof(MsgSysInfoRequest) ;
done :
   return rc ;
error :
   goto done ;
}

INT32 clientExtractSysInfoReply ( CHAR *pBuffer, BOOLEAN *endianConvert,
                                  INT32 *osType, INT32 *authVersion,
                                  INT16 *peerProtocolVersion,
                                  UINT64 *dbStartTime,
                                  UINT8 *version, UINT8 *subVersion,
                                  UINT8 *fixVersion )
{
   INT32 rc = SDB_OK ;
   MsgSysInfoReply *reply = (MsgSysInfoReply*)pBuffer ;
   BOOLEAN e = FALSE ;
   if ( reply->header.eyeCatcher == MSG_SYSTEM_INFO_EYECATCHER )
   {
      e = FALSE ;
   }
   else if ( reply->header.eyeCatcher == MSG_SYSTEM_INFO_EYECATCHER_REVERT )
   {
      e = TRUE ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( osType )
   {
      ossEndianConvertIf4( reply->osType, *osType, e ) ;
   }
   if ( authVersion )
   {
      ossEndianConvertIf4( reply->authVersion, *authVersion, e ) ;
   }
   if ( dbStartTime )
   {
      ossEndianConvertIf8( reply->dbStartTime, *dbStartTime, e ) ;
   }
   if ( version )
   {
      ossEndianConvertIf1( reply->version, *version, e ) ;
   }
   if ( subVersion )
   {
      ossEndianConvertIf1( reply->subVersion, *subVersion, e ) ;
   }
   if ( fixVersion )
   {
      ossEndianConvertIf1( reply->fixVersion, *fixVersion, e ) ;
   }
   if ( peerProtocolVersion )
   {
      BYTE digest[ SDB_MD5_DIGEST_LENGTH ] = { 0 } ;
      md5_state_t st ;
      md5_init( &st ) ;
      md5_append( &st, (const md5_byte_t *)reply ,
                  sizeof(MsgSysInfoReply) - sizeof(reply->fingerprint) ) ;
      md5_finish( &st, digest ) ;
      *peerProtocolVersion =
            ( 0 == ossStrncmp( reply->fingerprint,
                               (const CHAR *)digest,
                               sizeof(reply->fingerprint) ) ) ?
            SDB_PROTOCOL_VER_2 : SDB_PROTOCOL_VER_1 ;
   }
   if ( endianConvert )
   {
      *endianConvert = e ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 clientValidateSql( const CHAR *sql, BOOLEAN isExec )
{
   INT32 rc = SDB_INVALIDARG ;
   UINT32 size = ossStrlen( sql ) ;
   UINT32 i = 0 ;
   for ( ; i < size; i++ )
   {
      if ( ' ' == sql[i] || '\t' == sql[i] )
      {
         continue ;
      }
      else if ( isExec )
      {
         rc = ( 's' == sql[i] || 'S' == sql[i] ||
                'l' == sql[i] || 'L' == sql[i]) ?
              SDB_OK : SDB_INVALIDARG ;
         break ;
      }
      else
      {
         rc = ( 's' != sql[i] && 'S' != sql[i] &&
                'l' != sql[i] && 'L' != sql[i]) ?
              SDB_OK : SDB_INVALIDARG ;
         break ;
      }
   }

   return rc ;
}

INT32 clientBuildTestMsg( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *msg, UINT64 reqID,
                          BOOLEAN endianConvert )
{
   INT32 rc       = SDB_OK ;
   INT32 msgLen   = 0 ;
   INT32 len      = 0 ;
   MsgOpMsg *msgOpMsg = NULL ;
   msgLen = ossStrlen( msg ) + 1 ;
   len = sizeof( MsgOpMsg ) + ossRoundUpToMultipleX( msgLen,
                                                     sizeof(ossValuePtr) ) ;
   if ( len < 0 )
   {
      ossPrintf( "Packet size overflow"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientCheckBuffer( ppBuffer, bufferSize, len ) ;
   if ( rc )
   {
      ossPrintf( "Failed to check buffer, rc = %d"OSS_NEWLINE, rc );
      goto error ;
   }
   msgOpMsg = (MsgOpMsg*)(*ppBuffer) ;
   // append the massage header
   msgOpMsg->header.requestID     = reqID ;
   msgOpMsg->header.opCode        = MSG_BS_MSG_REQ ;
   msgOpMsg->header.messageLength = len ;
   msgOpMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   msgOpMsg->header.version       = SDB_PROTOCOL_VER_2 ;
   msgOpMsg->header.flags         = 0 ;
   msgOpMsg->header.routeID.value = 0 ;
   msgOpMsg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(msgOpMsg->header.globalID), 0, sizeof(msgOpMsg->header.globalID) ) ;
   ossMemset( msgOpMsg->header.reserve, 0, sizeof(msgOpMsg->header.reserve) ) ;

   ossMemcpy( msgOpMsg->msg, msg, msgLen ) ;
   if( endianConvert )
   {
      clientEndianConvertHeader ( &msgOpMsg->header ) ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildGetLobRTimeMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                 SINT32 flags, SINT16 w, SINT64 contextID,
                                 UINT64 reqID, BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_GETRTDETAIL_REQ, NULL,
                           flags, w, contextID, reqID, NULL,
                           NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildWriteLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *buf, UINT32 len,
                              SINT64 lobOffset, SINT32 flags,
                              SINT16 w, SINT64 contextID,
                              UINT64 reqID,
                              BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_WRITE_REQ, NULL,
                           flags, w, contextID, reqID, &lobOffset,
                           &len, buf, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildLockLobMsg( CHAR ** ppBuffer, INT32 *bufferSize,
                             INT64 offset, INT64 length,
                             SINT32 flags, SINT16 w,
                             SINT64 contextID, UINT64 reqID,
                             BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   bson obj ;

   bson_init( &obj ) ;

   rc = bson_append_long( &obj, FIELD_NAME_LOB_OFFSET, offset ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_long( &obj, FIELD_NAME_LOB_LENGTH, length ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_LOCK_REQ, &obj,
                           flags, w, contextID, reqID, NULL,
                           NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

INT32 clientBuildReadLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             UINT32 len, SINT64 lobOffset,
                             SINT32 flags, SINT64 contextID,
                             UINT64 reqID,
                             BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_READ_REQ, NULL,
                           flags, 1, contextID, reqID,
                           &lobOffset, &len, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 clientBuildCloseLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                              SINT32 flags, SINT16 w,
                              SINT64 contextID, UINT64 reqID,
                              BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   rc = clientBuildLobMsg( ppBuffer, bufferSize,
                           MSG_BS_LOB_CLOSE_REQ, NULL,
                           flags, w, contextID, reqID,
                           NULL, NULL, NULL, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

/*
   Other tool functions
*/
INT32 md5Encrypt( const CHAR *src,
                  CHAR *code,
                  UINT32 size )
{
   INT32 rc                                  = 0 ;
   INT32 i                                   = 0 ;
   UINT8 md5digest [ SDB_MD5_DIGEST_LENGTH ] = {0} ;
   md5_state_t st ;

   /* sanity check */
   if ( size <= 2*SDB_MD5_DIGEST_LENGTH )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   md5_init ( &st ) ;
   md5_append ( &st, (const md5_byte_t *) src, ossStrlen(src) ) ;
   md5_finish ( &st, md5digest ) ;
   for ( ; i < SDB_MD5_DIGEST_LENGTH; i++ )
   {
      ossSnprintf( code, 3, "%02x", md5digest[i]) ;
      code += 2 ;
   }
done :
   return rc ;
error :
   goto done ;
}

BOOLEAN isMd5String( const CHAR *str )
{
   UINT32 len = strlen( str ) ;
   if ( len == 2*SDB_MD5_DIGEST_LENGTH )
   {
      while( *str )
      {
         if ( ( *str >= '0' && *str <= '9' ) ||
              ( *str >= 'A' && *str <= 'F' ) ||
              ( *str >= 'a' && *str <= 'f' ) )
         {
            ++str ;
         }
         else
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }
   return FALSE ;
}

INT32 clientReplicaGroupExtractNode ( const CHAR *data,
                                      CHAR *pHostName,
                                      INT32 hostNameSize,
                                      CHAR *pServiceName,
                                      INT32 serviceNameSize,
                                      INT32 *pNodeID )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_iterator ele ;
   bson_init ( &obj ) ;

   if ( !data || !pHostName || !pServiceName || !pNodeID )
   {
      rc = SDB_INVALIDARG ;
      goto done ;
   }
   // bson_init_finished_data does not accept const CHAR*, however since we are
   // NOT going to perform any change, it's safe to cast const CHAR* to CHAR*
   // here
   bson_init_finished_data ( &obj, (CHAR*)data ) ;
   // find the host name
   if ( BSON_STRING == bson_find ( &ele, &obj, CAT_HOST_FIELD_NAME ) )
   {
      ossStrncpy ( pHostName, bson_iterator_string ( &ele ),
                   hostNameSize ) ;
   }

   // find the node id
   if ( BSON_INT == bson_find ( &ele, &obj, CAT_NODEID_NAME ) )
   {
      *pNodeID = bson_iterator_int ( &ele ) ;
   }

   // walk through services
   if ( BSON_ARRAY != bson_find ( &ele, &obj, CAT_SERVICE_FIELD_NAME ) )
   {
      // the service is not array
      rc = SDB_SYS ;
      goto error ;
   }
   {
      const CHAR *serviceList = bson_iterator_value ( &ele ) ;
      bson_iterator_from_buffer ( &ele, serviceList ) ;
      // loop for all elements in Services
      while ( bson_iterator_next ( &ele ) )
      {
         bson intObj ;
         bson_init ( &intObj ) ;
         // make sure each element is object and construct intObj object
         // bson_init_finished_data does not accept const CHAR*,
         // however since we are NOT going to perform any change, it's safe
         // to cast const CHAR* to CHAR* here
         if ( BSON_OBJECT == bson_iterator_type ( &ele ) &&
              BSON_OK == bson_init_finished_data ( &intObj,
                         (CHAR*)bson_iterator_value ( &ele ) ) )
         {
            bson_iterator k ;
            // look for Type
            if ( BSON_INT != bson_find ( &k, &intObj,
                                         CAT_SERVICE_TYPE_FIELD_NAME ) )
            {
               rc = SDB_SYS ;
               bson_destroy ( &intObj ) ;
               goto error ;
            }
            // if we find the right service, let's record the service name
            if ( MSG_ROUTE_LOCAL_SERVICE == bson_iterator_int ( &k ) )
            {
               if ( BSON_STRING == bson_find ( &k, &intObj,
                    CAT_SERVICE_NAME_FIELD_NAME ) )
               {
                  ossStrncpy ( pServiceName,
                               bson_iterator_string ( &k ),
                               serviceNameSize ) ;
                  bson_destroy ( &intObj ) ;
                  goto done ;
               }
            }
         }
      }
   }
done :
   bson_destroy ( &obj ) ;
   return rc ;
error :
   goto done ;
}

INT32 clientSnprintf( CHAR* pBuffer, INT32 bufSize, const CHAR* pFormat, ... )
{
   va_list ap ;
   INT32 n = -1 ;
   if ( !pBuffer || bufSize <= 0 )
   {
      return -1 ;
   }
   va_start( ap, pFormat ) ;
#if defined (_WINDOWS)
   n=_vsnprintf_s( pBuffer, bufSize, _TRUNCATE, pFormat, ap ) ;
#else
   n=vsnprintf( pBuffer, bufSize, pFormat, ap ) ;
#endif
   va_end( ap ) ;
   // Set terminate if the length is greater than buffer size
   if ( n <= 0 )
   {
      pBuffer[0] = '\0' ;
      n = -1 ;
   }
   else if ( n >= bufSize )
   {
      n = bufSize - 1 ;
      pBuffer[n] = '\0' ;
   }
   else
   {
      pBuffer[n] = '\0' ;
   }
   return n ;
}

void clientMsgHeaderUpgrade( const MsgHeaderV1 *msgHeader,
                             MsgHeader *newMsgHeader )
{
   newMsgHeader->messageLength = msgHeader->messageLength +
                                ( sizeof(MsgHeader) - sizeof(MsgHeaderV1) ) ;
   newMsgHeader->eye = MSG_COMM_EYE_DEFAULT ;
   newMsgHeader->version = SDB_PROTOCOL_VER_2 ;
   newMsgHeader->flags = 0 ;
   newMsgHeader->opCode = msgHeader->opCode ;
   newMsgHeader->TID = msgHeader->TID ;
   newMsgHeader->routeID = msgHeader->routeID ;
   newMsgHeader->requestID = msgHeader->requestID ;
   ossMemset( &(newMsgHeader->globalID), 0, sizeof( newMsgHeader->globalID) ) ;
   ossMemset( newMsgHeader->reserve, 0, sizeof( newMsgHeader->reserve) ) ;
}

void clientMsgHeaderDowngrade( const MsgHeader *msgHeader,
                               MsgHeaderV1 *newMsgHeader )
{
   newMsgHeader->messageLength = msgHeader->messageLength -
                                ( sizeof(MsgHeader) - sizeof(MsgHeaderV1) ) ;
   newMsgHeader->opCode = msgHeader->opCode ;
   newMsgHeader->TID = msgHeader->TID ;
   newMsgHeader->routeID = msgHeader->routeID ;
   newMsgHeader->requestID = msgHeader->requestID ;
}

void clientMsgReplyHeaderUpgrade( const MsgOpReplyV1 *replyHeader,
                                  MsgOpReply *newReplyHeader )
{
   clientMsgHeaderUpgrade( &replyHeader->header, &newReplyHeader->header ) ;
   newReplyHeader->header.messageLength = replyHeader->header.messageLength +
      sizeof(MsgOpReply) - sizeof(MsgOpReplyV1);
   newReplyHeader->contextID = replyHeader->contextID ;
   newReplyHeader->flags = replyHeader->flags ;
   newReplyHeader->startFrom = replyHeader->startFrom ;
   newReplyHeader->numReturned = replyHeader->numReturned ;
   newReplyHeader->returnMask = 0 ;
}


