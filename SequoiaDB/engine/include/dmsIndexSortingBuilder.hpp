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

   Source File Name = dmsIndexSortingBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_SORTING_BUILDER_HPP_
#define DMS_INDEX_SORTING_BUILDER_HPP_

#include "dmsIndexBuilder.hpp"
#include "dmsExtDataHandler.hpp"
#include "utilString.hpp"
#include "../bson/ordering.h"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{

   class _dmsIxmKeySorter ;

   class _dmsIndexSortingBuilder: public _dmsIndexBuilder
   {
   public:
      _dmsIndexSortingBuilder( _dmsStorageUnit* su,
                               _dmsMBContext* mbContext,
                               _pmdEDUCB* eduCB,
                               dmsExtentID indexExtentID,
                               dmsExtentID indexLogicID,
                               INT32 sortBufferSize,
                               dmsIndexBuildGuardPtr &guardPtr,
                               dmsDupKeyProcessor *dkProcessor,
                               dmsIdxTaskStatus* pIdxStatus = NULL ) ;
      ~_dmsIndexSortingBuilder() ;

   private:
      INT32 _init() ;
      INT32 _fillSorter( rtnTBScanner &scanner,
                         dmsRecordID &minRID,
                         dmsRecordID &maxRID,
                         dmsRecordID &moveRID,
                         ossScopedRWLock &lock ) ;
      INT32 _insertKeys( const dmsRecordID &minRID,
                         const dmsRecordID &maxRID,
                         const Ordering& ordering,
                         ossScopedRWLock &lock ) ;
      INT32 _build() ;

   private:
      BOOLEAN           _needBuildByRange ;
      _dmsIxmKeySorter* _sorter ;
      INT64             _bufSize ;
      INT64             _bufExtSize ;
      BOOLEAN           _eoc ;
   } ;
   typedef class _dmsIndexSortingBuilder dmsIndexSortingBuilder ;

}

#endif /* DMS_INDEX_SORTING_BUILDER_HPP_ */
