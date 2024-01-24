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
#include "pdSecure.hpp"

using namespace bson ;

namespace engine
{
#define RTN_UPSERT_MAX_RETRY_TIME 1

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATE1, "rtnUpdate" )
   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, utilUpdateResult *pResult,
                     const BSONObj *shardingKey, UINT32 logWriteMod,
                     IRtnOprHandler *opHandler )
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
                       dmsCB, dpsCB, 1, pResult, shardingKey, logWriteMod,
                       opHandler ) ;

      PD_TRACE_EXITRC ( SDB_RTNUPDATE1, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATE2, "rtnUpdate" )
   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w, utilUpdateResult *pResult,
                     const BSONObj *shardingKey, UINT32 logWriteMod,
                     IRtnOprHandler *opHandler )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNUPDATE2 ) ;
      BSONObj dummy ;
      // matcher, selector, order, hint, collection, skip, limit, flag
      rtnQueryOptions options( matcher, dummy, dummy, hint, pCollectionName,
                               0, -1, flags ) ;
      rc = rtnUpdate( options, updator, cb, dmsCB, dpsCB, w, pResult,
                      shardingKey, logWriteMod, opHandler ) ;
      PD_TRACE_EXITRC( SDB_RTNUPDATE2, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATE_OPTIONS, "rtnUpdate" )
   INT32 rtnUpdate ( rtnQueryOptions &options, const BSONObj &updator,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w, utilUpdateResult *pResult,
                     const BSONObj *shardingKey, UINT32 logWriteMod,
                     IRtnOprHandler *opHandler )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNUPDATE_OPTIONS ) ;

      SDB_ASSERT ( options.getCLFullName(), "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;

      pmdKRCB *krcb                    = pmdGetKRCB() ;
      SDB_RTNCB *rtnCB                 = krcb->getRTNCB() ;
      UINT64 numUpdatedRecords         = 0 ;
      dmsStorageUnit *su               = NULL ;
      dmsMBContext   *mbContext        = NULL ;
      dmsStorageUnitID suID            = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      optAccessPlanManager *apm        = NULL ;
      BOOLEAN updateOne                = options.testFlag( FLG_UPDATE_ONE ) ;
      BOOLEAN writable                 = FALSE ;
      BOOLEAN strictDataMode           = FALSE ;
      dmsScanner *pScanner             = NULL ;
      UINT32 scannerRetryTime          = 0 ;
      BSONObj emptyObj ;
      mthModifier modifier ;
      vector<INT64> dollarList ;

      optAccessPlanRuntime planRuntime ;
      monContextCB monCtxCB ;
      rtnReturnOptions returnOptions ;
      UINT32 upsertRetyTime = 0 ;

      // updator is modifier
      if ( updator.isEmpty() )
      {
         PD_LOG ( PDERROR, "modifier can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // writeable judge
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

      // get mb context
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
                                     strictDataMode,
                                     logWriteMod,
                                     TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for updator: "
                      "%s", PD_SECURE_OBJ( updator ) ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Invalid pattern is detected for update: %s: %s",
                  PD_SECURE_OBJ( updator ), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         UINT64 startDataRead = 0,
                startIndexRead = 0,
                startDataWrite = 0,
                startIndexWrite = 0 ;
         apm = rtnCB->getAPM() ;
         SDB_ASSERT ( apm, "apm shouldn't be NULL" ) ;

retry:
         // plan is released when exiting the function
         rc = apm->getAccessPlan( options, su, mbContext, planRuntime, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for %s for update, "
                      "rc: %d", options.getCLFullName(), rc ) ;

         if ( planRuntime.getScanType() == TBSCAN )
         {
            rc = rtnGetTBScanner( pCollectionShortName, &planRuntime, su,
                                  mbContext, cb, &pScanner,
                                  DMS_ACCESS_TYPE_UPDATE, opHandler ) ;
         }
         else if ( planRuntime.getScanType() == IXSCAN )
         {
            rc = rtnGetIXScanner( pCollectionShortName, &planRuntime, su,
                                  mbContext, cb, &pScanner,
                                  DMS_ACCESS_TYPE_UPDATE, opHandler ) ;
            if ( SDB_IXM_NOTEXIST == rc && scannerRetryTime < 1 )
            {
               // Maybe in the process of scanning the index,
               // the index is deleted
               planRuntime.reset() ;
               scannerRetryTime++ ;
               // We only need to try to scan once. In most cases,
               // the next scan is normal
               goto retry ;
            }
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
            startDataRead = cb->getMonAppCB()->totalDataRead ;
            startIndexRead = cb->getMonAppCB()->totalIndexRead ;
            startDataWrite = cb->getMonAppCB()->totalDataWrite ;
            startIndexWrite = cb->getMonAppCB()->totalIndexWrite ;
         }

         // update
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
               // pScanner->advance() may pause mbContext and scanner too.
               // so we should clear mbContext's subContext before advance()
               // to avoid pause scanner twice
               _dmsMBContextSubScope subScope( mbContext,
                                               pScanner->getScannerContext() ) ;
               if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                                  DMS_MB_ATTR_NOIDINDEX ) )
               {
                  PD_LOG( PDERROR, "can not update data when autoIndexId is "
                          "false" ) ;
                  rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
                  goto error ;
               }

               execStartTime = krcb->getCurTime() ;

               mthContext.getDollarList( &dollarList ) ;
               generator.getDataPtr( recordDataPtr ) ;

               rc = su->data()->updateRecord( mbContext, recordID,
                                              recordDataPtr, cb, dpsCB,
                                              modifier, NULL,
                                              pScanner->callbackHandler(),
                                              pResult ) ;
               PD_RC_CHECK( rc, PDERROR, "Update record failed, rc: %d", rc ) ;

               ++numUpdatedRecords ;
               mthContext.clear() ;
               mthContext.enableDollarList() ;

               execEndTime = krcb->getCurTime() ;
               monCtxCB.monExecuteTimeInc( execStartTime, execEndTime ) ;

               if ( updateOne && 1 == numUpdatedRecords )
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
         }

         // if we didn't update anything, let's attempt to insert if
         // we are doing upsert
         if ( ( 0 == numUpdatedRecords ) &&
              options.testFlag( FLG_UPDATE_UPSERT ) )
         {
            BSONObj source = planRuntime.getEqualityQueryObject() ;
            PD_LOG ( PDDEBUG, "equality query object: %s",
                     source.toString().c_str() ) ;

            BSONObj target ;
            ossTick execStartTime, execEndTime ;

            execStartTime = krcb->getCurTime() ;

            // upsertor means generate a new record from empty source
            rc = modifier.modify ( source, target ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to generate upsertor record, rc: %d",
                        rc ) ;
               goto error ;
            }
            PD_LOG ( PDDEBUG, "modified equality query object: %s",
                     PD_SECURE_OBJ( target ) ) ;

            BSONElement setOnInsert =
                       options.getHint().getField( FIELD_NAME_SET_ON_INSERT ) ;
            if ( !setOnInsert.eoo() )
            {
               rc = rtnUpsertSet( setOnInsert, target ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to set when upsert, rc: %d", rc ) ;
            }

            rc = su->data()->insertRecord( mbContext, target, cb, dpsCB,
                                           TRUE, TRUE, -1, pResult ) ;
            if ( SDB_IXM_DUP_KEY == rc && upsertRetyTime < RTN_UPSERT_MAX_RETRY_TIME )
            {
               BSONObj dupIndexKey = dotted2nested( pResult->getIdxKeyPattern() ) ;
               mthMatchTree *matcherTree = planRuntime.getMatchTree() ;
               if ( !source.isEmpty() && !dupIndexKey.isEmpty() && NULL != matcherTree )
               {
                  BOOLEAN needRetry = FALSE ;
                  if ( source.hasAllFieldNames( dupIndexKey ) )
                  {
                     rc = matcherTree->matches( target, needRetry ) ;
                     if ( SDB_OK != rc )
                     {
                        PD_LOG( PDERROR, "Check if matcher matches the target record failed, rc:%d", rc ) ;
                        goto error ;
                     }
                  }

                  if ( needRetry )
                  {
                     pResult->reset() ;
                     ++upsertRetyTime ;
                     planRuntime.reset() ;
                     if ( NULL != pScanner )
                     {
                        SDB_OSS_DEL pScanner ;
                        pScanner = NULL ;
                     }
                     goto retry ;
                  }
                  rc = SDB_IXM_DUP_KEY ;
               }
               goto error ;
            }
            else if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to insert record %s\ninto "
                        "collection: %s", PD_SECURE_OBJ( target ),
                        pCollectionShortName ) ;
               goto error ;
            }

            execEndTime = krcb->getCurTime() ;
            monCtxCB.monExecuteTimeInc( execStartTime, execEndTime ) ;
         }

         if ( NULL != cb )
         {
            monCtxCB.monDataReadInc( cb->getMonAppCB()->totalDataRead -
                                     startDataRead ) ;
            monCtxCB.monIndexReadInc( cb->getMonAppCB()->totalIndexRead -
                                      startIndexRead ) ;
            monCtxCB.monDataWriteInc( cb->getMonAppCB()->totalDataWrite -
                                      startDataWrite ) ;
            monCtxCB.monIndexWriteInc( cb->getMonAppCB()->totalIndexWrite -
                                       startIndexWrite ) ;
         }

         planRuntime.setQueryActivity( MON_UPDATE, monCtxCB, returnOptions,
                                       TRUE ) ;

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
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected: { %s }, "
                      "rc: %d", PD_SECURE_STR( setOnInsert.toString() ), rc ) ;
         rc = setModifier.modify( target, newTarget ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to generate upsertor "
                      "record(rc=%d) by " FIELD_NAME_SET_ON_INSERT, rc ) ;

         target = newTarget ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "failed to generate upsertor on { %s }, %s",
                  PD_SECURE_STR( setOnInsert.toString() ), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   BSONObj rtnUpdator2Obj( const BSONObj &source, const BSONObj &updator )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      mthModifier modifier ;

      try
      {
         rc = modifier.loadPattern( updator ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Load pattern[%s] failed, rc: %d",
                    PD_SECURE_OBJ( updator ), rc ) ;
            goto done ;
         }
         rc = modifier.modify( source, obj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Make modify[%s] failed, rc: %d",
                    PD_SECURE_OBJ( updator ), rc ) ;
            goto done ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto done ;
      }

   done:
      return obj ;
   }

}

