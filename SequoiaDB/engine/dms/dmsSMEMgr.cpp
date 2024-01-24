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
   _dmsSegmentSpace::_dmsSegmentSpace ( dmsExtentID startExtent,
                                        UINT16 initSize,
                                        UINT16 maxNode,
                                        _dmsSMEMgr *pSMEMgr )
   {
      _maxNode     = 0 ;
      _startExtent = startExtent ;
      _currentSize = initSize ;
      _totalSize   = maxNode ;
      _totalFree   = 0 ;
      _pSMEMgr     = pSMEMgr ;
      // no free space at beggining
   }

   _dmsSegmentSpace::~_dmsSegmentSpace ()
   {
      _freeSpaceList.clear() ;
   }

   // caller must hold exclusive latch
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMS__RSTMAX, "_dmsSegmentSpace::_resetMax" )
   void _dmsSegmentSpace::_resetMax ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSMS__RSTMAX ) ;
      list<_dmsSegmentNode>::iterator it ;
      _maxNode = 0 ;
      UINT32 size = 0 ;
      // loop through all free space
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

   // reserve numPages number of pages, if we don't have such contigious space
   // we'll return SDB_DMS_NOSPC, otherwise we return SDB_OK with foundPage
   // indicating the ID of starting page
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
         /// update free pos
         pFreePos->compareAndSwap( pos, pos + 1 ) ;
      }

      if ( numPages > _maxNode )
      {
         rc = SDB_DMS_NOSPC ;
         goto error ;
      }

      // loop through all free space
      for ( it = _freeSpaceList.begin(); it != _freeSpaceList.end(); ++it )
      {
         // get the node
         _dmsSegmentNode &node = (*it) ;
         oldsize = DMS_SEGMENT_NODE_GETSIZE( node ) ;
         if ( oldsize >= numPages )
         {
            foundPage = DMS_SEGMENT_NODE_GETSTART( node ) ;
            // if we request exactly the number of pages that left, we should
            // remove it from list
            if ( DMS_SEGMENT_NODE_GETSIZE( node ) == numPages )
            {
               _freeSpaceList.erase ( it ) ;
            }
            else
            {
               newsize = oldsize - numPages ;
               DMS_SEGMENT_NODE_SET ( node, foundPage+numPages, newsize ) ;
            }

            // add offset
            foundPage += _startExtent ;
            _totalFree -= numPages ;

            // if we were the max, we have to change the current max
            if ( oldsize == _maxNode )
            {
               _resetMax () ;
            }
            SDB_ASSERT ( _maxNode != _totalSize, "Internal logic error" ) ;
            rc = SDB_OK ;

            // Set SME bit to DMS_SME_ALLOCATED
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

      // should never hit here
      PD_LOG ( PDSEVERE, "Internal logic error" ) ;
      SDB_ASSERT ( FALSE, "Internal logic error" ) ;
      rc = SDB_SYS ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMS_RSVPAGES, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsSegmentSpace::releasePages ( dmsExtentID start, UINT16 numPages,
                                          BOOLEAN bitSet )
   {
      ossScopedLock lock( &_mutex ) ;
      return _releasePages( start, numPages, bitSet ) ;
   }

   // release numPages of pages, and merge the block with others
   // 1) sanity check to make sure we are in the right range
   // 2) if the free list is empty, simply insert and goto done
   // 3) otherwise loop through all nodes in the list
   // 4) if we hit end of the list, let's insert into the last position and
   //    goto 8
   // 5) otherwise if we find any nodes got bigger extentID, let's compare if we
   //    can merge with it
   // 6) if the start+numPages == extentID-for-next-block, that means it's
   //    contigious so that we can merge with a bigger block and goto 8
   // 7) otherwise it cannot be merged with next, goto 8
   // 8) after 4/5/6/7, we have to check whether able to merge the new node with
   //    previous node.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMS_RLSPAGES, "_dmsSegmentSpace::_releasePages" )
   INT32 _dmsSegmentSpace::_releasePages ( dmsExtentID start, UINT16 numPages,
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

      // make sure we are trying to release within range
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

      // build new node with starting position for numPages page
      DMS_SEGMENT_NODE_SET ( newnode, relaStart, numPages ) ;

      try
      {
         // if the list is empty, then we simply add it into list
         if ( _freeSpaceList.empty () )
         {
            SDB_ASSERT ( _maxNode == 0, "max node must be 0 when free space is "
                         "empty" ) ;
            // if the list is empty, we just need to insert
            _freeSpaceList.push_back ( newnode ) ;
            _maxNode = numPages ;
            goto done ;
         }

         // loop through all free space and see if we can merge with any
         for ( it = _freeSpaceList.begin(); ; ++it )
         {
            // if we hit the end, let's insert it to end and break the loop
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

            // get the node
            _dmsSegmentNode &node = (*it) ;
            UINT16      nodeStart = DMS_SEGMENT_NODE_GETSTART( node ) ;
            UINT16      nodeSize  = DMS_SEGMENT_NODE_GETSIZE( node ) ;
            // run until we find the next one with bigger start id
            if ( relaStart < nodeStart )
            {
               if ( relaStart + numPages > nodeStart )
               {
                  // something wrong
                  // | start        ... <numPages>   |
                  //                    | nodeStart    ...|
                  // It's not valid to have overlap
                  PD_LOG ( PDERROR, "Internal logic error" ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               // we should merge into this node
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
                  // too bad we can't merge, then let's insert one node in the
                  // middle
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
               // something wrong
               // | start   ... <numPages>  |
               // | nodeStart ....    |
               // OR
               //            | start ... <numPages> |
               // |nodeStart ....<nodeSize>|
               // It's not valid to have overlap
               PD_LOG ( PDERROR, "Internal logic error" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            oldit = it ;
            prevExist = TRUE ;
         } // for ( it = _freeSpaceList.begin();
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      // at last, let's try to merge the new node with previous one
      if ( prevExist )
      {
         // so we must have previous one when we break after scanning first
         // element
         _dmsSegmentNode &prevNode = (*oldit ) ;
         UINT16      prevStart = DMS_SEGMENT_NODE_GETSTART( prevNode ) ;
         UINT16      prevSize  = DMS_SEGMENT_NODE_GETSIZE( prevNode ) ;
         if ( prevStart + prevSize > relaStart )
         {
            // something wrong
            // | prevStart .... <prevSize> |
            //                    | start  ....|
            // It's not valid to have overlap
            PD_LOG ( PDERROR, "Internal logic error" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( prevStart + prevSize == relaStart )
         {
            // so we can merge
            DMS_SEGMENT_NODE_SETSIZE ( prevNode, newSize+prevSize ) ;
            _freeSpaceList.erase ( it1 ) ;
            if ( newSize+prevSize > _maxNode )
            {
               _maxNode = newSize+prevSize ;
            }
         }
         // otherwise we can't merge, so keep it as is
      }

   done :
      if ( SDB_OK == rc )
      {
         _totalFree += numPages ;

         if ( bitSet )
         {
            // set SME bit to DMS_SME_FREE
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMS_APPENDPAGES, "_dmsSegmentSpace::appendPages" )
   INT32 _dmsSegmentSpace::appendPages ( UINT16 numPages )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSMS_APPENDPAGES ) ;
      UINT32 newCurSize = 0 ;
      dmsExtentID start = 0 ;

      ossScopedLock lock( &_mutex ) ;

      if ( 0 == numPages )
      {
         goto done ;
      }

      // make sure append in the range
      newCurSize = _currentSize + numPages ;
      if ( newCurSize > _totalSize )
      {
         PD_LOG ( PDERROR, "Invalid append numPages[%d]. After append, "
                           "current size[%d] will be out of range[%d]",
                           numPages, newCurSize, _totalSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // extend new pages
      start = _startExtent + _currentSize ;
      rc = _releasePages( start, numPages, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release pages to the last segment at "
                  "start %d with numPages %d, rc = %d",
                  start, numPages, rc ) ;
         goto error ;
      }
      _currentSize = (UINT16)newCurSize ;
      // when _releasePage() succeed, _totalFree will be update

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMS_APPENDPAGES, rc );
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

      UINT32 segmentPages       = pStorageBase->segmentPages() ;
      UINT32 segmentPagesSqure  = pStorageBase->segmentPagesSquareRoot() ;
      UINT32 totalDataPages     = pStorageBase->pageNum() ;
      UINT32 initPages          = 0 ;
      UINT32 releaseBegin       = 0 ;
      BOOLEAN inUse             = FALSE ;
      dmsSegmentSpace *newspace = NULL ;
      UINT32 i                  = 0 ;

      for ( i = 0 ; i < totalDataPages ; ++i )
      {
         // the same with: 0 == i % segmentPages
         if ( 0 == ( i & ( ( 1 << segmentPagesSqure ) - 1 ) ) )
         {
            /// process the last space
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

            /// the new space
            initPages = ( ( totalDataPages - i ) >= segmentPages ) ?
                           segmentPages : ( totalDataPages - i ) ;
            newspace = SDB_OSS_NEW dmsSegmentSpace( i, initPages,
                                                    segmentPages, this ) ;
            if ( NULL == newspace )
            {
               PD_LOG ( PDERROR, "Unable to allocate memory" ) ;
               rc = SDB_OOM ;
               goto error ;
            }

            try
            {
               _segments.push_back( newspace ) ;
            }
            catch ( std::exception &e )
            {
               SDB_OSS_DEL newspace ;
               newspace = NULL ;
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
               goto error ;
            }
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

      // check whether we are still in free state at the end
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

   // attempt to reserve numPages pages from sme, if no more pages can be
   // found in existing pages, foundPage is set to DMS_INVALID_EXTENT
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

      // if there's no free space left, we still return SDB_OK but set foundPage
      // = DMS_INVALID_EXTENT
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
      // the same with: DMS_MAX_PG / segmentPages
      UINT32 maxSegments   = DMS_MAX_PG >> segmentPagesSqure ;
      // the same with: start / segmentPages
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

   INT32 _dmsSMEMgr::depositASegment ( dmsExtentID start )
   {
      return depositPages( start, (UINT16)_pStorageBase->segmentPages() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMEMGR_DEPOSITPAGES, "_dmsSMEMgr::depositPages" )
   INT32 _dmsSMEMgr::depositPages ( dmsExtentID start, UINT16 numPages )
   {
      PD_TRACE_ENTRY ( SDB__DMSSMEMGR_DEPOSITPAGES ) ;

      INT32 rc             = SDB_OK ;
      UINT16 segmentPages  = (UINT16)_pStorageBase->segmentPages() ;
      UINT32 segmentPagesSqure = _pStorageBase->segmentPagesSquareRoot() ;

      // the same with: DMS_MAX_PG / segmentPages
      UINT32 maxSegments   = DMS_MAX_PG >> segmentPagesSqure ;
      // the same with: start / segmentPages
      UINT32 segmentID     = start >> segmentPagesSqure ;
      dmsSegmentSpace *newspace = NULL ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      // the same with : 0 == start % segmentPages
      SDB_ASSERT ( 0 == ( start & ( ( 1 << segmentPagesSqure ) - 1 ) ),
                   "invalid start extent id" ) ;
      SDB_ASSERT ( segmentID < maxSegments, "extent is out of range" ) ;
      SDB_ASSERT ( ( numPages > 0 ) && ( numPages <= segmentPages ),
                   "invalid deposit size" ) ;

      if ( segmentID != (UINT32)_segments.size() )
      {
         PD_LOG ( PDERROR, "Extent id %d is not valid, segmentID[%u] "
                  "is not equal with segment num[%d]",
                  start, segmentID, _segments.size() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // memory is released in destructor
      newspace = SDB_OSS_NEW dmsSegmentSpace ( start, numPages,
                                               segmentPages, this ) ;
      if ( !newspace )
      {
         PD_LOG ( PDERROR, "Unable to allocate memory" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      try
      {
         _segments.push_back ( newspace ) ;
      }
      catch ( std::exception &e )
      {
         SDB_OSS_DEL newspace ;
         newspace = NULL ;
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = newspace->releasePages ( start, numPages, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page for segment %d at "
                  "start %d, rc = %d", segmentID, start, rc ) ;
         goto error ;
      }
      _totalFree.add( numPages ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMEMGR_DEPOSITPAGES, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSMEMGR_APPENDPAGES, "_dmsSMEMgr::appendPages" )
   INT32 _dmsSMEMgr::appendPages ( dmsExtentID start, UINT16 numPages )
   {
      PD_TRACE_ENTRY ( SDB__DMSSMEMGR_APPENDPAGES ) ;

      INT32 rc             = SDB_OK ;
      UINT16 segmentPages  = (UINT16)_pStorageBase->segmentPages() ;
      UINT32 segmentPagesSqure = _pStorageBase->segmentPagesSquareRoot() ;

      // the same with: DMS_MAX_PG / segmentPages
      UINT32 maxSegments   = DMS_MAX_PG >> segmentPagesSqure ;
      // the same with: start / segmentPages
      UINT32 segmentID     = start >> segmentPagesSqure ;
      dmsSegmentSpace *newspace = NULL ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      // the same with : 0 != start % segmentPages
      SDB_ASSERT ( 0 != ( start & ( ( 1 << segmentPagesSqure ) - 1 ) ),
                   "invalid start extent id" ) ;
      SDB_ASSERT ( segmentID < maxSegments, "extent is out of range" ) ;
      SDB_ASSERT ( ( numPages > 0 ) && ( numPages <= segmentPages ),
                   "invalid deposit size" ) ;

      // make sure we are in the last segment
      if ( _segments.size() == 0 )
      {
         PD_LOG ( PDERROR, "Invalid segment size" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( segmentID != ( (UINT32)_segments.size() - 1 ) )
      {
         PD_LOG ( PDERROR, "Extent id %d is not valid, segmentID[%u] "
                  "is not equal with segment num[%d]",
                  start, segmentID, (_segments.size() - 1) ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // get the last segment space and extend new pages in it
      newspace = _segments.back() ;
      rc = newspace->appendPages( numPages ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to append page for segment %d at "
                  "start %d with numPages %d, rc = %d",
                  segmentID, start, numPages, rc ) ;
         goto error ;
      }
      // try to adjust the available position of segment to a lower position
      _freePos.swapLesserThan( segmentID ) ;
      _totalFree.add( numPages ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSMEMGR_APPENDPAGES, rc );
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

