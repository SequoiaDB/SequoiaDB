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

   Source File Name = dmsIndexExtBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexExtBuilder.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsScanner.hpp"
#include "ixmKey.hpp"
#include "ixm.hpp"
#include "dmsCB.hpp"
#include "pmdEDU.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   _dmsIndexExtBuilder::_dmsIndexExtBuilder( _dmsStorageUnit* su,
                                             dmsMBContext* mbContext,
                                             pmdEDUCB* eduCB,
                                             dmsExtentID indexExtentID,
                                             dmsExtentID indexLogicID,
                                             dmsIndexBuildGuardPtr &guardPtr,
                                             dmsDupKeyProcessor *dkProcessor )
   : _dmsIndexBuilder( su, mbContext, eduCB, indexExtentID, indexLogicID,
                       guardPtr, dkProcessor ),
     _extHandler( NULL )
   {
      ossMemset( _collectionName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      ossMemset( _extDataName, 0, DMS_MAX_EXT_NAME_SIZE + 1 ) ;
   }

   _dmsIndexExtBuilder::~_dmsIndexExtBuilder()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXEXTBUILDER__ONINIT, "_dmsIndexExtBuilder::_onInit" )
   INT32 _dmsIndexExtBuilder::_onInit()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINDEXEXTBUILDER__ONINIT ) ;

      SDB_ASSERT( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT,
                                       _indexCB->getIndexType() ),
                  "Not text index" ) ;

      _extHandler = _suData->getExtDataHandler() ;
      PD_CHECK( _extHandler, SDB_SYS, error, PDERROR,
                "External data handle is NULL" ) ;

      ossStrncpy( _collectionName, _mbContext->mb()->_collectionName,
                  DMS_COLLECTION_NAME_SZ + 1 ) ;
      _idxName.clear() ;
      _idxName.append( _indexCB->getName() ) ;
      ossStrncpy( _extDataName, _indexCB->getExtDataName(),
                  DMS_MAX_EXT_NAME_SIZE + 1 ) ;
      _keyDef = _indexCB->keyPattern() ;

      // We are going to create the capped cs and cl. During the whole process,
      // no use of the index is allowed.
      if ( IXM_INDEX_FLAG_INVALID != _indexCB->getFlag() )
      {
         _indexCB->setFlag( IXM_INDEX_FLAG_INVALID ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINDEXEXTBUILDER__ONINIT, rc ) ;
      return rc ;
   error:
      _extHandler = NULL ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXEXTBUILDER__BUILD, "_dmsIndexExtBuilder::_build" )
   INT32 _dmsIndexExtBuilder::_build()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINDEXEXTBUILDER__BUILD ) ;
      BOOLEAN mbLocked = FALSE ;
      BOOLEAN hasRebuild = FALSE ;
      INT32 idxID = 0 ;

      if ( !_extHandler )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "External handler is NULL" ) ;
         goto error ;
      }

      rc = _extHandler->onRebuildTextIdx( _suIndex->getSuName(),
                                          _collectionName, _idxName.str(),
                                          _extDataName, _keyDef,
                                          _eduCB, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "External handle on text index rebuild "
                   "failed: %d", rc ) ;
      rc = _extHandler->done( DMS_EXTOPR_TYPE_REBUILDIDX, _eduCB, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "External done on text index rebuild failed: "
                   "%d", rc ) ;
      // Now the capped cs and cl have been created. So if any failure below,
      // they need to be dropped.
      hasRebuild = TRUE ;

      // Now we need to set the index as valid.
      // As we do not take any lock before this place, the cs/cl/index may have
      // been dropped already. So after taking the lock, we need to check again
      // if this is the original index.
      rc = _mbLockAndCheck( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      mbLocked = TRUE ;
      for ( idxID = 0; idxID < DMS_COLLECTION_MAX_INDEX; ++idxID )
      {
         if ( DMS_INVALID_EXTENT == _mbContext->mb()->_indexExtent[idxID] )
         {
            break ;
         }
         ixmIndexCB indexCBTmp( _mbContext->mb()->_indexExtent[idxID], _suIndex,
                                _mbContext ) ;
         PD_CHECK( indexCBTmp.isInitialized(), SDB_DMS_INIT_INDEX,
                   error, PDERROR, "Failed to initialize index" ) ;
         // Check if this is the original index by comparing the logical id.
         if ( _indexLID == indexCBTmp.getLogicalID() )
         {
            break ;
         }
      }

      if ( DMS_COLLECTION_MAX_INDEX == idxID )
      {
         // Didn't find this index. It may have been dropped. Need to undo what
         // has been done here, and nothing more should be done with the
         // indexCB.
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

      // Now we have done the external operation and everything is going
      // smoothly. Set the index as CREATING and it will be set as NORMAL
      // in _finish() of dmsIndexRebuilder.
      _indexCB->setFlag( IXM_INDEX_FLAG_CREATING ) ;

   done:
      if ( mbLocked )
      {
         _mbContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB__DMSINDEXEXTBUILDER__BUILD, rc ) ;
      return rc ;
   error:
      if ( hasRebuild )
      {
         // If the cs/cl/index has been dropped, or the cl has been truncated,
         // the external operation would have been done. Otherwise, do the
         // external on drop operation.
         if ( ( SDB_DMS_NOTEXIST != rc ) && ( SDB_DMS_TRUNCATED != rc ) &&
              ( SDB_DMS_INVALID_INDEXCB != rc ) )
         {
            INT32 rcTmp = _extHandler->onDropTextIdx( _extDataName, _eduCB, NULL ) ;
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "External operation on drop text index failed, "
                       "rc: %d", rcTmp ) ;
            }
         }
      }
      goto done ;
   }

}

