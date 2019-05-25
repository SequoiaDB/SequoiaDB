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

   Source File Name = optQgmCommStrategy.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPT_QGM_COMM_STRATEGY_HPP_
#define OPT_QGM_COMM_STRATEGY_HPP_

#include "optQgmStrategy.hpp"

namespace engine
{

   class _optQgmAcceptSty : public optQgmStrategyBase
   {
      public:
         _optQgmAcceptSty() {}
         virtual ~_optQgmAcceptSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result )
         {
            result = OPT_SS_ACCEPT ;
            return SDB_OK ;
         }

         virtual const CHAR* strategyName() const
         {
            return "ACEETP-Stragegy" ;
         }
   };
   typedef _optQgmAcceptSty optQgmAcceptSty ;

   class _optQgmRefuseSty : public optQgmStrategyBase
   {
      public:
         _optQgmRefuseSty() {}
         virtual ~_optQgmRefuseSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result )
         {
            result = OPT_SS_REFUSE ;
            return SDB_OK ;
         }

         virtual const CHAR* strategyName() const
         {
            return "REFUSE-Strategy" ;
         }
   };
   typedef _optQgmRefuseSty optQgmRefuseSty ;

   class _optQgmTakeOverSty : public optQgmStrategyBase
   {
      public:
         _optQgmTakeOverSty() {}
         virtual ~_optQgmTakeOverSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result )
         {
            result = OPT_SS_TAKEOVER ;
            return SDB_OK ;
         }

         virtual const CHAR* strategyName() const
         {
            return "TAKEOVER-Strategy" ;
         }
   };
   typedef _optQgmTakeOverSty optQgmTakeOverSty ;

}

#endif //OPT_QGM_COMM_STRATEGY_HPP_

