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

   Source File Name = clsCleanupJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/03/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsCleanupJob.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtn.hpp"
#include "rtnLobFetcher.hpp"
#include "rtnLob.hpp"

using namespace bson ;

namespace engine
{

   _clsCleanupJob::_clsCleanupJob ( const std::string &clFullName,
                                    utilCLUniqueID clUniqueID,
                                    const BSONObj &splitKeyObj,
                                    const BSONObj &splitEndKeyObj,
                                    BOOLEAN hasShardingIndex,
                                    BOOLEAN isHashSharding,
                                    SDB_DPSCB *dpsCB )
   {
      _clFullName = clFullName ;
      _splitKeyObj = splitKeyObj.getOwned() ;
      _splitEndKeyObj = splitEndKeyObj.getOwned() ;
      _hasShardingIndex = hasShardingIndex ;
      _isHashSharding = isHashSharding ;
      _clUniqueID = clUniqueID ;

      _dpsCB = dpsCB ;
      _dmsCB = pmdGetKRCB()->getDMSCB() ;

      _makeName() ;
   }

   _clsCleanupJob::~_clsCleanupJob ()
   {
   }

   void _clsCleanupJob::_makeName()
   {
      _name = "Cleanup[" ;
      _name += _clFullName ;
      _name += "]-" ;

      if ( CLS_CLEANUP_BY_RANGE == _cleanupType() )
      {
         _name += "Range[" ;
         _name += _splitKeyObj.toString() ;
         _name += "," ;
         _name += _splitEndKeyObj.toString() ;
         _name += "]" ;
      }
      else if ( CLS_CLEANUP_BY_CATAINFO == _cleanupType() )
      {
         _name += "CataInfo" ;
      }
      else
      {
         _name += "ShardingIndex[" ;
         _name += _splitKeyObj.toString() ;
         _name += "]" ;
      }
   }

   void _clsCleanupJob::_onAttach()
   {
   }

   CLS_CLEANUP_TYPE _clsCleanupJob::_cleanupType () const
   {
      if ( _splitKeyObj.isEmpty() && _splitEndKeyObj.isEmpty() )
      {
         return CLS_CLEANUP_BY_CATAINFO ;
      }
      else if ( _hasShardingIndex && !_isHashSharding &&
                _splitEndKeyObj.isEmpty() )
      {
         return CLS_CLEANUP_BY_SHARDINGINDEX ;
      }
      else
      {
         return CLS_CLEANUP_BY_RANGE ;
      }
   }

   RTN_JOB_TYPE _clsCleanupJob::type() const
   {
      return RTN_JOB_CLEANUP ;
   }

   const CHAR* _clsCleanupJob::name () const
   {
      return _name.c_str() ;
   }

   BOOLEAN _clsCleanupJob::muteXOn( const _rtnBaseJob * pOther )
   {
      BOOLEAN mutex = FALSE ;
      _clsCleanupJob *pCleanJob = NULL ;

      if ( type() != pOther->type() )
      {
         goto done ;
      }
      pCleanJob = ( _clsCleanupJob* )pOther ;

      if ( _clFullName == pCleanJob->_clFullName &&
           0 == _splitKeyObj.woCompare( pCleanJob->_splitKeyObj, BSONObj(),
                                        false ) &&
           0 == _splitEndKeyObj.woCompare( pCleanJob->_splitEndKeyObj,
                                           BSONObj(), false ) )
      {
         mutex = TRUE ;
      }

   done:
      return mutex ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCLNJOB_DOIT, "_clsCleanupJob::doit" )
   INT32 _clsCleanupJob::doit ()
   {
      // need to update catalog
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCLNJOB_DOIT );
      clsTaskMgr *pTaskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
      shardCB *pShardCB = sdbGetShardCB() ;
      catAgent *catAgent = pShardCB->getCataAgent() ;
      _clsCatalogSet* catSet = NULL ;
      INT32 w = 1 ;
      BOOLEAN dropCollection = FALSE ;
      BOOLEAN needClean = TRUE ;

