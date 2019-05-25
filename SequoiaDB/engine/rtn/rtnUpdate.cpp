/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnUpdate.cpp

   Descriptive Name = Runtime Update

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for update
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
#include "mthModifier.hpp"
#include "rtnIXScanner.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dmsScanner.hpp"

using namespace bson ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATE1, "rtnUpdate" )
   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, INT64 *pUpdateNum, INT32 *pInsertNum,
                     const BSONObj *shardingKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNUPDATE1 ) ;
      pmdKRCB *krcb = pmdGetKRCB () ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB () ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB () ;

      if ( dpsCB && cb->isFromLocal() && !dpsCB->isLogLocal() )
      {
         dpsCB = NULL ;
      }

      rc = rtnUpdate ( pCollectionName, matcher, updator, hint, flags, cb,
                       dmsCB, dpsCB, 1, pUpdateNum, pInsertNum, shardingKey ) ;

      PD_TRACE_EXITRC ( SDB_RTNUPDATE1, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATE2, "rtnUpdate" )
   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w, INT64 *pUpdateNum, INT32 *pInsertNum,
                     const BSONObj *shardingKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNUPDATE2 ) ;
      BSONObj dummy ;
      rtnQueryOptions options( matcher, dummy, dummy, hint, pCollectionName,
                               0, -1, flags ) ;
      rc = rtnUpdate( options, updator, cb, dmsCB, dpsCB, w, pUpdateNum,
                      pInsertNum, shardingKey ) ;
      PD_TRACE_EXITRC( SDB_RTNUPDATE2, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATE_OPTIONS, "rtnUpdate" )
   INT32 rtnUpdate ( rtnQueryOptions &options, const BSONObj &updator,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w, INT64 *pUpdateNum, INT32 *pInsertNum,
                     const BSONObj *shardingKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNUPDATE_OPTIONS ) ;

      SDB_ASSERT ( options.getCLFullName(), "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;

      pmdKRCB *krcb                    = pmdGetKRCB() ;
      SDB_RTNCB *rtnCB                 = krcb->getRTNCB() ;
      SINT64 numUpdatedRecords         = 0 ;
      INT32  insertNum                 = 0 ;
      dmsStorageUnit *su               = NULL ;
      dmsMBContext   *mbContext        = NULL ;
      dmsStorageUnitID suID            = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      optAccessPlanManager *apm        = NULL ;
      BOOLEAN writable                 = FALSE ;
      BOOLEAN strictDataMode           = FALSE ;
      dmsScanner *pScanner             = NULL ;
      BSONObj emptyObj ;
      mthModifier modifier ;
      vector<INT64> dollarList ;

      optAccessPlanRuntime planRuntime ;
      monContextCB monCtxCB ;
      rtnReturnOptions returnOptions ;

      if ( updator.isEmpty() )
      {
         PD_LOG ( PDERROR, "modifier can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = dmsCB->writable( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Database is not writable, rc = %d", rc ) ;
         goto error;
      }
      writable = TRUE;

      rc = rtnResolveCollectionNameAndLock ( options.getCLFullName(), dmsCB,
                                             &su, &pCollectionShortName,
                                             suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   options.getCLFullName(), rc ) ;

      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection[%s] mb context, "
                   "rc: %d", options.getCLFullName(), rc ) ;

      if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                         DMS_MB_ATTR_NOIDINDEX ) )
      {
         PD_LOG( PDERROR, "can not update data when autoIndexId is false" ) ;
         rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
         goto error ;
      }

      if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                         DMS_MB_ATTR_STRICTDATAMODE ) )
      {
         strictDataMode = TRUE ;
      }

      try
      {
         rc = modifier.loadPattern ( updator,
                                     &dollarList,
                                     TRUE,
                                     shardingKey,
                                     strictDataMode ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for updator: "
                      "%s", updator.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Invalid pattern is detected for update: %s: %s",
                  updator.toString().c_str(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      apm = rtnCB->getAPM() ;
      SDB_ASSERT ( apm, "apm shouldn't be NULL" ) ;

      rc = apm->getAccessPlan( options, FALSE, su, mbContext, planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for %s for update, "
                   "rc: %d", options.getCLFullName(), rc ) ;

      if ( planRuntime.getScanType() == TBSCAN )
      {
         rc = rtnGetTBScanner( pCollectionShortName, &planRuntime, su,
                               mbContext, cb, &pScanner,
                               DMS_ACCESS_TYPE_UPDATE ) ;
      }
      else if ( planRuntime.getScanType() == IXSCAN )
      {
         rc = rtnGetIXScanner( pCollectionShortName, &planRuntime, su, mbContext,
                               cb, &pScanner, DMS_ACCESS_TYPE_UPDATE ) ;
      }
      else
      {
         PD_LOG ( PDERROR, "Invalid return type for scan" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms scanner, rc: %d", rc ) ;

      {
         _mthRecordGenerator generator ;
         _mthMatchTreeContext mthContext ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr = 0 ;

         ossTick startTime, endTime, execStartTime, execEndTime ;
         ossTickDelta queryTime ;

         if ( cb->getMonConfigCB()->timestampON )
         {
            monCtxCB.recordStartTimestamp() ;
         }

         startTime = krcb->getCurTime() ;

         mthContext.enableDollarList() ;

         while ( SDB_OK == ( rc = pScanner->advance( recordID, generator,
                                                     cb, &mthContext ) ) )
         {
            execStartTime = krcb->getCurTime() ;

            mthContext.getDollarList( &dollarList ) ;
            generator.getDataPtr( recordDataPtr ) ;
            rc = su->data()->updateRecord( mbContext, recordID, recordDataPtr,
                                           cb, dpsCB, modifier ) ;
            PD_RC_CHECK( rc, PDERROR, "Update record failed, rc: %d", rc ) ;

            ++numUpdatedRecords ;
            mthContext.clear() ;
            mthContext.enableDollarList() ;

            execEndTime = krcb->getCurTime() ;
            monCtxCB.monExecuteTimeInc( execStartTime, execEndTime ) ;
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
      }

      if ( ( 0 == numUpdatedRecords ) && options.testFlag( FLG_UPDATE_UPSERT ) )
      {
         BSONObj source = planRuntime.getEqualityQueryObject() ;
         PD_LOG ( PDDEBUG, "equality query object: %s",
                  source.toString().c_str() ) ;

         BSONObj target ;
         ossTick execStartTime, execEndTime ;

         execStartTime = krcb->getCurTime() ;

         rc = modifier.modify ( source, target ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to generate upsertor record, rc: %d",
                     rc ) ;
            goto error ;
         }
         PD_LOG ( PDDEBUG, "modified equality query object: %s",
                  target.toString().c_str() ) ;

         BSONElement setOnInsert =
                        options.getHint().getField( FIELD_NAME_SET_ON_INSERT ) ;
         if ( !setOnInsert.eoo() )
         {
            rc = rtnUpsertSet( setOnInsert, target ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to set when upsert, rc: %d", rc ) ;
         }

         rc = su->data()->insertRecord( mbContext, target, cb, dpsCB,
                                        TRUE, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert record %s\ninto collection: %s",
                     target.toString().c_str(), pCollectionShortName ) ;
            goto error ;
         }
         ++insertNum ;

         execEndTime = krcb->getCurTime() ;
         monCtxCB.monExecuteTimeInc( execStartTime, execEndTime ) ;
      }

      planRuntime.setQueryActivity( MON_UPDATE, monCtxCB, returnOptions,
                                    TRUE ) ;

   done :
      if ( pUpdateNum )
      {
         *pUpdateNum = numUpdatedRecords ;
      }
      if ( pInsertNum )
      {
         *pInsertNum = insertNum ;
      }
      if ( pScanner )
      {
         SDB_OSS_DEL pScanner ;
      }
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      planRuntime.releasePlan() ;
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
      PD_TRACE_EXITRC ( SDB_RTNUPDATE_OPTIONS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 rtnUpsertSet( const BSONElement& setOnInsert, BSONObj& target )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj newTarget ;
         BSONObj setObj ;
         BSONObjBuilder builder ;
         builder.appendAs( setOnInsert, "$set" ) ;
         setObj = builder.obj() ;

         mthModifier setModifier ;
         rc = setModifier.loadPattern( setObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected: { %s }, rc: %d",
                      setOnInsert.toString().c_str(), rc ) ;
         rc = setModifier.modify( target, newTarget ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to generate upsertor "
                      "record(rc=%d) by " FIELD_NAME_SET_ON_INSERT, rc ) ;

         target = newTarget ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "failed to generate upsertor on { %s }, %s",
                  setOnInsert.toString().c_str(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

}

