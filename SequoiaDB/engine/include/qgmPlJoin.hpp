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

   Source File Name = qgmOptiJoin.hpp

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

#ifndef QGMPLJOIN_HPP_
#define QGMPLJOIN_HPP_

#include "qgmPlan.hpp"

namespace engine
{
   class _qgmPlJoin : public _qgmPlan
   {
   public:
      _qgmPlJoin( INT32 joinType )
      :_qgmPlan( QGM_PLAN_TYPE_NLJOIN, _qgmField() ),
       _joinType( joinType ),
       _outerAlias( NULL ),
       _innerAlias( NULL ),
       _outer( NULL ),
       _inner( NULL )
      {

      }

      virtual ~_qgmPlJoin()
      {
         _joinType = SQL_GRAMMAR::SQLMAX ;
         _outerAlias = NULL ;
         _innerAlias = NULL ;
         _outer = NULL ;
         _inner = NULL ;
      }

   public:
      virtual string toString() const
      {
         return SQL_GRAMMAR::INNERJOIN == _joinType ?
                "Type:INNERJOIN\n" : "Type: LEFT OUTER JOIN\n" ;
      }

   protected:
      INT32 _joinType ;
      const qgmField *_outerAlias ;
      const qgmField *_innerAlias ;
      _qgmPlan *_outer ;
      _qgmPlan *_inner ;
   } ;
   typedef class _qgmPlJoin qgmPlJoin ;
}

#endif