      if ( _dpsCB )
      {
         eduCB()->writingDB( TRUE ) ;
      }

      while ( TRUE )
      {
         if ( eduCB()->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = pShardCB->syncUpdateCatalog( _clFullName.c_str(), OSS_ONE_SEC ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            dropCollection = TRUE ;
            PD_LOG( PDDEBUG, "%s: Could not find collection [%s] in catalog, "
                    "drop collection", name(), _clFullName.c_str() ) ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "%s: Failed to find collection [%s] in catalog, "
                    "rc: %d, retry", name(), _clFullName.c_str(), rc ) ;
            continue ;
         }

         catAgent->lock_r() ;
         catSet = catAgent->collectionSet( _clFullName.c_str() ) ;
         if ( catSet )
         {
            if ( UTIL_UNIQUEID_NULL == _clUniqueID ||
                 _clUniqueID == catSet->clUniqueID() )
            {
               // same collection ( not re-create )
               // w = catSet->getW () ;
               dropCollection = ( 0 == catSet->groupCount() ) ? TRUE : FALSE ;
               PD_LOG( PDDEBUG, "%s: Found catalog cache for collection [%s], "
                       "need drop collection: %s", name(), _clFullName.c_str(),
                       dropCollection ? "TRUE" : "FALSE" ) ;
            }
            else if ( 0 == catSet->groupCount() )
            {
               // re-create on other group: drop collection
               dropCollection = TRUE ;
               PD_LOG( PDDEBUG, "%s: Found catalog cache for collection [%s] "
                       "on other group, expected %llu, found %llu, "
                       "drop collection", name(), _clFullName.c_str(),
                       _clUniqueID, catSet->clUniqueID() ) ;
            }
            else
            {
               // re-create on this group: do nothing
               // new one will drop the remained collection
               dropCollection = FALSE ;
               needClean = FALSE ;
               PD_LOG( PDDEBUG, "%s: Found catalog cache for collection [%s] "
                       "on this group, expected %llu, found %llu, "
                       "do nothing", name(), _clFullName.c_str(),
                       _clUniqueID, catSet->clUniqueID() ) ;
            }
         }
         else
         {
            catAgent->release_r() ;
            PD_LOG( PDDEBUG, "%s: no catalog cache for collection [%s] found, "
                    "retry", name(), _clFullName.c_str() ) ;
            continue ;
         }

         catAgent->release_r() ;
         break ;
      }

      // drop collection
      if ( dropCollection )
      {
         pTaskMgr->lockReg( SHARED ) ;
         if ( 0 == pTaskMgr->getRegCount( _clFullName, TRUE ) )
         {
            // delete the collection
            rc = rtnDropCollectionCommand( _clFullName.c_str(), eduCB(),
                                           _dmsCB, _dpsCB ) ;
            PD_LOG ( PDEVENT, "Job[%s] drop the collection[%s], rc:%d", name(),
                     _clFullName.c_str(), rc ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = SDB_OK ;
            }
            else if ( SDB_OK == rc || SDB_DMS_NOTEXIST == rc )
            {
               // drop empty collectionspace, ignore errors
               _dmsCB->dropEmptyCollectionSpace(
                        dmsGetCSNameFromFullName( _clFullName ).c_str(),
                        eduCB(), _dpsCB ) ;
               rc = SDB_OK ;
            }
            pTaskMgr->releaseReg( SHARED ) ;
            goto done ;
         }
         pTaskMgr->releaseReg( SHARED ) ;
      }

      if ( !needClean )
      {
         goto done ;
      }

