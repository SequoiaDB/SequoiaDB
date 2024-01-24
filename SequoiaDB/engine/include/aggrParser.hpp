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

   Source File Name = aggrParser.hpp

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
#ifndef AGGRPARSER_HPP__
#define AGGRPARSER_HPP__

#include "qgmPtrTable.hpp"
#include "qgmOptiTree.hpp"
#include "qgmParamTable.hpp"
#include "qgmOptiAggregation.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _qgmOptiTreeNode;
   class _qgmPtrTable;
   class _qgmParamTable;

   void aggrEmptyBSONObj( BSONObj &obj ) ;

   /*
      aggrParser define
   */
   class aggrParser : public SDBObject
   {
   public:
      virtual ~aggrParser(){}
      // change to plan to parent node
      virtual INT32 parse( const BSONElement &elem,
                           _qgmOptiTreeNode *&root,
                           _qgmPtrTable * pPtrTable,
                           _qgmParamTable *pParamTable,
                           const CHAR *pCollectionName,
                           BSONObj &hint ) ;

   private:
      virtual INT32 buildNode( const BSONElement &elem,
                               const CHAR *pCLName,
                               BSONObj &hint,
                               qgmOptiTreeNode *&pNode,
                               _qgmPtrTable *pTable,
                               _qgmParamTable *pParamTable ) = 0 ;

   } ;
}

#endif // AGGRPARSER_HPP__
