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
      _dmsIxmKeySorter* createSorter(  INT64 bufSize, const _dmsIxmKeyComparer& comparer  ) ;
      void releaseSorter( _dmsIxmKeySorter* sorter ) ;

   private:
      _rtnIxmKeySorterCreator( const _rtnIxmKeySorterCreator& ) ;
      void operator=( const _rtnIxmKeySorterCreator& ) ;
   } ;
   typedef class _rtnIxmKeySorterCreator rtnIxmKeySorterCreator ;
}

#endif /* RTN_IXM_KEY_SORTER_HPP_ */

