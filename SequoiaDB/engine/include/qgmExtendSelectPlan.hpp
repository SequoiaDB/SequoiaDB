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

   Source File Name = qgmExtendSelectPlan.hpp

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

#ifndef QGMEXTENDSELECTPLAN_HPP_
#define QGMEXTENDSELECTPLAN_HPP_

#include "qgmExtendPlan.hpp"
#include "qgmOptiAggregation.hpp"

namespace engine
{
   const UINT32 QGM_EXTEND_ORDERFILTER = 0 ;
   const UINT32 QGM_EXTEND_ORDERBY = 1 ;
   const UINT32 QGM_EXTEND_SPLITBY = 2 ;
   const UINT32 QGM_EXTEND_AGGR = 3 ;
   const UINT32 QGM_EXTEND_GROUPBY = 4 ;
   const UINT32 QGM_EXTEND_LOCAL = 5 ;

   class _qgmExtendSelectPlan : public _qgmExtendPlan
   {
   public:
      _qgmExtendSelectPlan() ;
      virtual ~_qgmExtendSelectPlan() ;

      void    clearConstraint() { _limit = -1 ; _skip = 0 ; }

   private:
      INT32 _extend( UINT32 id,
                     qgmOptiTreeNode *&extended ) ;

   protected:

   public:
      qgmAggrSelectorVec _funcSelector ;
      qgmOPFieldVec _groupby ;
      qgmOPFieldVec _orderby ;
      qgmOPFieldVec _original ;
      qgmDbAttr _splitby ;
      INT64         _limit ;
      INT64         _skip ;

   } ;

   typedef class _qgmExtendSelectPlan qgmExtendSelectPlan ;
}

#endif

