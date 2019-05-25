/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
   {
      SDB_ASSERT( index, "Index can't be NULL" ) ;
      SDB_ASSERT( type, "Type can't be NULL" ) ;

      _sourceData = NULL ;
      _srcDataLen = 0 ;
      _ownData = FALSE ;

      if ( index )
      {
         _index = std::string( index ) ;
      }
      if ( type )
      {
         _type = std::string( type ) ;
      }
   }

   _utilESBulkActionBase::~_utilESBulkActionBase()
   {
      if ( _sourceData && _ownData )
      {
         SDB_OSS_FREE( _sourceData ) ;
      }
   }

   INT32 _utilESBulkActionBase::setID( const CHAR *id )
   {
      INT32 rc = SDB_OK ;
      if ( !id )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "_id is NULL" ) ;
         goto error ;
      }

      _id = std::string( id ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESBulkActionBase::setSourceData( const CHAR *sourceData,
                                               INT32 length, BOOLEAN copy )
   {
      INT32 rc = SDB_OK ;

      if ( !sourceData || length <=0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid source data or length" ) ;
         goto error ;
      }

      if ( _sourceData )
      {
         if ( _ownData )
         {
            SDB_OSS_FREE( _sourceData ) ;
            _ownData = FALSE ;
         }
         _sourceData = NULL ;
      }

      if ( copy )
      {
         _sourceData = (CHAR *)SDB_OSS_MALLOC( length ) ;
         if ( !_sourceData )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory for source data failed, requested"
                    " size[ %d ]", length ) ;
            goto error ;
         }
         ossStrncpy( _sourceData, sourceData, length ) ;
         _ownData = TRUE ;
      }
      else
      {
         _sourceData = (CHAR *)sourceData ;
      }

      _srcDataLen = length ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESBulkActionBase::outSizeEstimate() const
   {
      return ( UTIL_ESBULK_MIN_META_SIZE + _index.length() +
               _type.length() + _id.length() + _srcDataLen ) ;
   }

   INT32 _utilESBulkActionBase::output( CHAR *buffer, INT32 size,
                                        INT32 &length, BOOLEAN withIndex,
                                        BOOLEAN withType, BOOLEAN withID ) const
   {
      INT32 rc = SDB_OK ;
      INT32 metaLen = 0 ;
      INT32 dataLen = 0 ;

      if ( size < outSizeEstimate() )
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

   INT32 _utilESBulkActionBase::_outputActionAndMeta( CHAR *buffer,
                                                      INT32 size,
                                                      INT32 &length,
                                                      BOOLEAN withIndex,
                                                      BOOLEAN withType,
                                                      BOOLEAN withID ) const
   {
      INT32 rc = SDB_OK ;
      BOOLEAN begin = TRUE ;

      std::string metaData = std::string( "{\"" ) + _getActionName() + "\":{" ;
      if ( withIndex )
      {
         if ( _index.empty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Index name is empty" ) ;
            goto error ;
         }
         metaData += "\"_index\":\"" + _index + "\"" ;
         begin = FALSE ;
      }
      if ( withType )
      {
         if ( _type.empty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Type name is empty" ) ;
            goto error ;
         }
         if ( !begin )
         {
            metaData += "," ;
         }
         metaData += "\"_type\":\"" + _type + "\"" ;
         begin = FALSE ;
      }
      if ( withID )
      {
         if ( _id.empty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "_id is empty" ) ;
            goto error ;
         }
         if ( !begin )
         {
            metaData += "," ;
         }
         metaData += "\"_id\":\"" + _id + "\"" ;
      }

      metaData += "}}\n" ;
      if ( (UINT32)size < metaData.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }

      SDB_ASSERT( buffer, "Buffer is NULL" ) ;
      ossStrncpy( buffer, metaData.c_str(), metaData.length() ) ;
      length = metaData.length() ;

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

   utilESBulkActionType _utilESActionCreate::getType() const
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

      if ( size < _srcDataLen + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }
      ossStrncpy( buffer, _sourceData, _srcDataLen ) ;
      buffer[ _srcDataLen ] = '\n' ;
      length = _srcDataLen + 1 ;

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

   utilESBulkActionType _utilESActionIndex::getType() const
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

      if ( size < _srcDataLen + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }
      ossStrncpy( buffer, _sourceData, _srcDataLen ) ;
      buffer[ _srcDataLen ] = '\n' ;
      length = _srcDataLen + 1 ;

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

   utilESBulkActionType _utilESActionUpdate::getType() const
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


      if ( size < (INT32)( _srcDataLen + BULK_UPDATE_PREFIX_LEN + upsertLen +
                           BULK_UPDATE_SUFFIX_LEN + 1 ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }

      ossStrncpy( buffer, BULK_UPDATE_PREFIX, BULK_UPDATE_PREFIX_LEN ) ;
      writePos = BULK_UPDATE_PREFIX_LEN ;
      ossStrncpy( buffer + writePos, _sourceData, _srcDataLen ) ;
      writePos += _srcDataLen ;
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

   utilESBulkActionType _utilESActionDelete::getType() const
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

      if ( size < _srcDataLen + 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %d ] is too small", size ) ;
         goto error ;
      }
      ossStrncpy( buffer, _sourceData, _srcDataLen ) ;
      buffer[ _srcDataLen ] = '\n' ;
      length = _srcDataLen + 1 ;

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

      if ( 0 == bufferSize || bufferSize > UTIL_ESBULK_MAX_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buffer size[ %u ] is invalid", bufferSize ) ;
         goto error ;
      }

      if ( _buffer )
      {
         SDB_ASSERT( _capacity, "_capacity is 0" ) ;
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Bulk builder has been initialized already" ) ;
         goto error ;
      }

      _buffer = (CHAR *)SDB_OSS_MALLOC( bufferSize ) ;
      if ( !_buffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for bulk buffer failed, requested "
                 "size[ %d ]", bufferSize ) ;
         goto error ;
      }

      ossMemset( _buffer, 0, bufferSize ) ;
      _capacity = bufferSize ;

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
   }

   INT32 _utilESBulkBuilder::getFreeSize() const
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

   done:
      return rc ;
   error:
      goto done ;
   }
}

