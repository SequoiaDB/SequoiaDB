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

   Source File Name = dmsCompress.cpp

   Descriptive Name =

   When/how to use: str util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsCompress.hpp"
#include "pmdEDU.hpp"
#include "dmsRecord.hpp"
#include "dmsTrace.hpp"
#include "utilCompressor.hpp"

using namespace bson ;

namespace engine
{
   _dmsCompressorEntry::_dmsCompressorEntry()
   : _compressor( NULL ),
     _dictionary( UTIL_INVALID_DICT ),
     _flags( UTIL_COMPRESS_EMPTY_FLAG )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCOMPRESSORENTRY_SETCOMPRESSOR, "_dmsCompressorEntry::setCompressor" )
   void _dmsCompressorEntry::setCompressor( _utilCompressor *compressor )
   {
      PD_TRACE_ENTRY( SDB__DMSCOMPRESSORENTRY_SETCOMPRESSOR ) ;

      _compressor = compressor ;

      PD_TRACE_EXIT( SDB__DMSCOMPRESSORENTRY_SETCOMPRESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCOMPRESSORENTRY_SETDICTIONARY, "_dmsCompressorEntry::setDictionary" )
   void _dmsCompressorEntry::setDictionary( const utilDictHandle dictionary )
   {
      PD_TRACE_ENTRY( SDB__DMSCOMPRESSORENTRY_SETDICTIONARY ) ;

      _dictionary = dictionary ;

      PD_TRACE_EXIT( SDB__DMSCOMPRESSORENTRY_SETDICTIONARY ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCOMPRESSORENTRY_SETFLAGS, "_dmsCompressorEntry::setFlags" )
   void _dmsCompressorEntry::setFlags ( UINT8 flags )
   {
      PD_TRACE_ENTRY( SDB__DMSCOMPRESSORENTRY_SETFLAGS ) ;

      _flags = flags ;

      PD_TRACE_EXIT( SDB__DMSCOMPRESSORENTRY_SETFLAGS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCOMPRESSORENTRY_RESET, "_dmsCompressorEntry::reset" )
   void _dmsCompressorEntry::reset()
   {
      PD_TRACE_ENTRY( SDB__DMSCOMPRESSORENTRY_RESET );

      _compressor = NULL ;
      _dictionary = UTIL_INVALID_DICT ;
      _flags = UTIL_COMPRESS_EMPTY_FLAG ;

      PD_TRACE_EXIT( SDB__DMSCOMPRESSORENTRY_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCOMPRESS2, "dmsCompress" )
   INT32 dmsCompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                       const CHAR *pInputData, INT32 inputSize,
                       const CHAR **ppData, INT32 *pDataSize,
                       UINT8 &ratio )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSCOMPRESS2 ) ;
      CHAR *pBuff = NULL ;
      UINT32 compressedLen = 0 ;

      SDB_ASSERT ( pInputData && ppData && pDataSize,
                   "Data pointer and size pointer can't be NULL" ) ;
      SDB_ASSERT( compressorEntry,
                  "Compressor entry pointer can't be NULL" ) ;

      _utilCompressor *compressor = compressorEntry->getCompressor() ;
      const utilDictHandle dictionary = compressorEntry->getDictionary( compressor ) ;
      SDB_ASSERT( compressor, "Compressor pointer can't be NULL" ) ;
      if ( !compressor )
      {
         PD_LOG( PDERROR, "Occur serious error: "
                 "The compressor handle is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = compressor->compressBound( inputSize, compressedLen,
                                      dictionary ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get max compressed length, rc: %d", rc ) ;
      pBuff = cb->getCompressBuff( compressedLen ) ;
      if ( !pBuff )
      {
         PD_LOG( PDERROR, "Failed to alloc compress buff, size: %d",
                 compressedLen ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = compressor->compress( pInputData, inputSize, pBuff,
                                 compressedLen, dictionary ) ;
      if ( rc )
      {
         PD_LOG( ( ( SDB_UTIL_COMPRESS_ABORT == rc ) ? PDINFO : PDERROR ),
                   "Failed to compress data, rc: %d", rc ) ;
         goto error ;
      }

      // assign the output buffer pointer
      if ( ppData )
      {
         *ppData = pBuff ;
      }
      if ( pDataSize )
      {
         *pDataSize = (INT32)compressedLen ;
      }
      ratio = (UINT8)( (*pDataSize) * 100 / inputSize ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSCOMPRESS2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCOMPRESS, "dmsCompress" )
   INT32 dmsCompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                       const BSONObj &obj, const CHAR* pOIDPtr, INT32 oidLen,
                       const CHAR **ppData, INT32 *pDataSize, UINT8 &ratio  )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSCOMPRESS ) ;
      CHAR *pObjData = NULL ;
      INT32 objSize = 0 ;

      SDB_ASSERT( compressorEntry, "Compressor entry can't be NULL" ) ;

      // if we want to append OID, then
      if ( oidLen && pOIDPtr )
      {
         // get the requested size by adding object size and oid size
         UINT32 requestedSize = obj.objsize() + oidLen ;
         rc = cb->allocBuff( requestedSize, &pObjData, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to alloc tmp buffer, size: %u",
                    requestedSize ) ;
            goto error ;
         }

         /// copy to new data
         *(UINT32*)pObjData = oidLen + obj.objsize() ;
         ossMemcpy( pObjData + sizeof(UINT32), pOIDPtr, oidLen ) ;
         ossMemcpy( pObjData + sizeof(UINT32) + oidLen,
                    obj.objdata() + sizeof(UINT32),
                    obj.objsize() - sizeof(UINT32) ) ;

         objSize = BSONObj(pObjData).objsize() ;
         rc = dmsCompress ( cb, compressorEntry, pObjData,
                            objSize, ppData, pDataSize, ratio ) ;
      }
      else
      {
         objSize = obj.objsize() ;
         rc = dmsCompress( cb, compressorEntry, obj.objdata(),
                           objSize, ppData, pDataSize, ratio ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( pObjData )
      {
         cb->releaseBuff( pObjData ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSCOMPRESS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSUNCOMPRESS, "dmsUncompress" )
   INT32 dmsUncompress ( _pmdEDUCB *cb, _dmsCompressorEntry *compressorEntry,
                         UINT8 compressType, const CHAR *pInputData, INT32 inputSize,
                         const CHAR **ppData, INT32 *pDataSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSUNCOMPRESS ) ;
      CHAR *pBuff = NULL ;
      UINT32 uncompressedLen = 0 ;

      SDB_ASSERT ( pInputData && ppData && pDataSize,
                   "Data pointer and size pointer can't be NULL" ) ;
      SDB_ASSERT( compressorEntry,
                  "Compressor entry pointer can't be NULL" ) ;

      _utilCompressor *compressor = compressorEntry->getCompressor() ;

      /// The compress type of collection had been altered
      if ( OSS_BIT_TEST( compressorEntry->getFlags(), UTIL_COMPRESS_ALTERABLE_FLAG ) )
      {
         compressor = getCompressorByType( (UTIL_COMPRESSOR_TYPE)compressType ) ;
      }

      SDB_ASSERT( compressor, "Compressor pointer can't be NULL" ) ;

      /// To compitable with the bug:'When not use compress, the compressor
      /// be set to snappy, so the data maybe compressed with snappy. But,
      /// restart the node, the compressor be set to null, so can't
      /// uncompressed'.
      if ( !compressor )
      {
         compressor = getCompressorByType( UTIL_COMPRESSOR_SNAPPY ) ;
      }

      if ( !compressor )
      {
         PD_LOG( PDERROR, "Occur serious error: "
                 "The compressor handle is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = compressor->getUncompressedLen( pInputData, inputSize,
                                           uncompressedLen ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get uncompressed length, rc: %d", rc ) ;

      pBuff = cb->getUncompressBuff( uncompressedLen ) ;
      if ( !pBuff )
      {
         PD_LOG( PDERROR, "Failed to allocate decompression buff, size: %d",
                 uncompressedLen ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = compressor->decompress( pInputData, inputSize, pBuff,
                                   uncompressedLen,
                                   compressorEntry->getDictionary( compressor ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to decompress data, rc: %d", rc ) ;

      // assign return value
      if ( ppData )
      {
         *ppData = pBuff ;
      }
      if ( pDataSize )
      {
         *pDataSize = uncompressedLen ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSUNCOMPRESS, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}

