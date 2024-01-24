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

   Source File Name = qgmPlAggregation.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLAGGREGATION_HPP_
#define QGMPLAGGREGATION_HPP_

#include "qgmPlan.hpp"
#include "qgmOptiAggregation.hpp"
#include "rtnSQLFunc.hpp"
#include "qgmSelector.hpp"

namespace engine
{
   class _qgmPlAggregation : public _qgmPlan
   {
   public:
      _qgmPlAggregation( const qgmAggrSelectorVec &selector,
                         const qgmOPFieldVec &groupby,
                         const qgmField &alias,
                         _qgmPtrTable *table ) ;

      virtual ~_qgmPlAggregation() ;

   public:
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext( qgmFetchOut &next ) ;

   private:
      INT32 _push( const qgmFetchOut &next ) ;

      INT32 _result( qgmFetchOut &result ) ;

      INT32 _select( const qgmFetchOut &next,
                     const qgmOPFieldVec &fields,
                     RTN_FUNC_PARAMS &param ) ;

   private:
      rtnSQLFuncPtrVec _func ;
      _qgmSelector _groupby ;
      BSONObj _groupbyKey ;
      BSONObj _preObj ;
      BOOLEAN _eoc ;
      BOOLEAN _pushedAtThisTime ;
      BOOLEAN _pushedAtAnyTime ;
      BOOLEAN _isAggr;
      BOOLEAN _isStat ;
   } ;

   typedef class _qgmPlAggregation qgmPlAggregation ;
}

#endif