      if ( CLS_CLEANUP_BY_SHARDINGINDEX == _cleanupType() )
      {
         rc = _cleanBySplitKeyObj ( w ) ;
      }
      else
      {
         rc = _cleanByTBSCan( w, _cleanupType() ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to clean data after split done:%d", rc ) ;
         goto error ;
      }

      rc = _cleanLobData( w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to clean lob data:%d", rc ) ;
      }

   done:
      eduCB()->writingDB( FALSE ) ;
      PD_TRACE_EXITRC ( SDB__CLSCLNJOB_DOIT, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCLNJOB__FILTERDEL, "_clsCleanupJob::_filterDel" )
   INT32 _clsCleanupJob::_filterDel( const dmsLobInfoOnPage &page,
                                     BOOLEAN &need2Remove )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSCLNJOB__FILTERDEL ) ;
      catAgent *catAgent = sdbGetShardCB()->getCataAgent() ;
      _clsCatalogSet *catSet = NULL ;
      UINT32 groupID = sdbGetShardCB()->nodeID().columns.groupID ;
      UINT32 belongTo = 0 ;
      BOOLEAN need2ReleaseR = FALSE ;

   retry:
      catAgent->lock_r() ;
      need2ReleaseR = TRUE ;
      catSet = catAgent->collectionSet( _clFullName.c_str() ) ;
      if ( NULL == catSet )
      {
         catAgent->release_r() ;
         need2ReleaseR = FALSE ;
         rc = sdbGetShardCB()->syncUpdateCatalog( _clFullName.c_str(),
                                                  OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            goto retry ;
         }
         else
         {
            if ( SDB_DMS_NOTEXIST != rc )
            {
               PD_LOG( PDERROR, "Failed to update catalog info of %s",
                       _clFullName.c_str() ) ;
            }
            goto error ;
         }
      }

      /// collection's unique id changed
      if ( UTIL_UNIQUEID_NULL != _clUniqueID &&
           _clUniqueID != catSet->clUniqueID() )
      {
         if ( 0 == catSet->groupCount() )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         else
         {
            rc = SDB_DMS_EOC ;
         }
         goto error ;
      }

      if ( CLS_CLEANUP_BY_CATAINFO == _cleanupType() )
      {
         rc = catSet->findGroupID( page._oid, page._sequence, belongTo ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get group id from cata set:%d", rc ) ;
            goto error ;
         }

         need2Remove = ( groupID != belongTo ) ;
         goto done ;
      }
      else if ( catSet->isHashSharding() && !_splitKeyObj.isEmpty()
                && _splitKeyObj.firstElement().isNumber() )
      {
         INT32 range = clsPartition( page._oid, page._sequence,
                                     catSet->getPartitionBit() ) ;
         need2Remove = _splitKeyObj.firstElement().Int() <= range && 
           ( _splitEndKeyObj.isEmpty() ||
             range < _splitEndKeyObj.firstElement().Int() ) ;
      }
      else
      {
         need2Remove = FALSE ;
      }
   done:
      if ( need2ReleaseR )
      {
         catAgent->release_r() ;
      }
      PD_TRACE_EXITRC( SDB__CLSCLNJOB__FILTERDEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCLNJOB__CLEANLOBDATA, "_clsCleanupJob::_cleanLobData" )
   INT32 _clsCleanupJob::_cleanLobData( INT32 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSCLNJOB__CLEANLOBDATA ) ;
      dmsLobInfoOnPage page ;
      rtnLobFetcher fetcher ;
      BOOLEAN need2Remove = FALSE ;

      if ( !_isHashSharding )
      {
         /// do not support non-hash sharding.
         goto done ;
      }
      else if ( CLS_CLEANUP_BY_SHARDINGINDEX == _cleanupType() )
      {
         PD_LOG( PDERROR, "we can not clean lob data when type is "
                 "SHARDINGINDEX " ) ;
         goto done ;
      }

