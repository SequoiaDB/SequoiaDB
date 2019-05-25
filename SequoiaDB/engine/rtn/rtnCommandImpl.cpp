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
#include "rtn.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dmsStorageUnit.hpp"
#include "mthSelector.hpp"
#include "monDump.hpp"
#include "msgDef.h"
#include "msgMessage.hpp"
#include "ixmExtent.hpp"
#include "rtnInternalSorting.hpp"
#include "utilList.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnExtDataHandler.hpp"

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

      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }
      if ( !options.isQueryEmpty() )
      {
         rtnContextBase *pContextBase = NULL ;
         SINT64 queryContextID = -1 ;

         BSONObj dummy ;
         rtnQueryOptions copiedOptions( options ) ;
         copiedOptions.setSelector( dummy ) ;
         copiedOptions.setOrderBy( dummy ) ;
         copiedOptions.setSkip( 0 ) ;
         copiedOptions.setLimit( -1 ) ;

         rc = rtnQuery ( copiedOptions, cb, dmsCB, rtnCB, queryContextID,
                         &pContextBase ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
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
            pContextBase->enableCountMode() ;
            rtnContextBuf buffObj ;

            while ( TRUE )
            {
               rc = rtnGetMore ( queryContextID, -1, buffObj, cb, rtnCB ) ;
               if ( rc )
               {
                  if ( SDB_DMS_EOC == rc )
                  {
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
                  totalCount += buffObj.recordNum() ;
               }
            }
         }
      }
      else
      {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETINDEXES, "rtnGetIndexes" )
   static INT32 rtnGetIndexes ( const CHAR *pCollection,
                                SDB_DMSCB *dmsCB,
                                rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNGETINDEXES ) ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      MON_IDX_LIST resultIndexes ;
      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }
      rc = su->getIndexes ( pCollectionShortName, resultIndexes ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get indexes %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      rc = monDumpIndexes ( resultIndexes, context ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to dump indexes %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

   done :
      resultIndexes.clear() ;
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNGETINDEXES, rc ) ;
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
      std::vector< dmsExtentID > extentList ;

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

   static UINT32 _rtnIndexKeyNodeCount( dmsExtentID extentID,
                                        dmsStorageUnit *su,
                                        UINT32 deep )
   {
      UINT32 count = 0 ;

      if ( 0 == deep || DMS_INVALID_EXTENT == extentID )
      {
         return count ;
      }

      ixmExtent extent( extentID, su->index() ) ;

      if ( 1 == deep )
      {
         count = extent.getNumKeyNode() ;
      }
      else
      {
         dmsExtentID childID = DMS_INVALID_EXTENT ;
         for ( UINT16 i = 0 ; i <= extent.getNumKeyNode() ; ++i )
         {
            childID = extent.getChildExtentID( i ) ;
            if ( DMS_INVALID_EXTENT != childID )
            {
               count += _rtnIndexKeyNodeCount( childID, su, deep - 1 ) ;
            }
         }
      }

      return count ;
   }

   static INT32 _rtnIndexKeyNodeInfo ( dmsExtentID rootExtentID,
                                       dmsStorageUnit * su,
                                       pmdEDUCB * cb,
                                       UINT32 sampleRecords,
                                       UINT64 totalRecords,
                                       BOOLEAN fullScan,
                                       UINT32 & targetLevel,
                                       UINT32 & levelCount,
                                       UINT32 & extentCount,
                                       UINT32 & targetKeyCount )
   {
      INT32 rc = SDB_OK ;

      _utilList< dmsExtentID, 32 > extentIDStack ;
      UINT32 targetLevelKeyCount = 0 ;
      UINT64 curLevelKeyCount = 0, curLevelExtCount = 0,
             nextLevelExtCount = 0 ;
      UINT32 maxExtKeyCount = 1 ;

      BOOLEAN foundTargetLevel = FALSE ;

      extentIDStack.push_back( rootExtentID ) ;
      curLevelExtCount = 1 ;

      targetLevel = 1 ;
      levelCount = 1 ;
      extentCount = 1 ;

      while ( curLevelExtCount > 0 )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         for ( UINT32 extIdx = 0 ; extIdx < curLevelExtCount ; extIdx ++ )
         {
            dmsExtentID curExtentID = extentIDStack.front() ;
            ixmExtent extent( curExtentID, su->index() ) ;

            extentIDStack.pop_front() ;

            for ( UINT16 i = 0 ; i <= extent.getNumKeyNode() ; ++i )
            {
               dmsExtentID childID = extent.getChildExtentID( i ) ;
               if ( DMS_INVALID_EXTENT != childID )
               {
                  extentIDStack.push_back( childID ) ;
                  extentCount ++ ;
               }
            }
            curLevelKeyCount += extent.getNumKeyNode() ;
            if ( extent.getNumKeyNode() > maxExtKeyCount )
            {
               maxExtKeyCount = extent.getNumKeyNode() ;
            }
         }

         nextLevelExtCount = extentIDStack.size() ;

         if ( !foundTargetLevel &&
               ( curLevelKeyCount > sampleRecords ||
                 0 == nextLevelExtCount ) )
         {
            targetLevelKeyCount = curLevelKeyCount ;
            foundTargetLevel = TRUE ;
            targetLevel = levelCount ;
         }

         if ( ( foundTargetLevel && levelCount > 1 && !fullScan ) ||
              0 == nextLevelExtCount )
         {
            break ;
         }

         curLevelExtCount = extentIDStack.size() ;
         levelCount ++ ;
      }

      if ( 0 != nextLevelExtCount )
      {
         UINT32 avgExtKeyCount = (UINT32)ceil( (double)curLevelKeyCount /
                                               (double)curLevelExtCount ) ;

         UINT32 levelExtCount = nextLevelExtCount ;
         UINT32 levelKeyCount = nextLevelExtCount * maxExtKeyCount ;
         levelCount ++ ;

         while ( levelKeyCount < totalRecords )
         {
            levelExtCount *= ( avgExtKeyCount + 1 ) ;
            levelKeyCount = levelExtCount * maxExtKeyCount ;
            extentCount += levelExtCount ;
            levelCount ++ ;
         }
      }

      targetKeyCount = targetLevelKeyCount ;

   done :
      return rc ;
   error :
      goto done ;
   }

   static const CHAR* _rtnIndexKeyData( dmsExtentID extentID,
                                        dmsStorageUnit *su,
                                        UINT32 deep,
                                        UINT32 index,
                                        dmsRecordID &rid )
   {
      if ( 0 == deep || DMS_INVALID_EXTENT == extentID )
      {
         return NULL ;
      }

      ixmExtent extent( extentID, su->index() ) ;

      if ( 1 == deep )
      {
         const ixmKeyNode *keyNode = extent.getKeyNode( index ) ;
         if ( !keyNode || DMS_INVALID_EXTENT == keyNode->_left )
         {
            return NULL ;
         }
         rid = keyNode->_rid ;
         rid._offset &= ~1 ;
         return extent.getKeyData( index ) ;
      }
      else
      {
         dmsExtentID childID = DMS_INVALID_EXTENT ;
         UINT32 count = 0 ;
         for ( UINT16 i = 0 ; i <= extent.getNumKeyNode() ; ++i )
         {
            childID = extent.getChildExtentID( i ) ;
            if ( DMS_INVALID_EXTENT == childID )
            {
               continue ;
            }
            count = _rtnIndexKeyNodeCount( childID, su, deep - 1 ) ;
            if ( count <= index )
            {
               index -= count ;
            }
            else
            {
               return _rtnIndexKeyData( childID, su, deep - 1, index, rid ) ;
            }
         }
      }

      return NULL ;
   }

   static const CHAR* _rtnIndexGetKey ( dmsExtentID extentID,
                                        dmsStorageUnit *su,
                                        UINT32 deep,
                                        UINT32 index )
   {
      if ( 0 == deep || DMS_INVALID_EXTENT == extentID )
      {
         return NULL ;
      }

      ixmExtent extent( extentID, su->index() ) ;

      if ( 1 == deep )
      {
         const ixmKeyNode *keyNode = extent.getKeyNode( index ) ;
         if ( !keyNode )
         {
            return NULL ;
         }
         return extent.getKeyData( index ) ;
      }
      else
      {
         dmsExtentID childID = DMS_INVALID_EXTENT ;
         UINT32 count = 0 ;
         for ( UINT16 i = 0 ; i <= extent.getNumKeyNode() ; ++i )
         {
            childID = extent.getChildExtentID( i ) ;
            if ( DMS_INVALID_EXTENT == childID )
            {
               continue ;
            }
            count = _rtnIndexKeyNodeCount( childID, su, deep - 1 ) ;
            if ( count <= index )
            {
               index -= count ;
            }
            else
            {
               return _rtnIndexGetKey( childID, su, deep - 1, index ) ;
            }
         }
      }

      return NULL ;
   }

   INT32 rtnGetIndexSeps( optAccessPlanRuntime *planRuntime,
                          dmsStorageUnit *su,
                          dmsMBContext *mbContext, pmdEDUCB * cb,
                          vector < BSONObj > &idxBlocks,
                          std::vector< dmsRecordID > &idxRIDs )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( planRuntime, "planRuntime is invalid" ) ;

      idxBlocks.clear() ;
      idxRIDs.clear() ;

      SDB_ASSERT( IXSCAN == planRuntime->getScanType(),
                  "Scan type must be IXSCAN" ) ;

      ixmIndexCB indexCB( planRuntime->getIndexCBExtent(), su->index(), NULL ) ;

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock dms mb context[%s], rc: %d",
                      mbContext->toString().c_str(), rc ) ;
      }

      if ( !indexCB.isInitialized() )
      {
         PD_LOG ( PDERROR, "unable to get proper index control block" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( indexCB.getLogicalID() != planRuntime->getIndexLID() )
      {
         PD_LOG( PDERROR, "Index[extent id: %d] logical id[%d] is not "
                 "expected[%d]", planRuntime->getIndexCBExtent(),
                 indexCB.getLogicalID(), planRuntime->getIndexLID() ) ;
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }

      {
         rtnPredicateList * predList = planRuntime->getPredList() ;
         SDB_ASSERT ( predList, "predList can't be NULL" ) ;

         BSONObj startObj = predList->startKey() ;
         BSONObj endObj = predList->endKey() ;
         BSONObj prevObj ;
         dmsRecordID prevRid ;
         if ( planRuntime->getDirection() < 0 )
         {
            startObj = endObj ;
            endObj = predList->startKey() ;
         }
         Ordering order = Ordering::make( indexCB.keyPattern() ) ;
         dmsExtentID rootID = indexCB.getRoot() ;
         const CHAR *keyData = NULL ;
         BOOLEAN findPos = FALSE ;
         BSONObj key ;
         dmsRecordID rid ;
         UINT32 segmentCount  = 1 ;

         if ( DMS_INVALID_EXTENT != mbContext->mb()->_mbExExtentID )
         {
            dmsExtRW extRW ;
            const dmsMBEx *mbEx  = NULL ;

            extRW = su->data()->extent2RW( mbContext->mb()->_mbExExtentID,
                                           -1 ) ;
            extRW.setNothrow( TRUE ) ;
            mbEx = extRW.readPtr<dmsMBEx>() ;

            if ( mbEx && mbEx->_header._usedSegNum > 0 )
            {
               segmentCount = mbEx->_header._usedSegNum ;
            }
         }

         UINT32 deep = 1 ;
         UINT32 mod  = 0 ;
         UINT32 step = 1 ;
         UINT32 index = 0 ;
         UINT32 keyNodeCount = _rtnIndexKeyNodeCount( rootID, su, deep ) ;
         while ( keyNodeCount < segmentCount && deep < 3 )
         {
            ++deep ;
            keyNodeCount = _rtnIndexKeyNodeCount( rootID, su, deep ) ;
         }

         if ( keyNodeCount > 0 && keyNodeCount < segmentCount )
         {
            segmentCount = keyNodeCount ;
         }

         step = keyNodeCount / segmentCount ;
         mod  = keyNodeCount % segmentCount ;

         idxBlocks.push_back( rtnUniqueKeyNameObj( startObj ) ) ;
         idxRIDs.push_back( dmsRecordID() ) ;
         prevObj = startObj ;
         prevRid.resetMin() ;

         while ( index < keyNodeCount )
         {
            keyData = _rtnIndexKeyData( rootID, su, deep, index, rid ) ;
            index += step ;

            if ( mod > 0 )
            {
               ++index ;
               --mod ;
            }

            if ( NULL == keyData )
            {
               continue ;
            }
            key = ixmKey( keyData ).toBson() ;

            if ( !findPos )
            {
               if ( key.woCompare( startObj, order, false ) >= 0 )
               {
                  findPos = TRUE ;
               }
            }
            else
            {
               if ( key.woCompare( endObj, order, false ) > 0 )
               {
                  break ;
               }
               else if ( 0 == key.woCompare( prevObj, order, false ) )
               {
                  if ( rid == prevRid )
                  {
                     continue ;
                  }
               }

               idxBlocks.push_back( rtnUniqueKeyNameObj( key ) ) ;
               idxRIDs.push_back( rid ) ;
               prevObj = key ;
               prevRid = rid ;
            }
         }

         idxBlocks.push_back( rtnUniqueKeyNameObj( endObj ) ) ;
         idxRIDs.push_back( dmsRecordID() ) ;
      }

      if ( idxBlocks.size() != idxRIDs.size() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "block array size not the same with rid array" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnGetIndexSamples ( _dmsStorageUnit *su,
                              ixmIndexCB *indexCB,
                              _pmdEDUCB * cb,
                              UINT32 sampleRecords,
                              UINT64 totalRecords,
                              BOOLEAN fullScan,
                              _rtnInternalSorting &sorter,
                              UINT32 &levels, UINT32 &pages )
   {
      INT32 rc = SDB_OK ;

      dmsExtentID rootID = indexCB->getRoot() ;
      const CHAR *keyData = NULL ;
      BSONObj key ;
      BSONObj dummy ;

      UINT32 targetLevel = 1 ;
      UINT32 sampleMod  = 0 ;
      UINT32 sampleStep = 1 ;
      UINT32 sampleIndex = 0 ;
      UINT32 keyNodeCount = 0 ;

      rc = _rtnIndexKeyNodeInfo( rootID, su, cb, sampleRecords, totalRecords,
                                 fullScan, targetLevel, levels, pages,
                                 keyNodeCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get index node info, rc: %d", rc ) ;

      PD_LOG( PDDEBUG, "Estimate index [%s] info levels %u pages %u",
              indexCB->getName(), levels, pages ) ;

      if ( keyNodeCount > 0 && keyNodeCount < sampleRecords )
      {
         sampleRecords = keyNodeCount ;
      }

      sampleStep = keyNodeCount / sampleRecords ;
      sampleMod  = keyNodeCount % sampleRecords ;

      sorter.clearBuf() ;

      while ( sampleIndex < keyNodeCount )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         keyData = _rtnIndexGetKey( rootID, su, targetLevel, sampleIndex ) ;
         sampleIndex += sampleStep ;

         if ( sampleMod > 0 )
         {
            ++sampleIndex ;
            --sampleMod ;
         }

         if ( NULL == keyData )
         {
            continue ;
         }
         key = ixmKey( keyData ).toBson() ;
         rc = sorter.push( key, dummy.objdata(), dummy.objsize(), NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push item into sorter, "
                      "rc: %d", rc ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   static INT32 rtnGetIndexblocks( dmsStorageUnit *su ,
                                   optAccessPlanRuntime *planRuntime,
                                   pmdEDUCB * cb,
                                   rtnContextDump *context,
                                   dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( planRuntime, "planRuntime is invalid" ) ;

      std::vector < BSONObj > idxBlocks ;
      std::vector < dmsRecordID > idxRIDs ;

      rc = rtnGetIndexSeps( planRuntime, su, mbContext, cb, idxBlocks, idxRIDs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get idnex seps, rc: %d", rc ) ;

      {
         ixmIndexCB indexCB( planRuntime->getIndexCBExtent(), su->index(), NULL ) ;
         rc = monDumpIndexblocks( idxBlocks, idxRIDs, indexCB.getName(),
                                  indexCB.getLogicalID(),
                                  planRuntime->getDirection(),
                                  context ) ;
         PD_RC_CHECK( rc, PDERROR, "Dump indexblocks failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnGetQueryMeta( const rtnQueryOptions & options,
                          SDB_DMSCB *dmsCB,
                          pmdEDUCB *cb,
                          rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      dmsStorageUnit *su = NULL ;
      const CHAR * pCollectionName = options.getCLFullName() ;
      const CHAR *pCollectionShortName = NULL ;
      dmsMBContext *mbContext = NULL ;
      optAccessPlanRuntime planRuntime ;
      optAccessPlanManager *apm = NULL ;

      ossTick startTime, endTime ;
      monContextCB monCtxCB ;

      BSONObj dummy ;
      rtnQueryOptions copiedOptions( options ) ;
      copiedOptions.setSelector( dummy ) ;
      copiedOptions.setSkip( 0 ) ;
      copiedOptions.setFlag( 0 ) ;
      copiedOptions.setLimit( -1 ) ;

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s",
                   pCollectionName ) ;

      rc = su->data()->getMBContext( &mbContext, pCollectionShortName, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      apm = rtnCB->getAPM() ;
      SDB_ASSERT ( apm, "apm shouldn't be NULL" ) ;

      rc = apm->getAccessPlan( copiedOptions, FALSE, su, mbContext, planRuntime ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get access plan for %s, "
                   "context %lld, rc: %d", pCollectionName,
                   context->contextID(), rc ) ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         monCtxCB.recordStartTimestamp() ;
      }

      startTime = krcb->getCurTime() ;

      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection[%s], rc: %d",
                   pCollectionName, rc ) ;

      if ( TBSCAN == planRuntime.getScanType() )
      {
         rc = _rtnGetDatablocks( su, cb, context, mbContext,
                                 pCollectionShortName ) ;
      }
      else if ( IXSCAN == planRuntime.getScanType() )
      {
         rc = rtnGetIndexblocks( su, &planRuntime, cb, context, mbContext ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Collection access plan scan type error: %d",
                 planRuntime.getScanType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to get collection[%s] query meta, "
                   "rc: %d", pCollectionName, rc ) ;

      endTime = krcb->getCurTime() ;

      monCtxCB.monQueryTimeInc( startTime, endTime ) ;
      planRuntime.setQueryActivity( MON_SELECT, monCtxCB, copiedOptions, TRUE ) ;

   done:
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      planRuntime.releasePlan() ;
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
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
      rtnContextDump *context = NULL ;

      rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                               contextID, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create new context, rc: %d", rc ) ;
         goto error ;
      }
      rc = context->open( options.getSelector(),
                          options.getQuery(),
                          options.isOrderByEmpty() ? options.getLimit() : -1,
                          options.isOrderByEmpty() ? options.getSkip() : 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      switch ( command )
      {
         case CMD_GET_INDEXES :
            rc = rtnGetIndexes ( pCollectionName, dmsCB, context ) ;
            break ;
         case CMD_GET_COUNT :
            rc = rtnGetCount ( options, dmsCB, cb, rtnCB, context ) ;
            break ;
         case CMD_GET_DATABLOCKS :
            rc = rtnGetDatablocks( pCollectionName, dmsCB, cb, context ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            break ;
      }
      PD_RC_CHECK( rc, PDERROR, "Dump collection[%s] info[command:%d] failed, "
                   "rc: %d", pCollectionName, command, rc ) ;

      if ( !options.isOrderByEmpty() )
      {
         rc = rtnSort( (rtnContext**)&context, options.getOrderBy(), cb,
                       options.getSkip(), options.getLimit(), contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNGETCOMMANDENTRY, rc ) ;
      return rc ;
   error :
      rtnCB->contextDelete ( contextID, cb ) ;
      contextID = -1 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATECSCOMMAND, "rtnCreateCollectionSpaceCommand" )
   INT32 rtnCreateCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                           pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                           SDB_DPSCB *dpsCB, INT32 pageSize,
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

      UINT32 length = ossStrlen ( pCollectionSpace ) ;
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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

      rc = dmsCB->nameToSUAndLock ( pCollectionSpace, suID, &su ) ;
      if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         su = NULL ;
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

      su = SDB_OSS_NEW dmsStorageUnit ( pCollectionSpace, 1,
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
                      TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create collection space %s at %s, rc: %d",
                  pCollectionSpace, pmdGetOptionCB()->getDbPath(),
                  rc ) ;
         goto error ;
      }
      su->setSyncConfig( optCB->getSyncInterval(),
                         optCB->getSyncRecordNum(),
                         optCB->getSyncDirtyRatio() ) ;
      su->setSyncDeep( optCB->isSyncDeep() ) ;

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
         su->remove() ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Create collectionspace[%s] succeed, PageSize:%u, "
              "LobPageSize:%u", pCollectionSpace, pageSize, lobPageSize ) ;

   done :
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
                                      UTIL_COMPRESSOR_TYPE compressorType,
                                      INT32 flags,
                                      BOOLEAN sysCall,
                                      const BSONObj *extOptions )
   {
      BSONObj obj ;
      return rtnCreateCollectionCommand ( pCollection,
                                          obj, attributes,
                                          cb, dmsCB, dpsCB,
                                          compressorType,
                                          flags, sysCall, extOptions ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATECLCOMMAND, "rtnCreateCollectionCommand" )
   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      const BSONObj &shardingKey,
                                      UINT32 attributes,
                                      _pmdEDUCB * cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      UTIL_COMPRESSOR_TYPE compType,
                                      INT32 flags, BOOLEAN sysCall,
                                      const BSONObj *extOptions )
   {
      INT32 rc              = SDB_OK ;
      INT32 rcTmp           = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCREATECLCOMMAND ) ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su    = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      BOOLEAN writable      = FALSE ;
      UINT16 collectionID = DMS_INVALID_MBID ;
      UINT32 logicalID = DMS_INVALID_CLID ;

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
         temp [ pCollectionShortName - pCollection - 1 ] = '\0' ;
         if ( SDB_OK == rtnCreateCollectionSpaceCommand ( temp, cb,
                                                          dmsCB, dpsCB,
                                                          DMS_PAGE_SIZE_DFT,
                                                          DMS_DEFAULT_LOB_PAGE_SZ,
                                                          type,
                                                          sysCall ) )
         {
            rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB,
                                                   &su, &pCollectionShortName,
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

      rc = su->data()->addCollection ( pCollectionShortName, &collectionID,
                                       attributes, cb, dpsCB, 0, sysCall,
                                       compType, &logicalID, extOptions ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create collection %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }
      if ( !shardingKey.isEmpty() )
      {
         try
         {
            BSONObj shardKeyObj = BSON ( "key"<<shardingKey<<"name"<<
                                         IXM_SHARD_KEY_NAME<<"v"<<0 ) ;
            rc = rtnCreateIndexCommand ( pCollection, shardKeyObj,
                                         cb, dmsCB, dpsCB, TRUE ) ;
            if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
            {
               rc = SDB_OK ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to create sharding key for "
                        "collection %s, rc = %d", pCollection, rc ) ;
               goto error_rollback ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to build sharding key: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
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

      {
         CHAR attrStr[ 64 + 1 ] = { 0 } ;
         mbAttr2String( attributes, attrStr, sizeof( attrStr ) - 1 ) ;
         if ( extOptions && !extOptions->isEmpty())
         {
            PD_LOG( PDEVENT, "Create collection[%s] succeed, ShardingKey:%s, "
                    "Attr:%s(0x%08x), CompressType:%s(%d), External options:%s",
                    pCollection, shardingKey.toString().c_str(), attrStr,
                    attributes, utilCompressType2String( (UINT8)compType ),
                    compType, extOptions->toString().c_str() ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Create collection[%s] succeed, ShardingKey:%s, "
                    "Attr:%s(0x%08x), CompressType:%s(%d)", pCollection,
                    shardingKey.toString().c_str(), attrStr, attributes,
                    utilCompressType2String( (UINT8)compType ),
                    compType ) ;
         }
      }

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
      rcTmp = rtnDropCollectionCommand ( pCollection, cb, dmsCB, dpsCB ) ;
      if ( rcTmp )
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
                                 INT32 sortBufferSize )
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

      if ( DMS_STORAGE_CAPPED == su->type() )
      {
         PD_LOG( PDERROR, "Index is not support on capped collection" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      rc = su->createIndex ( pCollectionShortName, indexObj,
                             cb, dpsCB, isSys, NULL, sortBufferSize ) ;
      if ( rc )
      {
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPINDEXCOMMAND, "rtnDropIndexCommand" )
   INT32 rtnDropIndexCommand ( const CHAR *pCollection,
                               const BSONElement &identifier,
                               pmdEDUCB *cb,
                               SDB_DMSCB *dmsCB,
                               SDB_DPSCB *dpsCB,
                               BOOLEAN sysCall )
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

      if ( identifier.type() == jstOID )
      {
         identifier.Val(oid) ;
         rc = su->dropIndex ( pCollectionShortName, oid, cb, dpsCB, sysCall ) ;
      }
      else if ( identifier.type() == String )
      {
         rc = su->dropIndex ( pCollectionShortName, identifier.valuestr(),
                              cb, dpsCB, sysCall ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNDROPCSCOMMAND, "rtnDropCollectionSpaceCommand" )
   INT32 rtnDropCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         _pmdEDUCB *cb,
                                         SDB_DMSCB *dmsCB,
                                         SDB_DPSCB *dpsCB,
                                         BOOLEAN   sysCall )
   {
      PD_TRACE_ENTRY ( SDB_RTNDROPCSCOMMAND ) ;
      INT32 rc = rtnDelCollectionSpaceCommand( pCollectionSpace, cb,
                                               dmsCB, dpsCB, sysCall,
                                               TRUE ) ;
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT, "Drop collectionspace[%s] succeed",
                 pCollectionSpace ) ;
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

      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      UINT32 length = ossStrlen ( pCollectionSpace ) ;
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Invalid length for collectionspace: %s, rc: %d",
                  pCollectionSpace, rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( pCollectionSpace, cb ) ;
      }

      dmsCB->aquireCSMutex( pCollectionSpace ) ;
      rc = dmsCB->dropCollectionSpaceP1( pCollectionSpace, cb, dpsCB ) ;
      dmsCB->releaseCSMutex( pCollectionSpace ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collectionspace %s, "
                   "rc: %d", pCollectionSpace, rc ) ;

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
                                    BOOLEAN   sysCall )
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
      rc = dmsCB->dropCollectionSpaceP2( pCollectionSpace, cb, dpsCB ) ;
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
                                    SDB_DPSCB *dpsCB )
   {
      INT32 rc                            = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNDROPCLCOMMAND ) ;
      dmsStorageUnitID suID               = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su                  = NULL ;
      const CHAR *pCollectionShortName    = NULL ;
      BOOLEAN writable                    = FALSE ;

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

      rc = su->data()->dropCollection ( pCollectionShortName, cb, dpsCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to drop collection %s, rc: %d",
                  pCollection, rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Drop collection[%s] succeed", pCollection ) ;

   done :
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRUNCCLCOMMAND, "rtnTruncCollectionCommand" )
   INT32 rtnTruncCollectionCommand( const CHAR *pCollection, pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTRUNCCLCOMMAND ) ;
      dmsStorageUnitID suID            = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su               = NULL ;
      const CHAR *pCollectionShortName = NULL ;
      BOOLEAN writable                 = FALSE ;
      dmsMBContext *context            = NULL ;
      dmsMB *mb                        = NULL ;

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

      rc = su->data()->truncateCollection( pCollectionShortName, cb, dpsCB ) ;
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
      rc = su->data()->getMBContext( &context, pCollectionShortName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get mb context of collection %s, rc: %d",
                   pCollection, rc ) ;
      mb = context->mb() ;

      if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == mb->_compressorType &&
           DMS_INVALID_EXTENT == mb->_dictExtentID )
      {
         dmsCB->pushDictJob( dmsDictJob( suID, su->LogicalCSID(),
                             context->mbID(), context->clLID() ) ) ;
      }

      PD_LOG( PDEVENT, "Truncate collection[%s] succeed",
              pCollection ) ;

   done :
      if ( context )
      {
         su->data()->releaseMBContext( context ) ;
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
                                         SDB_DMSCB *dmsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTESTCSCOMMAND ) ;
      SDB_ASSERT ( pCollectionSpace, "collection space can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      rc = dmsCB->nameToSUAndLock ( pCollectionSpace, suID, &su ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }
   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
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
                                    SDB_DMSCB *dmsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNTESTCLCOMMAND ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      SDB_ASSERT ( pCollection, "collection can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dms control block can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      const CHAR *pCollectionShortName = NULL ;
      UINT16 cID ;
      rc = rtnResolveCollectionNameAndLock ( pCollection, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = su->data()->findCollection ( pCollectionShortName, cID ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNTESTCLCOMMAND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPOPCOMMAND, "rtnPopCommand" )
   INT32 rtnPopCommand( const CHAR *pCollectionName, INT64 logicalID,
                        pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                        INT16 w, INT8 direction )
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

      if ( logicalID < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "LogicalID for pop is invalid[%lld], rc: %d",
                 logicalID, rc ) ;
         goto error ;
      }

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock( pCollectionName, dmsCB, &su,
                                            &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   pCollectionName, rc ) ;

      if ( DMS_STORAGE_CAPPED != su->type() )
      {
         PD_LOG( PDERROR, "pop can only be used on capped collection" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      rc = su->data()->getMBContext( &mbContext, clShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                   "rc: %d", pCollectionName, rc ) ;

      rc = su->data()->popRecord( mbContext, logicalID, cb, dpsCB, direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Pop record failed, rc: %d", rc ) ;

   done:
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
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_RTNPOPCOMMAND, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

