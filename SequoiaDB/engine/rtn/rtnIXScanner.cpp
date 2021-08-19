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

   Descriptive Name = RunTime Index Scanner Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/09/2018  YXC Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnIXScanner.hpp"
#include "dmsStorageUnit.hpp"

using namespace bson ;

namespace engine
{

   #define RTN_IXSCANNER_DEDUPBUFSZ_DFT      (1024*1024)
   /*
      _rtnScannerSharedInfo define
   */
   _rtnScannerSharedInfo::_rtnScannerSharedInfo()
   {
   }

   _rtnScannerSharedInfo::~_rtnScannerSharedInfo()
   {
   }

   BOOLEAN _rtnScannerSharedInfo::insert( const dmsRecordID &rid )
   {
      return _setDuplicate.insert( rid ).second ;
   }

   BOOLEAN _rtnScannerSharedInfo::remove( const dmsRecordID &rid )
   {
      return _setDuplicate.erase( rid ) ;
   }

   BOOLEAN _rtnScannerSharedInfo::exists( const dmsRecordID &rid ) const
   {
      return _setDuplicate.find( rid ) != _setDuplicate.end() ?
             TRUE : FALSE ;
   }

   void _rtnScannerSharedInfo::clear()
   {
      _setDuplicate.clear() ;
   }

   BOOLEAN _rtnScannerSharedInfo::isUpToLimit() const
   {
      return _setDuplicate.size() > RTN_IXSCANNER_DEDUPBUFSZ_DFT ?
             TRUE : FALSE ;
   }

   UINT64 _rtnScannerSharedInfo::getSize() const
   {
      return _setDuplicate.size() ;
   }

   /*
      _rtnIXScanner implement
   */
   _rtnIXScanner::_rtnIXScanner( ixmIndexCB *pIndexCB,
                                 rtnPredicateList *predList,
                                 _dmsStorageUnit *su,
                                 _pmdEDUCB *cb,
                                 BOOLEAN indexCBOwned )
   :_direction( predList->getDirection() ),
    _indexLID( pIndexCB->getLogicalID() ),
    _indexCBExtent( pIndexCB->getExtentID() ),
    _order( Ordering::make( pIndexCB->keyPattern() ) )
   {
      _indexCB = NULL ;
      _owned = FALSE ;
      _pPredList = predList ;
      _su = su ;
      _cb = cb ;
      _isReadonly = TRUE ;
      _eof = FALSE ;

      /// set shared info pointer
      _pInfo = &_sharedInfo ;
      pIndexCB->getIndexID( _indexOID ) ;

      if ( indexCBOwned )
      {
         _indexCB = pIndexCB ;
      }
      else if ( DMS_INVALID_EXTENT != _indexCBExtent )
      {
         _indexCB = SDB_OSS_NEW ixmIndexCB ( _indexCBExtent,
                                             _su->index(),
                                             NULL ) ;
         _owned = TRUE ;
      }
   }

   _rtnIXScanner::~_rtnIXScanner()
   {
      if ( _indexCB && _owned )
      {
         SDB_OSS_DEL _indexCB ;
         _indexCB = NULL ;
      }
      _owned = FALSE ;
   }

   INT32 _rtnIXScanner::init()
   {
      INT32 rc = SDB_OK ;

      if ( !_su || !_cb )
      {
         PD_LOG( PDERROR, "StorageUnit or EDUCB is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !_indexCB || !_indexCB->isInitialized() )
      {
         PD_LOG( PDERROR, "IndexCB is invalid" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnIXScanner::setReadonly( BOOLEAN isReadonly )
   {
      _isReadonly = isReadonly ;
   }

   BOOLEAN _rtnIXScanner::isReadonly() const
   {
      return _isReadonly ;
   }

   void _rtnIXScanner::setShareInfo( rtnScannerSharedInfo *pInfo )
   {
      _pInfo = pInfo ;
   }

   BOOLEAN _rtnIXScanner::isShareInfoFromSelf() const
   {
      return _pInfo == &_sharedInfo ? TRUE : FALSE ;
   }

   BOOLEAN _rtnIXScanner::removeDuplicatRID( const dmsRecordID &rid )
   {
      if ( _pInfo )
      {
         return _pInfo->remove( rid ) ;
      }
      return FALSE ;
   }

   dmsExtentID _rtnIXScanner::getIdxLID() const
   {
      return _indexLID ;
   }

   const OID& _rtnIXScanner::getIdxOID() const
   {
      return _indexOID ;
   }

   INT32 _rtnIXScanner::getDirection() const
   {
      return _direction ;
   }

   rtnPredicateList* _rtnIXScanner::getPredicateList()
   {
      return _pPredList ;
   }

   rtnScannerSharedInfo* _rtnIXScanner::getSharedInfo()
   {
      return _pInfo ;
   }

   ixmIndexCB* _rtnIXScanner::getIndexCB()
   {
      return _indexCB ;
   }

   _dmsStorageUnit* _rtnIXScanner::getSu()
   {
      return _su ;
   }

   BOOLEAN _rtnIXScanner::getIndexCBOwned() const
   {
      return _owned ;
   }

   _pmdEDUCB* _rtnIXScanner::getEDUCB()
   {
      return _cb ;
   }

   INT32 _rtnIXScanner::compareWithCurKeyObj( const BSONObj &keyObj ) const
   {
      return getCurKeyObj()->woCompare( keyObj, _order, false ) * _direction ;
   }

   BOOLEAN _rtnIXScanner::eof() const
   {
      return _eof ;
   }

   BOOLEAN _rtnIXScanner::_insert2Dup( const dmsRecordID &rid )
   {
      if ( _pInfo )
      {
         return _pInfo->insert( rid ) ;
      }
      return TRUE ;
   }

   INT32 _rtnIXScanner::syncPredStatus( _rtnIXScanner *source )
   {
      INT32 rc = SDB_OK ;

      rtnPredicateListIterator *targetPredIter = NULL ;
      rtnPredicateListIterator *sourcePredIter = NULL ;
      if ( NULL == source )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Source can't be NULL rc: %d", rc ) ;
         goto error ;
      }

      sourcePredIter = source->getPredicateListInterator() ;
      if ( NULL == sourcePredIter )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Source predicate list can't be NULL rc:%d", rc ) ;
         goto error ;
      }

      targetPredIter = getPredicateListInterator() ;
      if ( NULL == targetPredIter )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Target predicate list can't be NULL rc:%d", rc ) ;
         goto error ;
      }

      rc = targetPredIter->syncState( sourcePredIter ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync state rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}


