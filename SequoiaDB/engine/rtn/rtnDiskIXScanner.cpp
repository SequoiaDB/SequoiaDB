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
#include "rtnDiskIXScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson;

namespace engine
{

   _rtnDiskIXScanner::_rtnDiskIXScanner ( ixmIndexCB *indexCB,
                                          rtnPredicateList *predList,
                                          _dmsStorageUnit *su,
                                          _pmdEDUCB *cb,
                                          BOOLEAN indexCBOwnned )
   :_rtnIXScanner( indexCB, predList, su, cb, indexCBOwnned ),
     _listIterator( *predList ),
     _pMonCtxCB(NULL)
   {
      reset() ;
   }

   _rtnDiskIXScanner::~_rtnDiskIXScanner ()
   {
   }

   IXScannerType _rtnDiskIXScanner::getType() const
   {
      return SCANNER_TYPE_DISK ;
   }

   IXScannerType _rtnDiskIXScanner::getCurScanType() const
   {
      return SCANNER_TYPE_DISK ;
   }

   void _rtnDiskIXScanner::disableByType( IXScannerType type )
   {
      /// do nothing
   }

   INT32 _rtnDiskIXScanner::getLockModeByType( IXScannerType type ) const
   {
      return -1 ;
   }

   BOOLEAN _rtnDiskIXScanner::isAvailable() const
   {
      return TRUE ;
   }

   void _rtnDiskIXScanner::setMonCtxCB( _monContextCB *monCtxCB )
   {
      _pMonCtxCB = monCtxCB ;
   }

   void _rtnDiskIXScanner::reset()
   {
      _savedObj = BSONObj() ;
      _savedRID.reset() ;
      _curIndexRID.reset() ;
      _listIterator.reset() ;

      if ( _pInfo )
      {
         _pInfo->clear() ;
      }
      _init = FALSE ;
   }

