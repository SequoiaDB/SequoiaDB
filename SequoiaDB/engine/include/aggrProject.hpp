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

   Source File Name = aggrProject.hpp

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
#ifndef AGGRPROJECT_HPP__
#define AGGRPROJECT_HPP__

#include "aggrParser.hpp"

using namespace bson ;

namespace engine
{
   /*
      aggrProjectParser define
   */
   class aggrProjectParser : public aggrParser
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

      INT32 addField( const CHAR *pAlias, const CHAR *pPara,
                      const CHAR *pCLName, qgmOPFieldVec &selectorVec,
                      _qgmPtrTable *pTable ) ;

      INT32 addObj( const CHAR *pAlias, const BSONObj &Obj,
                    const CHAR *pCLName, qgmOPFieldVec &selectorVec,
                    _qgmPtrTable *pTable ) ;
   } ;

}

#endif // AGGRPROJECT_HPP__
