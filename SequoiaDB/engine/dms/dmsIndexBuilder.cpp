/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dmsIndexBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexBuilder.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsIndexBuilderImpl.hpp"
#include "ixm.hpp"

namespace engine
{
   _dmsIndexBuilder::_dmsIndexBuilder( _dmsStorageIndex* indexSU,
                                       _dmsStorageData* dataSU,
                                       _dmsMBContext* mbContext,
                                       _pmdEDUCB* eduCB,
                                       dmsExtentID indexExtentID )
   : _suIndex ( indexSU ),
     _suData ( dataSU ),
     _mbContext ( mbContext ),
     _eduCB ( eduCB ),
     _indexExtentID ( indexExtentID )
   {
      _indexCB = NULL ;
      _scanExtLID = DMS_INVALID_EXTENT ;
      _currentExtentID = DMS_INVALID_EXTENT ;
      _extent = NULL ;
      _unique = FALSE ;
      _dropDups = FALSE ;
   }

   _dmsIndexBuilder::~_dmsIndexBuilder()
   {
      _suIndex = NULL ;
      _suData = NULL ;
      _mbContext = NULL ;
      if ( NULL != _indexCB )
      {
         SDB_OSS_DEL( _indexCB ) ;
         _indexCB = NULL ;
      }
   }

