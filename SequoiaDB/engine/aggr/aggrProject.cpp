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
#include "qgmOptiSelect.hpp"
#include "aggrDef.hpp"
#include "aggrProject.hpp"
#include "rtnSQLFuncFactory.hpp"

using namespace bson;

namespace engine
{
   /*
      aggrProjectParser implement
   */
   INT32 aggrProjectParser::buildNode( const BSONElement &elem,
                                       const CHAR *pCLName,
                                       qgmOptiTreeNode *&pNode,
                                       _qgmPtrTable *pTable,
                                       _qgmParamTable *pParamTable )
   {
      INT32 rc = SDB_OK;
      BOOLEAN hasFunc = FALSE;
      BSONObj obj ;

      qgmOptiSelect *pSelect = SDB_OSS_NEW qgmOptiSelect( pTable,
                                                          pParamTable ) ;
      PD_CHECK( pSelect != NULL, SDB_OOM, error, PDERROR,
                "Malloc failed" ) ;

      PD_CHECK( elem.type() == Object, SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the parameter[%s], need be object",
                elem.toString( TRUE, TRUE ).c_str() ) ;

      try
      {
         obj = elem.embeddedObject() ;

         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            BSONElement beField = iter.next();
            const CHAR *pFieldName = beField.fieldName();
            PD_CHECK( pFieldName[0] != AGGR_KEYWORD_PREFIX, SDB_INVALIDARG,
                      error, PDERROR, "Failed to parse \"project\", "
                      "field name can't begin with\"$\"!" ) ;

            rc = parseSelectorField( beField, pCLName, pSelect->_selector,
                                     pTable, hasFunc ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the field[%s], rc: %d",
                         beField.toString( TRUE, TRUE ).c_str(), rc ) ;
         }

         if ( pSelect->_selector.empty() )
         {
            qgmOpField selectAll;
            selectAll.type = SQL_GRAMMAR::WILDCARD ;
            pSelect->_selector.push_back( selectAll ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse the element, occur unexpection: %s",
                   e.what() ) ;
      }

      pSelect->_limit = -1 ;
      pSelect->_skip = 0 ;
      pSelect->_type = QGM_OPTI_TYPE_SELECT ;
      pSelect->_hasFunc = hasFunc ;
      rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, pSelect->_alias ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                   AGGR_CL_DEFAULT_ALIAS, rc ) ;
      if ( pCLName != NULL )
      {
         qgmField clValAttr;
         qgmField clValRelegation;
         rc = pTable->getOwnField( pCLName, clValAttr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                      pCLName, rc ) ;

         rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS,
                                   pSelect->_collection.alias ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                      AGGR_CL_DEFAULT_ALIAS, rc ) ;
         pSelect->_collection.value = qgmDbAttr( clValRelegation, clValAttr ) ;
         pSelect->_collection.type = SQL_GRAMMAR::DBATTR ;
      }

      pNode = pSelect ;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE( pSelect ) ;
      goto done ;
   }

   INT32 aggrProjectParser::parseSelectorField( const BSONElement &beField,
                                                const CHAR *pCLName,
                                                qgmOPFieldVec &selectorVec,
                                                _qgmPtrTable *pTable,
                                                BOOLEAN &hasFunc )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( beField.isNumber() )
         {
            if ( 0 == beField.numberInt() )
            {
               goto done ;
            }
            const CHAR *pAlias = beField.fieldName();
            const CHAR *pPara = beField.fieldName();
            rc = addField( pAlias, pPara, pCLName, selectorVec, pTable ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add the field[%s], rc: %d",
                         beField.toString( TRUE, TRUE ).c_str(), rc ) ;
         }
         else if ( String == beField.type() )
         {
            const CHAR *pAlias = beField.fieldName();
            const CHAR *pPara = beField.valuestr();
            PD_CHECK( AGGR_KEYWORD_PREFIX == pPara[0], SDB_INVALIDARG,
                      error, PDERROR, "Failed to parse selector field[%s], "
                      "parameter must begin with \"$\"",
                      pPara ) ;
            rc = addField( pAlias, &(pPara[1]), pCLName, selectorVec, pTable ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add the field[%s], rc: %d",
                         beField.toString( TRUE, TRUE ).c_str(), rc ) ;
         }
         else if ( Object == beField.type() )
         {
            const CHAR *pAlias = beField.fieldName();
            BSONObj fieldObj = beField.embeddedObject();
            rc = addObj( pAlias, fieldObj, pCLName, selectorVec, pTable ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed add the object[%s], rc: %d",
                         beField.toString( TRUE, TRUE ).c_str(), rc ) ;
            hasFunc = TRUE ;
         }
         else
         {
            PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                         "Failed to parse the field[%s], invalid field type!",
                         beField.toString( TRUE, TRUE ).c_str() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse selector field, occur unexpection: %s",
                   e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 aggrProjectParser::addField( const CHAR *pAlias,
                                      const CHAR *pPara,
                                      const CHAR *pCLName,
                                      qgmOPFieldVec &selectorVec,
                                      _qgmPtrTable *pTable )
   {
      INT32 rc = SDB_OK ;
      qgmField slValAttr ;
      qgmField slValRelegation ;
      qgmOpField selector ;

      rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, slValRelegation ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the own field[%s], rc: %d",
                   AGGR_CL_DEFAULT_ALIAS, rc ) ;

      rc = pTable->getOwnField( pPara, slValAttr );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field[%s], rc: %d",
                   pPara, rc ) ;

      rc = pTable->getOwnField( pAlias, selector.alias );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the field[%s], rc: %d",
                   pAlias, rc ) ;

      {
         qgmDbAttr slVal( slValRelegation, slValAttr ) ;
         selector.value = slVal;
         selector.type = SQL_GRAMMAR::DBATTR;
         selectorVec.push_back( selector ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 aggrProjectParser::addObj( const CHAR *pAlias,
                                    const BSONObj &Obj,
                                    const CHAR *pCLName,
                                    qgmOPFieldVec &selectorVec,
                                    _qgmPtrTable *pTable )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      string strFunc ;

      try
      {
         ss << RTN_SQL_FUNC_BUILDOBJ << '(' ;
         UINT32 fieldNum = 0 ;

         BSONObjIterator iter( Obj ) ;
         while ( iter.more() )
         {
            BSONElement beField = iter.next();
            PD_CHECK( beField.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] type must be number!",
                      beField.toString( TRUE, TRUE ).c_str() ) ;

            if ( 0 == beField.numberInt() )
            {
               continue ;
            }

            if ( fieldNum > 0 )
            {
               ss << ',' ;
            }
            ss << AGGR_CL_DEFAULT_ALIAS"."
               << beField.fieldName() ;
            ++fieldNum ;
         }

         PD_CHECK( fieldNum > 0, SDB_INVALIDARG, error, PDERROR,
                   "Param[%s] can't be empty",
                   Obj.toString( TRUE, TRUE ).c_str() ) ;

         ss << ')' ;
         strFunc = ss.str() ;

         qgmField slValAttr;
         qgmField slValRelegation;
         rc = pTable->getOwnField( strFunc.c_str(), slValAttr );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the own field[%s], rc: %d",
                      strFunc.c_str() ) ;

         qgmDbAttr slVal( slValRelegation, slValAttr ) ;
         qgmField slAlias ;
         rc = pTable->getOwnField( pAlias, slAlias );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the own field[%s], rc: %d",
                      pAlias, rc ) ;

         qgmOpField selector ;
         selector.alias = slAlias ;
         selector.value = slVal ;
         selector.type = SQL_GRAMMAR::FUNC ;
         selectorVec.push_back( selector ) ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to add function-field, occur unexpection: %s",
                   e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

