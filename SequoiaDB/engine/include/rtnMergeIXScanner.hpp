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

   Source File Name = rtnMergeIXScanner.hpp

   Descriptive Name = RunTime Index Scanner Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/09/2018  YXC Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNMERGEIXSCANNER_HPP__
#define RTNMERGEIXSCANNER_HPP__

#include "rtnIXScanner.hpp"

using namespace bson ;

namespace engine
{

   // Index merge scanner is used to merge results from different scanner.
   // For instance, we have a scanner to traverse on disk index trees; we
   // also have in memory index tree to track previously committed indexes.
   // As part of index scan, if a transaction require RC level isolation,
   // it will use this merge scanner to retrieve the last committed version
   // record including the data on disk and in memory. In this case, we can
   // instantiate the two scanner member to rtnDiskScanner and
   // rtnMemIXTreeScanner
   class _rtnMergeIXScanner : public _rtnIXScanner
   {
   public:
      _rtnMergeIXScanner( ixmIndexCB *pIndexCB,
                          rtnPredicateList *predList,
                          _dmsStorageUnit  *su,
                          _pmdEDUCB        *cb,
                          BOOLEAN indexCBOwnned = FALSE ) ;

      virtual ~_rtnMergeIXScanner() ;

      void     setSubScannerType( IXScannerType leftType,
                                  IXScannerType rightType ) ;

   /// Interface
   public:
      virtual INT32 init() ;
      virtual void  setReadonly( BOOLEAN isReadonly ) ;

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

      virtual const BSONObj*  getCurKeyObj() const ;
      virtual const dmsRecordID& getSavedRID () const { return _savedRID ; }
      virtual const BSONObj*  getSavedObj () const { return &_savedObj ; }
   
      virtual INT32           isCursorSame( const BSONObj &saveObj,
                                            const dmsRecordID &saveRID,
                                            BOOLEAN &isSame ) ;

   protected:
      virtual INT32 relocateRID( BOOLEAN &found ) ;
      virtual rtnPredicateListIterator*   getPredicateListInterator() ;

   protected:
      const BSONObj*       getSavedObjFromChild() const ;
      const dmsRecordID&   getSavedRIDFromChild() const ;

      INT32                _createScanner( IXScannerType type,
                                           _rtnIXScanner *&pScanner ) ;

   private:

      // flag indicating if the last index used was from left scanner
      INT32                   _fromDir ;

      dmsRecordID             _lrid ;   // for leftScan return
      dmsRecordID             _rrid ;   // for rightScan return

      // After releas mbLatch, another session may change the index structure
      // So everytime before pause, we must store the current index key value
      // and the rid. We must keep this information in shared structure in
      // merge scan because the memory tree scan might not exist in previous
      // interval and now the mem tree was created due to new update. We need
      // the saved information to locate the key.
      // After resume, verify if it's still remaining the same. If the
      // object/rid doens't match, something must have changed. we should
      // relocate key.
      // Because malloc is involved during the saving, we should only setup
      // the BSONObj during pause(), otherwise it would affect runtime perf.
      BSONObj                 _savedObj ;
      dmsRecordID             _savedRID ;

      // pointer to left and right scanners to merge
      rtnIXScanner            *_leftIXScanner ;
      rtnIXScanner            *_rightIXScanner ;

      IXScannerType           _leftType ;
      IXScannerType           _rightType ;

      BOOLEAN                 _leftEnabled ;
      BOOLEAN                 _rightEnabled ;

   } ;
   typedef class _rtnMergeIXScanner rtnMergeIXScanner ;
}

#endif //RTNMERGEIXSCANNER_HPP__

