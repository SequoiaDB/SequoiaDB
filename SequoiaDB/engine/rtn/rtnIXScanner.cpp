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

   Source File Name = rtnIXScanner.cpp

   Descriptive Name = Runtime Index Scanner

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains code for index traversal,
   including advance, pause, resume operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnIXScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   _rtnIXScanner::_rtnIXScanner ( ixmIndexCB *indexCB,
                                  rtnPredicateList *predList,
                                  dmsStorageUnit *su, pmdEDUCB *cb )
   : _predList(predList),
     _listIterator(*predList),
     _order(Ordering::make(indexCB->keyPattern())),
     _su(su),
     _pMonCtxCB(NULL),
     _cb(cb),
     _direction(predList->getDirection())
   {
      SDB_ASSERT ( indexCB && _predList && _su,
                   "indexCB, predList and su can't be NULL" ) ;

      _dedupBufferSize = RTN_IXSCANNER_DEDUPBUFSZ_DFT ;
      indexCB->getIndexID ( _indexOID ) ;
      _indexCBExtent = indexCB->getExtentID () ;
      _indexLID = indexCB->getLogicalID() ;
      _indexCB = SDB_OSS_NEW ixmIndexCB ( _indexCBExtent, su->index(),
                                          NULL ) ;
      reset() ;
   }

   _rtnIXScanner::~_rtnIXScanner ()
   {
      if ( _indexCB )
      {
         SDB_OSS_DEL _indexCB ;
      }
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIXSCAN_RELORID1, "_rtnIXScanner::relocateRID" )
   INT32 _rtnIXScanner::relocateRID ( const BSONObj &keyObj,
                                      const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNIXSCAN_RELORID1 ) ;

      PD_CHECK ( _indexCB, SDB_OOM, error, PDERROR,
                 "Failed to allocate memory for indexCB" ) ;
      PD_CHECK ( _indexCB->isInitialized(), SDB_RTN_INDEX_NOTEXIST, error,
                 PDERROR, "Index does not exist" ) ;
      PD_CHECK ( _indexCB->getFlag() == IXM_INDEX_FLAG_NORMAL,
                 SDB_IXM_UNEXPECTED_STATUS, error, PDERROR,
                 "Unexpected index status: %d", _indexCB->getFlag() ) ;
      {
         monAppCB * pMonAppCB   = _cb ? _cb->getMonAppCB() : NULL ;
         dmsExtentID rootExtent = _indexCB->getRoot() ;
         ixmExtent root ( rootExtent, _su->index() ) ;
         BOOLEAN found          = FALSE ;
         rc = root.locate ( keyObj, rid, _order, _curIndexRID, found,
                            _direction, _indexCB ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to locate from new keyobj and rid: %s, %d,%d",
                       keyObj.toString().c_str(), rid._extent,
                       rid._offset ) ;
         _savedObj = keyObj.copy() ;
         _savedRID = rid ;
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
         DMS_MON_CONTEXT_COUNT_INC ( _pMonCtxCB, MON_INDEX_READ, 1 ) ;
      }
      _init = TRUE ;

   done :
      PD_TRACE_EXITRC ( SDB__RTNIXSCAN_RELORID1, rc ) ;
      return rc ;
   error :
      goto done ;
   }
   
   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIXSCAN_RELORID2, "_rtnIXScanner::relocateRID" )
   INT32 _rtnIXScanner::relocateRID ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNIXSCAN_RELORID2 ) ;
      monAppCB * pMonAppCB = _cb ? _cb->getMonAppCB() : NULL ;
      dmsExtentID rootExtent = _indexCB->getRoot() ;
      ixmExtent root ( rootExtent, _su->index() ) ;
      BOOLEAN found ;
      rc = root.locate ( _savedObj, _savedRID, _order, _curIndexRID, found,
                         _direction, _indexCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate from saved obj and rid: %s, %d,%d",
                  _savedObj.toString().c_str(), _savedRID._extent,
                  _savedRID._offset ) ;
         goto error ;
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
      DMS_MON_CONTEXT_COUNT_INC ( _pMonCtxCB, MON_INDEX_READ, 1 ) ;
   done :
      PD_TRACE_EXITRC ( SDB__RTNIXSCAN_RELORID2, rc ) ;
      return rc ;
   error :
      goto done ;
   }
   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIXSCAN_ADVANCE, "_rtnIXScanner::advance" )
   INT32 _rtnIXScanner::advance ( dmsRecordID &rid, BOOLEAN isReadOnly )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNIXSCAN_ADVANCE ) ;
      monAppCB * pMonAppCB = _cb ? _cb->getMonAppCB() : NULL ;
      ixmRecordID lastRID ;

   begin:
      SDB_ASSERT ( _indexCB, "_indexCB can't be NULL, call resumeScan first" ) ;
      if ( !_init )
      {
         dmsExtentID rootExtent = _indexCB->getRoot() ;
         ixmExtent root ( rootExtent, _su->index() ) ;
         rc = root.keyLocate ( _curIndexRID, BSONObj(),0,FALSE,
                               _listIterator.cmp(), _listIterator.inc(),
                               _order, _direction, _cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to locate first key, rc: %d", rc ) ;
            goto error ;
         }
         _init = TRUE ;
      }
      else
      {
         if ( _curIndexRID.isNull() )
         {
            rc = SDB_IXM_EOC ;
            goto done ;
         }
         ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;


         if ( _savedRID.isNull() )
         {
            lastRID = _curIndexRID ;
            rc = indexExtent.advance ( _curIndexRID, _direction ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to advance to next key, rc: %d", rc ) ;
               goto error ;
            }
            if ( lastRID == _curIndexRID )
            {
               _curIndexRID.reset() ;
            }
         }
         else if ( !isReadOnly )
         {
            dmsRecordID onDiskRID = indexExtent.getRID (
                  _curIndexRID._slot ) ;
            if ( onDiskRID.isNull() )
            {
               rc = relocateRID () ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to relocate RID, rc: %d", rc ) ;
                  goto error ;
               }
            }
            else if ( onDiskRID == _savedRID )
            {
               const CHAR *dataBuffer =
                  indexExtent.getKeyData ( _curIndexRID._slot );
               if ( dataBuffer )
               {
                  try
                  {
                     _curKeyObj = ixmKey(dataBuffer).toBson() ;
                     DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
                     DMS_MON_CONTEXT_COUNT_INC ( _pMonCtxCB, MON_INDEX_READ, 1 ) ;
                     if ( _curKeyObj.shallowEqual(_savedObj) )
                     {
                        rc = indexExtent.advance ( _curIndexRID, _direction ) ;
                        if ( rc )
                        {
                           PD_LOG ( PDERROR, "Failed to advance, rc: %d", rc ) ;
                           goto error ;
                        }
                     }
                     else
                     {
                        rc = relocateRID () ;
                        if ( rc )
                        {
                           PD_LOG ( PDERROR, "Failed to relocate RID, rc: :%d",
                                    rc ) ;
                           goto error ;
                        }
                     }
                  }
                  catch ( std::exception &e )
                  {
                     PD_LOG ( PDERROR, "Failed to convert buffer to bson from "
                              "current rid: %d,%d: %s", _curIndexRID._extent,
                              _curIndexRID._slot, e.what() ) ;
                     rc = SDB_SYS ;
                     goto error ;
                  }
               } // if ( dataBuffer )
               else
               {
                  PD_LOG ( PDERROR, "Unable to get buffer" ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } // if ( onDiskRID == _savedRID )
            else
            {
               rc = relocateRID () ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to relocate RID, rc: %d", rc ) ;
                  goto error ;
               }
            }
         } // if ( !isReadOnly )
         else
         {
            _savedRID.reset() ;
         }
      }
      while ( TRUE )
      {
         if ( _curIndexRID.isNull() )
         {
            rc = SDB_IXM_EOC ;
            goto done ;
         }
         ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;
         const CHAR *dataBuffer = indexExtent.getKeyData ( _curIndexRID._slot ) ;
         if ( !dataBuffer )
         {
            PD_LOG ( PDERROR, "Failed to get buffer from current rid: %d,%d",
                     _curIndexRID._extent, _curIndexRID._slot ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         try
         {
            try
            {
               _curKeyObj = ixmKey(dataBuffer).toBson() ;
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
               DMS_MON_CONTEXT_COUNT_INC ( _pMonCtxCB, MON_INDEX_READ, 1 ) ;
            }
            catch ( std::exception &e )
            {
               PD_RC_CHECK ( SDB_SYS, PDERROR,
                             "Failed to convert from buffer "
                             "to bson, rid: %d,%d: %s",
                             _curIndexRID._extent,
                             _curIndexRID._slot, e.what() ) ;
            }
            rc = _listIterator.advance ( _curKeyObj ) ;
            if ( -2 == rc )
            {
               rc = SDB_IXM_EOC ;
               goto done ;
            }
            else if ( rc >= 0 )
            {
               lastRID = _curIndexRID ;
               rc = indexExtent.keyAdvance ( _curIndexRID, _curKeyObj, rc,
                                             _listIterator.after(),
                                             _listIterator.cmp(),
                                             _listIterator.inc(),
                                             _order, _direction, _cb ) ;
               PD_RC_CHECK ( rc, PDERROR,
                             "Failed to advance, rc = %d", rc ) ;
               if ( lastRID == _curIndexRID )
               {
                  _curIndexRID.reset() ;
               }
               continue ;
            }
            else
            {
               _savedRID =
                     indexExtent.getRID ( _curIndexRID._slot ) ;
               if ( _savedRID.isNull() ||
                    _dupBuffer.end() != _dupBuffer.find ( _savedRID ) )
               {

                  _savedRID.reset() ;
                  goto begin ;
               }
               /*if ( _dupBuffer.size() >= _dedupBufferSize )
               {
                  rc = SDB_IXM_DEDUP_BUF_MAX ;
                  goto error ;
               }*/
               _dupBuffer.insert ( _savedRID ) ;
               rid = _savedRID ;
               if ( !isReadOnly )
               {
                  _savedObj = _curKeyObj.getOwned() ;
               }
               else
               {
                  _savedRID.reset() ;
               }
               rc = SDB_OK ;
               break ;
            }
         } // try
         catch ( std::exception &e )
         {
            PD_RC_CHECK ( SDB_SYS, PDERROR,
                          "exception during advance index tree: %s",
                          e.what() ) ;
         }
      } // while ( TRUE )
   done :
      PD_TRACE_EXITRC( SDB__RTNIXSCAN_ADVANCE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIXSCAN_PAUSESCAN, "_rtnIXScanner::pauseScan" )
   INT32 _rtnIXScanner::pauseScan( BOOLEAN isReadOnly )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNIXSCAN_PAUSESCAN ) ;
      if ( !_init || _curIndexRID.isNull() )
      {
         goto done ;
      }

      if ( !isReadOnly )
      {
         goto done ;
      }

      {
         ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;
         const CHAR *dataBuffer = indexExtent.getKeyData ( _curIndexRID._slot ) ;
         if ( !dataBuffer )
         {
            PD_LOG ( PDERROR, "Failed to get buffer from current rid: %d,%d",
                     _curIndexRID._extent, _curIndexRID._slot ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         try
         {
            _savedObj = ixmKey(dataBuffer).toBson().copy() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to convert buffer to bson from current "
                     "rid: %d,%d: %s", _curIndexRID._extent,
                     _curIndexRID._slot, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         _savedRID = indexExtent.getRID ( _curIndexRID._slot ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNIXSCAN_PAUSESCAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIXSCAN_RESUMESCAN, "_rtnIXScanner::resumeScan" )
   INT32 _rtnIXScanner::resumeScan( BOOLEAN isReadOnly )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNIXSCAN_RESUMESCAN ) ;

      if ( !_indexCB )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !_indexCB->isInitialized() )
      {
         rc = SDB_RTN_INDEX_NOTEXIST ;
         goto done ;
      }

      if ( _indexCB->getFlag() != IXM_INDEX_FLAG_NORMAL )
      {
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto done ;
      }

      if ( !_init || _curIndexRID.isNull() || _savedObj.isEmpty() )
      {
         rc = SDB_OK ;
         goto done ;
      }

      if ( !isReadOnly )
      {
         goto done ;
      }

      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_RTN_INDEX_NOTEXIST ;
         goto done ;
      }
      {
      ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;
      const CHAR *dataBuffer = NULL ;
      if ( indexExtent.isStillValid( _indexCB->getMBID() ) )
      {
         dataBuffer = indexExtent.getKeyData ( _curIndexRID._slot ) ;
      }

      if ( dataBuffer )
      {
         try
         {
            BSONObj curObj = ixmKey(dataBuffer).toBson() ;
            if ( curObj.shallowEqual(_savedObj) )
            {
               dmsRecordID onDiskRID = indexExtent.getRID (
                     _curIndexRID._slot ) ;
               if ( onDiskRID == _savedRID )
               {
                  _savedRID.reset() ;
                  goto done ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to convert buffer to bson from "
                     "current rid: %d,%d: %s", _curIndexRID._extent,
                     _curIndexRID._slot, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      {
         rc = relocateRID () ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to relocate RID, rc: %d", rc ) ;
            goto error ;
         }
      }
      }

   done :
      PD_TRACE_EXITRC ( SDB__RTNIXSCAN_RESUMESCAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

}

