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

   Source File Name = dmsIxmKeySorter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_IXM_KEY_SORTER_HPP_
#define DMS_IXM_KEY_SORTER_HPP_

#include "ixmKey.hpp"
#include "dms.hpp"
#include "../bson/ordering.h"

namespace engine
{
   class _dmsIxmKeyComparer
   {
   public:
      _dmsIxmKeyComparer( bson::Ordering& order )
      : _order( order )
      {
      }

      bool operator()( const ixmKey& key1, const ixmKey& key2 )
      {
         return ( key1.woCompare( key2, _order ) < 0 ) ;
      }

   private:
      bson::Ordering _order ;
   } ;
   typedef class _dmsIxmKeyComparer dmsIxmKeyComparer ;
   
   class _dmsIxmKeySorter: public SDBObject
   {
   private:
      // disallow copy and assign
      _dmsIxmKeySorter( const _dmsIxmKeySorter& ) ;
      void operator=( const _dmsIxmKeySorter& ) ;

   protected:
      _dmsIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer )
      : _bufSize( bufSize ), _comparer( comparer ) {}

   public:
      virtual ~_dmsIxmKeySorter() {}
      virtual INT32 push( const ixmKey& key, const dmsRecordID& recordID ) = 0 ;
      virtual INT32 sort() = 0 ;
      virtual INT32 fetch( ixmKey& key, dmsRecordID& recordID ) = 0 ;
      virtual INT32 reset() = 0 ;
      virtual INT64 bufferSize() const { return _bufSize ; }
      virtual INT64 usedBufferSize() const = 0 ;

   protected:
      INT64                _bufSize ;
      _dmsIxmKeyComparer   _comparer ;
   } ;
   typedef class _dmsIxmKeySorter dmsIxmKeySorter ;

   class _dmsIxmKeySorterCreator: public SDBObject
   {
   private:
      // disallow copy and assign
      _dmsIxmKeySorterCreator( const _dmsIxmKeySorterCreator& ) ;
      void operator=( const _dmsIxmKeySorterCreator& ) ;

   protected:
      _dmsIxmKeySorterCreator() {}

   public:
      virtual ~_dmsIxmKeySorterCreator() {}
      virtual INT32 createSorter( INT64 bufSize,
                                  const _dmsIxmKeyComparer& comparer,
                                  _dmsIxmKeySorter** ppSorter ) = 0 ;
      virtual void releaseSorter( _dmsIxmKeySorter* pSorter ) = 0 ;
   } ;
   typedef class _dmsIxmKeySorterCreator dmsIxmKeySorterCreator ;
}

#endif /* DMS_IXM_KEY_SORTER_HPP_ */

