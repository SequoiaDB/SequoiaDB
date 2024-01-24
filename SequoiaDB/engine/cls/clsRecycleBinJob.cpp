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

   Source File Name = clsRecycleBinJob.cpp

   Descriptive Name = Recycle Bin Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsRecycleBinJob.hpp"
#include "pmd.hpp"
#include "dmsStorageUnit.hpp"
#include "rtnLocalTask.hpp"
#include "rtn.hpp"
#include "clsTrace.hpp"

namespace engine
{

   /*
      _clsDropRecycleBinItemJob implement
    */
   _clsDropRecycleBinJob::_clsDropRecycleBinJob()
   : _recycleBinMgr( sdbGetClsCB()->getRecycleBinMgr() ),
     _isDropAll( FALSE )
   {
   }

   _clsDropRecycleBinJob::~_clsDropRecycleBinJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPRECYBINJOB_INITDROPITEMS, "_clsDropRecycleBinJob::initDropItems" )
   INT32 _clsDropRecycleBinJob::initDropItems( const UTIL_RECY_ITEM_LIST &recycleItems )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSDROPRECYBINJOB_INITDROPITEMS ) ;

      try
      {
         _recycleItems = recycleItems ;
         _isDropAll = FALSE ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save recycle items, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSDROPRECYBINJOB_INITDROPITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPRECYBINJOB_INITDROPALL, "_clsDropRecycleBinJob::initDropAll" )
   INT32 _clsDropRecycleBinJob::initDropAll()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSDROPRECYBINJOB_INITDROPALL ) ;

      _recycleItems.clear() ;
      _isDropAll = TRUE ;

      PD_TRACE_EXITRC( SDB_CLSDROPRECYBINJOB_INITDROPALL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDROPRECYBINJOB_DOIT, "_clsDropRecycleBinJob::doit" )
   INT32 _clsDropRecycleBinJob::doit( IExecutor *pExe,
                                      UTIL_LJOB_DO_RESULT &result,
                                      UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSDROPRECYBINJOB_DOIT ) ;

      pmdEDUCB *cb = (pmdEDUCB *)pExe ;

      PD_LOG( PDDEBUG, "Start job [%s]", name() ) ;

      PD_CHECK( pmdIsPrimary(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to check primary status" ) ;
      PD_CHECK( !cb->isInterrupted(), SDB_APP_INTERRUPT, error, PDERROR,
                "Failed to drop item, session is interrupted" ) ;

      // in async job, always check CATALOG
      if ( _isDropAll )
      {
         rc = dropAll( cb, TRUE ) ;
      }
      else
      {
         rc = dropItems( _recycleItems, cb, TRUE ) ;
      }

      if ( SDB_OK == rc )
      {
         result = UTIL_LJOB_DO_FINISH ;
      }
      else if ( SDB_DPS_TRANS_LOCK_INCOMPATIBLE == rc ||
                SDB_LOCK_FAILED == rc ||
                SDB_DMS_CS_DELETING == rc )
      {
         rc = SDB_OK ;
         result = UTIL_LJOB_DO_CONT ;
         sleepTime = RTN_RECYCLE_RETRY_INTERVAL ;
      }
      else
      {
         result = UTIL_LJOB_DO_FINISH ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle items, rc: %d", rc ) ;
      }

   done:
      PD_LOG( PDDEBUG, "Stop job [%s], rc: %d", name(), rc ) ;
      PD_TRACE_EXITRC( SDB__CLSDROPRECYBINJOB_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPRECYBINJOB_DROPITEM, "_clsDropRecycleBinJob::dropItem" )
   INT32 _clsDropRecycleBinJob::dropItem( const utilRecycleItem &recycleItem,
                                          _pmdEDUCB *cb,
                                          BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSDROPRECYBINJOB_DROPITEM ) ;

      BOOLEAN isDropped = FALSE ;

      rc = _recycleBinMgr->dropItemWithCheck( recycleItem,
                                              cb,
                                              checkCatalog,
                                              isDropped ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [%s], rc: %d",
                   recycleItem.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSDROPRECYBINJOB_DROPITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPRECYBINJOB_DROPITEMS, "_clsDropRecycleBinJob::dropItems" )
   INT32 _clsDropRecycleBinJob::dropItems( const UTIL_RECY_ITEM_LIST &recycleItems,
                                           _pmdEDUCB *cb,
                                           BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSDROPRECYBINJOB_DROPITEMS ) ;

      for ( UTIL_RECY_ITEM_LIST_CIT iter = recycleItems.begin() ;
            iter != recycleItems.end() ;
            ++ iter )
      {
         const utilRecycleItem &recycleItem = *iter ;
         rc = dropItem( recycleItem, cb, checkCatalog ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [%s], "
                      "rc: %d", recycleItem.getRecycleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSDROPRECYBINJOB_DROPITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSDROPRECYBINJOB_DROPALL, "_clsDropRecycleBinJob::dropAll" )
   INT32 _clsDropRecycleBinJob::dropAll( _pmdEDUCB *cb, BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSDROPRECYBINJOB_DROPALL ) ;

      rc = _recycleBinMgr->dropAllItems( cb, checkCatalog ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop all recycle items, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CLSDROPRECYBINJOB_DROPALL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSSTARTDROPRECYBINITEMJOB, "clsStartDropRecycleBinItemJob" )
   INT32 clsStartDropRecycleBinItemJob( const UTIL_RECY_ITEM_LIST &recycleItems )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSSTARTDROPRECYBINITEMJOB ) ;

      clsDropRecycleBinJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsDropRecycleBinJob() ;
      PD_CHECK( NULL != pJob, SDB_OOM, error, PDERROR,
                "Failed to allocate drop recycle bin job" ) ;

      rc = pJob->initDropItems( recycleItems ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize drop recycle bin job, "
                   "rc: %d", rc ) ;

      rc = pJob->submit( TRUE, UTIL_LJOB_PRI_MID ) ;
      pJob = NULL ;
      PD_RC_CHECK( rc, PDERROR, "Failed to submit drop recycle bin job, "
                   "rc: %d", rc ) ;

   done:
      if ( pJob )
      {
         SDB_OSS_DEL pJob ;
      }
      PD_TRACE_EXITRC( SDB_CLSSTARTDROPRECYBINITEMJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSSTARTDROPRECYBINALLJOB, "clsStartDropRecycleBinAllJob" )
   INT32 clsStartDropRecycleBinAllJob()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSSTARTDROPRECYBINALLJOB ) ;

      clsDropRecycleBinJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsDropRecycleBinJob() ;
      PD_CHECK( NULL != pJob, SDB_OOM, error, PDERROR,
                "Failed to allocate drop recycle bin job" ) ;

      rc = pJob->initDropAll() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize drop recycle bin job, "
                   "rc: %d", rc ) ;

      rc = pJob->submit( TRUE, UTIL_LJOB_PRI_MID ) ;
      pJob = NULL ;
      PD_RC_CHECK( rc, PDERROR, "Failed to submit drop recycle bin job, "
                   "rc: %d", rc ) ;

   done:
      if ( pJob )
      {
         SDB_OSS_DEL pJob ;
      }
      PD_TRACE_EXITRC( SDB_CLSSTARTDROPRECYBINALLJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
