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

   Source File Name = rtnContextRecycle.hpp

   Descriptive Name = RunTime Recycle Operation Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/01/2021  HGM initial version

   Last Changed =

*******************************************************************************/

#include "rtnContextRecycle.hpp"
#include "rtn.hpp"
#include "dpsOp2Record.hpp"
#include "clsMgr.hpp"
#include "rtnTrace.hpp"
#include "ossMemPool.hpp"
#include "rtnLocalTask.hpp"
#include "clsUniqueIDCheckJob.hpp"
#include "clsRecycleBinJob.hpp"

namespace engine
{

   /*
      _rtnCtxReturnHelper implement
    */
   _rtnCtxReturnHelper::_rtnCtxReturnHelper()
   : _recycleBinMgr( sdbGetClsCB()->getRecycleBinMgr() )
   {
      _recycleBinMgr->registerReturn() ;
   }

   _rtnCtxReturnHelper::~_rtnCtxReturnHelper()
   {
      if ( NULL != _recycleBinMgr )
      {
         _recycleBinMgr->unregisterReturn() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNHELP__DELITEM, "_rtnCtxReturnHelper::_deleteItem" )
   INT32 _rtnCtxReturnHelper::_deleteItem( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNHELP__DELITEM ) ;

      if ( _returnOptions._recycleItem.isValid() )
      {
         rc = _recycleBinMgr->deleteItem( _returnOptions._recycleItem, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle item "
                      "[origin %s, recycle %s], rc: %d",
                      _returnOptions._recycleItem.getOriginName(),
                      _returnOptions._recycleItem.getRecycleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNHELP__DELITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _rtnContextReturnCL implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextReturnCL,
                          RTN_CONTEXT_RETURNCL,
                          "RETURNCL" )
   _rtnContextReturnCL::_rtnContextReturnCL( SINT64 contextID, UINT64 eduID )
   : _BASE( contextID, eduID ),
     _returnBlockID( 0 )
   {
   }

   _rtnContextReturnCL::~_rtnContextReturnCL()
   {
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb      = eduMgr->getEDUByID( eduID() ) ;
      _releaseLock( cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCL_OPEN, "_rtnContextReturnCL::open" )
   INT32 _rtnContextReturnCL::open( const utilRecycleItem &recycleItem,
                                    const utilRecycleReturnInfo &returnInfo,
                                    _pmdEDUCB *cb,
                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCL_OPEN ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == recycleItem.getType(),
                  "invalid recycle type" ) ;

      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      CHAR origShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

      const CHAR *originName = NULL ;
      const CHAR *recycleName = NULL ;
      utilReturnNameInfo nameInfo( recycleItem.getOriginName() ) ;
      utilReturnUIDInfo uidInfo( recycleItem.getOriginID() ) ;

      _returnOptions._recycleItem = recycleItem ;

      // check if collection need to be renamed and changed unique ID
      rc = returnInfo.getReturnCLName( nameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                   "collection [%s], rc: %d", recycleItem.getOriginName(),
                   rc ) ;
      _returnOptions._recycleItem.setOriginName( nameInfo.getReturnName() ) ;

      rc = returnInfo.getReturnCLUID( uidInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return unique ID for "
                   "collection [%s], rc: %d",
                   recycleItem.getOriginName(), rc ) ;
      if ( uidInfo.isChanged() )
      {
         _returnOptions._recycleItem.setOriginID( uidInfo.getReturnUID() ) ;
      }

      PD_LOG( PDEVENT, "Got return collection [origin %s, recycle %s, "
              "return %s, origin UID %llu, return UID %llu]",
              recycleItem.getOriginName(),
              recycleItem.getRecycleName(),
              _returnOptions._recycleItem.getOriginName(),
              recycleItem.getOriginID(),
              _returnOptions._recycleItem.getOriginID() ) ;

      originName = _returnOptions._recycleItem.getOriginName() ;
      recycleName = _returnOptions._recycleItem.getRecycleName() ;

      // get collection space name
      rc = rtnResolveCollectionName( originName, ossStrlen( originName ),
                                     csName, DMS_COLLECTION_SPACE_NAME_SZ,
                                     origShortName, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection "
                   "name [%s], rc: %d", originName, rc ) ;

      // open base rename context
      rc = _BASE::open( csName, recycleName, origShortName, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open internal rename context, "
                   "rc: %d", rc ) ;

      // if recycle collection is created by truncate, we need to check if
      // it has new data after truncate
      if ( _returnOptions._recycleItem.isTruncate() )
      {
         dmsMBContext *mbContext = NULL ;
         rc = _su->data()->getMBContext( &mbContext, origShortName, SHARED ) ;
         if ( SDB_OK == rc )
         {
            // get number of records and lobs
            UINT64 totalRecords = mbContext->mbStat()->_totalRecords.fetch() ;
            UINT64 totalLobs = mbContext->mbStat()->_totalLobs.fetch() ;

            // make sure meta block is released
            _su->data()->releaseMBContext( mbContext ) ;

            PD_LOG_MSG_CHECK( 0 == totalRecords && 0 == totalLobs,
                              SDB_RECYCLE_CONFLICT, error, PDERROR,
                              "Failed to check recycle item [origin %s, "
                              "recycle %s] for truncate, collection [%s] "
                              "is not empty", originName, recycleName,
                              originName ) ;
         }
         else if ( SDB_DMS_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get count for collection [%s], "
                      "rc: %d", originName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCL_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _rtnContextReturnCL::_toString( stringstream &ss )
   {
      _BASE::_toString( ss ) ;
      ss << ",RecycleID:" <<
            _returnOptions._recycleItem.getRecycleID() <<
            ",RecycleOpType:" <<
            utilGetRecycleOpTypeName( _returnOptions._recycleItem.getOpType() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCL__TRYLOCK, "_rtnContextReturnCL::_tryLock" )
   INT32 _rtnContextReturnCL::_tryLock( const CHAR *csName,
                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCL__TRYLOCK ) ;

      const CHAR *originName = _returnOptions._recycleItem.getOriginName() ;

      rc = _pFreezingWnd->registerCL( originName, _returnBlockID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to block collection [%s], rc: %d",
                   originName, rc ) ;
      PD_LOG( PDEVENT, "Begin to block all write operations "
              "of collection [%s], ID: %llu", originName, _returnBlockID ) ;

      rc = _BASE::_tryLock( csName, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock for return collection [%s] "
                   "to [%s], rc: %d", _clFullName, originName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCL__TRYLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCL__RELEASELOCK, "_rtnContextReturnCL::_releaseLock" )
   INT32 _rtnContextReturnCL::_releaseLock( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCL__RELEASELOCK ) ;

      if ( 0 != _returnBlockID )
      {
         // WARNING: re-use block ID of rename context, should not reset
         const CHAR *originName = _returnOptions._recycleItem.getOriginName() ;
         _pFreezingWnd->unregisterCL( originName, _returnBlockID ) ;
         PD_LOG( PDEVENT, "End to block all write operations of "
                 "collection [%s], ID: %llu", originName, _returnBlockID ) ;
      }

      _BASE::_releaseLock( cb ) ;

      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCL__RELEASELOCK, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCL__DORENAME, "_rtnContextReturnCL::_doRename" )
   INT32 _rtnContextReturnCL::_doRename( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCL__DORENAME ) ;

      dmsMBContext *newMBContext = NULL ;

      SDB_ASSERT( NULL != _su, "storage unit is invalid" ) ;

      // return collection
      // NOTE: new name is origin name, old name is recycle name
      rc = _su->data()->returnCollection( _newCLShortName, _clShortName,
                                          _returnOptions, cb, _pDpsCB,
                                          &newMBContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to return collection from [%s] "
                   "to [%s], rc: %d", _clShortName, _newCLShortName, rc ) ;

      // enable compress if needed
      if ( NULL != newMBContext )
      {
         if ( OSS_BIT_TEST( newMBContext->mb()->_attributes,
                            DMS_MB_ATTR_COMPRESSED ) &&
              UTIL_COMPRESSOR_LZW == newMBContext->mb()->_compressorType &&
              DMS_INVALID_EXTENT == newMBContext->mb()->_dictExtentID )
         {
            _pDmsCB->pushDictJob( dmsDictJob( _su->CSID(),
                                              _su->LogicalCSID(),
                                              newMBContext->mbID(),
                                              newMBContext->clLID() ) ) ;
         }
         _su->data()->releaseMBContext( newMBContext ) ;
      }

      _deleteItem( cb ) ;

      // start index tasks
      sdbGetClsCB()->startIdxTaskCheckByCL(
            (utilCLUniqueID)( _returnOptions._recycleItem.getOriginID() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCL__DORENAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCL__INITLOCALTASK, "_rtnContextReturnCL::_initLocalTask" )
   INT32 _rtnContextReturnCL::_initLocalTask( rtnLocalTaskPtr &taskPtr,
                                              const CHAR *oldName,
                                              const CHAR *newName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCL__INITLOCALTASK ) ;

      SDB_ASSERT( NULL != oldName, "old name is invalid" ) ;
      SDB_ASSERT( NULL != newName, "new name is invalid" ) ;

      rtnLTReturnCL *pRenameTask =
                  dynamic_cast< rtnLTReturnCL * >( taskPtr.get() ) ;
      SDB_ASSERT( NULL != pRenameTask, "local task is invalid" ) ;

      pRenameTask->setInfo( oldName, newName, _returnOptions._recycleItem ) ;

      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCL__INITLOCALTASK, rc ) ;

      return rc ;
   }

   /*
      _rtnContextReturnCS implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextReturnCS,
                          RTN_CONTEXT_RETURNCS,
                          "RETURNCS" )
   _rtnContextReturnCS::_rtnContextReturnCS( SINT64 contextID, UINT64 eduID )
   : _BASE( contextID, eduID ),
     _returnBlockID( 0 )
   {
   }

   _rtnContextReturnCS::~_rtnContextReturnCS()
   {
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb      = eduMgr->getEDUByID( eduID() ) ;

      if ( RENAMECSPHASE_1 == _status )
      {
         _cancelRename( cb ) ;
         _status = RENAMECSPHASE_0 ;
      }

      _releaseLock( cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS_OPEN, "_rtnContextReturnCS::open" )
   INT32 _rtnContextReturnCS::open( const utilRecycleItem &recycleItem,
                                    const utilRecycleReturnInfo &returnInfo,
                                    _pmdEDUCB *cb,
                                    INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS_OPEN ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == recycleItem.getType(),
                  "invalid recycle type" ) ;

      const CHAR *originName = NULL ;
      const CHAR *recycleName = NULL ;

      utilReturnNameInfo nameInfo( recycleItem.getOriginName() ) ;

      _returnOptions._recycleItem = recycleItem ;

      rc = returnInfo.getReturnCSName( nameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name for collection "
                   "space, rc: %d", rc ) ;

      if ( nameInfo.isRenamed() )
      {
         _returnOptions._recycleItem.setOriginName( nameInfo.getReturnName() ) ;
      }

      PD_LOG( PDEVENT, "Got return collection space [origin %s, recycle %s, "
              "return %s, origin UID %llu, return UID %llu]",
              recycleItem.getOriginName(),
              recycleItem.getRecycleName(),
              _returnOptions._recycleItem.getOriginName(),
              recycleItem.getOriginID(),
              _returnOptions._recycleItem.getOriginID() ) ;

      originName = _returnOptions._recycleItem.getOriginName() ;
      recycleName = _returnOptions._recycleItem.getRecycleName() ;

      // from recycle to origin
      rc = _BASE::open( recycleName, originName, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open internal rename context, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _rtnContextReturnCS::_toString( stringstream &ss )
   {
      _BASE::_toString( ss ) ;
      ss << ",RecycleID:" <<
            _returnOptions._recycleItem.getRecycleID() <<
            ",RecycleOpType:" <<
            utilGetRecycleOpTypeName( _returnOptions._recycleItem.getOpType() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS__TRYLOCK, "_rtnContextReturnCS::_tryLock" )
   INT32 _rtnContextReturnCS::_tryLock( const CHAR *csName, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS__TRYLOCK ) ;

      rc = _pFreezingWnd->registerCS( _newName, _returnBlockID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to block collection [%s], rc: %d",
                   _newName, rc ) ;
      PD_LOG( PDEVENT, "Begin to block all write operations "
              "of collection [%s], ID: %llu", _newName, _returnBlockID ) ;

      rc = _BASE::_tryLock( csName, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock for return collection "
                   "space [%s] to [%s], rc: %d", _oldName, _newName,
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS__TRYLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS__RELEASELOCK, "_rtnContextReturnCS::_releaseLock" )
   INT32 _rtnContextReturnCS::_releaseLock( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS__RELEASELOCK ) ;

      if ( 0 != _returnBlockID )
      {
         // WARNING: re-use block ID of rename context, should not reset
         _pFreezingWnd->unregisterCS( _newName, _returnBlockID ) ;
         PD_LOG( PDEVENT, "End to block all write operations of "
                 "collection [%s], ID: %llu",
                 _newName, _returnBlockID ) ;
      }

      _BASE::_releaseLock( cb ) ;

      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS__RELEASELOCK, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS__DORENAMEP1, "_rtnContextReturnCS::_doRenameP1" )
   INT32 _rtnContextReturnCS::_doRenameP1( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS__DORENAMEP1 ) ;

      UINT32 retryTime = 0 ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      UINT32 suLogicalID = DMS_INVALID_LOGICCSID ;

      rc = _pDmsCB->nameToSULID( _oldName, suLogicalID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical ID for "
                   "collection space [%s], rc: %d", _oldName,
                   suLogicalID, rc ) ;
      SDB_ASSERT( DMS_INVALID_LOGICCSID != suLogicalID,
                  "logical ID should be valid" ) ;

      // let's find out whether the collection space is held by this
      // EDU. If so we have to get rid of those contexts
      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( _oldName, suLogicalID, cb ) ;
      }

      while ( TRUE )
      {
         if ( ( PMD_IS_DB_DOWN() ) ||
              ( NULL != cb && cb->isInterrupted() ) )
         {
            PD_LOG( PDWARNING, "Failed to return collection space [%s], "
                    "it is interrupted", _oldName ) ;
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         // tell others to close contexts on the same collection space
         if ( rtnCB->preDelContext( _oldName, suLogicalID ) > 0 )
         {
            ossSleep( 200 ) ;
         }

         /// rename cs at phase 1
         rc = _pDmsCB->returnCollectionSpaceP1( _returnOptions, cb, _pDpsCB ) ;
         if ( SDB_LOCK_FAILED == rc && retryTime < 100 )
         {
            ++ retryTime ;
            rc = SDB_OK ;
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to return collection space "
                      "[origin %s, recycle %s] at phase 1, rc: %d",
                      _returnOptions._recycleItem.getOriginName(),
                      _returnOptions._recycleItem.getRecycleName(),
                      rc ) ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS__DORENAMEP1, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS__DORENAME, "_rtnContextReturnCS::_doRename" )
   INT32 _rtnContextReturnCS::_doRename( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS__DORENAME ) ;

      rc = _pDmsCB->returnCollectionSpaceP2( _returnOptions, cb, _pDpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to return collection space "
                   "[origin %s, recycle %s], rc: %d",
                   _returnOptions._recycleItem.getOriginName(),
                   _returnOptions._recycleItem.getRecycleName(),
                   rc ) ;

      // make sure recycle item is deleted
      _deleteItem( cb ) ;

      // start index tasks
      sdbGetClsCB()->startIdxTaskCheckByCS(
            (utilCSUniqueID)( _returnOptions._recycleItem.getOriginID() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS__DORENAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS__CANCELRENAME, "_rtnContextReturnCS::_cancelRename" )
   INT32 _rtnContextReturnCS::_cancelRename( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS__CANCELRENAME ) ;

      INT32 tmpRC = SDB_OK ;
      tmpRC = _pDmsCB->returnCollectionSpaceP1Cancel( _returnOptions, cb,
                                                      _pDpsCB ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDERROR, "Failed to cancel return collection space "
                 "[origin %s, recycle %s], rc: %d",
                 _returnOptions._recycleItem.getOriginName(),
                 _returnOptions._recycleItem.getRecycleName(),
                 tmpRC ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS__CANCELRENAME, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNCS__INITLOCALTASK, "_rtnContextReturnCS::_initLocalTask" )
   INT32 _rtnContextReturnCS::_initLocalTask( rtnLocalTaskPtr &taskPtr,
                                              const CHAR *oldName,
                                              const CHAR *newName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNCS__INITLOCALTASK ) ;

      SDB_ASSERT( NULL != oldName, "old name is invalid" ) ;
      SDB_ASSERT( NULL != newName, "new name is invalid" ) ;

      rtnLTReturnCS *pRenameTask =
                  dynamic_cast< rtnLTReturnCS * >( taskPtr.get() ) ;
      SDB_ASSERT( NULL != pRenameTask, "local task is invalid" ) ;

      pRenameTask->setInfo( oldName, newName, _returnOptions._recycleItem ) ;

      PD_TRACE_EXITRC( SDB__RTNCTXRTRNCS__INITLOCALTASK, rc ) ;

      return rc ;
   }

   /*
      _rtnContextReturnMainCL implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextReturnMainCL,
                          RTN_CONTEXT_RETURNMAINCL,
                          "RETURNMAINCL" )

   _rtnContextReturnMainCL::_rtnContextReturnMainCL( SINT64 contextID,
                                                     UINT64 eduID )
   : _rtnContextBase( contextID, eduID ),
     _lockDMS( FALSE )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _dmsCB = krcb->getDMSCB() ;
      _rtnCB = krcb->getRTNCB() ;

      _hitEnd = FALSE ;
      _recycleFullName[ 0 ] = 0 ;
   }

   _rtnContextReturnMainCL::~_rtnContextReturnMainCL()
   {
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUCB *cb = eduMgr->getEDUByID( eduID() ) ;
      _clean( cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNMCL_OPEN, "_rtnContextReturnMainCL::open" )
   INT32 _rtnContextReturnMainCL::open( const utilRecycleItem &recycleItem,
                                        const utilRecycleReturnInfo &returnInfo,
                                        _pmdEDUCB *cb,
                                        INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNMCL_OPEN ) ;

      UTIL_RECY_ITEM_LIST subItemList ;

      utilReturnNameInfo nameInfo( recycleItem.getOriginName() ) ;
      utilReturnUIDInfo uidInfo( recycleItem.getOriginID() ) ;
      CHAR szName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

      SDB_ASSERT( recycleItem.isMainCL(), "invalid recycle item" ) ;

      _returnOptions._recycleItem = recycleItem ;
      _w = w ;

      rc = returnInfo.getReturnCLName( nameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                   "collection [%s], rc: %d", recycleItem.getOriginName(),
                   rc ) ;
      if ( nameInfo.isRenamed() )
      {
         _returnOptions._recycleItem.setOriginName( nameInfo.getReturnName() ) ;
      }

      cb->setCurProcessName( _returnOptions._recycleItem.getOriginName() ) ;

      rc = rtnResolveCollectionSpaceName(
            _returnOptions._recycleItem.getOriginName(),
            ossStrlen( _returnOptions._recycleItem.getOriginName() ),
            szName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve return name [%s], rc: %d",
                   _returnOptions._recycleItem.getOriginName(), rc ) ;

      ossSnprintf( _recycleFullName, sizeof( _recycleFullName ), "%s.%s",
                   szName, _returnOptions._recycleItem.getRecycleName() ) ;

      rc = returnInfo.getReturnCLUID( uidInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return unique ID for "
                   "collection [%s], rc: %d",
                   recycleItem.getOriginName(), rc ) ;
      if ( uidInfo.isChanged() )
      {
         _returnOptions._recycleItem.setOriginID( uidInfo.getReturnUID() ) ;
      }

      rc = _recycleBinMgr->getSubItems( _returnOptions._recycleItem, cb,
                                        subItemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection items, rc: %d",
                   rc ) ;

      rc = _dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check writable of dmsCB, "
                   "rc: %d", rc ) ;
      _lockDMS = TRUE ;

      for ( UTIL_RECY_ITEM_LIST_IT iter = subItemList.begin() ;
            iter != subItemList.end() ;
            ++ iter )
      {
         utilRecycleItem &subItem = *iter ;
         rc = _openSubContext( subItem, returnInfo, cb, _w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open sub-context, rc: %d", rc ) ;
      }

      _isOpened = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNMCL_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNMCL__CLEAN, "_rtnContextReturnMainCL::_clean" )
   void _rtnContextReturnMainCL::_clean( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCTXRTRNMCL__CLEAN ) ;

      if ( _lockDMS )
      {
         _dmsCB->writeDown( cb ) ;
         _lockDMS = FALSE ;
      }

      for ( SUBCL_CONTEXT_LIST::iterator iter = _subContextList.begin() ;
            iter != _subContextList.end() ;
            ++ iter )
      {
         if ( iter->second != -1 )
         {
            _rtnCB->contextDelete( iter->second, cb );
         }
      }
      _subContextList.clear() ;

      PD_TRACE_EXIT( SDB__RTNCTXRTRNMCL__CLEAN ) ;
   }

   void _rtnContextReturnMainCL::_toString( stringstream &ss )
   {
      ss << ",Name:" << _returnOptions._recycleItem.getRecycleName() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNMCL__OPENSUBCTX, "_rtnContextReturnMainCL::_openSubContext" )
   INT32 _rtnContextReturnMainCL::_openSubContext( const utilRecycleItem &subItem,
                                                   const utilRecycleReturnInfo &returnInfo,
                                                   _pmdEDUCB *cb,
                                                   INT32 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNMCL__OPENSUBCTX ) ;

      INT64 subContextID = -1 ;
      rtnContextReturnCL::sharePtr returnContext ;

      const CHAR *recycleName = subItem.getRecycleName() ;
      const CHAR *originName = subItem.getOriginName() ;

      utilReturnNameInfo nameInfo( subItem.getOriginName() ) ;

      // check if collection need to be renamed and changed unique ID
      rc = returnInfo.getReturnCLName( nameInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name for "
                   "collection [%s], rc: %d", subItem.getOriginName(),
                   rc ) ;

      rc = _rtnCB->contextNew( RTN_CONTEXT_RETURNCL,
                               returnContext,
                               subContextID,
                               cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create return collection context "
                   "to recycle collection, rc: %d", rc ) ;

      cb->switchToSubCL( nameInfo.getReturnName() ) ;
      rc = returnContext->open( subItem, returnInfo, cb, _w ) ;
      cb->switchToMainCL() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open sub-context for recycle "
                   "item [origin %s, recycle %s], rc: %d", originName,
                   recycleName, rc ) ;

      try
      {
         _subContextList.insert( make_pair( originName, subContextID ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save context for sub-collection [%s], "
                 "occur exception %s", originName, e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      subContextID = -1 ;

   done:
      if ( -1 != subContextID )
      {
         _rtnCB->contextDelete( subContextID, cb ) ;
         subContextID = -1 ;
      }
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNMCL__OPENSUBCTX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXRTRNMCL__PREPAREDATA, "_rtnContextReturnMainCL::_prepareData" )
   INT32 _rtnContextReturnMainCL::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCTXRTRNMCL__PREPAREDATA ) ;

      SUBCL_CONTEXT_LIST::iterator iterCtx ;

      /// drop sub collections
      iterCtx = _subContextList.begin() ;
      while( iterCtx != _subContextList.end() )
      {
         rtnContextBuf buffObj;
         rc = rtnGetMore( iterCtx->second, -1, buffObj, cb, _rtnCB ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDWARNING, "Failed to return main-collection, "
                    "should have one step to return sub-collection" ) ;
         }
         else if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get more from sub-context, "
                      "rc: %d", rc ) ;
         _subContextList.erase( iterCtx ++ ) ;
      }

      _clean( cb ) ;

      rc = SDB_DMS_EOC ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXRTRNMCL__PREPAREDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
