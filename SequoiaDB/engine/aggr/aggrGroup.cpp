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

   Source File Name = aggrGroup.cpp

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
#include "aggrGroup.hpp"
#include "qgmDef.hpp"
#include "qgmOptiSelect.hpp"
#include "aggrDef.hpp"
#include "msgDef.h"
#include "qgmOptiTree.hpp"
#include "ossUtil.h"

using namespace bson;

namespace engine
{

   /*
      aggrGroupParser implement
   */
   INT32 aggrGroupParser::buildNode( const BSONElement &elem,
                                     const CHAR *pCLName,
                                     BSONObj &hint,
                                     qgmOptiTreeNode *&pNode,
                                     _qgmPtrTable *pTable,
                                     _qgmParamTable *pParamTable )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasFunc = FALSE ;
      BSONObj obj ;

      qgmOptiSelect *pSelect = SDB_OSS_NEW qgmOptiSelect( pTable,
                                                          pParamTable ) ;
      PD_CHECK( pSelect!=NULL, SDB_OOM, error, PDERROR,
                "Malloc failed" ) ;

      PD_CHECK( elem.type() == Object, SDB_INVALIDARG, error, PDERROR,
                "Failed to parse the element[%s], type shoud be Object",
                elem.toString( TRUE, TRUE ).c_str() ) ;

