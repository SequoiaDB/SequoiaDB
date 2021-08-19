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

   Source File Name = ixmExtent.hpp

   Descriptive Name = Index Management Extent Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index extent and its methods. The B Tree Insert/Delete/Update methods are
   also defined in this file.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMEXTENT_HPP_
#define IXMEXTENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "ixmKey.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageBase.hpp"
#include "dmsPageMap.hpp"
#include "rtnPredicate.hpp"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{
   class _dmsStorageIndex ;

   /*
      _ixmKeyNode define
   */
   class _ixmKeyNode : public SDBObject
   {
   public :
      dmsExtentID _left ;
      dmsRecordID _rid ;
      UINT16      _keyOffset ;
      UINT16      _pad ;

      UINT16 keyDataOffset () const
      {
         return _keyOffset ;
      }
      void setKeyDataOffset ( UINT16 keyoff )
      {
         _keyOffset = keyoff ;
      }
      void setUnused()
      {
         _rid._offset |= 1 ;
      }
      void setUsed()
      {
         _rid._offset &= ~1 ;
      }
      BOOLEAN isUnused() const
      {
         return _rid._offset & 1 ;
      }
      BOOLEAN isUsed() const
      {
         return !isUnused() ;
      }
   } ;
   typedef class _ixmKeyNode ixmKeyNode ;

   /*
      IXM EXTENT HEAD EYE CATCHER DEFINE
   */
   #define IXM_EXTENT_EYECATCHER0      'I'
   #define IXM_EXTENT_EYECATCHER1      'E'

   /*
      IXM EXTENT HEAD VERSION DEFINE
   */
   #define IXM_EXTENT_VERSION_V0       0
   #define IXM_EXTENT_CURRENT_V        IXM_EXTENT_VERSION_V0

   /*
      _ixmExtentHead define
   */
   struct _ixmExtentHead : public SDBObject
   {
      // eye catcher, must be 'IE'
      CHAR        _eyeCatcher [2] ;
      // total number of keyNode in the extent
      UINT16      _totalKeyNodeNum ;
      // 1 to 4096, the collectionID for this index
      UINT16      _mbID ;
      // extent flag, including dms extent flag plus index extent flag
      // dms flag uses low 4 bits, index flag uses high 4 bits
      CHAR        _flag ;
      // current extent version
      CHAR        _version ;
      // the parent extent id
      dmsExtentID _parentExtentID ;
      // beginning of free space, it's starting from end of extent
      ixmOffset   _beginFreeOffset ;
      // current free space
      UINT16      _totalFreeSize ;
      // right pointer for the B+ tree
      // DMS_INVALID_EXTENT for leaf page
      dmsExtentID _right ;
   } ;
   typedef struct _ixmExtentHead ixmExtentHead ;

   /*
      _ixmExtent define
   */
   class _ixmExtent : public SDBObject
   {
   protected:
      const ixmExtentHead  *_extentHead ;
      dmsExtentID          _me ;
      _dmsStorageIndex     *_pIndexSu ;
      dmsPageMap           *_pPageMap ;
      INT32                _pageSize ;

      dmsExtRW             _extRW ;

      // reorganize the extent
      INT32 _reorg (const Ordering &order, UINT16 &newPos) ;
      INT32 _reorg (const Ordering &order) ;
      INT32 _alloc ( INT32 requestSpace, UINT16 &beginOffset ) ;

      INT32 _splitPos ( UINT16 pos, UINT16 &splitPos ) const ;

      // this function physically copy key and rid into extent page
      INT32 _basicInsert ( UINT16 &pos, const dmsRecordID &rid,
                           const ixmKey &key, const Ordering &order ) ;

      // this function is called when the page is too small for the new key, it
      // will split the page and do insert
      INT32 _split ( UINT16 pos, const dmsRecordID &rid,
                     const ixmKey &key, const Ordering &order,
                     const dmsExtentID lchild, const dmsExtentID rchild,
                     ixmIndexCB *indexCB ) ;

      enum _ixmExtentValidateLevel
      {
         NONE = 0,
         MIN,
         MID,
         MAX
      } ;
      INT32 _validate ( _ixmExtentValidateLevel level,
                        const Ordering &order ) const ;

      INT32 _validate( ixmIndexCB *indexCB, dmsExtentID parent ) ;

      INT32 _pushBack ( const dmsRecordID &rid, const ixmKey &key,
                        const Ordering &order, const dmsExtentID left,
                        UINT16 idxState = IXM_INDEX_FLAG_NORMAL ) ;

      INT32 _fixParentPtrs ( UINT16 startPos, UINT16 stopPos ) const ;

      void  _assignRight ( const dmsExtentID right ) ;

      INT32 _truncate ( UINT16 totalNodes,UINT16 &newPos,const Ordering &order);
      INT32 _insert ( const dmsRecordID &rid, const ixmKey &key,
                      const Ordering &order, BOOLEAN dupAllowed,
                      dmsExtentID lchild, dmsExtentID rchild,
                      ixmIndexCB *indexCB,
                      utilWriteResult *pResult = NULL ) ;
      INT32 _delKeyAtPos ( UINT16 pos ) ;
      INT32 _delKeyAtPos ( UINT16 pos, const Ordering &order,
                           ixmIndexCB *indexCB ) ;
      INT32 _mayBalanceWithNeighbors ( const Ordering &order,
                                       ixmIndexCB *indexCB,
                                       BOOLEAN &result ) ;
      INT32 _delExtent ( ixmIndexCB *indexCB ) ;
      INT32 _findChildExtent ( dmsExtentID childExtent, UINT16 &pos ) const ;
      INT32 _deleteInternalKey ( UINT16 pos, const Ordering &order,
                                 ixmIndexCB *indexCB ) ;
      INT32 _setInternalKey ( UINT16 pos, const dmsRecordID &rid,
                              const ixmKey &key,
                              const Ordering &order, dmsExtentID lchild,
                              dmsExtentID rchild, ixmIndexCB *indexCB ) ;
      INT32 _doMergeChildren ( UINT16 pos, const Ordering &order,
                               ixmIndexCB *indexCB, BOOLEAN &result ) ;
      INT32 _locate ( const ixmKey &key, const dmsRecordID &rid,
                      const Ordering &order, ixmRecordID &indexrid,
                      BOOLEAN &found, INT32 direction,
                      const ixmIndexCB *indexCB ) const ;

      INT32 _keyFind ( UINT16 low, UINT16 high, const BSONObj &prevKey,
                       INT32 keepFieldsNum, BOOLEAN skipToNext,
                       const VEC_ELE_CMP &matchEle,
                       const VEC_BOOLEAN &matchInclusive,
                       const Ordering &o,
                       INT32 direction,
                       ixmRecordID &bestIxmRID,
                       dmsExtentID &resultExtent, _pmdEDUCB *cb ) const ;
   public:
      // currentKey is the key from current disk location that trying to
      // be matched
      // prevKey is the key examed from previous run
      // keepFieldsNum is the number of fields from prevKey that should match
      // the
      // currentKey (for example if the prevKey is {c1:1, c2:1}, and
      // keepFieldsNum
      // = 1, that means we want to match c1:1 key for the current location.
      // Depends on if we have skipToNext set, if we do that means we want to
      // skip
      // c1:1 and match whatever the next (for example c1:1.1); otherwise we
      // want
      // to continue match the elements from matchEle )
      // Make this member function static so that other class can reuse the
      // key compare outside of the class or without an instance
      static INT32 _keyCmp ( const BSONObj &currentKey, const BSONObj &prevKey,
                             INT32 keepFieldsNum, BOOLEAN skipToNext,
                             const VEC_ELE_CMP &matchEle,
                             const VEC_BOOLEAN &matchInclusive,
                             const Ordering &o, INT32 direction ) ;

   public:
      // create new extent id without parent
      _ixmExtent ( dmsExtentID extentID, UINT16 mbID,
                   _dmsStorageIndex *pIndexSu );

      _ixmExtent ( dmsExtentID extentID,
                   _dmsStorageIndex *pIndexSu ) ;

      BOOLEAN verify() const ;
      OSS_INLINE UINT16 getNumKeyNode () const
      {
         return _extentHead->_totalKeyNodeNum ;
      }
      OSS_INLINE const ixmKeyNode *getKeyNode ( UINT16 i ) const
      {
         if ( i>_extentHead->_totalKeyNodeNum )
         {
            return NULL ;
         }
         return (const ixmKeyNode*)(((const CHAR*)_extentHead) +
                                    sizeof(ixmExtentHead) +
                                    sizeof(ixmKeyNode)*i) ;
      }
      OSS_INLINE ixmKeyNode *writeKeyNode ( UINT16 i )
      {
         if ( i > _extentHead->_totalKeyNodeNum )
         {
            return NULL ;
         }
         CHAR *pHeader = _extRW.writePtr( 0, _pageSize ) ;
         return (ixmKeyNode*)( pHeader + sizeof(ixmExtentHead) +
                               sizeof(ixmKeyNode) * i ) ;
      }
      OSS_INLINE const CHAR *getKeyData ( UINT16 i ) const
      {
         if ( i>=_extentHead->_totalKeyNodeNum )
         {
            return NULL ;
         }
         return (const CHAR*)_extentHead+getKeyNode(i)->_keyOffset ;
      }
      OSS_INLINE CHAR *writeKeyData( UINT16 i )
      {
         if ( i >= _extentHead->_totalKeyNodeNum )
         {
            return NULL ;
         }
         CHAR *pHeader = _extRW.writePtr( 0, _pageSize ) ;
         return pHeader + getKeyNode(i)->_keyOffset ;
      }
      OSS_INLINE UINT16 getFreeSize() const
      {
         return _extentHead->_totalFreeSize ;
      }
      OSS_INLINE UINT16 getMBID() const
      {
         return _extentHead->_mbID ;
      }
      OSS_INLINE UINT16 getTotalKeySize() const
      {
         return (UINT16)(_pageSize-1) - _extentHead->_totalFreeSize -
                (sizeof(ixmExtentHead) +
                 _extentHead->_totalKeyNodeNum * sizeof(ixmKeyNode)) ;
      }
      OSS_INLINE BOOLEAN isRoot() const
      {
         return DMS_INVALID_EXTENT == getParent() ;
      }
      // get the extent id for child
      dmsExtentID getChildExtentID ( UINT16 i ) const
      {
         if ( i>_extentHead->_totalKeyNodeNum )
         {
            return DMS_INVALID_EXTENT ;
         }
         return (i==_extentHead->_totalKeyNodeNum)?(_extentHead->_right):
                    (getKeyNode(i)->_left) ;
      }
      dmsRecordID getRID ( UINT16 i ) const
      {
         if ( i>=_extentHead->_totalKeyNodeNum )
         {
            return dmsRecordID() ;
         }
         const ixmKeyNode *kn = getKeyNode(i) ;
         if ( kn->isUnused() )
         {
            return dmsRecordID() ;
         }
         return kn->_rid ;
      }
      OSS_INLINE dmsExtentID getParent () const
      {
         dmsExtentID parentID = DMS_INVALID_EXTENT ;
         if ( !_pPageMap->findItem( _me, &parentID ) )
         {
            parentID = _extentHead->_parentExtentID ;
         }
         return parentID ;
      }
      OSS_INLINE void setParent ( dmsExtentID extentID,
                                  BOOLEAN rmItem = TRUE )
      {
         ixmExtentHead *pHead = _extRW.writePtr<ixmExtentHead>() ;
         pHead->_parentExtentID = extentID ;

         if ( rmItem )
         {
            _pPageMap->rmItem( _me ) ;
         }
      }
      void setChildExtentID ( UINT16 i, dmsExtentID extentID ) ;

      void setCompact()
      {
         CHAR *pHead = _extRW.writePtr( 0, _pageSize ) ;
         pHead[ _pageSize - 1 ] = 1 ;
      }
      void unsetCompact()
      {
         CHAR *pHead = _extRW.writePtr( 0, _pageSize ) ;
         pHead[ _pageSize - 1 ] = 0 ;
      }
      BOOLEAN isCompact() const
      {
         return 1 == ((const CHAR*)_extentHead)[ _pageSize - 1 ] ;
      }

      // find for a given key + rid
      INT32 find ( const ixmIndexCB *indexCB,
                   const ixmKey &key,
                   const dmsRecordID &rid,
                   const Ordering &order,
                   UINT16 &pos,
                   INT32 &keyFoundPos,
                   BOOLEAN &sameFound ) const ;

      INT32 locate ( const BSONObj &key, const dmsRecordID &rid,
                     const Ordering &order, ixmRecordID &indexrid,
                     BOOLEAN &found, INT32 direction,
                     const ixmIndexCB *indexCB ) const ;

      // actual insert into the ixmExtent at given pos
      INT32 insertHere ( UINT16 pos, const dmsRecordID &rid, const ixmKey &key,
                         const Ordering &order, dmsExtentID lchild,
                         dmsExtentID rchild,
                         ixmIndexCB *indexCB ) ;

      INT32 unindex ( const ixmKey &key, const dmsRecordID &rid,
                      const Ordering &order, ixmIndexCB *indexCB,
                      BOOLEAN &result ) ;
      INT32 advance ( ixmRecordID &keyRID, INT32 direction ) const ;
      INT32 exists ( const ixmKey &key, const Ordering &order,
                     const ixmIndexCB *indexCB, BOOLEAN &result ) const ;
      dmsExtentID getRoot() const ;
      INT32 findSingle ( const ixmKey &key, const Ordering &order,
                         dmsRecordID &rid, ixmIndexCB *indexCB ) const ;
      // syncronized insert, insert a key and rid into index
      INT32 insert ( const ixmKey &key, const dmsRecordID &rid,
                     const Ordering &order, BOOLEAN dupAllowed,
                     ixmIndexCB *indexCB,
                     utilWriteResult *pResult = NULL ) ;
      // wipe out everything in the extent and all child extents
      void truncate ( ixmIndexCB *indexCB, dmsExtentID parent, BOOLEAN &valid) ;
      // get the total number of elements in the index node and all children
      UINT64 count() const ;

      BOOLEAN isStillValid( UINT16 mbID ) const ;

      /******************************************************/
      /*      index cursor usage functions                  */
      /******************************************************/
      // This function locate the key by
      // 1) check if the key is smaller or greater than first or last key in
      // forward and backward search
      // 2) if so it will jump into the child if exist
      // 3) check if the key is greater or smaller than the last or first key in
      // forward and backward search
      // 4) if so it will jump into the child if exist
      // 5) check if the key is in the page, if the key got child it will jump
      // into it
      // the rid may or may not be changed by the following condition
      // A) forward scan
      //   A.1) if first key is greater than prevKey, rid = first key rid and
      //   scan most left pointer
      //   A.2) if last key is smaller than prevKey, rid unchange and scan most
      //   right pointer
      //   A.3) in other condition,do binary search using keyFind and scan child
      // B) backward scan
      //   B.1) if last key is smaller than prevKey, rid = last key rid and scan
      //   most right pointer
      //   B.2) if first key is greater than prevKey, rid unchange and scan most
      //   left pointer
      //   B.3) in other condition,do binary search using keyFind and scan child
      INT32 keyLocate ( ixmRecordID &rid, const BSONObj &prevKey,
                        INT32 keepFieldsNum, BOOLEAN skipToNext,
                        const VEC_ELE_CMP &matchEle,
                        const VEC_BOOLEAN &matchInclusive,
                        const Ordering &o, INT32 direction,
                        _pmdEDUCB *cb ) const ;
      INT32 keyAdvance ( ixmRecordID &rid, const BSONObj &prevKey,
                         INT32 keepFieldsNum, BOOLEAN skipToNext,
                         const VEC_ELE_CMP &matchEle,
                         const VEC_BOOLEAN &matchInclusive,
                         const Ordering &o, INT32 direction,
                         _pmdEDUCB *cb ) const ;
      INT32 dumpIndexExtentIntoLog() const ;
   } ;
   typedef class _ixmExtent ixmExtent ;
}

#endif //IXMEXTENT_HPP_

