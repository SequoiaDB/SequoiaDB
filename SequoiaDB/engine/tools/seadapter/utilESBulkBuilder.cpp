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

   Source File Name = utilESBulkBuilder.cpp

   Descriptive Name = Elasticsearch bulk operation builder.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilESBulkBuilder.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

#define BULK_UPDATE_PREFIX          "{\"doc\":"
#define BULK_UPDATE_PREFIX_LEN      ossStrlen( BULK_UPDATE_PREFIX )
#define BULK_UPDATE_SUFFIX          "}"
#define BULK_UPDATE_SUFFIX_LEN      ossStrlen( BULK_UPDATE_SUFFIX )

namespace seadapter
{
   _utilESBulkActionBase::_utilESBulkActionBase( const CHAR *index,
                                                 const CHAR *type )
   : _ownData( FALSE ),
     _index( index ),
     _type( type ),
     _id( NULL )
   {
   }

   _utilESBulkActionBase::~_utilESBulkActionBase()
   {
   }

   INT32 _utilESBulkActionBase::setID( const CHAR *id )
   {
      INT32 rc = SDB_OK ;
      if ( !id || ( 0 == ossStrlen( id ) )
           || ( ossStrlen( id ) > SEADPT_MAX_ID_SZ ))
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "_id is invalid" ) ;
         goto error ;
      }

      _id = id ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESBulkActionBase::setSourceData( const BSONObj &record )
   {
      SDB_ASSERT( !record.isEmpty(), "Data is empty" ) ;
      _dataObj = record ;
   }

   UINT32 _utilESBulkActionBase::outSizeEstimate() const
   {
      return ( UTIL_ESBULK_MIN_META_SIZE + ossStrlen( _index ) +
               ossStrlen( _type ) + ossStrlen( _id ) + _dataObj.objsize() ) ;
   }

   INT32 _utilESBulkActionBase::output( CHAR *buffer, INT32 size,
                                        INT32 &length, BOOLEAN withIndex,
                                        BOOLEAN withType, BOOLEAN withID ) const
   {
      INT32 rc = SDB_OK ;
      INT32 metaLen = 0 ;
      INT32 dataLen = 0 ;

      if ( (UINT32)size < outSizeEstimate() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[%d] is too small", size ) ;
         goto error ;
      }

      rc = _outputActionAndMeta( buffer, size, metaLen,
                                 withIndex, withType, withID ) ;
      PD_RC_CHECK( rc, PDERROR, "Output action and metadata failed[ %d ]",
                   rc ) ;

      if ( _hasSourceData() )
      {
         rc = _outputSrcData( buffer + metaLen, size - metaLen, dataLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Output source data failed[ %d ]", rc ) ;
      }

      length = metaLen + dataLen ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // The format of the action and metadata is as follows:
   // { action : { metadata }}\n
   // metadata can be empty, or it may contain index, type, _id of the
   // document. That's optional, depending on the url string of _bulk.
   INT32 _utilESBulkActionBase::_outputActionAndMeta( CHAR *buffer,
                                                      INT32 size,
                                                      INT32 &length,
                                                      BOOLEAN withIndex,
                                                      BOOLEAN withType,
                                                      BOOLEAN withID ) const
   {
      INT32 rc = SDB_OK ;
      BOOLEAN begin = TRUE ;
      INT32 writePos = 0 ;
      INT32 writeNum = 0 ;

      writeNum = ossSnprintf( buffer, size, "{\"%s\":{", _getActionName() ) ;
      writePos += writeNum ;
      if ( writePos >= size - 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[%d] is too small", size ) ;
         goto error ;
      }

      if ( withIndex )
      {
         writeNum = ossSnprintf( buffer + writePos, size - writePos,
                                 "\"_index\":\"%s\"", _index ) ;
         writePos += writeNum ;
         begin = FALSE ;
         if ( writePos >= size - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[%d] is too small", size ) ;
            goto error ;
         }
      }

      if ( withType )
      {
         if ( !begin )
         {
            buffer[ writePos ] = ',' ;
            ++writePos ;
         }
         writeNum = ossSnprintf( buffer + writePos, size - writePos,
                                 "\"_type\":\"%s\"", _type ) ;
         writePos += writeNum ;
         begin = FALSE ;
         if ( writePos >= size - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[%d] is too small", size ) ;
            goto error ;
         }
      }

      if ( withID )
      {
         if ( !begin )
         {
            buffer[ writePos ] = ',' ;
            ++writePos ;
         }
         writeNum = ossSnprintf( buffer + writePos, size - writePos,
                                 "\"_id\":\"%s\"", _id ) ;
         writePos += writeNum ;
         if ( writePos >= size - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[%d] is too small", size ) ;
            goto error ;
         }
      }

      writeNum = ossSnprintf( buffer + writePos, size - writePos, "}}\n" ) ;
      length = writePos + writeNum ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESActionCreate::_utilESActionCreate( const CHAR *index,
                                             const CHAR *type )
   : _utilESBulkActionBase( index, type )
   {
   }

   _utilESActionCreate::~_utilESActionCreate()
   {
   }

   utilESBulkActionType _utilESActionCreate::getActionType() const
   {
      return UTIL_ES_ACTION_CREATE ;
   }

   const CHAR* _utilESActionCreate::_getActionName() const
   {
      return "create" ;
   }

   BOOLEAN _utilESActionCreate::_hasSourceData() const
   {
      return TRUE ;
   }

   INT32 _utilESActionCreate::_outputSrcData( CHAR *buffer, INT32 size,
                                              INT32 &length ) const
   {
      INT32 rc = SDB_OK ;
      string dataStr = _dataObj.toString( false, true ) ;

      // One byte for the extra '\n' at the end of the line.
      if ( (UINT32)size < dataStr.length() + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }

      ossStrncpy( buffer, dataStr.c_str(), dataStr.length() ) ;
      buffer[ dataStr.length() ] = '\n' ;
      length = dataStr.length() + 1 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESActionIndex::_utilESActionIndex( const CHAR *index, const CHAR *type )
   : _utilESBulkActionBase( index, type )
   {
   }

   _utilESActionIndex::~_utilESActionIndex()
   {
   }

   utilESBulkActionType _utilESActionIndex::getActionType() const
   {
      return UTIL_ES_ACTION_INDEX ;
   }

   const CHAR* _utilESActionIndex::_getActionName() const
   {
      return "index" ;
   }

   BOOLEAN _utilESActionIndex::_hasSourceData() const
   {
      return TRUE ;
   }

   INT32 _utilESActionIndex::_outputSrcData( CHAR *buffer, INT32 size,
                                             INT32 &length ) const
   {
      INT32 rc = SDB_OK ;
      string dataStr = _dataObj.toString( false, true ) ;

      // One byte for the extra '\n' at the end of the line.
      if ( (UINT32)size < dataStr.length() + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }

      ossStrncpy( buffer, dataStr.c_str(), dataStr.length() ) ;
      buffer[ dataStr.length() ] = '\n' ;
      length = dataStr.length() + 1 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESActionUpdate::_utilESActionUpdate( const CHAR *index, const CHAR *type )
   : _utilESBulkActionBase( index, type )
   {
   }

   _utilESActionUpdate::~_utilESActionUpdate()
   {
   }

   utilESBulkActionType _utilESActionUpdate::getActionType() const
   {
      return UTIL_ES_ACTION_UPDATE ;
   }

   const CHAR* _utilESActionUpdate::_getActionName() const
   {
      return "update" ;
   }

   BOOLEAN _utilESActionUpdate::_hasSourceData() const
   {
      return TRUE ;
   }

   INT32 _utilESActionUpdate::_outputSrcData( CHAR *buffer, INT32 size,
                                              INT32 &length ) const
   {
      INT32 rc = SDB_OK ;
      UINT32 writePos = 0 ;
      const CHAR *upsertStr = ",\"doc_as_upsert\":true" ;
      UINT32 upsertLen = ossStrlen( upsertStr ) ;
      string dataStr = _dataObj.toString( false, true ) ;

      // The source data of update is in the following format:
      //    {"doc":{field1:val1, field2:val2,...,fieldn:valn}}\n

      // One byte for the extra '\n' at the end of the line.
      if ( size < (INT32)( dataStr.length() + BULK_UPDATE_PREFIX_LEN +
                           upsertLen + BULK_UPDATE_SUFFIX_LEN + 1 ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }

      ossStrncpy( buffer, BULK_UPDATE_PREFIX, BULK_UPDATE_PREFIX_LEN ) ;
      writePos = BULK_UPDATE_PREFIX_LEN ;
      ossStrncpy( buffer + writePos, dataStr.c_str(), dataStr.length() ) ;
      writePos += dataStr.length() ;
      ossStrncpy( buffer + writePos, upsertStr, upsertLen ) ;
      writePos += upsertLen ;
      ossStrncpy( buffer + writePos, BULK_UPDATE_SUFFIX,
                  BULK_UPDATE_SUFFIX_LEN ) ;
      writePos += BULK_UPDATE_SUFFIX_LEN ;
      buffer[ writePos ] = '\n' ;
      length = writePos + 1 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESActionDelete::_utilESActionDelete( const CHAR *index, const CHAR *type )
   : _utilESBulkActionBase( index, type )
   {
   }

   _utilESActionDelete::~_utilESActionDelete()
   {
   }

   utilESBulkActionType _utilESActionDelete::getActionType() const
   {
      return UTIL_ES_ACTION_DELETE ;
   }

   const CHAR* _utilESActionDelete::_getActionName() const
   {
      return "delete" ;
   }

   BOOLEAN _utilESActionDelete::_hasSourceData() const
   {
      return FALSE ;
   }

   INT32 _utilESActionDelete::_outputSrcData( CHAR *buffer, INT32 size,
                                              INT32 &length ) const
   {
      INT32 rc = SDB_OK ;
      string dataStr = _dataObj.toString( false, true ) ;

      // One byte for the extra '\n' at the end of the line.
      if ( (UINT32)size < dataStr.length() + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }

      ossStrncpy( buffer, dataStr.c_str(), dataStr.length() ) ;
      buffer[ dataStr.length() ] = '\n' ;
      length = dataStr.length() + 1 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESBulkBuilder::_utilESBulkBuilder()
   {
      _buffer = NULL ;
      _capacity = 0 ;
      _dataLen = 0 ;
   }

   _utilESBulkBuilder::~_utilESBulkBuilder()
   {
      if ( _buffer )
      {
         SDB_OSS_FREE( _buffer ) ;
      }
   }

   INT32 _utilESBulkBuilder::init( UINT32 bufferSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 capacity = 0 ;

      if ( 0 == bufferSize || bufferSize > UTIL_ESBULK_MAX_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %u ] is invalid", bufferSize ) ;
         goto error ;
      }

      capacity = bufferSize * 1024 * 1024 ;
      if ( _buffer )
      {
         SDB_ASSERT( _capacity, "_capacity is 0" ) ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Bulk builder has been initialized already" ) ;
         goto error ;
      }

      _buffer = (CHAR *)SDB_OSS_MALLOC( capacity ) ;
      if ( !_buffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for bulk buffer failed, requested "
                 "size[ %u ]", capacity ) ;
         goto error ;
      }

      ossMemset( _buffer, 0, capacity ) ;
      _capacity = capacity ;
      _itemNum = 0 ;

   done:
      return rc ;
   error:
      SAFE_OSS_FREE( _buffer ) ;
      goto done ;
   }

   void _utilESBulkBuilder::reset()
   {
      ossMemset( _buffer, 0, _capacity ) ;
      _dataLen = 0 ;
      _itemNum = 0 ;
   }

   UINT32 _utilESBulkBuilder::getFreeSize() const
   {
      return ( _capacity - _dataLen ) ;
   }

   INT32 _utilESBulkBuilder::appendItem( const utilESBulkActionBase &item,
                                         BOOLEAN withIndex, BOOLEAN withType,
                                         BOOLEAN withID )
   {
      INT32 rc = SDB_OK ;
      INT32 itemLen = 0 ;

      if ( item.outSizeEstimate() > getFreeSize() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer free space[ %u ] is not enough",
            getFreeSize()  ) ;
         goto error ;
      }

      rc = item.output( _buffer + _dataLen, getFreeSize(), itemLen, withIndex,
                        withType, withID ) ;
      PD_RC_CHECK( rc, PDERROR, "Append bulk item failed[ %d ]", rc ) ;
      _dataLen += itemLen ;
      _itemNum++ ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

