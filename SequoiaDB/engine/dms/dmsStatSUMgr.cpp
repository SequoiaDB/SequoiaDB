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

   Source File Name = dmsStatSUMgr.cpp

   Descriptive Name = Data Management Service Statistics Table Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   statistics table creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dmsStatSUMgr.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace bson
{
   extern BSONObj staticNull ;
}

namespace engine
{

#define DMS_STAT_CL_IDX_DEF \
   "{ " IXM_FIELD_NAME_NAME "      : \"" DMS_STAT_CL_IDX_NAME "\", \
      " IXM_FIELD_NAME_KEY "       : { " DMS_STAT_COLLECTION_SPACE " : 1, \
                                     " DMS_STAT_COLLECTION " : 1 }, \
      " IXM_FIELD_NAME_UNIQUE "    : true, \
      " IXM_FIELD_NAME_ENFORCED "  : true }"

#define DMS_STAT_IDX_IDX_DEF \
   "{ " IXM_FIELD_NAME_NAME "      : \"" DMS_STAT_IDX_IDX_NAME "\", \
      " IXM_FIELD_NAME_KEY "       : { " DMS_STAT_COLLECTION_SPACE " : 1, \
                                     " DMS_STAT_COLLECTION " : 1, \
                                     " DMS_STAT_IDX_INDEX " : 1 } , \
      " IXM_FIELD_NAME_UNIQUE "    : true, \
      " IXM_FIELD_NAME_ENFORCED "  : true }"

   const utilCSUniqueID DMS_STAT_CSUID = UTIL_CSUNIQUEID_SYS_MIN + 4 ;
   const utilCLUniqueID DMS_STAT_CL_CLUID =
               utilBuildCLUniqueID( DMS_STAT_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 1 ) ;
   const utilCLUniqueID DMS_STAT_IDX_CLUID =
               utilBuildCLUniqueID( DMS_STAT_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 2 ) ;

   /*
      _dmsStatSUMgr implement
    */
   _dmsStatSUMgr::_dmsStatSUMgr ( SDB_DMSCB *dmsCB )
   : _dmsSysSUMgr( dmsCB )
   {
      _initialized = FALSE ;
      _tbScanHint = staticNull ;
      _collectionHint = BSON( "" << DMS_STAT_CL_IDX_NAME ) ;
      _indexHint = BSON( "" << DMS_STAT_IDX_IDX_NAME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTATSUMGR_INIT, "_dmsStatSUMgr::init" )
   INT32 _dmsStatSUMgr::init ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTATSUMGR_INIT ) ;

      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;

      SDB_ASSERT ( _dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;

      // exclusive lock SYSSTAT cb. this function should be called during
      // process initialization, so it shouldn't be called in parallel by
      // agents
      DMSSYSSUMGR_XLOCK() ;

      // first to load collection space
      rc = rtnCollectionSpaceLock( DMS_STAT_SPACE_NAME, _dmsCB, TRUE,
                                   &_su, suID ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // create new SYSSTAT collection space
         rc = rtnCreateCollectionSpaceCommand ( DMS_STAT_SPACE_NAME, NULL,
                                                _dmsCB, NULL,
                                                DMS_STAT_CSUID,
                                                DMS_PAGE_SIZE_MAX,
                                                DMS_DO_NOT_CREATE_LOB,
                                                DMS_STORAGE_NORMAL, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create %s collection "
                      "space, rc: %d", DMS_STAT_SPACE_NAME, rc ) ;

         rc = rtnCollectionSpaceLock ( DMS_STAT_SPACE_NAME, _dmsCB, TRUE,
                                       &_su, suID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock %s collection space, "
                      "rc: %d", DMS_STAT_SPACE_NAME, rc ) ;
      }
      else if ( SDB_OK != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space [%s], "
                      "rc: %d", DMS_STAT_SPACE_NAME, rc ) ;
      }

      _su->data()->setTransSupport( FALSE ) ;

      _dmsCB->suUnlock( suID ) ;
      suID = DMS_INVALID_CS ;

      rc = _ensureStatMetadata( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create statistics collections or "
                   "indexes, rc: %d", rc ) ;