      rc = fetcher.init( _clFullName.c_str(), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init fetcher:%d", rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         need2Remove = FALSE ;
         rc = fetcher.fetch( eduCB(), page ) ;
         if ( SDB_OK == rc )
         {
            rc = _filterDel( page, need2Remove ) ;
            if ( SDB_DMS_NOTEXIST == rc )
            {
               clsTaskMgr *pTaskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
               pTaskMgr->lockReg( SHARED ) ;
               if ( 0 == pTaskMgr->getRegCount( _clFullName, TRUE ) )
               {
                  // delete the collection
                  rc = rtnDropCollectionCommand( _clFullName.c_str(),
                                                 eduCB(), _dmsCB, _dpsCB ) ;
                  PD_LOG ( PDEVENT, "Job[%s] drop the collection[%s], rc:%d",
                           name(), _clFullName.c_str(), rc ) ;
                  if ( SDB_DMS_CS_NOTEXIST == rc )
                  {
                     rc = SDB_OK ;
                  }
                  else if ( SDB_OK == rc || SDB_DMS_NOTEXIST == rc )
                  {
                     // drop empty collectionspace, ignore errors
                     _dmsCB->dropEmptyCollectionSpace(
                              dmsGetCSNameFromFullName( _clFullName ).c_str(),
                              eduCB(), _dpsCB ) ;
                     rc = SDB_OK ;
                  }
                  pTaskMgr->releaseReg( SHARED ) ;
                  goto done ;
               }
               pTaskMgr->releaseReg( SHARED ) ;

               break ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Failed to filter lob:%d", rc ) ;
               goto error ;
            }

            if ( need2Remove)
            {
               rc = rtnRemoveLobPiece( _clFullName.c_str(),
                                       page._oid, page._sequence,
                                       eduCB(), w, _dpsCB ) ;
               if ( SDB_CLS_WAIT_SYNC_FAILED == rc )
               {
                  rc = SDB_OK ;
               }
               else if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to remove lob[%s][%d]",
                          ", rc:%d", page._oid.str().c_str(),
                          page._sequence, rc ) ;
                  goto error ;
               }
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to fetch lob:%d", rc ) ;
            goto done ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSCLNJOB__CLEANLOBDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCLNJOB__CLNBYTBSCAN, "_clsCleanupJob::_cleanByTBSCan" )
   INT32 _clsCleanupJob::_cleanByTBSCan ( INT32 w, CLS_CLEANUP_TYPE cleanType )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCLNJOB__CLNBYTBSCAN );
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObjBuilder builder ;
      builder.appendNull( "" ) ;
      BSONObj hint = builder.obj() ;
      CHAR fullName[DMS_COLLECTION_NAME_SZ +
                    DMS_COLLECTION_SPACE_NAME_SZ + 2] = {0} ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      UINT32 groupID = sdbGetShardCB()->nodeID().columns.groupID ;
      ossStrncpy ( fullName, _clFullName.c_str(), sizeof(fullName)-1 ) ;

      rtnContextBuf buffObj ;

      // TABSCAN, and delete not self record
      rc = rtnQuery( fullName, selector, matcher, orderBy, hint,
                     0, eduCB(), 0, -1, _dmsCB, rtnCB, contextID ) ;
      if ( SDB_DMS_EOC == rc ||
           SDB_DMS_CS_NOTEXIST == rc ||
           SDB_DMS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG ( PDWARNING, "Job[%s] open contex failed[rc:%d]", name(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         if ( eduCB()->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = rtnGetMore( contextID, -1, buffObj, eduCB(), rtnCB ) ;
         if ( SDB_DMS_EOC == rc ||
              SDB_RTN_CONTEXT_NOTEXIST == rc ||
              SDB_DMS_NOTEXIST == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            goto done ;
         }
         else if ( SDB_OK != rc )
         {
            contextID = -1 ;
            PD_LOG ( PDERROR, "Job[%s] get more failed[rc:%d]", name(), rc ) ;
            goto error ;
         }

         //delete records
         if ( SDB_DMS_NOTEXIST == _filterDel( buffObj.data(), buffObj.size(),
                                              cleanType, groupID ) )
         {
            clsTaskMgr *pTaskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
            pTaskMgr->lockReg( SHARED ) ;
            if ( 0 == pTaskMgr->getRegCount( _clFullName, TRUE ) )
            {
               // delete the collection
               rc = rtnDropCollectionCommand( fullName, eduCB(), _dmsCB,
                                              _dpsCB ) ;
               PD_LOG ( PDEVENT, "Job[%s] drop the collection[%s], rc:%d",
                        name(), fullName, rc ) ;
               if ( SDB_DMS_CS_NOTEXIST == rc )
               {
                  rc = SDB_OK ;
               }
               else if ( SDB_OK == rc || SDB_DMS_NOTEXIST == rc )
               {
                  // drop empty collectionspace, ignore errors
                  _dmsCB->dropEmptyCollectionSpace(
                           dmsGetCSNameFromFullName( _clFullName ).c_str(),
                           eduCB(), _dpsCB ) ;
                  rc = SDB_OK ;
               }
               pTaskMgr->releaseReg( SHARED ) ;
               goto done ;
            }
            pTaskMgr->releaseReg( SHARED ) ;

            break ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, eduCB(), rtnCB ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC ( SDB__CLSCLNJOB__CLNBYTBSCAN, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsCleanupJob::_cleanBySplitKeyObj ( INT32 w )
   {
      INT32 rc = rtnTraversalDelete ( _clFullName.c_str(), _splitKeyObj,
                                      IXM_SHARD_KEY_NAME, 1, eduCB(),
                                      _dmsCB, _dpsCB, w ) ;
      if ( SDB_CLS_WAIT_SYNC_FAILED == rc )
      {
         rc = SDB_OK ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCLNJOB__FLTDEL, "_clsCleanupJob::_filterDel" )
   INT32 _clsCleanupJob::_filterDel( const CHAR * buff, INT32 buffSize,
                                     CLS_CLEANUP_TYPE cleanType,
                                     UINT32 groupID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCLNJOB__FLTDEL );
      catAgent *catAgent = sdbGetShardCB()->getCataAgent() ;
      INT32 indexPos = 0 ;
      const CHAR *pBuffIndex = buff + indexPos ;
      BOOLEAN needDel = FALSE ;
      _clsCatalogSet *catSet = NULL ;
      INT32 w = 1 ;

      try
      {
         while ( indexPos < buffSize )
         {
            BSONObj recordObj ( pBuffIndex ) ;
            needDel = FALSE ;

         retry:
            catAgent->lock_r() ;
            catSet = catAgent->collectionSet( _clFullName.c_str() ) ;

            if ( catSet )
            {
               /// collection's unique id changed
               if ( UTIL_UNIQUEID_NULL != _clUniqueID &&
                    _clUniqueID != catSet->clUniqueID() )
               {
                  if ( 0 == catSet->groupCount() )
                  {
                     rc = SDB_DMS_NOTEXIST ;
                  }
                  else
                  {
                     rc = SDB_DMS_EOC ;
                  }
                  goto error ;
               }

               if ( CLS_CLEANUP_BY_CATAINFO == cleanType )
               {
                  if ( !catSet->isObjInGroup( recordObj, groupID) )
                  {
                     needDel = TRUE ;
                     // w = catSet->getW() ;
                  }
               }
               else if ( CLS_CLEANUP_BY_RANGE == cleanType )
               {
                  BSONObj keyObj ;
                  rc = catSet->genKeyObj( recordObj, keyObj ) ;
                  if ( rc )
                  {
                     PD_LOG( PDWARNING, "Gen key obj failed, rc: %d, obj: %s, "
                             "cata info: %s", rc, recordObj.toString().c_str(),
                             catSet->toCataInfoBson().toString().c_str() ) ;
                  }
                  else if ( _isHashSharding )
                  {
                     INT32 hashValue = clsPartition( keyObj,
                                                     catSet->getPartitionBit(),
                                                     catSet->getInternalV() ) ;
                     if ( hashValue >= _splitKeyObj.firstElement().numberInt() &&
                          ( _splitEndKeyObj.isEmpty() ||
                         hashValue < _splitEndKeyObj.firstElement().numberInt() ) &&
                         !catSet->isKeyInGroup( BSON(""<<hashValue), groupID ) )
                     {
                        needDel = TRUE ;
                        // w = catSet->getW() ;
                     }
                  }
                  else if ( catSet->getOrdering() )
                  {
                     if ( keyObj.woCompare( _splitKeyObj, *catSet->getOrdering(),
                                            false ) >= 0 &&
                          ( _splitEndKeyObj.isEmpty() ||
                            keyObj.woCompare( _splitEndKeyObj,
                                       *catSet->getOrdering(), false ) < 0 ) &&
                          !catSet->isKeyInGroup( keyObj, groupID ) )
                     {
                        needDel = TRUE ;
                        // w = catSet->getW() ;
                     }
                  }
               }
            }

            catAgent->release_r() ;

            // not found collection catalog
            if ( !catSet )
            {
               while ( TRUE )
               {
                  if ( eduCB()->isInterrupted() )
                  {
                     rc = SDB_APP_INTERRUPT ;
                     goto error ;
                  }

                  rc = sdbGetShardCB()->syncUpdateCatalog( _clFullName.c_str(),
                                                           OSS_ONE_SEC ) ;
                  if ( SDB_OK == rc )
                  {
                     goto retry ;
                  }
                  else if ( SDB_DMS_NOTEXIST == rc )
                  {
                     break ;
                  }
               }
 
               PD_LOG ( PDWARNING, "Job[%s] filter del not found collection[%s]"
                        " catalog info", name(), _clFullName.c_str() ) ;
               rc = SDB_DMS_NOTEXIST ;
               break ;
            }

            // delete record
            if ( needDel )
            {
               BSONElement idEle = recordObj.getField( DMS_ID_KEY_NAME ) ;
               BSONObjBuilder selectorBuilder ;
               selectorBuilder.append( idEle ) ;
               BSONObj selector = selectorBuilder.obj() ;
               BSONObj hint = BSON(""<<IXM_ID_KEY_NAME) ;

               rc = rtnDelete( _clFullName.c_str(), selector, hint, 0,
                               eduCB(), _dmsCB, _dpsCB, w ) ;
               if ( SDB_OK != rc && SDB_CLS_WAIT_SYNC_FAILED != rc )
               {
                  PD_LOG ( PDWARNING, "Job[%s] delete record[%s] failed[rc:%d]",
                           name(), recordObj.toString().c_str(), rc ) ;
               }
            }

            indexPos += ossRoundUpToMultipleX( recordObj.objsize(), 4 ) ;
            pBuffIndex = buff + indexPos ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Job[%s] filterDel record exception: %s",
                  name(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCLNJOB__FLTDEL, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STRARTCLNJOB, "startCleanupJob" )
   INT32 startCleanupJob( const std::string &clFullName,
                          utilCLUniqueID clUniqueID,
                          const BSONObj &splitKeyObj,
                          const BSONObj &splitEndKeyObj,
                          BOOLEAN hasShardingIndex,
                          BOOLEAN isHashSharding,
                          SDB_DPSCB *dpsCB,
                          EDUID *pEDUID,
                          BOOLEAN returnResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_STRARTCLNJOB );

      _clsCleanupJob *pJob = SDB_OSS_NEW _clsCleanupJob( clFullName,
                                                         clUniqueID,
                                                         splitKeyObj,
                                                         splitEndKeyObj,
                                                         hasShardingIndex,
                                                         isHashSharding,
                                                         dpsCB ) ;
      if ( !pJob )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for cleanupJob" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE, pEDUID,
                                     returnResult ) ;

   done:
      PD_TRACE_EXITRC ( SDB_STRARTCLNJOB, rc );
      return rc ;
   error:
      goto done ;
   }

}