   // change the scanner's current location to a given key and rid
   // User can indicate if they want to reset _savedObj/_savedRID using 
   // selected index RID position (_curIndexRID)
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RELORID1, "_rtnDiskIXScanner::relocateRID" )
   INT32 _rtnDiskIXScanner::relocateRID( const BSONObj &keyObj,
                                         const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_RELORID1 ) ;

      PD_CHECK ( _indexCB, SDB_OOM, error, PDERROR,
                 "Failed to allocate memory for indexCB" ) ;

      // sanity check, make sure we are on valid index
      PD_CHECK ( _indexCB->isInitialized(), SDB_DMS_INVALID_INDEXCB, error,
                 PDERROR, "Index does not exist" ) ;

      PD_CHECK ( _indexCB->getFlag() == IXM_INDEX_FLAG_NORMAL,
                 SDB_IXM_UNEXPECTED_STATUS, error, PDERROR,
                 "Unexpected index status: %d", _indexCB->getFlag() ) ;

      {
         monAppCB * pMonAppCB   = _cb ? _cb->getMonAppCB() : NULL ;
         // get root
         dmsExtentID rootExtent = _indexCB->getRoot() ;
         ixmExtent root ( rootExtent, _su->index() ) ;
         BOOLEAN found          = FALSE ;

         // locate the new key, the returned RID is stored in _curIndexRID
         rc = root.locate ( keyObj, rid, _order, _curIndexRID,
                            found, _direction, _indexCB ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to locate from new keyobj(%s) "
                       "and rid(%d,%d), rc: %d", keyObj.toString().c_str(),
                       rid._extent, rid._offset, rc ) ;

         _savedObj = keyObj.getOwned() ;
         _savedRID = rid ;

         if ( found && !isReadonly() )
         {
            _savedRID._offset -= 1 ;
            found = FALSE ;
         }

         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
      }
      // mark _init to true so that advance won't call keyLocate again
      _init = TRUE ;

      // unset the eof flag. There is a case when disk scan finished
      // but merge scan detect changes in mem tree scan due to possible update
      // or rollback. we must restart/continue the disk scan after relocateRID.
      _eof  = _curIndexRID.isNull() ? TRUE : FALSE ;

   done :
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_RELORID1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RELORID2, "_rtnDiskIXScanner::relocateRID" )
   INT32 _rtnDiskIXScanner::relocateRID( BOOLEAN &found )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_RELORID2 ) ;

      found = FALSE ;
      monAppCB * pMonAppCB = _cb ? _cb->getMonAppCB() : NULL ;

      // either the key doesn't exist, or we got key/rid not match,
      // or the key is psuedo-deleted, we all get here
      dmsExtentID rootExtent = _indexCB->getRoot() ;
      ixmExtent root ( rootExtent, _su->index() ) ;

      rc = root.locate ( _savedObj, _savedRID, _order, _curIndexRID,
                         found, _direction, _indexCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate from saved obj(%s) and "
                  "rid(%d,%d), rc: %d", _savedObj.toString().c_str(),
                  _savedRID._extent, _savedRID._offset, rc ) ;
         goto error ;
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;

   done :
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_RELORID2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // return SDB_IXM_EOC for end of index
   // return SDB_IXM_DEDUP_BUF_MAX if hit max dedup buffer
   // otherwise rid is set to dmsRecordID
   // advance() must be called between resumeScan() and pauseScan()
   // caller must make sure the table is locked before calling resumeScan, and
   // must release the table lock right after pauseScan
   //PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_ADVANCE, "_rtnDiskIXScanner::advance" )
   INT32 _rtnDiskIXScanner::advance ( dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_ADVANCE ) ;

      SDB_ASSERT ( _indexCB, "_indexCB can't be NULL" ) ;

      monAppCB * pMonAppCB = _cb ? _cb->getMonAppCB() : NULL ;
      ixmRecordID lastRID ;

   begin:
      // first time run after reset, we need to locate the first key
      if ( !_init )
      {
         // when we get here, we should always hold lock on the collection, so
         // _indexCB should remain valid until pauseScan() and resumeScan(). So
         // in resumeScan() we should always validate if the index still the
         // same
         dmsExtentID rootExtent = _indexCB->getRoot() ;
         ixmExtent root ( rootExtent, _su->index() ) ;
         rc = root.keyLocate ( _curIndexRID, BSONObj(), 0, FALSE,
                               _listIterator.cmp(), _listIterator.inc(),
                               _order, _direction, _cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to locate first key, rc: %d, "
                     "rootExtent: %d", rc, rootExtent ) ;
            goto error ;
         }
         _init = TRUE ;
      }
      // otherwise _curIndexRID is pointing to the previous scanned location,
      // we should start from there
      else
      {
         // make sure the current _curIndexRID is not NULL
         if ( _curIndexRID.isNull() )
         {
            // if we do come here, compiler will give error for the following
            // indexExtent object declare
            rc = SDB_IXM_EOC ;
            goto done ;
         }
         ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;
         // in readonly mode, _savedRID should always be NULL unless pauseScan()
         // + resumeScan() found the index structure is changed and we have
         // relocateRID(), in that case _savedRID may not be NULL for readonly
         // mode

         // in write mode, the possible NULL savedRID is that when the
         // previous read is a psuedo-delete, note resumeScan() shouldn't set
         // _savedRID to NULL in this case

         // In such scenario, we advance to next element
         if ( _savedRID.isNull() )
         {
            lastRID = _curIndexRID ;
            // changed during the time
            rc = indexExtent.advance ( _curIndexRID, _direction ) ;
            if ( rc )
            {
               goto error ;
            }
            if ( lastRID == _curIndexRID )
            {
               _curIndexRID.reset() ;
            }
         }
         // need to specially deal with the situation that index tree structure
         // may changed during scan
         // in this case _savedRID can't be NULL
         // advance happened only when both _savedRID and _savedObj matches on
         // disk version
         else if ( !isReadonly() )
         {
            // if it's update or delete index scan, the index structure may get
            // changed so everytime we have to compare _curIndexRID and ondisk
            // rid, as well as the stored object

            BOOLEAN isSame = FALSE ;
            BOOLEAN hasRead = FALSE ;

            rc = _isCursorSame( &indexExtent, _savedObj, _savedRID,
                                isSame, &hasRead ) ;
            if ( rc )
            {
               goto error ;
            }

            // if not the same, we must have something changed in
            // the index, let's relocate RID
            if ( !isSame )
            {
               rc = relocateRID( isSame ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to relocate RID, rc: %d", rc ) ;
                  goto error ;
               }
               if ( isSame && ( !getSharedInfo() ||
                                !getSharedInfo()->exists( _savedRID ) ) )
               {
                  isSame = FALSE ;
               }
            }
            // if both on recordRID and key object are the same, let's
            // say the index is not changed, that means we should move on
            // to the next.
            if ( isSame )
            {
               rc = indexExtent.advance ( _curIndexRID, _direction ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to advance, rc: %d", rc ) ;
                  goto error ;
               }
            }

            if ( hasRead )
            {
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
            }
         } // if ( !isReadonly() )
      }

      // after getting the first key location or advanced to next, let's
      // exame if this index key is what we want
      while ( TRUE )
      {
         // after getting _curIndexRID, we have to check if it's null
         if ( _curIndexRID.isNull() )
         {
            // if doing goto here, compile will get error for the following
            // indexExtent object declare
            rc = SDB_IXM_EOC ;
            goto done ;
         }
         // now let's get the binary key and create BSONObj from it
         ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;
         const CHAR *dataBuffer = indexExtent.getKeyData( _curIndexRID._slot ) ;
         if ( !dataBuffer )
         {
            PD_LOG ( PDERROR, "Failed to get buffer from current rid: %d,%d",
                     _curIndexRID._extent, _curIndexRID._slot ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         try
         {
            // get the key from index rid
            try
            {
               _builder.reset();
               _curKeyObj = ixmKey(dataBuffer).toBson( &_builder ) ;
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;
            }
            catch ( std::exception &e )
            {
               PD_RC_CHECK ( SDB_SYS, PDERROR,
                             "Failed to convert from buffer "
                             "to bson, rid: %d,%d: %s",
                             _curIndexRID._extent,
                             _curIndexRID._slot, e.what() ) ;
            }
            // compare the key in list iterator
            rc = _listIterator.advance ( _curKeyObj ) ;
            // if -2, that means we hit end of iterator, so all other keys in
            // index are not within our select range
            if ( -2 == rc )
            {
               rc = SDB_IXM_EOC ;
               goto done ;
            }
            // if >=0, that means the key is not selected and we want to
            // further advance the key in index
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
            // otherwise let's attempt to get dms rid
            else
            {
               _savedRID = indexExtent.getRID( _curIndexRID._slot ) ;
               // make sure the RID we read is not psuedo-deleted
               if ( _savedRID.isNull() || !_insert2Dup( _savedRID ) )
               {
                  // usually this means a psuedo-deleted rid, we should jump
                  // back to beginning of the function and advance to next
                  // key
                  // if we are able to find the recordid in dupBuffer, that
                  // means we've already processed the record, so let's also
                  // jump back to begin
                  _savedRID.reset() ;
                  goto begin ;
               }
               // make sure we don't hit maximum size of dedup buffer
               /*if ( _pInfo && _pInfo->isUpToLimit() )
               {
                  rc = SDB_IXM_DEDUP_BUF_MAX ;
                  goto error ;
               }*/

               // ready to return to caller
               rid = _savedRID ;

               // if we are write mode, let's record the _savedObj as well
               if ( !isReadonly() )
               {
                  _savedObj = _curKeyObj.getOwned() ;
               }
               // otherwise if we are read mode, let's reset _savedRID
               else
               {
                  // in readonly scenario, _savedRID should always be null
                  // unless pauseScan() is called
                  _savedRID.reset() ;
               }
               rc = SDB_OK ;
               break ;
            }
         } // try
         catch( std::bad_alloc &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         catch ( std::exception &e )
         {
            PD_RC_CHECK ( SDB_SYS, PDERROR,
                          "exception during advance index tree: %s",
                          e.what() ) ;
         }
      } // while ( TRUE )

   done :
      if ( SDB_IXM_EOC == rc )
      {
         _eof = TRUE ;
         rid.reset() ;

         PD_LOG( PDDEBUG, "Hit end with last obj(%s)",
                 _curKeyObj.toString().c_str() ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNDISKIXSCAN_ADVANCE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // save the bson key + rid for the current index rid, before releasing
   // X latch on the collection
   // we have to call resumeScan after getting X latch again just in case
   // other sessions changed tree structure
   // this is used for query scan only
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_PAUSESCAN, "_rtnDiskIXScanner::pauseScan" )
   INT32 _rtnDiskIXScanner::pauseScan()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_PAUSESCAN ) ;

      if ( !_init || _curIndexRID.isNull() )
      {
         goto done ;
      }

      // for write mode, since we write _savedRID and _savedObj in advance, we
      // don't do it here
      if ( isReadonly() )
      {
         const CHAR *dataBuffer = NULL ;
         // for read mode, let's copy savedobj then
         ixmExtent indexExtent( _curIndexRID._extent, _su->index() ) ;
         dataBuffer = indexExtent.getKeyData( _curIndexRID._slot ) ;
         if ( !dataBuffer )
         {
            PD_LOG ( PDERROR, "Failed to get buffer from current rid: %d,%d",
                     _curIndexRID._extent, _curIndexRID._slot ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         try
         {
            _savedObj = ixmKey(dataBuffer).toBson().getOwned() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to convert buffer to bson from current "
                     "rid: %d,%d: %s", _curIndexRID._extent,
                     _curIndexRID._slot, e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         _savedRID = indexExtent.getRID( _curIndexRID._slot ) ;

         PD_LOG( PDDEBUG, "Paused in obj(%s) with rid(%d,%d)",
                 _savedObj.toString().c_str(),
                 _savedRID._extent, _savedRID._offset ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_PAUSESCAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // restoring the bson key and rid for the current index rid. This is done by
   // comparing the current key/rid pointed by _curIndexRID still same as
   // previous saved one. If so it means there's no change in this key, and we
   // can move on the from the current index rid. Otherwise we have to locate
   // the new position for the saved key+rid
   // this is used in query scan only
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKIXSCAN_RESUMESCAN, "_rtnDiskIXScanner::resumeScan" )
   INT32 _rtnDiskIXScanner::resumeScan( BOOLEAN *pIsCursorSame )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNDISKIXSCAN_RESUMESCAN ) ;
      BOOLEAN isSame = TRUE ;

      _curKeyObj = BSONObj() ;

      if ( !_indexCB )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !_indexCB->isInitialized() )
      {
         rc = SDB_DMS_INIT_INDEX ;
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

      // compare the historical index OID with the current index oid, to make
      // sure the index is not changed during the time
      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto done ;
      }

      rc = isCursorSame( _savedObj, _savedRID, isSame ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( isSame )
      {
         _curKeyObj = _savedObj ;
      }

      if ( !isReadonly() )
      {
         goto done ;
      }

      if ( isSame )
      {
         // this means the last scaned record is still here, so let's
         // reset _savedRID so that we'll call advance()
         _savedRID.reset() ;
      }
      else
      {
         // when we get here, it means something changed and we need to
         // relocateRID
         // note relocateRID may relocate to the index that already read.
         // However after advance() returning the RID we'll check if the
         // index already has been read, so we should be safe to not
         // reset _savedRID
         rc = relocateRID( isSame ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to relocate RID, rc: %d", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Relocate in obj(%s) with rid(%d,%d), found(%d)",
                 _savedObj.toString().c_str(), _savedRID._extent,
                 _savedRID._offset, isSame ) ;

         if ( isSame )
         {
            _savedRID.reset() ;
            _curKeyObj = _savedObj ;
         }
      }

   done:
      if ( pIsCursorSame )
      {
         *pIsCursorSame = isSame ;
      }
      PD_TRACE_EXITRC ( SDB__RTNDISKIXSCAN_RESUMESCAN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   rtnPredicateListIterator* _rtnDiskIXScanner::getPredicateListInterator()
   {
      return &_listIterator ;
   }

   INT32 _rtnDiskIXScanner::isCursorSame( const BSONObj &saveObj,
                                          const dmsRecordID &saveRID,
                                          BOOLEAN &isSame )
   {
      INT32 rc = SDB_OK ;

      isSame = FALSE ;

      if ( _init && !_curIndexRID.isNull() )
      {
         ixmExtent indexExtent ( _curIndexRID._extent, _su->index() ) ;
         if ( indexExtent.isStillValid( _indexCB->getMBID() ) )
         {
            rc = _isCursorSame( &indexExtent, saveObj, saveRID, isSame ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnDiskIXScanner::_isCursorSame( ixmExtent *pExtent,
                                           const BSONObj &saveObj,
                                           const dmsRecordID &saveRID,
                                           BOOLEAN &isSame,
                                           BOOLEAN *hasRead )
   {
      INT32 rc = SDB_OK ;
      dmsRecordID onDiskRID ;
      const CHAR *dataBuffer = NULL ;

      isSame = FALSE ;

      onDiskRID = pExtent->getRID ( _curIndexRID._slot ) ;

      if ( onDiskRID.isNull() || onDiskRID != saveRID )
      {
         goto done ;
      }

      dataBuffer = pExtent->getKeyData ( _curIndexRID._slot ) ;
      if ( dataBuffer )
      {
         try
         {
            BSONObj obj = ixmKey(dataBuffer).toBson() ;
            if ( hasRead  )
            {
               *hasRead = TRUE ;
            }

            /// compare
            if ( obj.shallowEqual( saveObj ) )
            {
               isSame = TRUE ;
               goto done ;
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
      else
      {
         PD_LOG ( PDERROR, "Unable to get buffer from current rid: %s,%d",
                  _curIndexRID._extent, _curIndexRID._slot ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

