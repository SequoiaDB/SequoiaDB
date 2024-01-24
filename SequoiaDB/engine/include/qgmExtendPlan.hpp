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
#include "ossMemPool.hpp"
#include <queue>

namespace engine
{

   typedef INT32 QGM_EXTEND_ID ;
   typedef ossPoolMap<QGM_EXTEND_ID, qgmOptiTreeNode*> QGM_EXTEND_TABLE ;

   class _qgmExtendPlan : public _utilPooledObject
   {
   public:
      _qgmExtendPlan() ;
      virtual ~_qgmExtendPlan() ;

   public:
      INT32 extend( qgmOptiTreeNode *&extended ) ;

      /// when local is not NULL, it will be defined as local.
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