      _initialized = TRUE ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         _dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTATSUMGR_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADALLCLSTATS, "_dmsStatSUMgr::loadAllCollectionStats" )
   INT32 _dmsStatSUMgr::loadAllCollectionStats ( const MON_CS_SIM_LIST &monCSList,
                                                 dmsStatCacheMap &statCacheMap,
                                                 pmdEDUCB *cb,
                                                 _SDB_DMSCB *dmsCB,
                                                 _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADALLCLSTATS ) ;

      INT64 contextID = -1 ;
      BSONObj boDummy ;

      if ( NULL == dmsCB )
      {
         dmsCB = _dmsCB ;
      }

      if ( NULL == rtnCB )
      {
         rtnCB = pmdGetKRCB()->getRTNCB() ;
      }

      // query
      rc = rtnQuery( DMS_STAT_COLLECTION_CL_NAME, boDummy, boDummy, boDummy,
                     _tbScanHint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Query collection [%s] failed, rc: %d",
                   DMS_STAT_COLLECTION_CL_NAME, rc ) ;

      // get more
      while ( TRUE )
      {
         dmsCollectionStat *pCollectionStat = NULL ;
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc ) ;

         pCollectionStat = SDB_OSS_NEW dmsCollectionStat() ;
         PD_CHECK( pCollectionStat, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for index statistics" ) ;

         try
         {
            BSONObj boCollectionStat = BSONObj( contextBuf.data() ) ;

            rc = pCollectionStat->init( boCollectionStat ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING,
                       "Failed to initialize collection statistics with %s",
                       boCollectionStat.toString( FALSE, TRUE ).c_str() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING,
                    "Get index statistics for collection occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
         }

         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( pCollectionStat ) ;
            rc = SDB_OK ;
            continue ;
         }

         rc = _addCollectionStat( monCSList, statCacheMap, pCollectionStat,
                                  FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING,
                    "Failed to add collection statistics [%s.%s], rc: %d",
                    pCollectionStat->getCSName(), pCollectionStat->getCLName(),
                    rc ) ;
            SAFE_OSS_DELETE( pCollectionStat ) ;
         }
         // Continue
         rc = SDB_OK ;
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADALLCLSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADALLIDXSTATS, "_dmsStatSUMgr::loadAllIndexStats" )
   INT32 _dmsStatSUMgr::loadAllIndexStats ( const MON_CS_SIM_LIST &monCSList,
                                            dmsStatCacheMap &statCacheMap,
                                            pmdEDUCB *cb,
                                            _SDB_DMSCB *dmsCB,
                                            _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADALLIDXSTATS ) ;

      INT64 contextID = -1 ;
      BSONObj boDummy ;

      if ( NULL == dmsCB )
      {
         dmsCB = _dmsCB ;
      }

      if ( NULL == rtnCB )
      {
         rtnCB = pmdGetKRCB()->getRTNCB() ;
      }

      // query
      rc = rtnQuery( DMS_STAT_INDEX_CL_NAME, boDummy, boDummy, boDummy,
                     _tbScanHint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Query collection [%s] failed, rc: %d",
                   DMS_STAT_INDEX_CL_NAME, rc ) ;

      // get more
      while ( TRUE )
      {
         dmsIndexStat *pIndexStat = NULL ;
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc ) ;

         pIndexStat = SDB_OSS_NEW dmsIndexStat() ;
         PD_CHECK( pIndexStat, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for index statistics" ) ;

         try
         {
            BSONObj boIndexStat = BSONObj( contextBuf.data() ) ;

            rc = pIndexStat->init( boIndexStat ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING,
                       "Failed to initialize index statistics with %s",
                       boIndexStat.toString( FALSE, TRUE ).c_str() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING,
                    "Get index statistics for index occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( pIndexStat ) ;
            rc = SDB_OK ;
            continue ;
         }

         rc = _addIndexStat( monCSList, statCacheMap, pIndexStat, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING,
                    "Failed to add index statistics [%s.%s, %s], rc: %d",
                    pIndexStat->getCSName(), pIndexStat->getCLName(),
                    pIndexStat->getIndexName(), rc ) ;
            SAFE_OSS_DELETE( pIndexStat ) ;
         }
         // Continue
         rc = SDB_OK ;
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADALLIDXSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADSUCLSTATS_MON, "_dmsStatSUMgr::loadSUCollectionStats" )
   INT32 _dmsStatSUMgr::loadSUCollectionStats ( const monCSSimple *pMonCS,
                                                dmsStatCache *pStatCache,
                                                pmdEDUCB *cb,
                                                _SDB_DMSCB *dmsCB,
                                                _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADSUCLSTATS_MON ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pMonCS->_name ) ) ;

      rc = _loadCollectionStats( pMonCS, NULL, pStatCache, boMatcher, cb, dmsCB,
                                 rtnCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to load collection statistics for "
                   "collection space [%s], rc: %d", pMonCS->_name, rc ) ;

      pStatCache->setStatus( UTIL_SU_CACHE_UNIT_STATUS_CACHED ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADSUCLSTATS_MON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADSUIDXSTATS_MON, "_dmsStatSUMgr::loadSUIndexStats" )
   INT32 _dmsStatSUMgr::loadSUIndexStats ( const monCSSimple *pMonCS,
                                           dmsStatCache *pStatCache,
                                           pmdEDUCB *cb,
                                           _SDB_DMSCB *dmsCB,
                                           _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADSUIDXSTATS_MON ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pMonCS->_name ) ) ;

      rc = _loadIndexStats( pMonCS, NULL, NULL, pStatCache, boMatcher, cb,
                            dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to load index statistics for "
                   "collection space [%s], rc: %d", pMonCS->_name, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADSUIDXSTATS_MON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADCLSTAT, "_dmsStatSUMgr::loadCollectionStat" )
   INT32 _dmsStatSUMgr::loadCollectionStat ( const monCSSimple *pMonCS,
                                             const monCLSimple *pMonCL,
                                             dmsStatCache *pStatCache,
                                             pmdEDUCB *cb,
                                             _SDB_DMSCB *dmsCB,
                                             _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADCLSTAT ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pMonCS->_name <<
                               DMS_STAT_COLLECTION << pMonCL->_clname ) ) ;

      rc = _loadCollectionStats( pMonCS, pMonCL, pStatCache, boMatcher, cb,
                                 dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to load collection statistics for "
                   "collection [%s], rc: %d", pMonCL->_name, rc ) ;

      pStatCache->setStatus( pMonCL->_blockID,
                             UTIL_SU_CACHE_UNIT_STATUS_CACHED ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADCLSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADCLIDXSTATS, "_dmsStatSUMgr::loadCLIndexStats" )
   INT32 _dmsStatSUMgr::loadCLIndexStats ( const monCSSimple *pMonCS,
                                           const monCLSimple *pMonCL,
                                           dmsStatCache *pStatCache,
                                           pmdEDUCB *cb,
                                           _SDB_DMSCB *dmsCB,
                                           _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADCLIDXSTATS ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pMonCS->_name <<
                               DMS_STAT_COLLECTION << pMonCL->_clname ) ) ;

      rc = _loadIndexStats( pMonCS, pMonCL, NULL, pStatCache, boMatcher, cb,
                            dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to load index statistics for "
                   "collection [%s], rc: %d", pMonCL->_name, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADCLIDXSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_LOADIDXSTATS, "_dmsStatSUMgr::loadIndexStats" )
   INT32 _dmsStatSUMgr::loadIndexStats ( const monCSSimple *pMonCS,
                                         const monCLSimple *pMonCL,
                                         const monIndex *pMonIX,
                                         dmsStatCache *pStatCache,
                                         pmdEDUCB *cb,
                                         _SDB_DMSCB *dmsCB,
                                         _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_LOADIDXSTATS ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;
      SDB_ASSERT( pMonIX, "pMonIX is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pMonCS->_name <<
                               DMS_STAT_COLLECTION << pMonCL->_clname <<
                               DMS_STAT_IDX_INDEX << pMonIX->getIndexName() ) ) ;

      rc = _loadIndexStats( pMonCS, pMonCL, pMonIX, pStatCache, boMatcher,
                            cb, dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to load index statistics [%s %s], "
                   "rc: %d", pMonCL->_name, pMonIX->getIndexName(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_LOADIDXSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_UPDATECLSTAT, "_dmsStatSUMgr::updateCollectionStat" )
   INT32 _dmsStatSUMgr::updateCollectionStat ( const dmsCollectionStat *pCollectionStat,
                                               pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                               _SDB_RTNCB *rtnCB,
                                               _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_UPDATECLSTAT ) ;

      SDB_ASSERT( pCollectionStat, "pCollectionStat is invalid" ) ;

      const CHAR *pCSName = pCollectionStat->getCSName() ;
      const CHAR *pCLName = pCollectionStat->getCLName() ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                               DMS_STAT_COLLECTION << pCLName ) ) ;
      BSONObj boUpdator = BSON( "$set" << pCollectionStat->toBSON() ) ;

      rc = rtnUpdate( DMS_STAT_COLLECTION_CL_NAME, boMatcher,
                      boUpdator, _collectionHint, FLG_UPDATE_UPSERT,
                      cb, dmsCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to update collection statistics "
                   "[%s.%s], rc: %d", pCSName, pCLName, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_UPDATECLSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_UPDATEIDXSTAT, "_dmsStatSUMgr::updateIndexStat" )
   INT32 _dmsStatSUMgr::updateIndexStat ( const dmsIndexStat *pIndexStat,
                                          pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                          _SDB_RTNCB *rtnCB,
                                          _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_UPDATEIDXSTAT ) ;

      SDB_ASSERT( pIndexStat, "pIndexStat is invalid" ) ;

      const CHAR *pCSName = pIndexStat->getCSName() ;
      const CHAR *pCLName = pIndexStat->getCLName() ;
      const CHAR *pIXName = pIndexStat->getIndexName() ;

      BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                               DMS_STAT_COLLECTION << pCLName <<
                               DMS_STAT_IDX_INDEX << pIXName ) ) ;
      BSONObj boUpdator ;

      if ( pIndexStat->isValidForEstimate() )
      {
         boUpdator = BSON( "$set" << pIndexStat->toBSON() ) ;
      }
      else
      {
         // Unset optional fields, which do not exist in default statistics
         boUpdator = BSON( "$set" << pIndexStat->toBSON() <<
                           "$unset" << BSON( DMS_STAT_IDX_MCV << "" ) ) ;
      }


      rc = rtnUpdate( DMS_STAT_INDEX_CL_NAME, boMatcher,
                      boUpdator, _indexHint, FLG_UPDATE_UPSERT,
                      cb, dmsCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to update index statistics "
                   "[%s.%s %s], rc: %d", pCSName, pCLName, pIXName, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_UPDATEIDXSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONUNLOADCS, "_dmsStatSUMgr::onUnloadCS" )
   INT32 _dmsStatSUMgr::onUnloadCS ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONUNLOADCS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            pCache->clearCacheUnits() ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONUNLOADCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONRENAMECS, "_dmsStatSUMgr::onRenameCS" )
   INT32 _dmsStatSUMgr::onRenameCS ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const CHAR *pOldCSName,
                                     const CHAR *pNewCSName,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needUpdate = FALSE ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONRENAMECS ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            for ( UINT16 unitID = 0 ; unitID < pCache->getSize() ; unitID ++ )
            {
               dmsCollectionStat *pCollectionStat =
                     (dmsCollectionStat *)pCache->getCacheUnit( unitID ) ;
               if ( pCollectionStat )
               {
                  pCollectionStat->renameCS( pNewCSName ) ;
               }
            }
            needUpdate = TRUE ;
         }
      }

      if ( needUpdate && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pOldCSName ) ) ;
         BSONObj boNewName( BSON( DMS_STAT_COLLECTION_SPACE << pNewCSName ) ) ;
         BSONObj boUpdator( BSON( "$set" << boNewName ) ) ;

         rc = _updateCollectionStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update collection statistics when rename "
                      "collection space [%s] to [%s], rc: %d",
                      pOldCSName, pNewCSName, rc ) ;

         rc = _updateIndexStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update index statistics when rename "
                      "collection space [%s] to [%s], rc: %d",
                      pOldCSName, pNewCSName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONRENAMECS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONDROPCS, "_dmsStatSUMgr::onDropCS" )
   INT32 _dmsStatSUMgr::onDropCS ( SDB_EVENT_OCCUR_TYPE type,
                                   IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventSUItem &suItem,
                                   dmsDropCSOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONDROPCS ) ;

      BOOLEAN needDelete = FALSE ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         goto done ;
      }

      if ( !_initialized )
      {
         PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         goto done ;
      }

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            pCache->clearCacheUnits() ;
            needDelete = TRUE ;
         }
      }

      if ( needDelete && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         INT32 tmpRC = SDB_OK ;

         const CHAR *pCSName = suItem._pCSName ;

         BSONObj boMatcher ;

         try
         {
            boMatcher = BSON( DMS_STAT_COLLECTION_SPACE << pCSName ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to build matcher, occur exception %s",
                    e.what() ) ;
            goto done ;
         }

         tmpRC = _deleteCollectionStat( boMatcher, cb, NULL ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to drop collection statistics when dropping "
                    "collection space [%s], rc: %d", pCSName, tmpRC ) ;
         }
         tmpRC = _deleteIndexStat( boMatcher, cb, NULL ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING,
                    "Failed to delete index statistics when dropping "
                    "collection space [%s], rc: %d", pCSName, tmpRC ) ;
         }
      }

   done :
      PD_TRACE_EXIT( SDB_DMSSTATSUMGR_ONDROPCS ) ;
      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONRENAMECL, "_dmsStatSUMgr::onRenameCL" )
   INT32 _dmsStatSUMgr::onRenameCL ( IDmsEventHolder *pEventHolder,
                                     IDmsSUCacheHolder *pCacheHolder,
                                     const dmsEventCLItem &clItem,
                                     const CHAR *pNewCLName,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needUpdate = FALSE ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONRENAMECL ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == pCache->getStatus( clItem._mbID ) )
            {
               needUpdate = TRUE ;
            }
            else
            {
               // For statistics cache, mbID is ID of cache unit
               dmsCollectionStat *pCollectionStat =
                     (dmsCollectionStat *)pCache->getCacheUnit( clItem._mbID ) ;
               if ( pCollectionStat )
               {
                  pCollectionStat->renameCL( pNewCLName ) ;
                  needUpdate = TRUE ;
               }
            }
         }
      }

      if ( needUpdate && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         const CHAR *pCSName = pEventHolder->getCSName() ;
         const CHAR *pOldCLName = clItem._pCLName ;

         BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                                  DMS_STAT_COLLECTION << pOldCLName ) ) ;
         BSONObj boNewName( BSON( DMS_STAT_COLLECTION << pNewCLName ) ) ;
         BSONObj boUpdator( BSON( "$set" << boNewName ) ) ;

         rc = _updateCollectionStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update collection statistics when rename "
                      "collection [%s.%s] to [%s.%s], rc: %d",
                      pCSName, pOldCLName, pCSName, pNewCLName, rc ) ;

         rc = _updateIndexStat( boMatcher, boUpdator, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update index statistics when rename "
                      "collection [%s.%s] to [%s.%s], rc: %d",
                      pCSName, pOldCLName, pCSName, pNewCLName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONRENAMECL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONTRUNCCL, "_dmsStatSUMgr::onTruncateCL" )
   INT32 _dmsStatSUMgr::onTruncateCL ( SDB_EVENT_OCCUR_TYPE type,
                                       IDmsEventHolder *pEventHolder,
                                       IDmsSUCacheHolder *pCacheHolder,
                                       const dmsEventCLItem &clItem,
                                       dmsTruncCLOptions *options,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONTRUNCCL ) ;

      if ( SDB_EVT_OCCUR_AFTER == type )
      {
         if ( _initialized )
         {
            BOOLEAN needDelete = FALSE ;

            SDB_ASSERT( pCacheHolder, "Event holder is invalid" ) ;

            if ( pCacheHolder )
            {
               dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
               if ( pCache )
               {
                  if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == pCache->getStatus( clItem._mbID ) )
                  {
                     needDelete = TRUE ;
                  }
                  else
                  {
                     // For statistics cache, mbID is key of cache unit
                     needDelete = pCache->removeCacheUnit( clItem._mbID, TRUE ) ;
                  }
               }
            }

            if ( needDelete && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
            {
               INT32 tmpRC = SDB_OK ;

               const CHAR *pCSName = pEventHolder->getCSName() ;
               const CHAR *pCLName = clItem._pCLName ;

               BSONObj boMatcher ;

               try
               {
                  boMatcher = BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                                    DMS_STAT_COLLECTION << pCLName ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDWARNING, "Failed to build matcher, occur exception %s",
                          e.what() ) ;
                  goto done ;
               }

               tmpRC = _deleteCollectionStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete collection statistics when truncating "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }

               tmpRC = _deleteIndexStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete index statistics when truncating "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         }
      }

   done :
      PD_TRACE_EXIT( SDB_DMSSTATSUMGR_ONTRUNCCL ) ;
      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONDROPCL, "_dmsStatSUMgr::onDropCL" )
   INT32 _dmsStatSUMgr::onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                   IDmsEventHolder *pEventHolder,
                                   IDmsSUCacheHolder *pCacheHolder,
                                   const dmsEventCLItem &clItem,
                                   dmsDropCLOptions *options,
                                   pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONDROPCL ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         if ( _initialized )
         {
            BOOLEAN needDelete = FALSE ;

            SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

            if ( pCacheHolder )
            {
               dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
               if ( pCache )
               {
                  if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == pCache->getStatus( clItem._mbID ) )
                  {
                     needDelete = TRUE ;
                  }
                  else
                  {
                     // For statistics cache, mbID is key of cache unit
                     needDelete = pCache->removeCacheUnit( clItem._mbID, TRUE ) ;
                  }
               }
            }

            if ( needDelete && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
            {
               INT32 tmpRC = SDB_OK ;

               const CHAR *pCSName = pEventHolder->getCSName() ;
               const CHAR *pCLName = clItem._pCLName ;

               BSONObj boMatcher ;

               try
               {
                  boMatcher = BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                                    DMS_STAT_COLLECTION << pCLName ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDWARNING, "Failed to build matcher, occur exception %s",
                          e.what() ) ;
                  goto done ;
               }

               tmpRC = _deleteCollectionStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete collection statistics when dropping "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }

               tmpRC = _deleteIndexStat( boMatcher, cb, NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING,
                          "Failed to delete index statistics when dropping "
                          "collection [%s.%s], rc: %d", pCSName, pCLName, tmpRC ) ;
               }
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Statistics SU is not initialized" ) ;
         }
      }

   done :
      PD_TRACE_EXIT( SDB_DMSSTATSUMGR_ONDROPCL ) ;
      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ONIDXOPTR, "_dmsStatSUMgr::_onIndexOperator" )
   INT32 _dmsStatSUMgr::_onIndexOperator ( IDmsEventHolder *pEventHolder,
                                           IDmsSUCacheHolder *pCacheHolder,
                                           const dmsEventCLItem &clItem,
                                           const dmsEventIdxItem &idxItem,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ONIDXOPTR ) ;

      BOOLEAN needDelete = FALSE ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            if ( UTIL_SU_CACHE_UNIT_STATUS_EMPTY == pCache->getStatus( clItem._mbID ) )
            {
               needDelete = TRUE ;
            }
            else
            {
               // For statistics cache, mbID is ID of cache unit
               dmsCollectionStat *pCollectionStat =
                     (dmsCollectionStat *)pCache->getCacheUnit( clItem._mbID ) ;
               if ( pCollectionStat )
               {
                  needDelete = pCollectionStat->removeIndexStat( idxItem._idxLID, TRUE ) ;
               }
            }
         }
      }

      if ( needDelete && pEventHolder && SDB_DB_NORMAL == PMD_DB_STATUS() )
      {
         const CHAR *pCSName = pEventHolder->getCSName() ;
         const CHAR *pCLName = clItem._pCLName ;
         const CHAR *pIXName = idxItem._pIXName ;

         BSONObj boMatcher( BSON( DMS_STAT_COLLECTION_SPACE << pCSName <<
                                  DMS_STAT_COLLECTION << pCLName <<
                                  DMS_STAT_IDX_INDEX << pIXName ) ) ;

         rc = _deleteIndexStat( boMatcher, cb, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to delete index statistics "
                      "when operating on index [%s.%s %s] , rc: %d", pCSName,
                      pCLName, pIXName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ONIDXOPTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONCREATEIDX, "_dmsStatSUMgr::onCreateIndex" )
   INT32 _dmsStatSUMgr::onCreateIndex ( IDmsEventHolder *pEventHolder,
                                        IDmsSUCacheHolder *pCacheHolder,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONCREATEIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      rc = _onIndexOperator( pEventHolder, pCacheHolder, clItem, idxItem, cb,
                             dpsCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to delete statistics when creating "
                   "index, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONCREATEIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONDROPIDX, "_dmsStatSUMgr::onDropIndex" )
   INT32 _dmsStatSUMgr::onDropIndex ( IDmsEventHolder *pEventHolder,
                                      IDmsSUCacheHolder *pCacheHolder,
                                      const dmsEventCLItem &clItem,
                                      const dmsEventIdxItem &idxItem,
                                      pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONDROPIDX ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      rc = _onIndexOperator( pEventHolder, pCacheHolder, clItem, idxItem, cb,
                             dpsCB ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to delete statistics when dropping "
                   "index, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONDROPIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONCLRSUCACHES, "_dmsStatSUMgr::onClearSUCaches" )
   INT32 _dmsStatSUMgr::onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONCLRSUCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            pCache->clearCacheUnits() ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONCLRSUCACHES, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR_ONCLRCLCACHES, "_dmsStatSUMgr::onClearCLCaches" )
   INT32 _dmsStatSUMgr::onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder,
                                          const dmsEventCLItem &clItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR_ONCLRCLCACHES ) ;

      SDB_ASSERT( pEventHolder, "Event holder is invalid" ) ;

      PD_CHECK( _initialized, SDB_INVALIDARG, error, PDWARNING,
                "Statistics SU is not initialized" ) ;

      if ( pCacheHolder )
      {
         dmsSUCache *pCache = pCacheHolder->getSUCache( DMS_CACHE_TYPE_STAT ) ;
         if ( pCache )
         {
            pCache->removeCacheUnit( clItem._mbID, TRUE ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR_ONCLRCLCACHES, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _dmsStatSUMgr::_ensureStatMetadata ( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTestAndCreateCL( DMS_STAT_COLLECTION_CL_NAME, cb, _dmsCB, NULL,
                               DMS_STAT_CL_CLUID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create collection [%s], rc: %d",
                   DMS_STAT_COLLECTION_CL_NAME, rc ) ;

      rc = rtnTestAndCreateCL( DMS_STAT_INDEX_CL_NAME, cb, _dmsCB, NULL, DMS_STAT_IDX_CLUID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create collection [%s], rc: %d",
                   DMS_STAT_INDEX_CL_NAME, rc ) ;

      {
         BSONObj idxDef ;

         rc = fromjson( DMS_STAT_CL_IDX_DEF, idxDef ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build index object [%s], rc: %d",
                      DMS_STAT_CL_IDX_DEF, rc ) ;

         // Initialized before rtn, so no sorterCreator could be used, set
         // sort buffer size to 0 to build index without sorterCreator
         rc = rtnTestAndCreateIndex( DMS_STAT_COLLECTION_CL_NAME, idxDef, cb,
                                     _dmsCB, NULL, TRUE, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create index [%s], rc: %d",
                      DMS_STAT_CL_IDX_DEF, rc ) ;
      }

      {
         BSONObj idxDef ;

         rc = fromjson( DMS_STAT_IDX_IDX_DEF, idxDef ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build index object [%s], rc: %d",
                      DMS_STAT_IDX_IDX_DEF, rc ) ;

         // Initialized before rtn, so no sorterCreator could be used, set
         // sort buffer size to 0 to build index without sorterCreator
         rc = rtnTestAndCreateIndex( DMS_STAT_INDEX_CL_NAME, idxDef, cb,
                                     _dmsCB, NULL, TRUE, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create index [%s], rc: %d",
                      DMS_STAT_IDX_IDX_DEF, rc ) ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDCLSTAT, "_dmsStatSUMgr::_addCollectionStat" )
   INT32 _dmsStatSUMgr::_addCollectionStat ( const MON_CS_SIM_LIST &monCSList,
                                             dmsStatCacheMap &statCacheMap,
                                             dmsCollectionStat *pCollectionStat,
                                             BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDCLSTAT ) ;

      const CHAR *pCSName = pCollectionStat->getCSName() ;
      const CHAR *pCLName = pCollectionStat->getCLName() ;

      dmsStatCacheMap::iterator iterSUStat ;
      dmsStatCache *pStatCache = NULL ;

      const monCSSimple *pMonCS = NULL ;

      // Get collection space information
      pMonCS = monCSSimple::getCollectionSpace( monCSList, pCSName ) ;
      PD_CHECK( pMonCS, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Could not get collection space [%s] for statistics",
                pCSName ) ;

      // Get statistics cache
      iterSUStat = statCacheMap.find( pCSName ) ;
      if ( iterSUStat == statCacheMap.end() )
      {
         pStatCache = SDB_OSS_NEW dmsStatCache( NULL ) ;
         PD_CHECK( pStatCache, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for statistics map" ) ;
         statCacheMap.insert( dmsStatCacheMap::value_type( pMonCS->_name,
                                                           pStatCache ) ) ;
      }
      else
      {
         pStatCache = iterSUStat->second ;
         if ( NULL == pStatCache )
         {
            statCacheMap.erase( iterSUStat ) ;

            pStatCache = SDB_OSS_NEW dmsStatCache( NULL ) ;
            PD_CHECK( pStatCache, SDB_OOM, error, PDWARNING,
                      "Failed to allocate memory for statistics map" ) ;

            statCacheMap.insert( dmsStatCacheMap::value_type( pMonCS->_name,
                                                              pStatCache ) ) ;
         }
      }

      rc = _addSUCollectionStat( pMonCS, NULL, pStatCache, pCollectionStat,
                                 FALSE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to add collection statistics [%s.%s], rc: %d",
                   pCSName, pCLName, rc ) ;
  done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDIDXSTAT, "_dmsStatSUMgr::_addIndexStat" )
   INT32 _dmsStatSUMgr::_addIndexStat ( const MON_CS_SIM_LIST &monCSList,
                                        dmsStatCacheMap &statCacheMap,
                                        dmsIndexStat *pIndexStat,
                                        BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDIDXSTAT ) ;

      const CHAR *pCSName = pIndexStat->getCSName() ;

      dmsStatCacheMap::iterator iterSUStat ;
      dmsStatCache *pStatCache = NULL ;

      const monCSSimple *pMonCS = NULL ;

      // Get collection space information
      pMonCS = monCSSimple::getCollectionSpace( monCSList, pCSName ) ;
      PD_CHECK( pMonCS, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Could not get collection space [%s] for statistics",
                pCSName ) ;

      // Get statistics cache
      iterSUStat = statCacheMap.find( pCSName ) ;

      PD_CHECK( iterSUStat != statCacheMap.end(), SDB_DMS_CS_NOTEXIST, error,
                PDWARNING, "Collection space [%s] is not found for statistics",
                pCSName ) ;

      pStatCache = iterSUStat->second ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDWARNING,
                "Collection space [%s] is not found for statistics",
                pCSName ) ;

      rc = _addSUIndexStat( pMonCS, NULL, NULL, pStatCache,
                            pIndexStat, ignoreCrtTime ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to add index statistics [%s.%s, %s], rc: %d",
                   pCSName, pIndexStat->getCLName(),
                   pIndexStat->getIndexName(), rc ) ;

  done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDSUCLSTAT, "_dmsStatSUMgr::_addSUCollectionStat" )
   INT32 _dmsStatSUMgr::_addSUCollectionStat ( const monCSSimple *pMonCS,
                                               const monCLSimple *pMonCL,
                                               dmsStatCache *pStatCache,
                                               dmsCollectionStat *pCollectionStat,
                                               BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDSUCLSTAT ) ;

      const CHAR *pCSName = pCollectionStat->getCSName() ;
      const CHAR *pCLName = pCollectionStat->getCLName() ;

      BOOLEAN needCheck = TRUE ;

      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      if ( pMonCS )
      {
         PD_CHECK( 0 == ossStrcmp( pMonCS->_name, pCSName ),
                   SDB_SYS, error, PDWARNING,
                   "Names of collection spaces are different, dump [%s], "
                   "statistics [%s]", pMonCS->_name, pCSName ) ;

         if ( NULL == pMonCL )
         {
            pMonCL = pMonCS->getCollection( pCLName ) ;
            PD_CHECK( pMonCL, SDB_DMS_NOTEXIST, error, PDWARNING,
                      "Could not get collection [%s.%s] for statistics",
                      pCSName, pCLName ) ;
         }
         else
         {
            PD_CHECK( 0 == ossStrcmp( pMonCL->_clname, pCLName ),
                      SDB_SYS, error, PDWARNING,
                      "Names of collection are different, dump [%s], "
                      "statistics [%s]", pMonCL->_clname, pCLName ) ;
         }

         pCollectionStat->setSULogicalID( pMonCS->_logicalID ) ;
         pCollectionStat->setCLLogicalID( pMonCL->_logicalID ) ;
         pCollectionStat->setMBID( pMonCL->_blockID ) ;

         needCheck = FALSE ;
      }

      PD_CHECK( pStatCache->addCacheUnit( pCollectionStat, ignoreCrtTime,
                                          needCheck ),
                SDB_INVALIDARG, error, PDWARNING,
                "Could not add collection statistics [%s.%s] to statistics map",
                pCSName, pCLName ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDSUCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__ADDSUIDXSTAT, "_dmsStatSUMgr::_addSUIndexStat" )
   INT32 _dmsStatSUMgr::_addSUIndexStat ( const monCSSimple *pMonCS,
                                          const monCLSimple *pMonCL,
                                          const monIndex *pMonIX,
                                          dmsStatCache *pStatCache,
                                          dmsIndexStat *pIndexStat,
                                          BOOLEAN ignoreCrtTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__ADDSUIDXSTAT ) ;

      const CHAR *pCSName = pIndexStat->getCSName() ;
      const CHAR *pCLName = pIndexStat->getCLName() ;
      const CHAR *pIXName = pIndexStat->getIndexName() ;

      BOOLEAN needCheck = TRUE ;

      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      if ( pMonCS )
      {
         PD_CHECK( 0 == ossStrcmp( pMonCS->_name, pCSName ),
                   SDB_SYS, error, PDWARNING,
                   "Names of collection spaces are different, dump [%s], "
                   "statistics [%s]", pMonCS->_name, pCSName ) ;

         if ( pMonCL == NULL )
         {
            pMonCL = pMonCS->getCollection( pCLName ) ;
            PD_CHECK( pMonCL, SDB_DMS_NOTEXIST, error, PDWARNING,
                      "Could not get collection [%s.%s] for statistics",
                      pCSName, pCLName ) ;
         }
         else
         {
            PD_CHECK( 0 == ossStrcmp( pMonCL->_clname, pCLName ),
                      SDB_SYS, error, PDWARNING,
                      "Names of collection are different, dump [%s], "
                      "statistics [%s]", pMonCL->_clname, pCLName ) ;
         }

         if ( NULL == pMonIX )
         {
            pMonIX = pMonCL->getIndex( pIXName ) ;
            PD_CHECK( pMonIX, SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Could not get index [%s.%s %s] for statistics",
                      pCSName, pCLName, pIXName ) ;
         }
         else
         {
            PD_CHECK( 0 == ossStrcmp( pMonIX->getIndexName(), pIXName ),
                      SDB_SYS, error, PDWARNING,
                      "Names of indexes are different, dump [%s], "
                      "statistics [%s]", pMonIX->getIndexName(), pIXName ) ;
         }

         try
         {
            BSONObj dumpKeyPattern = pMonIX->getKeyPattern() ;
            BSONObj statKeyPattern = pIndexStat->getKeyPattern() ;
            PD_CHECK( 0 == dumpKeyPattern.woCompare( statKeyPattern, BSONObj(),
                                                     TRUE ),
                      SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Index [%s.%s %s] is not found for statistics: "
                      "different key pattern, dump [%s], stat [%s]",
                      pCSName, pCLName, pIXName,
                      dumpKeyPattern.toString( FALSE, TRUE ).c_str(),
                      statKeyPattern.toString( FALSE, TRUE ).c_str() ) ;

            PD_CHECK( pMonIX->isUnique() == pIndexStat->isUnique(),
                      SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Index [%s.%s %s] is not found for statistics: "
                      "different unique definition, dump [%s], stat [%s]",
                      pCSName, pCLName, pIXName,
                      pMonIX->isUnique() ? "true" : "false",
                      pIndexStat->isUnique() ? "true" : "false" ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Checking index [%s.%s, %s] occur exception: %s",
                    pCSName, pCLName, pIXName, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         pIndexStat->setSULogicalID( pMonCS->_logicalID ) ;
         pIndexStat->setCLLogicalID( pMonCL->_logicalID ) ;
         pIndexStat->setMBID( pMonCL->_blockID ) ;
         pIndexStat->setIndexLogicalID( pMonIX->_indexLID ) ;

         needCheck = FALSE ;
      }

      PD_CHECK( pStatCache->addCacheSubUnit( pIndexStat, ignoreCrtTime,
                                             needCheck ),
                SDB_INVALIDARG, error, PDWARNING, "Failed to add index "
                "statistics [%s.%s %s] to statistics", pCSName, pCLName,
                pIXName ) ;

  done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__ADDSUIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__DELCLSTAT, "_dmsStatSUMgr::_deleteCollectionStat" )
   INT32 _dmsStatSUMgr::_deleteCollectionStat ( const BSONObj &boMatcher,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__DELCLSTAT ) ;

      rc = rtnDelete( DMS_STAT_COLLECTION_CL_NAME, boMatcher, _collectionHint,
                      0, cb, _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Delete collection statistics [%s] failed, rc: %d",
                   boMatcher.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__DELCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__DELIDXSTAT, "_dmsStatSUMgr::_deleteIndexStat" )
   INT32 _dmsStatSUMgr::_deleteIndexStat ( const BSONObj &boMatcher,
                                           _pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__DELIDXSTAT ) ;

      rc = rtnDelete( DMS_STAT_INDEX_CL_NAME, boMatcher, _indexHint, 0, cb,
                      _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Delete index statistics [%s] failed, rc: %d",
                   boMatcher.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__DELIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__UPDATECLSTAT, "_dmsStatSUMgr::_updateCollectionStat" )
   INT32 _dmsStatSUMgr::_updateCollectionStat ( const BSONObj &boMatcher,
                                                const BSONObj &boUpdator,
                                                _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__UPDATECLSTAT ) ;

      rc = rtnUpdate( DMS_STAT_COLLECTION_CL_NAME, boMatcher, boUpdator,
                      _collectionHint, 0, cb, _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Update collection statistics [%s] with [%s] failed, rc: %d",
                   boMatcher.toString( FALSE, TRUE ).c_str(),
                   boUpdator.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__UPDATECLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__UPDATEIDXSTAT, "_dmsStatSUMgr::_updateIndexStat" )
   INT32 _dmsStatSUMgr::_updateIndexStat ( const BSONObj &boMatcher,
                                           const BSONObj &boUpdator,
                                           _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__UPDATEIDXSTAT ) ;

      rc = rtnUpdate( DMS_STAT_INDEX_CL_NAME, boMatcher, boUpdator,
                      _indexHint, 0, cb, _dmsCB, dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Update index statistics [%s] with [%s] failed, rc: %d",
                   boMatcher.toString( FALSE, TRUE ).c_str(),
                   boUpdator.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__UPDATEIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__LOADCLSTATS, "_dmsStatSUMgr::_loadCollectionStats" )
   INT32 _dmsStatSUMgr::_loadCollectionStats ( const monCSSimple *pMonCS,
                                               const monCLSimple *pMonCL,
                                               dmsStatCache *pStatCache,
                                               const BSONObj &boMatcher,
                                               pmdEDUCB *cb,
                                               _SDB_DMSCB *dmsCB,
                                               _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__LOADCLSTATS ) ;

      BSONObj boDummy ;
      INT64 contextID = -1 ;

      // The collection is specified, could not skip errors
      BOOLEAN couldContinue = !pMonCL ;

      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      if ( NULL == dmsCB )
      {
         dmsCB = _dmsCB ;
      }

      if ( NULL == rtnCB )
      {
         rtnCB = pmdGetKRCB()->getRTNCB() ;
      }

      // query
      rc = rtnQuery( DMS_STAT_COLLECTION_CL_NAME, boDummy, boMatcher, boDummy,
                     _collectionHint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Query statistics [%s] from [%s] failed, "
                   "rc: %d", boMatcher.toString( FALSE, TRUE ).c_str(),
                   DMS_STAT_COLLECTION_CL_NAME, rc ) ;

      // get more
      while ( TRUE )
      {
         dmsCollectionStat *pCollectionStat = NULL ;
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc ) ;

         pCollectionStat = SDB_OSS_NEW dmsCollectionStat() ;
         PD_CHECK( pCollectionStat, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for collection statistics" ) ;

         try
         {
            BSONObj boCollectionStat = BSONObj( contextBuf.data() ) ;

            rc = pCollectionStat->init( boCollectionStat ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING,
                       "Failed to initialize collection statistics with %s",
                       boCollectionStat.toString( FALSE, TRUE ).c_str() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING,
                    "Get index statistics for collection occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
         }

         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( pCollectionStat ) ;
            if ( couldContinue )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               goto error ;
            }
         }

         rc = _addSUCollectionStat( pMonCS, pMonCL, pStatCache,
                                    pCollectionStat, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( ( couldContinue ? PDWARNING : PDERROR ),
                    "Failed to add collection statistics [%s.%s], rc: %d",
                    pCollectionStat->getCSName(), pCollectionStat->getCLName(),
                    rc ) ;
            SAFE_OSS_DELETE( pCollectionStat ) ;

            if ( !couldContinue )
            {
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__LOADCLSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSSTATSUMGR__LOADIDXSTATS, "_dmsStatSUMgr::_loadIndexStats" )
   INT32 _dmsStatSUMgr::_loadIndexStats ( const monCSSimple *pMonCS,
                                          const monCLSimple *pMonCL,
                                          const monIndex *pMonIX,
                                          dmsStatCache *pStatCache,
                                          const BSONObj &boMatcher,
                                          pmdEDUCB *cb,
                                          _SDB_DMSCB *dmsCB,
                                          _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_DMSSTATSUMGR__LOADIDXSTATS ) ;

      BSONObj boDummy ;
      INT64 contextID = -1 ;

      // The index is specified, could not skip errors
      BOOLEAN couldContinue = !pMonIX ;

      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      if ( NULL == dmsCB )
      {
         dmsCB = _dmsCB ;
      }

      if ( NULL == rtnCB )
      {
         rtnCB = pmdGetKRCB()->getRTNCB() ;
      }

      // query
      rc = rtnQuery( DMS_STAT_INDEX_CL_NAME, boDummy, boMatcher, boDummy,
                     _indexHint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Query statistics [%s] from [%s] failed, "
                   "rc: %d", boMatcher.toString( FALSE, TRUE ).c_str(),
                   DMS_STAT_INDEX_CL_NAME, rc ) ;

      // get more
      while ( TRUE )
      {
         dmsIndexStat *pIndexStat = NULL ;
         rtnContextBuf contextBuf ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Get more failed, rc: %d", rc ) ;

         pIndexStat = SDB_OSS_NEW dmsIndexStat() ;
         PD_CHECK( pIndexStat, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for index statistics" ) ;

         try
         {
            BSONObj boIndexStat = BSONObj( contextBuf.data() ) ;

            rc = pIndexStat->init( boIndexStat ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING,
                       "Failed to initialize index statistics with %s",
                       boIndexStat.toString( FALSE, TRUE ).c_str() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING,
                    "Get index statistics for index occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
         }

         if ( SDB_OK != rc )
         {
            SAFE_OSS_DELETE( pIndexStat ) ;
            if ( couldContinue )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               goto error ;
            }
         }

         rc = _addSUIndexStat( pMonCS, pMonCL, pMonIX, pStatCache,
                               pIndexStat, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( ( couldContinue ? PDWARNING : PDERROR ),
                    "Failed to add index statistics [%s.%s, %s], rc: %d",
                    pIndexStat->getCSName(), pIndexStat->getCLName(),
                    pIndexStat->getIndexName(), rc ) ;
            SAFE_OSS_DELETE( pIndexStat ) ;

            if ( !couldContinue )
            {
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_DMSSTATSUMGR__LOADIDXSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}

