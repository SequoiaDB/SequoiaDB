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

   Source File Name = dmsIndexBuilderImpl.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_BUILDER_IMPL_HPP_
#define DMS_INDEX_BUILDER_IMPL_HPP_

#include "dmsIndexBuilder.hpp"
#include "dmsExtDataHandler.hpp"
#include "../bson/ordering.h"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _dmsIndexOnlineBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexOnlineBuilder( _dmsStorageIndex* indexSU,
                              _dmsStorageData* dataSU,
                              _dmsMBContext* mbContext,
                              _pmdEDUCB* eduCB,
                              dmsExtentID indexExtentID ) ;
      ~_dmsIndexOnlineBuilder() ;

   private:
      INT32 _build() ;
   } ;
   typedef class _dmsIndexOnlineBuilder dmsIndexOnlineBuilder ;

   class _dmsIxmKeySorter ;

   class _dmsIndexSortingBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexSortingBuilder( _dmsStorageIndex* indexSU,
                               _dmsStorageData* dataSU,
                               _dmsMBContext* mbContext,
                               _pmdEDUCB* eduCB,
                               dmsExtentID indexExtentID,
                               INT32 sortBufferSize ) ;
      ~_dmsIndexSortingBuilder() ;

   private:
      INT32 _init() ;
      INT32 _fillSorter() ;
      INT32 _insertKeys( const Ordering& ordering ) ;
      INT32 _build() ;

   private:
      _dmsIxmKeySorter* _sorter ;
      INT64             _bufSize ;
      INT64             _bufExtSize ;
      BOOLEAN           _eoc ;
   } ;
   typedef class _dmsIndexSortingBuilder dmsIndexSortingBuilder ;

   class _dmsIndexExtBuilder : public _dmsIndexBuilder
   {
   public:
      _dmsIndexExtBuilder( _dmsStorageIndex* indexSU,
                           _dmsStorageData* dataSU,
                           _dmsMBContext* mbContext,
                           _pmdEDUCB* eduCB,
                           dmsExtentID indexExtentID ) ;
      ~_dmsIndexExtBuilder() ;

   private:
      virtual INT32 _onInit() ;
      INT32 _build() ;

   private:
      IDmsExtDataHandler *_extHandler ;
      CHAR _collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] ;
      CHAR _idxName[ IXM_INDEX_NAME_SIZE + 1 ] ;
   } ;
   typedef _dmsIndexExtBuilder dmsIndexExtBuilder ;
}

#endif /* DMS_INDEX_BUILDER_IMPL_HPP_ */

