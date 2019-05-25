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

   Source File Name = aggrSort.hpp

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
#ifndef AGGRSORT_HPP__
#define AGGRSORT_HPP__

#include "aggrParser.hpp"

using namespace bson ;

namespace engine
{
   /*
      aggrSortParser define
   */
   class aggrSortParser : public aggrParser
   {
   private:
      INT32 buildNode( const BSONElement &elem,
                       const CHAR *pCLName,
                       qgmOptiTreeNode *&pNode,
                       _qgmPtrTable *pTable,
                       _qgmParamTable *pParamTable ) ;

      INT32 buildOrderBy( const BSONElement &elem,
                          qgmOPFieldVec &orderBy,
                          _qgmPtrTable *pTable,
                          const CHAR *pCLName ) ;
   } ;

}

#endif // AGGRSORT_HPP__

