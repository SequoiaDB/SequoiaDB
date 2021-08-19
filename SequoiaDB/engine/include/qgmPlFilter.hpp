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

   Source File Name = qgmPlFilter.hpp

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

#ifndef QGMPLFILTER_HPP_
#define QGMPLFILTER_HPP_

#include "qgmPlan.hpp"
#include "qgmMatcher.hpp"
#include "qgmSelector.hpp"

namespace engine
{
   struct _qgmConditionNode ;

   class _qgmPlFilter : public _qgmPlan
   {
   public:
      /// not the same to qgmPlScan, selector and condition
      /// should include alias.
      /// eg: element in selector is T.a:alias
      _qgmPlFilter( const qgmOPFieldVec &selector,
                    _qgmConditionNode *condition,
                    INT64 numSkip,
                    INT64 numReturn,
                    const qgmField &alias ) ;

      virtual ~_qgmPlFilter() ;

   public:
      virtual string toString() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

   protected:
      qgmSelector _selector ;
      INT64 _return ;
      INT64 _skip ;
      INT64 _currentSkip ;
      INT64 _currentReturn ;

   private:
      qgmMatcher _matcher ;
      _qgmConditionNode *_condition ;
      BOOLEAN _hasSelector ;
   } ;

   typedef class _qgmPlFilter qgmPlFilter ;
}

#endif

