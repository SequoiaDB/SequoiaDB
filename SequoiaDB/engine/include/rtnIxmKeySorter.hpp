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

   Source File Name = rtnIxmKeySorter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_IXM_KEY_SORTER_HPP_
#define RTN_IXM_KEY_SORTER_HPP_

#include "dmsIxmKeySorter.hpp"

namespace engine
{
   class _rtnIxmKeySorter: public dmsIxmKeySorter
   {
   private:
      // disallow copy and assign
      _rtnIxmKeySorter( const _rtnIxmKeySorter& ) ;
      void operator=( const _rtnIxmKeySorter& ) ;

   public:
      _rtnIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer ) ;
      ~_rtnIxmKeySorter() ;
      INT32 init() ;
      INT32 push( const ixmKey& key, const dmsRecordID& recordID ) ;
      INT32 sort() ;
      INT32 fetch( ixmKey& key, dmsRecordID& recordID ) ;
      INT32 reset() ;
      INT64 usedBufferSize() const ;

   private:
      CHAR*                _buf ;
      INT64                _headOffset ;
      INT64                _tailOffset ;
      INT64                _keyNum ;
      INT64                _fetchedNum ;
      BOOLEAN              _sorted ;
      BOOLEAN              _inited ;
   } ;
   typedef class _rtnIxmKeySorter rtnIxmKeySorter ;

   class _rtnIxmKeySorterCreator: public dmsIxmKeySorterCreator
   {
   public:
      _rtnIxmKeySorterCreator() {}
      ~_rtnIxmKeySorterCreator() {}
      INT32 createSorter( INT64 bufSize,
                          const _dmsIxmKeyComparer& comparer,
                          _dmsIxmKeySorter** ppSorter ) ;
      void releaseSorter( _dmsIxmKeySorter* pSorter ) ;

   private:
      // disallow copy and assign
      _rtnIxmKeySorterCreator( const _rtnIxmKeySorterCreator& ) ;
      void operator=( const _rtnIxmKeySorterCreator& ) ;
   } ;
   typedef class _rtnIxmKeySorterCreator rtnIxmKeySorterCreator ;
}

#endif /* RTN_IXM_KEY_SORTER_HPP_ */

