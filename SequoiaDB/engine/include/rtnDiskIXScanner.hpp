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

   Source File Name = rtnDiskIXScanner.hpp

   Descriptive Name = RunTime Disk Index Scanner Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNDISKIXSCANNER_HPP__
#define RTNDISKIXSCANNER_HPP__

#include "rtnIXScanner.hpp"
#include "interface/ICursor.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnDiskIXScanner define
      A scanner to traverse through on disk index pages/slots
   */
   class _rtnDiskIXScanner : public _rtnIXScanner
   {
   public:
      _rtnDiskIXScanner ( ixmIndexCB *pIndexCB,
                          rtnPredicateList *predList,
                          _dmsStorageUnit  *su,
                          _dmsMBContext    *mbContext,
                          BOOLEAN          isAsync,
                          _pmdEDUCB        *cb,
                          BOOLEAN indexCBOwnned = FALSE ) ;

      virtual ~_rtnDiskIXScanner () ;

   /// Interface
   public:
      virtual INT32 advance ( dmsRecordID &rid ) ;
      virtual INT32 resumeScan( BOOLEAN &isCursorSame ) ;
      virtual INT32 pauseScan() ;
      virtual INT32 checkSnapshotID( BOOLEAN &isCursorSame ) ;

      virtual void  setMonCtxCB( _monContextCB *monCtxCB ) ;

      virtual INT32 relocateRID( const BSONObj &keyObj,
                                 const dmsRecordID &rid ) ;

      virtual BOOLEAN         isAvailable() const ;
      virtual rtnScannerType  getType() const ;
      virtual rtnScannerType  getCurScanType() const ;
      virtual void            disableByType( rtnScannerType type ) ;
      virtual BOOLEAN         isTypeEnabled( rtnScannerType type ) const ;
      virtual INT32           getIdxLockModeByType( rtnScannerType type ) const ;

      virtual const BSONObj*  getCurKeyObj() const { return &_curKeyObj ; }
      virtual const dmsRecordID& getSavedRID () const { return _savedRID ; }
      virtual const BSONObj*  getSavedObj () const { return &_savedObj ; }

      virtual BOOLEAN canPrefetch() const
      {
         return _cursorPtr ? _cursorPtr->isAsync() : FALSE ;
      }

      virtual IStorageSession *getSession()
      {
         return _cursorPtr ? _cursorPtr->getSession() : nullptr ;
      }

   protected:
      virtual INT32 _relocateRID( BOOLEAN &found ) ;
      virtual rtnPredicateListIterator*   _getPredicateListInterator() ;

   protected:
      void                    _reset() ;

      INT32                   _relocateRID( const BSONObj &keyObj,
                                            const dmsRecordID &rid,
                                            INT32 direction,
                                            BOOLEAN &isFound ) ;

      INT32                   _firstInit() ;
      INT32                   _advance() ;
      INT32                   _fetchNext( dmsRecordID &rid, BOOLEAN &needAdvance ) ;

   private:
      rtnPredicateListIterator   _listIterator ;
      monContextCB               *_pMonCtxCB ;

      // Flag to indicate if we need to locate/relocate the key
      BOOLEAN                    _init ;

      // after the caller release X latch, another session may change the index
      // structure. So everytime before releasing X latch, the caller must store
      // the current obj, and then verify if it's still remaining the same after
      // next check. If the object doens't match the on-disk obj, something must
      // goes changed and we should relocate
      BSONObj                  _savedObj ;
      dmsRecordID              _savedRID ;

      BSONObj                  _curKeyObj ;
      dmsRecordID              _relocatedRID ;

      BufBuilder               _builder ;

      std::unique_ptr<IIndexCursor> _cursorPtr ;
   } ;
   typedef class _rtnDiskIXScanner rtnDiskIXScanner ;

}

#endif //RTNDISKIXSCANNER_HPP__

