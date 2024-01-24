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

   Source File Name = rtnCommandImpl.cpp

   Descriptive Name = Runtime Commands Implementation

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Commands component,
   which is handling user admin commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <set>
#include "dmsStatUnit.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dmsStorageUnit.hpp"
#include "mthSelector.hpp"
#include "monDump.hpp"
#include "msgDef.h"
#include "msgMessage.hpp"
#include "rtnInternalSorting.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnExtDataHandler.hpp"
#include "rtnContextDel.hpp"
#include "ossMemPool.hpp"
#include "rtnTSClt.hpp"

using namespace bson ;

namespace engine
{
   /***********************************************
    * Totally 5 types of commands
    * 1) create
    *    create command takes 1 parameter, which indicates the object
    *    information
    *    it only returns return code
    * 2) drop
    *    drop command takes 1 parameter, which indicates the object information
    *    it only returns return code
    * 3) list
    *    list command takes 3 parameters, which indicates the selector and
    *    matcher and orderBy it supposed to query from the result set
    *    it returns return code + result set
    * 4) snapshot
    *    snapshot command takes 3 parameters, which indicates the selector and
    *    matcher and orderBy it supposed to query from the result set
    *    it returns return code + result set
    * 5) get
    *    get command takes 4 parameters, which indicates the selector, matcher,
    *    orderBy and object information
    *    it returns return code + result set
    * 6) rename
    *    rename command takes 1 parameter ( matcher )
    *    in "$rename collection" command, matcher has 3 elements:
    *    collectionspace + oldname + newname
    ***********************************************/

   static BOOLEAN _isSimpleTextSearch( BSONObj condition )
   {
      BOOLEAN result = FALSE ;

      if ( !condition.isEmpty() )
      {
         BSONObjIterator itr( condition ) ;
         BSONElement firstEle = itr.next() ;
         if ( itr.more() )
         {
            goto done ;
         }

         if ( 0 != ossStrlen( firstEle.fieldName() ) ||
              Object != firstEle.type() )
         {
            goto done ;
         }

         {
            BSONObjIterator subItr( firstEle.Obj() ) ;
            BSONElement subFirstEle = subItr.next() ;
            if ( itr.more() )
            {
               goto done ;
            }

            if ( 0 != ossStrcmp( subFirstEle.fieldName(), FIELD_NAME_TEXT ) )
            {
               goto done ;
            }
            result = TRUE ;
         }
      }

   done:
      return result ;
   }

