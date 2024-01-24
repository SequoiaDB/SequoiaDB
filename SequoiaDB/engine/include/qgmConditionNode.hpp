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

   Source File Name = qgmConditionNode.hpp

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

#ifndef QGMCONDITIONNODE_HPP_
#define QGMCONDITIONNODE_HPP_

#include "qgmOptiTree.hpp"
#include "pd.hpp"

namespace engine
{
   struct _qgmConditionNode : public _utilPooledObject
   {
      qgmDbAttr value ;

      /// when type > SQL_GRAMMAR::SQLMAX, var is effective.
      const BSONElement *var ;
      INT32 type ;
      _qgmConditionNode *left ;
      _qgmConditionNode *right ;

      _qgmConditionNode( INT32 t )
      :var(NULL),
       type( t ),
       left( NULL ),
       right( NULL )
      {

      }

      _qgmConditionNode( const _qgmConditionNode *node )
      {
         SDB_ASSERT( NULL != node, "impossible" ) ;
         type = node->type ;
         value = node->value ;
         var = node->var ;
         left = NULL ;
         right = NULL ;
         if ( NULL != node->left )
         {
            left = SDB_OSS_NEW _qgmConditionNode(node->left) ;
            if ( NULL == left )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
            }
         }

         if ( NULL != node->right )
         {
            right = SDB_OSS_NEW _qgmConditionNode(node->right) ;
            if ( NULL == right )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
            }
         }
      }

      virtual ~_qgmConditionNode()
      {
         SAFE_OSS_DELETE( left ) ;
         SAFE_OSS_DELETE( right ) ;
      }

      void  dettach ()
      {
         left = NULL ;
         right = NULL ;
      }
   } ;
   typedef struct _qgmConditionNode             qgmConditionNode ;
   typedef ossPoolVector< qgmConditionNode* >   qgmConditionNodePtrVec ;
   typedef ossPoolVector< qgmConditionNode >    qgmConditionNodeVec ;
}

#endif

