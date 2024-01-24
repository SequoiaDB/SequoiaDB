/******************************************************************************


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

   Source File Name = utilSegment.hpp

   Descriptive Name = Segment manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2018  JT  Initial Draft
          01/08/2019  CW  Optimize memory access

   Last Changed =

******************************************************************************/
#ifndef UTIL_SEGMENT_HPP__
#define UTIL_SEGMENT_HPP__

#include <vector>
#include "ossLatch.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "ossAtomic.hpp"
#include "utilInPlaceRBTree.hpp"

typedef UINT32 UTIL_OBJIDX ;
#define UTIL_INVALID_OBJ_INDEX   (( UTIL_OBJIDX )( -1 ))

// #define UTIL_SEGMENT_USE_SORT

namespace engine
{

   /*
      _utilSegmentHandler define
   */
   class _utilSegmentHandler
   {
      public:
         _utilSegmentHandler() {}
         virtual ~_utilSegmentHandler() {}

      public:
         virtual BOOLEAN   canAllocSegment( UINT64 size ) = 0 ;
         virtual BOOLEAN   canBallonUp( INT32 id, UINT64 size ) = 0 ;
         virtual BOOLEAN   canShrink( UINT32 objectSize,
                                      UINT32 totalObjNum,
                                      UINT32 usedObjNum ) = 0 ;
         virtual UINT64    getDBTick() const = 0 ;
         virtual UINT64    getTickSpanTime( UINT64 lastTick ) const = 0 ;
         virtual BOOLEAN   canShrinkDiscrete() const = 0 ;

         virtual void*     allocBlock( UINT64 size ) = 0 ;
         virtual void      releaseBlock( void *p, UINT64 size ) = 0 ;
   } ;
   typedef _utilSegmentHandler utilSegmentHandler ;

// Segment pool 0 :
//
//    segment 0: a segment is an array of object ( T )
//      +-------------+-------------+   +-------------+
//      | obj 0       | obj 1       |...| obj n - 1   |
//      +-------------+-------------+   +-------------+
//
//    segment 1:
//      +-------------+-------------+   +-------------+
//      | obj n+0     | obj n+1     |...| obj n+n-1   |
//      +-------------+-------------+   +-------------+
//
//      ...
//
//    segment m:
//      +-------------+-------------+   +-------------+
//      | obj m*n + 0 | obj m*n + 1 |...| obj m*n+n-1 |
//      +-------------+-------------+   +-------------+
//
//
// segments are logically organized by pools, each segment contains same number
// of objects. Each object has a unique id, object index, a 32bit unsigned
// integer. The high 4 bits of the object index reprsents the pool number,
// while the lower 28 bits is object number within that pool.
//

   #define UTIL_SEGMENT_NAME_LEN       ( 16 )

   // max number of segment pool, shall be power of 2
   #define _SEGMENT_MGR_MAX_POOLS      ( ( UINT32 ) 16 )
   // masks for bitwise operations on object index correlated with pool number
   #define _SEGMENT_OBJ_INDEX_MASK     ( ( UINT32 ) 0x0FFFFFFF )
   #define _SEGMENT_OBJ_POOL_MASK      ( ( UINT32 ) 0xF0000000 )
   #define _SEGMENT_OBJ_POOLID_SHIFT   ( 28 )
   #define _SEGMENT_OBJ_MAX_NUM        ( _SEGMENT_OBJ_INDEX_MASK + 1 )
   #define _SEGMENT_OBJ_MAX_SEG        ( 65536 )  /// UINT16 for segmentID

   #define _SEGMENT_OBJ_EYE_CATCHER    ( ( UINT8 ) 0xBE )
   #define _SEGMENT_OBJ_FLAG_ACQUIRED  ( ( UINT8 ) 0x55 )
   #define _SEGMENT_OBJ_FLAG_RELEASED  ( ( UINT8 ) 0xAA )

   #define IS_VALID_SEG_OBJ_INDEX( _objectIndex_ ) \
      ( UTIL_INVALID_OBJ_INDEX != _objectIndex_ )

   #define _GET_OBJX_ADDRESS( _p_ )      \
       ((CHAR*)( _p_ ) - sizeof(_utilSegObjSuper) )

   #define _GET_PACKED_POOLID( _pool_ )  \
             (( _pool_ << _SEGMENT_OBJ_POOLID_SHIFT ) & _SEGMENT_OBJ_POOL_MASK )

   #define _GET_UNPACKED_POOLID( _idx_ ) \
             (( _idx_ & _SEGMENT_OBJ_POOL_MASK ) >> _SEGMENT_OBJ_POOLID_SHIFT )

   /*
      shrink define
   */
   #define UTIL_SEGMENT_SEGKEEP_AUTO               ( 0xFFFFFFFF )
   #define UTIL_SEGMENT_FREE_TIMEOUT               ( 600 * OSS_ONE_SEC )
   #define UTIL_SEGMENT_PENDING_TIMEOUT            ( 120 * OSS_ONE_SEC )
   #define UTIL_SEGMENT_OBJ_IN_USE_RATIO_THRESHOLD ( 0.92 )
   #define UTIL_SEGMENT_SHRINK_STEP                ( 10 )

   /*
      _utilSegObjSuper define
   */
   struct _utilSegObjSuper : public SDBObject
   {
      UINT8       _eyeCatcher ;
      UINT8       _flag ;
      UINT16      _segmentID ;
      UTIL_OBJIDX _index ;

      _utilSegObjSuper()
      {
         init() ;
      }

      void init()
      {
         _eyeCatcher = _SEGMENT_OBJ_EYE_CATCHER ;
         _flag       = _SEGMENT_OBJ_FLAG_RELEASED ;
         _segmentID  = 0 ;
         _index      = UTIL_INVALID_OBJ_INDEX ;

         SDB_ASSERT( 8 == sizeof( _utilSegObjSuper ), "Invalid size" ) ;
      }
   } ;

   template < class T >
   class _utilSegmentPool : public SDBObject
   {
   public :
      enum _direction{ FORWARD, BACKWARD } ;

      struct _objX : public _utilSegObjSuper
      {
         T           _obj ;
      } ;

      struct _freeNodeX : public _utilSegObjSuper
      {
         _freeNodeX* _next ;
      } ;

      struct _blockX : public SDBObject
      {
         _objX          *_pBuff ;
         _freeNodeX     *_pFreeHeader ;
         UTIL_OBJIDX    _size ;
         UINT32         _blockSize ;
         UTIL_OBJIDX    _usedNum ;
         UINT32         _poolID ;
         UINT32         _segmentID ;
         UINT64         _lastAcquireTick ;
         _utilSegmentHandler *_pHandler ;

         /// free list
         _blockX        *_next ;
         _blockX        *_prev ;

         /// tree ele
         _blockX        *_parent ;
         INT32          _color ;

         _blockX* left() { return _next ; }
         void left( _blockX *l ) { _next = l ; }
         _blockX* right() { return _prev ; }
         void right( _blockX *r ) { _prev = r ; }
         _blockX* parent() { return _parent ; }
         void parent( _blockX *p ) { _parent = p ; }
         INT32 color() { return _color ; }
         void color( INT32 c ) { _color = c ; }
         bool operator< ( const _blockX &rhs ) const { return _segmentID < rhs._segmentID ; }

         _blockX( UINT32 segmentID, _utilSegmentHandler *pHandler )
         {
            _pBuff = NULL ;
            _pFreeHeader = NULL ;
            _size = 0 ;
            _blockSize = 0 ;
            _usedNum = 0 ;
            _poolID = 0 ;
            _segmentID = segmentID ;
            _lastAcquireTick = 0 ;
            _pHandler = pHandler ;

            _next = NULL ;
            _prev = NULL ;
            _parent = NULL ;
            _color = 0 ;
         }

         UINT32 getSegmentID() const { return _segmentID ; }
         UINT64 lastAcquireTick() const { return _lastAcquireTick ; }

         INT32 init( UINT32 poolID, UTIL_OBJIDX size, UINT32 blockSize )
         {
            _freeNodeX *pFreeNode = NULL ;
            UINT32 packedPoolID = _GET_PACKED_POOLID( poolID ) ;
            UTIL_OBJIDX baseObjID = _segmentID * size ;

            if ( _pBuff || size < 0 || size * sizeof( _objX ) > blockSize )
            {
               return SDB_SYS ;
            }

            _pBuff = (_objX*)_pHandler->allocBlock( blockSize ) ;
            if ( !_pBuff )
            {
               return SDB_OOM ;
            }

            _size = size ;
            _blockSize = blockSize ;
            _usedNum = 0 ;
            _poolID = poolID ;

            for ( UTIL_OBJIDX i = 0 ; i < _size ; ++i )
            {
               /// first init
               _pBuff[ i ].init() ;
               /// set segment id
               _pBuff[ i ]._segmentID = (UINT16)_segmentID ;
               /// set index
               _pBuff[ i ]._index = (((baseObjID + i) & _SEGMENT_OBJ_INDEX_MASK) | packedPoolID) ;
               /// set free node list
               pFreeNode = (_freeNodeX*)&( _pBuff[ i ] ) ;
               if ( i < _size - 1 )
               {
                  pFreeNode->_next = (_freeNodeX*)&( _pBuff[ i + 1 ] ) ;
               }
               else
               {
                  pFreeNode->_next = NULL ;
               }
            }

            /// set the free header
            _pFreeHeader = (_freeNodeX*)&( _pBuff[ 0 ] ) ;

            return SDB_OK ;
         }

         UTIL_OBJIDX release()
         {
            UTIL_OBJIDX size = _size ;

            if ( _pBuff )
            {
               _pHandler->releaseBlock( (void*)_pBuff, _blockSize ) ;
            }
            _pBuff = NULL ;
            _pFreeHeader = NULL ;
            _size = 0 ;
            _blockSize = 0 ;
            _usedNum = 0 ;
            _lastAcquireTick = 0 ;

            return size ;
         }