   // get total number of records for a given query.
   // there are two scenarios
   // 1) users provided query condition
   // 2) users didn't specify any condition
   // for condition (1), we convert it into a normal query and count the total
   // number of records we read. In this case it will go through the regular
   // codepath for rtnQuery + rtnGetMore by using tbscan or ixscan
   // for condition (2), we directly call DMS countCollection function, this
   // will bypass fetching records by records. Instead it will read each extent
   // and get the _recCount in extent header for quick count
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETCOUNT, "rtnGetCount" )
   INT32 rtnGetCount ( const rtnQueryOptions & options,
                       SDB_DMSCB *dmsCB,
                       _pmdEDUCB *cb,
                       SDB_RTNCB *rtnCB,
                       INT64 *count )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETCOUNT ) ;
      SINT64 totalCount = 0 ;
      const CHAR * pCollection = options.getCLFullName() ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      SDB_ASSERT ( count, "count can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      SINT64 queryContextID = -1 ;
      BOOLEAN hasRange = options.hasRangeInHint() ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }
      if ( !options.isQueryEmpty() ||
           hasRange )
      {
         rtnContextPtr pContextBase ;

         if ( _isSimpleTextSearch( options.getQuery() ) )
         {
            rtnRemoteMessenger* messenger = rtnCB->getRemoteMessenger() ;
            if ( !messenger || !messenger->isReady() )
            {
               rc = SDB_NET_NOT_CONNECT ;
               PD_LOG( PDERROR, "Remote messenger is not ready. Maybe the "
                       "adapter is offline" ) ;
               goto error ;
            }

            rc = rtnTSGetCount( options, cb, totalCount ) ;
            PD_RC_CHECK( rc, PDERROR, "Get count by text search failed[%d]",
                         rc ) ;
         }
         else
         {
            BSONObj dummy ;
            rtnQueryOptions copiedOptions( options ) ;
            copiedOptions.setSelector( dummy ) ;
            copiedOptions.setOrderBy( dummy ) ;
            copiedOptions.setSkip( 0 ) ;
            copiedOptions.setLimit( -1 ) ;
            // if count query has range in index field, set flag so that
            // rtn context know the count query can try to use index cover
            if ( hasRange )
            {
               copiedOptions.setFlag( FLG_FORCE_INDEX_SELECTOR ) ;
            }
            copiedOptions.setInternalFlag( RTN_INTERNAL_QUERY_COUNT_FLAG ) ;

            rc = rtnQuery ( copiedOptions, cb, dmsCB, rtnCB, queryContextID,
                            &pContextBase ) ;
            if ( rc )
            {
               // any error will clean up queryContext
               if ( SDB_DMS_EOC == rc )
               {
                  // if we hit end of collection, let's clear the rc
                  // in this case, totalCount = 0
                  queryContextID = -1 ;
                  rc = SDB_OK ;
               }
               else
               {
                  PD_LOG ( PDERROR,"Failed to query for count for collection %s, "
                           "rc: %d", pCollection, rc ) ;
                  goto error ;
               }
            }
            else
            {
               if ( !hasRange )
               {
                  pContextBase->enableCountMode() ;
               }
               rtnContextBuf buffObj ;

               if ( NULL != pContextBase->getPlanRuntime() &&
                    pContextBase->getPlanRuntime()->isAllRangeScan() &&
                    !hasRange )
               {
                  // use quick extent header count
                  rc = su->countCollection ( pCollectionShortName, totalCount,
                                             cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to get count %s, rc: %d",
                               pCollection, rc ) ;
               }
               else
               {
                  while ( TRUE )
                  {
                     rc = rtnGetMore ( queryContextID, -1, buffObj, cb, rtnCB ) ;
                     if ( rc )
                     {
                        // any error will clean up query context
                        if ( SDB_DMS_EOC == rc )
                        {
                           queryContextID = -1 ;
                           rc = SDB_OK ;
                           break ;
                        }
                        else
                        {
                           PD_LOG ( PDERROR, "Failed to fetch for count for "
                                    "collecion %s, rc: %d", pCollection, rc ) ;
                           goto error ;
                        }
                     }
                     else
                     {
                        // since rtnGetMore only takes 32 bit count, so let's pass
                        // count and add into totalCount every round
                        totalCount += buffObj.recordNum() ;
                     }
                  }
               }
            }
         }
      }
      else
      {
         // use quick extent header count
         rc = su->countCollection ( pCollectionShortName, totalCount, cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get count %s, rc: %d",
                     pCollection, rc ) ;
            goto error ;
         }
      }

      *count = totalCount ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( -1 != queryContextID )
      {
         rtnCB->contextDelete( queryContextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNGETCOUNT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 rtnGetCount ( const rtnQueryOptions & options,
                       SDB_DMSCB *dmsCB,
                       _pmdEDUCB *cb,
                       SDB_RTNCB *rtnCB,
                       rtnContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETCOUNT ) ;
      const CHAR * pCollection = options.getCLFullName() ;
      SINT64 totalCount = 0 ;
      BSONObj obj ;
      BSONObjBuilder ob ;

      rc = rtnGetCount( options, dmsCB, cb, rtnCB, &totalCount ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get count for collection %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      ob.append ( FIELD_NAME_TOTAL, totalCount ) ;
      obj = ob.obj () ;
      rc = context->append ( obj ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to append context for collection %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNGETCOUNT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   static INT32 _rtnGetDatablocks( dmsStorageUnit *su,
                                   pmdEDUCB * cb,
                                   rtnContextDump *context,
                                   dmsMBContext *mbContext,
                                   const CHAR *pCLShortName )
   {
      INT32 rc = SDB_OK ;
      ossPoolVector< dmsExtentID > extentList ;

      rc = su->getSegExtents( pCLShortName, extentList, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] segment extents failed, "
                   "rc: %d", pCLShortName, rc ) ;

      rc = monDumpDatablocks( extentList, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Dump datablocks failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 rtnGetDatablocks( const CHAR *collectionName,
                                  SDB_DMSCB *dmsCB,
                                  pmdEDUCB * cb,
                                  rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      const CHAR *pCLShortName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      rc = rtnResolveCollectionNameAndLock( collectionName, dmsCB, &su,
                                            &pCLShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name[%s], rc: %d",
                   collectionName, rc ) ;

      rc = _rtnGetDatablocks( su, cb, context, NULL, pCLShortName ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] data blocks failed, rc: %d",
                   collectionName, rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETCOLLECTIONDETAIL, "rtnGetCollectionDetail" )
   static INT32 rtnGetCollectionDetail( const CHAR *pCollection,
                                        SDB_DMSCB *dmsCB,
                                        rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETCOLLECTIONDETAIL ) ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      monCollection info ;
      UINT32 infoMask = MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
      BSONObjBuilder builder ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to resolve collection name %s, rc: %d",
                   pCollection, rc ) ;

      rc = su->dumpCLInfo( pCollectionShortName, info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump collection[%s] info, rc: %d",
                   pCollectionShortName, rc ) ;

      dmsCB->suUnlock( suID ) ;
      suID = DMS_INVALID_SUID ;

      rc = monCollection2Obj( info, infoMask, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON object, rc: %d", rc ) ;

      rc = context->monAppend( builder.obj() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to add object to context, rc: %d", rc ) ;
   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNGETCOLLECTIONDETAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETCOLLECTIONSTAT, "rtnGetCollectionStat" )
   static INT32 rtnGetCollectionStat( const CHAR *pCLFullName,
                                      _pmdEDUCB *cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_RTNCB *rtnCB,
                                      rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETCOLLECTIONSTAT ) ;
      SDB_ASSERT( pCLFullName, "collection can't be NULL" ) ;
      SDB_ASSERT( rtnCB, "rtn control block can't be NULL" ) ;
      SDB_ASSERT( dmsCB, "dms control block can't be NULL" ) ;

      dmsStorageUnit *pSU = NULL ;
      dmsStatCache *statCache = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsMBContext *mbContext = NULL ;
      const CHAR *pCLName = NULL ;
      BOOLEAN isDefault = FALSE ;
      BOOLEAN isExpired = FALSE ;
      dmsCollectionStat *pCollectionStat = NULL ;
      dmsCollectionStat dummyStat ;
      pmdOptionsCB *optCB  = pmdGetOptionCB() ;
      BSONObj infoObj ;

      rc = rtnResolveCollectionNameAndLock( pCLFullName, dmsCB, &pSU, &pCLName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

      rc = pSU->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      statCache = pSU->getStatCache() ;

      // Check if statistics need to be loaded
      if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == statCache->getStatus( mbContext->mbID() ) )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK ( rc, PDERROR, "Lock collection failed, rc:%d ", rc) ;

         rc = rtnReloadCLStats( pSU, mbContext, cb, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Reload CL statistics failed, rc: %d", rc ) ;

         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK ( rc, PDERROR, "Lock collection failed, rc:%d ", rc) ;

      }

      pCollectionStat = ( dmsCollectionStat* )statCache->getCacheUnit( mbContext->mbID() ) ;
      if ( NULL == pCollectionStat )
      {
         PD_LOG( PDDEBUG, "Failed to find collectionStat[%s] info", pCLFullName ) ;
         isDefault = TRUE ;
         pCollectionStat = &dummyStat ;
      }
      else if ( optCheckStatExpiredByPage( mbContext->mbStat()->_totalDataPages,
                                     pCollectionStat->getTotalDataPages(),
                                     optCB->getOptCostThreshold(),
                                     pSU->getPageSizeLog2() ) )
      {
         // if the statistics is expired, ignore it
         PD_LOG( PDDEBUG, "Statistics for collection [%s.%s] is expired, "
                 "current pages [%d], statistics pages [%d], "
                 "cost threshold [%d]", pCollectionStat->getCSName(),
                 pCollectionStat->getCLName(), mbContext->mbStat()->_totalDataPages,
                 pCollectionStat->getTotalDataPages(),
                 optCB->getOptCostThreshold() ) ;
         isExpired = TRUE ;
      }

      rc = monCollectionStatInfo2Obj( pCollectionStat, pCLFullName, isDefault, isExpired, infoObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Set collection stat info obj failed, rc: %d", rc ) ;

      rc = context->monAppend( infoObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add object to context, rc: %d", rc ) ;

   done:
      if ( NULL != pSU && NULL != mbContext )
      {
         pSU->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNGETCOLLECTIONSTAT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETINDEXSTAT, "rtnGetIndexStat" )
   static INT32 rtnGetIndexStat( const rtnQueryOptions &options,
                                 SDB_DMSCB *dmsCB,
                                 _pmdEDUCB *cb,
                                 SDB_RTNCB *rtnCB,
                                 rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETINDEXSTAT ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;

      rtnContextPtr pContextBase ;
      rtnContextBuf buffObj ;
      SINT64 queryContextID = -1 ;
      BSONObj dummy ;

      // Build the matcher to only query the target index statistics.
      const CHAR *pCollection = options.getCLFullName() ;
      CHAR strCollectionFullName [ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {0} ;
      ossStrncpy( strCollectionFullName, pCollection,
                  sizeof( strCollectionFullName ) - 1 ) ;
      CHAR *pDot = ossStrchr( strCollectionFullName, '.' ) ;
      *pDot = '\0' ;
      CHAR *pCollectionSpaceName = strCollectionFullName ;
      CHAR *pCollectionShortName = pDot + 1 ;

      const CHAR *pIndexName = NULL ;
      BOOLEAN detail = FALSE ;

      rtnQueryOptions queryOptions ;
      queryOptions.setSelector( dummy ) ;
      queryOptions.setOrderBy( dummy ) ;
      queryOptions.setSkip( 0 ) ;
      queryOptions.setLimit( -1 ) ;
      queryOptions.setFlag( 0 ) ;
      queryOptions.setCLFullName( DMS_STAT_INDEX_CL_NAME ) ;

      try
      {
         BSONObj hint = options.getHint() ;
         pIndexName = hint.getField( FIELD_NAME_INDEX ).valuestrsafe() ;
         BSONElement beOpt = hint.getField( CMD_ADMIN_PREFIX FIELD_NAME_OPTIONS ) ;
         if ( beOpt.type() == Object )
         {
            detail = beOpt.embeddedObject().getField( FIELD_NAME_DETAIL ).booleanSafe() ;
         }

         // { CollectionSpace: <cs>, Collection: <cl>, Index: <idx> }
         BSONObjBuilder queryOb( 128 ) ;
         queryOb.append( FIELD_NAME_COLLECTIONSPACE, pCollectionSpaceName ) ;
         queryOb.append( FIELD_NAME_COLLECTION, pCollectionShortName ) ;
         queryOb.append( FIELD_NAME_INDEX, pIndexName ) ;
         queryOptions.setQuery( queryOb.obj() ) ;
         // { "": "STATIDXIDX" }
         BSONObjBuilder hintOb( 32 ) ;
         hintOb.append( "", DMS_STAT_IDX_IDX_NAME ) ;
         queryOptions.setHint( hintOb.obj() ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      // Query from SYSSTAT.SYSINDEXSTAT. If no statistics, do nothing.
      rc = rtnQuery ( queryOptions, cb, dmsCB, rtnCB, queryContextID,
                      &pContextBase ) ;
      if ( SDB_DMS_EOC == rc || SDB_DMS_CS_NOTEXIST == rc ||
           SDB_DMS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to query for index statistics. collection[%s], "
                   "index[%s], rc: %d", pCollection, pIndexName, rc ) ;

      while ( 0 == ( rc = rtnGetMore( queryContextID, -1, buffObj, cb, rtnCB ) ) )
      {
         BSONObj stat( buffObj.data() ) ;
         BSONObjBuilder ob ;

         rc = monBuildIndexStatResult( stat, 0, ob, detail ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON object, rc: %d", rc ) ;

         rc = context->monAppend( ob.obj() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add object to context, rc: %d",
                      rc ) ;
      }
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get more index statistics. collection[%s], "
                   "index[%s], rc: %d", pCollection, pIndexName, rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNGETINDEXSTAT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnGetQueryMeta( const rtnQueryOptions & options,
                          SDB_DMSCB *dmsCB,
                          pmdEDUCB *cb,
                          rtnContextDump *context )
   {
      return SDB_ENGINE_NOT_SUPPORT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETCOMMANDENTRY, "rtnGetCommandEntry" )
   INT32 rtnGetCommandEntry ( RTN_COMMAND_TYPE command,
                              const rtnQueryOptions & options,
                              pmdEDUCB *cb,
                              SDB_DMSCB *dmsCB,
                              SDB_RTNCB *rtnCB,
                              SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETCOMMANDENTRY ) ;
      const CHAR * pCollectionName = options.getCLFullName() ;
      SDB_ASSERT ( pCollectionName, "collection name can't be NULL " ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "runtimeCB can't be NULL" ) ;
      rtnContextDump::sharePtr context ;

      // create cursors
      rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, context,
                               contextID, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create new context, rc: %d", rc ) ;
         goto error ;
      }

      // For count(), the condition is pushed down to rtnQuery. So passing an
      // empty one to open() here. Otherwise count() with text search condition
      // will fail.
      rc = context->open( options.getSelector(),
                          ( CMD_GET_COUNT == command ) ? BSONObj() : options.getQuery(),
                          options.isOrderByEmpty() ? options.getLimit() : -1,
                          options.isOrderByEmpty() ? options.getSkip() : 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

      // sample timestamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      // do each commands, $get
      switch ( command )
      {
         case CMD_GET_COUNT :
            rc = rtnGetCount ( options, dmsCB, cb, rtnCB, context ) ;
            break ;
         case CMD_GET_DATABLOCKS :
            rc = rtnGetDatablocks( pCollectionName, dmsCB, cb, context ) ;
            break ;
         case CMD_GET_CL_DETAIL :
            rc = rtnGetCollectionDetail( pCollectionName, dmsCB, context ) ;
            break ;
         case CMD_GET_CL_STAT :
            rc = rtnGetCollectionStat( pCollectionName, cb, dmsCB, rtnCB, context ) ;
            break ;
         case CMD_GET_INDEX_STAT :
            rc = rtnGetIndexStat( options, dmsCB, cb, rtnCB, context ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            break ;
      }
      PD_RC_CHECK( rc, PDERROR, "Dump collection[%s] info[command:%d] failed, "
                   "rc: %d", pCollectionName, command, rc ) ;

      if ( !options.isOrderByEmpty() )
      {
         rc = rtnSort( context, options.getOrderBy(), cb,
                       options.getSkip(), options.getLimit(), contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNGETCOMMANDENTRY, rc ) ;
      return rc ;
   error :
      // delete the context when something goes wrong
      rtnCB->contextDelete ( contextID, cb ) ;
      contextID = -1 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATECSCOMMAND, "rtnCreateCollectionSpaceCommand" )
   INT32 rtnCreateCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                           pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                           SDB_DPSCB *dpsCB,
                                           utilCSUniqueID csUniqueID,
                                           INT32 pageSize,
                                           INT32 lobPageSize,
                                           DMS_STORAGE_TYPE type,
                                           BOOLEAN sysCall )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCREATECSCOMMAND ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su   = NULL ;
      BOOLEAN writable     = FALSE ;
      BOOLEAN hasAquired   = FALSE ;
      pmdOptionsCB *optCB  = pmdGetOptionCB() ;

      // make sure the collectionspace length is not out of range
      UINT32 length = ossStrlen ( pCollectionSpace ) ;
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // validate collection space name
      rc = dmsCheckCSName ( pCollectionSpace, sysCall ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Invalid collection space name, rc = %d",
                  rc ) ;
         goto error ;
      }

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      // let's see if the CS already exist or not
      rc = dmsCB->nameToSUAndLock ( pCollectionSpace, suID, &su ) ;
      if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         // make sure assign su to NULL so that we won't delete it at exit
         su = NULL ;
         // collectionspace already exist
         PD_LOG ( PDERROR, "Collection space %s is already exist",
                  pCollectionSpace ) ;
         rc = SDB_DMS_CS_EXIST ;
         goto error ;
      }

      dmsCB->aquireCSMutex( pCollectionSpace ) ;
      hasAquired = TRUE ;

      rc = dmsCB->nameToSUAndLock ( pCollectionSpace, suID, &su ) ;
      if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         su = NULL ;
         PD_LOG ( PDERROR, "Collection space %s is already exist",
                  pCollectionSpace ) ;
         rc = SDB_DMS_CS_EXIST ;
         goto error ;
      }

      // only for standalone
      if ( SDB_ROLE_STANDALONE == pmdGetKRCB()->getDBRole() )
      {
         rc = rtnLoadCollectionSpace ( pCollectionSpace,
                                       pmdGetOptionCB()->getDbPath(),
                                       pmdGetOptionCB()->getIndexPath(),
                                       pmdGetOptionCB()->getLobPath(),
                                       pmdGetOptionCB()->getLobMetaPath(),
                                       cb, dmsCB, FALSE ) ;
         if ( rc != SDB_DMS_CS_NOTEXIST )
         {
            PD_LOG ( PDERROR, "The container file for collect space %s exists "
                     "or load failed, rc: %d", pCollectionSpace, rc ) ;
            goto done ;
         }
      }

      if ( !UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         rc = dmsCB->allocCSUniqueID( csUniqueID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate colleciton space "
                      "unique ID, rc: %d", rc ) ;
         PD_LOG( PDEVENT, "Allocate collection space unique ID [%u]", csUniqueID ) ;
      }

      // new storage unit, will insert into dmsCB->addCollectionSpace
      su = SDB_OSS_NEW dmsStorageUnit ( dmsCB->getStorageService(),
                                        pCollectionSpace,
                                        csUniqueID, 1,
                                        pmdGetBuffPool(),
                                        pageSize,
                                        lobPageSize,
                                        type,
                                        rtnGetExtDataHandler() ) ;
      if ( !su )
      {
         PD_LOG ( PDERROR, "Failed to allocate new storage unit" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = su->open ( pmdGetOptionCB()->getDbPath(),
                      pmdGetOptionCB()->getIndexPath(),
                      pmdGetOptionCB()->getLobPath(),
                      pmdGetOptionCB()->getLobMetaPath(),
                      pmdGetSyncMgr(),
                      dmsCB->getStatMgr(),
                      TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create collection space %s at %s, rc: %d",
                  pCollectionSpace, pmdGetOptionCB()->getDbPath(),
                  rc ) ;
         goto error ;
      }
      /// set config
      su->setSyncConfig( optCB->getSyncInterval(),
                         optCB->getSyncRecordNum(),
                         optCB->getSyncDirtyRatio() ) ;
      su->setSyncDeep( optCB->isSyncDeep() ) ;

      /// add collctionspace
      rc = dmsCB->addCollectionSpace( pCollectionSpace, 1, su, cb, dpsCB, TRUE ) ;
      if ( rc )
      {
         if ( SDB_DMS_CS_EXIST == rc )
         {
            PD_LOG ( PDWARNING, "Failed to add collectionspace because it's "
                     "already exist: %s", pCollectionSpace ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to add collection space, rc = %d", rc ) ;
         }
         /// need to remove the files
         su->remove() ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Create collectionspace[name: %s, id: %u] succeed, "
              "PageSize:%u, LobPageSize:%u", pCollectionSpace,
              csUniqueID, pageSize, lobPageSize ) ;

   done :
      // Unlock the existing storage unit
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( hasAquired )
      {
         dmsCB->releaseCSMutex( pCollectionSpace ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCREATECSCOMMAND, rc ) ;
      return rc ;
   error :
      if ( su )
      {
         SDB_OSS_DEL (su) ;
         su = NULL ;
      }
      goto done ;
   }

   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      UINT32 attributes,
                                      _pmdEDUCB * cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      utilCLUniqueID clUniqueID,
                                      UTIL_COMPRESSOR_TYPE compressorType,
                                      INT32 flags,
                                      BOOLEAN sysCall,
                                      const BSONObj *extOptions,
                                      const BSONObj *pIdIdxDef,
                                      BOOLEAN addIdxIDIfNotExist )
   {
      BSONObj shardIdxDef ;
      return rtnCreateCollectionCommand ( pCollection,
                                          shardIdxDef,
                                          attributes, cb, dmsCB, dpsCB,
                                          clUniqueID, compressorType,
                                          flags, sysCall, extOptions,
                                          pIdIdxDef, addIdxIDIfNotExist ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATECLCOMMAND, "rtnCreateCollectionCommand" )
   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      const BSONObj &shardIdxDef,
                                      UINT32 attributes,
                                      _pmdEDUCB *cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      utilCLUniqueID clUniqueID,
                                      UTIL_COMPRESSOR_TYPE compType,
                                      INT32 flags, BOOLEAN sysCall,
                                      const BSONObj *extOptions,
                                      const BSONObj *pIdIdxDef,
                                      BOOLEAN addIdxIDIfNotExist )
   {
      INT32 rc              = SDB_OK ;
      INT32 rcTmp           = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCREATECLCOMMAND ) ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su    = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      BOOLEAN writable      = FALSE ;
      UINT16 collectionID   = DMS_INVALID_MBID ;
      UINT32 logicalID      = DMS_INVALID_CLID ;
      const CHAR *pCollectionShortName = NULL ;
      utilCSUniqueID csUniqueID = utilGetCSUniqueID( clUniqueID ) ;
      CHAR attrStr[ 64 + 1 ] = { 0 } ;

      // Check writable before su lock
      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;

      if ( rc && pCollectionShortName && (flags&FLG_CREATE_WHEN_NOT_EXIST) )
      {
         CHAR temp [ DMS_COLLECTION_SPACE_NAME_SZ +
                     DMS_COLLECTION_NAME_SZ + 2 ] = {0} ;
         ossStrncpy ( temp, pCollection, sizeof(temp) ) ;
         DMS_STORAGE_TYPE type =
            OSS_BIT_TEST( attributes, DMS_MB_ATTR_CAPPED ) ?
            DMS_STORAGE_CAPPED : DMS_STORAGE_NORMAL ;
         SDB_ASSERT ( pCollectionShortName > pCollection, "Collection pointer "
                      "is not part of full collection name" ) ;
         // set '.' to '\0'
         temp [ pCollectionShortName - pCollection - 1 ] = '\0' ;
         if ( SDB_OK == rtnCreateCollectionSpaceCommand ( temp, cb,
                                                          dmsCB, dpsCB,
                                                          csUniqueID,
                                                          DMS_PAGE_SIZE_DFT,
                                                          DMS_DEFAULT_LOB_PAGE_SZ,
                                                          type,
                                                          sysCall ) )
         {
            //restore '\0' to '.'
            rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                                   &pCollectionShortName,
                                                   suID ) ;
         }
      }

      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      if ( DMS_STORAGE_CAPPED != su->type() &&
           OSS_BIT_TEST( attributes, DMS_MB_ATTR_CAPPED ) )
      {
         PD_LOG( PDERROR, "Capped collection[%s] can only be created on "
                 "capped collection space[%s]",
                 pCollectionShortName, su->CSName() ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      if ( UTIL_CLUNIQUEID_LOCAL == clUniqueID )
      {
         if ( UTIL_CSUNIQUEID_LOCAL != su->CSUniqueID() )
         {
            clUniqueID = utilBuildCLUniqueID( su->CSUniqueID(),
                                              UTIL_CLINNERID_LOCAL ) ;
         }
      }

      rc = su->data()->addCollection ( pCollectionShortName, &collectionID,
                                       clUniqueID, attributes, cb,
                                       dpsCB, 0, sysCall,
                                       compType, &logicalID, extOptions,
                                       pIdIdxDef, addIdxIDIfNotExist ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to create collection [name:%s, id:%llu], rc: %d",
                  pCollection, clUniqueID, rc ) ;
         goto error ;
      }

      if ( !shardIdxDef.isEmpty() )
      {
         rc = rtnCreateIndexCommand ( pCollection, shardIdxDef,
                                      cb, dmsCB, dpsCB, TRUE,
                                      SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                      NULL, NULL, addIdxIDIfNotExist ) ;
         if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
         {
            /// same defined index already exists.
            rc = SDB_OK ;
         }
         else if ( SDB_OK != rc )
         {
               PD_LOG ( PDERROR, "Failed to create shard index for "
                        "collection %s, rc = %d", pCollection, rc ) ;
            goto error_rollback ;
         }
      }

      if ( OSS_BIT_TEST( attributes, DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == compType )
      {
         /*
          * If the compression type is snappy, set it directly. If it's lzw, push
          * it to the dictionary creating list.
          */
         dmsCB->pushDictJob( dmsDictJob( suID, su->LogicalCSID(),
                                         collectionID, logicalID ) ) ;
      }

      mbAttr2String( attributes, attrStr, sizeof( attrStr ) - 1 ) ;
      PD_LOG( PDEVENT, "Create collection[name: %s, id: %llu] succeed, "
              "ShardingKey:%s, Attr:%s(0x%08x), CompressType:%s(%d)%s%s%s%s",
              pCollection, clUniqueID,
              shardIdxDef.getObjectField(IXM_FIELD_NAME_KEY).toString().c_str(),
              attrStr, attributes,
              utilCompressType2String( (UINT8)compType ), compType,
              extOptions && !extOptions->isEmpty() ? ", External options:" : "",
              extOptions && !extOptions->isEmpty() ? extOptions->toString().c_str() : "",
              pIdIdxDef && !pIdIdxDef->isEmpty() ? ", Id Index:" : "",
              pIdIdxDef && !pIdIdxDef->isEmpty() ? pIdIdxDef->toString().c_str() : "" ) ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCREATECLCOMMAND, rc ) ;
      return rc ;
   error_rollback :
      rcTmp = rtnDropCollectionCommand ( pCollection, cb, dmsCB, dpsCB,
                                         clUniqueID ) ;
      if ( SDB_OK != rcTmp && SDB_DMS_NOTEXIST != rcTmp )
      {
         PD_LOG ( PDERROR, "Failed to rollback creating collection %s, rc = %d",
                  pCollection, rcTmp ) ;
      }
      goto done ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATEINDEXCOMMAND, "rtnCreateIndexCommand" )
   INT32 rtnCreateIndexCommand ( const CHAR *pCollection,
                                 const BSONObj &indexObj,
                                 _pmdEDUCB *cb,
                                 SDB_DMSCB *dmsCB,
                                 SDB_DPSCB *dpsCB,
                                 BOOLEAN isSys,
                                 INT32 sortBufferSize,
                                 utilWriteResult *pResult,
                                 dmsIdxTaskStatus *pIdxStatus,
                                 BOOLEAN addUIDIfNotExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCREATEINDEXCOMMAND ) ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;

      dmsStorageUnit *su            = NULL ;
      dmsStorageUnitID suID         = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      BOOLEAN writable              = FALSE ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      if ( DMS_STORAGE_CAPPED == su->type() )
      {
         PD_LOG( PDERROR, "Index is not support on capped collection" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      rc = su->createIndex ( pCollectionShortName, indexObj,
                             cb, dpsCB, isSys, NULL, sortBufferSize,
                             pResult, pIdxStatus, FALSE, addUIDIfNotExist ) ;
      if ( rc )
      {
         // SDB_IXM_EXIST may happen when user mistakenly type index name with
         // same name, so we display INFO instead of ERROR
         PD_LOG ( PDERROR, "Failed to create index %s: %s, rc: %d",
                  pCollection, indexObj.toString().c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Create index[%s] for collection[%s] succeed",
              indexObj.toString().c_str(), pCollection ) ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCREATEINDEXCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATEINDEXCOMMAND1, "rtnCreateIndexCommand" )
   INT32 rtnCreateIndexCommand ( utilCLUniqueID clUniqID,
                                 const BSONObj &indexObj,
                                 _pmdEDUCB *cb,
                                 SDB_DMSCB *dmsCB,
                                 SDB_DPSCB *dpsCB,
                                 BOOLEAN isSys,
                                 INT32 sortBufferSize,
                                 utilWriteResult *pResult,
                                 dmsIdxTaskStatus *pIdxStatus,
                                 BOOLEAN addUIDIfNotExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCREATEINDEXCOMMAND1 ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;

      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      BOOLEAN writable = FALSE ;
      utilCSUniqueID csUniqID = utilGetCSUniqueID( clUniqID ) ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->idToSUAndLock( csUniqID, suID, &su, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to loop up su by cs unique id[%u], rc: %d",
                   csUniqID, rc ) ;

      if ( DMS_STORAGE_CAPPED == su->type() )
      {
         PD_LOG( PDERROR, "Index is not support on capped collection" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      rc = su->createIndex( clUniqID, indexObj, cb, dpsCB, isSys, NULL,
                            sortBufferSize, pResult, pIdxStatus, FALSE,
                            addUIDIfNotExist ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create index[%s] for collection[%llu], "
                  "rc: %d", indexObj.toString().c_str(), clUniqID, rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Create index[%s] for collection[%llu] succeed",
              indexObj.toString().c_str(), clUniqID ) ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCREATEINDEXCOMMAND1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPINDEXCOMMAND, "rtnDropIndexCommand" )
   INT32 rtnDropIndexCommand ( const CHAR *pCollection,
                               const BSONElement &identifier,
                               pmdEDUCB *cb,
                               SDB_DMSCB *dmsCB,
                               SDB_DPSCB *dpsCB,
                               BOOLEAN sysCall,
                               dmsIdxTaskStatus *pIdxStatus,
                               BOOLEAN onlyStandalone )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPINDEXCOMMAND ) ;

      OID oid ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su               = NULL ;
      dmsStorageUnitID suID            = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      BOOLEAN writable                 = FALSE ;

      if ( identifier.type() != jstOID && identifier.type() != String )
      {
         PD_LOG ( PDERROR, "Invalid index identifier type: %s",
                 identifier.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      if ( identifier.type() == jstOID )
      {
         identifier.Val(oid) ;
         rc = su->dropIndex ( pCollectionShortName, oid, cb, dpsCB, sysCall,
                              NULL, pIdxStatus, onlyStandalone ) ;
      }
      else if ( identifier.type() == String )
      {
         rc = su->dropIndex ( pCollectionShortName, identifier.valuestr(),
                              cb, dpsCB, sysCall, NULL, pIdxStatus,
                              onlyStandalone ) ;
      }
      else
      {
         PD_LOG ( PDERROR, "Invalid identifier type" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to drop index %s: %s, rc: %d",
                  pCollection, identifier.toString().c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Drop index[%s] for collection[%s] succeed",
              identifier.toString().c_str(), pCollection ) ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNDROPINDEXCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPINDEXCOMMAND1, "rtnDropIndexCommand" )
   INT32 rtnDropIndexCommand ( utilCLUniqueID clUniqID,
                               const BSONElement &identifier,
                               pmdEDUCB *cb,
                               SDB_DMSCB *dmsCB,
                               SDB_DPSCB *dpsCB,
                               BOOLEAN sysCall,
                               dmsIdxTaskStatus *pIdxStatus,
                               BOOLEAN onlyStandalone )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPINDEXCOMMAND1 ) ;

      OID oid ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su      = NULL ;
      dmsStorageUnitID suID   = DMS_INVALID_CS ;
      BOOLEAN writable        = FALSE ;
      utilCSUniqueID csUniqID = utilGetCSUniqueID( clUniqID ) ;

      if ( identifier.type() != jstOID && identifier.type() != String )
      {
         PD_LOG ( PDERROR, "Invalid index identifier type: %s",
                 identifier.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->idToSUAndLock( csUniqID, suID, &su, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to loop up su by cs unique id[%u], rc: %d",
                   csUniqID, rc ) ;

      if ( identifier.type() == jstOID )
      {
         identifier.Val(oid) ;
         rc = su->dropIndex( clUniqID, oid, cb, dpsCB, sysCall, NULL,
                             pIdxStatus, onlyStandalone ) ;
      }
      else if ( identifier.type() == String )
      {
         rc = su->dropIndex( clUniqID, identifier.valuestr(), cb, dpsCB,
                             sysCall, NULL, pIdxStatus, onlyStandalone ) ;
      }
      else
      {
         PD_LOG ( PDERROR, "Invalid identifier type" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to drop index[%s] for collection[%llu], rc: %d",
                  identifier.toString().c_str(), clUniqID, rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Drop index[%s] for collection[%llu] succeed",
              identifier.toString().c_str(), clUniqID ) ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNDROPINDEXCOMMAND1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRENAMECSCOMMAND, "rtnRenameCollectionSpaceCommand" )
   INT32 rtnRenameCollectionSpaceCommand ( const CHAR *csName,
                                           const CHAR *newCSName,
                                           _pmdEDUCB *cb,
                                           SDB_DMSCB *dmsCB,
                                           SDB_DPSCB *dpsCB,
                                           BOOLEAN blockWrite )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNRENAMECSCOMMAND ) ;
      BOOLEAN lockDMS = FALSE ;
      utilRenameLogger logger ;
      dmsTaskStatusMgr* pTaskStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;

      /// dms lock
      if ( blockWrite )
      {
         // When two threads concurrently do rename, blockWrite() will report
         // -148. We retry multiple times to reduce the error.
         INT16 i = 0 ;
         while ( ( rc = dmsCB->blockWrite( cb ) )  &&
                 ( i < RTN_RENAME_BLOCKWRITE_TIMES ) )
         {
            ossSleep( RTN_RENAME_BLOCKWRITE_INTERAL ) ;
            i++ ;
         }
         PD_RC_CHECK( rc, PDERROR, "Block dms write failed, rc: %d", rc ) ;
         PD_LOG( PDINFO, "Block write operation succeed" ) ;
      }
      else
      {
         rc = dmsCB->writable( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      }
      lockDMS = TRUE ;

      /// log to .SEQUOIADB_RENAME_INFO
      {
         utilRenameLog aLog ( csName, newCSName ) ;

         rc = logger.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init rename logger, rc: %d", rc );

         rc = logger.log( aLog ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to log rename info to file, rc: %d", rc ) ;
      }

      /// dms rename
      rc = dmsCB->renameCollectionSpace( csName, newCSName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collectionspace from %s to %s, rc: %d",
                   csName, newCSName, rc ) ;

      /// remove .SEQUOIADB_RENAME_INFO
      rc = logger.clear() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to clear rename info, rc: %d", rc ) ;

      pTaskStatMgr->renameCS( csName, newCSName ) ;

      PD_LOG( PDEVENT, "Rename cs[%s] to [%s] succeed", csName, newCSName ) ;

   done:
      if ( lockDMS )
      {
         if ( blockWrite )
         {
            dmsCB->unblockWrite( cb ) ;
            PD_LOG( PDINFO, "Unblock write operation succeed" ) ;
         }
         else
         {
            dmsCB->writeDown( cb ) ;
         }
         lockDMS = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB_RTNRENAMECSCOMMAND, rc ) ;
      return rc ;
   error:
      {
         INT32 tmpRC = logger.clear() ;
         if ( tmpRC )
         {
            PD_LOG( PDERROR, "Failed to clear rename info, rc: %d", tmpRC ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRTRNCLCMD, "_rtnReturnCLCommand" )
   static INT32 _rtnReturnCLCommand( dmsReturnOptions &options,
                                     _pmdEDUCB *cb,
                                     SDB_DMSCB *dmsCB,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRTRNCLCMD ) ;

      const CHAR *originName = options._recycleItem.getOriginName() ;
      const CHAR *recycleName = options._recycleItem.getRecycleName() ;

      const CHAR *originCLName = NULL ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsMBContext *origMBContext = NULL ;

      rc = rtnResolveCollectionNameAndLock( originName, dmsCB, &su,
                                            &originCLName, suID, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit for collection "
                   "[%s], rc: %d", originName, rc ) ;

      rc = su->data()->returnCollection( originCLName, recycleName, options, cb,
                                         dpsCB, &origMBContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to return collection [%s] to [%s], "
                   "rc: %d", recycleName, originCLName, rc ) ;

      if ( NULL != origMBContext &&
           OSS_BIT_TEST( origMBContext->mb()->_attributes,
                         DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == origMBContext->mb()->_compressorType )
      {
         dmsCB->pushDictJob( dmsDictJob( su->CSID(),
                                         su->LogicalCSID(),
                                         origMBContext->mbID(),
                                         origMBContext->clLID() ) ) ;
      }

   done:
      if ( NULL != origMBContext && NULL != su )
      {
         su->data()->releaseMBContext( origMBContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNRTRNCLCMD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRTRNCSCMD, "_rtnReturnCSCommand" )
   static INT32 _rtnReturnCSCommand( dmsReturnOptions &options,
                                     _pmdEDUCB *cb,
                                     SDB_DMSCB *dmsCB,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRTRNCSCMD ) ;

      const CHAR *originName = options._recycleItem.getOriginName() ;
      const CHAR *recycleName = options._recycleItem.getRecycleName() ;

      UINT32 suLogicalID = DMS_INVALID_LOGICCSID ;

      rc = dmsCB->nameToSULID( recycleName, suLogicalID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical ID for "
                   "collection space [%s], rc: %d", recycleName,
                   suLogicalID, rc ) ;
      SDB_ASSERT( DMS_INVALID_LOGICCSID != suLogicalID,
                  "logical ID should be valid" ) ;

      rc = dmsCB->returnCollectionSpace( options, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to return collection space "
                   "[origin %s, recycle %s], rc: %d", originName,
                   recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNRTRNCSCMD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRTRNCMD, "rtnReturnCommand" )
   INT32 rtnReturnCommand( dmsReturnOptions &options,
                           _pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_DPSCB *dpsCB,
                           BOOLEAN blockWrite )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNRTRNCMD ) ;

      BOOLEAN lockDMS = FALSE ;

      PD_CHECK( options._recycleItem.isValid(), SDB_SYS, error, PDERROR,
                "Failed to run return command, recycle item is invalid" ) ;

      if ( blockWrite )
      {
         // When two threads concurrently do rename, blockWrite() will report
         // -148. We retry multiple times to reduce the error.
         INT16 i = 0 ;
         while ( ( rc = dmsCB->blockWrite( cb ) )  &&
                 ( i < RTN_RENAME_BLOCKWRITE_TIMES ) )
         {
            ossSleep( RTN_RENAME_BLOCKWRITE_INTERAL ) ;
            i++ ;
         }
         PD_RC_CHECK( rc, PDERROR, "Block dms write failed, rc: %d", rc ) ;
         PD_LOG( PDINFO, "Block write operation succeed" ) ;
      }
      else
      {
         rc = dmsCB->writable( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      }
      lockDMS = TRUE ;

      if ( UTIL_RECYCLE_CS == options._recycleItem.getType() )
      {
         rc = _rtnReturnCSCommand( options, cb, dmsCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return collection space, "
                      "rc: %d", rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == options._recycleItem.getType() )
      {
         rc = _rtnReturnCLCommand( options, cb, dmsCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to return collection, rc: %d",
                      rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to run return "
                   "command, invalid recycle type [%d]",
                   options._recycleItem.getType() ) ;
      }

   done:
      if ( lockDMS )
      {
         if ( blockWrite )
         {
            dmsCB->unblockWrite( cb ) ;
            PD_LOG( PDINFO, "Unblock write operation succeed" ) ;
         }
         else
         {
            dmsCB->writeDown( cb ) ;
         }
         lockDMS = FALSE ;
      }
      PD_TRACE_EXITRC( SDB_RTNRTRNCMD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPCSCOMMAND, "rtnDropCollectionSpaceCommand" )
   INT32 rtnDropCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         _pmdEDUCB *cb,
                                         SDB_DMSCB *dmsCB,
                                         SDB_DPSCB *dpsCB,
                                         BOOLEAN   sysCall,
                                         BOOLEAN   ensureEmpty,
                                         dmsDropCSOptions *options )
   {
      PD_TRACE_ENTRY ( SDB_RTNDROPCSCOMMAND ) ;
      INT32 rc = rtnDelCollectionSpaceCommand( pCollectionSpace, cb,
                                               dmsCB, dpsCB, sysCall,
                                               TRUE, ensureEmpty, options ) ;
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Drop collectionspace[%s] succeed",
                 pCollectionSpace ) ;

         dmsTaskStatusMgr* pTaskStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;
         pTaskStatMgr->dropCS( pCollectionSpace ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNDROPCSCOMMAND, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPCSP1, "rtnDropCollectionSpaceP1" )
   INT32 rtnDropCollectionSpaceP1 ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPCSP1 ) ;

      UINT32 retryTime = 0 ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      UINT32 suLogicalID = DMS_INVALID_LOGICCSID ;

      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      // make sure the collectionspace length is not out of range
      UINT32 length = ossStrlen ( pCollectionSpace ) ;
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = dmsCB->nameToSULID( pCollectionSpace, suLogicalID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical ID for "
                   "collection space [%s], rc: %d", pCollectionSpace, rc ) ;
      SDB_ASSERT( DMS_INVALID_LOGICCSID != suLogicalID,
                  "logical ID should be valid" ) ;

      // let's find out whether the collection space is held by this
      // EDU. If so we have to get rid of those contexts
      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( pCollectionSpace, suLogicalID, cb ) ;
      }

      while ( TRUE )
      {
         if ( ( PMD_IS_DB_DOWN() ) ||
              ( NULL != cb && cb->isInterrupted() ) )
         {
            PD_LOG( PDWARNING, "Failed to drop collection space [%s], "
                    "it is interrupted", pCollectionSpace ) ;
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         // - tell others to close contexts on the same collection space
         // - tell other waiting transactions to give up
         if ( ( rtnCB->preDelContext( pCollectionSpace, suLogicalID ) > 0 ) ||
              ( NULL != cb && cb->getTransExecutor()->useTransLock() &&
                transCB->transLockKillWaiters( suLogicalID,
                                               DMS_INVALID_MBID,
                                               NULL,
                                               SDB_DPS_TRANS_LOCK_INCOMPATIBLE ) ) )
         {
            ossSleep( 200 ) ;
         }

         dmsCB->aquireCSMutex( pCollectionSpace ) ;
         rc = dmsCB->dropCollectionSpaceP1( pCollectionSpace, cb, dpsCB ) ;
         dmsCB->releaseCSMutex( pCollectionSpace ) ;
         if ( SDB_LOCK_FAILED == rc && retryTime < 100 )
         {
            ++ retryTime ;
            rc = SDB_OK ;
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop collectionspace %s, "
                      "rc: %d", pCollectionSpace, rc ) ;
         break ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNDROPCSP1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPCSP1CANCEL, "rtnDropCollectionSpaceP1Cancel" )
   INT32 rtnDropCollectionSpaceP1Cancel ( const CHAR *pCollectionSpace,
                                          _pmdEDUCB *cb,
                                          SDB_DMSCB *dmsCB,
                                          SDB_DPSCB *dpsCB,
                                          BOOLEAN   sysCall )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPCSP1CANCEL ) ;
      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" );
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" );
      UINT32 length = ossStrlen ( pCollectionSpace ) ;
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      dmsCB->aquireCSMutex( pCollectionSpace ) ;
      rc = dmsCB->dropCollectionSpaceP1Cancel( pCollectionSpace, cb, dpsCB ) ;
      dmsCB->releaseCSMutex( pCollectionSpace ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to cancel remove cs(name:%s, rc=%d)",
                   pCollectionSpace, rc );
   done:
      PD_TRACE_EXITRC ( SDB_RTNDROPCSP1CANCEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPCSP2, "rtnDropCollectionSpaceP2" )
   INT32 rtnDropCollectionSpaceP2 ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall,
                                    dmsDropCSOptions *options )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPCSP2 ) ;
      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" );
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" );
      UINT32 length = ossStrlen ( pCollectionSpace ) ;

      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      dmsCB->aquireCSMutex( pCollectionSpace ) ;
      rc = dmsCB->dropCollectionSpaceP2( pCollectionSpace, cb, dpsCB, options ) ;
      dmsCB->releaseCSMutex( pCollectionSpace ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to drop cs(name:%s, rc=%d)",
                   pCollectionSpace, rc ) ;

      PD_LOG( PDEVENT, "Drop collectionspace[%s] succeed", pCollectionSpace ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNDROPCSP2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPCLCOMMAND, "rtnDropCollectionCommand" )
   INT32 rtnDropCollectionCommand ( const CHAR *pCollection,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    utilCLUniqueID clUniqueID,
                                    dmsDropCLOptions *options )
   {
      INT32 rc                            = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPCLCOMMAND ) ;
      dmsStorageUnitID suID               = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su                  = NULL ;
      const CHAR *pCollectionShortName    = NULL ;
      BOOLEAN writable                    = FALSE ;
      dmsMBContext * mbContext            = NULL ;
      dmsTaskStatusMgr* pTaskStatMgr      = sdbGetRTNCB()->getTaskStatusMgr() ;

      if ( dmsCheckFullCLName( pCollection, TRUE ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Collection name is invalid[%s], rc: %d",
                 pCollection, rc ) ;
         goto error ;
      }

      // Check writable before su lock
      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      if ( UTIL_UNIQUEID_NULL != clUniqueID )
      {
         rc = su->data()->getMBContext( &mbContext, pCollectionShortName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get mbContext for collection "
                      "%s, rc: %d", pCollection, rc ) ;

         PD_CHECK( mbContext->mb()->_clUniqueID == clUniqueID,
                   SDB_DMS_NOTEXIST, error, PDWARNING,
                   "Collection %s with unique ID %llu had been dropped, "
                   "current unique ID is %llu", pCollection,
                   clUniqueID, mbContext->mb()->_clUniqueID ) ;
      }

      rc = su->data()->dropCollection ( pCollectionShortName, cb, dpsCB,
                                        TRUE, mbContext, options ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to drop collection %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      pTaskStatMgr->dropCL( pCollection ) ;

      PD_LOG( PDEVENT, "Drop collection[%s] succeed", pCollection ) ;

   done :
      if ( NULL != mbContext )
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
      PD_TRACE_EXITRC ( SDB_RTNDROPCLCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRENAMECLCOMMAND, "rtnRenameCollectionCommand" )
   INT32 rtnRenameCollectionCommand ( const CHAR *csName,
                                      const CHAR *clShortName,
                                      const CHAR *newCLShortName,
                                      _pmdEDUCB *cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      BOOLEAN blockWrite )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNRENAMECLCOMMAND ) ;
      dmsStorageUnitID suID   = DMS_INVALID_CS ;
      dmsStorageUnit *su      = NULL ;
      BOOLEAN lockDMS         = FALSE ;

      // When two threads concurrently do rename, blockWrite() will report
      // -148. We retry multiple times to reduce the error.
      if ( blockWrite )
      {
         INT16 i = 0 ;
         while ( ( rc = dmsCB->blockWrite( cb ) )  &&
                 ( i < RTN_RENAME_BLOCKWRITE_TIMES ) )
         {
            ossSleep( RTN_RENAME_BLOCKWRITE_INTERAL ) ;
            i++ ;
         }
         PD_RC_CHECK( rc, PDERROR, "Block dms write failed, rc: %d", rc ) ;
         PD_LOG( PDINFO, "Block write operation succeed" ) ;
      }
      else
      {
         rc = dmsCB->writable( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      }
      lockDMS = TRUE ;

      rc = rtnCollectionSpaceLock ( csName, dmsCB, FALSE,
                                    &su, suID, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Fail to lock collection space name[%s]",
                   csName ) ;

      rc = su->data()->renameCollection ( clShortName, newCLShortName,
                                          cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection from %s to %s, rc: %d",
                   clShortName, newCLShortName, rc ) ;

      {
         CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         CHAR newCLFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         ossSnprintf( clFullName, sizeof( clFullName ),
                      "%s.%s", csName, clShortName ) ;
         ossSnprintf( newCLFullName, sizeof( newCLFullName ),
                      "%s.%s", csName, newCLShortName ) ;
         dmsTaskStatusMgr* pTaskStatMgr = sdbGetRTNCB()->getTaskStatusMgr() ;
         pTaskStatMgr->renameCL( clFullName, newCLFullName ) ;
      }

      PD_LOG( PDEVENT, "Rename collection[%s.%s] to [%s.%s] succeed",
              csName, clShortName, csName, newCLShortName ) ;

   done:
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( lockDMS )
      {
         if ( blockWrite )
         {
            dmsCB->unblockWrite( cb ) ;
            PD_LOG( PDINFO, "Unblock write operation succeed" ) ;
         }
         else
         {
            dmsCB->writeDown( cb ) ;
         }
         lockDMS = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB_RTNRENAMECLCOMMAND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRUNCCLCOMMAND, "rtnTruncCollectionCommand" )
   INT32 rtnTruncCollectionCommand( const CHAR *pCollection, pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                                    dmsMBContext *mbContext,
                                    dmsTruncCLOptions *options )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTRUNCCLCOMMAND ) ;
      dmsStorageUnitID suID            = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su               = NULL ;
      const CHAR *pCollectionShortName = NULL ;
      BOOLEAN writable                 = FALSE ;
      dmsMB *mb                        = NULL ;
      BOOLEAN getContext               = FALSE ;

      // Check writable before su lock
      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      rc = su->data()->truncateCollection( pCollectionShortName, cb, dpsCB,
                                           TRUE, mbContext, TRUE, TRUE,
                                           options ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to truncate collection %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      /*
       * The original dictionary and compressor will be removed during
       * truncation. So it should be pushed to the dictionary creating list
       * again after truncation.
       */
      if ( NULL == mbContext )
      {
         rc = su->data()->getMBContext( &mbContext, pCollectionShortName,
                                        SHARED ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get mb context of collection %s, rc: %d",
                      pCollection, rc ) ;
         getContext = TRUE ;
      }
      else if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context of "
                      "collection %s, rc: %d", pCollection, rc ) ;
      }

      mb = mbContext->mb() ;

      if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == mb->_compressorType &&
           DMS_INVALID_EXTENT == mb->_dictExtentID )
      {
         dmsCB->pushDictJob( dmsDictJob( suID, su->LogicalCSID(),
                             mbContext->mbID(), mbContext->clLID() ) ) ;
      }

      PD_LOG( PDEVENT, "Truncate collection[%s] succeed",
              pCollection ) ;

   done :
      if ( getContext && mbContext )
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
      PD_TRACE_EXITRC ( SDB_RTNTRUNCCLCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTESTCSCOMMAND, "rtnTestCollectionSpaceCommand" )
   INT32 rtnTestCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         SDB_DMSCB *dmsCB,
                                         utilCSUniqueID *pCsUniqueID,
                                         utilCSUniqueID *pCurCsUniqueID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTESTCSCOMMAND ) ;
      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      utilCSUniqueID curCsUniqueID = UTIL_UNIQUEID_NULL ;

      if ( 0 == ossStrcmp( pCollectionSpace, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS ) )
      {
         goto done ;
      }

      rc = dmsCB->nameToSUAndLock ( pCollectionSpace, suID, &su ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }
      curCsUniqueID = su->CSUniqueID() ;

      if ( pCsUniqueID )
      {
         if ( curCsUniqueID != *pCsUniqueID )
         {
            if ( UTIL_IS_VALID_CSUNIQUEID( curCsUniqueID ) &&
                 UTIL_IS_VALID_CSUNIQUEID( *pCsUniqueID ) )
            {
               rc = SDB_DMS_CS_REMAIN ;
            }
            else
            {
               rc = SDB_DMS_CS_UNIQUEID_CONFLICT ;
            }
            PD_LOG ( PDERROR, "Collection space[%s]'s cs unique id error, "
                     "expect: %u, actual: %u, rc: %d",
                     pCollectionSpace, *pCsUniqueID, curCsUniqueID, rc ) ;
            goto error ;
         }
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( NULL != pCurCsUniqueID )
      {
         *pCurCsUniqueID = curCsUniqueID ;
      }
      PD_TRACE_EXITRC ( SDB_RTNTESTCSCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 rtnTestIndex( const CHAR *pCollection, const CHAR *pIndexName,
                       SDB_DMSCB *dmsCB, const BSONObj *pIndexDef,
                       BOOLEAN *pIsSame )
   {
      INT32 rc                   = SDB_OK ;
      dmsStorageUnit *su         = NULL ;
      dmsStorageUnitID suID      = DMS_INVALID_SUID ;
      const CHAR *pCLShortName   = NULL ;
      dmsMBContext *mbContext    = NULL ;
      dmsExtentID extentID       = DMS_INVALID_EXTENT ;

      rc = rtnResolveCollectionNameAndLock( pCollection, dmsCB, &su,
                                            &pCLShortName, suID ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = su->data()->getMBContext( &mbContext, pCLShortName, SHARED ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( pIndexDef && String == pIndexDef->getField( IXM_NAME_FIELD ).type() )
      {
         pIndexName = pIndexDef->getField( IXM_NAME_FIELD ).valuestr() ;
      }

      rc = su->index()->getIndexCBExtent( mbContext, pIndexName, extentID ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( pIndexDef && pIsSame )
      {
         ixmIndexCB indexCB( extentID, su->index(), NULL ) ;
         PD_CHECK( indexCB.isInitialized(), SDB_DMS_INIT_INDEX,
                   error, PDERROR, "Failed to initialize index" ) ;
         if ( indexCB.isSameDef( *pIndexDef ) )
         {
            *pIsSame = TRUE ;
         }
         else
         {
            *pIsSame = FALSE ;
         }
      }
      mbContext->mbUnlock() ;

   done:
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTESTCLCOMMAND, "rtnTestCollectionCommand" )
   INT32 rtnTestCollectionCommand ( const CHAR *pCollection,
                                    SDB_DMSCB *dmsCB,
                                    utilCLUniqueID *pClUniqueID,
                                    utilCLUniqueID *pCurClUniqueID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTESTCLCOMMAND ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      const CHAR *pCollectionShortName = NULL ;
      UINT16 cID ;
      dmsMBContext* mbContext = NULL ;
      utilCLUniqueID curClUniqueID = UTIL_UNIQUEID_NULL ;

      if ( 0 == ossStrcmp( pCollection, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         goto done ;
      }

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( pClUniqueID )
      {
         utilCSUniqueID expCsUniqueID = utilGetCSUniqueID( *pClUniqueID ) ;
         utilCSUniqueID curCsUniqueID = su->CSUniqueID() ;
         if ( curCsUniqueID != expCsUniqueID )
         {
            if ( UTIL_IS_VALID_CSUNIQUEID( curCsUniqueID ) &&
                 UTIL_IS_VALID_CSUNIQUEID( expCsUniqueID ) )
            {
               rc = SDB_DMS_CS_REMAIN ;
            }
            else
            {
               rc = SDB_DMS_CS_UNIQUEID_CONFLICT ;
            }
            PD_LOG ( PDERROR, "Collection[%s]'s cs unique id error, "
                     "expect: %u, actual: %u, rc: %d",
                     pCollection, expCsUniqueID, curCsUniqueID, rc ) ;
            goto error ;
         }
      }

      rc = su->data()->findCollection ( pCollectionShortName, cID,
                                        &curClUniqueID ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( pClUniqueID )
      {
         if ( curClUniqueID != *pClUniqueID )
         {
            if ( UTIL_IS_VALID_CLUNIQUEID( curClUniqueID ) &&
                 UTIL_IS_VALID_CLUNIQUEID( *pClUniqueID ) )
            {
               rc = SDB_DMS_REMAIN ;
            }
            else
            {
               rc = SDB_DMS_UNIQUEID_CONFLICT ;
            }
            PD_LOG ( PDERROR, "Collection[%s]'s cl unique id error, "
                     "expect: %llu, actual: %llu, rc: %d",
                     pCollection, *pClUniqueID, curClUniqueID, rc ) ;
            goto error ;
         }
      }

   done :
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( NULL != pCurClUniqueID )
      {
         *pCurClUniqueID = curClUniqueID ;
      }
      PD_TRACE_EXITRC ( SDB_RTNTESTCLCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPOPCOMMAND, "rtnPopCommand" )
   INT32 rtnPopCommand( const CHAR *pCollectionName, INT64 value,
                        pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                        INT8 direction, BOOLEAN byNumber )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNPOPCOMMAND ) ;

      SDB_ASSERT( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT( cb, "eduCB can't be NULL" ) ;
      SDB_ASSERT( dmsCB, "dmsCB can't be NULL" ) ;

      BOOLEAN writable = FALSE ;
      dmsStorageUnit *su = NULL ;
      const CHAR *clShortName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsMBContext *mbContext = NULL ;
      dmsRecordID recordID ;
      rtnExtDataHandler *extHandler = NULL ;
      LOCK_HANDLE lockHandle = RTN_INVALID_LOCK_HANDLE ;

      if ( value < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Target value for pop is invalid[%lld], rc: %d",
                 value, rc ) ;
         goto error ;
      }

      // Check writable before su lock
      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock( pCollectionName, dmsCB, &su,
                                            &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   pCollectionName, rc ) ;

      // pop can only be done on capped collections.
      if ( DMS_STORAGE_CAPPED != su->type() )
      {
         PD_LOG( PDERROR, "pop can only be used on capped collection" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      // The lock here is to avoid concurrency issue described in
      // SEQUOIADBMAINSTREAM-4475. The cause of the issue is that the pop
      // operation may happen between the data change of the capped collection
      // and the replica log written of the operation on the original
      // collection.
      if ( ossStrncasecmp( clShortName, SYS_PREFIX,
                           ossStrlen( SYS_PREFIX ) ) == 0
           && ossStrncasecmp( pCollectionName, clShortName,
                              ossStrlen(clShortName) ) == 0 )
      {
         extHandler = rtnGetExtDataHandler() ;
         rc = extHandler->acquireLock( clShortName, EXCLUSIVE, lockHandle ) ;
         PD_RC_CHECK( rc, PDERROR, "Acquire external lock for collection[%s] "
                      "failed[%d]", pCollectionName, rc ) ;
      }

      rc = su->data()->getMBContext( &mbContext, clShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                   "rc: %d", pCollectionName, rc ) ;

      if ( RTN_INVALID_LOCK_HANDLE != lockHandle )
      {
         extHandler->releaseLock( lockHandle ) ;
         lockHandle = RTN_INVALID_LOCK_HANDLE ;
      }

      if ( NULL != cb )
      {
         cb->registerMonCRUDCB( &( mbContext->mbStat()->_crudCB ) ) ;
      }

      rc = su->data()->popRecord( mbContext, value, cb, dpsCB, direction,
                                  byNumber ) ;
      PD_RC_CHECK( rc, PDERROR, "Pop record failed, rc: %d", rc ) ;

   done:
      if ( RTN_INVALID_LOCK_HANDLE != lockHandle )
      {
         extHandler->releaseLock( lockHandle ) ;
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
         dmsCB->suUnlock( suID ) ;
      }

      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNPOPCOMMAND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCHGUID, "rtnChangeUniqueID" )
   INT32 rtnChangeUniqueID( const CHAR* csName,
                            utilCSUniqueID csUniqueID,
                            const BSONObj& clInfoObj,
                            pmdEDUCB* cb,
                            SDB_DMSCB* dmsCB, SDB_DPSCB* dpsCB )
   {
      PD_TRACE_ENTRY( SDB_RTNCHGUID ) ;
      INT32 rc = SDB_OK ;
      BOOLEAN writable = FALSE ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->changeUniqueID( csName, csUniqueID,
                                  clInfoObj, TRUE, NULL, FALSE,
                                  cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to change unique id, rc: %d", rc ) ;

   done :
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCHGUID, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}

