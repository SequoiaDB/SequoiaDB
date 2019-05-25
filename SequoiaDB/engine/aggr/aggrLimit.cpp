/******************************************************************************

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

   Source File Name = aggrLimit.cpp

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
#include "aggrLimit.hpp"
#include "qgmOptiSelect.hpp"
#include "aggrDef.hpp"

using namespace bson;

namespace engine
{

   /*
      aggrLimitParser implement
   */
   INT32 aggrLimitParser::buildNode( const BSONElement &elem,
                                     const CHAR *pCLName,
                                     qgmOptiTreeNode *&pNode,
                                     _qgmPtrTable *pTable,
                                     _qgmParamTable *pParamTable )
   {
      INT32 rc = SDB_OK ;
      qgmOptiSelect *pSelect = NULL ;

      pSelect = SDB_OSS_NEW qgmOptiSelect( pTable, pParamTable ) ;
      PD_CHECK( pSelect != NULL, SDB_OOM, error, PDERROR,
               "Malloc failed!" ) ;

      PD_CHECK( elem.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the parameter(%s),type must be number!",
                elem.toString( TRUE, TRUE ).c_str() ) ;

      try
      {
         qgmOpField selectAll ;
         selectAll.type = SQL_GRAMMAR::WILDCARD;
         pSelect->_selector.push_back( selectAll ) ;
         pSelect->_limit = elem.numberLong() ;
         pSelect->_skip = 0 ;
         pSelect->_type = QGM_OPTI_TYPE_SELECT ;
         pSelect->_hasFunc = FALSE ;

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
                   "Failed to parse the \"limit\", occur unexpection: %s",
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