         UTIL_OBJIDX address2Index( const _objX *pObj )
         {
            if ( _pBuff && pObj >= _pBuff && pObj < _pBuff + _size )
            {
               return ( pObj - _pBuff ) + _segmentID * _size ;
            }
            return UTIL_INVALID_OBJ_INDEX ;
         }

         _objX* index2Address( UTIL_OBJIDX index )
         {
            if ( _pBuff && index >= _segmentID * _size &&
                 index < ( _segmentID + 1 ) * _size )
            {
               return &_pBuff[ index - _segmentID * _size ] ;
            }
            return NULL ;
         }

         INT32 alloc( _objX *& pObj )
         {
            INT32 rc = SDB_OOM ;
            if ( _pFreeHeader )
            {
               UINT32 packedPoolID = _GET_PACKED_POOLID( _poolID ) ;
               pObj = (_objX*)_pFreeHeader ;
               /// check
               if ( ( packedPoolID == ( pObj->_index & _SEGMENT_OBJ_POOL_MASK ) ) &&
                    ( _segmentID == pObj->_segmentID ) &&
                    ( _SEGMENT_OBJ_EYE_CATCHER == pObj->_eyeCatcher ) &&
                    ( _SEGMENT_OBJ_FLAG_RELEASED == pObj->_flag ) )
               {
                  _lastAcquireTick = _pHandler->getDBTick() ;
                  pObj->_flag = _SEGMENT_OBJ_FLAG_ACQUIRED ;
                  _pFreeHeader = _pFreeHeader->_next ;
                  ++_usedNum ;
                  rc = SDB_OK ;
               }
               else
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDSEVERE,
                          "Sanity check failed: " OSS_NEWLINE
                          "  PoolID        : %u" OSS_NEWLINE
                          "  Segment ID    : %u" OSS_NEWLINE
                          "  Obj EyeCatcher: %x" OSS_NEWLINE
                          "  Obj Flag      : %x" OSS_NEWLINE
                          "  Obj Index     : %x" OSS_NEWLINE
                          "  Obj Segment ID: %hu" OSS_NEWLINE
                          "  Obj addr      : %p" OSS_NEWLINE
                          "  Buffer addr   : %p" OSS_NEWLINE
                          "  ObjT addr     : %p" OSS_NEWLINE
                          "  ObjT Size     : %u" OSS_NEWLINE
                          "  ObjX Size     : %u" OSS_NEWLINE,
                          _poolID,
                          _segmentID,
                          pObj->_eyeCatcher,
                          pObj->_flag,
                          pObj->_index,
                          pObj->_segmentID,
                          pObj,
                          _pBuff,
                          &( pObj->_obj ),
                          sizeof( T ),
                          sizeof( _objX ) ) ;
                  pObj = NULL ;
#ifdef _DEBUG
                  SDB_ASSERT ( FALSE, "Sanity check failed !") ;
#endif
               }
            }
            return rc ;
         }

         void dealloc( _objX *pObj )
         {
            if ( !pObj )
            {
               return ;
            }
            pObj->_flag = _SEGMENT_OBJ_FLAG_RELEASED ;
            _freeNodeX *pFreeNode = (_freeNodeX*)pObj ;
            pFreeNode->_next = _pFreeHeader ;
            _pFreeHeader = pFreeNode ;
            --_usedNum ;
         }

         BOOLEAN     empty() const { return 0 == _usedNum ? TRUE : FALSE ; }
         BOOLEAN     full() const { return _usedNum == _size ? TRUE : FALSE ; }
         UINT32      freeSize() const { return _size - _usedNum ; }
         UINT32      usedSize() const { return _usedNum ; }
         UINT32      size() const { return _size ; }

