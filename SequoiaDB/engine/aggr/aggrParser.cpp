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

   Source File Name = aggrParser.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

******************************************************************************/
#include "aggrParser.hpp"
#include "qgmDef.hpp"
#include "qgmOptiSelect.hpp"
#include "aggrDef.hpp"
#include "msgDef.h"
#include "qgmOptiTree.hpp"

using namespace bson;

namespace engine
{
   /*
      aggrParser implement
   */
   INT32 aggrParser::parse( const BSONElement &elem,
                            _qgmOptiTreeNode *&root,
                            _qgmPtrTable * pPtrTable,
                            _qgmParamTable *pParamTable,
                            const CHAR *pCollectionName )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( pPtrTable!=NULL, "pPtrTable can't be NULL!" );
      SDB_ASSERT( pParamTable!=NULL, "pParamTable can't be NULL!" );
      SDB_ASSERT( pCollectionName != NULL || root != NULL,
                  "collectionname can't be NULL in leaf-node" );
      _qgmOptiTreeNode *pNode = NULL;

      rc = buildNode( elem, pCollectionName, pNode,
                      pPtrTable, pParamTable ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build the node, rc: %d", rc ) ;

      if ( root != NULL )
      {
         rc = pNode->addChild( root ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add the child, rc: %d", rc ) ;
         root->_father = pNode ;
      }

      root = pNode ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNode ) ;
      goto done ;
   }

}

