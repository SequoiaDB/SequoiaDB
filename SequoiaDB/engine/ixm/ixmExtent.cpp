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

   Source File Name = ixmExtent.cpp

   Descriptive Name = Index Manager Extent

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for index
   extent implmenetation. This include B tree insert/update/delete.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ixmExtent.hpp"
#include "dmsStorageIndex.hpp"
#include "pd.hpp"
#include "monCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsDump.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"

using namespace bson ;

namespace engine
{

   // create new extent id without parent
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT2, "_ixmExtent::_ixmExtent" )
   _ixmExtent::_ixmExtent ( dmsExtentID extentID, UINT16 mbID,
                            dmsStorageIndex *pIndexSu )
   {
      SDB_ASSERT ( pIndexSu, "index su can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__IXMEXT2 ) ;
      ixmExtentHead *pHeader = NULL ;
      _extRW = pIndexSu->extent2RW( extentID, mbID ) ;
      _pIndexSu = pIndexSu ;
      _pPageMap = _pIndexSu->getPageMap( mbID ) ;
      _pageSize = _pIndexSu->pageSize() ;

      pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      _extentHead = (const ixmExtentHead*)pHeader ;
      SDB_ASSERT(_extentHead, "extent can't be NULL" ) ;
      _me = extentID ;
      pHeader->_eyeCatcher [0] = IXM_EXTENT_EYECATCHER0 ;
      pHeader->_eyeCatcher [1] = IXM_EXTENT_EYECATCHER1 ;
      pHeader->_totalKeyNodeNum = 0 ;
      pHeader->_mbID = mbID ;
      // not change flag here
      pHeader->_version = IXM_EXTENT_CURRENT_V ;
      pHeader->_parentExtentID = DMS_INVALID_EXTENT ;
      // set to 65535, indicating end of the page
      pHeader->_beginFreeOffset = _pageSize-1 ;
      pHeader->_right = DMS_INVALID_EXTENT ;
      pHeader->_totalFreeSize = pHeader->_beginFreeOffset -
                        (sizeof(ixmExtentHead) +
                        (pHeader->_totalKeyNodeNum*sizeof(ixmKeyNode))) ;
      pIndexSu->addStatFreeSpace( mbID, pHeader->_totalFreeSize ) ;

      PD_TRACE_EXIT ( SDB__IXMEXT2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT4, "_ixmExtent::_ixmExtent" )
   _ixmExtent::_ixmExtent ( dmsExtentID extentID,
                            dmsStorageIndex *pIndexSu )
   {
      SDB_ASSERT ( pIndexSu, "index su can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__IXMEXT4 );
      _extRW = pIndexSu->extent2RW( extentID, -1 ) ;
      _pIndexSu = pIndexSu ;
      _pageSize = _pIndexSu->pageSize() ;
      _me = extentID ;
      _extentHead = _extRW.readPtr<ixmExtentHead>( 0, _pageSize ) ;
      /// set collection id
      _extRW.setCollectionID( _extentHead->_mbID ) ;
      _pPageMap = _pIndexSu->getPageMap( _extentHead->_mbID ) ;
      PD_TRACE_EXIT ( SDB__IXMEXT4 );
      SDB_ASSERT(_extentHead, "extent can't be NULL" ) ;
   }

   BOOLEAN _ixmExtent::verify () const
   {
      if ( !_extentHead )
      {
         PD_LOG ( PDERROR, "NULL index extent" ) ;
         return FALSE ;
      }
      if ( _extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           _extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         PD_LOG ( PDERROR, "Invalid index eye-catcher" ) ;
         return FALSE ;
      }
      if ( !(_extentHead->_flag & DMS_MB_FLAG_USED) )
      {
         PD_LOG ( PDERROR, "Unused extent" ) ;
         return FALSE ;
      }
      return TRUE ;
   }

   // Find a given key and RID, returns key position if the caller want to
   // insert a new key, and also return boolean found
   // Each index page is 65536 bytes, with 20 bytes head at beginning, 1 byte
   // reserve at end, we have 65515 bytes. Each ixmKeyNode is 16 bytes, and
   // minimal compact key size is 1 byte, so each index page can maximumly store
   // 65515/17=3853 records, so with binary search we shouldn't spent more than
   // 12 (2^12 = 4096) compares to get the key in the worst case
   // Input:
   //    indexCB : index control block
   //    ixmKey  : index key tries to match
   //    rid     : record ID for the record in collection
   //    order   : order for the index key def
   // Output:
   //    pos     : key position if found, or the expected key position if the
   //              given key is not in the index
   //    keyFound  : whether the key exist in the page
   //    sameFound : whether the key + rid exist in the page.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_FIND, "_ixmExtent::find" )
   INT32 _ixmExtent::find ( const ixmIndexCB *indexCB,
                            const ixmKey &key,
                            const dmsRecordID &rid,
                            const Ordering &order,
                            UINT16 &pos,
                            INT32 &keyFoundPos,
                            BOOLEAN &sameFound ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_FIND ) ;

      keyFoundPos = -1 ;
      sameFound = FALSE ;

      // use binary search, start from 0 and totalKeyNodeNum-1
      INT32 low = 0 ;
      INT32 high = _extentHead->_totalKeyNodeNum-1 ;
      // get the middle pos
      INT32 middle = (low + high)/2 ;
      // loop until high>=low
      while ( low <= high )
      {
         PD_TRACE3 ( SDB__IXMEXT_FIND,
                     PD_PACK_INT ( low ),
                     PD_PACK_INT ( high ),
                     PD_PACK_INT ( middle ) ) ;
         // get the key for the middle
         const CHAR *keyData = getKeyData( middle ) ;
         // the key supposed to exist, otherwise there's some corruption happen
         if ( !keyData )
         {
            PD_LOG ( PDERROR, "Unable to locate key" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         // create ixmKey object and let it compare with the input
         ixmKey keyDisk( keyData ) ;
         INT32 result = key.woCompare ( keyDisk, order ) ;
         PD_TRACE1 ( SDB__IXMEXT_FIND, PD_PACK_INT ( result ) ) ;
         // if the result are the same, let's check whether we allows key
         // duplicate first
         if ( 0 == result )
         {
            keyFoundPos = middle ;
            const ixmKeyNode *M = getKeyNode( middle ) ;
            // let's continue compare the RID
            result = rid.compare( M->_rid ) ;
         }
         // if the compare result shows disk value is smaller, let's set high =
         // middle-1
         if ( result < 0 )
         {
            high = middle -1 ;
         }
         // if the compare result shows disk value is greater, let's set low =
         // middle+1
         else if ( result > 0 )
         {
            low = middle + 1 ;
         }
         // otherwise we have both key+rid identical
         else
         {
            // note it's not possible for an unused key hit this path, because
            // unused key got 1 in least bit, but all normal RID are 4 bytes
            // aligned, that means when result = 0, it always means we found
            // duplicate key + rid for used record
            pos = middle ;
            sameFound = TRUE ;
            goto done ;
         }
         // continue with a new middle
         middle = (low + high)/2 ;
      }
      // if still unable to find
      pos = low ;
      PD_TRACE1 ( SDB__IXMEXT_FIND, PD_PACK_USHORT(pos) ) ;
      // sanity check, this is essential even in release build, just in case
      // index corruption happened on disk
      if ( pos != _extentHead->_totalKeyNodeNum )
      {
         // make sure the requested key is NOT greater than the next key
         {
            const CHAR *keyData = getKeyData (pos) ;
            ixmKey keyDisk( keyData ) ;
            if ( key.woCompare ( keyDisk, order ) > 0 )
            {
               PD_LOG ( PDERROR, "Internal logic error, key compare wrong" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         // make sure the previous key is NOT greater than the requested key
         if ( pos > 0 )
         {
            const CHAR *keyData = getKeyData( pos-1 ) ;
            ixmKey keyDisk(keyData) ;
            if ( keyDisk.woCompare ( key, order ) > 0 )
            {
               PD_LOG ( PDERROR, "Internal logic error, key compare wrong" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_FIND, rc );
      return rc ;
   error :
      goto done ;
   }

   // syncronized insert, insert a key and rid into index
   INT32 _ixmExtent::insert ( const ixmKey &key, const dmsRecordID &rid,
                              const Ordering &order, BOOLEAN dupAllowed,
                              ixmIndexCB *indexCB,
                              utilWriteResult *pResult )
   {
      return _insert ( rid, key, order, dupAllowed, DMS_INVALID_EXTENT,
                       DMS_INVALID_EXTENT, indexCB, pResult ) ;
   }

   // This function is the wrapper for _basicInsert and _split, depends on
   // whether the current extent has sufficient space for a new record
   // If the new record is inserted into current page, it will set left pointer
   // for the new key, and right pointer for the page
   // Input:
   //    pos       : insert position
   //    rid       : data rid
   //    key       : data key
   //    order     : index key ordering
   //    lchild    : left child
   //    rchild    : right child, lchild and rchild represents the dmsExtentID
   //                that should be set to key->_left and extentHead->_right,
   //                these two parameters could be DMS_INVALID_EXTENT for most
   //                new inserts, and may represent things during promoting keys
   //                into parent during split
   //   indexCB    : index control block
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_INSERTHERE, "_ixmExtent::insertHere" )
   INT32 _ixmExtent::insertHere ( UINT16 pos, const dmsRecordID &rid,
                                  const ixmKey &key, const Ordering &order,
                                  dmsExtentID lchild, dmsExtentID rchild,
                                  ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_INSERTHERE ) ;

      // attempt to physically insert the key into page
      // if there's no space in the page, it will attempt to reorg the page
      // first, if still not enough space it will return SDB_IXM_NOSPC
      rc = _basicInsert ( pos, rid, key, order ) ;
      if ( rc )
      {
         // if there's no space in the extent, let's split and insert
         if ( SDB_IXM_NOSPC == rc )
         {
            rc = _split ( pos, rid, key, order, lchild, rchild, indexCB ) ;
            goto done ;
         }
         // dump error message if other errCode returned
         PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
         goto error ;
      }
      // if insert completed in the current extent, let's reset the left and
      // right pointer
      {
         // get the inserted node
         ixmKeyNode *kn = writeKeyNode( pos ) ;
         // if the node is at end of the page, it means the new value is greater
         // than
         if ( pos+1 == getNumKeyNode() )
         {
            // if we are inserting at the last position, that means we don't
            // have _right for the page (otherwise it will go to _right), and
            // if this is a promoted key from split, we expect _right is
            // pointing to the same extent as lchild
            if ( _extentHead->_right != lchild )
            {
               PD_LOG ( PDERROR, "index logic error[lchild:%d, rchild:%d, "
                        "pos:%u, _extentHead->_right:%d]", lchild, rchild, pos,
                        _extentHead->_right ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
            // let's set the _left for the key to previous extentHead->right,
            // and set extentHead->_right to new rchild
            // this could be the same when the last page in the index tree
            // splits, and promoted to the parent node
            // When this happened, _left will be the original _right, and _right
            // will be the newly created index node
            kn->_left = _extentHead->_right ;
            // no need to set Parent because we are copying inside extent

            _assignRight ( rchild ) ;
         }
         else
         {
            // if we are inserting in the middle, we don't need to worry about
            // _right then
            kn->_left = lchild ;
            // no need to set parent for lchild because it has to be in the
            // same extent. Otherwise we would return SDB_SYS in the following
            // check

            // Since we already shifted all slots to next, let's grab pos+1 as
            // the original key
            ixmKeyNode *kn1 = writeKeyNode( pos + 1 ) ;
            // make sure the original key's _left same as lchild
            if ( kn1->_left != lchild )
            {
               PD_LOG ( PDERROR, "index logic error[lchild:%d, rchild:%d, "
                        "pos:%u, kn1->_left:%d]", lchild, rchild, pos,
                        kn1->_left ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }

            // then let's set it's _left to rchild, when rchild !=
            // INVALID_EXTENT, usually it happens during split
            kn1->_left = rchild ;
            // if rchild is not invalid, we should set the parent extent for the
            // rchild to _me
            if ( DMS_INVALID_EXTENT != rchild )
            {
               _ixmExtent ( rchild, _pIndexSu ).setParent ( _me ) ;
            }
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_INSERTHERE, rc );
      return rc ;
   error :
      goto done ;
   }
   // This function physically insert a key/rid into page. Please note that this
   // function does NOT fix the childs for the adj keys. This operation is
   // performed by insertHere()
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__BASICINS, "_ixmExtent::_basicInsert" )
   INT32 _ixmExtent::_basicInsert ( UINT16 &pos, const dmsRecordID &rid,
                                    const ixmKey &key, const Ordering &order )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__BASICINS );
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      UINT16 bytesNeeded = 0 ;
      // first let's validate the pos is same or less than the total number of
      // keys in the extent
      if ( pos > getNumKeyNode () )
      {
         PD_LOG ( PDERROR, "insert pos out of range" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // Then let's calculate how many bytes needed
      bytesNeeded = key.dataSize() + sizeof(ixmKeyNode) ;
      // If it's greater than the free size in the page, let's perform reorg and
      // check again
      if ( bytesNeeded > getFreeSize() )
      {
         // before reorg let's get the child extent id for pos
         dmsExtentID ch = getChildExtentID ( pos ) ;
         // note _reorg may change pos
         rc = _reorg ( order, pos ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "index extent reorg failed with rc : %d", rc ) ;
            goto error ;
         }
         SDB_ASSERT ( pos <= getNumKeyNode(), "pos is out of range" ) ;
         // if we still don't have enough space, let's return error
         if ( bytesNeeded > getFreeSize() )
         {
            rc = SDB_IXM_NOSPC ;
            goto error ;
         }
         // after reorg, the pos may points to an element with different lchild,
         // in this case we should be careful and perform find again
         if ( getChildExtentID( pos ) != ch )
         {
            rc = SDB_IXM_REORG_DONE ;
            goto error ;
         }
      }
      // move getNumKeyNode-pos elements to next slot
      ossMemmove ( (void*)writeKeyNode(pos+1), (void*)getKeyNode(pos),
                   sizeof(ixmKeyNode)*(getNumKeyNode()-pos) ) ;
      // free size minus the size of keynode (16 bytes)
      pHeader->_totalFreeSize -= sizeof(ixmKeyNode) ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID, sizeof(ixmKeyNode) ) ;
      pHeader->_totalKeyNodeNum ++ ;
      {
         // copy the key into the page
         INT32 datasize = key.dataSize() ;
         ixmKeyNode *kn = writeKeyNode( pos ) ;
         kn->_left = DMS_INVALID_EXTENT ;
         kn->_rid = rid ;
         // allocate datasize bytes from the page
         rc = _alloc ( datasize, kn->_keyOffset ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to allocate %d bytes in index",
                     key.dataSize()) ;
            goto error ;
         }
         // copy the data into the position
         ossMemcpy ( ((CHAR*)pHeader) + kn->_keyOffset,
                      key.data(), datasize ) ;
      }
#if defined (_DEBUG)
      rc = _validate(MAX, order) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to validate the extent, rc = %d", rc ) ;
         goto error ;
      }
#endif
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__BASICINS, rc );
      return rc ;
   error :
      goto done ;
   }

   // split function, this function is private and should not be called outside
   // ixmExtent. It will attempt to split a page in 50/50 mode or 90/10 mode,
   // depends on whether the new record is at end of the page. Once the split
   // position is found, it will allocate a new page and copy all records from
   // split position into new page
   // After that, if the current page is root page, it will allocate another
   // page as root, and prompt the last key from current page into new root
   // If there is parent page, it will also prompt the last key from current
   // page into new root
   // Once the left/right pointer in root are fixed, it will truncate the
   // current page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__SPLIT, "_ixmExtent::_split" )
   INT32 _ixmExtent::_split ( UINT16 pos, const dmsRecordID &rid,
                              const ixmKey &key, const Ordering &order,
                              const dmsExtentID lchild,
                              const dmsExtentID rchild,
                              ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__SPLIT );
      UINT16 splitPos = 0, newPos = 0 ;
      SDB_ASSERT ( indexCB, "index control block can't be NULL" ) ;
      dmsExtentID newExtentID = DMS_INVALID_EXTENT ;
      const ixmKeyNode *splitKey = NULL ;
      // find the split position
      rc = _splitPos ( pos, splitPos ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get split position, rc = %d", rc ) ;
         goto error ;
      }
      PD_TRACE2 ( SDB__IXMEXT__SPLIT, PD_PACK_USHORT(pos),
                  PD_PACK_USHORT(splitPos) ) ;
      // allocate new extent
      rc = indexCB->allocExtent ( newExtentID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to allocate new extent for index, rc = %d",
                  rc ) ;
         goto error ;
      }
      {
         // initialize the header for the new extent
         _ixmExtent newExtent( newExtentID, _extentHead->_mbID, _pIndexSu ) ;
         // copy all keys from the split pos to new extent
         for ( UINT16 i = splitPos + 1 ; i < getNumKeyNode() ; i++ )
         {
            const ixmKeyNode *kn = getKeyNode(i) ;
            rc = newExtent._pushBack ( kn->_rid,
                                    ixmKey(((const CHAR*)_extentHead)+kn->_keyOffset),
                                    order, kn->_left, indexCB->getFlag() ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to push back key %d to new extent, "
                        "rc = %d", (INT32)i, rc ) ;
               goto error ;
            }
         }
         // assign the right pointer
         newExtent._assignRight ( _extentHead->_right ) ;

#if defined (_DEBUG)
         rc = newExtent._validate(MAX, order) ;
#else
         rc = newExtent._validate(MIN, order) ;
#endif
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to validate the new extent, rc = %d",
                     rc ) ;
            goto error ;
         }

         // promote the split key into parent
         splitKey = getKeyNode( splitPos ) ;
         _assignRight ( splitKey->_left ) ;

         // is this root page
         if ( DMS_INVALID_EXTENT == getParent() )
         {
            // if this is root page, let's allocate another page
            dmsExtentID rootExtentID = DMS_INVALID_EXTENT ;
            // allocate new extent
            rc = indexCB->allocExtent ( rootExtentID ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate new extent for index, "
                        "rc = %d", rc ) ;
               goto error ;
            }
            // initialize the header for the new extent
            _ixmExtent rootExtent( rootExtentID, _extentHead->_mbID,
                                   _pIndexSu ) ;
            // promote the split key into parent, key._left point to the current
            // extent
            rc = rootExtent._pushBack ( splitKey->_rid,
                                        ixmKey(((const CHAR*)_extentHead)+
                                        splitKey->_keyOffset), order, _me,
                                        indexCB->getFlag() ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to promote split key into root, "
                        "rc = %d", rc ) ;
               goto error ;
            }
            // the _right is pointing to the splitted node
            rootExtent._assignRight ( newExtentID ) ;
#if defined (_DEBUG)
            rc = rootExtent._validate(MAX, order) ;
#else
            rc = rootExtent._validate(MIN, order) ;
#endif
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to validate the new root, rc = %d",
                        rc ) ;
               goto error ;
            }
            // set new root page
            indexCB->setRoot ( rootExtentID ) ;
         }
         else
         {
            // when there is parent page exist (so we are not root)
            newExtent.setParent ( getParent(),
                                  IXM_INDEX_FLAG_NORMAL == indexCB->getFlag() ) ;
            // get the parent extent
            _ixmExtent parentExtent( getParent(), _pIndexSu ) ;
            // do physical insert into it
            rc = parentExtent._insert( splitKey->_rid,
                       ixmKey(((const CHAR*)_extentHead)+splitKey->_keyOffset),
                       order, TRUE, _me,
                       newExtentID, indexCB ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to promote into parent, rc = %d",
                        rc ) ;
               goto error ;
            }
         }
         // now new page and(or) root page are created, and all keys are copied,
         // so we are safe to truncate
         newPos = pos ;
         // newPos will be the pos that after _reorg
         rc = _truncate ( splitPos, newPos, order ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index extent, rc = %d",
                     rc ) ;
            goto error ;
         }
         PD_TRACE1 ( SDB__IXMEXT__SPLIT, PD_PACK_USHORT(newPos) ) ;
         // if the insert position is smaller than split position, it will be
         // insert into the original page
         if ( pos <= splitPos )
         {
            SDB_ASSERT ( 0xFFFF != newPos, "Invalid newPos" ) ;
            // insert into newPos since _truncate will call _reorg, which will
            // remove unused keys from original extent, which may change newPos
            rc = insertHere ( newPos, rid, key, order, lchild, rchild,
                              indexCB ) ;
         }
         else
         {
            // otherwise the insert will be performed in new page
            rc = newExtent.insertHere ( pos-splitPos-1, rid, key, order, lchild,
                                        rchild, indexCB ) ;
         }
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert into splitted page, rc = %d",
                     rc ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__SPLIT, rc );
      return rc ;
   error :
      goto done ;
   }
   // truncate a page and leave totalNodes. Passin a newPos as
   // input/output, for any interested slot that may move its position.
   // For example the original layout looks like
   // <key1><removed><key2><key3><removed><key4>
   // so <key3> is at position 3.
   // After truncate(4)+reorg, the layout will be like
   // <key1><key2><key3>
   // so <key3> will be at position 2
   INT32 _ixmExtent::_truncate ( UINT16 totalNodes, UINT16 &newPos,
                                 const Ordering &order )
   {
      if ( totalNodes < getNumKeyNode() )
      {
         ixmExtentHead *pExtent = _extRW.writePtr<ixmExtentHead>() ;
         pExtent->_totalKeyNodeNum = totalNodes ;
         unsetCompact() ;
         return _reorg( order, newPos ) ;
      }
      return SDB_OK ;
   }

   // calculate the split position
   // pos is the position where we are trying to insert new record
   // splitPos is the returned value for where split should starts
   // if the pos is at end of the page, then we do 90/10 split, otherwise we do
   // 50/50 split
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__SPLITPOS, "_ixmExtent::_splitPos" )
   INT32 _ixmExtent::_splitPos ( UINT16 pos, UINT16 &splitPos ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__SPLITPOS );
      UINT16 rightSize = 0 ;
      UINT16 maxRightSize = 0 ;
      UINT16 totalKeySize = getTotalKeySize() ;
      PD_TRACE1 ( SDB__IXMEXT__SPLITPOS, PD_PACK_USHORT(totalKeySize) );
      splitPos = 1 ;
      // we should never call this function when there are less than two keys
      // (if that happen, after split and prompt to parent, we'll have empty
      // page
      if ( getNumKeyNode() < IXM_KEY_NODE_NUM_MIN )
      {
         PD_LOG ( PDERROR, "Only %d elements in the index",
                  (INT32)getNumKeyNode() ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( pos == _extentHead->_totalKeyNodeNum )
      {
         // if the new key is at end of the page, we do 90%+10% split
         maxRightSize = totalKeySize / 10 ;
      }
      else
      {
         // otherwise we do half-half split
         maxRightSize = totalKeySize / 2 ;
      }
      // calculate starting from right to left, and calculate the size of each
      // key
      for ( INT32 i = _extentHead->_totalKeyNodeNum-1 ; i >= 0 ; --i )
      {
         rightSize += ixmKey(getKeyData(i)).dataSize() ;
         if ( rightSize > maxRightSize )
         {
            splitPos = i ;
            break ;
         }
      }
      if ( splitPos > getNumKeyNode() - 2 )
      {
         splitPos = getNumKeyNode() - 2 ;
      }
      PD_TRACE1 ( SDB__IXMEXT__SPLITPOS, PD_PACK_USHORT( splitPos ) ) ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__SPLITPOS, rc );
      return rc ;
   error :
      goto done ;
   }

   // fix parent pointers for all child pages
   // loop through all keynodes, if the child exist, it will go to child and set
   // the parent extent to the current extent id
   // usually this happened during split for the new page
   INT32 _ixmExtent::_fixParentPtrs ( UINT16 startPos, UINT16 stopPos ) const
   {
      for ( UINT16 i = startPos; i < stopPos; i++ )
      {
         const ixmKeyNode *kn = getKeyNode ( i ) ;
         if ( DMS_INVALID_EXTENT != kn->_left )
         {
            /// add to page map
            _pPageMap->addItem( kn->_left, _me ) ;
         }
      }
      return SDB_OK ;
   }

   void _ixmExtent::_assignRight ( const dmsExtentID right )
   {
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>() ;
      pHeader->_right = right ;
      if ( DMS_INVALID_EXTENT != right )
      {
         _ixmExtent childExtent ( right, _pIndexSu ) ;
         childExtent.setParent ( _me ) ;
      }
   }

   void _ixmExtent::setChildExtentID ( UINT16 i, dmsExtentID extentID )
   {
      if ( i>_extentHead->_totalKeyNodeNum )
      {
         return ;
      }
      else if ( i == _extentHead->_totalKeyNodeNum )
      {
         _assignRight ( extentID ) ;
      }
      else
      {
         writeKeyNode(i)->_left = extentID ;
         if ( DMS_INVALID_EXTENT != extentID )
         {
            _ixmExtent childExtent ( extentID, _pIndexSu ) ;
            childExtent.setParent ( _me ) ;
         }
      }
   }

   // simply push a key/rid into the page, the caller has to ensure the key+rid
   // is greater than every other nodes and the new keynode will be inserted
   // into the last position
   // the caller should also need to ensure that the bytes required is smaller
   // than free size
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__PSHBACK, "_ixmExtent::_pushBack" )
   INT32 _ixmExtent::_pushBack ( const dmsRecordID &rid, const ixmKey &key,
                                 const Ordering &order, const dmsExtentID left,
                                 UINT16 idxState )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__PSHBACK );
      UINT16 bytesNeeded = key.dataSize() + sizeof(ixmKeyNode) ;
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      ixmKeyNode *kn = NULL ;
      // make sure we are not out of range
      if ( bytesNeeded > _extentHead->_totalFreeSize )
      {
         PD_LOG ( PDERROR, "Bytes needed should never smaller than "
                  "_totalFreeSize" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // if we have something in the page, let's make sure the new key is
      // greater than the last key in the page
      if ( getNumKeyNode() )
      {
         ixmKey lastkey ( getKeyData(getNumKeyNode()-1) ) ;
         if ( lastkey.woCompare(key, order) > 0 )
         {
            PD_LOG ( PDERROR, "New key smaller than the last key during "
                     "push" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      // allocate space and copy over key
      pHeader->_totalFreeSize -= sizeof(ixmKeyNode) ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID, sizeof(ixmKeyNode) ) ;
      kn = writeKeyNode(_extentHead->_totalKeyNodeNum) ;
      pHeader->_totalKeyNodeNum++ ;
      kn->_left = left ;

      if ( DMS_INVALID_EXTENT != kn->_left )
      {
         // Only add to page map if the index is in normal state. Otherwise,
         // set parent for the child in extent directly.
         // This is to avoid one concurrent problem: When multiple indices are
         // being created on the same collection, they may hit the code here
         // at the same time. Because during rebuilding, they only hold shared
         // lock of the collection. But the addItem will do WRITE operation to
         // the map, which is shared by all indices on the collection. This will
         // corrupt the map, and crash the program.
         if ( IXM_INDEX_FLAG_NORMAL == idxState )
         {
            /// add to page map
            _pPageMap->addItem( kn->_left, _me ) ;
         }
         else
         {
            ixmExtent child( kn->_left, _pIndexSu ) ;
            child.setParent( _me, FALSE ) ;
         }
      }

      kn->_rid = rid ;
      rc = _alloc ( key.dataSize(), kn->_keyOffset ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to allocate %d bytes in index",
                  key.dataSize()) ;
         goto error ;
      }
      ossMemcpy ( ((CHAR*)pHeader)+kn->_keyOffset,
                  key.data(), key.dataSize()) ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__PSHBACK, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__VALIDATE2, "_ixmExtent::_validate" )
   INT32 _ixmExtent::_validate( ixmIndexCB *indexCB, dmsExtentID parent )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__IXMEXT__VALIDATE2 ) ;

      if ( _extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           _extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         PD_LOG ( PDERROR, "Invalid index extent eye catcher" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( _extentHead->_beginFreeOffset - sizeof(ixmExtentHead) -
           _extentHead->_totalKeyNodeNum*sizeof(ixmKeyNode) !=
           _extentHead->_totalFreeSize )
      {
         PD_LOG ( PDERROR, "Inconsistent free size" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( !(_extentHead->_flag & DMS_MB_FLAG_USED) )
      {
         PD_LOG ( PDERROR, "Invalid flag" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT != parent
           && getParent() != parent )
      {
         PD_LOG( PDERROR, "Invalid index extent parent" ) ;
         dumpIndexExtentIntoLog() ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( getMBID() != indexCB->getMBID() )
      {
         PD_LOG( PDERROR, "Invalid index extent mb id" ) ;
         dumpIndexExtentIntoLog() ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMEXT__VALIDATE2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // validate a page, there are 4 levels
   // NONE, MIN, MID, MAX
   // NONE will return SDB_OK right away
   // MIN will validate extent head information
   // MID will compare the first and last keys in the page and return failed if
   // the first key greater than last
   // MAX will compare each key to its next in sequence, and make sure all
   // previous keys are smaller or equal to the next
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__VALIDATE, "_ixmExtent::_validate" )
   INT32 _ixmExtent::_validate ( _ixmExtentValidateLevel level,
                                 const Ordering &order ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__VALIDATE );
      if ( NONE == level )
         goto done ;
      // min/mid/max
      if ( _extentHead->_eyeCatcher[0] != IXM_EXTENT_EYECATCHER0 ||
           _extentHead->_eyeCatcher[1] != IXM_EXTENT_EYECATCHER1 )
      {
         PD_LOG ( PDERROR, "Invalid index extent eye catcher" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( _extentHead->_beginFreeOffset - sizeof(ixmExtentHead) -
           _extentHead->_totalKeyNodeNum*sizeof(ixmKeyNode) !=
           _extentHead->_totalFreeSize )
      {
         PD_LOG ( PDERROR, "Inconsistent free size" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( !(_extentHead->_flag & DMS_MB_FLAG_USED) )
      {
         PD_LOG ( PDERROR, "Invalid flag" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }

      // mid
      if ( MID == level )
      {
         ixmKey k1 ( getKeyData(0) ) ;
         ixmKey k2 ( getKeyData(_extentHead->_totalKeyNodeNum-1) ) ;
         if ( k1.woCompare(k2, order) > 0 )
         {
            PD_LOG ( PDERROR, "First key is greater than the last" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else if ( MAX == level )
      {
         for ( UINT16 i = 0; i < _extentHead->_totalKeyNodeNum-1; i++ )
         {
            ixmKey k1 ( getKeyData(i)) ;
            ixmKey k2 ( getKeyData(i+1)) ;
            INT32 result = k1.woCompare(k2, order) ;
            if ( result > 0 )
            {
               PD_LOG ( PDERROR, "%d'th key is greater than its next",
                        (INT32)i ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
            else if ( 0 == result )
            {
               dmsRecordID rid1 = getKeyNode(i)->_rid ;
               dmsRecordID rid2 = getKeyNode(i+1)->_rid ;
               if ( rid1.compare(rid2) >=0 )
               {
                  PD_LOG ( PDERROR, "%d'th key's RID is greater or equal to "
                           "the next", (INT32)i ) ;
                  dumpIndexExtentIntoLog () ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } //else if ( 0 == result )
         } //for (
      } //else if ( MAX == level )
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__VALIDATE, rc );
      return rc ;
   error :
      goto done ;
   }
   // inline reorg an index page, wrapper for the other _reorg function
   INT32 _ixmExtent::_reorg (const Ordering &order)
   {
      UINT16 dummy = 0xFFFF ;
      return _reorg ( order, dummy ) ;
   }
   // inline reorg an index page, newPos represent the input/output for a key
   // after reorg happened
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__REORG, "_ixmExtent::_reorg" )
   INT32 _ixmExtent::_reorg (const Ordering &order, UINT16 &newPos)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__REORG );

      ixmExtentHead *pHeader = NULL ;
      UINT16 beginFreeOffset = _pageSize-1 ;
      UINT16 totalKeyNodeNum = 0 ;
      UINT16 totalFreeSize = beginFreeOffset - sizeof(ixmExtentHead) ;
      CHAR   buffer[DMS_PAGE_SIZE_MAX] ;

      if ( isCompact() )
      {
         goto done ;
      }

      pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;

      // loop through all keys in the page
      for ( UINT16 i = 0 ; i < pHeader->_totalKeyNodeNum ; i++ )
      {
         ixmKeyNode *kn = writeKeyNode(i) ;
         INT32 keyDataSize = 0 ;
         // if the slot doesn't same as previous, and that is what we are
         // looking for, then let's set newPos to the new position after reorg
         if ( newPos == i )
         {
            newPos = totalKeyNodeNum ;
         }
         // if there is no child and it's unused, let's skip it ( that means it
         // will not be copied and count, so it's actually deleted)
         if ( kn->isUnused() && DMS_INVALID_EXTENT == kn->_left )
         {
            /// When all node is unused, should keep the one node.
            /// Otherwise the page will has no key node
            if ( totalKeyNodeNum > 0 ||
                 i < pHeader->_totalKeyNodeNum - 1 )
            {
               continue ;
            }
         }
         totalFreeSize -= sizeof(ixmKeyNode) ;
         // copy the key
         ixmKey key ( ((const CHAR*)pHeader)+kn->_keyOffset) ;
         keyDataSize = key.dataSize() ;
         if ( (INT32)beginFreeOffset - keyDataSize < 0 ||
              (INT32)totalFreeSize - keyDataSize < 0 )
         {
            PD_LOG ( PDERROR, "key is too large" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         beginFreeOffset -= keyDataSize ;
         totalFreeSize -= keyDataSize ;
         ossMemcpy ( &buffer[beginFreeOffset],
                     ((const CHAR*)pHeader)+kn->_keyOffset,
                      keyDataSize ) ;
         kn->_keyOffset = beginFreeOffset ;
         // copy the slot
         if ( totalKeyNodeNum != i )
         {
            ossMemcpy ( ((CHAR*)pHeader) + sizeof(ixmExtentHead) +
                        totalKeyNodeNum*sizeof(ixmKeyNode),
                        ((const CHAR*)pHeader) + sizeof(ixmExtentHead) +
                        i*sizeof(ixmKeyNode),
                        sizeof(ixmKeyNode)) ;
         }
         ++totalKeyNodeNum ;
      }
      // handle the situation where requested pos is right-most
      if ( pHeader->_totalKeyNodeNum == newPos )
      {
         newPos = totalKeyNodeNum ;
      }
      else if ( pHeader->_totalKeyNodeNum < newPos )
      {
         newPos = 0xFFFF ;
      }

      PD_TRACE1 ( SDB__IXMEXT__REORG, PD_PACK_USHORT( newPos ) ) ;

      pHeader->_beginFreeOffset = beginFreeOffset ;
      pHeader->_totalKeyNodeNum = totalKeyNodeNum ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID,
                                   pHeader->_totalFreeSize ) ;
      pHeader->_totalFreeSize = totalFreeSize ;
      _pIndexSu->addStatFreeSpace( pHeader->_mbID,
                                   pHeader->_totalFreeSize ) ;
      ossMemcpy ( ((CHAR*)pHeader)+beginFreeOffset,
                  &buffer[beginFreeOffset],
                  _pageSize - beginFreeOffset ) ;
#if defined (_DEBUG)
      rc = _validate(MAX, order) ;
#else
      rc = _validate(MIN, order) ;
#endif
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to validate the new extent, rc = %d", rc ) ;
         goto error ;
      }
      setCompact() ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__REORG, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _ixmExtent::_alloc ( INT32 requestSpace, UINT16 &beginOffset )
   {
      if ( requestSpace > (INT32)getFreeSize() )
      {
         return SDB_IXM_NOSPC ;
      }
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>() ;
      pHeader->_beginFreeOffset -= requestSpace ;
      pHeader->_totalFreeSize -= requestSpace ;
      _pIndexSu->decStatFreeSpace( pHeader->_mbID, requestSpace ) ;
      beginOffset = pHeader->_beginFreeOffset ;
      return SDB_OK ;
   }

   // find + insertHere
   // Internal function, insert an rid/key pair into the current page. This
   // function will perform find() for the given key/rid pair, and recursively
   // call itself if there's child page associate with the keynodes until hit
   // leaf. In leaf it will call insertHere()
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__INSERT, "_ixmExtent::_insert" )
   INT32 _ixmExtent::_insert ( const dmsRecordID &rid, const ixmKey &key,
                               const Ordering &order, BOOLEAN dupAllowed,
                               dmsExtentID lchild, dmsExtentID rchild,
                               ixmIndexCB *indexCB,
                               utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__INSERT ) ;

      INT32 keyFoundPos = -1 ;
      BOOLEAN sameFound = FALSE ;
      UINT16 pos = 0 ;
      dmsExtentID ch = DMS_INVALID_EXTENT ;
      const ixmKeyNode *kn = NULL ;

      // sanity check
      INT32 keySize = key.dataSize() ;
      if ( keySize > _pIndexSu->indexKeySizeMax() )
      {
         PD_LOG ( PDERROR, "key size[%d] must be less than or equal to [%d]",
                  keySize, _pIndexSu->indexKeySizeMax() ) ;
         rc = SDB_IXM_KEY_TOO_LARGE ;
         goto error ;
      }
      if ( key.dataSize() <= 0 )
      {
         PD_LOG ( PDERROR, "key size must be greater than 0" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( indexCB->notNull() && key.hasNullOrUndefined() )
      {
         rc = SDB_IXM_KEY_NOTNULL ;
         PD_LOG ( PDERROR, "Any field of index key cannot be null "
                  "or does not exist, rc: %d", rc ) ;
         goto error ;
      }

   retry :
      // try to locate where the insert should happen
      rc = find ( indexCB, key, rid, order, pos, keyFoundPos, sameFound ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error happened during find, rc = %d", rc ) ;
         goto error ;
      }

      if ( sameFound )
      {
         kn = getKeyNode( pos ) ;
         if ( kn->isUnused() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Page[%d]'s key node[%d] should be used",
                    _me, pos ) ;
            dumpIndexExtentIntoLog() ;
            goto error ;
         }
         PD_LOG ( PDINFO, "same key + rid is already in index" ) ;
         // have same key/rid point to same record
         rc = SDB_IXM_IDENTICAL_KEY ;
         goto error ;
      }
      else if ( !dupAllowed && -1 != keyFoundPos )
      {
         kn = getKeyNode( keyFoundPos ) ;
         // if we find duplicate, let's check whether the key includes all
         // Undefined. If this is the case, it's a special case that user
         // doesn't define those keys, so we should allow it proceed ( which
         // may violate unique definition ). If we restricted this behavior,
         // user cannot insert records that does not contains the keys twice,
         // which is very violating "schemaless"
         if ( kn->isUsed() && ( indexCB->enforced() || !key.isUndefined () ) )
         {
            // this error only returned when dupAllowed == FALSE
            // this error represent duplicate key is not allowed and
            // duplicate key is detected
#ifdef _DEBUG
            PD_LOG ( PDWARNING, "Duplicate key is detected with rid(%d, %d), "
                     "page:%d, keynode:%d, insert rid:(%d, %d)",
                     kn->_rid._extent, kn->_rid._offset, _me, keyFoundPos,
                     rid._extent, rid._offset ) ;
#else
            PD_LOG ( PDINFO, "Duplicate key is detected with rid(%d, %d), "
                     "page:%d, keynode:%d, insert rid:(%d, %d)",
                     kn->_rid._extent, kn->_rid._offset, _me, keyFoundPos,
                     rid._extent, rid._offset ) ;
#endif
            if ( pResult )
            {
               pResult->setCurRID( rid ) ;
               pResult->setPeerRID( kn->_rid ) ;
            }
            rc = SDB_IXM_DUP_KEY ;
            goto error ;
         }
      }

      ch = getChildExtentID( pos ) ;
      // if there's no child, of course we will insert into the current page
      // and if there is child, but rchild is specified, this means the function
      // is called by the child extent in split (when prompt the last key to the
      // parent, rchild represent the newly created page), in this case we also
      // simply insert it into the page instead of traversing down
      if ( DMS_INVALID_EXTENT == ch || DMS_INVALID_EXTENT != rchild )
      {
         rc = insertHere ( pos, rid, key, order, lchild, rchild, indexCB ) ;
         if ( rc )
         {
            // we have performed reorg and found the position we supposed to
            // insert got a left pointer, so let's reperform find
            if ( SDB_IXM_REORG_DONE == rc )
            {
               rc = SDB_OK ;
               goto retry ;
            }
            PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
            goto error ;
         }
      }
      // otherwise let's traverse down
      else
      {
         rc = _ixmExtent(ch, _pIndexSu)._insert( rid, key, order, dupAllowed,
                                                 lchild, rchild, indexCB,
                                                 pResult ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to insert, rc = %d", rc ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__INSERT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // Find and remove specific key from an index
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_UNINDEX, "_ixmExtent::unindex" )
   INT32 _ixmExtent::unindex ( const ixmKey &key, const dmsRecordID &rid,
                               const Ordering &order, ixmIndexCB *indexCB,
                               BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_UNINDEX );
      BOOLEAN found ;
      ixmRecordID indexrid ;
      result = FALSE ;

      rc = _locate ( key, rid, order, indexrid, found, 1, indexCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate key and rid" ) ;
         goto error ;
      }
      if ( found )
      {
         rc = ixmExtent( indexrid._extent, _pIndexSu)._delKeyAtPos (
                         indexrid._slot, order, indexCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "failed to delete key" ) ;
            goto error ;
         }
         result = TRUE ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_UNINDEX, rc );
      return rc ;
   error :
      goto done ;
   }
   // delete a key from a given position
   // caller must make sure the pos is smaller than the total number of keys in
   // the page, and there is no left pointer on the key
   // this function will physically remove keynode on the page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELKEYATPOS1, "_ixmExtent::_delKeyAtPos" )
   INT32 _ixmExtent::_delKeyAtPos ( UINT16 pos )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELKEYATPOS1 );
      ixmKeyNode *kn = NULL ;
      ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>( 0, _pageSize ) ;
      if ( pos >= getNumKeyNode() )
      {
         PD_LOG ( PDERROR, "pos out of range, pos=%d, totalKey=%d",
                  (INT32)pos, (INT32)getNumKeyNode() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      kn = writeKeyNode( pos ) ;
      if ( DMS_INVALID_EXTENT != kn->_left )
      {
         PD_LOG ( PDERROR, "left pointer must be NULL" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pHeader->_totalFreeSize += sizeof(ixmKeyNode) ;
      _pIndexSu->addStatFreeSpace( pHeader->_mbID, sizeof(ixmKeyNode) ) ;
      pHeader->_totalKeyNodeNum-- ;
      ossMemmove ( (CHAR*)kn, (const CHAR*)getKeyNode(pos+1),
                   sizeof(ixmKeyNode)*(pHeader->_totalKeyNodeNum-pos) ) ;
      unsetCompact() ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__DELKEYATPOS1, rc );
      return rc ;
   error :
      goto done ;
   }
   // delete a key from page at pos, caller do NOT need to validate left pointer
   // and root
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELKEYATPOS2, "_ixmExtent::_delKeyAtPos" )
   INT32 _ixmExtent::_delKeyAtPos ( UINT16 pos, const Ordering &order,
                                    ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELKEYATPOS2 );
      dmsExtentID left = DMS_INVALID_EXTENT ;
      BOOLEAN result = FALSE ;
      if ( pos >= getNumKeyNode() )
      {
         PD_LOG ( PDERROR, "pos out of range, pos=%d, totalKey=%d",
                 (INT32)pos, (INT32)getNumKeyNode() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      left = getKeyNode( pos )->_left ;

      // All code path within this block will endup jumping to either
      // done or error
      if ( 1 == getNumKeyNode() )
      {
         if ( DMS_INVALID_EXTENT == left &&
              DMS_INVALID_EXTENT == _extentHead->_right )
         {
            // first let's remove the key since we knows the left pointer is
            // NULL
            rc = _delKeyAtPos ( pos ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to delete at pos %d", (INT32) pos ) ;
               goto error ;
            }
            if ( DMS_INVALID_EXTENT != getParent() )
            {
               // if we have only 1 key and there's no left/right children in
               // the page, and we are not root, let's first attempt to share
               // some keys from neighbors
               rc = _mayBalanceWithNeighbors ( order, indexCB, result ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to balance with neighbors" ) ;
                  goto error ;
               }
               if ( !result )
               {
                  // if we don't have neighbors to share, let's remove the node
                  rc = _delExtent ( indexCB ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to delete extent for the "
                              "index" ) ;
                     goto error ;
                  }
               }
            }
            // when we get here, we already balanced with neighbor or deleted
            // the extent, or it's root page, let's return
            goto done ;
         }
         // when we get here, that means either left pointer is not null or
         // there's right pointer in the page, then let's attempt to do delete
         // internal key
         rc = _deleteInternalKey ( pos, order, indexCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to delete internal key" ) ;
            goto error ;
         }
         goto done ;
      }  // end of if ( 1 == getNumKeyNode() )

      // when we get there, that means we have more than 1 key in the extent
      if ( DMS_INVALID_EXTENT == left )
      {
         // No left, we can remove the key from the extent
         rc = _delKeyAtPos ( pos ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to delete at pos %d", (INT32)pos ) ;
            goto error ;
         }
         rc = _mayBalanceWithNeighbors ( order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to balance with neighbors" ) ;
            goto error ;
         }
      }
      else
      {
         // if the left pointer is not null, let's do internal delete, this may
         // touch/move the next key
         rc = _deleteInternalKey ( pos, order, indexCB ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to delete internal key" ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__DELKEYATPOS2, rc );
      return rc ;
   error :
      goto done ;
   }

   // we should do rebalance and merge in this code, but let's leave it for now
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__MAYBLCWITHNGB, "_ixmExtent::_mayBalanceWithNeighbors" )
   INT32 _ixmExtent::_mayBalanceWithNeighbors ( const Ordering &order,
                                                ixmIndexCB *indexCB,
                                                BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__MAYBLCWITHNGB );
      result = FALSE ;
      UINT16 pos ;
      //BOOLEAN mayBalanceRight ;
      //BOOLEAN mayBalanceLeft ;
      // let's return if it's root

      if ( DMS_INVALID_EXTENT == getParent() )
      {
         goto done ;
      }
      {
         // get the parent extent
         ixmExtent parent( getParent(), _pIndexSu ) ;
         // find the key pointing to this extent
         rc = parent._findChildExtent( _me, pos ) ;
         // if we can't find the key, something really bad happened
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to find the extent in it's parent" ) ;
            goto error ;
         }
      }

      // if we are not the _right, and our next slot got child, we may do right
      // balance
      /*mayBalanceRight = (pos < parent.getNumKeyNode() &&
                         parent.getChildExtentID(pos+1) !=
                            DMS_INVALID_EXTENT ) ;
      // if we are not the first, and our previous slot got child, we may do
      // left balance
      mayBalanceLeft = (pos>0 && parent.getChildExtentID(pos-1) !=
                            DMS_INVALID_EXTENT ) ;*/
      /*
      // attempt to balance child
      if ( mayBalanceRight )
      {
         // for right balance, we merge pos and pos+1
         rc = parent._tryBalanceChildren ( pos, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         if ( result )
            goto done ;
      }
      if ( mayBalanceLeft )
      {
         // for left balance, we merge pos-1 and pos
         rc = parent._tryBalanceChildren ( pos-1, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         if ( result )
            goto done ;
      }
      // attempt to merge child
      if ( mayBalanceRight )
      {
         // for right balance, we merge pos and pos+1
         rc = parent._doMergeChildren ( pos, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         goto done ;
      }
      if ( mayBalanceLeft )
      {
         // for left balance, we merge pos-1 and pos
         rc = parent._doMergeChildren ( pos-1, order, indexCB, result ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to try balance children" ) ;
            goto error ;
         }
         goto done ;
      } */
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__MAYBLCWITHNGB, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELEXT, "_ixmExtent::_delExtent" )
   INT32 _ixmExtent::_delExtent ( ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELEXT );
      UINT16 pos = 0 ;
      UINT16 mbID = 0 ;
      UINT16 freeSize = 0 ;

      // if we are root, we simply return
      if ( DMS_INVALID_EXTENT != getParent() )
      {
         // get the parent extent
         ixmExtent parent( getParent(), _pIndexSu ) ;
         // find the key pointing to this extent
         rc = parent._findChildExtent ( _me, pos ) ;
         // if we can't find the key, something really bad happened
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to find the extent in it's parent" ) ;
            goto error ;
         }
         parent.setChildExtentID ( pos, DMS_INVALID_EXTENT ) ;
         mbID = _extentHead->_mbID ;
         freeSize = _extentHead->_totalFreeSize ;
         rc = indexCB->freeExtent ( _me ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to free extent" ) ;
            goto error ;
         }
         _pIndexSu->decStatFreeSpace( mbID, freeSize ) ;
         _pPageMap->rmItem( _me ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__DELEXT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__FNDCHLDEXT, "_ixmExtent::_findChildExtent" )
   INT32 _ixmExtent::_findChildExtent ( dmsExtentID childExtent,
                                        UINT16 &pos ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__FNDCHLDEXT );
      if ( _extentHead->_right == childExtent )
      {
         pos = getNumKeyNode() ;
         goto done ;
      }
      for ( UINT16 i =0 ; i<getNumKeyNode(); i++ )
      {
         if ( getChildExtentID (i) == childExtent )
         {
            pos = i ;
            goto done ;
         }
      }
      rc = SDB_IXM_KEY_NOTEXIST ;
   done :
      PD_TRACE1 ( SDB__IXMEXT__FNDCHLDEXT, PD_PACK_USHORT( pos ) );
      PD_TRACE_EXITRC ( SDB__IXMEXT__FNDCHLDEXT, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__DELITNKEY, "_ixmExtent::_deleteInternalKey" )
   INT32 _ixmExtent::_deleteInternalKey ( UINT16 pos, const Ordering &order,
                                          ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__DELITNKEY );
      dmsExtentID lchild = getChildExtentID(pos) ;
      dmsExtentID rchild = getChildExtentID(pos+1) ;
      ixmRecordID nextIndexKey ;
      INT32 direction ;
      if ( DMS_INVALID_EXTENT == lchild && DMS_INVALID_EXTENT == rchild )
      {
         PD_LOG ( PDERROR, "both left/right child are NULL" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      direction = (DMS_INVALID_EXTENT == lchild)?1:-1 ;
      nextIndexKey._extent = _me ;
      nextIndexKey._slot = pos ;
      rc = advance ( nextIndexKey, direction ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to find the next index key" ) ;
         goto error ;
      }
      // since we already checked that either lchild or rchild exist,
      // nextIndexKey should never be NULL here
      if ( nextIndexKey.isNull() )
      {
         PD_LOG ( PDERROR, "advance key shouldn't be NULL" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      {
         // now the nextExtent contains the next key
         ixmExtent nextExtent ( nextIndexKey._extent, _pIndexSu ) ;
         // if the next key do have child or its next (including _right)
         // do have child, let's use a simple way to set the key unused (a
         // better way could be recursively swap+_deleteInternalKey /
         // _delKeyAtPos)
         if ( nextExtent.getChildExtentID ( nextIndexKey._slot ) !=
                    DMS_INVALID_EXTENT ||
              nextExtent.getChildExtentID ( nextIndexKey._slot+1 ) !=
                    DMS_INVALID_EXTENT )
         {
            writeKeyNode(pos)->setUnused() ;
         }
         else
         {
            // if there's no child for the next key, let's replace the next key
            // to the current keynode and remove the next key from its original
            // extent
            const ixmKeyNode *kn = nextExtent.getKeyNode
                  ( nextIndexKey._slot ) ;
            ixmKey nextKey ( nextExtent.getKeyData(nextIndexKey._slot)) ;
            if ( !kn )
            {
               PD_LOG ( PDERROR, "Failed to find key node" ) ;
               dumpIndexExtentIntoLog () ;
               rc = SDB_SYS ;
               goto error ;
            }
            rc = _setInternalKey ( pos, kn->_rid, nextKey, order,
                                   getChildExtentID ( pos ) ,
                                   getChildExtentID ( pos+1 ) ,
                                   indexCB ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "failed to set internal key" ) ;
               goto error ;
            }
            rc = nextExtent._delKeyAtPos ( nextIndexKey._slot, order,
                                           indexCB ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "failed to delete key" ) ;
               goto error ;
            }
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__DELITNKEY, rc );
      return rc ;
   error :
      goto done ;
   }

   // Move to the next key based on the direction
   // keyRID is for input and output
   // direction=1 means forward, -1 means backward
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_ADVANCE, "_ixmExtent::advance" )
   INT32 _ixmExtent::advance ( ixmRecordID &keyRID, INT32 direction ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_ADVANCE );
      INT32 adj ;
      INT32 ko ;
      dmsExtentID nextDown ;
      dmsExtentID childExtent ;
      dmsExtentID parent ;

      if ( keyRID._slot >= getNumKeyNode() )
      {
         PD_LOG ( PDERROR, "key slot is out of range" ) ;
         rc = SDB_IXM_KEY_NOTEXIST ;
         goto error ;
      }
      adj = direction < 0 ? 1:0 ;
      ko = keyRID._slot + direction ;
      // for forward, we get _left for the next key
      // for backward, we get _left for the current key
      nextDown = getChildExtentID((UINT16)(ko+adj)) ;
      if ( DMS_INVALID_EXTENT != nextDown )
      {
         // loop until hitting leaf, we always find the _left from next element
         // in forward search, or find the biggest element from the _left for
         // the current element for backward search
         while ( TRUE )
         {
            ixmExtent childExtent(nextDown, _pIndexSu) ;
            // for forward, we get first element in the child, for backward we
            // get the last element in the child
            keyRID._slot = direction>0?0:
                (childExtent.getNumKeyNode()-1) ;
            // get the _left for the child extent
            dmsExtentID child = childExtent.getChildExtentID(keyRID._slot+adj) ;
            if ( DMS_INVALID_EXTENT == child )
               break ;
            nextDown = child ;
         }
         // after loop, the nextDown should represent the element without _left,
         // and keyRID._slot is updated for the target slot. So let's just
         // update keyRID._extent to nextDown
         keyRID._extent = nextDown ;
         goto done ;
      }
      // if we don't have _left, let's just check if we are on the key (instead
      // of end of page)
      if ( ko < getNumKeyNode() && ko >= 0 )
      {
         keyRID._slot = (UINT16)ko ;
         keyRID._extent = _me ;
         goto done ;
      }
      // here we are at end of bucket, we should go to parent
      childExtent = _me ;
      parent = getParent() ;
      while ( TRUE )
      {
         // we don't continue if getting to root
         if ( DMS_INVALID_EXTENT == parent )
            break ;
         ixmExtent parentExtent ( parent, _pIndexSu ) ;
         // in the parent extent, let's see who's _left pointing to the current
         // extent, then that's what we are looking for
         for ( UINT16 i=0; i<parentExtent.getNumKeyNode(); i++ )
         {
            if ( childExtent == parentExtent.getChildExtentID(i+adj) )
            {
               keyRID._slot = i ;
               keyRID._extent = parent ;
               goto done ;
            }
         }
         // we should never hit here in forward search, because each _left must
         // have a valid keynode, unless it's at _right node
         if ( direction > 0 && parentExtent._extentHead->_right != childExtent )
         {
            PD_LOG ( PDERROR,"Invalid tree structure" ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         childExtent = parent ;
         parent = parentExtent.getParent() ;
      }
      // when we get here, it means there's no other keys avaliable
      keyRID.reset() ;
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_ADVANCE, rc );
      return rc ;
   error :
      goto done ;
   }
   // caller must make sure there's no _left for pos
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__SETITNKEY, "_ixmExtent::_setInternalKey" )
   INT32 _ixmExtent::_setInternalKey (UINT16 pos, const dmsRecordID &rid,
                                      const ixmKey &key,
                                      const Ordering &order, dmsExtentID lchild,
                                      dmsExtentID rchild, ixmIndexCB *indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__SETITNKEY );
      setChildExtentID ( pos, DMS_INVALID_EXTENT ) ;
      rc = _delKeyAtPos ( pos ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to delete key at pos" ) ;
         goto error ;
      }
      // since _delKeyAtPos moved all following keynodes back to one, we check
      // pos again to get next keynode
      if ( getChildExtentID ( pos ) != rchild )
      {
         PD_LOG ( PDERROR, "rchild doesn't match" ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // set child extent for the next to lchild
      setChildExtentID ( pos, lchild ) ;
      rc = insertHere ( pos, rid, key, order, lchild, rchild, indexCB ) ;
      if ( rc )
      {
         // we don't need to worry about SDB_IXM_REORG_DONE because we already
         // set child extent id to lchild, so _reorg should never remove the
         // slot
         PD_LOG ( PDERROR, "Failed to insert here" ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__SETITNKEY, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _ixmExtent::_doMergeChildren ( UINT16 pos, const Ordering &order,
                                        ixmIndexCB *indexCB, BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      return rc ;
   }
   INT32 _ixmExtent::locate ( const BSONObj &key, const dmsRecordID &rid,
                              const Ordering &order, ixmRecordID &indexrid,
                              BOOLEAN &found, INT32 direction,
                              const ixmIndexCB *indexCB ) const
   {
      ixmKeyOwned ixkey ( key ) ;
      return _locate ( ixkey, rid, order, indexrid, found, direction, indexCB );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__LOCATE, "_ixmExtent::_locate" )
   INT32 _ixmExtent::_locate ( const ixmKey &key, const dmsRecordID &rid,
                               const Ordering &order, ixmRecordID &indexrid,
                               BOOLEAN &found, INT32 direction,
                               const ixmIndexCB *indexCB ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__LOCATE );
      SDB_ASSERT ( 1 == direction || -1 == direction, "Invalid direction" ) ;
      UINT16 pos = 0 ;
      dmsExtentID childExtent = DMS_INVALID_EXTENT ;
      INT32 keyFoundPos = -1 ;

      rc = find ( indexCB, key, rid, order, pos, keyFoundPos, found ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to find in locate" ) ;
         goto error ;
      }
      // if the key and rid exist in this page and not psuedo-deleted
      // then let's just record the extent id and position and return
      if ( found )
      {
         indexrid._extent = _me ;
         indexrid._slot = pos ;
         goto done ;
      }

      // when we get here, that means result == FALSE
      childExtent = getChildExtentID ( pos ) ;
      if ( DMS_INVALID_EXTENT != childExtent )
      {
         // if we get left pointer, that means we have child page, then let's do
         // _locate recursively
         rc = ixmExtent(childExtent, _pIndexSu)._locate( key, rid, order,
                                                         indexrid, found,
                                                         direction, indexCB ) ;
         if ( rc )
         {
            // don't have to repeatedly log in interm pages
            goto error ;
         }
         // if child found the key/rid, or if it find a good place for "next",
         // then we simply return
         // otherwise jump out if and do other checks
         if ( !indexrid.isNull() )
         {
            goto done ;
         }
      }
      // check scan direction
      if ( (direction<0 && 0==pos) || (direction>0 && getNumKeyNode()==pos) )
      {
         indexrid.reset() ;
      }
      else
      {
         indexrid._extent = _me ;
         indexrid._slot = direction<0?pos-1:pos ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__LOCATE, rc );
      return rc ;
   error :
      goto done ;
   }
   // Weather a key exists in the index tree
   // output in result
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_EXIST, "_ixmExtent::exists" )
   INT32 _ixmExtent::exists ( const ixmKey &key, const Ordering &order,
                             const ixmIndexCB *indexCB, BOOLEAN &result ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_EXIST );
      BOOLEAN found ;
      dmsRecordID dummyID ;
      ixmRecordID indexrid ;
      result = FALSE ;
      // try to locate the key and (-1,-1) for rid
      rc = _locate ( key, dummyID, order, indexrid, found, 1, indexCB );
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate key" ) ;
         goto error ;
      }
      // loop until indexrid is invalid
      while ( TRUE )
      {
         if ( indexrid.isNull() )
            break ;
         // create extent for indexrid
         ixmExtent extent ( indexrid._extent, _pIndexSu ) ;
         // get the keynode
         const ixmKeyNode *kn = extent.getKeyNode(indexrid._slot) ;
         // skip unused keys (psuedo-deleted)
         if ( kn->isUsed() )
         {
            // compare the on-disk key and the one we are looking for, if they
            // match that means we got exists
            result = ixmKey(extent.getKeyData(indexrid._slot)).woEqual(key) ;
            goto done ;
         }
         // advance to next keynode
         rc = extent.advance ( indexrid, 1 ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to advance" ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_EXIST, rc );
      return rc ;
   error :
      goto done ;
   }
   // in order to avoid parent pointer pointing to itself (from disk
   // corruption), we loop 100 rounds max, usually B tree will never exceed 100
   // levels
   // This function returns the extent id for root of the index page
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_GETROOT, "_ixmExtent::getRoot" )
   dmsExtentID _ixmExtent::getRoot() const
   {
      PD_TRACE_ENTRY ( SDB__IXMEXT_GETROOT );
      dmsExtentID extentID = _me ;
      INT32 maxLoop = 100 ;
      while ( DMS_INVALID_EXTENT != extentID &&
              maxLoop > 0 )
      {
         ixmExtent extent ( extentID, _pIndexSu ) ;
         if ( extent.isRoot() )
         {
            PD_TRACE_EXIT ( SDB__IXMEXT_GETROOT );
            return extentID ;
         }
         extentID = extent.getParent() ;
         maxLoop-- ;
      }
      // normally we should never reach here
      PD_LOG ( PDERROR, "loop more than %d times to get root", 100 ) ;
      PD_TRACE_EXIT ( SDB__IXMEXT_GETROOT );
      return DMS_INVALID_EXTENT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_FNDSNG, "_ixmExtent::findSingle" )
   INT32 _ixmExtent::findSingle ( const ixmKey &key, const Ordering &order,
                                  dmsRecordID &rid, ixmIndexCB *indexCB ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_FNDSNG );

      BOOLEAN found = FALSE ;
      dmsRecordID dummyID ;
      ixmRecordID indexrid ;

      rc = _locate ( key, dummyID, order, indexrid, found, 1, indexCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to locate key" ) ;
         goto error ;
      }
      // loop until indexrid is invalid
      while ( TRUE )
      {
         if ( indexrid.isNull() )
         {
            indexrid.reset() ;
            break ;
         }
         // create extent for indexrid
         ixmExtent extent ( indexrid._extent, _pIndexSu ) ;
         // get the keynode
         const ixmKeyNode *kn = extent.getKeyNode(indexrid._slot) ;
         // skip unused keys (psuedo-deleted)
         if ( kn->isUsed() )
         {
            // compare the on-disk key and the one we are looking for, if they
            // match that means we got exists
            if ( ixmKey(extent.getKeyData(indexrid._slot)).woCompare (
                        key, order ) != 0 )
            {
               // if the key doesn't match, it means we don't have the key in
               // index, so we reset rid to -1,-1
               rid.reset() ;
               goto done ;
            }
            // if we find the key, let's set rid to kn->_rid
            rid = kn->_rid ;
            goto done ;
         }
         // advance to next keynode
         rc = extent.advance ( indexrid, 1 ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to advance" ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_FNDSNG, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_TRUNC, "_ixmExtent::truncate" )
   void _ixmExtent::truncate( ixmIndexCB *indexCB, dmsExtentID parent,
                              BOOLEAN &valid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_TRUNC );
      dmsExtentID childExtentID = DMS_INVALID_EXTENT ;
      UINT16 totalFreeSize = _pageSize - 1 - sizeof(ixmExtentHead) ;
      dmsPageMap *pPageMap = _pIndexSu->getPageMap( getMBID() ) ;

      rc = _validate( indexCB, parent ) ;
      if ( rc )
      {
         valid = FALSE ;
         PD_LOG( PDERROR, "Invalid index extent[%d], rc: %d", _me, rc ) ;
         goto done ;
      }
      else
      {
         valid = TRUE ;
         // starting from _right, until first keynode
         for ( INT32 i = (INT32)getNumKeyNode() ; i >= 0; i-- )
         {
            BOOLEAN childValid = TRUE ;
            childExtentID = getChildExtentID ((UINT16)i) ;
            if ( childExtentID != DMS_INVALID_EXTENT )
            {
               // truncated, the page is empty
               // we need to set _totalIndexFreeSpace before freeExtent()
               _pIndexSu->decStatFreeSpace( _extentHead->_mbID,
                                            totalFreeSize ) ;
               try
               {
                  ixmExtent( childExtentID, _pIndexSu ).truncate ( indexCB,
                                                                   _me,
                                                                   childValid) ;
                  // If the child extent is invalid, it's safer not to release
                  // it, and its space will be lost...
                  // It happend that the child extent is the index CB extent,
                  // and finally resulted in crash.
                  // This may happen during recovery after crash.
                  if ( childValid )
                  {
                     indexCB->freeExtent ( childExtentID ) ;
                     pPageMap->rmItem( childExtentID ) ;
                  }
               }
               catch ( std::exception &e )
               {
                  _pIndexSu->addStatFreeSpace( _extentHead->_mbID,
                                               totalFreeSize ) ;
                  PD_LOG( PDWARNING, "Occur exception:%s", e.what() ) ;
               }

               /*
               * To improve performance, not change the page except root
               */
               if ( isRoot() )
               {
                  setChildExtentID ( i, DMS_INVALID_EXTENT ) ;
               }
            }
         }
      }

      _pIndexSu->decStatFreeSpace( _extentHead->_mbID,
                                   _extentHead->_totalFreeSize ) ;

      /*
      * To improve performance, not change the page except root
      */
      if ( isRoot() )
      {
         ixmExtentHead *pHeader = _extRW.writePtr<ixmExtentHead>() ;
         pHeader->_totalKeyNodeNum = 0 ;
         pHeader->_beginFreeOffset = _pageSize - 1 ;
         pHeader->_totalFreeSize = totalFreeSize ;
      }
      _pIndexSu->addStatFreeSpace( _extentHead->_mbID, totalFreeSize ) ;

   done:
      PD_TRACE_EXIT ( SDB__IXMEXT_TRUNC );
   }

   // get the total number of elements in the index node and all children
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMEXT_COUNT, "_ixmExtent::count" )
   UINT64 _ixmExtent::count() const
   {
      PD_TRACE_ENTRY ( SDB_IXMEXT_COUNT );
      UINT64 totalCount = 0 ;
      const ixmKeyNode *kn = NULL ;

      for ( INT32 i = (INT32)getNumKeyNode() - 1 ; i >= 0 ; i-- )
      {
         kn = getKeyNode(i) ;
         if ( kn->isUsed() )
         {
            ++totalCount;
         }

         if ( kn->_left != DMS_INVALID_EXTENT )
         {
            totalCount += ixmExtent( kn->_left, _pIndexSu ).count() ;
         }
      }

      if ( DMS_INVALID_EXTENT != _extentHead->_right )
      {
         totalCount += ixmExtent(_extentHead->_right, _pIndexSu).count() ;
      }

      PD_TRACE_EXIT ( SDB_IXMEXT_COUNT ) ;
      return totalCount ;
   }

   BOOLEAN _ixmExtent::isStillValid( UINT16 mbID ) const
   {
      if ( IXM_EXTENT_EYECATCHER0 != _extentHead->_eyeCatcher[0] ||
           IXM_EXTENT_EYECATCHER1 != _extentHead->_eyeCatcher[1] )
      {
         return FALSE ;
      }
      else if ( _extentHead->_mbID != mbID )
      {
         return FALSE ;
      }
      else if ( DMS_EXTENT_FLAG_INUSE != _extentHead->_flag )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   // used in index cursor
   // currentKey is the key from current disk location that trying to be matched
   // prevKey is the key examed from previous run
   // keepFieldsNum is the number of fields from prevKey that should match the
   // currentKey (for example if the prevKey is {c1:1, c2:1}, and keepFieldsNum
   // = 1, that means we want to match c1:1 key for the current location.
   // Depends on if we have skipToNext set, if we do that means we want to skip
   // c1:1 and match whatever the next (for example c1:1.1); otherwise we want
   // to continue match the elements from matchEle )
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMEXT__KEYCMP, "_ixmExtent::_keyCmp" )
   INT32 _ixmExtent::_keyCmp ( const BSONObj &currentKey,
                               const BSONObj &prevKey,
                               INT32 keepFieldsNum, BOOLEAN skipToNext,
                               const VEC_ELE_CMP &matchEle,
                               const VEC_BOOLEAN &matchInclusive,
                               const Ordering &o, INT32 direction )
   {
      PD_TRACE_ENTRY ( SDB_IXMEXT__KEYCMP );
      BSONObjIterator ll ( currentKey ) ;
      BSONObjIterator rr ( prevKey ) ;
      VEC_ELE_CMP::const_iterator eleItr = matchEle.begin() ;
      VEC_BOOLEAN ::const_iterator incItr = matchInclusive.begin() ;
      UINT32 mask = 1 ;
      INT32 retCode = 0 ;
      // match keepFieldsNum fields
      for ( INT32 i = 0 ; i < keepFieldsNum; ++i, mask<<=1 )
      {
         BSONElement curEle = ll.next() ;
         BSONElement prevEle = rr.next() ;
         // skip those fields since we don't want to match them from
         // startstopkey iterator
         ++eleItr ;
         ++incItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
      }
      // if all the keepFieldsNum fields got matched, let's see if we want to
      // simply skip to next key, if so we don't need to match all other
      // elements
      // if that happen, the return value should be -direction, since we want to
      // return -1 if searching forward, otherwise return 1
      if ( skipToNext )
      {
         retCode = -direction ;
         goto done ;
      }
      // if all keepFieldsNum fields got matched, and we want to further match
      // startstopkey iterator, let's move on
      for ( ; ll.more(); mask<<=1 )
      {
         // curEle is always get from current key
         BSONElement curEle = ll.next() ;
         // now let's get the expected element from startstopkey iterator
         BSONElement prevEle = **eleItr ;
         ++eleItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
         // when getting here, that means the key matches expectation, then
         // let's see if we want inclusive predicate. If not we need to return
         // the negative of direction ( -1 for forward scan, otherwise 1 )
         if ( !*incItr )
         {
            retCode = -direction ;
            goto done ;
         }
         // when get here, it means key match AND inclusive
         ++incItr ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_IXMEXT__KEYCMP, retCode );
      return retCode ;
   }

   // bestIxmRID and resultExtent are the output
   // if rresultExtent != DMS_INVALID_EXTENT, it means there's child extent for
   // the best matched key and we should further dig into that node
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT__KEYFIND, "_ixmExtent::_keyFind" )
   INT32 _ixmExtent::_keyFind ( UINT16 l, UINT16 h, const BSONObj &prevKey,
                                INT32 keepFieldsNum, BOOLEAN skipToNext,
                                const VEC_ELE_CMP &matchEle,
                                const VEC_BOOLEAN &matchInclusive,
                                const Ordering &o, INT32 direction,
                                ixmRecordID &bestIxmRID,
                                dmsExtentID &resultExtent, _pmdEDUCB *cb ) const
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT__KEYFIND );
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      SDB_ASSERT ( l <= h, "low must be less than high" ) ;
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_READ, 1 ) ;

      INT32 low = ( INT32 )l ;
      INT32 high = ( INT32 )h ;
      INT32 m = 0 ;
      BufBuilder builder;

      while ( TRUE )
      {
         if ( low > high )
         {
            INT32 tmpSlot = direction > 0 ? low : high ;
            if ( tmpSlot < (INT32)l || tmpSlot > (INT32)h )
            {
               bestIxmRID.reset() ;
            }
            else
            {
               bestIxmRID._extent = _me ;
               bestIxmRID._slot = tmpSlot ;
            }
            resultExtent = getChildExtentID( low ) ;
            goto done ;
         }
         // no need to worry about 16 bit overflow, since each page is only
         // 65536 and each key slot will always > 2 bytes, so h+l won't hit
         // 0xFFFF
         m = ( low + high ) / 2 ;
         const CHAR *data = getKeyData ( m ) ;
         if ( !data )
         {
            PD_LOG ( PDERROR, "slot %d doesn't have matching key", m ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }

         builder.reset();
         INT32 r = _keyCmp ( ixmKey(data).toBson(&builder), prevKey, keepFieldsNum,
                             skipToNext, matchEle, matchInclusive, o, direction);
         if ( r < 0 )
         {
            low = m + 1 ;
         }
         else if ( r > 0 )
         {
            high = m - 1 ;
         }
         else
         {
            if ( direction < 0 )
            {
               low = m + 1 ;
            }
            else
            {
               high = m - 1 ;
            }
         }
      } // while ( TRUE )
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT__KEYFIND, rc );
      return rc ;
   error :
      goto done ;
   }

   // This function locate the key by
   // 1) check if the key is smaller or greater than first or last key in
   // forward and backward search
   // 2) if so it will jump into the child if exist
   // 3) check if the key is greater or smaller than the last or first key in
   // forward and backward search
   // 4) if so it will jump into the child if exist
   // 5) check if the key is in the page, if the key got child it will jump into
   // it
   // the rid may or may not be changed by the following condition
   // A) forward scan
   //   A.1) if first key is greater than prevKey, rid = first key rid and scan
   //   most left pointer
   //   A.2) if last key is smaller than prevKey, rid unchange and scan most
   //   right pointer
   //   A.3) in other condition, do binary search using keyFind and scan child
   // B) backward scan
   //   B.1) if last key is smaller than prevKey, rid = last key rid and scan
   //   most right pointer
   //   B.2) if first key is greater than prevKey, rid unchange and scan most
   //   left pointer
   //   B.3) in other condition, do binary search using keyFind and scan child
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_KEYLOCATE, "_ixmExtent::keyLocate" )
   INT32 _ixmExtent::keyLocate ( ixmRecordID &rid, const BSONObj &prevKey,
                                 INT32 keepFieldsNum, BOOLEAN skipToNext,
                                 const VEC_ELE_CMP &matchEle,
                                 const VEC_BOOLEAN &matchInclusive,
                                 const Ordering &o, INT32 direction,
                                 _pmdEDUCB *cb ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_KEYLOCATE );
      UINT16 l, h, z ;
      const CHAR *data = NULL ;
      INT32 result ;
      dmsExtentID childExtentID ;
      SDB_ASSERT ( direction == 1 || direction == -1, "direction must be "
                   "either 1 or -1" ) ;
      // empty root?
      if ( 0 == getNumKeyNode() )
      {
         rid.reset() ;
         goto done ;
      }

      // keep going until find the smallest/biggest target
      l = 0 ;
      h = getNumKeyNode() - 1 ;
      // when direction = 1, z = 0
      // when direction = -1, z = h
      z = (1-direction)/2*h ;
      data = getKeyData ( z ) ;
      if ( !data )
      {
         PD_LOG ( PDERROR, "slot %d doesn't have matching key", z ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // first let's compare the extream condition for first or last record
      // (depends on forward or backward scan)
      result = _keyCmp ( ixmKey(data).toBson(), prevKey, keepFieldsNum,
                         skipToNext, matchEle, matchInclusive, o, direction ) ;
      // if we search forward and first key is greater than expected, or if we
      // search backward and last key is smaller than expected, let's go down
      // a tree level and continue search if possible

      // if we want to exam the one before first ( in forward search ), or the
      // one after last ( in backward search )
      if ( direction * result >= 0 )
      {
         // set best match here. This part should be done here since the first
         // key in this page is less than expected in forward phase ( or greater
         // than expected in backward phase), so this key will be a better match
         rid._extent = _me ;
         rid._slot   = z ;
         if ( direction > 0 )
         {
            // for forward scan, this code path means the requested key is
            // smaller than the lowest
            childExtentID = getChildExtentID(0) ;
         }
         else
         {
            // for backward scan, this code path means the requested key is
            // greater than the last
            childExtentID = _extentHead->_right ;
         }
         // is child exist? if not let's just return the best match
         if ( DMS_INVALID_EXTENT != childExtentID )
         {
            // otherwise get the child and recursively call keyLocate
            ixmExtent nextExtent ( childExtentID, _pIndexSu ) ;
            rc = nextExtent.keyLocate ( rid, prevKey, keepFieldsNum, skipToNext,
                                        matchEle, matchInclusive, o, direction,
                                        cb );
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to run keyLocate from extent %d",
                        childExtentID ) ;
               goto error ;
            }
         }
         goto done ;
      }

      // now let's check another extream condition, that the last and first key
      // in the page for forward and backward condition
      data = getKeyData( h-z ) ;
      if ( !data )
      {
         PD_LOG ( PDERROR, "slot %d doesn't have matching key", z ) ;
         dumpIndexExtentIntoLog () ;
         rc = SDB_SYS ;
         goto error ;
      }
      // first let's compare the extream condition for first or last record
      // (depends on forward or backward scan)
      result = _keyCmp ( ixmKey(data).toBson(), prevKey, keepFieldsNum,
                         skipToNext, matchEle, matchInclusive, o, direction ) ;
      // if we search forward and last key is less than expected, or if we
      // search backward and first key is greater than expected, let's go down
      // a tree level and continue search if possible
      if ( direction * result < 0 )
      {
         // in this case, be careful we shouldn't overwrite rid as it's less than
         // our expect in forward phase ( or greater than our expect in backward
         // phase ), so we should go into the child if exist. If no child exist
         // let's simply return without touching rid
         // get child
         if ( direction > 0 )
         {
            // for forward scan, this code path means the requested key is
            // greater than the largest
            childExtentID = _extentHead->_right ;
         }
         else
         {
            // for backward scan, this code path means the requested key is
            // smaller than the lowest
            childExtentID = getChildExtentID(0) ;
         }
         // is child exist? if not let's just return the best match
         if ( DMS_INVALID_EXTENT != childExtentID )
         {
            // otherwise get the child and recursively call keyLocate
            ixmExtent nextExtent ( childExtentID, _pIndexSu ) ;
            rc = nextExtent.keyLocate ( rid, prevKey, keepFieldsNum, skipToNext,
                                        matchEle, matchInclusive, o, direction,
                                        cb );
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to run keyLocate from extent %d",
                        childExtentID ) ;
               goto error ;
            }
         }
         goto done ;
      }

      // otherwise the key must fall in this page
      rc = _keyFind ( l, h, prevKey, keepFieldsNum, skipToNext, matchEle,
                      matchInclusive, o, direction, rid, childExtentID, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to run keyFind from extent %d", _me ) ;
         goto error ;
      }
      if ( DMS_INVALID_EXTENT != childExtentID )
      {
         ixmExtent nextExtent ( childExtentID, _pIndexSu ) ;
         rc = nextExtent.keyLocate ( rid, prevKey, keepFieldsNum, skipToNext,
                                     matchEle, matchInclusive, o, direction,
                                     cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to run keyLocate from extent %d",
                     childExtentID ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_KEYLOCATE, rc );
      return rc ;
   error :
      goto done ;
   }

   // get the rid for the smallest/greatest key matching prevKey (forward and
   // backward scan), if there's no such thing exist, rid is reset.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_KEYADVANCE, "_ixmExtent::keyAdvance" )
   INT32 _ixmExtent::keyAdvance ( ixmRecordID &rid, const BSONObj &prevKey,
                                 INT32 keepFieldsNum, BOOLEAN skipToNext,
                                 const VEC_ELE_CMP &matchEle,
                                 const VEC_BOOLEAN &matchInclusive,
                                 const Ordering &o, INT32 direction,
                                 _pmdEDUCB *cb ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_KEYADVANCE );
      UINT16 l = 0, h = 0 ;
      BOOLEAN currentLevel = FALSE ;
      dmsExtentID childExtentID = DMS_INVALID_EXTENT ;
      dmsExtentID parentExtentID = DMS_INVALID_EXTENT ;
      const CHAR *data = NULL ;

      // first let's compare the last/first item (forward and backward scan)
      // with the expect key
      if ( direction > 0 )
      {
         // for forward scan, compare if the latest key is greater than the
         // target
         l = rid.isNull()?(0):rid._slot ;
         h = getNumKeyNode() - 1 ;
         data = getKeyData( h ) ;
         if ( !data )
         {
            PD_LOG ( PDERROR, "slot %d doesn't have matching key", h ) ;
            dumpIndexExtentIntoLog () ;
            rc = SDB_SYS ;
            goto error ;
         }
         currentLevel = ( _keyCmp ( ixmKey(data).toBson(), prevKey,
                                    keepFieldsNum, skipToNext, matchEle,
                                    matchInclusive, o, direction) >= 0 ) ;
      }
      else
      {
         // for backward scan, compare if the first key is smaller than the
         // target
         l = 0 ;
         h = rid.isNull()?(getNumKeyNode()-1):rid._slot ;
         data = getKeyData ( l ) ;
         if ( !data )
         {
            PD_LOG ( PDERROR, "slot %d doesn't have matching key", l ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         currentLevel = ( _keyCmp ( ixmKey(data).toBson(), prevKey,
                                    keepFieldsNum, skipToNext, matchEle,
                                    matchInclusive, o, direction) <= 0 ) ;
      }
      // if the latest/first key is greater/smaller than the target, that means
      // we don't need to traversal up. So let's simply call keyFind in the
      // current level
      // if keyFind get us a slot without child, that's our target then.
      // Otherwise we have to keep going to drill down
      if ( currentLevel )
      {
         rc = _keyFind ( l, h, prevKey, keepFieldsNum, skipToNext,
                         matchEle, matchInclusive, o, direction,
                         rid, childExtentID, cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to keyFind in extent %d", _me ) ;
            goto error ;
         }

         if ( DMS_INVALID_EXTENT == childExtentID )
         {
            goto done ;
         }
         else
         {
            ixmExtent childExtent ( childExtentID, _pIndexSu ) ;
            rc = childExtent.keyLocate ( rid, prevKey, keepFieldsNum,
                                         skipToNext, matchEle,
                                         matchInclusive, o, direction, cb ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to keyLocate in extent %d",
                        childExtentID ) ;
               goto error ;
            }
         }
      }
      else if ( (parentExtentID = getParent()) != DMS_INVALID_EXTENT )
      {
         // we need to go up, so let's first reset rid
         rid.reset() ;
         // if we get here, that means the target is greater or smaller than
         // latest/first key (forward and backward). That means the key is not
         // within the current node, and we should traversal up
         ixmExtent parentExtent ( parentExtentID, _pIndexSu ) ;
         rc = parentExtent.keyAdvance ( rid, prevKey, keepFieldsNum,
                                        skipToNext, matchEle, matchInclusive,
                                        o, direction, cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to keyAdvance in extent %d",
                     parentExtentID ) ;
            goto error ;
         }
      }
      else
      {
         // we have to reset rid here, so that if there's no further keys
         // in index scan let's return NULL
         rid.reset() ;
         // if we are root?
         rc = keyLocate ( rid, prevKey, keepFieldsNum, skipToNext, matchEle,
                          matchInclusive, o, direction, cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to keyLocate in extent %d", _me ) ;
            goto error ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__IXMEXT_KEYADVANCE, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMEXT_DMPINXEXT2LOG, "_ixmExtent::dumpIndexExtentIntoLog" )
   INT32 _ixmExtent::dumpIndexExtentIntoLog () const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__IXMEXT_DMPINXEXT2LOG );
      // 1MB buffer should be enough for output
      INT32 indexExtentDumpBufferSize = 1024 * 1024 ;
      std::deque<dmsExtentID> childExtents ;
      CHAR *pBuffer = (CHAR*)SDB_OSS_MALLOC ( indexExtentDumpBufferSize ) ;
      PD_CHECK ( pBuffer, SDB_OOM, error, PDERROR,
                 "Failed to allocate memory for dump buffer" ) ;
      rc = dmsDump::dumpIndexExtent ( (CHAR*)_extentHead,
                                       _pageSize,
                                       pBuffer, indexExtentDumpBufferSize,
                                       NULL,
                                       DMS_SU_DMP_OPT_HEX |
                                       DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                       DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                       DMS_SU_DMP_OPT_FORMATTED,
                                       childExtents,
                                       TRUE ) ;
      if ( rc > 0 )
      {
         PD_LOG ( PDERROR, "Index Page Dump:\n%s", pBuffer ) ;
      }
      rc = SDB_OK ;

   done :
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
      }
      PD_TRACE_EXITRC ( SDB__IXMEXT_DMPINXEXT2LOG, rc );
      return rc ;
   error :
      goto done ;
   }

}


