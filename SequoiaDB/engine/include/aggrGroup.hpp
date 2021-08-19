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

   Source File Name = aggrGroup.hpp

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
#ifndef AGGRGROUP_HPP__
#define AGGRGROUP_HPP__

#include "aggrParser.hpp"
#include "qgmPtrTable.hpp"
#include "qgmOptiTree.hpp"
#include "qgmParamTable.hpp"
#include "qgmOptiAggregation.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   /*
      aggrGroupParser define
   */
   class aggrGroupParser : public aggrParser
   {
   private:
      INT32 buildNode( const BSONElement &elem,
                       const CHAR *pCLName,
                       BSONObj &hint,
                       qgmOptiTreeNode *&pNode,
                       _qgmPtrTable *pTable,
                       _qgmParamTable *pParamTable ) ;

      INT32 parseSelectorField( const BSONElement &beField,
                                const CHAR *pCLName,
                                qgmOPFieldVec &selectorVec,
                                _qgmPtrTable *pTable,
                                BOOLEAN &hasFunc ) ;

      INT32 addGroupByField( const CHAR *pFieldName,
                             qgmOPFieldVec &groupby,
                             _qgmPtrTable *pTable,
                             const CHAR *pCLName ) ;

      INT32 parseGroupbyField( const BSONElement &beId,
                               qgmOPFieldVec &groupby,
                               _qgmPtrTable *pTable,
                               const CHAR *pCLName ) ;

      INT32 parseInputFunc( const BSONObj &funcObj,
                            const CHAR *pCLName,
                            qgmField &funcField,
                            _qgmPtrTable *pTable ) ;
   } ;

}

#endif // AGGRGROUP_HPP__
