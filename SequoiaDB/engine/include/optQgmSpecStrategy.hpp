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

   Source File Name = optQgmSpecStrategy.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPT_QGM_SPEC_STRATEGY_HPP_
#define OPT_QGM_SPEC_STRATEGY_HPP_

#include "optQgmStrategy.hpp"
#include "qgmConditionNode.hpp"

namespace engine
{

   class _optQgmSortAggrSty : public optQgmStrategyBase
   {
      public:
         _optQgmSortAggrSty() {}
         virtual ~_optQgmSortAggrSty () {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmSortAggrSty optQgmSortAggrSty ;

   class _optQgmSortFilterSty : public optQgmStrategyBase
   {
      public:
         _optQgmSortFilterSty() {}
         virtual ~_optQgmSortFilterSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   } ;
   typedef _optQgmSortFilterSty optQgmSortFilterSty ;

   class _optQgmSortJoinSty : public optQgmStrategyBase
   {
      public:
         _optQgmSortJoinSty() {}
         virtual ~_optQgmSortJoinSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmSortJoinSty optQgmSortJoinSty ;

   class _optQgmFilterSortSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterSortSty() {}
         virtual ~_optQgmFilterSortSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterSortSty optQgmFilterSortSty ;

   class _optQgmFilterFilterSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterFilterSty() {}
         virtual ~_optQgmFilterFilterSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterFilterSty optQgmFilterFilterSty ;

   class _optQgmFilterAggrSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterAggrSty() {}
         virtual ~_optQgmFilterAggrSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterAggrSty optQgmFilterAggrSty ;

   class _optQgmFilterJoinSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterJoinSty() {}
         virtual ~_optQgmFilterJoinSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterJoinSty optQgmFilterJoinSty ;

   class _optQgmAggrFilterSty : public optQgmStrategyBase
   {
      public:
         _optQgmAggrFilterSty() {}
         virtual ~_optQgmAggrFilterSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmAggrFilterSty optQgmAggrFilterSty ;

   class _optQgmFilterScanSty : public optQgmStrategyBase
   {
   public:
      _optQgmFilterScanSty() {}
      virtual ~_optQgmFilterScanSty() {}

   public:
      virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

      virtual const CHAR* strategyName() const ;
   } ;
   typedef class _optQgmFilterScanSty optQgmFilterScanSty ;

   /////////////////////////////////////////////////////////////////////////////
   // tool functions
   /////////////////////////////////////////////////////////////////////////////

   BOOLEAN isCondSameRele( qgmConditionNode *condNode, BOOLEAN allowEmpty = TRUE ) ;

   BOOLEAN isCondIncludedNull( qgmConditionNode *condNode ) ;

}

#endif //OPT_QGM_SPEC_STRATEGY_HPP_

