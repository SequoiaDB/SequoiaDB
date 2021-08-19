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

   Source File Name = rtnDelete.cpp

   Descriptive Name = Runtime Delete

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for delete
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "ossTypes.hpp"
#include "optAccessPlanRuntime.hpp"
#include "rtnIXScanner.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dmsScanner.hpp"

using namespace bson ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDEL1, "rtnDelete" )
   INT32 rtnDelete ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &hint, INT32 flags, pmdEDUCB *cb,
                     utilDeleteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDEL1 ) ;
      pmdKRCB *krcb = pmdGetKRCB () ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB () ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB () ;

      if ( dpsCB && cb->isFromLocal() && !dpsCB->isLogLocal() )
      {
         dpsCB = NULL ;
      }

      rc = rtnDelete ( pCollectionName, matcher, hint, flags, cb,
                       dmsCB, dpsCB, 1, pResult ) ;

      PD_TRACE_EXITRC ( SDB_RTNDEL1, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDEL2, "rtnDelete" )
   INT32 rtnDelete ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &hint, INT32 flags, pmdEDUCB *cb,
                     SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB, INT16 w,
                     utilDeleteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNDEL2 ) ;
      BSONObj dummy ;
      // matcher, selector, order, hint, collection, skip, limit, flag
      rtnQueryOptions options( matcher, dummy, dummy, hint, pCollectionName,
                               0, -1, flags ) ;
      rc = rtnDelete( options, cb, dmsCB, dpsCB, w, pResult ) ;
      PD_TRACE_EXITRC( SDB_RTNDEL2, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDEL_OPTIONS, "rtnDelete" )
   INT32 rtnDelete ( rtnQueryOptions &options, pmdEDUCB *cb,
                     SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB, INT16 w,
                     utilDeleteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDEL_OPTIONS ) ;

      SDB_ASSERT ( options.getCLFullName(), "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;

      pmdKRCB *krcb                       = pmdGetKRCB() ;
      SDB_RTNCB *rtnCB                    = krcb->getRTNCB() ;
      dmsStorageUnit *su                  = NULL ;
      dmsMBContext   *mbContext           = NULL ;
      dmsStorageUnitID suID               = DMS_INVALID_CS ;
      optAccessPlanManager *apm           = NULL ;
      const CHAR *pCollectionShortName    = NULL ;
      dmsScanner *pScanner                = NULL ;
      BOOLEAN writable                    = FALSE ;
      BOOLEAN deleteOne                   = options.testFlag( FLG_DELETE_ONE ) ;
      UINT64 numDeletedRecords            = 0 ;

      optAccessPlanRuntime planRuntime ;

      rc = dmsCB->writable( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Database is not writable, rc = %d", rc ) ;
         goto error;
      }
      writable = TRUE;

      rc = rtnResolveCollectionNameAndLock ( options._fullName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   options._fullName, rc ) ;

      // get mb context
      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection[%s] mb context, "
                   "rc: %d", options._fullName, rc ) ;

      // Capped collection has no index, but delete is supported.
      if ( OSS_BIT_TEST( mbContext->mb()->_attributes, DMS_MB_ATTR_NOIDINDEX )
           &&
           !(OSS_BIT_TEST( mbContext->mb()->_attributes, DMS_MB_ATTR_CAPPED ) ) )
      {
         PD_LOG( PDERROR, "can not delete data when autoIndexId is false" ) ;
         rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
         goto error ;
      }

      try
      {
         apm = rtnCB->getAPM() ;
         SDB_ASSERT ( apm, "apm shouldn't be NULL" ) ;

         // plan is released when exiting the function
         rc = apm->getAccessPlan( options, FALSE, su, mbContext, planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for %s for delete"
                      ", rc: %d", options._fullName, rc ) ;

         if ( planRuntime.getScanType() == TBSCAN )
         {
            rc = rtnGetTBScanner( pCollectionShortName, &planRuntime, su,
                                  mbContext, cb, &pScanner,
                                  DMS_ACCESS_TYPE_DELETE ) ;
         }
         else if ( planRuntime.getScanType() == IXSCAN )
         {
            rc = rtnGetIXScanner( pCollectionShortName, &planRuntime, su,
                                  mbContext, cb, &pScanner,
                                  DMS_ACCESS_TYPE_DELETE ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Invalid return type for scan" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get dms scanner, rc: %d", rc ) ;

         if ( NULL != cb )
         {
            cb->registerMonCRUDCB( &( mbContext->mbStat()->_crudCB ) ) ;
         }

         // delete
         {
            _mthRecordGenerator generator ;
            dmsRecordID recordID ;
            ossValuePtr recordDataPtr = 0 ;

            ossTick startTime, endTime, execStartTime, execEndTime ;
            ossTickDelta queryTime ;
            monContextCB monCtxCB ;
            rtnReturnOptions returnOptions ;

            if ( cb->getMonConfigCB()->timestampON )
            {
               monCtxCB.recordStartTimestamp() ;
            }

            startTime = krcb->getCurTime() ;

            while ( SDB_OK == ( rc = pScanner->advance( recordID, generator,
                                                        cb ) ) )
            {
               // Capped collection has no index, but delete is supported.
               if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                                  DMS_MB_ATTR_NOIDINDEX )
                    &&
                    !(OSS_BIT_TEST( mbContext->mb()->_attributes,
                                    DMS_MB_ATTR_CAPPED ) ) )
               {
                  PD_LOG( PDERROR, "can not delete data when autoIndexId is "
                          "false" ) ;
                  rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
                  goto error ;
               }

               // by now, we have a qualified record to delete, and already hold
               // the record lock in X
               execStartTime = krcb->getCurTime() ;

               generator.getDataPtr( recordDataPtr ) ;
               rc = su->data()->deleteRecord( mbContext, recordID, recordDataPtr,
                                              cb, dpsCB,
                                              pScanner->callbackHandler(),
                                              pScanner->recordInfo() ) ;
               PD_RC_CHECK( rc, PDERROR, "Delete record failed, rc: %d", rc ) ;

               numDeletedRecords++ ;
               if ( pResult )
               {
                  pResult->incDeletedNum() ;
               }

               execEndTime = krcb->getCurTime() ;
               monCtxCB.monExecuteTimeInc( execStartTime, execEndTime ) ;

               if ( deleteOne && 1 == numDeletedRecords )
               {
                  break ;
               }
            }

            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get next record, rc: %d", rc ) ;
               goto error ;
            }

            endTime = krcb->getCurTime() ;
            queryTime = endTime - startTime ;
            queryTime -= monCtxCB.getExecuteTime() ;
            monCtxCB.setQueryTime( queryTime ) ;

            planRuntime.setQueryActivity( MON_DELETE, monCtxCB, returnOptions,
                                          TRUE ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      if ( pScanner )
      {
         SDB_OSS_DEL pScanner ;
      }
      if ( NULL != cb )
      {
         cb->unregisterMonCRUDCB() ;
      }
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      planRuntime.reset() ;
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB_RTNDEL_OPTIONS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRAVERDEL, "rtnTraversalDelete" )
   INT32 rtnTraversalDelete ( const CHAR *pCollectionName,
                              const BSONObj &key,
                              const CHAR *pIndexName,
                              INT32 dir,
                              pmdEDUCB *cb,
                              SDB_DMSCB *dmsCB,
                              SDB_DPSCB *dpsCB,
                              INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTRAVERDEL ) ;

      SDB_ASSERT ( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;

      dmsStorageUnit *su               = NULL ;
      dmsMBContext   *mbContext        = NULL ;
      dmsStorageUnitID suID            = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      BOOLEAN writable                 = FALSE;
      dmsScanner *pScanner             = NULL ;

      BSONObj hint ;
      BSONObj dummy ;

      // make sure the database is not doing any offline operations
      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK ( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE;

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                    pCollectionName, rc ) ;

      // get mb context
      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection[%s] mb context, "
                   "rc: %d", pCollectionName, rc ) ;

      try
      {
         // build hint
         hint = BSON( "" << pIndexName ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Failed to construct hint object: %s",
                       e.what() ) ;
      }

      {
         optAccessPlanRuntime planRuntime ;
         // matcher, selector, order, hint, collection, skip, limit, flag
         rtnQueryOptions options( dummy, dummy, dummy, hint, pCollectionName,
                                  0, -1, 0 ) ;
         rc = sdbGetRTNCB()->getAPM()->getTempAccessPlan( options, su,
                                                          mbContext,
                                                          planRuntime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan, rc: %d", rc ) ;

         // Must apply the hint to find index-scan plan
         PD_CHECK ( planRuntime.getScanType() == IXSCAN &&
                    !planRuntime.isAutoGen(),
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to generate access plan by index %s", pIndexName ) ;

         SDB_ASSERT( NULL != planRuntime.getPredList(),
                     "predList can't be NULL" ) ;

         // set traversal direction
         planRuntime.getPredList()->setDirection ( dir ) ;

         // we do NOT need to create callback for this code path now because
         // of the complexity of split
         rc = rtnGetIXScanner( pCollectionShortName, &planRuntime, su, mbContext,
                               cb, &pScanner, DMS_ACCESS_TYPE_DELETE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get dms ixscanner, rc: %d", rc ) ;

         if ( NULL != cb )
         {
            cb->registerMonCRUDCB( &( mbContext->mbStat()->_crudCB ) ) ;
         }

         // relocate key
         {
            rtnIXScanner *scanner = ((dmsIXScanner*)pScanner)->getScanner() ;
            dmsRecordID rid ;

            if ( !scanner )
            {
               rc = SDB_OOM ;
               PD_RC_CHECK ( rc, PDERROR,
                             "Unable to allocate memory for index scanner:" ) ;
            }

            if ( -1 == dir )
            {
               rid.resetMax () ;
            }
            else
            {
               rid.resetMin () ;
            }
            rc = scanner->relocateRID ( key, rid ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to relocate key to the specified "
                          "location: %s, rc: %d", key.toString().c_str(), rc ) ;
         }

         // delete
         {
            _mthRecordGenerator generator ;
            dmsRecordID recordID ;
            ossValuePtr recordDataPtr = 0 ;

            while ( SDB_OK == ( rc = pScanner->advance( recordID, generator,
                                                        cb ) ) )
            {
               generator.getDataPtr( recordDataPtr ) ;
               rc = su->data()->deleteRecord( mbContext, recordID, recordDataPtr,
                                              cb, dpsCB,
                                              pScanner->callbackHandler(),
                                              pScanner->recordInfo() ) ;
               PD_RC_CHECK( rc, PDERROR, "Delete record failed, rc: %d", rc ) ;
            }

            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get next record, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done :
      if ( pScanner )
      {
         SDB_OSS_DEL pScanner ;
      }
      if ( NULL != cb )
      {
         cb->unregisterMonCRUDCB() ;
      }
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB_RTNTRAVERDEL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

}

