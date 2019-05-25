/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dmsIndexBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_BUILDER_HPP_
#define DMS_INDEX_BUILDER_HPP_

#include "dmsStorageBase.hpp"
#include "ixmKey.hpp"
#include "dmsExtent.hpp"

namespace engine
{
   class _dmsMBContext ;
   class _dmsStorageIndex ;
   class _dmsStorageData ;
   class _pmdEDUCB ;
   class _ixmIndexCB ;
   class _dmsMBContext ;

   class _dmsIndexBuilder: public SDBObject
   {
   public:
      _dmsIndexBuilder( _dmsStorageIndex* indexSU,
                        _dmsStorageData* dataSU,
                        _dmsMBContext* mbContext,
                        _pmdEDUCB* eduCB,
                        dmsExtentID indexExtentID ) ;
      virtual ~_dmsIndexBuilder() ;
      INT32 build() ;

   protected:
      virtual INT32 _build() = 0 ;
      virtual INT32 _onInit() ;

      #define _DMS_SKIP_EXTENT 1
      virtual INT32 _beforeExtent() ;
      virtual INT32 _afterExtent() ;

      INT32 _getKeySet( ossValuePtr recordDataPtr, BSONObjSet& keySet ) ;
      INT32 _insertKey( ossValuePtr recordDataPtr, const dmsRecordID &rid, const Ordering& ordering ) ;
      INT32 _insertKey( const ixmKey &key, const dmsRecordID &rid, const Ordering& ordering ) ;
      INT32 _checkIndexAfterLock( INT32 lockType ) ;

   private:
      INT32 _init() ;
      INT32 _finish() ;

   protected:
      _dmsStorageIndex*  _suIndex ;
      _dmsStorageData*   _suData ;
      _dmsMBContext*     _mbContext ;
      _pmdEDUCB*         _eduCB ;
      dmsExtentID        _indexExtentID ;
      dmsExtentID        _indexLID ;
      _ixmIndexCB*       _indexCB ;
      OID                _indexOID ;
      dmsExtentID        _scanExtLID ;
      dmsExtentID        _currentExtentID ;
      dmsExtRW           _extRW ;
      const dmsExtent*   _extent ;
      BOOLEAN            _unique ;
      BOOLEAN            _dropDups ;

   public:
      static _dmsIndexBuilder* createInstance( _dmsStorageIndex* indexSU,
                                               _dmsStorageData* dataSU,
                                               _dmsMBContext* mbContext,
                                               _pmdEDUCB* eduCB,
                                               dmsExtentID indexExtentID,
                                               INT32 sortBufferSize,
                                               UINT16 indexType ) ;
      static void releaseInstance( _dmsIndexBuilder* builder ) ;
   } ;
   typedef class _dmsIndexBuilder dmsIndexBuilder ;
}

#endif /* DMS_INDEX_BUILDER_HPP_ */

