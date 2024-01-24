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

   Source File Name = aggrSkip.cpp

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
#include "aggrSkip.hpp"
#include "qgmOptiSelect.hpp"
#include "aggrDef.hpp"

using namespace bson;

namespace engine
{
   /*
      aggrSkipParser implement
   */
   INT32 aggrSkipParser::buildNode( const BSONElement &elem,
                                    const CHAR *pCLName,
                                    BSONObj &hint,
                                    qgmOptiTreeNode *&pNode,
                                    _qgmPtrTable *pTable,
                                    _qgmParamTable *pParamTable )
   {
      INT32 rc = SDB_OK ;
      qgmOptiSelect *pSelect = NULL ;

      pSelect = SDB_OSS_NEW qgmOptiSelect( pTable, pParamTable );
      PD_CHECK( pSelect != NULL, SDB_OOM, error, PDERROR,
               "Malloc failed!" ) ;

      PD_CHECK( elem.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the parameter[%s],type must be number!",
                elem.toString( TRUE, TRUE ).c_str() ) ;

      try
      {
         qgmOpField selectAll ;
         selectAll.type = SQL_GRAMMAR::WILDCARD ;
         pSelect->_selector.push_back( selectAll ) ;
         pSelect->_limit = -1 ;
         pSelect->_skip = elem.numberLong() ;
         pSelect->_type = QGM_OPTI_TYPE_SELECT ;
         pSelect->_hasFunc = FALSE ;
         pSelect->_objHint = hint ;
         aggrEmptyBSONObj( hint ) ;

         rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, pSelect->_alias ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the field[%s], rc: %d",
                      AGGR_CL_DEFAULT_ALIAS, rc ) ;
         if ( pCLName != NULL )
         {
            qgmField clValAttr ;
            qgmField clValRelegation ;
            rc = pTable->getOwnField( pCLName, clValAttr ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the field[%s], rc: %d",
                         pCLName, rc ) ;

            rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS,
                                      pSelect->_collection.alias ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the field[%s], rc: %d",
                         AGGR_CL_DEFAULT_ALIAS, rc ) ;

            pSelect->_collection.value = qgmDbAttr( clValRelegation,
                                                    clValAttr ) ;
            pSelect->_collection.type = SQL_GRAMMAR::DBATTR ;
         }
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse the \"skip\", occur unexpection: %s",
                   e.what() ) ;
      }

      pNode = pSelect ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pSelect ) ;
      goto done ;
   }

}