   INT32 _dmsIndexBuilder::_init()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _suIndex != NULL, "_suIndex can't be NULL" ) ;
      SDB_ASSERT( _suData != NULL, "_suData can't be NULL" ) ;
      SDB_ASSERT( _mbContext != NULL, "_mbContext can't be NULL" ) ;

      rc = _mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      PD_CHECK ( DMS_INVALID_EXTENT != _indexExtentID,
                 SDB_INVALIDARG, error, PDERROR,
                 "Index Extent ID %d is invalid for collection %d",
                 _indexExtentID, (INT32)_mbContext->mbID() ) ;

      _indexCB = SDB_OSS_NEW ixmIndexCB ( _indexExtentID,
                                          _suIndex, _mbContext ) ;
      if ( NULL == _indexCB )
      {
         PD_LOG ( PDERROR, "failed to allocate _indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      PD_CHECK ( _indexCB->isInitialized(), SDB_DMS_INIT_INDEX,
                 error, PDERROR, "Failed to initialize index" ) ;

      _indexLID = _indexCB->getLogicalID() ;

      rc = _indexCB->getIndexID ( _indexOID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get indexID, rc = %d", rc ) ;

      if ( IXM_INDEX_FLAG_CREATING == _indexCB->getFlag() )
      {
         _scanExtLID = _indexCB->scanExtLID() ;
      }
      else if ( IXM_INDEX_FLAG_NORMAL != _indexCB->getFlag() &&
                IXM_INDEX_FLAG_INVALID != _indexCB->getFlag() )
      {
         PD_LOG ( PDERROR, "Index is either creating or dropping: %d",
                  (INT32)_indexCB->getFlag() ) ;
         _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto error ;
      }
      else
      {
         rc = _indexCB->truncate ( FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index, rc: %d", rc ) ;
            _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            goto error ;
         }
         _indexCB->setFlag ( IXM_INDEX_FLAG_CREATING ) ;
      }

      _unique = _indexCB->unique() ;
      _dropDups = _indexCB->dropDups() ;

      rc = _onInit() ;
      if ( rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDERROR, "Post init operation failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      if ( NULL != _indexCB )
      {
         SDB_OSS_DEL( _indexCB ) ;
         _indexCB = NULL ;
      }
      goto done ;
   }

   INT32 _dmsIndexBuilder::_onInit()
   {
      INT32 rc = SDB_OK ;

      _currentExtentID = _mbContext->mb()->_firstExtentID ;
      if ( DMS_INVALID_EXTENT == _currentExtentID )
      {
         _indexCB->setFlag ( IXM_INDEX_FLAG_NORMAL ) ;
         _indexCB->scanExtLID ( DMS_INVALID_EXTENT ) ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_finish()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_OK == ( rc = _checkIndexAfterLock( SHARED ) ) )
      {
         _indexCB->setFlag ( IXM_INDEX_FLAG_NORMAL ) ;
         _indexCB->scanExtLID ( DMS_INVALID_EXTENT ) ;
         _mbContext->mbUnlock() ;
      }

      return rc ;
   }

   INT32 _dmsIndexBuilder::build()
   {
      INT32 rc = SDB_OK ;

      rc = _init() ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         goto init_error ;
      }

      rc = _build() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _finish() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   init_error:
      goto done ;
   error:
      if ( SDB_DMS_INVALID_INDEXCB != rc )
      {
         if ( SDB_OK == _checkIndexAfterLock( SHARED ) )
         {
            _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            _mbContext->mbUnlock() ;
         }
      }
      goto done ;
   }

   INT32 _dmsIndexBuilder::_beforeExtent()
   {
      INT32 rc = SDB_OK ;

      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

      _extRW = _suData->extent2RW( _currentExtentID, _mbContext->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      _extent = _extRW.readPtr<dmsExtent>() ;
      if ( NULL == _extent )
      {
         PD_LOG ( PDERROR, "Invalid extent: %d", _currentExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT != _scanExtLID &&
           _extent->_logicID < _scanExtLID )
      {
         _currentExtentID = _extent->_nextExtent ;
         rc = _DMS_SKIP_EXTENT ;
      }

   done:
      return rc ;
   error:
      _extent = NULL ;
      goto done ;
   }

   INT32 _dmsIndexBuilder::_afterExtent()
   {
      _indexCB->scanExtLID ( _extent->_logicID ) ;
      return SDB_OK ;
   }

   INT32 _dmsIndexBuilder::_getKeySet( ossValuePtr recordDataPtr, BSONObjSet& keySet )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj obj ( (CHAR*)recordDataPtr ) ;

         rc = _indexCB->getKeysFromObject ( obj, keySet ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to get keys from object %s",
                       obj.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_insertKey( const ixmKey &key, const dmsRecordID &rid, const Ordering& ordering )
   {
      INT32 rc = SDB_OK ;

      rc = _suIndex->_indexInsert( _indexCB, key, rid, ordering, _eduCB, !_unique, _dropDups ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_IDENTICAL_KEY == rc )
         {
            rc = SDB_OK ;
            PD_LOG ( PDWARNING, "Identical key is detected "
                     "during index rebuild, ignore" ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to insert into index, rc: %d", rc ) ;
         }
      }

      return rc ;
   }

   INT32 _dmsIndexBuilder::_insertKey( ossValuePtr recordDataPtr, const dmsRecordID &rid, const Ordering& ordering )
   {
      BSONObjSet keySet ;
      BSONObjSet::iterator it ;
      INT32 rc = SDB_OK ;

      rc = _getKeySet( recordDataPtr, keySet ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for ( it = keySet.begin() ; it != keySet.end() ; it++ )
      {
         ixmKeyOwned ko( *it ) ;
         rc = _insertKey( ko, rid, ordering ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_checkIndexAfterLock( INT32 lockType )
   {
      INT32 rc = SDB_OK ;

      rc = _mbContext->mbLock( lockType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _mbContext->mbUnlock() ;
      goto done ;
   }

   _dmsIndexBuilder* _dmsIndexBuilder::createInstance( _dmsStorageIndex* indexSU,
                                                       _dmsStorageData* dataSU,
                                                       _dmsMBContext* mbContext,
                                                       _pmdEDUCB* eduCB,
                                                       dmsExtentID indexExtentID,
                                                       INT32 sortBufferSize,
                                                       UINT16 indexType )
   {
      _dmsIndexBuilder* builder = NULL ;

      SDB_ASSERT( indexSU != NULL, "indexSU can't be NULL" ) ;
      SDB_ASSERT( dataSU != NULL, "dataSU can't be NULL" ) ;
      SDB_ASSERT( mbContext != NULL, "mbContext can't be NULL" ) ;
      SDB_ASSERT( eduCB != NULL, "eduCB can't be NULL" ) ;

      if ( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT, indexType ) )
      {
         builder = SDB_OSS_NEW _dmsIndexExtBuilder( indexSU,
                                                    dataSU,
                                                    mbContext,
                                                    eduCB,
                                                    indexExtentID ) ;
         if ( NULL == builder)
         {
            PD_LOG ( PDERROR, "failed to allocate _dmsIndexExtBuilder" ) ;
         }
      }
      else
      {
         PD_LOG ( PDDEBUG, "index sort buffer size: %dMB", sortBufferSize ) ;

         if ( sortBufferSize < 0 )
         {
            PD_LOG ( PDERROR, "invalid sort buffer size: %d", sortBufferSize ) ;
         }
         else if ( 0 == sortBufferSize )
         {
            builder = SDB_OSS_NEW _dmsIndexOnlineBuilder( indexSU,
                                                          dataSU,
                                                          mbContext,
                                                          eduCB,
                                                          indexExtentID ) ;
            if ( NULL == builder)
            {
               PD_LOG ( PDERROR, "failed to allocate _dmsIndexOnlineBuilder" ) ;
            }
         }
         else
         {
            builder = SDB_OSS_NEW _dmsIndexSortingBuilder( indexSU,
                                                           dataSU,
                                                           mbContext,
                                                           eduCB,
                                                           indexExtentID,
                                                           sortBufferSize ) ;
            if ( NULL == builder)
            {
               PD_LOG ( PDERROR, "failed to allocate _dmsIndexSortingBuilder" ) ;
            }
         }
      }

      return builder ;
   }

   void _dmsIndexBuilder::releaseInstance( _dmsIndexBuilder* builder )
   {
      SDB_ASSERT( builder != NULL, "builder can't be NULL" ) ;

      SDB_OSS_DEL builder ;
   }
}

