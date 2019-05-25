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

   Source File Name = dmsSMEMgr.cpp

   Descriptive Name = Data Management Service Space Management Extent Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   space management extent manager, which is reponsible for fast free extent
   lookup and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/17/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsSMEMgr.hpp"
#include "dmsStorageBase.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{
   /*
      _dmsSegmentSpace : implement
   */
   _dmsSegmentSpace::_dmsSegmentSpace ( dmsExtentID startExtent, UINT16 maxNode,
                                        _dmsSMEMgr *pSMEMgr )
   {
      _maxNode     = 0 ;
      _startExtent = startExtent ;
      _totalSize   = maxNode ;
      _totalFree   = 0 ;
      _pSMEMgr     = pSMEMgr ;
   }

   _dmsSegmentSpace::~_dmsSegmentSpace ()
   {
      _freeSpaceList.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMS__RSTMAX, "_dmsSegmentSpace::_resetMax" )
   void _dmsSegmentSpace::_resetMax ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSMS__RSTMAX ) ;
      list<_dmsSegmentNode>::iterator it ;
      _maxNode = 0 ;
      UINT32 size = 0 ;
      for ( it = _freeSpaceList.begin(); it != _freeSpaceList.end(); ++it )
      {
         _dmsSegmentNode &node = (*it) ;
         size = DMS_SEGMENT_NODE_GETSIZE( node ) ;
         if ( size > _maxNode )
         {
            _maxNode = size ;
         }
      }
      PD_TRACE_EXIT ( SDB__DMSSMS__RSTMAX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMS_RSVPAGES, "_dmsSegmentSpace::reservePages" )
   INT32 _dmsSegmentSpace::reservePages ( UINT16 numPages,
                                          dmsExtentID &foundPage,
                                          UINT32 pos,
                                          ossAtomic32 *pFreePos )
   {
      INT32 rc = SDB_DMS_NOSPC ;
      PD_TRACE_ENTRY ( SDB__DMSSMS_RSVPAGES );
      UINT16 oldsize = 0 ;
      UINT16 newsize = 0 ;
      list<_dmsSegmentNode>::iterator it ;

      ossScopedLock lock( &_mutex ) ;

      if ( 0 == _totalFree )
      {
         pFreePos->compareAndSwap( pos, pos + 1 ) ;
      }

      if ( numPages > _maxNode )
      {
         rc = SDB_DMS_NOSPC ;
         goto error ;
      }

      for ( it = _freeSpaceList.begin(); it != _freeSpaceList.end(); ++it )
      {
         _dmsSegmentNode &node = (*it) ;
         oldsize = DMS_SEGMENT_NODE_GETSIZE( node ) ;
         if ( oldsize >= numPages )
         {
            foundPage = DMS_SEGMENT_NODE_GETSTART( node ) ;
            if ( DMS_SEGMENT_NODE_GETSIZE( node ) == numPages )
            {
               _freeSpaceList.erase ( it ) ;
            }
            else
            {
               newsize = oldsize - numPages ;
               DMS_SEGMENT_NODE_SET ( node, foundPage+numPages, newsize ) ;
            }

            foundPage += _startExtent ;
            _totalFree -= numPages ;

            if ( oldsize == _maxNode )
            {
               _resetMax () ;
            }
            SDB_ASSERT ( _maxNode != _totalSize, "Internal logic error" ) ;
            rc = SDB_OK ;

            {
               UINT32 bitStart = (UINT32)foundPage ;
               UINT32 bitEnd   = bitStart + numPages ;
               for ( ; bitStart < bitEnd ; bitStart++ )
               {
                  SDB_ASSERT( DMS_SME_FREE == _pSMEMgr->_pSME->getBitMask( bitStart ),
                              "SME corrupted, bit must be free" ) ;
                  _pSMEMgr->_pSME->setBitMask( bitStart ) ;
               }
               _pSMEMgr->_totalFree.sub( numPages ) ;
            }
            goto done ;
         } // if ( oldsize >= numPages )
      } // for ( it = _freeSpaceList.begin();

      PD_LOG ( PDSEVERE, "Internal logic error" ) ;
      SDB_ASSERT ( FALSE, "Internal logic error" ) ;
      rc = SDB_SYS ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMS_RSVPAGES, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMS_RLSPAGES, "_dmsSegmentSpace::releasePages" )
   INT32 _dmsSegmentSpace::releasePages ( dmsExtentID start, UINT16 numPages,
                                          BOOLEAN bitSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSMS_RLSPAGES );
      list<_dmsSegmentNode>::iterator it ;
      list<_dmsSegmentNode>::iterator it1 ;
      list<_dmsSegmentNode>::iterator oldit ;
      _dmsSegmentNode newnode   = 0 ;
      UINT16          newSize   = 0 ;
      BOOLEAN         prevExist = FALSE ;
      UINT16          relaStart = (UINT16)(start - _startExtent) ;

      ossScopedLock lock( &_mutex ) ;

      if ( start < _startExtent ||
           (start+numPages) > (_startExtent + _totalSize) )
      {
         PD_LOG ( PDERROR, "release pages is out of range: request %d:%d; "
                  "contains %d:%d", start, numPages, _startExtent,
                  _totalSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( 0 == numPages )
      {
         goto done ;
      }

      DMS_SEGMENT_NODE_SET ( newnode, relaStart, numPages ) ;

      if ( _freeSpaceList.empty () )
      {
         SDB_ASSERT ( _maxNode == 0, "max node must be 0 when free space is "
                      "empty" ) ;
         _freeSpaceList.push_back ( newnode ) ;
         _maxNode = numPages ;
         goto done ;
      }

      for ( it = _freeSpaceList.begin(); ; ++it )
      {
         if ( _freeSpaceList.end() == it )
         {
            newSize = numPages ;
            _freeSpaceList.push_back ( newnode ) ;
            if ( newSize > _maxNode )
            {
               _maxNode = newSize ;
            }

            list<_dmsSegmentNode>::reverse_iterator rit =
               _freeSpaceList.rbegin();
            it1 = (++rit).base() ;
            break ;
         }

         _dmsSegmentNode &node = (*it) ;
         UINT16      nodeStart = DMS_SEGMENT_NODE_GETSTART( node ) ;
         UINT16      nodeSize  = DMS_SEGMENT_NODE_GETSIZE( node ) ;
         if ( relaStart < nodeStart )
         {
            if ( relaStart + numPages > nodeStart )
            {
               PD_LOG ( PDERROR, "Internal logic error" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            else if ( relaStart + numPages == nodeStart )
            {
               newSize = nodeSize + numPages ;
               DMS_SEGMENT_NODE_SET ( node, relaStart, newSize ) ;
               if ( newSize > _maxNode )
               {
                  _maxNode = newSize ;
               }
               it1 = it ;
            }
            else
            {
               newSize = numPages ;
               it1 = _freeSpaceList.insert ( it, newnode ) ;
               if ( newSize > _maxNode )
               {
                  _maxNode = newSize ;
               }
            }
            break ;
         } // if ( start < nodeStart )
         else if ( relaStart == nodeStart ||
                   nodeStart + nodeSize > relaStart )
         {
            PD_LOG ( PDERROR, "Internal logic error" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         oldit = it ;
         prevExist = TRUE ;
      } // for ( it = _freeSpaceList.begin();

      if ( prevExist )
      {
         _dmsSegmentNode &prevNode = (*oldit ) ;
         UINT16      prevStart = DMS_SEGMENT_NODE_GETSTART( prevNode ) ;
         UINT16      prevSize  = DMS_SEGMENT_NODE_GETSIZE( prevNode ) ;
         if ( prevStart + prevSize > relaStart )
         {
            PD_LOG ( PDERROR, "Internal logic error" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( prevStart + prevSize == relaStart )
         {
            DMS_SEGMENT_NODE_SETSIZE ( prevNode, newSize+prevSize ) ;
            _freeSpaceList.erase ( it1 ) ;
            if ( newSize+prevSize > _maxNode )
            {
               _maxNode = newSize+prevSize ;
            }
         }
      }

   done :
      if ( SDB_OK == rc )
      {
         _totalFree += numPages ;

         if ( bitSet )
         {
            UINT32 bitStart = (UINT32)start ;
            UINT32 bitEnd   = bitStart + numPages ;
            for ( ; bitStart < bitEnd ; bitStart++ )
            {
               SDB_ASSERT( DMS_SME_ALLOCATED == _pSMEMgr->_pSME->getBitMask( bitStart ),
                           "SME corrupted, bit must be allocated" ) ;
               _pSMEMgr->_pSME->freeBitMask( bitStart ) ;
            }
            _pSMEMgr->_totalFree.add( numPages ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB__DMSSMS_RLSPAGES, rc );
      return rc ;
   error :
      goto done ;
   }

   UINT16 _dmsSegmentSpace::totalFree ()
   {
      ossScopedLock lock( &_mutex ) ;
      return _totalFree ;
   }

   /*
      _dmsSMEMgr : implement
   */
   _dmsSMEMgr::_dmsSMEMgr ()
   :_totalFree( 0 ), _freePos( 0 )
   {
      _pStorageBase  = NULL ;
      _pSME          = NULL ;
   }

   _dmsSMEMgr::~_dmsSMEMgr ()
   {
      vector<dmsSegmentSpace*>::iterator it ;
      for ( it = _segments.begin(); it != _segments.end(); ++it )
      {
         SDB_OSS_DEL ( *it ) ;
      }
      _segments.clear() ;
      _pStorageBase = NULL ;
      _pSME = NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMEMGR_INIT, "_dmsSMEMgr::init" )
   INT32 _dmsSMEMgr::init ( _dmsStorageBase *pStorageBase,
                            _dmsSpaceManagementExtent *pSME )
   {
      PD_TRACE_ENTRY ( SDB__DMSSMEMGR_INIT ) ;

      INT32 rc          = SDB_OK ;
      _pageSize         = pStorageBase->pageSize() ;
      _pStorageBase     = pStorageBase ;
      _pSME             = pSME ;

      UINT32 segmentPages = pStorageBase->segmentPages() ;
      UINT32 segmentPagesSqure = pStorageBase->segmentPagesSquareRoot() ;
      UINT32 releaseBegin  = 0 ;
      BOOLEAN inUse        = FALSE ;
      dmsSegmentSpace      *newspace = NULL ;
      UINT32 i             = 0 ;

      for ( i = 0 ; i < pStorageBase->pageNum() ; ++i )
      {
         if ( 0 == ( i & ( ( 1 << segmentPagesSqure ) - 1 ) ) )
         {
            if ( newspace && !inUse )
            {
               rc = newspace->releasePages( releaseBegin, i - releaseBegin,
                                            FALSE ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to release pages, rc = %d", rc ) ;
                  goto error ;
               }
               _totalFree.add( i - releaseBegin ) ;
            }

            newspace = SDB_OSS_NEW dmsSegmentSpace( i, segmentPages, this ) ;
            if ( NULL == newspace )
            {
               PD_LOG ( PDERROR, "Unable to allocate memory" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            _segments.push_back( newspace ) ;
            inUse = FALSE ;
            releaseBegin = i ;
         }

         if ( DMS_SME_FREE != pSME->getBitMask( i ) && !inUse )
         {
            rc = newspace->releasePages( releaseBegin, i - releaseBegin,
                                         FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to release pages, rc = %d", rc ) ;
               goto error ;
            }
            _totalFree.add( i - releaseBegin ) ;
            inUse = TRUE ;
         }
         else if ( DMS_SME_FREE == pSME->getBitMask( i ) && inUse )
         {
            inUse = FALSE ;
            releaseBegin = i ;
         }
      }

      if ( i > 0 && DMS_SME_FREE == pSME->getBitMask( i - 1 ) )
      {
         rc = newspace->releasePages ( releaseBegin, i - releaseBegin, FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to release pages, rc = %d", rc ) ;
            goto error ;
         }
         _totalFree.add( i - releaseBegin ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMEMGR_INIT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMEMGR_RSVPAGES, "_dmsSMEMgr::reservePages" )
   INT32 _dmsSMEMgr::reservePages ( UINT16 numPages, dmsExtentID &foundPage,
                                    UINT32 *pSegmentNum )
   {
      PD_TRACE_ENTRY ( SDB__DMSSMEMGR_RSVPAGES ) ;

      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      UINT32 size = 0 ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      size = _segments.size() ;
      pos = _freePos.fetch() ;

      while( pos < size )
      {
         rc = _segments[pos]->reservePages( numPages, foundPage,
                                            pos, &_freePos ) ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }
         else if ( SDB_DMS_NOSPC != rc )
         {
            PD_LOG ( PDERROR, "Failed to reserve pages, rc = %d", rc ) ;
            goto error ;
         }
         ++pos ;
      }

      rc = SDB_OK ;
      foundPage = DMS_INVALID_EXTENT ;

   done :
      if ( pSegmentNum )
      {
         *pSegmentNum = _segments.size() ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSMEMGR_RSVPAGES, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMEMGR_RLSPAGES, "_dmsSMEMgr::releasePages" )
   INT32 _dmsSMEMgr::releasePages ( dmsExtentID start, UINT16 numPages )
   {
      PD_TRACE_ENTRY ( SDB__DMSSMEMGR_RLSPAGES ) ;

      INT32 rc             = SDB_OK ;
      UINT32 segmentPagesSqure = _pStorageBase->segmentPagesSquareRoot () ;
      UINT32 maxSegments   = DMS_MAX_PG >> segmentPagesSqure ;
      UINT32 segmentID     = start >> segmentPagesSqure ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      SDB_ASSERT ( segmentID < maxSegments, "extent is out of range" ) ;
      if ( segmentID >= (UINT32)_segments.size() )
      {
         PD_LOG ( PDERROR, "Extent id %d is out of range", start ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _segments[segmentID]->releasePages ( start, numPages, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page for segment %d at "
                  "start %d, rc = %d", segmentID, start, rc ) ;
         goto error ;
      }
      _freePos.swapLesserThan( segmentID ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMEMGR_RLSPAGES, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMEMGR_DEPOSIT, "_dmsSMEMgr::deposit" )
   INT32 _dmsSMEMgr::depositASegment ( dmsExtentID start )
   {
      PD_TRACE_ENTRY ( SDB__DMSSMEMGR_DEPOSIT ) ;

      INT32 rc             = SDB_OK ;
      UINT16 segmentPages  = (UINT16)_pStorageBase->segmentPages() ;
      UINT32 segmentPagesSqure = _pStorageBase->segmentPagesSquareRoot() ;
      
      UINT32 maxSegments   = DMS_MAX_PG >> segmentPagesSqure ;
      UINT32 segmentID     = start >> segmentPagesSqure ;
      dmsSegmentSpace *newspace = NULL ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      SDB_ASSERT ( segmentID < maxSegments, "extent is out of range" ) ;
      if ( segmentID != (UINT32)_segments.size() )
      {
         PD_LOG ( PDERROR, "Extent id %d is not valid", start ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      newspace = SDB_OSS_NEW dmsSegmentSpace ( start, segmentPages, this ) ;
      if ( !newspace )
      {
         PD_LOG ( PDERROR, "Unable to allocate memory" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _segments.push_back ( newspace ) ;
      rc = newspace->releasePages ( start, segmentPages, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page for segment %d at "
                  "start %d, rc = %d", segmentID, start, rc ) ;
         goto error ;
      }
      _totalFree.add( segmentPages ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMEMGR_DEPOSIT, rc );
      return rc ;
   error :
      goto done ;
   }

   UINT32 _dmsSMEMgr::segmentNum ()
   {
      ossScopedRWLock lock( &_mutex, SHARED ) ;
      return _segments.size() ;
   }

   UINT32 _dmsSMEMgr::totalFree () const
   {
#ifdef _DEBUG
/*      UINT32 totalFree = 0 ;
      dmsSegmentSpace *pSpace = NULL ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;
      vector<dmsSegmentSpace*>::iterator it = _segments.begin() ;

      for ( ; it != _segments.end(); ++it )
      {
         pSpace = ( dmsSegmentSpace* )(*it) ;
         totalFree += ( UINT32 )( pSpace->totalFree() ) ;
      }

      SDB_ASSERT( _totalFree == totalFree, "Total free space error" ) ; */
#endif // _DEBUG
      return _totalFree.peek() ;
   }

}

