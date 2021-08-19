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

   Source File Name = rtnMemIXScanner.hpp

   Descriptive Name = RunTime Index Scanner Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2012  YXC Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNMEMIXTREESCANNER_HPP__
#define RTNMEMIXTREESCANNER_HPP__

#include "rtnIXScanner.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "dpsTransCB.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnMemIXTreeScanner define
      This scanner is used to scan the in memory index tree which holds
      old copy (last committed) of index values. During runtime, index scan
      will merge the result from this scan with the rtnDiskIXScanner.
      See rtnMergeIXScanner for details
   */
   class _rtnMemIXTreeScanner : public _rtnIXScanner
   {
   public:
      _rtnMemIXTreeScanner( ixmIndexCB *pIndexCB,
                            rtnPredicateList *predList,
                            _dmsStorageUnit  *su,
                            _pmdEDUCB        *cb,
                            BOOLEAN indexCBOwnned = FALSE ) ;
 
      virtual ~_rtnMemIXTreeScanner() ;

   /// Interface
   public:
      virtual INT32 init() ;

      virtual INT32 advance ( dmsRecordID &rid ) ;
      virtual INT32 resumeScan( BOOLEAN *pIsCursorSame = NULL ) ;
      virtual INT32 pauseScan() ;

      virtual INT32 relocateRID( const BSONObj &keyObj,
                                 const dmsRecordID &rid ) ;

      virtual BOOLEAN         isAvailable() const ;
      virtual IXScannerType   getType() const ;
      virtual IXScannerType   getCurScanType() const ;
      virtual void            disableByType( IXScannerType type ) ;
      virtual INT32           getLockModeByType( IXScannerType type ) const ;

      virtual const BSONObj*  getCurKeyObj() const { return &_curKeyObj ; }
      virtual const dmsRecordID& getSavedRID () const { return _savedRID ; }
      virtual const BSONObj*  getSavedObj () const { return &_savedObj ; }

      virtual INT32           isCursorSame( const BSONObj &saveObj,
                                            const dmsRecordID &saveRID,
                                            BOOLEAN &isSame ) ;

   protected:
      virtual INT32 relocateRID( BOOLEAN &found )  ;
      virtual rtnPredicateListIterator*   getPredicateListInterator() ;

   protected:
      void                    reset() ;

   private :
      rtnPredicateListIterator   _listIterator ;
      dpsTransCB                 *_pTransCB ;
      globIdxID                  _gid ;

      // set to true by relocateRID or first run of advance.
      // indicate if have done first run which will call keyLocate to point
      // _curIndexIter to a best starting point
      BOOLEAN                    _init ;
      BOOLEAN                    _available ;

      // Track the current index iterator location under proper protection.
      // This could become invalid if the index is erased from the tree
      // even worse case, if someone reused the same memory, they could
      // set some other information. So this iterator only gives a quick
      // access to previous locationr. As soon as we released the tree latch
      // you can no longer trust the information. We have to re-validate the
      // content (key information) in second against _curKeyObj. If they don't
      // match, we have to do a relocateRID based on savedObj.
      // If we hold tree latch, after calling relocateRID, we can simply
      // manipulate the iterator towards the direction. We can always store
      // current key value in _curKeyObj for computation/comparison purpose.
      INDEX_TREE_CPOS            _curIndexPos ;
      BSONObj                    _curKeyObj ;

      // After releasing mbLatch and treeLatch, another session may change
      // the index structure. So everytime before pause, we must store
      // the current index key value into _savedInMemObj, and the rid into
      // dmsRecordID. After resume, verify if it's still remaining the same.
      // If the object doens't match the obj/rid in LRBHdr, something must
      // goes changed and we should relocate
      // Because malloc is involved during the saving, we should only setup
      // the BSONObj during pause(), otherwise it would affect runtime perf.
      BSONObj                    _savedObj ;
      dmsRecordID                _savedRID ;

      // pointer to the in memory index tree. It's hanging off dpsTransCB
      preIdxTreePtr              _memIdxTree ;
      // Protocal to take and hold tree latch:
      // During tree scan (advance), we will hold the treeLatch so that commit
      // or rollback won't remove the nodes from the tree while scanner is
      // walking over the tree. The latch is aquired by resume and released
      // by pause.
      BOOLEAN                    _treeLatchHeld ;

   } ;
   typedef class _rtnMemIXTreeScanner rtnMemIXTreeScanner ;

}

#endif //RTNMEMIXTREESCANNER_HPP__

