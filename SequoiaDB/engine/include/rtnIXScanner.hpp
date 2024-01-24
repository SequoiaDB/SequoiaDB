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

   Source File Name = rtnIXScanner.hpp

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
#ifndef RTNIXSCANNER_HPP__
#define RTNIXSCANNER_HPP__

#include "ixm.hpp"
#include "monCB.hpp"
#include "rtnScanner.hpp"
#include "rtnPredicate.hpp"
#include "../bson/ordering.h"
#include "../bson/oid.h"
#include "utilPooledAutoPtr.hpp"

using namespace bson ;

namespace engine
{
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _pmdEDUCB ;

   /*
      _rtnScannerSharedInfo define
   */
   class _rtnScannerSharedInfo : public SDBObject
   {
   public:
      _rtnScannerSharedInfo() ;
      ~_rtnScannerSharedInfo() ;

      BOOLEAN     insert( const dmsRecordID &rid ) ;
      BOOLEAN     remove( const dmsRecordID &rid ) ;
      BOOLEAN     exists( const dmsRecordID &rid ) const ;

      void        clear() ;

      BOOLEAN     isUpToLimit() const ;
      UINT64      getSize() const ;

   private:
      // need to store a duplicate buffer for dmsRecordID for each ixscan,
      // because each record may be refered by more than one index key (say
      // {c1:[1,2,3,4,5]}, there will be 5 index keys pointing to the same
      // record), so index scan may return the same record more than one time,
      // which should be avoided. Thus we keep a set ( maybe we can further
      // improve performance by customize data structure ) to record id that
      // already scanned.
      // note that a record id will not be changed during update because it will
      // use overflowed record with existing record head
      // we used a pointer here because the memIXTreeScanner are likely used
      // together with another scanner (like disk scanner), we should share
      // the same dupBuffer.
      SET_RECORDID              _setDuplicate ;

   } ;
   typedef _rtnScannerSharedInfo rtnScannerSharedInfo ;

   /*
      _rtnIXScanner define
      This is a super class for all index scanners to inherit from
   */
   class _rtnIXScanner : public _rtnScanner
   {
   public:
      _rtnIXScanner( ixmIndexCB *pIndexCB,
                     rtnPredicateList *predList,
                     _dmsStorageUnit  *su,
                     _dmsMBContext    *mbContext,
                     BOOLEAN           isAsync,
                     _pmdEDUCB        *cb,
                     BOOLEAN indexCBOwned = FALSE ) ;

      virtual ~_rtnIXScanner() ;

      virtual rtnScannerStorageType getStorageType() const
      {
         return SCANNER_TYPE_INDEX ;
      }

      BOOLEAN     isReadonly() const ;

      void        setShareInfo( rtnScannerSharedInfo *pInfo ) ;
      BOOLEAN     isShareInfoFromSelf() const ;

      virtual BOOLEAN removeDuplicatRID( const dmsRecordID &rid ) ;
      virtual dmsExtentID getIdxLID() const ;

      const OID&  getIdxOID() const ;
      INT32       getDirection () const ;

      rtnPredicateList*       getPredicateList () ;
      rtnScannerSharedInfo*   getSharedInfo() ;

      _dmsStorageUnit*        getSu() ;
      _dmsMBContext*          getMBContext() ;
      ixmIndexCB*             getIndexCB() ;
      BOOLEAN                 getIndexCBOwned() const ;
      _pmdEDUCB*              getEDUCB() ;

      INT32       compareWithCurKeyObj( const BSONObj &keyObj ) const ;
      INT32       syncPredStatus( _rtnIXScanner *source ) ;

      BOOLEAN                isIndexCover() const ;
      void                   setIndexCover( const BOOLEAN indexCover ) ;
      ixmIndexCover&         getIndex() ;

   /// Interface
   public:
      virtual INT32 init() ;
      virtual void  setReadonly( BOOLEAN isReadonly ) ;

      virtual INT32 relocateRID( const BSONObj &keyObj,
                                 const dmsRecordID &rid ) = 0 ;

      virtual BOOLEAN         isAvailable() const = 0 ;
      virtual const BSONObj*  getCurKeyObj() const = 0 ;
      virtual const dmsRecordID& getSavedRID () const = 0 ;
      virtual const BSONObj*  getSavedObj () const = 0 ;

   protected:
      virtual INT32 _relocateRID( BOOLEAN &found ) = 0 ;
      virtual rtnPredicateListIterator*   _getPredicateListInterator() = 0 ;

   protected:
      BOOLEAN                 _insert2Dup( const dmsRecordID &rid ) ;

   protected:
      ixmIndexCB              *_indexCB ;
      BOOLEAN                 _owned ;
      rtnPredicateList        *_pPredList ;
      rtnScannerSharedInfo    *_pInfo ;

      dmsExtentID             _indexLID ;
      dmsExtentID             _indexCBExtent ;
      OID                     _indexOID ;
      Ordering                _order ;

   private:
      BOOLEAN                 _isReadonly ;
      rtnScannerSharedInfo    _sharedInfo ;
      BOOLEAN                 _indexCover ;
      ixmIndexCover           _index ;
   } ;
   typedef class _rtnIXScanner rtnIXScanner ;

}

#endif //RTNIXSCANNER_HPP__

