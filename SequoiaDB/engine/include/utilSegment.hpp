/******************************************************************************


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
#include "utilBitmap.hpp"  // _utilBitmap

typedef UINT32 UTIL_OBJIDX ;
#define UTIL_INVALID_OBJ_INDEX   (( UTIL_OBJIDX )( -1 ))

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
         virtual void      onAllocSegment( UINT64 size ) = 0 ;
         virtual void      onReleaseSegment( UINT64 freedSize ) = 0 ;
         virtual BOOLEAN   canShrink( UINT32 blockSize,
                                      UINT64 totalSize,
                                      UINT64 usedSize ) = 0 ;
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
//    _list: an array of index ( object index in segments )
//
//       0     1    2    3          m*n + n-1
//      +----+----+----+----+      +----+
//      | -1 | -1 |  2 | 73 | ...  | 28 |
//      +----+----+----+----+      +----+
//                  ^
//                  |
//                 _begin
//
//    _begin     : the position in _list where to acquire a free object
//    _begin - 1 : the position in _list where to release/return an object
//
//
// Segment pool 1 :
//
// ...
//
// Segment pool 15
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

   #define _SEGMENT_OBJ_EYE_CATCHER    ( ( UINT16 ) 0xBEEF )
   #define _SEGMENT_OBJ_FLAG_ACQUIRED  ( ( UINT16 ) 0x5555 )
   #define _SEGMENT_OBJ_FLAG_RELEASED  ( ( UINT16 ) 0xAAAA )

   #define IS_VALID_SEG_OBJ_INDEX( _objectIndex_ ) \
      ( UTIL_INVALID_OBJ_INDEX != _objectIndex_ )

   // obtain the container _objX address from an _obj ptr, where _objX is
   // defined as following :
   //    template < class T >
   //    struct _objX : public SDBObject
   //    {
   //       UINT16      _eyeCatcher ;
   //       UINT16      _flag ;
   //       UTIL_OBJIDX _index ;
   //       T           _obj ;
   //    } ;
   #define _GET_OBJX_ADDRESS( _p_ )      \
       ((CHAR*)( _p_ ) - sizeof(UINT16) - sizeof(UINT16) - sizeof(UTIL_OBJIDX))

   #define _GET_PACKED_POOLID( _pool_ )  \
             (( _pool_ << _SEGMENT_OBJ_POOLID_SHIFT ) & _SEGMENT_OBJ_POOL_MASK )

   #define _GET_UNPACKED_POOLID( _idx_ ) \
             (( _idx_ & _SEGMENT_OBJ_POOL_MASK ) >> _SEGMENT_OBJ_POOLID_SHIFT )


   template < class T >
   class _utilSegmentPool : public SDBObject
   {
   public :
      struct _objX : public SDBObject
      {
         UINT16      _eyeCatcher ;
         UINT16      _flag ;
         UTIL_OBJIDX _index ;
         T           _obj ;
         _objX()
         {
            _eyeCatcher = _SEGMENT_OBJ_EYE_CATCHER ;
            _flag       = _SEGMENT_OBJ_FLAG_RELEASED ;
            _index      = UTIL_INVALID_OBJ_INDEX ;
         }
      } ;

   private :
      UTIL_OBJIDX *  _list  ;         // a list/array of obj indices
      UINT32         _begin ;         // the position in list where acquire from
      UINT32         _delta ;         // number of objects in a segment
      UINT32         _numOfObjs ;     // total number of objects in this pool
      UINT32         _maxNumOfObjs ;  // max number of objects in this pool
      UINT32         _exponent ;      // exponent when round up to power of 2
      UINT32         _poolId  ;       // pool ID
      ossAtomic32    _maintaining ;   // shrinking in progres
      UINT32         _highWatermark ; // max object index in use
      BOOLEAN        _isInitialized ;
      ossSpinXLatch  _latch ;
      std::vector< _objX * > _segList;// a list of segments, each segment is
                                      // an array of object(T)
      utilSegmentHandler   *_pHandler;// callback handler

      /// stat info
      UINT64         _acquireTimes ;
      UINT64         _releaseTimes ;
      UINT64         _oomTimes ;
      UINT64         _oolTimes ;
      UINT64         _shrinkSize ;

   protected:
      void   clearStat_i()
      {
         _acquireTimes = 0 ;
         _releaseTimes = 0 ;
         _oomTimes = 0 ;
         _oolTimes = 0 ;
         _shrinkSize = 0 ;
      }

   public :
      _utilSegmentPool() : _list(NULL),
                           _begin(0),
                           _delta(0),
                           _numOfObjs(0),
                           _maxNumOfObjs(0),
                           _exponent(0),
                           _poolId(0),
                           _maintaining(0),
                           _highWatermark(0),
                           _isInitialized( FALSE ),
                           _pHandler( NULL )
      {
         clearStat_i() ;
      }

      ~_utilSegmentPool() ;

      void  setMaxObjects( UINT32 maxNumberOfObjs )
      {
         _maxNumOfObjs = maxNumberOfObjs ;

         if ( _maxNumOfObjs > _SEGMENT_OBJ_MAX_NUM )
         {
            _maxNumOfObjs = _SEGMENT_OBJ_MAX_NUM ;
         }
         else if ( _maxNumOfObjs > 0 && _maxNumOfObjs < _delta )
         {
            _maxNumOfObjs = _delta ;
         }
      }

      UINT32 dump( CHAR *pBuff, UINT32 buffLen,
                   UINT64 *pAcquireTimes = NULL,
                   UINT64 *pReleaseTimes = NULL,
                   UINT64 *pOOMTimes = NULL,
                   UINT64 *pOOLTimes = NULL,
                   UINT64 *pShrinkSize = NULL )
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

         len = ossSnprintf( pBuff, buffLen,
                            OSS_NEWLINE
                            "       Pool ID : %u"OSS_NEWLINE
                            "   Max Objects : %u"OSS_NEWLINE
                            "         Delta : %u"OSS_NEWLINE
                            "   Objects Num : %u"OSS_NEWLINE
                            "   Segment Num : %u"OSS_NEWLINE
                            "     Begin Pos : %u"OSS_NEWLINE
                            " High Watermark: %u"OSS_NEWLINE
                            " Acquire Times : %llu"OSS_NEWLINE
                            " Release Times : %llu"OSS_NEWLINE
                            "     OOM Times : %llu"OSS_NEWLINE
                            "     OOL Times : %llu"OSS_NEWLINE
                            "   Shrink Size : %llu"OSS_NEWLINE,
                            _poolId,
                            _maxNumOfObjs,
                            _delta,
                            _numOfObjs,
                            _numOfObjs / _delta,
                            _begin,
                            _highWatermark,
                            _acquireTimes,
                            _releaseTimes,
                            _oomTimes,
                            _oolTimes,
                            _shrinkSize ) ;
         return len ;
      }

   private :
      //
      // Description: add a new segment and expand the _list
      //              when no free objects
      // Input      : none
      // Return     : SDB_OK               -- normal
      //              SDB_OSS_UP_TO_LIMIT  -- exceeds resource max threshold
      //              SDB_OOM              -- out of memory
      // Dependency : this function is called by acquire() only; and the caller
      //              shall make sure the operation is protected by latch
      INT32 _expandList() ;

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
         return ( _numOfObjs - 1 ) <= _begin ? TRUE : FALSE ;
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
         return _numOfObjs >= _maxNumOfObjs ? TRUE : FALSE ;
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
      OSS_INLINE _objX * _getObjXByIndex( const UTIL_OBJIDX idx )
      {
         _objX * pObj      = NULL ;
         UTIL_OBJIDX index = ( idx & _SEGMENT_OBJ_INDEX_MASK ) ;

#ifdef _DEBUG
         if ( IS_VALID_SEG_OBJ_INDEX( idx ) && ( index < _numOfObjs ) )
#endif
         {
            // i = idx / _delta ;
            // j = idx % _delta ;
            // the _delta is round up to nearest power of 2,
            // so divide and modulo can be optimized
            _objX * pSegList = ( _objX * )( _segList[ index >> _exponent ] ) ;
            if ( pSegList )
            {
               pObj  = ( _objX * )&( pSegList[ index & ( _delta - 1 ) ] ) ;
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
      INT32 init(  UINT32 poolId,
                   UINT32 numberOfObjs,
                   UINT32 maxNumberOfObjs,
                   utilSegmentHandler *pHandler = NULL ) ;

      //
      // Description: Free all objects allocated in segments and the object
      //              index array, _list
      // Return     : none
      // Dependency : this function shall be called just before destroy,
      //              the caller shall guarantee no other threads are accessing
      //              the objects.
      //
      void fini() ;

      //
      // Description: get total number of objects allocated in a segment pool
      // Return     : total number of objects allocated in a segment pool
      //
      OSS_INLINE UINT32 getNumOfObjAllocated() { return _numOfObjs ; }

      //
      // Description: get an object's address by its index
      // Input      :
      //              idx -- the object index
      // Return     : address of the object ( specified by the index )
      // Dependency : this function shall be called after the class is
      //              initialized. It expects an correct index, i.e.,
      //              idx < _numOfObjs
      T * getObjPtrByIndex( const UTIL_OBJIDX idx ) ;

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
      UTIL_OBJIDX getIndexByAddr ( const T * pT ) ;

      //
      // Description: acquire/apply a free object from the segments
      // Input      : none
      // Output     :
      //              idx -- the object index, each object has an unique index,
      //                     the index number is continuous, starting from 0
      //              pT  -- the pointer/address of the object
      // Return     : SDB_OK,
      //              SDB_OSS_UP_TO_LIMIT,
      //              SDB_SYS,
      //              error rc returned from _expandList()
      // Dependency : this function shall be called after the class is
      //              initialized.
      //              when _begin is equal to '_numOfObjs - 1',
      //              means lack of free objects and a new segment ( array of
      //              object T ) will be added and the _list will be expanded.
      //              acquire() protects all underneath operations by latch.
      //
      INT32 acquire( UTIL_OBJIDX & idx,  T * &pT ) ;

      //
      // Description: acquire/apply a free object from the segments
      // Input      : none
      // Output     :
      //              pT  -- the pointer/address of the object
      // Return     : SDB_OK,
      //              SDB_OSS_UP_TO_LIMIT,
      //              SDB_SYS,
      //              error rc returned from _expandList()
      // Dependency : this function is thin wrapper of above acquire()
      //
      INT32 acquire( T * &pT ) ;

      //
      // Description: release/return an object to the segments by its index
      // Input      : idx -- the object index
      // Output     : none
      // Return     : SDB_OK
      //              SDB_SYS -- when error occurs
      // Dependency : this function shall be called after the class is
      //              initialized.
      //              release() protects all underneath operations by latch.
      //
      INT32 release( const UTIL_OBJIDX idx ) ;

      //
      // Description: release/return an object to the segments by its address
      // Input      : pT -- address/pointer of the object
      // Output     : none
      // Return     : SDB_OK
      //              SDB_SYS        -- when error occurs
      //              SDB_INVALIDARG -- pT is invalid address
      // Dependency : this function is thin wrapper function of above release()
      //              this function will be slower than above release() as an
      //              extra operation, getIndexByAddr(), is performed ( convert
      //              address to index )
      //
      INT32 release( const T * pT ) ;

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
      //              SDB_OOM       -- out of memory
      //
      // Dependency : this function shall be called after the class is
      //              initialized.
      //              shrink() protects all underneath operations by latch.
      //
      INT32 shrink( UINT32 freeSegToKeep = 1, UINT64 *pFreedSize = NULL ) ;

   #ifdef _DEBUG
      UTIL_OBJIDX * getListAddr() { return _list ; }
      UINT32 getNumOfObjInuse() { return _begin ; }
   #endif
   } ;

   // get an object's index by its address
   template < class T >
   OSS_INLINE UTIL_OBJIDX _utilSegmentPool< T >::getIndexByAddr( const T * pT )
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
         // verify the index
         UINT32  packedPoolID = _GET_PACKED_POOLID( _poolId ) ;
         UTIL_OBJIDX index = UTIL_INVALID_OBJ_INDEX ;
         UINT32 segs       = _segList.size() ;
         _objX  * pSegList = NULL ;

         for ( UTIL_OBJIDX i = 0; i < segs ; i++ )
         {
            pSegList = _segList[ i ] ;
            if (    pSegList
                 && ( pT >= &( pSegList[ 0 ]._obj ) )
                 && ( pT <= &( pSegList[ _delta - 1 ]._obj ) ) )
            {
               index = i * _delta +
                     ((CHAR*)pT - (CHAR*)&(pSegList[0]._obj)) / sizeof( _objX );
               index = ( ( index & _SEGMENT_OBJ_INDEX_MASK ) | packedPoolID ) ;

               SDB_ASSERT( ( index == idx ),
                           "Verification failed, invalid address." ) ;
               break ;
            }
         }
#endif

      }
      return idx ;
   }

   // Free all objects allocated in segments and the object index array, _list
   template < class T >
   void _utilSegmentPool< T >::fini()
   {
      if ( _isInitialized )
      {
         UINT32 len = _segList.size() ;
         for ( UINT32 i = 0; i < len ; i++ )
         {
            if ( _segList[i] )
            {
               SDB_OSS_DEL [] ( _segList[i] ) ;
            }
            _segList[i] = NULL ;

            if ( _pHandler )
            {
               _pHandler->onReleaseSegment( (UINT64)_delta * sizeof( _objX ) ) ;
            }
         }
         _segList.clear() ;
         SAFE_OSS_FREE( _list ) ;
         _list = NULL ;
         _isInitialized = FALSE ;
         _pHandler = NULL ;
      }
   }

   template < class T >
   _utilSegmentPool< T >::~_utilSegmentPool()
   {
      if ( _isInitialized )
      {
         fini();
      }
   }

   // allocate a new segment of objects and expand the _list
   template < class T >
   INT32 _utilSegmentPool< T >::_expandList()
   {
      INT32         rc       = SDB_OK ;
      UTIL_OBJIDX * pListTmp = NULL ;
      _objX       * pSegTmp  = NULL ;
      UINT32    numOfObjects = _numOfObjs ;
      UINT32    newSize      = numOfObjects + _delta ;
      UINT32    packedPoolID = _GET_PACKED_POOLID( _poolId ) ;

#ifdef _DEBUG
      SDB_ASSERT( _isInitialized,
                  "Expand can only be done when segment is initialized" ) ;
#endif
      if (    ( UTIL_INVALID_OBJ_INDEX != newSize )
           && ( newSize <= _maxNumOfObjs )
           && ( 0 != _delta ) )
      {
         // allocate a new segment of object
         pSegTmp  = SDB_OSS_NEW _objX[ _delta ] ;
         // expand _list with new size
         pListTmp = ( UTIL_OBJIDX * )SDB_OSS_MALLOC( sizeof( UTIL_OBJIDX ) *
                                                     newSize ) ;
         if ( ( NULL != pListTmp ) && ( NULL != pSegTmp ) )
         {
            // copy old _list content
            if ( _list )
            {
               ossMemcpy( pListTmp, _list, sizeof(UTIL_OBJIDX) * numOfObjects );
            }

            // initiate the newly allocated portion
            for ( UINT32 i = 0 ; i < _delta ; i++ )
            {
               // initialize the list
               pListTmp[ numOfObjects + i ] = numOfObjects + i ;
               // initialize the _objX
               pSegTmp[i]._index =
               (((numOfObjects + i) & _SEGMENT_OBJ_INDEX_MASK) | packedPoolID) ;
            }

            // set _numOfObjs to new size
            _numOfObjs = newSize ;

            // discard old _list
            SAFE_OSS_FREE( _list ) ;

            // assign _list with new allocation, pListTmp
            _list = pListTmp ;

            // add new segment to segment list
            _segList.push_back( pSegTmp ) ;
         }
         else
         {
            SAFE_OSS_FREE( pListTmp ) ;
            if ( NULL != pSegTmp )
            {
               SDB_OSS_DEL [] pSegTmp ;
            }
            rc = SDB_OOM ;
#ifdef _DEBUG
            PD_LOG( PDERROR,
                    "Out of memory when expand : %d"OSS_NEWLINE
                    " Delta         : %u"OSS_NEWLINE
                    " MaxNumOfObjs  : %u"OSS_NEWLINE
                    " NewSize       : %u"OSS_NEWLINE
                    " ObjT Size     : %u"OSS_NEWLINE
                    " ObjX Size     : %u"OSS_NEWLINE,
                    rc,
                    _maxNumOfObjs,
                    newSize,
                    sizeof( T ),
                    sizeof( _objX ) ) ;
#endif
         }
      }
      else
      {
         // exceed the lock resorce limitation
         rc = SDB_OSS_UP_TO_LIMIT ;
      }

      return rc ;
   }

   #define UTIL_SEGMENT_OBJ_IN_USE_RATIO_THRESHOLD ( 0.85 )
   template < class T >
   INT32 _utilSegmentPool< T >::shrink( UINT32 freeSegToKeep,
                                        UINT64 *pFreedSize )
   {
      INT32         rc       = SDB_OK ;
      BOOLEAN       bLatched = FALSE ;
      UTIL_OBJIDX * pListTmp = NULL ;
      utilBitmap  * pUsedList= NULL ;
      UINT32        numOfObjects, maxObj, newSize, segInUse ;
      UINT64        freedSize = 0 ;
      UINT32        freeSegNum = 0 ;

#ifdef _DEBUG
      SDB_ASSERT( _isInitialized,
                  "shirk can only be done when segment is initialized" ) ;
#endif

      _latch.get() ;
      bLatched = TRUE ;

      numOfObjects = _numOfObjs ;

      // this flag is checked without latching
      _maintaining.swap( 1 ) ;
      if ( _isInitialized && ( NULL != _list ) && numOfObjects > 0 )
      {
         maxObj       = 0 ;
         FLOAT64 ratio= 0.0 ;

         if ( _begin > 0 && 1 == _segList.size() )
         {
            goto done ;
         }

         // if 85% of objects in pool are in use, implies the system might be
         // busy. Likely a new segment might be added in soon.
         ratio = (FLOAT64)_begin / numOfObjects ;

         if ( _pHandler )
         {
            if ( !_pHandler->canShrink( sizeof( T ),
                                        (UINT64)numOfObjects * sizeof ( _objX ),
                                        (UINT64)_begin * sizeof ( _objX ) ) )
            {
               goto done ;
            }
         }
         else if ( ratio >= UTIL_SEGMENT_OBJ_IN_USE_RATIO_THRESHOLD )
         {
            goto done ;
         }

         // find the max obj index in use
         if ( _begin > 0 )
         {
            pUsedList = SDB_OSS_NEW utilBitmap( numOfObjects ) ;
            if ( NULL == pUsedList )
            {
               rc = SDB_OOM ;
               goto error ;
            }

            // for each free object in _list, mark its corresponding position
            // as 1 in the newly constructed bitmap, pUsedList. Thus, we get
            // a list of all object are currently in use (the bits remain as 0)
            for ( UINT32 i= _begin ; i < numOfObjects; i++ )
            {
               pUsedList->setBit( _list[i] ) ;
            }
            // then, starting from the end of the pUsedList, find the first
            // bit is equal to 0, and its position is the max object index
            // in use
            for ( UINT32 i = numOfObjects - 1 ; i >= 0 ; i-- )
            {
               if ( ! pUsedList->testBit( i ) )
               {
                  maxObj = i ;
                  break ;
               }
            }
         }

         // set high water mark if it is greater than 2 times of segment
         if ( maxObj >= ( _delta << 1 ) )
         {
            _highWatermark = maxObj ;
         }

         // calculate the highest segment in use by max obj index
         if ( _begin > 0 )
         {
            segInUse  = ( maxObj >> _exponent ) + 1 + freeSegToKeep ;
         }
         else
         {
            segInUse = freeSegToKeep ;
         }

         // if there are enough segments to be freed
         if ( _segList.size() > segInUse )
         {
            // calculate new size
            newSize = ( segInUse << _exponent ) ;

            if ( newSize > 0 )
            {
               // allocate _list with new size
               pListTmp = ( UTIL_OBJIDX * )SDB_OSS_MALLOC( sizeof( UTIL_OBJIDX ) *
                                                           newSize ) ;
               if ( !pListTmp )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }
            }

            if ( NULL != pListTmp )
            {
               //
               // restore the free objects
               //
               // We may simply copy the free obj index in _list starting
               // from _begin and exclude these are greater than newsize.
               // However, following approach has sorting effect on object
               // index, obj with higher index is put closer to the end of
               // the list. Thus, next shrink operation may have better
               // chance to free more segments
               if ( NULL != pUsedList )
               {
                  for ( UINT32 i = 0, j = _begin ; i < newSize; i++ )
                  {
                     if ( pUsedList->testBit( i ) )
                     {
                        pListTmp[j] = i ;
                        j++ ;
                     }
                  }
               }
               else
               {
#ifdef _DEBUG
                  SDB_ASSERT( ( 0 == _begin ), "_begin must be 0" ) ;
#endif
                  for ( UINT32 i = 0; i < newSize; i++ )
                  {
                     pListTmp[i] = i ;
                  }
               }
            }

            // set _numOfObjs to new size
            _numOfObjs = newSize ;

            // discard old _list
            SAFE_OSS_FREE( _list ) ;

            // assign _list with new allocation, pListTmp
            _list = pListTmp ;

            // free the segments
            for ( INT32 i = (INT32)_segList.size() - 1 ;
                  i >= (INT32)segInUse ;
                  --i )
            {
               SDB_OSS_DEL [] ( _segList[i] ) ;
               _segList.pop_back() ;
               ++freeSegNum ;
            }

            freedSize = ( (UINT64)freeSegNum << _exponent ) *
                        sizeof( _objX ) ;

            _shrinkSize += freedSize ;

            if ( _pHandler )
            {
               _pHandler->onReleaseSegment( freedSize ) ;
            }
         }  // no enough free segs, _segList.size() > segInUse
      }

   done:
      _maintaining.swap( 0 ) ;
      if ( bLatched )
      {
         _latch.release() ;
      }
      if ( NULL != pUsedList )
      {
         SDB_OSS_DEL pUsedList ;
      }
      if ( pFreedSize )
      {
         *pFreedSize += freedSize ;
      }
      return rc ;
   error:
      goto done ;
   }

   // Initialization
   //   numberOfObjs     -- number of objects in a segment
   //   maxNumberOfObjs  -- max number of objects
   template < class T >
   INT32 _utilSegmentPool< T >::init
   (
      UINT32 poolId,
      UINT32 numberOfObjs,
      UINT32 maxNumberOfObjs,
      utilSegmentHandler *pHandler
   )
   {
      INT32   rc           = SDB_OK ;

      if ( numberOfObjs > _SEGMENT_OBJ_MAX_NUM )
      {
         numberOfObjs = _SEGMENT_OBJ_MAX_NUM ;
      }
      if ( maxNumberOfObjs > _SEGMENT_OBJ_MAX_NUM )
      {
         maxNumberOfObjs = _SEGMENT_OBJ_MAX_NUM ;
      }

      if (    ( numberOfObjs > 0 )
           && ( poolId < _SEGMENT_MGR_MAX_POOLS ) )
      {
         // round up numberOfObjs to the nearest power of 2
         _delta        = ossNextPowerOf2( numberOfObjs, &_exponent ) ;
         _maxNumOfObjs = maxNumberOfObjs ;
         _poolId       = poolId ;

         if ( _maxNumOfObjs > 0 && _maxNumOfObjs < _delta )
         {
            _maxNumOfObjs = _delta ;
         }
         _highWatermark = ( _delta << 1 ) ;
         _pHandler = pHandler ;
         _isInitialized = TRUE ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Failed initialize due to invalid arguments, rc:%d"OSS_NEWLINE
                 "   PoolID          : %d"OSS_NEWLINE
                 "   NumberOfObjs    : %u"OSS_NEWLINE
                 "   MaxNumberOfObjs : %u"OSS_NEWLINE
                 "   ObjT Size       : %u"OSS_NEWLINE
                 "   ObjX Size       : %u"OSS_NEWLINE,
                 rc,
                 poolId,
                 numberOfObjs,
                 maxNumberOfObjs,
                 sizeof( T ),
                 sizeof( _objX ) ) ;
         SDB_ASSERT( ( SDB_OK == rc ), "Invalid arguments" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   // get an object's address by its index
   template < class T >
   OSS_INLINE T * _utilSegmentPool< T >::getObjPtrByIndex
   (
      const UTIL_OBJIDX idx
   )
   {
      T * pObj     = NULL ;
      _objX *pObjX = _getObjXByIndex( idx ) ;
      if ( pObjX )
      {
         pObj = ( T * )&( pObjX->_obj ) ;
      }
      return pObj ;
   }

   // acquire a free object, upon successfully return,
   //   idx -- the index of the object
   //   pT  -- the address of the object
   template < class T >
   OSS_INLINE INT32 _utilSegmentPool< T >::acquire( UTIL_OBJIDX & idx, T * &pT )
   {
      INT32   rc                = SDB_OK ;
      BOOLEAN bLatched          = FALSE ;
      _objX * pObjX             = NULL ;
      const UINT32 packedPoolID = _GET_PACKED_POOLID( _poolId ) ;

      //         *       .
      // -       -       -       -      -
      //         ^
      //         |  ==>
      //         begin
      // when _begin is equal to '_numOfObjs - 1',
      // means lack of free objects and we will
      // add a new segment and expand the _list
      _latch.get() ;
      bLatched = TRUE ;
      if ( _isInitialized )
      {
         if ( !_list || _needExpand() ) // need to expand
         {
            if ( _isUpToLimit() )
            {
               // exceed limitation
               ++_oolTimes ;
               rc = SDB_OSS_UP_TO_LIMIT ;
               PD_LOG( PDINFO,
                       "Exceed resource limitation "
                       "when attempt to expand: %d"OSS_NEWLINE
                       "  PoolID        : %u"OSS_NEWLINE
                       "  Delta         : %u"OSS_NEWLINE
                       "  MaxNumOfObjs  : %u"OSS_NEWLINE
                       "  NumOfObjs     : %u"OSS_NEWLINE
                       "  BeginPos      : %u"OSS_NEWLINE
                       "  ObjT Size     : %u"OSS_NEWLINE
                       "  ObjX Size     : %u"OSS_NEWLINE,
                       rc,
                       _poolId,
                       _delta,
                       _maxNumOfObjs,
                       _numOfObjs,
                       _begin,
                       sizeof( T ),
                       sizeof( _objX ) ) ;
               goto error ;
            }
            else if ( _pHandler &&
                      !_pHandler->canAllocSegment( (UINT64)_delta *
                                                   sizeof( _objX ) ) )
            {
               ++_oolTimes ;
               rc = SDB_OSS_UP_TO_LIMIT ;
               PD_LOG( PDINFO, "Can't alloc segment[%u * %u(ObjT Size: %u)] "
                       "by handler", _delta, sizeof( _objX ), sizeof( T ) ) ;
               goto error ;
            }

            rc = _expandList() ;
            if ( rc )
            {
               ++_oomTimes ;
               PD_LOG( PDWARNING,
                       "Failed to expand : %d"OSS_NEWLINE
                       "  PoolID         : %u"OSS_NEWLINE
                       "  Delta          : %u"OSS_NEWLINE
                       "  MaxNumOfObjs   : %u"OSS_NEWLINE
                       "  NumOfObjs      : %u"OSS_NEWLINE
                       "  BeginPos       : %u"OSS_NEWLINE
                       "  ObjT Size      : %u"OSS_NEWLINE
                       "  ObjX Size      : %u"OSS_NEWLINE,
                       rc,
                       _poolId,
                       _delta,
                       _maxNumOfObjs,
                       _numOfObjs,
                       _begin,
                       sizeof( T ),
                       sizeof( _objX ) ) ;
               goto error ;
            }

            if ( _pHandler )
            {
               _pHandler->onAllocSegment( (UINT64)_delta * sizeof( _objX ) ) ;
            }
         }

         pObjX = _getObjXByIndex( _list[ _begin ] ) ;

         // sanity check
         if (    ( NULL != pObjX )
              && ( packedPoolID == ( pObjX->_index & _SEGMENT_OBJ_POOL_MASK ) )
              && ( _SEGMENT_OBJ_EYE_CATCHER   == pObjX->_eyeCatcher )
              && ( _SEGMENT_OBJ_FLAG_RELEASED == pObjX->_flag ) )
         {
            // mark the _objX flag
            pObjX->_flag = _SEGMENT_OBJ_FLAG_ACQUIRED ;
         }
         else
         {
            idx = UTIL_INVALID_OBJ_INDEX ;
            pT  = NULL ;
            rc  = SDB_SYS ;

            if ( pObjX )
            {
               PD_LOG( PDSEVERE,
                       "Sanity check failed: "OSS_NEWLINE
                       "  PoolID    : %u"OSS_NEWLINE
                       "  Obj Idx   : %u"OSS_NEWLINE
                       "  EyeCatcher: %x"OSS_NEWLINE
                       "  Flag      : %x"OSS_NEWLINE
                       "  ObjX addr : %p"OSS_NEWLINE
                       "  ObjT addr : %p"OSS_NEWLINE
                       "  ObjT Size : %u"OSS_NEWLINE
                       "  ObjX Size : %u"OSS_NEWLINE,
                       _poolId,
                       _list[ _begin ],
                       pObjX->_eyeCatcher,
                       pObjX->_flag,
                       pObjX,
                       &( pObjX->_obj ),
                       sizeof( T ),
                       sizeof( _objX ) ) ;
            }
            else
            {
               PD_LOG( PDSEVERE,
                       "Sanity check failed: "OSS_NEWLINE
                       "  PoolID    : %u"OSS_NEWLINE
                       "  Obj Idx   : %u"OSS_NEWLINE
                       "  ObjX addr : %p"OSS_NEWLINE
                       "  ObjT Size : %u"OSS_NEWLINE
                       "  ObjX Size : %u"OSS_NEWLINE,
                       _poolId,
                       _list[ _begin ],
                       pObjX,
                       sizeof( T ),
                       sizeof( _objX ) ) ;

            }
#ifdef _DEBUG
            SDB_ASSERT ( ( SDB_OK == rc ), "Sanity check failed !") ;
#endif
            goto error ;
         }

         // get the obj index( to the segment, the object array )
         idx = (( _list[ _begin ] & _SEGMENT_OBJ_INDEX_MASK ) | packedPoolID ) ;

         // mark this slot is empty with invalid index number, -1
         // _list[ _begin ] = UTIL_INVALID_OBJ_INDEX ;

         // advance to next position
         _begin ++ ;
         ++_acquireTimes ;

         // return the object address
         pT = ( T * )&( pObjX->_obj ) ;
      }
      else
      {
         SDB_ASSERT( ( _isInitialized && _list ),
                     "_utilSegmentPool has to be initialized." ) ;
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

   // acquire a free object, upon successfully return,
   //   pT  -- the address of the object
   template < class T >
   OSS_INLINE INT32 _utilSegmentPool< T >::acquire( T * &pT )
   {
      UTIL_OBJIDX idx = UTIL_INVALID_OBJ_INDEX ;
      return acquire( idx, pT ) ;
   }

   // release/return an object to the segments by its address
   //   pT -- address of the object
   template < class T >
   OSS_INLINE INT32 _utilSegmentPool< T >::release( const T * pT )
   {
      INT32 rc   = SDB_OK ;
      UTIL_OBJIDX idx = getIndexByAddr( pT ) ;
      if ( IS_VALID_SEG_OBJ_INDEX( idx ) )
      {
         rc = release( idx ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
#ifdef _DEBUG
         SDB_ASSERT( ( SDB_OK == rc ), "Invalid object address" ) ;
#endif
      }
      return rc ;
   }

   // release/return an object to the segments by its index
   //   idx -- the object index
   template < class T >
   OSS_INLINE INT32 _utilSegmentPool< T >::release( const UTIL_OBJIDX idx )
   {
      INT32   rc           = SDB_OK ;
      BOOLEAN bLatched     = FALSE ;
      _objX * pObjX        = NULL ;
      const UTIL_OBJIDX packedPoolID = _GET_PACKED_POOLID( _poolId ) ;

      if ( !( ( UTIL_INVALID_OBJ_INDEX != idx ) &&
              ( ( idx & _SEGMENT_OBJ_POOL_MASK ) == packedPoolID ) ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _latch.get() ;
      bLatched = TRUE ;
      if ( _isInitialized && ( NULL != _list ) )
      {
         if ( _begin > 0 && 
             ( idx & _SEGMENT_OBJ_INDEX_MASK ) < _numOfObjs )
         {
            // get the _objX address by the index
            pObjX = _getObjXByIndex( idx ) ;

            // sanity check
            if (( NULL != pObjX ) &&
                ((pObjX->_index & _SEGMENT_OBJ_POOL_MASK) == packedPoolID) &&
                ( _SEGMENT_OBJ_EYE_CATCHER   == pObjX->_eyeCatcher ) &&
                ( _SEGMENT_OBJ_FLAG_ACQUIRED == pObjX->_flag ) )
            {
               // mark the _objX flag
               pObjX->_flag = _SEGMENT_OBJ_FLAG_RELEASED ;
            }
            else
            {
               rc = SDB_SYS ;

               if ( NULL != pObjX  )
               {
                  PD_LOG( PDSEVERE,
                          "Sanity check failed: "OSS_NEWLINE
                          "  PoolID    : %u"OSS_NEWLINE
                          "  Obj Idx   : %u"OSS_NEWLINE
                          "  EyeCatcher: %x"OSS_NEWLINE
                          "  Flag      : %x"OSS_NEWLINE
                          "  ObjX addr : %p"OSS_NEWLINE
                          "  ObjT addr : %p"OSS_NEWLINE
                          "  ObjT Size : %u"OSS_NEWLINE
                          "  ObjX Size : %u"OSS_NEWLINE,
                          _poolId,
                          idx,
                          pObjX->_eyeCatcher,
                          pObjX->_flag,
                          pObjX,
                          &( pObjX->_obj ),
                          sizeof( T ),
                          sizeof( _objX ) ) ;
               }
               else
               {
                  PD_LOG( PDSEVERE,
                          "Sanity check failed: "OSS_NEWLINE
                          "  PoolID    : %u"OSS_NEWLINE
                          "  Obj Idx   : %u"OSS_NEWLINE
                          "  ObjX addr : %p"OSS_NEWLINE
                          "  ObjT Size : %u"OSS_NEWLINE
                          "  ObjX Size : %u"OSS_NEWLINE,
                          _poolId,
                          idx,
                          pObjX,
                          sizeof( T ),
                          sizeof( _objX ) ) ;
               }
#ifdef _DEBUG
               SDB_ASSERT ( ( SDB_OK == rc ), "Sanity check failed !");
#endif
               goto error ;
            }

            // fill in current slot with the obj index( to the segment)
            _list[ --_begin ] = ( idx & _SEGMENT_OBJ_INDEX_MASK ) ;
            ++_releaseTimes ;
         }
         else
         {
            // error, probably the caller or someone else,
            // returned/released a non-acquired object
            rc = SDB_SYS ;
#ifdef _DEBUG
            SDB_ASSERT( ( SDB_OK == rc ),
                        "Can't release object more than acquired" ) ;
#endif
            goto error ;
         }
      }
      else
      {
#ifdef _DEBUG
         SDB_ASSERT( ( _isInitialized && _list ),
                     "_utilSegmentPool has to be initialized." ) ;
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

   template < class T >
   class _utilSegmentManager : public SDBObject
   {
      private :
         // acquire operation proceed in round robin
         ossAtomic32 _round ;
         CHAR        _name[ UTIL_SEGMENT_NAME_LEN + 1 ] ;
         UINT32      _poolNum ;

         /// stat info
         UINT64      _acquireTimes ;
         UINT64      _releaseTimes ;
         UINT64      _oomTimes ;
         UINT64      _oolTimes ;
         UINT64      _shrinkSize ;

         _utilSegmentPool< T > _pool[ _SEGMENT_MGR_MAX_POOLS ] ;

      private :
         // internal / helper function to get the poolId an object belongs to
         OSS_INLINE UINT32 _getPoolIdByAddr( const T * pT )
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
            _shrinkSize = 0 ;
         }

      public  :
         _utilSegmentManager( const CHAR *pName = NULL ) : _round( 0 )
         {
            ossMemset( _name, 0, sizeof( _name ) ) ;
            if ( pName )
            {
               ossStrncpy( _name, pName, UTIL_SEGMENT_NAME_LEN ) ;
            }
            _poolNum = 0 ;

            clearStat_i() ;
         }
         _utilSegmentManager( UINT64 numberOfObjs,
                              UINT64 maxNumberOfObjs,
                              const CHAR *pName = NULL )
         {
            ossMemset( _name, 0, sizeof( _name ) ) ;
            if ( pName )
            {
               ossStrncpy( _name, pName, UTIL_SEGMENT_NAME_LEN ) ;
            }
            _poolNum = 0 ;

            clearStat_i() ;

            init( numberOfObjs, maxNumberOfObjs ) ;
         }

         ~_utilSegmentManager() { fini(); }

         INT32 init( UINT64 numberOfObjs,
                     UINT64 maxNumberOfObjs,
                     UINT8 poolNum = _SEGMENT_MGR_MAX_POOLS,
                     utilSegmentHandler *pHandler = NULL )
         {
            INT32 rc = SDB_OK ;
            UINT64 poolMaxObjs = 0 ;
            _poolNum = ossNextPowerOf2( poolNum, NULL ) ;

            if ( _poolNum > _SEGMENT_MGR_MAX_POOLS )
            {
               _poolNum = _SEGMENT_MGR_MAX_POOLS ;
            }

            poolMaxObjs = maxNumberOfObjs / _poolNum ;
            if ( poolMaxObjs > _SEGMENT_OBJ_MAX_NUM )
            {
               poolMaxObjs = _SEGMENT_OBJ_MAX_NUM ;
            }

            for ( UINT32 i = 0 ; i < _poolNum ; i++ )
            {
               rc = _pool[ i ].init( i,
                                     numberOfObjs    / _poolNum,
                                     poolMaxObjs,
                                     pHandler ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR,
                          "Initialzation failed, pool:%u, rc:%d", i, rc ) ;
                  break ;
               }
            }
            return rc ;
         }

         void setMaxObjects( UINT64 maxNumberOfObjs )
         {
            UINT64 poolMaxObjs = maxNumberOfObjs / _poolNum ;
            if ( poolMaxObjs > _SEGMENT_OBJ_MAX_NUM )
            {
               poolMaxObjs = _SEGMENT_OBJ_MAX_NUM ;
            }

            for ( UINT32 i = 0 ; i < _poolNum ; i++ )
            {
               _pool[ i ].setMaxObjects( poolMaxObjs ) ;
            }
         }

         void fini()
         {
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               _pool[ i ].fini() ;
            }
         }

         UINT32 getNumOfObjAllocated()
         {
            UINT32 objs = 0 ;
            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               objs += _pool[ i ].getNumOfObjAllocated() ;
            }
            return objs ;
         }

         OSS_INLINE T * getObjPtrByIndex( const UTIL_OBJIDX idx )
         {
            if ( ! IS_VALID_SEG_OBJ_INDEX( idx ) )
            {
               return NULL ;
            }
            return _pool[ _GET_UNPACKED_POOLID( idx ) ].getObjPtrByIndex( idx );
         }

         OSS_INLINE UTIL_OBJIDX getIndexByAddr ( const T * pT )
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

         OSS_INLINE BOOLEAN isLowerThanHWM( const T * pT )
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

         OSS_INLINE INT32 acquire( UTIL_OBJIDX & idx,  T * &pT )
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
               rc = _pool[ pool ].acquire( idx, pT ) ;
               if ( SDB_OK == rc )
               {
                  break ;
               }
            } while ( ( SDB_OSS_UP_TO_LIMIT == rc ) &&
                      ( retryCount < _poolNum ) ) ;
            return rc ;
         }

         OSS_INLINE INT32 acquire( T * &pT )
         {
            UTIL_OBJIDX idx = UTIL_INVALID_OBJ_INDEX ;
            return acquire( idx, pT ) ;
         }

         OSS_INLINE INT32 release( const UTIL_OBJIDX idx )
         {
            return _pool[ _GET_UNPACKED_POOLID( idx ) ].release( idx ) ;
         }

         OSS_INLINE INT32 release( const T * pT )
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

         OSS_INLINE INT32 shrink( UINT32 freeSegToKeep = 1,
                                  UINT64 *pFreedSize = NULL )
         {
            // REVISIT :
            // proper scheduling algorithm to be implemented
            INT32 rc = SDB_OK ;

            if ( pFreedSize )
            {
               *pFreedSize = 0 ;
            }

            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               rc = _pool[ i ].shrink( freeSegToKeep, pFreedSize ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to shrink pool:%u, rc:%d", i, rc ) ;
                  break ;
               }
            }

            return rc ;
         }

         OSS_INLINE UINT32 dump( CHAR *pBuff, UINT32 buffLen,
                                 UINT64 *pAcquireTimes = NULL,
                                 UINT64 *pReleaseTimes = NULL,
                                 UINT64 *pOOMTimes = NULL,
                                 UINT64 *pOOLTimes = NULL,
                                 UINT64 *pShrinkSize = NULL)
         {
            UINT32 len = 0 ;

            UINT64 acquireTimes = 0 ;
            UINT64 releaseTimes = 0 ;
            UINT64 oomTimes = 0 ;
            UINT64 oolTimes = 0 ;
            UINT64 shrinkSize = 0 ;

            len = ossSnprintf( pBuff, buffLen,
                               OSS_NEWLINE
                               "---- Segment Name( %s ) ----"OSS_NEWLINE
                               "      Pool Num : %u"OSS_NEWLINE
                               "    Total Size : %llu"OSS_NEWLINE,
                               _name,
                               _poolNum,
                               (UINT64)getNumOfObjAllocated() *
                               sizeof( typename _utilSegmentPool< T >::_objX ) ) ;

            for ( UINT32 i = 0; i < _poolNum ; i++ )
            {
               len += _pool[ i ].dump( pBuff + len, buffLen - len,
                                       &acquireTimes,
                                       &releaseTimes,
                                       &oomTimes,
                                       &oolTimes,
                                       &shrinkSize ) ;
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
                                "Segment Stat"OSS_NEWLINE
                                " Acquire Times : %llu (Inc: %lld )"OSS_NEWLINE
                                " Release Times : %llu (Inc: %lld )"OSS_NEWLINE
                                "     OOM Times : %llu (Inc: %lld )"OSS_NEWLINE
                                "     OOL Times : %llu (Inc: %lld )"OSS_NEWLINE
                                "   Shrink Size : %llu (Inc: %lld )"OSS_NEWLINE,
                                acquireTimes,
                                acquireTimes - _acquireTimes,
                                releaseTimes,
                                releaseTimes - _releaseTimes,
                                oomTimes,
                                oomTimes - _oomTimes,
                                oolTimes,
                                oolTimes - _oolTimes,
                                shrinkSize,
                                shrinkSize - _shrinkSize ) ;

            _acquireTimes = acquireTimes ;
            _releaseTimes = releaseTimes ;
            _oomTimes = oomTimes ;
            _oolTimes = oolTimes ;
            _shrinkSize = shrinkSize ;

            return len ;
         }

#ifdef _DEBUG
         _utilSegmentPool< T > * getPoolHandle( const UINT32 pool )
         {
            return &( _pool[ pool % _poolNum ] ) ;
         }
#endif
   } ;

} // namespace engine

#endif // UTIL_SEGMENT_HPP__