      // 1.parse the fields
      try
      {
         obj = elem.embeddedObject() ;

         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            BSONElement beField = iter.next() ;
            const CHAR *pFieldName = beField.fieldName() ;
            if ( 0 == ossStrcmp( pFieldName, FIELD_NAME_GROUPBY_ID ))
            {
               // process groupby field(_id)
               rc = parseGroupbyField( beField, pSelect->_groupby,
                                       pTable, pCLName ) ;
            }
            else
            {
               // process normal field(selector)
               rc = parseSelectorField( beField, pCLName, pSelect->_selector,
                                        pTable, hasFunc ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the element[%s], rc: %d",
                         elem.toString( TRUE, TRUE ).c_str(), rc ) ;
         }

         /// when selector is empty, need push (*) selector
         if ( pSelect->_selector.empty() )
         {
            qgmOpField selectAll ;
            selectAll.type = SQL_GRAMMAR::WILDCARD ;
            pSelect->_selector.push_back( selectAll ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse the element, occur unexption: %s",
                   e.what() ) ;
      }

      // 2.build the node
      pSelect->_limit = -1 ;
      pSelect->_skip = 0 ;
      pSelect->_type = QGM_OPTI_TYPE_SELECT ;
      pSelect->_hasFunc = hasFunc ;
      pSelect->_objHint = hint ;
      aggrEmptyBSONObj( hint ) ;

      rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, pSelect->_alias ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the own field, rc: %d", rc ) ;

      if ( pCLName )
      {
         qgmField clValAttr ;
         qgmField clValRelegation ;
         rc = pTable->getOwnField( pCLName, clValAttr );
         PD_RC_CHECK( rc, PDERROR, "Failed to get own field, rc: %d", rc ) ;

         rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS,
                                   pSelect->_collection.alias ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field, rc: %d", rc ) ;

         pSelect->_collection.value = qgmDbAttr( clValRelegation, clValAttr ) ;
         pSelect->_collection.type = SQL_GRAMMAR::DBATTR ;
      }

      pNode = pSelect ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pSelect ) ;
      goto done ;
   }

   INT32 aggrGroupParser::parseGroupbyField( const bson::BSONElement &beId,
                                             qgmOPFieldVec &groupby,
                                             _qgmPtrTable *pTable,
                                             const CHAR *pCLName )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( pTable != NULL , "_qgmPtrTable can't be NULL!" ) ;

      if ( beId.isNull() )
      {
         goto done ;
      }

      try
      {
         switch ( beId.type() )
         {
            case String:
               {
                  const CHAR *pFieldName = beId.valuestr() ;
                  rc = addGroupByField( pFieldName, groupby, pTable, pCLName ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to parse _id, rc: %d", rc ) ;
                  break ;
               }
            case Object:
               {
                  BSONObj idObj = beId.embeddedObject() ;
                  BSONObjIterator iter( idObj ) ;
                  while( iter.more() )
                  {
                     BSONElement beField = iter.next() ;
                     PD_CHECK( beField.type()==String, SDB_INVALIDARG,
                               error, PDERROR,
                               "Failed to parse _id, sub-field type must "
                               "be string!" ) ;
                     const CHAR *pFieldName = beField.valuestr();
                     rc = addGroupByField( pFieldName, groupby,
                                           pTable, pCLName ) ;
                     PD_RC_CHECK( rc, PDERROR,
                                  "Failed to parse _id, rc: %d", rc ) ;
                  }
                  break;
               }
            default:
               {
                  PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                               "The type of _id[%s] is invalid",
                               beId.toString( TRUE, TRUE ).c_str() ) ;
                  break ;
               }
         }
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse _id, occur unexpection: %s",
                   e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 aggrGroupParser::addGroupByField( const CHAR *pFieldName,
                                           qgmOPFieldVec &groupby,
                                           _qgmPtrTable *pTable,
                                           const CHAR *pCLName )
   {
      INT32 rc = SDB_OK;
      qgmField gbValAttr ;
      qgmField gbValRelegation ;

      rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS,
                                gbValRelegation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                   AGGR_CL_DEFAULT_ALIAS, rc ) ;

      PD_CHECK( AGGR_KEYWORD_PREFIX == pFieldName[0], SDB_INVALIDARG,
                error, PDERROR, "fieldname must begin with \"$\"!" ) ;

      rc = pTable->getOwnField( &(pFieldName[1]), gbValAttr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get the own field[%s], rc: %d",
                   &(pFieldName[1]), rc ) ;

      {
         qgmDbAttr gbVal( gbValRelegation, gbValAttr ) ;
         qgmOpField gbOpField ;
         gbOpField.value = gbVal ;
         gbOpField.type = SQL_GRAMMAR::ASC ;
         groupby.push_back( gbOpField ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 aggrGroupParser::parseSelectorField( const BSONElement &beField,
                                              const CHAR *pCLName,
                                              qgmOPFieldVec &selectorVec,
                                              _qgmPtrTable *pTable,
                                              BOOLEAN &hasFunc )
   {
      INT32 rc = SDB_OK ;

      try
      {
         // format ex: {a:{$first:"$a"}} or {a:"$a"}
         qgmOpField selector;
         // parse field
         const CHAR *pAlias = beField.fieldName() ;
         qgmField slAlias;
         rc = pTable->getOwnField( pAlias, slAlias ) ;
         PD_RC_CHECK( rc, PDERROR, "Get owned filed[%s] failed, rc: %d",
                      pAlias, rc ) ;

         if ( Object == beField.type() )
         {
            BSONObj funcObj ;
            funcObj = beField.embeddedObject() ;
            const CHAR *pFuncName = funcObj.firstElementFieldName() ;
            PD_CHECK( AGGR_KEYWORD_PREFIX == pFuncName[0], SDB_INVALIDARG,
                      error, PDERROR, "Failed to parse selector field, "
                      "function name must begin with \"$\"" ) ;

            hasFunc = TRUE;

            // build selector
            qgmField slValAttr;
            qgmField slValRelegation;

            rc = parseInputFunc( funcObj, pCLName, slValAttr, pTable ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse selector field, rc: %d",
                         rc ) ;

            qgmDbAttr slVal( slValRelegation, slValAttr );
            selector.alias = slAlias;
            selector.value = slVal;
            selector.type = SQL_GRAMMAR::FUNC ;
            selectorVec.push_back( selector ) ;
         }
         else if ( String == beField.type() &&
                   AGGR_KEYWORD_PREFIX == beField.valuestr()[0] )
         {
            // build selector
            qgmField slValAttr;
            qgmField slValRelegation ;

            rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, slValRelegation ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the field[%s], rc: %d",
                         AGGR_CL_DEFAULT_ALIAS, rc ) ;

            selector.alias = slAlias;
            rc = pTable->getOwnField( &beField.valuestr()[1], slValAttr ) ;
            PD_RC_CHECK( rc, PDERROR,"Failed to get the field[%s], rc: %d",
                         beField.valuestr(), rc ) ;

            qgmDbAttr slVal( slValRelegation, slValAttr ) ;
            selector.value = slVal ;
            selector.type = SQL_GRAMMAR::DBATTR ;
            selectorVec.push_back( selector ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to parse selector field, field type "
                    "must be object or field name with '$': %s",
                    beField.toString( true, true ).c_str() ) ;
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

   INT32 aggrGroupParser::parseInputFunc( const BSONObj &funcObj,
                                          const CHAR *pCLName,
                                          qgmField &funcField,
                                          _qgmPtrTable *pTable )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pFunc = NULL ;
      stringstream ss ;
      string strFunc ;

      try
      {
         BSONElement beField = funcObj.firstElement() ;

         pFunc = beField.fieldName();
         PD_CHECK( AGGR_KEYWORD_PREFIX == pFunc[0], SDB_INVALIDARG,
                   error, PDERROR, "Failed to parse function, "
                   "function name must begin with \"$\"" ) ;

         ss << &pFunc[1] ;
         ss << '(' ;

         /// sum or count allow number
         if ( beField.isNumber() &&
              ( ( pFunc[1] == 's' && pFunc[2] == 'u' &&
                  pFunc[3] == 'm' && pFunc[4] == '\0' ) ||
                ( pFunc[1] == 'c' && pFunc[2] == 'o' &&
                  pFunc[3] == 'u' && pFunc[4] == 'n' &&
                  pFunc[3] == 't' && pFunc[4] == '\0' )
               ) )
         {
            ss << AGGR_CL_DEFAULT_ALIAS"." ;

            if ( beField.type() == NumberLong )
            {
               ss << beField.numberLong() ;
            }
            else
            {
               ss << beField.numberInt() ;
            }
         }
         /// when has multi params, need use [ "$a", "$b" ... ]
         else if ( Array == beField.type() )
         {
            UINT32 paramNum = 0 ;
            BSONObj objParams = beField.embeddedObject() ;
            BSONObjIterator itr( objParams ) ;
            while( itr.more() )
            {
               BSONElement eParam = itr.next() ;
               if ( String != eParam.type() )
               {
                  PD_LOG( PDERROR, "Function's param[%s] must be string",
                          beField.toString( TRUE, TRUE ).c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else if ( AGGR_KEYWORD_PREFIX != *eParam.valuestr() )
               {
                  PD_LOG( PDERROR, "Function's param[%s] must begin with "
                          "\"$\"", beField.toString( TRUE, TRUE ).c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               if ( paramNum > 0 )
               {
                  ss << ',' ;
               }
               ss << AGGR_CL_DEFAULT_ALIAS"."
                  << ( eParam.valuestr() + 1 ) ;
               ++paramNum ;
            }
         }
         else if ( String == beField.type() )
         {
            if ( AGGR_KEYWORD_PREFIX != *beField.valuestr() )
            {
               PD_LOG( PDERROR, "Function's param[%s] must begin with "
                       "\"$\"", beField.toString( TRUE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            ss << AGGR_CL_DEFAULT_ALIAS"."
               << ( beField.valuestr() + 1 ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Function's param[%s] must be string",
                    beField.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         /// end function
         ss << ')' ;
         strFunc = ss.str() ;

         rc = pTable->getOwnField( strFunc.c_str(), funcField );
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                      strFunc.c_str(), rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse function, occur unexpection: %s",
                   e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

