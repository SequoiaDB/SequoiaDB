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

   Source File Name = rtnAnalyze.cpp

   Descriptive Name = Runtime Analyze

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime analyze functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/18/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dpsOp2Record.hpp"
#include "rtnInternalSorting.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   #define RTN_ANALYZE_SORT_BUF_SIZE ( 32 * 1024 * 1024 )

   /**
    * @brief Just compact all resources for internal sorting.
    */
   struct _rtnInternalSortArea
   {
      _rtnInternalSortArea()
      : _tupleDirectory( UTIL_BUFF_MODE_SERIAL, RTN_SORT_TUPLE_DIR_MIN_SZ,
                         &_monitor ),
        _tupleBuff( UTIL_BUFF_MODE_SCATTER, RTN_SORT_TUPLE_MIN_SZ, &_monitor )
      {
      }

      INT32 init( UINT64 limit )
      {
         return _monitor.init( limit ) ;
      }

      utilBuffMonitor _monitor ;
      utilCommBuff _tupleDirectory ;
      utilCommBuff _tupleBuff ;
   } ;

   static INT32 _rtnAnalyzeAll ( const rtnAnalyzeParam &param,
                                 pmdEDUCB *cb,
                                 _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB,
                                 _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeCS ( const CHAR *pCSName,
                                const rtnAnalyzeParam &param,
                                pmdEDUCB *cb,
                                _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB,
                                _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeCL ( const CHAR *pCLName,
                                const rtnAnalyzeParam &param,
                                pmdEDUCB *cb,
                                _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB,
                                _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeIndex ( const CHAR *pCLName,
                                   const CHAR *pIndexName,
                                   const rtnAnalyzeParam &param,
                                   pmdEDUCB *cb,
                                   _SDB_DMSCB *dmsCB,
                                   _SDB_RTNCB *rtnCB,
                                   _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnReloadAllStats ( const MON_CS_SIM_LIST &monCSList,
                                     BOOLEAN needCheck,
                                     pmdEDUCB *cb,
                                     _SDB_DMSCB *dmsCB,
                                     _SDB_RTNCB *rtnCB ) ;

   static INT32 _rtnReplaceCSStats ( const monCSSimple *pMonCS,
                                     dmsStatCache *pInputStatCache,
                                     BOOLEAN needCheck,
                                     _SDB_DMSCB *dmsCB ) ;

   static INT32 _rtnReloadCSStats ( const monCSSimple *pMonCS,
                                    BOOLEAN needCheck,
                                    pmdEDUCB *cb,
                                    _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB ) ;

   static INT32 _rtnReloadCLStats ( const monCSSimple *pMonCS,
                                    const monCLSimple *pMonCL,
                                    dmsStatCache *pStatCache,
                                    pmdEDUCB *cb,
                                    _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB ) ;

   static INT32 _rtnReloadIdxStat ( const monCSSimple *pMonCS,
                                    const monCLSimple *pMonCL,
                                    const monIndex *pMonIX,
                                    dmsStatCache *pStatCache,
                                    pmdEDUCB *cb,
                                    _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB ) ;

   static INT32 _rtnAnalyzeAllStats ( const MON_CS_SIM_LIST &monCSList,
                                      const rtnAnalyzeParam &param,
                                      pmdEDUCB *cb,
                                      _SDB_DMSCB *dmsCB,
                                      _SDB_RTNCB *rtnCB,
                                      _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeCSStats ( const monCSSimple *pMonCS,
                                     const rtnAnalyzeParam &param,
                                     _rtnInternalSortArea *sortArea,
                                     pmdEDUCB *cb,
                                     _SDB_DMSCB *dmsCB,
                                     _SDB_RTNCB *rtnCB,
                                     _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeCLStats ( const monCSSimple *pMonCS,
                                     const monCLSimple *pMonCL,
                                     const rtnAnalyzeParam &param,
                                     _rtnInternalSortArea *sortArea,
                                     pmdEDUCB *cb,
                                     _SDB_DMSCB *dmsCB,
                                     _SDB_RTNCB *rtnCB,
                                     _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeCLInternal ( dmsStorageUnit *pSU,
                                        dmsMBContext *mbContext,
                                        const rtnAnalyzeParam &param,
                                        BOOLEAN clearPlans,
                                        pmdEDUCB *cb,
                                        _SDB_DMSCB *dmsCB,
                                        _SDB_RTNCB *rtnCB,
                                        _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeIndexStat ( const monCSSimple *pMonCS,
                                       const monCLSimple *pMonCL,
                                       const monIndex *pMonIX,
                                       const rtnAnalyzeParam &param,
                                       BOOLEAN needUpdateCL,
                                       _rtnInternalSortArea *sortArea,
                                       pmdEDUCB *cb,
                                       _SDB_DMSCB *dmsCB,
                                       _SDB_RTNCB *rtnCB,
                                       _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnAnalyzeIndexInternal ( dmsStorageUnit *pSU,
                                           dmsMBContext *mbContext,
                                           ixmIndexCB *indexCB,
                                           const rtnAnalyzeParam &param,
                                           BOOLEAN needUpdateCL,
                                           BOOLEAN clearPlans,
                                           _rtnInternalSortArea *sortArea,
                                           pmdEDUCB *cb,
                                           _SDB_DMSCB *dmsCB,
                                           _SDB_RTNCB *rtnCB,
                                           _dpsLogWrapper *dpsCB ) ;

   static UINT32 _rtnGetSampleRecords ( UINT64 totalRecords,
                                        const rtnAnalyzeParam &param ) ;

   static BSONObj _rtnBuildAnalyzeOrder ( const BSONObj &keyPattern ) ;

   static INT32 _rtnBuildMCVSet ( dmsIndexStat *pIndexStat,
                                  dmsStorageUnit *pSU,
                                  dmsMBContext *mbContext,
                                  ixmIndexCB *indexCB,
                                  UINT32 sampleRecords,
                                  UINT64 totalRecords,
                                  BOOLEAN fullScan,
                                  _rtnInternalSortArea *sortArea,
                                  pmdEDUCB *cb ) ;

   static INT32 _rtnPostAnalyzeAll ( const rtnAnalyzeParam & param,
                                     _SDB_RTNCB *rtnCB,
                                     _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnPostAnalyzeCS ( const CHAR * pCSName,
                                    const rtnAnalyzeParam & param,
                                    _SDB_RTNCB *rtnCB,
                                    _dpsLogWrapper *dpsCB ) ;

   static INT32 _rtnFillCLStat( dmsStorageUnit *pSU,
                                     dmsMBContext *mbContext,
                                     const rtnAnalyzeParam &param,
                                     dmsCollectionStat *pCollectionStat,
                                     pmdEDUCB *cb,
                                     _SDB_DMSCB *dmsCB,
                                     _SDB_RTNCB *rtnCB ) ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNANALYZE, "rtnAnalyze" )
   INT32 rtnAnalyze ( const CHAR *pCSName,
                      const CHAR *pCLName,
                      const CHAR *pIXName,
                      const rtnAnalyzeParam &param,
                      pmdEDUCB *cb,
                      _SDB_DMSCB *dmsCB,
                      _SDB_RTNCB *rtnCB,
                      _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNANALYZE ) ;

      if ( SDB_ROLE_DATA != pmdGetDBRole() &&
           SDB_ROLE_STANDALONE != pmdGetDBRole() )
      {
         goto done ;
      }

      if ( NULL == rtnCB )
      {
         rtnCB = pmdGetKRCB()->getRTNCB() ;
      }

      if ( NULL == dmsCB )
      {
         dmsCB = pmdGetKRCB()->getDMSCB() ;
      }

      // fsync before analyze
      if ( dmsCB->getStorageService() &&
           ( SDB_ANALYZE_MODE_SAMPLE == param._mode ||
             SDB_ANALYZE_MODE_FULL == param._mode ) )
      {
         dmsCB->getStorageService()->fsync( TRUE, TRUE, cb ) ;
      }

      if ( NULL != pCSName )
      {
         rc = _rtnAnalyzeCS( pCSName, param, cb, dmsCB, rtnCB, dpsCB ) ;
      }
      else if ( NULL != pCLName )
      {
         if ( NULL == pIXName )
         {
            rc = _rtnAnalyzeCL( pCLName, param, cb, dmsCB, rtnCB, dpsCB ) ;
         }
         else
         {
            rc = _rtnAnalyzeIndex( pCLName, pIXName, param, cb, dmsCB, rtnCB, dpsCB ) ;
         }
      }
      else
      {
         rc = _rtnAnalyzeAll( param, cb, dmsCB, rtnCB, dpsCB ) ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to run analyze command, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNANALYZE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNRELOADCLSTATS, "rtnReloadCLStats" )
   INT32 rtnReloadCLStats ( dmsStorageUnit *pSU, dmsMBContext *mbContext,
                            pmdEDUCB *cb, _SDB_DMSCB *dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNRELOADCLSTATS ) ;

      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      dmsStatCache *pStatCache = pSU->getStatCache() ;

      if ( SDB_ROLE_DATA != pmdGetDBRole() &&
           SDB_ROLE_STANDALONE != pmdGetDBRole() )
      {
         goto done ;
      }

      if ( NULL != pStatCache )
      {
         monCSSimple monCS ;
         monCLSimple monCL ;

         pSU->dumpInfo( monCS, FALSE, FALSE, FALSE ) ;
         pSU->dumpInfo( monCL, mbContext, TRUE ) ;

         rc = _rtnReloadCLStats( &monCS, &monCL, pStatCache, cb, dmsCB, NULL ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to load statistics for collection "
                      "[%s], rc: %d", monCL._name, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNRELOADCLSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZEALL, "_rtnAnalyzeAll" )
   INT32 _rtnAnalyzeAll ( const rtnAnalyzeParam &param,
                          pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZEALL ) ;

      SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      MON_CS_SIM_LIST monCSList ;
      BOOLEAN lockDms = FALSE ;

      PD_LOG( PDINFO, "Analyze full node, mode [%d]", param._mode ) ;

      PD_CHECK( param._mode != SDB_ANALYZE_MODE_GENDFT,
                SDB_INVALIDARG, error, PDERROR,
                "Do not support generating default statistics on all "
                "collection spaces" ) ;

      // reload mode and clear mode do not write dps log, so they don't need to
      // check writable
      if ( param._mode != SDB_ANALYZE_MODE_RELOAD &&
           param._mode != SDB_ANALYZE_MODE_CLEAR )
      {
         rc = dmsCB->writable ( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
         lockDms = TRUE ;
      }

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD )
      {
         // Dump all information here, reload statistics is a quick process
         dmsCB->dumpInfo( monCSList, FALSE, TRUE, TRUE ) ;
      }
      else
      {
         // Dump all information later, analyze is a long process
         dmsCB->dumpInfo( monCSList, FALSE, FALSE, FALSE ) ;
      }

      if ( monCSList.empty() &&
           param._mode != SDB_ANALYZE_MODE_CLEAR )
      {
         // No collection space is found, need to do nothing except for clear
         // mode which need to clear cached plans as well
         INT32 tmpRC = _rtnPostAnalyzeAll( param, rtnCB, dpsCB ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to process after analyze all, "
                    "rc: %d", tmpRC ) ;
         }
         goto done ;
      }

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD )
      {
         rc = _rtnReloadAllStats( monCSList, param._needCheck,
                                  cb, dmsCB, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to reload statistics, rc: %d", rc ) ;
      }
      else if ( param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         // Clear cached plans and statistics
         dmsCB->clearSUCaches( monCSList,
                               DMS_EVENT_MASK_PLAN | DMS_EVENT_MASK_STAT ) ;
      }
      else
      {
         rc = _rtnAnalyzeAllStats( monCSList, param, cb, dmsCB, rtnCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to analyze, rc: %d", rc ) ;
      }

      rc = _rtnPostAnalyzeAll( param, rtnCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process after analyze all, "
                   "rc: %d", rc ) ;

   done :
      if ( lockDms )
      {
         dmsCB->writeDown( cb ) ;
         lockDms = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__RTNANALYZEALL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZECS, "_rtnAnalyzeCS" )
   INT32 _rtnAnalyzeCS ( const CHAR *pCSName,
                         const rtnAnalyzeParam &param,
                         pmdEDUCB *cb,
                         _SDB_DMSCB *dmsCB,
                         _SDB_RTNCB *rtnCB,
                         _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZECS ) ;

      SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      BOOLEAN suLocked = FALSE ;
      BOOLEAN lockDms = FALSE ;
      monCSSimple monCS ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      dmsStorageUnit *pSU = NULL ;
      dmsStatCache *pStatCache = NULL ;

      OSS_LATCH_MODE csLockType = SHARED ;

      PD_LOG( PDINFO, "Analyze collection space [%s] mode [%d]", pCSName,
              param._mode ) ;

      PD_CHECK( param._mode != SDB_ANALYZE_MODE_GENDFT,
                SDB_INVALIDARG, error, PDERROR,
                "Do not support generating default statistics on collection "
                "space" ) ;

      PD_CHECK( !dmsIsSysCSName( pCSName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection space [%s]", pCSName ) ;

      // reload mode and clear mode do not write dps log, so they don't need to
      // check writable
      if ( param._mode != SDB_ANALYZE_MODE_RELOAD &&
           param._mode != SDB_ANALYZE_MODE_CLEAR )
      {
         rc = dmsCB->writable ( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
         lockDms = TRUE ;
      }

      if ( param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         csLockType = EXCLUSIVE ;
      }

      rc = dmsCB->nameToSUAndLock( pCSName, suID, &pSU, csLockType,
                                   OSS_ONE_SEC ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            // Collection space doesn't have storage unit, but might contain
            // main-collections
            INT32 tmpRC = _rtnPostAnalyzeCS( pCSName, param, rtnCB, dpsCB ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to process after analyze CS [%s], "
                       "rc: %d", pCSName, tmpRC ) ;
            }
         }

         PD_LOG( PDERROR, "Failed to get collection space [%s], rc: %d",
                 pCSName, rc ) ;

         goto error ;
      }

      suLocked = TRUE ;

      // Check if statistics cache is ready for collection space
      pStatCache = pSU->getStatCache() ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDERROR,
                "No statistics manger in storage unit [%s]", pCSName ) ;

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD )
      {
         // Dump index list with collections, since reload statistics process
         // will be quick
         pSU->dumpInfo( monCS, FALSE, TRUE, TRUE ) ;
      }
      else if ( param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         // Do nothing
      }
      else
      {
         // Dump index list later, since analyze process will be long time
         pSU->dumpInfo( monCS, FALSE, TRUE, FALSE ) ;
      }

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD )
      {
         dmsCB->suUnlock( suID, csLockType ) ;
         suLocked = FALSE ;

         rc = _rtnReloadCSStats( &monCS, param._needCheck, cb, dmsCB, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to reload statistics for "
                      "collection space [%s], rc: %d", pCSName, rc ) ;

         // Make sure main-collection plans are removed
         rtnCB->getAPM()->invalidateSUPlans( pCSName ) ;
      }
      else if ( param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         pSU->getEventHolder()->onClearSUCaches( DMS_EVENT_MASK_PLAN |
                                                 DMS_EVENT_MASK_STAT ) ;

         dmsCB->suUnlock( suID, csLockType ) ;
         suLocked = FALSE ;

         // Make sure main-collection plans are removed
         rtnCB->getAPM()->invalidateSUPlans( pCSName ) ;
      }
      else
      {
         dmsCB->suUnlock( suID, csLockType ) ;
         suLocked = FALSE ;

         rc = _rtnAnalyzeCSStats( &monCS, param, NULL,
                                  cb, dmsCB, rtnCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to analyze collection space [%s], "
                      "rc: %d", pCSName, rc ) ;

         // Make sure main-collection plans are removed
         rtnCB->getAPM()->invalidateSUPlans( pCSName ) ;
      }

      rc = _rtnPostAnalyzeCS( pCSName, param, rtnCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process after analyze CS [%s], "
                   "rc: %d", pCSName, rc ) ;

   done :
      if ( suLocked )
      {
         dmsCB->suUnlock( suID, csLockType ) ;
      }
      if ( lockDms )
      {
         dmsCB->writeDown( cb ) ;
         lockDms = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__RTNANALYZECS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZECL, "_rtnAnalyzeCL" )
   INT32 _rtnAnalyzeCL ( const CHAR *pCLFullName,
                         const rtnAnalyzeParam &param,
                         pmdEDUCB *cb,
                         _SDB_DMSCB *dmsCB,
                         _SDB_RTNCB *rtnCB,
                         _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZECL ) ;

      SDB_ASSERT( pCLFullName, "pCLFullName is invalid" ) ;
      SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *pSU = NULL ;
      dmsStatCache *pStatCache = NULL ;
      dmsMBContext *mbContext = NULL ;

      const CHAR *pCSName = NULL ;
      const CHAR *pCLName = NULL ;

      monCSSimple monCS ;
      monCLSimple monCL ;

      INT32 clLockType = SHARED ;

      PD_LOG( PDINFO, "Analyze collection [%s] mode [%d]", pCLFullName,
              param._mode ) ;

      rc = rtnResolveCollectionNameAndLock ( pCLFullName, dmsCB, &pSU,
                                             &pCLName, suID, SHARED,
                                             OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

      pCSName = pSU->CSName() ;

      PD_CHECK( !dmsIsSysCSName( pCSName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection space [%s]", pCSName ) ;
      PD_CHECK( !dmsIsSysCLName( pCLName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection [%s]", pCLFullName ) ;

      // Check if statistics cache is ready for collection space
      pStatCache = pSU->getStatCache() ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDERROR,
                "No statistics manger in storage unit [%s]", pCSName ) ;

      pSU->dumpInfo( monCS, FALSE, FALSE, FALSE ) ;

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD ||
           param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         clLockType = EXCLUSIVE ;
      }

      rc = pSU->data()->getMBContext( &mbContext, pCLName, clLockType ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s], rc: %d",
                   pCLFullName, rc ) ;

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD )
      {
         dmsEventCLItem clItem( pCLName, mbContext->mbID(),
                                mbContext->clLID() ) ;

         // Get index list here, reload statistics will be a quick process
         pSU->dumpInfo( monCL, mbContext, TRUE ) ;

         rc = _rtnReloadCLStats( &monCS, &monCL, pStatCache,
                                 cb, dmsCB, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to reload statistics for "
                      "collection [%s], rc: %d", pCLFullName, rc ) ;

         // Clear cached plans based on old statistics
         pSU->getEventHolder()->onClearCLCaches( DMS_EVENT_MASK_PLAN, clItem ) ;
      }
      else if ( param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         dmsEventCLItem clItem( pCLName, mbContext->mbID(),
                                mbContext->clLID() ) ;

         // Clear cached plans and statistics
         pSU->getEventHolder()->onClearCLCaches( DMS_EVENT_MASK_PLAN |
                                                 DMS_EVENT_MASK_STAT, clItem ) ;
      }
      else
      {
         // Get index list later, analyze process will be long time
         pSU->dumpInfo( monCL, mbContext, FALSE ) ;

         // Unlock first
         pSU->data()->releaseMBContext( mbContext ) ;
         dmsCB->suUnlock( suID, SHARED ) ;
         pSU = NULL ;
         suID = DMS_INVALID_SUID ;
         mbContext = NULL ;

         rc = _rtnAnalyzeCLStats( &monCS, &monCL, param, NULL,
                                  cb, dmsCB, rtnCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to analyze statistics for "
                      "collection [%s], rc: %d", pCLFullName, rc ) ;
      }

   done :
      if ( pSU && mbContext )
      {
         pSU->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNANALYZECL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZEIX, "_rtnAnalyzeIndex" )
   INT32 _rtnAnalyzeIndex ( const CHAR *pCLFullName,
                            const CHAR *pIndexName,
                            const rtnAnalyzeParam &param,
                            pmdEDUCB *cb,
                            _SDB_DMSCB *dmsCB,
                            _SDB_RTNCB *rtnCB,
                            _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZEIX ) ;

      SDB_ASSERT( pCLFullName, "pCLFullName is invalid" ) ;
      SDB_ASSERT( pIndexName, "pIndexName is invalid" ) ;
      SDB_ASSERT( NULL != rtnCB, "rtnCB is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *pSU = NULL ;
      dmsMBContext *mbContext = NULL ;

      const CHAR *pCSName = NULL ;
      const CHAR *pCLName = NULL ;

      dmsStatSUMgr *pStatSUMgr = NULL ;
      dmsStatCache *pStatCache = NULL ;

      monCSSimple monCS ;
      monCLSimple monCL ;
      monIndex monIX ;

      INT32 clLockType = SHARED ;

      PD_LOG( PDEVENT, "Analyze index[%s:%s] mode[%d]",
              pCLFullName, pIndexName, param._mode ) ;

      pStatSUMgr = dmsCB->getStatSUMgr() ;
      PD_CHECK( pStatSUMgr && pStatSUMgr->initialized(), SDB_SYS, error,
                PDERROR, "Statistics SU is not initialized" ) ;

      rc = rtnResolveCollectionNameAndLock ( pCLFullName, dmsCB, &pSU,
                                             &pCLName, suID, SHARED,
                                             OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

      pCSName = pSU->CSName() ;

      PD_CHECK( !dmsIsSysCSName( pCSName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection space [%s]", pCSName ) ;
      PD_CHECK( !dmsIsSysCLName( pCLName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection [%s]", pCLFullName ) ;

      // Check if statistics cache is ready for collection space
      pStatCache = pSU->getStatCache() ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDERROR,
                "No statistics manger in storage unit [%s]", pCSName ) ;

      // Dump CS information
      pSU->dumpInfo( monCS, FALSE, FALSE, FALSE ) ;

      // Reload and clear mode acquire exclusive lock
      if ( param._mode == SDB_ANALYZE_MODE_RELOAD ||
           param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         clLockType = EXCLUSIVE ;
      }

      // Get mbContext
      rc = pSU->data()->getMBContext( &mbContext, pCLName, clLockType ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s], rc: %d",
                   pCLFullName, rc ) ;

      pSU->dumpInfo( monCL, mbContext, FALSE ) ;

      if ( param._mode == SDB_ANALYZE_MODE_RELOAD )
      {
         dmsEventCLItem clItem( pCLName, mbContext->mbID(),
                                mbContext->clLID() ) ;

         pSU->getIndex( mbContext, pIndexName, monIX ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s %s], rc: %d",
                      pCLFullName, pIndexName, rc ) ;

         rc = _rtnReloadIdxStat( &monCS, &monCL, &monIX, pStatCache,
                                 cb, dmsCB, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to reload statistics for "
                      "index [%s %s], rc: %d", pCLFullName, pIndexName, rc ) ;

         // Clear cached plans based on old statistics
         pSU->getEventHolder()->onClearCLCaches( DMS_EVENT_MASK_PLAN, clItem ) ;
      }
      else if ( param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         dmsEventCLItem clItem( pCLName, mbContext->mbID(),
                                mbContext->clLID() ) ;

         // Clear cached plans and statistics
         pSU->getEventHolder()->onClearCLCaches( DMS_EVENT_MASK_PLAN |
                                                 DMS_EVENT_MASK_STAT, clItem ) ;
      }
      else
      {
         rtnAnalyzeParam localParam( param ) ;

         pSU->getIndex( mbContext, pIndexName, monIX ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s %s], rc: %d",
                      pCLFullName, pIndexName, rc ) ;

         localParam._sampleRecords =
               _rtnGetSampleRecords( mbContext->mbStat()->_totalRecords.fetch(), param ) ;
         localParam._sampleByNum = FALSE ;

         // Unlock first
         pSU->data()->releaseMBContext( mbContext ) ;
         dmsCB->suUnlock( suID, SHARED ) ;
         pSU = NULL ;
         suID = DMS_INVALID_SUID ;
         mbContext = NULL ;

         rc = _rtnAnalyzeIndexStat( &monCS, &monCL, &monIX,
                                    localParam, TRUE, NULL,
                                    cb, dmsCB, rtnCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to analyze statistics for "
                      "index [%s %s], rc: %d", pCLFullName, pIndexName, rc ) ;
      }

   done :
      if ( pSU && mbContext )
      {
         pSU->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNANALYZEIX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRELOADALLSTATS, "_rtnReloadAllStats" )
   INT32 _rtnReloadAllStats ( const MON_CS_SIM_LIST &monCSList,
                              BOOLEAN needCheck,
                              pmdEDUCB *cb,
                              _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRELOADALLSTATS ) ;

      dmsStatCacheMap nodeStatMap ;

      dmsStatSUMgr *pStatSUMgr = dmsCB->getStatSUMgr() ;

      PD_CHECK( pStatSUMgr && pStatSUMgr->initialized(), SDB_SYS, error,
                PDERROR, "Statistics SU is not initialized" ) ;

      rc = pStatSUMgr->loadAllCollectionStats( monCSList, nodeStatMap,
                                               cb, dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load collection statistics, "
                   "rc: %d", rc ) ;

      rc = pStatSUMgr->loadAllIndexStats( monCSList, nodeStatMap, cb, dmsCB,
                                          rtnCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load index statistics, rc: %d",
                   rc ) ;

      for ( dmsStatCacheMap::iterator iterSUStat = nodeStatMap.begin() ;
            iterSUStat != nodeStatMap.end() ;
            ++ iterSUStat )
      {
         const monCSSimple *pMonCS = NULL ;

         const CHAR *pCSName = iterSUStat->first._pString ;
         dmsStatCache *pStatCache = iterSUStat->second ;

         if ( NULL == pStatCache )
         {
            continue ;
         }

         pMonCS = monCSSimple::getCollectionSpace( monCSList, pCSName ) ;
         if ( NULL == pMonCS )
         {
            continue ;
         }

         // Need check logical IDs again, since we unlock the objects
         // after dumping objects
         rc = _rtnReplaceCSStats( pMonCS, pStatCache, needCheck, dmsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to replace collection space [%s], "
                    "rc: %d", pCSName, rc ) ;
            rc = SDB_OK ;
         }
      }

   done :
      if ( nodeStatMap.size() > 0 )
      {
         dmsStatCacheMap::iterator iterSUStat = nodeStatMap.begin() ;
         while ( iterSUStat != nodeStatMap.end() )
         {
            dmsStatCache *pStatCache = iterSUStat->second ;
            iterSUStat = nodeStatMap.erase( iterSUStat ) ;
            SAFE_OSS_DELETE( pStatCache ) ;
         }
      }
      PD_TRACE_EXITRC( SDB__RTNRELOADALLSTATS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREPLACECSSTATS, "_rtnReplaceCSStats" )
   INT32 _rtnReplaceCSStats ( const monCSSimple *pMonCS,
                              dmsStatCache *pInputStatCache,
                              BOOLEAN needCheck,
                              _SDB_DMSCB *dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNREPLACECSSTATS ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;

      const CHAR *pCSName = pMonCS->_name ;
      dmsStorageUnit *pSU = NULL ;
      dmsStatCache *pStatCache = NULL ;

      BOOLEAN suLocked = FALSE ;

      dmsEventSUItem suItem( pMonCS->_name, pMonCS->_suID,
                             pMonCS->_logicalID ) ;

      rc = dmsCB->verifySUAndLock( &suItem, &pSU, EXCLUSIVE, OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection space [%s], rc: %d",
                   pCSName, rc ) ;

      suLocked = TRUE ;

      // Check if statistics cache is ready for collection space
      pStatCache = pSU->getStatCache() ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDERROR,
                "No statistics manger in storage unit [%s]", pCSName ) ;

      // Clear current statistics caches
      pSU->getEventHolder()->onClearSUCaches( DMS_EVENT_MASK_STAT ) ;

      if ( pInputStatCache )
      {
         for ( UINT16 unitID = 0 ; unitID < pInputStatCache->getSize() ; unitID ++ )
         {
            dmsCollectionStat *pCollectionStat =
                  (dmsCollectionStat *)pInputStatCache->getCacheUnit( unitID ) ;
            if ( pCollectionStat != NULL &&
                 pStatCache->addCacheUnit( pCollectionStat, TRUE, needCheck ) )
            {
               pInputStatCache->removeCacheUnit( unitID, FALSE ) ;
            }
         }
         pStatCache->setStatus( UTIL_SU_CACHE_UNIT_STATUS_CACHED ) ;
      }

      // Clear current plan caches based on old statistics
      pSU->getEventHolder()->onClearSUCaches( DMS_EVENT_MASK_PLAN ) ;

   done :
      if ( suLocked )
      {
         dmsCB->suUnlock( pMonCS->_suID, EXCLUSIVE ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNREPLACECSSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRELOADCSSTAT, "_rtnReloadCSStats" )
   INT32 _rtnReloadCSStats ( const monCSSimple *pMonCS,
                             BOOLEAN needCheck,
                             pmdEDUCB *cb,
                             _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRELOADCSSTAT ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;

      const CHAR *pCSName = pMonCS->_name ;
      dmsStatCache statMap( NULL ) ;
      dmsStatSUMgr *pStatSUMgr = dmsCB->getStatSUMgr() ;

      PD_CHECK( pStatSUMgr && pStatSUMgr->initialized(), SDB_SYS, error,
                PDERROR, "Statistics SU is not initialized" ) ;

      rc = pStatSUMgr->loadSUCollectionStats( pMonCS, &statMap, cb, dmsCB,
                                              rtnCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load collection statistics for "
                   "collection space [%s], rc: %d", pCSName, rc ) ;

      rc = pStatSUMgr->loadSUIndexStats( pMonCS, &statMap, cb, dmsCB,
                                         rtnCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load index statistics for "
                   "collection space [%s], rc: %d", pCSName, rc ) ;

      // Need check logical IDs again, since we unlock the objects
      // after dumping objects
      rc = _rtnReplaceCSStats( pMonCS, &statMap, needCheck, dmsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to replace collection space [%s], "
                 "rc: %d", pCSName, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNRELOADCSSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRELOADCLSTAT, "_rtnReloadCLStats" )
   INT32 _rtnReloadCLStats ( const monCSSimple *pMonCS,
                             const monCLSimple *pMonCL,
                             dmsStatCache *pStatCache,
                             pmdEDUCB *cb,
                             _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRELOADCSSTAT ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      const CHAR *pCLFullName = pMonCL->_name ;
      dmsStatSUMgr *pStatSUMgr = dmsCB->getStatSUMgr() ;

      PD_CHECK( pStatSUMgr && pStatSUMgr->initialized(), SDB_SYS, error,
                PDERROR, "Statistics SU is not initialized" ) ;

      pStatCache->removeCacheUnit( pMonCL->_blockID, TRUE ) ;

      rc = pStatSUMgr->loadCollectionStat( pMonCS, pMonCL, pStatCache,
                                           cb, dmsCB, rtnCB) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load collection statistics for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

      rc = pStatSUMgr->loadCLIndexStats( pMonCS, pMonCL, pStatCache,
                                         cb, dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load index statistics for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNRELOADCSSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNRELOADIDXSTAT, "_rtnReloadIdxStat" )
   INT32 _rtnReloadIdxStat ( const monCSSimple *pMonCS,
                             const monCLSimple *pMonCL,
                             const monIndex *pMonIX,
                             dmsStatCache *pStatCache,
                             pmdEDUCB *cb,
                             _SDB_DMSCB *dmsCB,
                             _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNRELOADCSSTAT ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;
      SDB_ASSERT( pMonIX, "pMonIX is invalid" ) ;
      SDB_ASSERT( pStatCache, "pStatCache is invalid" ) ;

      const CHAR *pCLFullName = pMonCL->_name ;
      dmsExtentID indexLID = pMonIX->_indexLID ;

      dmsCollectionStat *pCollectionStat = NULL ;
      dmsStatSUMgr *pStatSUMgr = dmsCB->getStatSUMgr() ;

      PD_CHECK( pStatSUMgr && pStatSUMgr->initialized(), SDB_SYS, error,
                PDERROR, "Statistics SU is not initialized" ) ;

      pCollectionStat =
            (dmsCollectionStat *)pStatCache->getCacheUnit( pMonCL->_blockID ) ;

      // The collection statistics is empty, try reload first
      if ( NULL == pCollectionStat )
      {
         rc = pStatSUMgr->loadCollectionStat( pMonCS, pMonCL, pStatCache,
                                              cb, dmsCB, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load collection "
                      "statistics for collection [%s], rc: %d",
                      pCLFullName, rc ) ;
         pCollectionStat =
               (dmsCollectionStat *)pStatCache->getCacheUnit( pMonCL->_blockID ) ;
      }

      PD_CHECK( pCollectionStat, SDB_INVALIDARG, error, PDERROR,
                "No statistics found for collection [%s]", pCLFullName ) ;

      pCollectionStat->removeIndexStat( indexLID, TRUE ) ;

      rc = pStatSUMgr->loadIndexStats( pMonCS, pMonCL, pMonIX, pStatCache,
                                       cb, dmsCB, rtnCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load index statistics for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNRELOADCSSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZEALLSTATS, "_rtnAnalyzeAllStats" )
   INT32 _rtnAnalyzeAllStats ( const MON_CS_SIM_LIST &monCSList,
                               const rtnAnalyzeParam &param,
                               pmdEDUCB *cb,
                               _SDB_DMSCB *dmsCB,
                               _SDB_RTNCB *rtnCB,
                               _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZEALLSTATS ) ;

      _rtnInternalSortArea sortArea ;
      rc = sortArea.init( RTN_ANALYZE_SORT_BUF_SIZE ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize sort area failed[%d]", rc ) ;

      for ( MON_CS_SIM_LIST::iterator iterCS = monCSList.begin() ;
            iterCS != monCSList.end() ;
            ++ iterCS )
      {
         monCSSimple monCS = (*iterCS) ;

         const CHAR *pCSName = monCS._name ;
         dmsEventSUItem suItem( pCSName, monCS._suID, monCS._logicalID ) ;
         dmsStorageUnit *pSU = NULL ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         // Dump collection list
         rc = dmsCB->verifySUAndLock( &suItem, &pSU, SHARED, OSS_ONE_SEC ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Skip analyze collection space [%s], rc: %d",
                    pCSName, rc ) ;
            rc = SDB_OK ;
            continue ;
         }
         pSU->dumpInfo( monCS._clList, FALSE, FALSE ) ;
         dmsCB->suUnlock( suItem._suID, SHARED ) ;
         pSU = NULL ;

         rc = _rtnAnalyzeCSStats( &monCS, param, &sortArea,
                                  cb, dmsCB, rtnCB, dpsCB ) ;
         if ( SDB_APP_INTERRUPT == rc )
         {
            PD_LOG( PDERROR, "Failed to analyze collection space [%s], rc: %d",
                    pCSName, rc ) ;
            goto error ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Skip analyze collection space [%s], rc: %d",
                    pCSName, rc ) ;
            rc = SDB_OK ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNANALYZEALLSTATS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZECSSTATS, "_rtnAnalyzeCSStats" )
   INT32 _rtnAnalyzeCSStats ( const monCSSimple *pMonCS,
                              const rtnAnalyzeParam &param,
                              _rtnInternalSortArea *sortArea,
                              pmdEDUCB *cb,
                              _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB,
                              _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZECSSTATS ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;

      _rtnInternalSortArea sortAreaTmp ;

      if ( pMonCS->_clList.empty() )
      {
         goto done ;
      }

      if ( !sortArea )
      {
         sortArea = &sortAreaTmp ;
         rc = sortArea->init( RTN_ANALYZE_SORT_BUF_SIZE ) ;
         PD_RC_CHECK( rc, PDERROR, "Initialize sort area failed[%d]", rc ) ;
      }

      for ( MON_CL_SIM_VEC::const_iterator iterCL = pMonCS->_clList.begin() ;
            iterCL != pMonCS->_clList.end() ;
            ++ iterCL )
      {
         const monCLSimple *pMonCL = &(*iterCL) ;
         const CHAR *pCLFullName = pMonCL->_name ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = _rtnAnalyzeCLStats( pMonCS, pMonCL, param, sortArea,
                                  cb, dmsCB, rtnCB, dpsCB ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc ||
              SDB_APP_INTERRUPT == rc )
         {
            PD_LOG( PDERROR, "Failed to analyze collection [%s], rc: %d",
                    pCLFullName, rc ) ;
            goto error ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Skip analyze collection [%s], rc: %d",
                    pCLFullName, rc ) ;
            rc = SDB_OK ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNANALYZECSSTATS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZECLSTATS, "_rtnAnalyzeCLStats" )
   INT32 _rtnAnalyzeCLStats ( const monCSSimple *pMonCS,
                              const monCLSimple *pMonCL,
                              const rtnAnalyzeParam &param,
                              _rtnInternalSortArea *sortArea,
                              pmdEDUCB *cb,
                              _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB,
                              _dpsLogWrapper *dpsCB )
   {
       INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZECLSTATS ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;

      const CHAR *pCSName = pMonCS->_name ;
      const CHAR *pCLFullName = pMonCL->_name ;
      const CHAR *pCLName = pMonCL->_clname ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *pSU = NULL ;
      dmsMBContext *mbContext = NULL ;

      _rtnInternalSortArea sortAreaTmp ;

      rtnAnalyzeParam localParam( param ) ;

      MON_IDX_LIST monIdxList ;
      BOOLEAN writable = FALSE ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->nameToSUAndLock( pCSName, suID, &pSU, SHARED, OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

      PD_CHECK( pSU->CSID() == pMonCS->_suID &&
                pSU->LogicalCSID() == pMonCS->_logicalID,
                SDB_DMS_CS_NOTEXIST, error, PDERROR,
                "Collection space [%s] had been updated: "
                "original [ su ID: %d, logical ID: %u ], "
                "now [ su ID: %d, logical ID: %u ]", pCSName,
                pMonCS->_suID, pMonCS->_logicalID,
                pSU->CSID(), pSU->LogicalCSID() ) ;

      rc = pSU->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                   pCLFullName, rc ) ;

      PD_CHECK( mbContext->mbID() == pMonCL->_blockID &&
                mbContext->clLID() == pMonCL->_logicalID,
                SDB_DMS_NOTEXIST, error, PDERROR,
                "Collection [%s] had been updated: "
                "orig [ block ID: %u, logical ID: %u ], "
                "now [ block ID: %u, logical ID: %u ]", pCLFullName,
                pMonCL->_blockID, pMonCL->_logicalID,
                mbContext->mbID(), mbContext->clLID() ) ;

      // Dump index list
      pSU->getIndexes( mbContext, monIdxList ) ;

      // Analyze collection
      localParam._sampleRecords =
            _rtnGetSampleRecords( mbContext->mbStat()->_totalRecords.fetch(), param ) ;
      localParam._sampleByNum = TRUE ;

      rc = _rtnAnalyzeCLInternal( pSU, mbContext, localParam, TRUE, cb,
                                  dmsCB, rtnCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to analyze collection [%s.%s], rc: %d",
                   pCSName, pCLName, rc ) ;

      pSU->data()->releaseMBContext( mbContext ) ;
      dmsCB->suUnlock( suID, SHARED ) ;
      dmsCB->writeDown( cb ) ;
      pSU = NULL ;
      suID = DMS_INVALID_SUID ;
      mbContext = NULL ;
      writable = FALSE ;

      if ( monIdxList.empty() )
      {
         goto done ;
      }

      if ( !sortArea && localParam._sampleRecords > 0 )
      {
         sortArea = &sortAreaTmp ;
         rc = sortArea->init( RTN_ANALYZE_SORT_BUF_SIZE ) ;
         PD_RC_CHECK( rc, PDERROR, "Initialize sort area failed[%d]", rc ) ;
      }

      for ( MON_IDX_LIST::const_iterator iterIdx = monIdxList.begin() ;
            iterIdx != monIdxList.end() ;
            ++ iterIdx )
      {
         const monIndex *pMonIX = &(*iterIdx) ;
         const CHAR *pIXName = pMonIX->getIndexName() ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = _rtnAnalyzeIndexStat( pMonCS, pMonCL, pMonIX,
                                    localParam, FALSE, sortArea,
                                    cb, dmsCB, rtnCB, dpsCB ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc ||
              SDB_DMS_NOTEXIST == rc ||
              SDB_APP_INTERRUPT == rc )
         {
            PD_LOG( PDERROR, "Failed to analyze index [%s %s], rc: %d",
                    pCLFullName, pIXName, rc ) ;
            goto error ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Skip analyze index [%s %s], rc: %d",
                   pCLFullName, pIXName, rc ) ;
            rc = SDB_OK ;
         }
      }

      // Notify backup nodes to clear old cached statistics
      rc = rtnAnalyzeDpsLog( NULL, pCLFullName, NULL, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write analyze log, rc: %d", rc ) ;

   done :
      if ( pSU && mbContext )
      {
         pSU->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNANALYZECLSTATS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZECL_INT, "_rtnAnalyzeCLInternal" )
   INT32 _rtnAnalyzeCLInternal ( dmsStorageUnit *pSU,
                                 dmsMBContext *mbContext,
                                 const rtnAnalyzeParam &param,
                                 BOOLEAN clearPlans,
                                 pmdEDUCB *cb,
                                 _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB,
                                 _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZECL_INT ) ;

      SDB_ASSERT( pSU, "pSU is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      const CHAR *pCSName = pSU->CSName() ;
      const CHAR *pCLName = mbContext->mb()->_collectionName ;

      dmsStatSUMgr *pStatSUMgr = dmsCB->getStatSUMgr() ;
      dmsStatCache *pStatCache = NULL ;
      dmsCollectionStat *pCollectionStat = NULL ;
      const dmsCollectionStat *pTmpCLStat = NULL ;

      pStatCache = pSU->getStatCache() ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDERROR,
                "No statistics manger in storage unit [%s]", pCSName ) ;

      // Do not set the version, set it when lock collection exclusive
      pCollectionStat = SDB_OSS_NEW dmsCollectionStat( pCSName, pCLName,
                                                       pSU->LogicalCSID(),
                                                       mbContext->mbID(),
                                                       mbContext->clLID(),
                                                       0 ) ;
      PD_CHECK( pCollectionStat, SDB_OOM, error, PDERROR,
                "Failed to allocate memory for collection statistics [%s.%s]",
                pCSName, pCLName ) ;


      if ( SDB_ANALYZE_MODE_SAMPLE == param._mode ||
           SDB_ANALYZE_MODE_FULL == param._mode )
      {
         rc = _rtnFillCLStat( pSU, mbContext, param, pCollectionStat, cb, dmsCB, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to fill collection [%s.%s] statistics, "
                      "rc: %d", pCSName, pCLName, rc ) ;
      }
      else
      {
         pCollectionStat->setTotalRecords( mbContext->mbStat()->_totalRecords.fetch() ) ;
         pCollectionStat->setSampleRecords( param._sampleRecords ) ;
         pCollectionStat->setTotalDataPages( mbContext->mbStat()->_totalDataPages ) ;
         pCollectionStat->setTotalDataSize( mbContext->mbStat()->_totalOrgDataLen.fetch() ) ;
         pCollectionStat->setAvgNumFields( DMS_STAT_DEF_AVG_NUM_FIELDS ) ;
      }

      rc = pCollectionStat->postInit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize collection statistics, "
                   "rc: %d", rc ) ;

      rc = mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s.%s], rc: %d",
                   pCSName, pCLName, rc ) ;

      // The collection is locked exclusive, set the version
      pCollectionStat->setCreateTime( ossGetCurrentMilliseconds() ) ;

      PD_CHECK( pStatCache->addCacheUnit( pCollectionStat, TRUE, FALSE ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to add collection statistics [%s.%s]",
                pCSName, pCLName ) ;

      // The collection statistics is inserted into statistics cache,
      // let the cache manage it
      pTmpCLStat = pCollectionStat ;
      pCollectionStat = NULL ;

      rc = pStatSUMgr->updateCollectionStat( pTmpCLStat, cb, dmsCB,
                                             rtnCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection statistics "
                   "[%s.%s], rc: %d", pCSName, pCLName, rc ) ;

      if ( clearPlans )
      {
         dmsEventCLItem clItem( pCLName, mbContext->mbID(),
                                mbContext->clLID() ) ;

         // Clear cached plans based on old statistics
         pSU->getEventHolder()->onClearCLCaches( DMS_EVENT_MASK_PLAN, clItem ) ;
      }

   done :
      SAFE_OSS_DELETE( pCollectionStat ) ;
      PD_TRACE_EXITRC( SDB__RTNANALYZECL_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZEIXSTAT, "_rtnAnalyzeIndexStat" )
   INT32 _rtnAnalyzeIndexStat ( const monCSSimple *pMonCS,
                                const monCLSimple *pMonCL,
                                const monIndex *pMonIX,
                                const rtnAnalyzeParam &param,
                                BOOLEAN needUpdateCL,
                                _rtnInternalSortArea *sortArea,
                                pmdEDUCB *cb,
                                _SDB_DMSCB *dmsCB,
                                _SDB_RTNCB *rtnCB,
                                _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZEIXSTAT ) ;

      SDB_ASSERT( pMonCS, "pMonCS is invalid" ) ;
      SDB_ASSERT( pMonCL, "pMonCL is invalid" ) ;
      SDB_ASSERT( pMonIX, "pMonIX is invalid" ) ;

      const CHAR *pCSName = pMonCS->_name ;
      const CHAR *pCLFullName = pMonCL->_name ;
      const CHAR *pCLName = pMonCL->_clname ;
      const CHAR *pIXName = pMonIX->getIndexName() ;

      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *pSU = NULL ;
      dmsMBContext *mbContext = NULL ;
      dmsExtentID indexExtID = DMS_INVALID_EXTENT ;

      _rtnInternalSortArea sortAreaTmp ;
      BOOLEAN writable = FALSE ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->nameToSUAndLock( pCSName, suID, &pSU, SHARED, OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space for "
                   "collection [%s], rc: %d", pCLFullName, rc ) ;

      PD_CHECK( pSU->CSID() == pMonCS->_suID &&
                pSU->LogicalCSID() == pMonCS->_logicalID,
                SDB_DMS_CS_NOTEXIST, error, PDERROR,
                "Collection space [%s] had been updated: "
                "original [ su ID: %d, logical ID: %u ], "
                "now [ su ID: %d, logical ID: %u ]", pCSName,
                pMonCS->_suID, pMonCS->_logicalID,
                pSU->CSID(), pSU->LogicalCSID() ) ;

      rc = pSU->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                   pCLFullName, rc ) ;

      PD_CHECK( mbContext->mbID() == pMonCL->_blockID &&
                mbContext->clLID() == pMonCL->_logicalID,
                SDB_DMS_NOTEXIST, error, PDERROR,
                "Collection [%s] had been updated: "
                "orig [ block ID: %u, logical ID: %u ], "
                "now [ block ID: %u, logical ID: %u ]", pCLFullName,
                pMonCL->_blockID, pMonCL->_logicalID,
                mbContext->mbID(), mbContext->clLID() ) ;

      rc = pSU->index()->getIndexCBExtent( mbContext, pIXName, indexExtID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s] from "
                   "collection [%s], rc: %d", pIXName, pCLFullName, rc ) ;

      try
      {
         ixmIndexCB indexCB ( indexExtID, pSU->index(), mbContext ) ;

         PD_CHECK( indexCB.isInitialized(),
                   SDB_DMS_INIT_INDEX, error, PDERROR,
                   "Index [%s %s] is invalid", pCLFullName, pIXName ) ;
         PD_CHECK( indexCB.getFlag() == IXM_INDEX_FLAG_NORMAL,
                   SDB_IXM_UNEXPECTED_STATUS, error, PDDEBUG,
                   "Index [%s %s] is not normal status", pCLFullName,
                   pIXName ) ;

         PD_CHECK( indexCB.getLogicalID() == pMonIX->_indexLID &&
                   indexCB.isSameDef( pMonIX->_indexDef , TRUE ),
                   SDB_IXM_NOTEXIST, error, PDERROR,
                   "Index [%s %s] had been updated: "
                   "orig [ logical ID: %u, define: %s ], "
                   "now [ logical ID: %u, define: %s ]",
                   pCLFullName, pIXName,
                   pMonIX->_indexLID,
                   pMonIX->_indexDef.toString( FALSE, TRUE ).c_str(),
                   indexCB.getLogicalID(),
                   indexCB.getDef().toString( FALSE, TRUE ).c_str() ) ;

         if ( !sortArea )
         {
            sortArea = &sortAreaTmp ;
            rc = sortArea->init( RTN_ANALYZE_SORT_BUF_SIZE ) ;
            PD_RC_CHECK( rc, PDERROR, "Initialize sort area failed[%d]", rc ) ;
         }

         rc = _rtnAnalyzeIndexInternal( pSU, mbContext, &indexCB,
                                        param, needUpdateCL, TRUE,
                                        sortArea, cb, dmsCB, rtnCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to analyze index [%s.%s %s], "
                      "rc: %d", pCSName, pCLName, pIXName, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // Notify backup nodes to clear old cached statistics
      rc = rtnAnalyzeDpsLog( NULL, pCLFullName, pIXName, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write analyze log, rc: %d", rc ) ;

   done :
      if ( pSU && mbContext )
      {
         pSU->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNANALYZEIXSTAT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNANALYZEIX_INT, "_rtnAnalyzeIndexInternal" )
   INT32 _rtnAnalyzeIndexInternal ( dmsStorageUnit *pSU,
                                    dmsMBContext *mbContext,
                                    ixmIndexCB *indexCB,
                                    const rtnAnalyzeParam &param,
                                    BOOLEAN needUpdateCL,
                                    BOOLEAN clearPlans,
                                    _rtnInternalSortArea *sortArea,
                                    pmdEDUCB *cb,
                                    _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB,
                                    _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNANALYZEIX_INT ) ;

      SDB_ASSERT( pSU, "pSU is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( indexCB, "indexCB is invalid" ) ;

      const CHAR *pCSName = pSU->CSName() ;
      const CHAR *pCLName = mbContext->mb()->_collectionName ;
      const CHAR *pIXName = indexCB->getName() ;

      dmsStatSUMgr *pStatSUMgr = dmsCB->getStatSUMgr() ;
      dmsStatCache *pStatCache = NULL ;

      dmsIndexStat *pIndexStat = NULL ;
      const dmsIndexStat *pTmpIdxStat = NULL ;

      PD_CHECK( pStatSUMgr && pStatSUMgr->initialized(), SDB_SYS, error,
                PDERROR, "Statistics SU is not initialized" ) ;

      pStatCache = pSU->getStatCache() ;
      PD_CHECK( pStatCache, SDB_INVALIDARG, error, PDERROR,
                "No statistics manger in storage unit [%s]", pCSName ) ;

      if ( needUpdateCL )
      {
         // Re-analyze collection statistics if needed
         rc = _rtnAnalyzeCLInternal( pSU, mbContext, param, FALSE, cb,
                                     dmsCB, rtnCB, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to analyze collection [%s.%s], "
                      "rc: %d", pCSName, pCLName, rc ) ;

         // Make statistics of other indexes are loaded
         rtnReloadCLStats( pSU, mbContext, cb, dmsCB ) ;

         // Lock shared to allow parallel reading during analyze index
         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s.%s], rc: %d",
                      pCSName, pCLName, rc ) ;
      }

      try
      {
         UINT64 totalRecords = mbContext->mbStat()->_totalRecords.fetch() ;

         pIndexStat = SDB_OSS_NEW dmsIndexStat( pCSName, pCLName, pIXName,
                                                pSU->LogicalCSID(),
                                                mbContext->mbID(),
                                                mbContext->clLID(), 0 ) ;
         PD_CHECK( pIndexStat, SDB_OOM, error, PDERROR,
                   "Failed to allocate memory for index statistics "
                   "[%s.%s %s]", pCSName, pCLName, pIXName ) ;

         pIndexStat->setKeyPattern( indexCB->keyPattern() ) ;
         pIndexStat->setUnique( indexCB->unique() ) ;
         pIndexStat->setIndexLogicalID( indexCB->getLogicalID() ) ;

         if ( param._sampleRecords > 0 )
         {
            BOOLEAN fullScan = ( SDB_ANALYZE_MODE_FULL == param._mode ?
                                 TRUE : FALSE ) ;
            rc = _rtnBuildMCVSet( pIndexStat, pSU, mbContext, indexCB,
                                  param._sampleRecords, totalRecords, fullScan,
                                  sortArea, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build MCV set, rc: %d", rc ) ;
         }
         else
         {
            pIndexStat->setSampleRecords( 0 ) ;
         }

         pIndexStat->setTotalRecords( totalRecords ) ;

         rc = pIndexStat->postInit() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize index statistics, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s.%s], rc: %d",
                   pCSName, pCLName, rc ) ;

      // The collection is locked exclusive, set the version
      pIndexStat->setCreateTime( ossGetCurrentMilliseconds() ) ;

      PD_CHECK( pStatCache->addCacheSubUnit( pIndexStat, TRUE, FALSE ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to add index statistics [%s.%s %s]",
                pCSName, pCLName, pIXName ) ;

      // The collection statistics is inserted into statistics cache,
      // let the cache manage it
      pTmpIdxStat = pIndexStat ;
      pIndexStat = NULL ;

      rc = pStatSUMgr->updateIndexStat( pTmpIdxStat, cb, dmsCB, rtnCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update index statistics "
                   "[%s.%s %s], rc: %d", pCSName, pCLName, pIXName, rc ) ;

      if ( clearPlans )
      {
         dmsEventCLItem clItem( pCLName, mbContext->mbID(),
                                mbContext->clLID() ) ;

         // Clear cached plans based on old statistics
         pSU->getEventHolder()->onClearCLCaches( DMS_EVENT_MASK_PLAN, clItem ) ;
      }

   done :
      SAFE_OSS_DELETE( pIndexStat ) ;
      PD_TRACE_EXITRC( SDB__RTNANALYZEIX_INT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   UINT32 _rtnGetSampleRecords ( UINT64 totalRecords,
                                 const rtnAnalyzeParam &param )
   {
      UINT32 sampleRecords = 0 ;
      if ( param._mode == SDB_ANALYZE_MODE_GENDFT ||
           param._mode == SDB_ANALYZE_MODE_RELOAD ||
           param._mode == SDB_ANALYZE_MODE_CLEAR )
      {
         // Samples are not needed in these modes
         sampleRecords = 0 ;
      }
      else
      {
         if ( param._sampleByNum )
         {
            sampleRecords = param._sampleRecords ;
         }
         else
         {
            sampleRecords = (UINT32)ceil( (double)totalRecords / 100.0 *
                                          (double)param._samplePercent ) ;
         }

         sampleRecords = DMS_STAT_ROUND( sampleRecords,
                                         SDB_ANALYZE_SAMPLE_MIN,
                                         SDB_ANALYZE_SAMPLE_MAX ) ;

         sampleRecords = OSS_MIN( sampleRecords, totalRecords ) ;
      }

      return sampleRecords ;
   }

   BSONObj _rtnBuildAnalyzeOrder ( const BSONObj &keyPattern )
   {
      BSONObjBuilder orderBuilder ;
      BSONObjIterator iterKey( keyPattern ) ;

      while ( iterKey.more() )
      {
         BSONElement beKey = iterKey.next() ;
         orderBuilder.append( beKey.fieldName(), 1 ) ;
      }

      return orderBuilder.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNBUILDMCVSET, "_rtnBuildMCVSet" )
   INT32 _rtnBuildMCVSet ( dmsIndexStat *pIndexStat,
                           dmsStorageUnit *pSU,
                           dmsMBContext *mbContext,
                           ixmIndexCB *indexCB,
                           UINT32 sampleRecords,
                           UINT64 totalRecords,
                           BOOLEAN fullScan,
                           _rtnInternalSortArea *sortArea,
                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNBUILDMCVSET ) ;

      SDB_ASSERT( pSU, "pSU is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( indexCB, "indexCB is invalid" ) ;
      SDB_ASSERT( sortArea, "Sort area is invalid" ) ;

      const CHAR *pCSName = pSU->CSName() ;
      const CHAR *pCLName = mbContext->mb()->_collectionName ;
      const CHAR *pIXName = indexCB->getName() ;

      BSONObj boOrder = _rtnBuildAnalyzeOrder( indexCB->keyPattern() ) ;
      UINT32 sortCount = 0, prevCount = 0 ;
      BSONObj prevKey, dummy ;
      double fraction = 0.0 ;

      _rtnSortTuple *tuple = NULL ;
      UINT64 totalIndexSize = 0, freeIndexSize = 0 ;
      UINT32 totalPages = 0 ;
      UINT32 pageSize = pSU->getPageSize() ;

      sortArea->_tupleDirectory.clear() ;
      sortArea->_tupleBuff.clear() ;

      _rtnInternalSorting sorter( boOrder, &(sortArea->_tupleDirectory),
                                  &(sortArea->_tupleBuff), -1 ) ;

      std::shared_ptr<IIndex> idxPtr ;
      std::unique_ptr<IIndexCursor> cursorPtr ;

      rc = pSU->index()->getIndex( mbContext, indexCB, cb, idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s.%s], index [%s], rc: %d",
                   pCSName, pCLName, pIXName, rc ) ;

      // get file size
      rc = idxPtr->getIndexStats( totalIndexSize, freeIndexSize, FALSE, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s.%s] index [%s] stats, "
                   "rc: %d", pCSName, pCLName, pIXName, rc ) ;

      totalPages = totalIndexSize / pageSize ;
      pIndexStat->setIndexPages( totalPages ) ;

      // initialize index levels as 1, will calculate it later
      pIndexStat->setIndexLevels( 1 ) ;

      // get samples
      if ( sampleRecords < totalRecords )
      {
         rc = idxPtr->createIndexSampleCursor( cursorPtr, sampleRecords, cb ) ;
      }
      else
      {
         // no enough records, use full scan
         keystring::keyString emptyKey ;
         rc = idxPtr->createIndexCursor( cursorPtr, emptyKey, FALSE, TRUE, cb ) ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_EOC != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to create sample cursor for "
                         "collection [%s.%s] index [%s], rc: %d",
                         pCSName, pCLName, pIXName, rc ) ;
         }
         rc = SDB_OK ;
      }
      else
      {
         ossPoolSet<dmsRecordID> ridSet ;
         UINT64 totalSampleCounts = 0 ;
         UINT64 totalKeySize = 0 ;
         while ( TRUE )
         {
            keystring::keyString key ;
            BSONObj keyObj ;
            dmsRecordID rid ;

            rc = cursorPtr->getCurrentKeyString( key ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get key string for collection [%s.%s] "
                         "index [%s], rc: %d", pCSName, pCLName, pIXName, rc ) ;
            rc = cursorPtr->getCurrentKey( keyObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get key for collection [%s.%s] "
                        "index [%s], rc: %d", pCSName, pCLName, pIXName, rc ) ;
            rc = cursorPtr->getCurrentRecordID( rid ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get record ID for collection [%s.%s] "
                         "index [%s], rc: %d", pCSName, pCLName, pIXName, rc ) ;

            ++ totalSampleCounts ;
            totalKeySize += key.getKeySize() ;

            if ( ridSet.insert( rid ).second )
            {
               rc = sorter.push( keyObj, dummy.objdata(), dummy.objsize(), NULL ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to push key into sorter, rc: %d", rc ) ;
            }

            if ( sorter.getObjNum() >= sampleRecords ||
                 totalSampleCounts >= totalRecords )
            {
               break ;
            }

            rc = cursorPtr->advance( cb ) ;
            if ( SDB_IXM_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to advance cursor for collection [%s.%s] "
                         "index [%s], rc: %d", pCSName, pCLName, pIXName, rc ) ;
         }

         // simply calculate the index levels
         FLOAT64 avgKeySize = (FLOAT64)( totalKeySize ) / (FLOAT64)totalSampleCounts ;
         FLOAT64 avgKeyNum = (FLOAT64)( pageSize ) / avgKeySize ;
         if ( totalPages <= 1 )
         {
            pIndexStat->setIndexLevels( 1 ) ;
         }
         else if ( totalPages <= 2 + avgKeyNum )
         {
            pIndexStat->setIndexLevels( 2 ) ;
         }
         else
         {
            UINT32 indexLevels =
                  (UINT32)( ceil( log( (FLOAT64)totalPages ) /
                                  log( avgKeyNum + 1 ) ) ) + 1 ;
            indexLevels = OSS_MAX( 1, OSS_MIN( indexLevels, totalPages ) ) ;
            pIndexStat->setIndexLevels( indexLevels ) ;
         }
      }

      sortCount = (UINT32)sorter.getObjNum() ;

      if ( sortCount == 0 )
      {
         goto done ;
      }

      rc = pIndexStat->initMCVSet( sortCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize MCV set, rc: %d", rc ) ;

      pIndexStat->setSampleRecords( sortCount ) ;

      rc = sorter.sort( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sort index samples, rc: %d", rc ) ;

      while ( sorter.more() )
      {
         BSONObj curKey ;
         rc = sorter.next( &tuple ) ;
         PD_RC_CHECK( rc, PDERROR,  "Failed to fetch tuple from  sorter, "
                      "rc: %d", rc ) ;

         curKey = BSONObj( tuple->key() ) ;

         if ( prevKey.isEmpty() )
         {
            prevKey = curKey ;
            prevCount = 0 ;
         }
         else
         {
            if ( 0 != prevKey.woCompare( curKey, dummy, FALSE ) )
            {
               // A different key, push the previous one into MCV set
               fraction = (double) prevCount / (double) sortCount ;
               rc = pIndexStat->pushMCVSet( prevKey, fraction ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to insert MCV value, rc: %d",
                            rc ) ;
               prevKey = curKey ;
               prevCount = 0 ;
            }
         }
         prevCount ++ ;
      }

      // Push the last one into MCV set
      fraction = (double) prevCount / (double) sortCount ;
      rc = pIndexStat->pushMCVSet( prevKey, fraction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert MCV value, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNBUILDMCVSET, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNANALYZEDPSLOG, "rtnAnalyzeDpsLog" )
   INT32 rtnAnalyzeDpsLog ( const CHAR *pCSName,
                            const CHAR *pCLFullName,
                            const CHAR *pIndexName,
                            _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNANALYZEDPSLOG ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dpsTransCB *transCB = sdbGetTransCB() ;
      UINT32 logRecSize = 0 ;

      if ( NULL != pCSName && NULL == pCLFullName )
      {
         pCLFullName = pCSName ;
      }
      else if ( NULL == pCSName && NULL == pCLFullName )
      {
         // Reload all statistics
         pCLFullName = "SYS" ;
      }

      if ( NULL != dpsCB )
      {
         dpsMergeInfo info ;
         info.setInfoEx( ~0, ~0, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET, NULL ) ;
         dpsLogRecord &record = info.getMergeBlock().record() ;

         rc = dpsInvalidCata2Record( DPS_LOG_INVALIDCATA_TYPE_STAT,
                                     pCLFullName, pIndexName,
                                     record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build analyze log, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = transCB->reservedLogSpace( logRecSize, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space for "
                    "invalid-cata log, rc: %d", rc ) ;
            logRecSize = 0 ;
            goto error ;
         }

         rc = dpsCB->prepare(info ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare analyze log, rc: %d", rc ) ;

         dpsCB->writeData( info ) ;
      }
      else if ( NULL != cb )
      {
         cb->setDataExInfo( pCLFullName, ~0, ~0, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         transCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNANALYZEDPSLOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPOSTANALYZEALL, "_rtnPostAnalyzeAll" )
   INT32 _rtnPostAnalyzeAll ( const rtnAnalyzeParam & param,
                              _SDB_RTNCB *rtnCB,
                              _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNPOSTANALYZEALL ) ;

      SDB_ASSERT( rtnCB, "rtnCB is invalid" ) ;

      // Make sure main-collection plans are removed
      rtnCB->getAPM()->invalidateAllPlans() ;

      if ( param._mode != SDB_ANALYZE_MODE_RELOAD &&
           param._mode != SDB_ANALYZE_MODE_CLEAR )
      {
         // Notify backup nodes to clear old cached statistics
         rc = rtnAnalyzeDpsLog( NULL, NULL, NULL, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write analyze log, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNPOSTANALYZEALL, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNPOSTANALYZECS, "_rtnPostAnalyzeCS" )
   INT32 _rtnPostAnalyzeCS ( const CHAR * pCSName,
                             const rtnAnalyzeParam & param,
                             _SDB_RTNCB *rtnCB,
                             _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNPOSTANALYZECS ) ;

      SDB_ASSERT( pCSName, "pCSName is invalid" ) ;
      SDB_ASSERT( rtnCB, "rtnCB is invalid" ) ;

      // Make sure main-collection plans are removed
      rtnCB->getAPM()->invalidateSUPlans( pCSName ) ;

      if ( param._mode != SDB_ANALYZE_MODE_RELOAD &&
           param._mode != SDB_ANALYZE_MODE_CLEAR )
      {
         // Notify backup nodes to clear old cached statistics
         rc = rtnAnalyzeDpsLog( pCSName, NULL, NULL, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write analyze log, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNPOSTANALYZECS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNFILLCLSTAT, "_rtnFillCLStat" )
   INT32 _rtnFillCLStat( dmsStorageUnit *pSU,
                         dmsMBContext *mbContext,
                         const rtnAnalyzeParam &param,
                         dmsCollectionStat *pCollectionStat,
                         pmdEDUCB *cb,
                         _SDB_DMSCB *dmsCB,
                         _SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNFILLCLSTAT ) ;

      SDB_ASSERT( pSU, "pSU is invalid" ) ;
      SDB_ASSERT( mbContext, "mbContext is invalid" ) ;

      const CHAR *pCSName = pSU->CSName() ;
      const CHAR *pCLName = mbContext->mb()->_collectionName ;

      UINT32 totalFieldCounts = 0, totalSampleCounts = 0 ;
      UINT64 totalDataSize = 0, freeDataSize = 0 ;
      std::unique_ptr<IDataCursor> cursorPtr ;

      pCollectionStat->setTotalRecords( mbContext->mbStat()->_totalRecords.fetch() ) ;
      pCollectionStat->setSampleRecords( param._sampleRecords ) ;

      rc = mbContext->getCollPtr()->getDataStats( totalDataSize, freeDataSize,
                                                  FALSE, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s.%s] data stats, "
                   "rc: %d", pCSName, pCLName, rc ) ;
      pCollectionStat->setTotalDataPages( totalDataSize / pSU->getPageSize() ) ;
      pCollectionStat->setTotalDataSize( mbContext->mbStat()->_totalOrgDataLen.fetch() ) ;

      // sample records
      rc = mbContext->getCollPtr()->createDataSampleCursor( cursorPtr,
                                                            param._sampleRecords,
                                                            FALSE,
                                                            cb ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to create sample cursor for "
                         "collection [%s.%s], rc: %d", pCSName, pCLName, rc ) ;
         }
         rc = SDB_OK ;
      }
      else
      {
         while ( TRUE )
         {
            dmsRecordData recordData ;
            rc = cursorPtr->getCurrentRecord( recordData ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get record data for collection [%s.%s], "
                        "rc: %d", pCSName, pCLName, rc ) ;

            try
            {
               BSONObj recordObj( recordData.data() ) ;
               totalFieldCounts += recordObj.nFields() ;
               totalSampleCounts ++ ;
            }
            catch ( exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Failed to get record data for collection [%s.%s], "
                     "occurred exception: %s", pCSName, pCLName, e.what() ) ;
               goto error ;
            }

            if ( totalSampleCounts >= param._sampleRecords )
            {
               break ;
            }

            rc = cursorPtr->advance( cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to advance cursor for collection [%s.%s], "
                        "rc: %d", pCSName, pCLName, rc ) ;
         }
      }

      if ( totalSampleCounts > 0 )
      {
         pCollectionStat->setAvgNumFields( ceil( (double)totalFieldCounts /
                                                 (double)totalSampleCounts ) ) ;
      }
      else
      {
         pCollectionStat->setAvgNumFields( DMS_STAT_DEF_AVG_NUM_FIELDS ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNFILLCLSTAT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
