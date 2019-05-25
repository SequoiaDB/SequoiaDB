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

   Source File Name = qgmPlSort.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMPLSORT_HPP_
#define QGMPLSORT_HPP_

#include "qgmPlan.hpp"

namespace engine
{
   class _SDB_RTNCB ;

   class _qgmPlSort : public _qgmPlan
   {
   public:
      _qgmPlSort( const qgmOPFieldVec &order ) ;
      virtual ~_qgmPlSort() ;

   public:
      virtual void close() ;
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

   private:
      SINT64 _contextID ;
      SINT64 _contextSort ;
      BSONObj _orderBy ;
      _SDB_RTNCB *_rtnCB ;
   } ;
}

#endif

