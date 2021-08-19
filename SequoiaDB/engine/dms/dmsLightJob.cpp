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

   Source File Name = dmsLightJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/12/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsLightJob.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "pmd.hpp"

namespace engine
{

   _dmsDeleteRecordJob::_dmsDeleteRecordJob( INT32 csID, UINT16 clID,
                                             UINT32 csLID, UINT32 clLID,
                                             const dmsRecordID &rid )
   {
      _csID = csID ;
      _clID = clID ;
      _csLID = csLID ;
      _clLID = clLID ;
      _rid = rid ;
   }

   _dmsDeleteRecordJob::~_dmsDeleteRecordJob()
   {
   }

   const CHAR* _dmsDeleteRecordJob::name() const
   {
      return "Delete Record" ;
   }

   INT32 _dmsDeleteRecordJob::doit( IExecutor *pExe,
                                    UTIL_LJOB_DO_RESULT &result,
                                    UINT64 &sleepTime )
   {
      static SDB_DMSCB  *pDmsCB = sdbGetDMSCB() ;
      static dpsTransCB *pTransCB = sdbGetTransCB() ;

      INT32 rcTmp = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *pContext = NULL ;
      dmsRecordRW recordRW ;
      const dmsRecord *pRecord = NULL ;
      sleepTime = 60000000 ;   /// 60 second

      su = pDmsCB->suLock( _csID ) ;
      if ( !su || su->LogicalCSID() != _csLID )
      {
         /// cs not exist
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      if ( SDB_OK != su->data()->getMBContext( &pContext, _clID,
                                               _clLID, _clLID ) )
      {
         /// cl not exist
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      rcTmp = pContext->mbTryLock( EXCLUSIVE ) ;
      if ( SDB_TIMEOUT == rcTmp )
      {
         /// wait next time
         result = UTIL_LJOB_DO_CONT ;
         goto done ;
      }
      else if ( rcTmp )
      {
         /// cl not exist
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// Test Lock
      rcTmp = pTransCB->transLockTestX( (pmdEDUCB*)pExe, _csLID,
                                         _clID, &_rid ) ;
      if ( rcTmp )
      {
         /// lock failed, wait next time
         result = UTIL_LJOB_DO_CONT ;
         goto done ;
      }

      recordRW = su->data()->record2RW( _rid, _clID ) ;
      recordRW.setNothrow( TRUE ) ;
      pRecord = recordRW.readPtr<dmsRecord>() ;
      if ( !pRecord || pRecord->getMyOffset() != _rid._offset )
      {
         /// record not exist
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      if ( !pRecord->isDeleting() )
      {
         /// record not deleting
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      /// delete record
      su->data()->deleteRecord( pContext, _rid, (ossValuePtr)pRecord,
                                (pmdEDUCB*)pExe, NULL, NULL, NULL ) ;
      result = UTIL_LJOB_DO_FINISH ;

   done:
      if ( pContext )
      {
         su->data()->releaseMBContext( pContext ) ;
      }
      if ( su )
      {
         pDmsCB->suUnlock( su->CSID(), SHARED ) ;
      }
      return SDB_OK ;
   }

   void dmsStartAsyncDeleteRecord( INT32 csID, UINT16 clID,
                                   UINT32 csLID, UINT32 clLID,
                                   const dmsRecordID &rid )
   {
      if ( pmdGetOptionCB()->recycleRecord() )
      {
         INT32 rc = SDB_OK ;
         dmsDeleteRecordJob *pJob = NULL ;

         pJob = SDB_OSS_NEW dmsDeleteRecordJob( csID, clID, csLID,
                                                clLID, rid ) ;
         if ( !pJob )
         {
            PD_LOG( PDWARNING, "Alloc dmsDeleteRecordJob(CSID:%u, CLID:%u, "
                    "CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) failed",
                    csID, clID, csLID, clLID, rid._extent, rid._extent ) ;
         }
         else
         {
            rc = pJob->submit( TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Submit dmsDeleteRecordJob(CSID:%u, CLID:%u, "
                       "CSLID:%u, CLLID:%u, ExtentID:%u, Offset:%u) failed, "
                       "rc: %d", csID, clID, csLID, clLID, rid._extent,
                       rid._extent, rc ) ;
            }
         }
      }
   }

}