         void resetFreeList()
         {
            if ( _size > 0 && 0 == _usedNum )
            {
               _freeNodeX *pFreeNode = NULL ;
               for ( UTIL_OBJIDX i = 0 ; i < _size ; ++i )
               {
                  /// set free node list
                  pFreeNode = (_freeNodeX*)&( _pBuff[ i ] ) ;
                  if ( i < _size - 1 )
                  {
                     pFreeNode->_next = (_freeNodeX*)&( _pBuff[ i + 1 ] ) ;
                  }
                  else
                  {
                     pFreeNode->_next = NULL ;
                  }
               }

               /// set the free header
               _pFreeHeader = (_freeNodeX*)&( _pBuff[ 0 ] ) ;
            }
         }
      } ;

   private :
      INT32          _id ;            // slot ID
      UINT32         _allocatedNum ;
      UINT32         _blockSize ;
      UINT32         _delta ;         // number of objects in a segment
      UINT32         _numOfObjs ;     // total number of objects in this pool
      UINT32         _maxNumOfObjs ;  // max number of objects in this pool
      UINT32         _maxHardNumOfObjs ; // max hard number of objects in this pool
      UINT32         _ballonNumOfObjs ;
      UINT32         _poolId  ;       // pool ID
      ossAtomic32    _maintaining ;   // shrinking in progres
      UINT32         _highWatermark ; // max object index in use
      BOOLEAN        _isInitialized ;
      ossSpinXLatch  _latch ;
      ossSpinXLatch  _ballonLatch ;
      std::vector< _blockX* > _segList;// a list of segments, each segment is
                                       // an array of object(T)
      UINT32         _usedSegNum ;     // only for used segment, not include pending...

      /// free _blockX info
      _blockX       *_header ;
      _blockX       *_tailer ;
      _blockX       *_current ;
      /// pending _blockX info
      _blockX       *_pendingHeader ;
      _blockX       *_pendingTailer ;
      /// compact _blockX info
      _utilInPlaceRBTree<_blockX>  _compactTree ;

      UINT32         _emptyNum ;      // empty segment number
      UINT32         _fullNum ;       // full segment number
      UINT32         _pendingNum ;    // pending segment number
      utilSegmentHandler   *_pHandler;// callback handler
      INT32          _maxDataSeqID ;

      /// stat info
      UINT64         _acquireTimes ;
      UINT64         _releaseTimes ;
      UINT64         _oomTimes ;
      UINT64         _oolTimes ;
      UINT64         _shrinkSize ;
      UINT64         _ballonTimes ;
      UINT64         _lastAcquireTick ;

   protected:
      void   clearStat_i()
      {
         _acquireTimes = 0 ;
         _releaseTimes = 0 ;
         _oomTimes = 0 ;
         _oolTimes = 0 ;
         _shrinkSize = 0 ;
         _ballonTimes = 0 ;

         _lastAcquireTick = 0 ;
      }

   public :
      _utilSegmentPool() : _id(0),
                           _allocatedNum(0),
                           _blockSize(0),
                           _delta(0),
                           _numOfObjs(0),
                           _maxNumOfObjs(0),
                           _maxHardNumOfObjs(_SEGMENT_OBJ_MAX_NUM),
                           _ballonNumOfObjs(0),
                           _poolId(0),
                           _maintaining(0),
                           _highWatermark(0),
                           _isInitialized( FALSE ),
                           _usedSegNum(0),
                           _header( NULL ),
                           _tailer( NULL ),
                           _current( NULL ),
                           _pendingHeader( NULL ),
                           _pendingTailer( NULL ),
                           _emptyNum( 0 ),
                           _fullNum( 0 ),
                           _pendingNum( 0 ),
                           _pHandler( NULL ),
                           _maxDataSeqID( -1 )
      {
         clearStat_i() ;
      }

      ~_utilSegmentPool()
      {
         if ( _isInitialized )
         {
            fini();
         }
      }

      UINT32 getAllocatedNum() const { return _allocatedNum ; }
      UINT32 getDelta() const { return _delta ; }
      UINT32 getBlockSize() const { return _blockSize ; }
      UINT32 getObjXSize() const { return sizeof( _objX ) ; }
      UINT32 getMaxNumOfObjs() const { return _maxNumOfObjs ; }
      UINT32 getBallonNumOfObjs() const { return _ballonNumOfObjs ; }
      UINT32 getRealMaxNumOfObjs() const { return _maxNumOfObjs + _ballonNumOfObjs ; }

      UINT32 getEmptySegNum() const { return _emptyNum ; }
      UINT32 getPendingSegNum() const { return _pendingNum ; }
      UINT32 getFullSegNum() const { return _fullNum ; }
      UINT32 getUsedSegNum() const { return _usedSegNum ; }
      UINT32 getAllSegNum() const { return _usedSegNum + _pendingNum ; }

      void  setMaxSize( UINT64 maxSize )
      {
         if ( !_isInitialized )
         {
            return ;
         }

         ossScopedLock __lock( &_latch ) ;

         /// roundup _maxNumOfObjs
         if ( ( maxSize + getObjXSize() - 1 ) / getObjXSize() > _maxHardNumOfObjs )
         {
            _maxNumOfObjs = _maxHardNumOfObjs ;
         }
         else
         {
            _maxNumOfObjs = ( maxSize + getObjXSize() - 1 ) / getObjXSize() ;
         }

         if ( _maxNumOfObjs > 0 )
         {
            UINT32 segmentNum = 0 ;
   
            if ( _maxNumOfObjs < _delta )
            {
               _maxNumOfObjs = _delta ;
               segmentNum = 1 ;
            }
            else
            {
               segmentNum = _maxNumOfObjs / _delta ;
               _maxNumOfObjs = segmentNum * _delta ;
            }
         }

         _ballonNumOfObjs = 0 ;
      }

      UINT32 dump( CHAR *pBuff, UINT32 buffLen,
                   UINT64 *pAcquireTimes = NULL,
                   UINT64 *pReleaseTimes = NULL,
                   UINT64 *pOOMTimes = NULL,
                   UINT64 *pOOLTimes = NULL,
                   UINT64 *pShrinkSize = NULL,
                   UINT64 *pBallonTimes = NULL )
      {
         UINT32 len = 0 ;

         /// don't use _latch, because will callback by kill -23 signal,
         /// and the thread maybe in acquire function with hold the _latch,
         /// so dump use _latch, will occur dead lock

         if ( pAcquireTimes )
         {
            *pAcquireTimes += _acquireTimes ;
         }
         if ( pReleaseTimes )
         {
            *pReleaseTimes += _releaseTimes ;
         }
         if ( pOOMTimes )
         {
            *pOOMTimes += _oomTimes ;
         }
         if ( pOOLTimes )
         {
            *pOOLTimes += _oolTimes ;
         }
         if ( pShrinkSize )
         {
            *pShrinkSize += _shrinkSize ;
         }
         if ( pBallonTimes )
         {
            *pBallonTimes += _ballonTimes ;
         }

         len = ossSnprintf( pBuff, buffLen,
                            OSS_NEWLINE
                            "          Pool ID : %u" OSS_NEWLINE
                            "      Max Objects : %u" OSS_NEWLINE
                            "   Ballon Objects : %u" OSS_NEWLINE
                            "            Delta : %u" OSS_NEWLINE
                            "       Block Size : %u" OSS_NEWLINE
                            "      Objects Num : %u" OSS_NEWLINE
                            "     Allocate Num : %u" OSS_NEWLINE
                            "      Used SegNum : %u" OSS_NEWLINE
                            "   Pending SegNum : %u" OSS_NEWLINE
                            "     Empty SegNum : %u" OSS_NEWLINE
                            "      Full SegNum : %u" OSS_NEWLINE
                            "   Compact SegNum : %u" OSS_NEWLINE
                            "Max In-Data SegID : %d" OSS_NEWLINE
                            "   High Watermark : %u" OSS_NEWLINE
                            "    Acquire Times : %llu" OSS_NEWLINE
                            "    Release Times : %llu" OSS_NEWLINE
                            "        OOM Times : %llu" OSS_NEWLINE
                            "        OOL Times : %llu" OSS_NEWLINE
                            "     Ballon Times : %llu" OSS_NEWLINE
                            "      Shrink Size : %llu" OSS_NEWLINE,
                            _poolId,
                            getRealMaxNumOfObjs(),
                            _ballonNumOfObjs,
                            _delta,
                            _blockSize,
                            _numOfObjs,
                            _allocatedNum,
                            _usedSegNum,
                            _pendingNum,
                            _emptyNum,
                            _fullNum,
                            _compactTree.count(),
                            _maxDataSeqID,
                            _highWatermark,
                            _acquireTimes,
                            _releaseTimes,
                            _oomTimes,
                            _oolTimes,
                            _ballonTimes,
                            _shrinkSize ) ;
         return len ;
      }

   private :
      _blockX* _popSegmentFrom( _blockX *&header, _blockX *&tailer,
                                UINT32 &num, _direction direction )
      {
         _blockX *pSegment = NULL ;
         if ( header )
         {
            SDB_ASSERT( tailer && num > 0, "tailer must be valid and num > 0" ) ;

            --num ;

            if ( header == tailer )
            {
               pSegment = header ;
               header = NULL ;
               tailer = NULL ;
               SDB_ASSERT( 0 == num, "number must be 0" ) ;
            }
            else if ( BACKWARD == direction )
            {
               pSegment = tailer ;
               tailer = tailer->_prev ;
               tailer->_next = NULL ;
               SDB_ASSERT( num > 0 , "number must > 0" ) ;
            }
            else
            {
               pSegment = header ;
               header = header->_next ;
               header->_prev = NULL ;
               SDB_ASSERT( num > 0 , "number must > 0" ) ;
            }

            pSegment->_next = NULL ;
            pSegment->_prev = NULL ;
         }
         return pSegment ;
      }

      _blockX* _popSegmentFromPending( _direction direction = FORWARD )
      {
         return _popSegmentFrom( _pendingHeader, _pendingTailer, _pendingNum, direction ) ;
      }

      _blockX* _popSegmentFromCompact()
      {
         _blockX *pSegment = _compactTree.begin() ;
         if ( pSegment )
         {
            _compactTree.remove( pSegment ) ;
         }
         return pSegment ;
      }

      bool _search( _blockX *pSegment, _blockX *header )
      {
         BOOLEAN found = FALSE ;
         _blockX *pSearch = header ;
         while( pSearch )
         {
            if ( pSearch == pSegment )
            {
               found = TRUE ;
               break ;
            }
            pSearch = pSearch->_next ;
         }
         return found ;
      }

      void _insertSegmentTo( _blockX *pSegment, _blockX *&header,
                             _blockX *&tailer, UINT32 *pNum,
                             _direction direction )
      {
         if ( pSegment )
         {
            BOOLEAN hasInsert = TRUE ;

            SDB_ASSERT( !pSegment->_next && !pSegment->_prev,
                        "Segment's next and prev isn't invalid" ) ;
/*
#ifdef _DEBUG
            SDB_ASSERT( !_search(), "Segment already exists" ) ;
#endif //_DEBUG
*/
            if ( !tailer )
            {
               SDB_ASSERT( !header && ( !pNum || 0 == *pNum ),
                           "Header must be NULL and num must be 0" ) ;
               pSegment->_next = NULL ;
               pSegment->_prev = NULL ;
               header = pSegment ;
               tailer = pSegment ;
            }
#ifdef UTIL_SEGMENT_USE_SORT
            else if ( pSegment->getSegmentID() > tailer->getSegmentID() )
            {
               /// insert to tailer
               tailer->_next = pSegment ;
               pSegment->_prev = tailer ;
               pSegment->_next = NULL ;
               tailer = pSegment ;
            }
            else if ( pSegment->getSegmentID() < header->getSegmentID() )
            {
               /// insert to header
               header->_prev = pSegment ;
               pSegment->_next = header ;
               pSegment->_prev = NULL ;
               header = pSegment ;
            }
            else if ( BACKWARD == direction )
            {
               /// find the position to insert
               _blockX *pFind = tailer ;
               while( pFind && pSegment->getSegmentID() < pFind->getSegmentID() )
               {
                  pFind = pFind->_prev ;
               }
               SDB_ASSERT( pFind, "pFind can't be NULL" ) ;
               if ( pFind && pFind != pSegment )
               {
                  pSegment->_next = pFind->_next ;
                  pSegment->_next->_prev = pSegment ;
                  pSegment->_prev = pFind ;
                  pFind->_next = pSegment ;
               }
               else
               {
                  hasInsert = FALSE ;
                  SDB_ASSERT( FALSE, "Segment already exists" ) ;
               }
            }
            else
            {
               /// find the position to insert
               _blockX *pFind = header ;
               while( pFind && pSegment->getSegmentID() > pFind->getSegmentID() )
               {
                  pFind = pFind->_next ;
               }
               SDB_ASSERT( pFind, "pFind can't be NULL" ) ;
               if ( pFind && pFind != pSegment )
               {
                  pSegment->_prev = pFind->_prev ;
                  pSegment->_prev->_next = pSegment ;
                  pSegment->_next = pFind ;
                  pFind->_prev = pSegment ;
               }
               else
               {
                  hasInsert = FALSE ;
                  SDB_ASSERT( FALSE, "Segment already exists" ) ;
               }
            }
#else // no defined UTIL_SEGMENT_USE_SORT
            else if ( BACKWARD == direction )
            {
               /// insert to tailer
               tailer->_next = pSegment ;
               pSegment->_prev = tailer ;
               pSegment->_next = NULL ;
               tailer = pSegment ;
            }
            else
            {
               /// insert to header
               header->_prev = pSegment ;
               pSegment->_next = header ;
               pSegment->_prev = NULL ;
               header = pSegment ;
            }
#endif // UTIL_SEGMENT_USE_SORT

            if ( pNum && hasInsert )
            {
               ++(*pNum) ;
            }
         }
      }

      void _insertSegmentToCompact( _blockX *pSegment )
      {
         _compactTree.insert( pSegment ) ;
      }

      void _insertSegmentToPending( _blockX *pSegment, _direction direction = BACKWARD )
      {
         _insertSegmentTo( pSegment, _pendingHeader, _pendingTailer, &_pendingNum, direction ) ;
      }

      void _insertSegmentToFree( _blockX *pSegment, BOOLEAN addObjNum,
                                 _direction direction = BACKWARD )
      {
         _insertSegmentTo( pSegment, _header, _tailer, &_emptyNum, direction ) ;
         if ( addObjNum )
         {
            _numOfObjs += pSegment->size() ;
            ++_usedSegNum ;
         }
      }

      _blockX* _removeSegmentFrom( _blockX *pSegment, _blockX *&header,
                                   _blockX *&tailer, UINT32 *pNum,
                                   _direction direction )
      {
         _blockX *pNext = NULL ;

         SDB_ASSERT( pSegment && header && tailer && ( !pNum || *pNum > 0 ),
                     "header and tailer must be valid and num > 0" ) ;

         pNext = ( FORWARD == direction ) ? pSegment->_next : pSegment->_prev ;

         if ( pSegment == header )
         {
            if ( header == tailer )
            {
               if ( pNum )
               {
                  SDB_ASSERT( 1 == *pNum, "number must be 1" ) ;
               }
               SDB_ASSERT( NULL == header->_next && NULL == header->_prev,
                           "Next and prev must be NULL" ) ;
               header = NULL ;
               tailer = NULL ;
            }
            else
            {
               header = header->_next ;
               header->_prev = NULL ;
            }
         }
         else if ( pSegment == tailer )
         {
            tailer = tailer->_prev ;
            tailer->_next = NULL ;
         }
         else
         {
/*
#ifdef _DEBUG
            SDB_ASSERT( _search( pSegment, header ), "Not found" ) ;
#endif // _DEBUG
*/
            SDB_ASSERT( pSegment->_prev && pSegment->_next,
                        "Segment's next and prev must be valid" ) ;
            pSegment->_prev->_next = pSegment->_next ;
            pSegment->_next->_prev = pSegment->_prev ;
         }

         pSegment->_next = NULL ;
         pSegment->_prev = NULL ;

         if ( pNum )
         {
            --(*pNum) ;
         }

         return pNext ;
      }

      _blockX* _removeSegmentFromFree( _blockX *pSegment, BOOLEAN decObjNum, _direction direction )
      {
         _blockX *ret = _removeSegmentFrom( pSegment, _header, _tailer, &_emptyNum, direction ) ;
         if ( decObjNum )
         {
            _numOfObjs -= pSegment->size() ;
            --_usedSegNum ;
         }

         /// reset current
         if ( _current == pSegment )
         {
            _current = NULL ;
         }
         return ret ;
      }

      _blockX* _removeSegmentFromPending( _blockX *pSegment, _direction direction )
      {
         return _removeSegmentFrom( pSegment, _pendingHeader, _pendingTailer,
                                    &_pendingNum, direction ) ;
      }

      //
      // Description: add a new segment and expand the _list
      //              when no free objects
      // Input      : none
      // Return     : SDB_OK               -- normal
      //              SDB_OSS_UP_TO_LIMIT  -- exceeds resource max threshold
      //              SDB_OOM              -- out of memory
      // Dependency : this function is called by acquire() only; and the caller
      //              shall make sure the operation is protected by latch
      INT32 _expandList()
      {
         INT32     rc           = SDB_OK ;

#ifdef _DEBUG
         SDB_ASSERT( _isInitialized,
                     "Expand can only be done when segment is initialized" ) ;
#endif
         if ( _delta > 0 && _numOfObjs + _delta <= getRealMaxNumOfObjs() )
         {
            /// first get from pending
            _blockX *pSegment = _popSegmentFromPending() ;
            if ( pSegment )
            {
               SDB_ASSERT( _delta == pSegment->size(), "Size must be equal with _delta" ) ;
               SDB_ASSERT( pSegment->empty(), "Must be empty" ) ;
               pSegment->resetFreeList() ;
               _insertSegmentToFree( pSegment, TRUE ) ;
            }
            else
            {
               if ( !_pHandler->canAllocSegment( getBlockSize() ) )
               {
                  rc = SDB_OSS_UP_TO_LIMIT ;
                  PD_LOG( PDINFO, "Can't alloc segment[%u] by handler", _blockSize ) ;
                  goto error ;
               }

               pSegment = _popSegmentFromCompact() ;
               if ( !pSegment )
               {
                  pSegment = SDB_OSS_NEW _blockX( _segList.size(), _pHandler ) ;
                  if ( !pSegment )
                  {
                     rc = SDB_OOM ;
                     PD_LOG( PDERROR, "Allocate segment(ObjT Size:%u, SegmentID:%u) element "
                             "failed, rc: %d", sizeof( T ), _segList.size(), rc ) ;
                     goto error ;
                  }
                  /// push to segList
                  try
                  {
                     _segList.push_back( pSegment ) ;
                  }
                  catch( std::exception &e )
                  {
                     rc = ossException2RC( &e ) ;
                     SDB_OSS_DEL pSegment ;
                     pSegment = NULL ;
                     PD_LOG( PDERROR, "Push segment(ObjT Size:%u, SegmentID:%u) element to "
                             "vector failed, rc: %d", sizeof( T ), _segList.size(), rc ) ;
                     goto error ;
                  }
               }

               /// alloc segment memory
               rc = pSegment->init( _poolId, _delta, getBlockSize() ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Init segment(ObjT Size:%u, SegmentID:%u) failed, rc: %d",
                          sizeof( T ), pSegment->getSegmentID(), rc ) ;
                  pSegment->release() ;
                  /// save segment to compact list
                  _insertSegmentToCompact( pSegment ) ;

                  goto error ;
               }
               _insertSegmentToFree( pSegment, TRUE ) ;
            }
         }
         else
         {
            // exceed the lock resorce limitation
            rc = SDB_OSS_UP_TO_LIMIT ;
            goto error ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE void _shrinkPending( UINT32 *pFreeSegNum, UINT32 expectSegNum )
      {
         UINT32 shrinkSegmentNum = 0 ;
         _blockX *pSegment = NULL ;
         UINT32 stepCount = 0 ;
         UINT32 threshold = _pendingNum >> 1 ;

         if ( threshold < UTIL_SEGMENT_SHRINK_STEP )
         {
            threshold = UTIL_SEGMENT_SHRINK_STEP ;
         }

         /// refix it by expectSegNum
         if ( threshold < expectSegNum )
         {
            threshold = expectSegNum ;
         }

         pSegment = _pendingTailer ;
         while( pSegment )
         {
            if ( 0 == expectSegNum && _pHandler &&
                  _pHandler->getTickSpanTime( pSegment->lastAcquireTick() ) <
                 UTIL_SEGMENT_PENDING_TIMEOUT )
            {
               /// not timeout
               break ;
            }
            _blockX *pFind = pSegment ;
            /// remove from list
            pSegment = _removeSegmentFromPending( pSegment, BACKWARD ) ;
            /// release space
            pFind->release() ;
            /// save to compact segment
            _insertSegmentToCompact( pFind ) ;
            ++shrinkSegmentNum ;

            if ( ++stepCount > threshold )
            {
               break ;
            }
         }

         if ( _ballonNumOfObjs > ( shrinkSegmentNum * _delta ) )
         {
            _ballonNumOfObjs -= ( shrinkSegmentNum * _delta ) ;
         }
         else
         {
            _ballonNumOfObjs = 0 ;
         }

         if ( pFreeSegNum )
         {
            *pFreeSegNum += shrinkSegmentNum ;
         }

         threshold = _compactTree.count() >> 1 ;
         if ( threshold < UTIL_SEGMENT_SHRINK_STEP )
         {
            threshold = UTIL_SEGMENT_SHRINK_STEP ;
         }
         stepCount = 0 ;
         /// remove compact segments
         pSegment = _compactTree.begin( _utilInPlaceRBTree<_blockX>::BACKWARD ) ;
         while( pSegment )
         {
            if ( pSegment->getSegmentID() != _segList.size() - 1 )
            {
               /// not the last, break
               break ;
            }

            _blockX *pFind = pSegment ;
            /// remove from tree
            pSegment = _compactTree.remove( pSegment, false,
                                            _utilInPlaceRBTree<_blockX>::BACKWARD ) ;
            /// delete the segment element
            SDB_ASSERT( pFind == _segList.back(), "Invalid segment elements" ) ;
            _segList.pop_back() ;
            SDB_OSS_DEL pFind ;
            pFind = NULL ;

            if ( ++stepCount > threshold )
            {
               break ;
            }
         }
      }

      //
      // Description: check if need to expand before acquire an free object
      // Input      : none
      // Return     : TRUE   -- need to expand
      //              FALSE  -- no need to expand, acquire operation can proceed
      // Dependency : this function is called by acquire() only; the caller
      //              shall acquire the latch first.
      //
      OSS_INLINE BOOLEAN _needExpand() const
      {
         return _allocatedNum >= _numOfObjs ? TRUE : FALSE ;
      }

      //
      // Description: check if current _numOfObjs is beyond the max threshold
      //              _maxNumOfObjs
      // Input      : none
      // Return     : TRUE
      //              FALSE
      // Dependency : this function is called by acquire() only; the caller
      //              shall acquire the latch first.
      //
      OSS_INLINE BOOLEAN _isUpToLimit() const
      {
         return _numOfObjs >= getRealMaxNumOfObjs() ? TRUE : FALSE ;
      }

      //
      // Description: get the address of internal object _objX by its index
      // Input      :
      //              idx     -- object index
      // Output     :
      //              _objX * -- the address of the object
      //
      // Dependency : this function is internal/private helper function
      //              called by getObjPtrByIndex(), acquire(), release()
      //
      OSS_INLINE _objX * _getObjXByIndex( const UTIL_OBJIDX idx,
                                          _blockX **ppSegBlock )
      {
         _objX * pObj      = NULL ;
         UTIL_OBJIDX index = ( idx & _SEGMENT_OBJ_INDEX_MASK ) ;

#ifdef _DEBUG
         if ( IS_VALID_SEG_OBJ_INDEX( idx ) )
#endif
         {
            // i = idx / _delta ;
            // j = idx % _delta ;
            // the _delta is round up to nearest power of 2,
            // so divide and modulo can be optimized
            UINT32 segmentID = index / _delta ;
            if ( segmentID < _segList.size() )
            {
               if ( ppSegBlock )
               {
                  *ppSegBlock = _segList[ segmentID ] ;
               }
               pObj = _segList[ segmentID ]->index2Address( index ) ;
            }
         }
         return pObj ;
      }

      OSS_INLINE _objX* _allocObjX( _blockX **ppSegBlock )
      {
         _objX *pObj = NULL ;

         if ( !_current )
         {
            _current = _header ;
         }

         if ( _current )
         {
            if ( SDB_OK == _current->alloc( pObj ) )
            {
               _allocatedNum ++ ;

               if ( ppSegBlock )
               {
                  *ppSegBlock = _current ;
               }

               if ( 1 == _current->usedSize() )
               {
                  --_emptyNum ;

                  if ( _maxDataSeqID < 0 ||
                       _maxDataSeqID < (INT32)_current->getSegmentID() )
                  {
                     _maxDataSeqID = (INT32)_current->getSegmentID() ;
                  }
               }
               else if ( _current->full() )
               {
                  ++_fullNum ;
                  /// when current is full, need remove from list
                  _removeSegmentFrom( _current, _header, _tailer, NULL, BACKWARD ) ;
                  _current = NULL ;
               }
            }
         }

         return pObj ;
      }

   public :
      //
      // Description: initialization
      //               . allocate the first array of objects, segment,
      //                 and save the address of this segment in _segList
      //               . fill the object index array, _list,
      //                 with index of the newly allocated object
      // Input      :
      //              poolId          -- pool id
      //              numberOfObjs    -- number of objects in a segment
      //              maxNumberOfObjs -- max number of objects, the max thresold
      //
      // Return     : SDB_OK          -- initialized successfully
      //              SDB_INVALIDARG  -- invalid arguments
      //              SDB_OOM         -- out of memory
      // Dependency : this function shall be called one time only, and before
      //              any other function of this class.
      //
      INT32 init( INT32 id,
                  UINT32 poolId,
                  UINT32 blockSize,
                  UINT32 maxSize,
                  utilSegmentHandler *pHandler )
      {
         INT32   rc           = SDB_OK ;

         _id            = id ;
         _poolId        = poolId ;
         _blockSize     = blockSize ;

         if ( _blockSize < getObjXSize() ||
              _blockSize / getObjXSize() > _maxHardNumOfObjs )
         {
            /// invalid blockSize
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid blockSize(%u)", blockSize ) ;
            goto error ;
         }

         _delta        = _blockSize / getObjXSize() ;

         if ( _maxHardNumOfObjs / _delta > _SEGMENT_OBJ_MAX_SEG )
         {
            _maxHardNumOfObjs = _SEGMENT_OBJ_MAX_SEG * _delta ;
         }

         /// roundup _maxNumOfObjs
         if ( ( maxSize + getObjXSize() - 1 ) / getObjXSize() > _maxHardNumOfObjs )
         {
            _maxNumOfObjs = _maxHardNumOfObjs ;
         }
         else
         {
            _maxNumOfObjs = ( maxSize + getObjXSize() - 1 ) / getObjXSize() ;
         }

         if ( _maxNumOfObjs > 0 )
         {
            UINT32 segmentNum = 0 ;

            if ( _maxNumOfObjs < _delta )
            {
               _maxNumOfObjs = _delta ;
               segmentNum = 1 ;
            }
            else
            {
               segmentNum = _maxNumOfObjs / _delta ;
               _maxNumOfObjs = segmentNum * _delta ;
            }
         }

         _highWatermark = ( _delta - 1 ) ;
         _pHandler = pHandler ;
         _isInitialized = TRUE ;

      done:
         return rc ;
      error:
         goto done ;
      }

      //
      // Description: Free all objects allocated in segments and the object
      //              index array, _list
      // Return     : none
      // Dependency : this function shall be called just before destroy,
      //              the caller shall guarantee no other threads are accessing
      //              the objects.
      //
      void fini()
      {
         if ( _isInitialized )
         {
            _blockX *pSegment = NULL ;
            UINT32 len = _segList.size() ;

            for ( UINT32 i = 0; i < len ; i++ )
            {
               pSegment = _segList[ i ] ;

               if ( pSegment->empty() )
               {
                  pSegment->release() ;
                  SDB_OSS_DEL pSegment ;
               }
            }

            _segList.clear() ;
            _usedSegNum = 0 ;
            _header = NULL ;
            _tailer = NULL ;
            _current = NULL ;
            _pendingHeader = NULL ;
            _pendingTailer = NULL ;
            _emptyNum = 0 ;
            _fullNum = 0 ;
            _pendingNum = 0 ;
            _allocatedNum = 0 ;
            _numOfObjs = 0 ;
            _isInitialized = FALSE ;
            _pHandler = NULL ;
            _maxDataSeqID = -1 ;
         }
      }

      //
      // Description: get total number of objects allocated in a segment pool
      // Return     : total number of objects allocated in a segment pool
      //
      OSS_INLINE UINT32 getNumOfObjs() const { return _numOfObjs ; }

      OSS_INLINE void   getSizeInfo( UINT64 &total, UINT64 &used ) const
      {
         total = ( _usedSegNum + _pendingNum ) * getBlockSize() ;
         used  = _allocatedNum * getObjXSize() ;
      }

      //
      // Description: get an object's address by its index
      // Input      :
      //              idx -- the object index
      // Return     : address of the object ( specified by the index )
      // Dependency : this function shall be called after the class is
      //              initialized. It expects an correct index, i.e.,
      //              idx < _numOfObjs
      T * getObjPtrByIndex( const UTIL_OBJIDX idx )
      {
         T * pObj     = NULL ;

         ossScopedLock lock( &_latch, SHARED ) ;
         _objX *pObjX = _getObjXByIndex( idx, NULL ) ;
         if ( pObjX )
         {
            pObj = ( T * )&( pObjX->_obj ) ;
         }
         return pObj ;
      }

      //
      // Description: get an object's index by its address
      // Return     : the object's index
      // Input      :
      //              idx -- the object index
      // Return     : index of the object ( specified by the address )
      // Dependency : this function shall be called after the class is
      //              initialized. It may return UTIL_INVALID_OBJ_INDEX,
      //              if invalid address is passed in.
      //
      UTIL_OBJIDX getIndexByAddr ( const void * pT )
      {
         UTIL_OBJIDX idx  = UTIL_INVALID_OBJ_INDEX ;

         if ( NULL != pT )
         {
            _objX * pObjX = ( _objX * )_GET_OBJX_ADDRESS( pT ) ;
            if ( pObjX &&
                 ( _SEGMENT_OBJ_EYE_CATCHER == pObjX->_eyeCatcher ) &&
                 ( ( pObjX->_index & _SEGMENT_OBJ_POOL_MASK ) ==
                    _GET_PACKED_POOLID( _poolId ) ) )
            {
               idx = pObjX->_index ;
            }

#ifdef _DEBUG
            {
               UINT32  packedPoolID = _GET_PACKED_POOLID( _poolId ) ;
               UINT32 segmentID = pObjX->_segmentID ;
               _blockX *pSegment = NULL ;
               UTIL_OBJIDX index = UTIL_INVALID_OBJ_INDEX ;

               // WARNING: getIndexByAddr should not hold _latch
               // acquire lock for _segList access
               ossScopedLock lock( &_latch, SHARED ) ;

               if ( segmentID < _segList.size() )
               {
                  pSegment = _segList[ segmentID ] ;
                  index = pSegment->address2Index( pObjX ) ;
                  index = ( ( index & _SEGMENT_OBJ_INDEX_MASK ) | packedPoolID ) ;

                  SDB_ASSERT( index == idx, "Verification failed, invalid address" ) ;
               }
               else
               {
                  SDB_ASSERT( FALSE, "Invalid segment ID" ) ;
               }
            }
#endif
         }
         return idx ;
      }

      //
      // Description: acquire/apply a free object from the segments
      // Input      : none
      // Output     :
      //              pT  -- the pointer/address of the object
      //              pIndex -- the object index, each object has an unique index,
      //                        the index number is continuous, starting from 0
      // Return     : SDB_OK,
      //              SDB_OSS_UP_TO_LIMIT,
      //              SDB_OOM,
      //              SDB_SYS,
      //              error rc returned from _expandList()
      // Dependency : this function shall be called after the class is
      //              initialized.
      //              when _needExpand(),
      //              means lack of free objects and a new segment ( array of
      //              object T ) will be expanded.
      //              acquire() protects all underneath operations by latch.
      //
      INT32 acquire( void *&pT, UTIL_OBJIDX *pIndex = NULL )
      {
         INT32   rc                = SDB_OK ;
         BOOLEAN bLatched          = FALSE ;
         _objX * pObjX             = NULL ;
         _blockX *pSegBlock        = NULL ;
         BOOLEAN hasBallonUp       = FALSE ;

         _latch.get() ;
         bLatched = TRUE ;

         if ( _isInitialized )
         {
            while ( _needExpand() ) // need to expand
            {
               if ( _isUpToLimit() )
               {
                  BOOLEAN canBallonUp = FALSE ;
                  UINT32 tmpRealMaxNumOfObjs = getRealMaxNumOfObjs() ;

                  if ( tmpRealMaxNumOfObjs <= ( _maxHardNumOfObjs - _delta ) )
                  {
                     if ( _pHandler )
                     {
                        /// release latch and lock ballonLatch
                        _latch.release() ;
                        bLatched = FALSE ;

                        _ballonLatch.get() ;
                        /// double check
                        if ( tmpRealMaxNumOfObjs != getRealMaxNumOfObjs() )
                        {
                           /// has changed by other session
                           _ballonLatch.release() ;

                           _latch.get() ;
                           bLatched = TRUE ;
                           continue ;
                        }

                        canBallonUp = _pHandler->canBallonUp( _id, getBlockSize() ) ;
                        _latch.get() ;
                        bLatched = TRUE ;

                        if ( canBallonUp &&
                             getRealMaxNumOfObjs() <= ( _maxHardNumOfObjs - _delta ) )
                        {
                           _ballonNumOfObjs += _delta ;
                           hasBallonUp = TRUE ;
                           ++_ballonTimes ;
                        }

                        /// release ballonLatch
                        _ballonLatch.release() ;
                     }
                  }

                  if ( !hasBallonUp )
                  {
                     // exceed limitation
                     ++_oolTimes ;
                     rc = SDB_OSS_UP_TO_LIMIT ;
                     PD_LOG( PDINFO,
                             "Exceed resource limitation "
                             "when attempt to expand: %d" OSS_NEWLINE
                             "  PoolID        : %u" OSS_NEWLINE
                             "  Delta         : %u" OSS_NEWLINE
                             "  MaxNumOfObjs  : %u" OSS_NEWLINE
                             "BallonNumOfObjs : %u" OSS_NEWLINE
                             "  NumOfObjs     : %u" OSS_NEWLINE
                             "  Allocated Num : %u" OSS_NEWLINE
                             " Pending SegNum : %u" OSS_NEWLINE
                             " Compact SegNum : %u" OSS_NEWLINE
                             "  ObjT Size     : %u" OSS_NEWLINE
                             "  ObjX Size     : %u" OSS_NEWLINE,
                             rc,
                             _poolId,
                             _delta,
                             getRealMaxNumOfObjs(),
                             _ballonNumOfObjs,
                             _numOfObjs,
                             _allocatedNum,
                             _pendingNum,
                             _compactTree.count(),
                             sizeof( T ),
                             sizeof( _objX ) ) ;
                     goto error ;
                  }
               }

               rc = _expandList() ;
               if ( rc )
               {
                  if ( hasBallonUp )
                  {
                     _ballonNumOfObjs -= _delta ;
                     hasBallonUp = FALSE ;
                     --_ballonTimes ;
                  }
                  if ( SDB_OSS_UP_TO_LIMIT == rc )
                  {
                     ++_oolTimes ;
                  }
                  else if ( SDB_OOM == rc )
                  {
                     ++_oomTimes ;
                  }

                  PD_LOG( ( SDB_OSS_UP_TO_LIMIT == rc ? PDINFO : PDWARNING ),
                          "Failed to expand : %d" OSS_NEWLINE
                          "  PoolID         : %u" OSS_NEWLINE
                          "  Delta          : %u" OSS_NEWLINE
                          "  MaxNumOfObjs   : %u" OSS_NEWLINE
                          "BallonNumOfObjs  : %u" OSS_NEWLINE
                          "  NumOfObjs      : %u" OSS_NEWLINE
                          "  Allocated Num  : %u" OSS_NEWLINE
                          " Pending SegNum  : %u" OSS_NEWLINE
                          " Compact SegNum  : %u" OSS_NEWLINE
                          "  ObjT Size      : %u" OSS_NEWLINE
                          "  ObjX Size      : %u" OSS_NEWLINE,
                          rc,
                          _poolId,
                          _delta,
                          getRealMaxNumOfObjs(),
                          _ballonNumOfObjs,
                          _numOfObjs,
                          _allocatedNum,
                          _pendingNum,
                          _compactTree.count(),
                          sizeof( T ),
                          sizeof( _objX ) ) ;
                  goto error ;
               }

               break ;
            }

            pObjX = _allocObjX( &pSegBlock ) ;
            if ( pObjX )
            {
               if ( pIndex )
               {
                  *pIndex = pObjX->_index ;
               }

               ++_acquireTimes ;
               _lastAcquireTick = pSegBlock->lastAcquireTick() ;

               // return the object address
               pT = ( void * )&( pObjX->_obj ) ;
            }
            else
            {
               if ( pIndex )
               {
                  *pIndex = UTIL_INVALID_OBJ_INDEX ;
               }
               pT  = NULL ;
               rc  = SDB_SYS ;

               PD_LOG( PDSEVERE,
                       "Sanity check failed: " OSS_NEWLINE
                       "  PoolID        : %u" OSS_NEWLINE
                       "  Delta         : %u" OSS_NEWLINE
                       "  Allocated Num : %u" OSS_NEWLINE
                       "  Objects Num   : %u" OSS_NEWLINE
                       "  Max Objects   : %u" OSS_NEWLINE
                       " Ballon Objects : %u" OSS_NEWLINE
                       "  Empty Num     : %u" OSS_NEWLINE
                       "   Full Num     : %u" OSS_NEWLINE
                       "  Current addr  : %p" OSS_NEWLINE
                       "  Header addr   : %p" OSS_NEWLINE
                       "  ObjT Size     : %u" OSS_NEWLINE
                       "  ObjX Size     : %u" OSS_NEWLINE,
                       _poolId,
                       _delta,
                       _allocatedNum,
                       _numOfObjs,
                       getRealMaxNumOfObjs(),
                       _ballonNumOfObjs,
                       _emptyNum,
                       _fullNum,
                       _current,
                       _header,
                       sizeof( T ),
                       sizeof( _objX ) ) ;
#ifdef _DEBUG
               SDB_ASSERT ( FALSE, "Sanity check failed !") ;
#endif
               goto error ;
            }
         }
         else
         {
            SDB_ASSERT( _isInitialized, "_utilSegmentPool has to be initialized." ) ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         if ( bLatched )
         {
            _latch.release() ;
         }
         return rc ;
      error:
         goto done ;
      }

      //
      // Description: release/return an object to the segments by its index
      // Input      : pT -- address/pointer of the object
      // Output     : none
      // Return     : SDB_OK
      //              SDB_SYS -- when error occurs
      // Dependency : this function shall be called after the class is
      //              initialized.
      //              release() protects all underneath operations by latch.
      //
      INT32 release( const void * pT )
      {
         INT32   rc           = SDB_OK ;
         UINT32 segmentID     = 0 ;
         BOOLEAN bLatched     = FALSE ;
         _blockX *pSegment    = NULL ;
         _objX * pObjX        = NULL ;
         BOOLEAN addToFree    = FALSE ;
         const UTIL_OBJIDX packedPoolID = _GET_PACKED_POOLID( _poolId ) ;

         if ( !pT )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         pObjX = ( _objX * )_GET_OBJX_ADDRESS( pT ) ;

         _latch.get() ;
         bLatched = TRUE ;

         if ( _isInitialized && pObjX )
         {
            if ( ( _SEGMENT_OBJ_EYE_CATCHER == pObjX->_eyeCatcher ) &&
                 ( ( pObjX->_index & _SEGMENT_OBJ_POOL_MASK ) == packedPoolID ) &&
                 ( _SEGMENT_OBJ_FLAG_ACQUIRED == pObjX->_flag ) &&
                 ( ( segmentID = pObjX->_segmentID ) < _segList.size() ) )
            {
               pSegment = _segList[ segmentID ] ;
#ifdef _DEBUG
               UTIL_OBJIDX idx = pObjX->_index ;
               UTIL_OBJIDX index = pSegment->address2Index( pObjX ) ;
               index = ( ( index & _SEGMENT_OBJ_INDEX_MASK ) | packedPoolID ) ;
               SDB_ASSERT( index == idx, "Verification failed, invalid address" ) ;
#endif // _DEBUG

               if ( pSegment->full() )
               {
                  addToFree = TRUE ;
               }
               pSegment->dealloc( pObjX ) ;
               --_allocatedNum ;
               if ( addToFree )
               {
                  --_fullNum ;
                  _insertSegmentTo( pSegment, _header, _tailer, NULL, FORWARD ) ;
               }
               else if ( pSegment->empty() )
               {
#ifdef UTIL_SEGMENT_USE_SORT
                  ++_emptyNum ;
#else // not define UTIL_SEGMENT_USE_SORT
                  /// remove and insert into tail
                  _removeSegmentFrom( pSegment, _header, _tailer, NULL, BACKWARD ) ;
                  _insertSegmentToFree( pSegment, FALSE, BACKWARD ) ;
                  if ( _current == pSegment )
                  {
                     _current = NULL ;
                  }
#endif // UTIL_SEGMENT_USE_SORT
                  /// fix _maxDataSeqID
                  if ( _maxDataSeqID == (INT32)pSegment->getSegmentID() )
                  {
                     /// do in while, no-op
                     while ( --_maxDataSeqID >= 0 && _segList[ _maxDataSeqID ]->empty() ) ;
                  }
               }

               ++_releaseTimes ;
            }
            else
            {
               rc = SDB_SYS ;
               PD_LOG( PDSEVERE,
                       "Sanity check failed: " OSS_NEWLINE
                       "  PoolID        : %u" OSS_NEWLINE
                       "  Segment Size  : %u" OSS_NEWLINE
                       "  Obj EyeCatcher: %x" OSS_NEWLINE
                       "  Obj Flag      : %x" OSS_NEWLINE
                       "  Obj Index     : %x" OSS_NEWLINE
                       "  Obj Segment   : %hu" OSS_NEWLINE
                       "  ObjX addr     : %p" OSS_NEWLINE
                       "  ObjT addr     : %p" OSS_NEWLINE
                       "  ObjT Size     : %u" OSS_NEWLINE
                       "  ObjX Size     : %u" OSS_NEWLINE,
                       _poolId,
                       (UINT32)_segList.size(),
                       pObjX->_eyeCatcher,
                       pObjX->_flag,
                       pObjX->_index,
                       pObjX->_segmentID,
                       pObjX,
                       pT,
                       sizeof( T ),
                       sizeof( _objX ) ) ;
#ifdef _DEBUG
               SDB_ASSERT ( FALSE, "Sanity check failed !");
#endif
               goto error ;
            }
         }
         else
         {
#ifdef _DEBUG
            SDB_ASSERT( _isInitialized, "_utilSegmentPool has to be initialized." ) ;
#endif
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         if ( bLatched )
         {
            _latch.release() ;
            bLatched = FALSE ;
         }
#ifdef _DEBUG
         SDB_ASSERT( ( SDB_OK == rc ), "Release failed" ) ;
#endif
         return rc ;
      error:
         goto done ;
      }

      OSS_INLINE BOOLEAN shrinkInProgress()
      {
         return _maintaining.compare( 1 ) ;
      }

      OSS_INLINE UINT32 getHighWatermark() { return _highWatermark ; }

      //
      // Description: shrink unused segments and the _list
      //
      // Input      : none
      // Return     : SDB_OK        -- normal
      //
      // Dependency : this function shall be called after the class is
      //              initialized.
      //              shrink() protects all underneath operations by latch.
      //
      INT32 shrink( UINT32 freeSegToKeep = 1, UINT64 *pFreedSize = NULL, UINT64 expectSize = 0 )
      {
         INT32         rc       = SDB_OK ;
         BOOLEAN       bLatched = FALSE ;
         UTIL_OBJIDX   numOfObjects = 0 ;
         UINT64        freedSize = 0 ;
         UINT32        freeSegNum = 0 ;
         _blockX      *pSegment = NULL ;
         BOOLEAN       doMaintaining = FALSE ;

#ifdef _DEBUG
         SDB_ASSERT( _isInitialized,
                     "shirk can only be done when segment is initialized" ) ;
#endif

         // this flag is checked without latching
         if ( !_maintaining.compareAndSwap( 0, 1 ) )
         {
            /// has someone do maintaining already
            goto done ;
         }

         doMaintaining = TRUE ;

         _latch.get() ;
         bLatched = TRUE ;

         /// shrink pending and compact segment
         _shrinkPending( &freeSegNum, ( expectSize + getBlockSize() - 1 ) / getBlockSize() ) ;
         if ( freeSegNum > 0 )
         {
            freedSize = (UINT64)freeSegNum * getBlockSize() ;
            _shrinkSize += freedSize ;
         }

         /// calc high watermask
         if ( _maxDataSeqID < 0 || _allocatedNum == 0 )
         {
            _highWatermark = _delta - 1 ;
         }
         else
         {
            _highWatermark = ( (UINT32)( _maxDataSeqID + 1 ) * _delta ) - 1 ;
         }

         /// Auto segment to keep
         if ( freeSegToKeep == (UINT32)UTIL_SEGMENT_SEGKEEP_AUTO )
         {
            if ( _lastAcquireTick > 0 &&
                 _pHandler->getTickSpanTime( _lastAcquireTick ) < UTIL_SEGMENT_FREE_TIMEOUT )
            {
               freeSegToKeep = 1 ;
            }
            else
            {
               freeSegToKeep = 0 ;
            }
         }

         numOfObjects = _numOfObjs ;

         if ( _emptyNum > freeSegToKeep )
         {
            FLOAT64 ratio= 0.0 ;

            // if 85% of objects in pool are in use, implies the system might be
            // busy. Likely a new segment might be added in soon.
            ratio = (FLOAT64)_allocatedNum / numOfObjects ;

            if ( _pHandler )
            {
               if ( !_pHandler->canShrink( getObjXSize(), numOfObjects, _allocatedNum ) )
               {
                  goto done ;
               }
            }
            else if ( ratio >= UTIL_SEGMENT_OBJ_IN_USE_RATIO_THRESHOLD )
            {
               goto done ;
            }

            pSegment = _tailer ;
            while ( pSegment && _emptyNum > freeSegToKeep )
            {
               if ( _pHandler && !_pHandler->canShrinkDiscrete() &&
                    _maxDataSeqID >= (INT32)pSegment->getSegmentID() )
               {
                  /// not contiguous with the tailer
                  break ;
               }
               else if ( !pSegment->empty() )
               {
                  /// not empty, skip
                  pSegment = pSegment->_prev ;
                  continue ;
               }
               /// remove from free list
               _blockX *pFind = pSegment ;
               pSegment = _removeSegmentFromFree( pSegment, TRUE, BACKWARD ) ;
               /// save to pending segment
               _insertSegmentToPending( pFind, FORWARD ) ;
            }

            SDB_ASSERT( _numOfObjs == ( _segList.size() - _pendingNum -
                                        _compactTree.count() ) * _delta,
                        "Invalid _numOfObjs" ) ;
            SDB_ASSERT( _usedSegNum == _segList.size() - _pendingNum - _compactTree.count(),
                        "Invalid usedSegNum" ) ;
         }

      done:
         if ( doMaintaining )
         {
            _maintaining.swap( 0 ) ;
         }
         if ( bLatched )
         {
            _latch.release() ;
         }
         if ( pFreedSize )
         {
            *pFreedSize += freedSize ;
         }
         return rc ;
      }

   } ;

   /*
      _utilSegmentInterface
   */
   class _utilSegmentInterface : public SDBObject
   {
      public:
         _utilSegmentInterface() {}
         virtual ~_utilSegmentInterface() {}

         virtual const CHAR* getName() const = 0 ;

      public:
         virtual INT32 init( UINT32 blockSize,
                             UINT64 maxSize,
                             UINT8  poolNum,
                             utilSegmentHandler *pHandler ) = 0 ;

         virtual void setMaxSize( UINT64 maxSize ) = 0 ;

         virtual UINT32 getNumOfObjs() const = 0 ;

         virtual void getSizeInfo( UINT64 &total, UINT64 &used ) const = 0 ;

         virtual UINT64 getUsedSize() const = 0 ;

         virtual BOOLEAN hasPendingSeg() const = 0 ;
         virtual BOOLEAN hasEmptySeg() const = 0 ;
         virtual BOOLEAN hasPendingOrEmptySeg() const = 0 ;

         virtual UINT32  getPendingSegNum() const = 0 ;
         virtual UINT32  getEmptySegNum() const = 0 ;
         virtual UINT32  getPendingAndEmptySegNum() const = 0 ;
         virtual UINT32  getFullSegNum() const = 0 ;

         virtual UINT64  getSegBlockSize() const = 0 ;

         virtual INT32 acquire( void * &pT, UTIL_OBJIDX *pIndex = NULL ) = 0 ;

         virtual INT32 release( const void * pT ) = 0 ;

         virtual INT32 shrink( UINT32 freeSegToKeep = 1,
                               UINT64 *pFreedSize = NULL,
                               UINT64 expectSize = 0 ) = 0 ;

         virtual UINT32 dump( CHAR *pBuff, UINT32 buffLen,
                                       UINT64 *pAcquireTimes = NULL,
                                       UINT64 *pReleaseTimes = NULL,
                                       UINT64 *pOOMTimes = NULL,
                                       UINT64 *pOOLTimes = NULL,
                                       UINT64 *pShrinkSize = NULL ) = 0 ;

   } ;
   typedef _utilSegmentInterface utilSegmentInterface ;

   /*
      _utilSegmentManager define and implement
   */
   template < class T >
   class _utilSegmentManager : public _utilSegmentInterface
   {
      private :
         INT32       _id ;
         // acquire operation proceed in round robin
         ossAtomic32 _round ;
         CHAR        _name[ UTIL_SEGMENT_NAME_LEN + 1 ] ;
         UINT32      _poolNum ;

         /// stat info
         UINT64      _acquireTimes ;
         UINT64      _releaseTimes ;
         UINT64      _oomTimes ;
         UINT64      _oolTimes ;
         UINT64      _ballonTimes ;
         UINT64      _shrinkSize ;

         _utilSegmentPool< T > _pool[ _SEGMENT_MGR_MAX_POOLS ] ;

      private :
         // internal / helper function to get the poolId an object belongs to
         OSS_INLINE UINT32 _getPoolIdByAddr( const void * pT )
         {
            UINT32 poolId = UTIL_INVALID_OBJ_INDEX ;
            if ( NULL != pT )
            {
               typename _utilSegmentPool< T >::_objX * p =
                  ( typename _utilSegmentPool< T >::_objX * )_GET_OBJX_ADDRESS( pT ) ;
               if ( p && ( _SEGMENT_OBJ_EYE_CATCHER == p->_eyeCatcher ) )
               {
                  // unpack poolId from _objX-> _index
                  poolId = _GET_UNPACKED_POOLID( p->_index ) ;
               }
            }
            return poolId ;
         }

         OSS_INLINE void   clearStat_i()
         {
            _acquireTimes = 0 ;
            _releaseTimes = 0 ;
            _oomTimes = 0 ;
            _oolTimes = 0 ;
            _ballonTimes = 0 ;
            _shrinkSize = 0 ;
         }

      public  :
         _utilSegmentManager( INT32 id, const CHAR *pName = NULL ) : _round( 0 )
         {
            _id = id ;
            ossMemset( _name, 0, sizeof( _name ) ) ;
            if ( pName )
            {
               ossStrncpy( _name, pName, UTIL_SEGMENT_NAME_LEN ) ;
            }
            _poolNum = 0 ;

            clearStat_i() ;
         }
         _utilSegmentManager( INT32 id,
                              UINT64 numberOfObjs,
                              UINT64 maxNumberOfObjs,
                              const CHAR *pName = NULL )
         {
            _id = id ;
            ossMemset( _name, 0, sizeof( _name ) ) ;
            if ( pName )
            {
               ossStrncpy( _name, pName, UTIL_SEGMENT_NAME_LEN ) ;
            }
            _poolNum = 0 ;

            clearStat_i() ;

            init( numberOfObjs, maxNumberOfObjs ) ;
         }

         virtual ~_utilSegmentManager() { fini(); }

         virtual const CHAR* getName() const
         {
            return _name ;
         }

         virtual INT32 init( UINT32 blockSize,
                             UINT64 maxSize,
                             UINT8  poolNum,
                             utilSegmentHandler *pHandler )
         {
            INT32 rc = SDB_OK ;
            UINT64 poolMaxSize = 0 ;

            if ( !pHandler )
            {
               SDB_ASSERT( FALSE, "pHandler can't be NULL" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            _poolNum = poolNum ;
            if ( _poolNum < 1 )
            {
               _poolNum = 1 ;
            }
            else if ( _poolNum > _SEGMENT_MGR_MAX_POOLS )
            {
               _poolNum = _SEGMENT_MGR_MAX_POOLS ;
            }

            poolMaxSize = maxSize / _poolNum ;
            if ( poolMaxSize / _pool[0].getObjXSize() > _SEGMENT_OBJ_MAX_NUM )
            {
               poolMaxSize = _SEGMENT_OBJ_MAX_NUM * _pool[0].getObjXSize() ;
            }

            for ( UINT32 i = 0 ; i < _poolNum ; i++ )
            {
               rc = _pool[ i ].init( _id,
                                     i,
                                     blockSize,
                                     poolMaxSize,
                                     pHandler ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Initialzation slot[%s] failed, pool: %u, rc: %d",
                          getName(), i, rc ) ;
                  goto error ;
               }
            }

         done:
            return rc ;
         error:
            goto done ;
         }

         virtual void setMaxSize( UINT64 maxSize )
         {
            UINT64 poolMaxSize = maxSize / _poolNum ;

            for ( UINT32 i = 0 ; i < _poolNum ; i++ )
            {
               _pool[ i ].setMaxSize( poolMaxSize ) ;
            }
         }

         void fini()
         {
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               _pool[ i ].fini() ;
            }
         }

         virtual UINT32 getNumOfObjs() const
         {
            UINT32 objs = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               objs += _pool[ i ].getNumOfObjs() ;
            }
            return objs ;
         }

         virtual void getSizeInfo( UINT64 &total, UINT64 &used ) const
         {
            UINT64 poolTotal = 0 ;
            UINT64 poolUsed = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               _pool[ i ].getSizeInfo( poolTotal, poolUsed ) ;
               total += poolTotal ;
               used += poolUsed ;
            }
         }

         virtual UINT64 getUsedSize() const
         {
            UINT64 total = 0 ;
            UINT64 used = 0 ;
            getSizeInfo( total, used ) ;
            return used ;
         }

         virtual BOOLEAN hasPendingSeg() const
         {
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               if ( _pool[ i ].getPendingSegNum() > 0 )
               {
                  return TRUE ;
               }
            }
            return FALSE ;
         }

         virtual BOOLEAN hasEmptySeg() const
         {
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               if ( _pool[ i ].getEmptySegNum() > 0 )
               {
                  return TRUE ;
               }
            }
            return FALSE ;
         }

         virtual BOOLEAN hasPendingOrEmptySeg() const
         {
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               if ( _pool[ i ].getPendingSegNum() > 0 ||
                    _pool[ i ].getEmptySegNum() > 0 )
               {
                  return TRUE ;
               }
            }
            return FALSE ;
         }

         virtual UINT32  getPendingSegNum() const
         {
            UINT32 pendingSegNum = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               pendingSegNum += _pool[ i ].getPendingSegNum() ;
            }
            return pendingSegNum ;
         }

         virtual UINT32  getEmptySegNum() const
         {
            UINT32 emptySegNum = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               emptySegNum += _pool[ i ].getEmptySegNum() ;
            }
            return emptySegNum ;
         }

         virtual UINT32  getPendingAndEmptySegNum() const
         {
            UINT32 segNum = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               segNum += _pool[ i ].getPendingSegNum() ;
               segNum += _pool[ i ].getEmptySegNum() ;
            }
            return segNum ;
         }

         virtual UINT32  getFullSegNum() const
         {
            UINT32 fullSegNum = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               fullSegNum += _pool[ i ].getFullSegNum() ;
            }
            return fullSegNum ;
         }

         virtual UINT64  getSegBlockSize() const
         {
            if ( _poolNum > 0 )
            {
               return _pool[ 0 ].getBlockSize() ;
            }
            return 0 ;
         }

         OSS_INLINE T * getObjPtrByIndex( const UTIL_OBJIDX idx )
         {
            if ( ! IS_VALID_SEG_OBJ_INDEX( idx ) )
            {
               return NULL ;
            }
            return _pool[ _GET_UNPACKED_POOLID( idx ) ].getObjPtrByIndex( idx );
         }

         OSS_INLINE UTIL_OBJIDX getIndexByAddr ( const void * pT )
         {
            UINT32 pool = _getPoolIdByAddr( pT ) ;
            if ( UTIL_INVALID_OBJ_INDEX != pool )
            {
               return _pool[ pool ].getIndexByAddr( pT ) ;
            }
            else
            {
               return UTIL_INVALID_OBJ_INDEX ;
            }
         }

         OSS_INLINE BOOLEAN isLowerThanHWM( const void * pT )
         {
            BOOLEAN result = FALSE ;
#ifdef _DEBUG
            SDB_ASSERT ( ( NULL != pT ),
                         "Invalid argument, input pointer can't be NULL " ) ;
#endif
            UINT32 pool = _getPoolIdByAddr( pT ) ;
#ifdef _DEBUG
            SDB_ASSERT( ( UTIL_INVALID_OBJ_INDEX != pool ),
                        "Invalid pool number" ) ;
#endif
            if ( ( ( _pool[ pool ].getHighWatermark() ) &
                   _SEGMENT_OBJ_INDEX_MASK ) >
                 ( ( _pool[ pool ].getIndexByAddr( pT ) ) &
                   _SEGMENT_OBJ_INDEX_MASK ) )
            {
               result = TRUE ;
            }
            return result ;
         }

         virtual INT32 acquire( void * &pT, UTIL_OBJIDX *pIndex = NULL )
         {
            INT32 rc = SDB_OK ;
            UINT32 pool ;
            UINT32 retryCount = 0 ;
            do
            {
               retryCount++ ;
               // pool = _round.inc() % _poolNum ;
               // the _poolNum is power of 2
               // so modulo can be optimized
               pool = _round.inc() & ( _poolNum - 1 ) ;
               // switch to next pool if shrinking in progress
               while ( _pool[ pool ].shrinkInProgress() )
               {
                  pool = _round.inc() & ( _poolNum - 1 ) ;
               }
               rc = _pool[ pool ].acquire( pT, pIndex ) ;
               if ( SDB_OK == rc )
               {
                  break ;
               }
            } while ( ( SDB_OSS_UP_TO_LIMIT == rc ) &&
                      ( retryCount < _poolNum ) ) ;
            return rc ;
         }

         virtual INT32 release( const void * pT )
         {
            UINT32 pool = _getPoolIdByAddr( pT ) ;
            if ( UTIL_INVALID_OBJ_INDEX != pool )
            {
               return _pool[ pool ].release( pT ) ;
            }
            else
            {
               return SDB_INVALIDARG ;
            }
         }

         virtual INT32 shrink( UINT32 freeSegToKeep = 1,
                               UINT64 *pFreedSize = NULL,
                               UINT64 expectSize = 0 )
         {
            // REVISIT :
            // proper scheduling algorithm to be implemented
            INT32 rc = SDB_OK ;
            UINT64 freedSize = 0 ;
            UINT64 poolExpectSize = expectSize ;

            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               rc = _pool[ i ].shrink( freeSegToKeep, &freedSize, poolExpectSize ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to shrink pool:%u, rc:%d", i, rc ) ;
               }

               if ( expectSize > 0 )
               {
                  if ( freedSize > expectSize )
                  {
                     break ;
                  }
                  else
                  {
                     poolExpectSize = expectSize - freedSize ;
                  }
               }
            }

            if ( pFreedSize )
            {
               *pFreedSize += freedSize ;
            }

            return rc ;
         }

         virtual UINT32 dump( CHAR *pBuff, UINT32 buffLen,
                              UINT64 *pAcquireTimes = NULL,
                              UINT64 *pReleaseTimes = NULL,
                              UINT64 *pOOMTimes = NULL,
                              UINT64 *pOOLTimes = NULL,
                              UINT64 *pShrinkSize = NULL )
         {
            UINT32 len = 0 ;

            UINT64 acquireTimes = 0 ;
            UINT64 releaseTimes = 0 ;
            UINT64 oomTimes = 0 ;
            UINT64 oolTimes = 0 ;
            UINT64 ballonTimes = 0 ;
            UINT64 shrinkSize = 0 ;

            UINT64 totalSize = 0 ;
            UINT64 usedSize = 0 ;

            getSizeInfo( totalSize, usedSize ) ;

            len = ossSnprintf( pBuff, buffLen,
                               OSS_NEWLINE
                               "---- Segment Name( %s ) ----" OSS_NEWLINE
                               "         Pool Num : %u" OSS_NEWLINE
                               "       Total Size : %llu" OSS_NEWLINE
                               "        Used Size : %llu" OSS_NEWLINE
                               "        Free Size : %llu" OSS_NEWLINE,
                               _name,
                               _poolNum,
                               totalSize,
                               usedSize,
                               totalSize - usedSize ) ;

            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               len += _pool[ i ].dump( pBuff + len, buffLen - len,
                                       &acquireTimes,
                                       &releaseTimes,
                                       &oomTimes,
                                       &oolTimes,
                                       &shrinkSize,
                                       &ballonTimes ) ;
            }

            if ( pAcquireTimes )
            {
               *pAcquireTimes += acquireTimes ;
            }
            if ( pReleaseTimes )
            {
               *pReleaseTimes += releaseTimes ;
            }
            if ( pOOMTimes )
            {
               *pOOMTimes += oomTimes ;
            }
            if ( pOOLTimes )
            {
               *pOOLTimes += oolTimes ;
            }
            if ( pShrinkSize )
            {
               *pShrinkSize += shrinkSize ;
            }

            len += ossSnprintf( pBuff + len, buffLen - len,
                                OSS_NEWLINE
                                "Segment Stat" OSS_NEWLINE
                                "    Acquire Times : %llu (Inc: %lld )" OSS_NEWLINE
                                "    Release Times : %llu (Inc: %lld )" OSS_NEWLINE
                                "        OOM Times : %llu (Inc: %lld )" OSS_NEWLINE
                                "        OOL Times : %llu (Inc: %lld )" OSS_NEWLINE
                                "     Ballon Times : %llu (Inc: %lld )" OSS_NEWLINE
                                "      Shrink Size : %llu (Inc: %lld )" OSS_NEWLINE,
                                acquireTimes,
                                acquireTimes - _acquireTimes,
                                releaseTimes,
                                releaseTimes - _releaseTimes,
                                oomTimes,
                                oomTimes - _oomTimes,
                                oolTimes,
                                oolTimes - _oolTimes,
                                ballonTimes,
                                ballonTimes - _ballonTimes,
                                shrinkSize,
                                shrinkSize - _shrinkSize ) ;

            _acquireTimes = acquireTimes ;
            _releaseTimes = releaseTimes ;
            _oomTimes = oomTimes ;
            _oolTimes = oolTimes ;
            _ballonTimes = ballonTimes ;
            _shrinkSize = shrinkSize ;

            return len ;
         }

   } ;

} // namespace engine

#endif // UTIL_SEGMENT_HPP__

