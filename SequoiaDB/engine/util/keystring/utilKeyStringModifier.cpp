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

   Source File Name = utilKeyStringModifier.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "keystring/utilKeyStringModifier.hpp"
#include "keystring/utilKeyStringCoder.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "utilStrictBuffer.hpp"

namespace engine
{
namespace keystring
{

   /*
      _keyStringModifier implement
    */
   _keyStringModifier::_keyStringModifier( const keyString &src )
   : _src( src )
   {
   }

   _keyStringModifier::~_keyStringModifier()
   {
      if ( nullptr != _buffer )
      {
         _allocator.free( _buffer ) ;
         _buffer = nullptr ;
      }
   }

   void _keyStringModifier::init( const keyString &ks )
   {
      reset() ;
      _src = ks ;
   }

   void _keyStringModifier::reset()
   {
      _src.reset() ;
      if ( nullptr != _buffer )
      {
         _allocator.free( _buffer ) ;
         _buffer = nullptr ;
      }
      _bufferSize = 0 ;
   }

   INT32 _keyStringModifier::incRID( BOOLEAN force )
   {
      INT32 rc = SDB_OK ;

      utilStrictBuffer buffer ;
      dmsRecordID rid ;
      UINT32 bufferOffset = 0 ;

      if ( OSS_UNLIKELY( !_src.isValid() ) )
      {
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      else if ( _src.getKeyTailSize() < keyStringCoder::RID_ENCODING_SIZE )
      {
         PD_LOG( PDERROR, "has no rid coded" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rid = _src.getRID() ;
      if ( !force && !rid.isValid() )
      {
         PD_LOG( PDERROR, "can not modify invalid rid unforced" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      else if ( rid.isMax() )
      {
         PD_LOG( PDERROR, "already been max rid" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      ++ rid._offset ;

      rc = _ensureBuffer( _src.getRawDataSize() + _src.getTailDataSize() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      bufferOffset = _src.getKeySizeExceptTail() ;
      buffer.makeWritable( _bufferSize, _buffer ) ;
      buffer.write( 0, _src.getRawDataSize(), _src.getRawDataPtr() ) ;
      if ( _src.getTailDataSize() > 0 )
      {
         buffer.write( _src.getRawDataSize(),
                       _src.getTailDataSize(),
                       _src.getTailDataPtr() ) ;
      }
      keyStringCoder().encodeRID(
            rid, buffer.getWritablePtr( bufferOffset, sizeof( UINT32 ) + sizeof( UINT32 ) ) ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _keyStringModifier::decRID( BOOLEAN force )
   {
      INT32 rc = SDB_OK ;

      utilStrictBuffer buffer ;
      dmsRecordID rid ;
      UINT32 bufferOffset = 0 ;

      if ( OSS_UNLIKELY( !_src.isValid() ) )
      {
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      else if ( _src.getKeyTailSize() < keyStringCoder::RID_ENCODING_SIZE )
      {
         PD_LOG( PDERROR, "has no rid coded" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rid = _src.getRID() ;
      if ( !force && !rid.isValid() )
      {
         PD_LOG( PDERROR, "can not modify invalid rid unforced" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      else if ( rid.isMin() )
      {
         PD_LOG( PDERROR, "already been min rid" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      -- rid._offset ;

      rc = _ensureBuffer( _src.getRawDataSize() + _src.getTailDataSize() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      bufferOffset = _src.getKeySizeExceptTail() ;
      buffer.makeWritable( _bufferSize, _buffer ) ;
      buffer.write( 0, _src.getRawDataSize(), _src.getRawDataPtr() ) ;
      if ( _src.getTailDataSize() > 0 )
      {
         buffer.write( _src.getRawDataSize(),
                       _src.getTailDataSize(),
                       _src.getTailDataPtr() ) ;
      }
      keyStringCoder().encodeRID(
            rid, buffer.getWritablePtr( bufferOffset, sizeof( UINT32 ) + sizeof( UINT32 ) ) ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   keyString _keyStringModifier::getShallowKeyString() const
   {
      SDB_ASSERT( 0 < _bufferSize, "can not be invalid" ) ;
      return keyString( utilSlice( _bufferSize, _buffer ) ) ;
   }

   INT32 _keyStringModifier::_ensureBuffer( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 0 < size, "can not be invalid" ) ;
      if ( size <= _bufferSize )
      {
         goto done ;
      }
      else if ( nullptr == _buffer )
      {
         CHAR *buffer = (CHAR *)( _allocator.malloc( size ) ) ;
         if ( OSS_UNLIKELY( nullptr == buffer ) )
         {
            PD_LOG( PDERROR, "Failed to allocate memory" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         _buffer = buffer ;
         _bufferSize = size ;
      }
      else
      {
         CHAR *buffer = (CHAR *)( _allocator.realloc( _buffer, size ) ) ;
         if ( OSS_UNLIKELY( nullptr == buffer ) )
         {
            PD_LOG( PDERROR, "Failed to allocate memory" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         _buffer = buffer ;
         _bufferSize = size ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}
}
