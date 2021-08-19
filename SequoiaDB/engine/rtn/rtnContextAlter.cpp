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

   Source File Name = rtnContextAlter.cpp

   Descriptive Name = RunTime Alter Operation Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          3/12/2018   HGM Init draft

   Last Changed =

*******************************************************************************/

#include "rtnContextAlter.hpp"
#include "rtn.hpp"
#include "dpsOp2Record.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   /*
      _rtnContextAlterBase implement
    */
   _rtnContextAlterBase::_rtnContextAlterBase ( SINT64 contextID, UINT64 eduID )
   : _rtnContextBase( contextID, eduID ),
     _rtnAlterJobHolder(),
     _dmsCB( sdbGetDMSCB() ),
     _transCB( sdbGetTransCB() ),
     _reservedLogSpace( 0 ),
     _checkedWritable( FALSE ),
     _phase( RTN_ALTER_PHASE_0 )
   {
      _pDpsCB = sdbGetDPSCB() ;
   }

   _rtnContextAlterBase::~_rtnContextAlterBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERBASE_OPEN, "_rtnContextAlterBase::open" )
   INT32 _rtnContextAlterBase::open ( rtnAlterJobHolder & holder,
                                      _pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERBASE_OPEN ) ;

      _w = w ;

      setAlterJob( holder, FALSE ) ;

      rc = _checkWritable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check DMS writable, rc: %d", rc ) ;

      rc = _reserveLogSpace( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reserve log space, rc: %d", rc ) ;

      rc = _lockTransaction( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock transaction, rc: %d", rc ) ;

      rc = _openInternal( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, rc: %d", rc ) ;

      setAlterJob( holder, TRUE ) ;

      _phase = RTN_ALTER_PHASE_1 ;
      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERBASE_OPEN, rc ) ;
      return rc ;

   error :
      _close( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERBASE__CLOSE, "_rtnContextAlterBase::_close" )
   void _rtnContextAlterBase::_close (  _pmdEDUCB * cb )
   {
      PD_TRACE_ENTRY( SDB__RTNALTERBASE__CLOSE ) ;

      INT32  rc = _closeInternal( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to close context, rc: %d", rc ) ;
      }

      _releaseTransaction( cb ) ;
      _releaseLogSpace( cb ) ;
      _releaseWritable( cb ) ;

      PD_TRACE_EXIT( SDB__RTNALTERBASE__CLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERBASE__CHKWRITABLE, "_rtnContextAlterBase::_checkWritable" )
   INT32 _rtnContextAlterBase::_checkWritable (  _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERBASE__CHKWRITABLE ) ;

      _releaseWritable( cb ) ;

      rc = _dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check writable, rc: %d" ) ;

      _checkedWritable = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERBASE__CHKWRITABLE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERBASE__RELEASEWRITABLE, "_rtnContextAlterBase::_releaseWritable" )
   void _rtnContextAlterBase::_releaseWritable (  _pmdEDUCB * cb )
   {
      PD_TRACE_ENTRY( SDB__RTNALTERBASE__RELEASEWRITABLE ) ;

      if ( _checkedWritable )
      {
         _dmsCB->writeDown( cb ) ;
         _checkedWritable = FALSE ;
      }

      PD_TRACE_EXIT( SDB__RTNALTERBASE__RELEASEWRITABLE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERBASE__RESERVELOG, "_rtnContextAlterBase::_reserveLogSpace" )
   INT32 _rtnContextAlterBase::_reserveLogSpace (  _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERBASE__RESERVELOG ) ;

      if ( NULL != getDPSCB() )
      {
         dpsMergeInfo info ;
         dpsLogRecord & record = info.getMergeBlock().record() ;
         UINT32 logRecSize = 0 ;
         RTN_ALTER_OBJECT_TYPE objType = _alterJob->getObjectType() ;

         _releaseLogSpace( cb ) ;

         rc = dpsAlter2Record( _alterJob->getObjectName(), objType,
                               _alterJob->getJobObject(), record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = getDPSCB()->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = _transCB->reservedLogSpace( logRecSize, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to reserved log space (length=%u), rc: %d",
                      logRecSize, rc ) ;

         _reservedLogSpace = logRecSize ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERBASE__RESERVELOG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERBASE__RELEASELOG, "_rtnContextAlterBase::_releaseLogSpace" )
   void _rtnContextAlterBase::_releaseLogSpace ( _pmdEDUCB * cb )
   {
      PD_TRACE_ENTRY( SDB__RTNALTERBASE__RELEASELOG ) ;

      if ( _reservedLogSpace > 0 )
      {
         _transCB->releaseLogSpace( _reservedLogSpace, cb ) ;
         _reservedLogSpace = 0 ;
      }

      PD_TRACE_EXIT( SDB__RTNALTERBASE__RELEASELOG ) ;
   }

   /*
      _rtnContextAlterCS implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextAlterCS, RTN_CONTEXT_ALTERCS, "ALTERCS" )

   _rtnContextAlterCS::_rtnContextAlterCS ( SINT64 contextID, UINT64 eduID )
   : _rtnContextAlterBase( contextID, eduID ),
     _logicalCSID( DMS_INVALID_LOGICCSID ),
     _su( NULL )
   {
   }

   _rtnContextAlterCS::~_rtnContextAlterCS ()
   {
      pmdEDUMgr * eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB * cb = eduMgr->getEDUByID( eduID() ) ;
      _close( cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSCTX__OPENINT, "_rtnContextAlterCS::_openInternal" )
   INT32 _rtnContextAlterCS::_openInternal ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSCTX__OPENINT ) ;

      const CHAR * collectionSpace = _alterJob->getObjectName() ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      rc = _dmsCB->nameToSUAndLock( collectionSpace, suID, &_su, EXCLUSIVE,
                                    OSS_ONE_SEC ) ;
      if ( SDB_TIMEOUT == rc )
      {
         rc = SDB_LOCK_FAILED ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to lock storage unit, rc: %d", rc ) ;

      PD_CHECK( DMS_INVALID_LOGICCSID == _logicalCSID ||
                _su->LogicalCSID() == _logicalCSID,
                SDB_DMS_NOTEXIST, error, PDERROR,
                "Failed to get storage unit" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSCTX__OPENINT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSCTX__CLOSEINT, "rtnContextAlterCS::_closeInternal" )
   INT32 _rtnContextAlterCS::_closeInternal ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSCTX__CLOSEINT ) ;

      if ( NULL != _su )
      {
         _dmsCB->suUnlock( _su->CSID(), EXCLUSIVE ) ;
         _su = NULL ;
      }

      PD_TRACE_EXITRC( SDB__RTNALTERCSCTX__CLOSEINT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSCTX__PREPAREDATA, "_rtnContextAlterCS::_prepareData" )
   INT32 _rtnContextAlterCS::_prepareData ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSCTX__PREPAREDATA ) ;

      const CHAR * collectionSpace = _alterJob->getObjectName() ;
      const rtnAlterOptions * options = _alterJob->getOptions() ;
      const RTN_ALTER_TASK_LIST & alterTasks = _alterJob->getAlterTasks() ;

      for ( RTN_ALTER_TASK_LIST::const_iterator iter = alterTasks.begin() ;
            iter != alterTasks.end() ;
            ++ iter )
      {
         const rtnAlterTask * task = ( *iter ) ;
         rc = rtnAlterCollectionSpace( collectionSpace, task, options, cb,
                                       getDPSCB(), _su, _dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to run alter task [%s] on "
                      "collection space [%s], rc: %d", task->getActionName(),
                      collectionSpace, rc ) ;
      }

      _close( cb ) ;

      _hitEnd = TRUE ;
      rc = SDB_DMS_EOC ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSCTX__PREPAREDATA, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   void _rtnContextAlterCS::_toString ( stringstream & ss )
   {
      if ( NULL != _alterJob )
      {
         ss << ",Name:" << _alterJob->getObjectName() ;
      }
      ss << ",Step:" << _phase ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSCTX__LOCKTRANS, "_rtnContextAlterCS::_lockTransaction" )
   INT32 _rtnContextAlterCS::_lockTransaction ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSCTX__LOCKTRANS ) ;

      if ( NULL != getDPSCB() )
      {
         const CHAR * collectionSpace = _alterJob->getObjectName() ;
         dpsTransRetInfo lockConflict ;
         dmsStorageUnitID suID = DMS_INVALID_CS ;
         UINT32 logicalCSID = DMS_INVALID_LOGICCSID ;
         dmsStorageUnit * su = NULL ;

         rc = _dmsCB->nameToSUAndLock( collectionSpace, suID, &su ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space [%s], "
                      "rc: %d", collectionSpace, rc ) ;
         logicalCSID = su->LogicalCSID() ;
         _dmsCB->suUnlock( suID ) ;

         /*
         Modified by Xujianhui: Alter collection don't need trans lock

         rc = _transCB->transLockTryS( cb, logicalCSID, DMS_INVALID_MBID,
                                       NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get transaction-lock of "
                      "collection space [%s], rc: %d"OSS_NEWLINE
                      "Conflict( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      collectionSpace, rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;
         */

         _logicalCSID = logicalCSID ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSCTX__LOCKTRANS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSCTX__RELEASETRANS, "_rtnContextAlterCS::_releaseTransaction" )
   void _rtnContextAlterCS::_releaseTransaction ( _pmdEDUCB * cb )
   {

      PD_TRACE_ENTRY( SDB__RTNALTERCSCTX__RELEASETRANS ) ;

      if ( NULL != cb && NULL != getDPSCB() &&
           DMS_INVALID_LOGICCSID != _logicalCSID )
      {
         /*
         Modified by Xujianhui: Alter collection don't need trans lock

         _transCB->transLockRelease( cb, _logicalCSID ) ;
         */
         _logicalCSID = DMS_INVALID_LOGICCSID ;
      }

      PD_TRACE_EXIT( SDB__RTNALTERCSCTX__RELEASETRANS ) ;
   }

   /*
      _rtnContextAlterCL implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextAlterCL, RTN_CONTEXT_ALTERCL, "ALTERCL" )

   _rtnContextAlterCL::_rtnContextAlterCL ( SINT64 contextID, UINT64 eduID )
   : _rtnContextAlterBase( contextID, eduID ),
     _logicalCSID( DMS_INVALID_LOGICCSID ),
     _mbID( DMS_INVALID_MBID ),
     _su( NULL ),
     _mbContext( NULL )
   {
   }

   _rtnContextAlterCL::~_rtnContextAlterCL ()
   {
      pmdEDUMgr * eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB * cb = eduMgr->getEDUByID( eduID() ) ;
      _close( cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__OPENINT, "_rtnContextAlterCL::_openInternal" )
   INT32 _rtnContextAlterCL::_openInternal ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLCTX__OPENINT ) ;

      const rtnAlterOptions * options = _alterJob->getOptions() ;
      const RTN_ALTER_TASK_LIST & alterTasks = _alterJob->getAlterTasks() ;

      const CHAR * collection = _alterJob->getObjectName() ;

      PD_CHECK( NULL != _su, SDB_INVALIDARG, error, PDERROR,
                "Failed to get su" ) ;
      PD_CHECK( NULL != _mbContext, SDB_INVALIDARG, error, PDERROR,
                "Failed to get mbContext" ) ;

      for ( RTN_ALTER_TASK_LIST::const_iterator iter = alterTasks.begin() ;
            iter != alterTasks.end() ;
            ++ iter )
      {
         const rtnAlterTask * task = ( *iter ) ;
         rc = rtnCheckAlterCollection( collection, task, cb, _mbContext, _su,
                                       _dmsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to check alter task [%s] on "
                    "collection [%s], rc: %d", task->getActionName(),
                    collection, rc ) ;
            if ( options->isIgnoreException() )
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
      PD_TRACE_EXITRC( SDB__RTNALTERCLCTX__OPENINT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__CLOSEINT, "_rtnContextAlterCL::_closeInternal" )
   INT32 _rtnContextAlterCL::_closeInternal ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSCTX__CLOSEINT ) ;

      if ( NULL != _su && NULL != _mbContext )
      {
         _su->data()->releaseMBContext( _mbContext ) ;
         _mbContext = NULL ;
      }
      if ( NULL != _su )
      {
         _dmsCB->suUnlock( _su->CSID() ) ;
         _su = NULL ;
      }

      PD_TRACE_EXITRC( SDB__RTNALTERCSCTX__CLOSEINT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__PREPAREDATA, "_rtnContextAlterCL::_prepareData" )
   INT32 _rtnContextAlterCL::_prepareData ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLCTX__PREPAREDATA ) ;

      const CHAR * collection = _alterJob->getObjectName() ;
      const rtnAlterOptions * options = _alterJob->getOptions() ;
      const RTN_ALTER_TASK_LIST & alterTasks = _alterJob->getAlterTasks() ;

      PD_CHECK( NULL != _su, SDB_INVALIDARG, error, PDERROR,
                "Failed to get su" ) ;
      PD_CHECK( NULL != _mbContext, SDB_INVALIDARG, error, PDERROR,
                "Failed to get mbContext" ) ;

      // Update catalog cache
      rc = sdbGetShardCB()->syncUpdateCatalog( collection ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection catalog, "
                   "rc: %d", rc ) ;

      for ( RTN_ALTER_TASK_LIST::const_iterator iter = alterTasks.begin() ;
            iter != alterTasks.end() ;
            ++ iter )
      {
         const rtnAlterTask * task = ( *iter ) ;
         rc = rtnAlterCollection( collection, task, options, cb, getDPSCB(),
                                  _mbContext, _su, _dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to run alter task [%s] on "
                      "collection [%s], rc: %d", task->getActionName(),
                      collection, rc ) ;
      }

      _close( cb ) ;

      _hitEnd = TRUE ;
      rc = SDB_DMS_EOC ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLCTX__PREPAREDATA, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   void _rtnContextAlterCL::_toString ( stringstream & ss )
   {
      if ( NULL != _alterJob )
      {
         ss << ",Name:" << _alterJob->getObjectName() ;
      }
      ss << ",Step:" << _phase ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__LOCKTRANS, "_rtnContextAlterCL::_lockTransaction" )
   INT32 _rtnContextAlterCL::_lockTransaction ( _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLCTX__LOCKTRANS ) ;

      const CHAR * collection = _alterJob->getObjectName() ;
      const CHAR * clShortName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      rc = rtnResolveCollectionNameAndLock( collection, _dmsCB, &_su,
                                            &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to resolve collection name and lock storage unit, "
                   "rc: %d", rc ) ;

      rc = _su->data()->getMBContext( &_mbContext, clShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get mb context with exclusive lock, "
                   "rc: %d", rc ) ;

      if ( NULL != getDPSCB() )
      {
         dpsTransRetInfo lockConflict ;

         /*
         Modified by Xujianhui: Alter collection don't need trans lock

         rc = _transCB->transLockTryS( cb, _su->LogicalCSID(),
                                       _mbContext->mbID(),
                                       NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get transaction-lock of "
                      "collection [%s], rc: %d"OSS_NEWLINE
                      "Conflict( representative ):"OSS_NEWLINE
                      "   EDUID:  %llu"OSS_NEWLINE
                      "   TID:    %u"OSS_NEWLINE
                      "   LockId: %s"OSS_NEWLINE
                      "   Mode:   %s"OSS_NEWLINE,
                      collection, rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;
         */

         _logicalCSID = _su->LogicalCSID() ;
         _mbID = _mbContext->mbID() ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLCTX__LOCKTRANS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__RELEASETRANS, "_rtnContextAlterCL::_releaseTransaction" )
   void _rtnContextAlterCL::_releaseTransaction ( _pmdEDUCB * cb )
   {

      PD_TRACE_ENTRY( SDB__RTNALTERCLCTX__RELEASETRANS ) ;

      if ( NULL != cb && DMS_INVALID_LOGICCSID != _logicalCSID &&
           DMS_INVALID_MBID != _mbID )
      {
         /*
         Modified by Xujianhui: Alter collection don't need trans lock

         _transCB->transLockRelease( cb, _logicalCSID, _mbID ) ;
         */
         _logicalCSID = DMS_INVALID_LOGICCSID ;
         _mbID = DMS_INVALID_MBID ;
      }

      PD_TRACE_EXIT( SDB__RTNALTERCLCTX__RELEASETRANS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__CHKCOMPRESS, "_rtnContextAlterCL::_checkCompress" )
   INT32 _rtnContextAlterCL::_checkCompress ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLCTX__CHKCOMPRESS ) ;

      PD_CHECK( NULL != _su, SDB_INVALIDARG, error, PDERROR,
                "Failed to get su" ) ;
      PD_CHECK( NULL != _mbContext, SDB_INVALIDARG, error, PDERROR,
                "Failed to get mbContext" ) ;

      rc = _su->canSetCollectionCompressor( _mbContext ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check collection for setting compress, rc: %d",
                   rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLCTX__CHKCOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCTX__CHKEXTOPT, "_rtnContextAlterCL::_checkExtOptions" )
   INT32 _rtnContextAlterCL::_checkExtOptions ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLCTX__CHKEXTOPT ) ;

      PD_CHECK( NULL != _su, SDB_INVALIDARG, error, PDERROR,
                "Failed to get su" ) ;
      PD_CHECK( NULL != _mbContext, SDB_INVALIDARG, error, PDERROR,
                "Failed to get mbContext" ) ;

      PD_CHECK( DMS_STORAGE_CAPPED == _su->type(),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check collection for setting ext options: "
                "should be capped" ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLCTX__CHKEXTOPT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}
