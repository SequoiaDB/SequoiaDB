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
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNIXSCANNER_HPP__
#define RTNIXSCANNER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ixm.hpp"
#include "ixmExtent.hpp"
#include "rtnPredicate.hpp"
#include "pmdEDU.hpp"
#include "../bson/ordering.h"
#include "../bson/oid.h"
#include "dms.hpp"
#include "monCB.hpp"
#include <set>

namespace engine
{
   class _dmsStorageUnit ;

   #define RTN_IXSCANNER_DEDUPBUFSZ_DFT      (1024*1024)

   /*
      _rtnIXScanner define
   */
   class _rtnIXScanner : public SDBObject
   {
   private :
      ixmIndexCB *_indexCB ;
      rtnPredicateList *_predList ;
      rtnPredicateListIterator _listIterator ;
      Ordering _order ;
      _dmsStorageUnit *_su ;
      monContextCB *_pMonCtxCB ;
      _pmdEDUCB *_cb ;
      INT32 _direction ;

      ixmRecordID _curIndexRID ;
      BOOLEAN _init ;
      UINT64 _dedupBufferSize ;
      set<dmsRecordID> _dupBuffer ;

      BSONObj _savedObj ;
      dmsRecordID _savedRID ;

      BSONObj _curKeyObj ;

      OID _indexOID ;
      dmsExtentID _indexCBExtent ;
      dmsExtentID _indexLID ;

   public :
      _rtnIXScanner ( ixmIndexCB *indexCB, rtnPredicateList *predList,
                      _dmsStorageUnit *su, _pmdEDUCB *cb ) ;
      ~_rtnIXScanner () ;

      void setMonCtxCB ( monContextCB *monCtxCB )
      {
         _pMonCtxCB = monCtxCB ;
      }

      BSONObj getSavedObj () const
      {
         return _savedObj ;
      }

      dmsRecordID getSavedRID () const
      {
         return _savedRID ;
      }

      ixmRecordID getCurIndexRID () const
      {
         return _curIndexRID ;
      }

      const BSONObj& getCurKeyObj () const
      {
         return _curKeyObj ;
      }

      INT32 getDirection () const
      {
         return _direction ;
      }

      rtnPredicateList* getPredicateList ()
      {
         return _predList ;
      }

      INT32 compareWithCurKeyObj ( const BSONObj &keyObj ) const
      {
         return _curKeyObj.woCompare( keyObj, _order, false ) * _direction ;
      }

      void reset()
      {
         _curIndexRID.reset() ;
         _listIterator.reset() ;
         _dupBuffer.clear() ;
         _init    = FALSE ;
      }
      INT32 pauseScan( BOOLEAN isReadOnly = TRUE ) ;
      INT32 resumeScan( BOOLEAN isReadOnly = TRUE ) ;

      INT32 advance ( dmsRecordID &rid, BOOLEAN isReadOnly = TRUE ) ;
      INT32 relocateRID () ;
      INT32 relocateRID ( const BSONObj &keyObj, const dmsRecordID &rid ) ;
   } ;
   typedef class _rtnIXScanner rtnIXScanner ;

}

#endif //RTNIXSCANNER_HPP__

