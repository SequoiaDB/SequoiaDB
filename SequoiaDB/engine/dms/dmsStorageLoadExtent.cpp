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

   Source File Name = dmsStorageLoadExtent.cpp

   Descriptive Name =

   When/how to use: load extent to database

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#include "dms.hpp"
#include "dmsExtent.hpp"
#include "dmsSMEMgr.hpp"
#include "dmsStorageLoadExtent.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "migLoad.hpp"
#include "dmsTransLockCallback.hpp"

using namespace bson ;

namespace engine
{
   void dmsStorageLoadOp::_initExtentHeader ( dmsExtent *extAddr,
                                              UINT16 numPages )
   {
      SDB_ASSERT ( _pageSize * numPages == _currentExtentSize,
                   "extent size doesn't match" ) ;
      extAddr->_eyeCatcher[0]          = DMS_EXTENT_EYECATCHER0 ;
      extAddr->_eyeCatcher[1]          = DMS_EXTENT_EYECATCHER1 ;
      extAddr->_blockSize              = numPages ;
      extAddr->_mbID                   = 0 ;
      extAddr->_flag                   = DMS_EXTENT_FLAG_INUSE ;
      extAddr->_version                = DMS_EXTENT_CURRENT_V ;
      extAddr->_logicID                = DMS_INVALID_EXTENT ;
      extAddr->_prevExtent             = DMS_INVALID_EXTENT ;
      extAddr->_nextExtent             = DMS_INVALID_EXTENT ;
      extAddr->_recCount               = 0 ;
      extAddr->_firstRecordOffset      = DMS_INVALID_EXTENT ;
      extAddr->_lastRecordOffset       = DMS_INVALID_EXTENT ;
      extAddr->_freeSpace              = _pageSize * numPages -
                                         sizeof(dmsExtent) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT, "dmsStorageLoadOp::_allocateExtent" )
   INT32 dmsStorageLoadOp::_allocateExtent ( INT32 requestSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT );
      if ( requestSize < DMS_MIN_EXTENT_SZ(_pageSize) )
      {
         requestSize = DMS_MIN_EXTENT_SZ(_pageSize) ;
      }
      else if ( requestSize > (INT32)DMS_MAX_EXTENT_SZ(_su->data()) )
      {
         requestSize = DMS_MAX_EXTENT_SZ(_su->data()) ;
      }
      else
      {
         requestSize = ossRoundUpToMultipleX ( requestSize, _pageSize ) ;
      }

      if ( (UINT32)requestSize > _buffSize && _pCurrentExtent )
      {
         SDB_OSS_FREE( _pCurrentExtent ) ;
         _pCurrentExtent = NULL ;
         _buffSize = 0 ;
      }

      if ( (UINT32)requestSize > _buffSize )
      {
         _pCurrentExtent = ( CHAR* )SDB_OSS_MALLOC( requestSize << 1 ) ;
         if ( !_pCurrentExtent )
         {
            PD_LOG( PDERROR, "Alloc memroy[%d] failed",
                    requestSize << 1 ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _buffSize = ( requestSize << 1 ) ;
      }

      _currentExtentSize = requestSize ;
      _initExtentHeader ( (dmsExtent*)_pCurrentExtent,
                          _currentExtentSize/_pageSize ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK, "dmsStorageLoadOp::pushToTempDataBlock" )
   INT32 dmsStorageLoadOp::pushToTempDataBlock ( dmsMBContext *mbContext,
                                                 pmdEDUCB *cb,
                                                 BSONObj &record,
                                                 BOOLEAN isLast,
                                                 BOOLEAN isAsynchr )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK );

      SDB_ASSERT( mbContext, "mb context can't be NULL" ) ;

      rc = _su->data()->prepareCollectionLoads( mbContext, record, isLast, isAsynchr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare collection loads, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__LDDATA, "dmsStorageLoadOp::loadBuildPhase" )
   INT32 dmsStorageLoadOp::loadBuildPhase ( dmsMBContext *mbContext,
                                            pmdEDUCB *cb,
                                            BOOLEAN isAsynchr,
                                            migMaster *pMaster,
                                            UINT32 *success,
                                            UINT32 *failure )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__LDDATA ) ;

      rc = _su->data()->buildCollectionLoads( mbContext, isAsynchr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build collection loads, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__LDDATA, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, "dmsStorageLoadOp::loadRollbackPhase" )
   INT32 dmsStorageLoadOp::loadRollbackPhase( dmsMBContext *mbContext, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT ) ;

      SDB_ASSERT ( _su, "_su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "mb context can't be NULL" ) ;

      PD_LOG ( PDEVENT, "Start loadRollbackPhase" ) ;

      rc = _su->data()->truncateCollectionLoads( mbContext, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection loads, rc: %d", rc ) ;

   done:
      PD_LOG ( PDEVENT, "End loadRollbackPhase" ) ;
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, rc );
      return rc ;
   error:
      goto done ;
   }

}

