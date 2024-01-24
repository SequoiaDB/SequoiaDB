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

   Source File Name = catContextRecycleBin.cpp

   Descriptive Name = Runtime Context of Catalog

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context of Catalog
   helper functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catContextRecycleBin.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "ossMemPool.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _catCMDReturnRecycleBinBase implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxReturnRecycleBin,
                          RTN_CONTEXT_CAT_RETURN_RECYCLEBIN,
                          "CAT_RETURN_RECYCLEBIN" )

   _catCtxReturnRecycleBin::_catCtxReturnRecycleBin( INT64 contextID,
                                                     UINT64 eduID )
   : _catCtxDataBase( contextID, eduID ),
     _catCtxRecycleHelper( UTIL_RECYCLE_UNKNOWN, UTIL_RECYCLE_OP_UNKNOWN ),
     _taskHandler( _lockMgr )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
      _recycleBinMgr->registerReturn() ;
   }

   _catCtxReturnRecycleBin::~_catCtxReturnRecycleBin()
   {
      _recycleBinMgr->unregisterReturn() ;
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN_OPEN, "_catCtxReturnRecycleBin::open" )
   INT32 _catCtxReturnRecycleBin::open( const BSONObj &queryObject,
                                        BOOLEAN isReturnToName,
                                        rtnContextBuf &buffObj,
                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN_OPEN ) ;

      _returnConf.setReturnToName( isReturnToName ) ;

      rc = _open( queryObject, MSG_BS_QUERY_REQ, buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open return recycle bin context, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__REGEVENTHANDLERS, "_catCtxReturnRecycleBin::_regEventHandlers" )
   INT32 _catCtxReturnRecycleBin::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__REGEVENTHANDLERS ) ;

      rc = _BASE::_regEventHandlers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register base handlers, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__PARSEQUERY, "_catCtxReturnRecycleBin::_parseQuery" )
   INT32 _catCtxReturnRecycleBin::_parseQuery( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__PARSEQUERY ) ;

      try
      {
         utilRecycleItem dummyItem ;

         rc = rtnGetSTDStringElement( _boQuery,
                                      FIELD_NAME_RECYCLE_NAME,
                                      _targetName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_RECYCLE_NAME, rc ) ;

         rc = dummyItem.fromRecycleName( _targetName.c_str() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse recycle item "
                      "name [%s], rc: %d", _targetName.c_str(), rc ) ;

         if ( _returnConf.isReturnToName() )
         {
            const CHAR *returnName = NULL ;

            PD_LOG_MSG_CHECK( !_boQuery.hasField( FIELD_NAME_ENFORCED1 ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to return recycle item to name, "
                              "should not have field [%s]",
                              FIELD_NAME_ENFORCED1 ) ;

            PD_LOG_MSG_CHECK( !_boQuery.hasField( FIELD_NAME_ENFORCED ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to return recycle item to name, "
                              "should not have field [%s]",
                              FIELD_NAME_ENFORCED ) ;

            rc = rtnGetStringElement( _boQuery, FIELD_NAME_RETURN_NAME,
                                      &returnName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         FIELD_NAME_RETURN_NAME, rc ) ;

            _returnConf.setReturnName( returnName ) ;
         }
         else
         {
            BOOLEAN isEnforced = FALSE ;

            PD_LOG_MSG_CHECK( !_boQuery.hasField( FIELD_NAME_RETURN_NAME ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to return recycle item, should not have "
                              "field [%s]", FIELD_NAME_RETURN_NAME ) ;

            rc = rtnGetBooleanElement( _boQuery,
                                       FIELD_NAME_ENFORCED1,
                                       isEnforced ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = rtnGetBooleanElement( _boQuery,
                                          FIELD_NAME_ENFORCED,
                                          isEnforced ) ;
               if ( SDB_FIELD_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
                  isEnforced = FALSE ;
               }
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         FIELD_NAME_ENFORCED1, rc ) ;

            _returnConf.setEnforced( isEnforced ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse query, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__PARSEQUERY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__CHECK_INT, "_catCtxReturnRecycleBin::_checkInternal" )
   INT32 _catCtxReturnRecycleBin::_checkInternal( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__CHECK_INT ) ;

      BOOLEAN lockedRecycleBin = FALSE ;

      INT16 w = _pCatCB->majoritySize() ;

      PD_CHECK( _recycleBinMgr->tryLockDropLatch(), SDB_LOCK_FAILED, error,
                PDERROR, "Failed to lock recycle bin to block drop expired "
                "background job" ) ;
      lockedRecycleBin = TRUE ;

      // wrap in try-catch to avoid leak of drop latch
      try
      {
         rc = _recycleBinMgr->getItem( _targetName.c_str(), cb, _recycleItem ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], "
                      "rc: %d", _targetName.c_str(), rc ) ;

         // lock recycle item
         rc = _recycleBinMgr->tryLockItem( _recycleItem, cb, EXCLUSIVE, _lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock recycle item "
                      "[origin %s, recycle %s], rc: %d",
                      _recycleItem.getOriginName(),
                      _recycleItem.getRecycleName(), rc ) ;

         if ( UTIL_RECYCLE_CL == _recycleItem.getType() )
         {
            rc = _checkReturnCL( _recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check return collection "
                         "[origin %s, recycle %s], rc: %d",
                         _recycleItem.getOriginName(),
                         _recycleItem.getRecycleName(), rc ) ;
         }
         else if ( UTIL_RECYCLE_CS == _recycleItem.getType() )
         {
            rc = _checkReturnCS( _recycleItem, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check return collection "
                         "space [origin %s, recycle %s], rc: %d",
                         _recycleItem.getOriginName(),
                         _recycleItem.getRecycleName(), rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to return recycle bin, unknown type [%d]",
                    _recycleItem.getType() ) ;
            SDB_ASSERT( FALSE, "invalid recycle type" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _returnInfo.checkConflicts( _recycleItem,
                                          _returnConf.isEnforced(),
                                          _returnConf.isReturnToName(),
                                          _lockMgr, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check conflicts for return "
                      "recycle item [origin %s, recycle %s], rc: %d",
                      _recycleItem.getOriginName(),
                      _recycleItem.getRecycleName(), rc ) ;

         rc = _returnInfo.lockReplaceCS( _lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock conflict collection spaces, "
                      "rc: %d", rc ) ;

         rc = _returnInfo.lockReplaceCL( _lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock conflict collections, "
                      "rc: %d", rc ) ;

         rc = _returnInfo.lockDomains( _lockMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock domains, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to check return recycle item, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( lockedRecycleBin )
      {
         _recycleBinMgr->unlockDropLatch() ;
         lockedRecycleBin = FALSE ;
      }
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__CHECK_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNCS, "_catCtxReturnRecycleBin::_checkReturnCS" )
   INT32 _catCtxReturnRecycleBin::_checkReturnCS( utilRecycleItem &recycleItem,
                                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNCS ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == recycleItem.getType(),
                  "recycle type is invalid" ) ;

      catReturnCSChecker csChecker( _recycleBinMgr,
                                    recycleItem,
                                    _returnConf,
                                    _returnInfo,
                                    _lockMgr,
                                    _groupHandler ) ;

      rc = _recycleBinMgr->processObjects( csChecker, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return collection "
                   "space [%s], rc: %d", recycleItem.getOriginName(), rc ) ;

      rc = _checkReturnIdx( csChecker, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return indexes on "
                   "collection space [%s], rc: %d", recycleItem.getOriginName(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNCL, "_catCtxReturnRecycleBin::_checkReturnCL" )
   INT32 _catCtxReturnRecycleBin::_checkReturnCL( utilRecycleItem &recycleItem,
                                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNCL ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == recycleItem.getType(),
                  "recycle type is invalid" ) ;

      SET_UINT32 groupIDSet ;

      catReturnCLChecker clChecker( _recycleBinMgr,
                                    recycleItem,
                                    _returnConf,
                                    _returnInfo,
                                    _lockMgr,
                                    _groupHandler ) ;

      rc = _recycleBinMgr->processObjects( clChecker, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return collection [%s], "
                   "rc: %d", recycleItem.getOriginName(), rc ) ;

      rc = _checkReturnIdx( clChecker, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return indexes on "
                   "collection [%s], rc: %d", recycleItem.getOriginName(),
                   rc ) ;

      // returning truncated collection, and no conflicts, we can use
      // on site return, to return the collection in the origin place
      if ( ( UTIL_RECYCLE_OP_TRUNCATE == recycleItem.getOpType() ) &&
           ( !( _returnInfo.hasConflicts() ) ) )
      {
         _returnInfo.setOnSiteReturn( TRUE ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNIDX, "_catCtxReturnRecycleBin::_checkReturnIdx" )
   INT32 _catCtxReturnRecycleBin::_checkReturnIdx( catReturnChecker &checker,
                                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNIDX ) ;

      catReturnIdxChecker idxChecker( checker,
                                      ( IXM_EXTENT_TYPE_TEXT |
                                        IXM_EXTENT_TYPE_GLOBAL ) ) ;

      rc = _recycleBinMgr->processObjects( idxChecker, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check return index for "
                   "recycle item [origin: %s, recycle: %s], rc: %d",
                   checker.getRecycleItem().getOriginName(),
                   checker.getRecycleItem().getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__CHECK_RTRNIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN_EXECUTE_INT, "_catCtxReturnRecycleBin::_executeInternal" )
   INT32 _catCtxReturnRecycleBin::_executeInternal( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN_EXECUTE_INT ) ;

      // need double check existence of data source
      // NOTE: we don't have lock for data source, and dropping data source
      //       won't check recycle bin
      _returnInfo.resetExistDSSet() ;

      rc = _recycleBinMgr->returnItem( _recycleItem, _returnInfo, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to return recycle item [%s] to [%s], "
                   "rc: %d", _recycleItem.getRecycleName(),
                   _recycleItem.getOriginName(), rc ) ;

      rc = _returnInfo.processMissingSubCL( cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process missing sub-collections, "
                   "rc: %d", rc ) ;

      rc = _returnInfo.processMissingMainCL( cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process missing main-collections, "
                   "rc: %d", rc ) ;

      rc = _returnInfo.processRebuildIndex( cb, _pDmsCB, _pDpsCB, w,
                                            _taskHandler.getTaskSet() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process rebuild indexes, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN_EXECUTE_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRTRNRECYBIN__BLDP1REPLY, "_catCtxReturnRecycleBin::_buildP1Reply" )
   INT32 _catCtxReturnRecycleBin::_buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXRTRNRECYBIN__BLDP1REPLY ) ;

      rc = _recycleItem.toBSON( builder, FIELD_NAME_RECYCLE_ITEM ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build reply for "
                   "recycle item [origin %s, recycle %s], rc: %d",
                   _recycleItem.getOriginName(), _recycleItem.getRecycleName(),
                   rc ) ;

      rc = _returnInfo.toBSON( builder, UTIL_RETURN_MASK_ALL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for return info, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXRTRNRECYBIN__BLDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
