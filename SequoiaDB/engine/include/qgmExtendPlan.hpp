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

   Source File Name = qgmExtendPlan.hpp

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

#ifndef QGMEXTENDPLAN_HPP_
#define QGMEXTENDPLAN_HPP_

#include "qgmOptiTree.hpp"
#include <queue>
#include "utilMap.hpp"

namespace engine
{

   typedef INT32 QGM_EXTEND_ID ;
   typedef _utilMap<QGM_EXTEND_ID, qgmOptiTreeNode*, 8 > QGM_EXTEND_TABLE ;

   class _qgmExtendPlan : public SDBObject
   {
   public:
      _qgmExtendPlan() ;
      virtual ~_qgmExtendPlan() ;

   public:
      INT32 extend( qgmOptiTreeNode *&extended ) ;

      INT32 insertPlan( UINT32 id, qgmOptiTreeNode *ex = NULL ) ;

      OSS_INLINE qgmOptiTreeNode *getNode( UINT32 id )
      {
         QGM_EXTEND_TABLE::iterator it = _table.find( id ) ;
         return ( it == _table.end() ) ? NULL : it->second ;
      }

      OSS_INLINE void pushAlias( const qgmField &alias )
      {
         _aliases.push( alias ) ;
      }

   private:
      virtual INT32 _extend( UINT32 id,
                             qgmOptiTreeNode *&extended ) = 0 ;

   protected:
      QGM_EXTEND_TABLE _table ;
      qgmOptiTreeNode  *_local ;
      UINT32 _localID ;
      std::queue<qgmField> _aliases ;

   } ;

   typedef class _qgmExtendPlan qgmExtendPlan ;
}

#endif

