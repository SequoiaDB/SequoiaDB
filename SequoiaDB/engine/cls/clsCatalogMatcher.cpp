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

   Source File Name = clsCatalogMatcher.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "clsCatalogMatcher.hpp"
#include "clsCatalogPredicate.hpp"
#include "clsCatalogAgent.hpp"
#include "clsTrace.hpp"
#include "pdTrace.hpp"
#include "mthCommon.hpp"


using namespace bson;
namespace engine
{
   clsCatalogMatcher::clsCatalogMatcher( const BSONObj & shardingKey )
   :_predicateSet( shardingKey ),
   _shardingKey( shardingKey )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_LOADPATTERN, "clsCatalogMatcher::loadPattern" )
   INT32 clsCatalogMatcher::loadPattern( const BSONObj &matcher )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_LOADPATTERN ) ;
      _matcher = matcher.copy();
      rc = parseAnObj( _matcher, _predicateSet ) ;
      _predicateSet.adjustByShardingKey();
      PD_RC_CHECK( rc, PDERROR, "Failed to load pattern(rc=%d)", rc );

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_LOADPATTERN, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSEANOBJ, "clsCatalogMatcher::parseAnObj" )
   INT32 clsCatalogMatcher::parseAnObj( const BSONObj &matcher,
                                        clsCatalogPredicateTree &predicateSet )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_PARSEANOBJ ) ;
      clsCatalogPredicateTree *pPredicateSet = NULL;
      BOOLEAN isNew = FALSE ;

      try
      {
         BSONObjIterator i( matcher ) ;
         while ( i.more() )
         {
            BSONElement beTmp = i.next();
            const CHAR *pFieldName = beTmp.fieldName() ;
            if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_OR )
            {
               pPredicateSet = SDB_OSS_NEW
                  clsCatalogPredicateTree( _shardingKey ) ;
               PD_CHECK( pPredicateSet != NULL, SDB_OOM, error, PDERROR,
                         "malloc failed" ) ;
               isNew = TRUE ;
            }
            else
            {
               pPredicateSet = &predicateSet ;
            }
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               rc = parseLogicOp( beTmp, *pPredicateSet );
            }
            else
            {
               rc = parseCmpOp( beTmp, *pPredicateSet );
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the field(rc=%d)",
                         rc ) ;
            if ( isNew )
            {
               predicateSet.addChild( pPredicateSet ) ;
            }
            if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_OR &&
                 predicateSet.isUniverse() )
            {
               goto done ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the matcher(%s), "
                      "occured unexpected error:%s",
                      matcher.toString( false, false ).c_str(),
                      e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSEANOBJ, rc ) ;
      return rc;
   error:
      if ( isNew )
      {
         SDB_OSS_DEL( pPredicateSet );
         pPredicateSet = NULL;
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSELOGICOP, "clsCatalogMatcher::parseLogicOp" )
   INT32 clsCatalogMatcher::parseLogicOp( const  BSONElement & beField,
                                          clsCatalogPredicateTree & predicateSet )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_PARSELOGICOP ) ;
      clsCatalogPredicateTree *pPredicateSet = NULL ;
      BOOLEAN isNew = FALSE ;
      CLS_CATA_LOGIC_TYPE logicType = CLS_CATA_LOGIC_INVALID ;
      try
      {
         const CHAR *pFieldName = beField.fieldName() ;
         if ( beField.type() == Array )
         {
            if ( 'a' == pFieldName[1] && 'n' == pFieldName[2] &&
                 'd' == pFieldName[3] && 0 == pFieldName[4] )
            {
               logicType = CLS_CATA_LOGIC_AND ;
               predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
               pPredicateSet = &predicateSet;
            }
            else if( 'o' == pFieldName[1] && 'r' == pFieldName[2] &&
                     0 == pFieldName[3] )
            {
               if ( predicateSet.getLogicType() != CLS_CATA_LOGIC_INVALID )
               {
                  pPredicateSet =
                     SDB_OSS_NEW clsCatalogPredicateTree( _shardingKey );
                  PD_CHECK( pPredicateSet != NULL, SDB_OOM, error, PDERROR,
                            "malloc failed" ) ;
                  pPredicateSet->setLogicType( CLS_CATA_LOGIC_OR ) ;
                  isNew = TRUE ;
               }
               else
               {
                  predicateSet.setLogicType( CLS_CATA_LOGIC_OR ) ;
                  pPredicateSet = &predicateSet ;
               }
               logicType = CLS_CATA_LOGIC_OR ;
            }
            else // parse "$not"
            {
            }

            if ( logicType != CLS_CATA_LOGIC_INVALID )
            {
               BSONObjIterator iter( beField.embeddedObject() );
               while ( iter.more() )
               {
                  BSONElement beTmp = iter.next();
                  if ( Object == beTmp.type() )
                  {
                     BSONObj boTmp = beTmp.embeddedObject();
                     rc = parseAnObj( boTmp, *pPredicateSet );
                     PD_RC_CHECK( rc, PDERROR, "Failed to parse the field(rc=%d)",
                                  rc ) ;
                  }
                  else
                  {
                     PD_LOG( PDERROR, "Wrong type of logic field" ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
               }
               if ( isNew )
               {
                  predicateSet.addChild( pPredicateSet ) ;
               }
               goto done ;
            }
         }
         if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_OR )
         {
            predicateSet.upgradeToUniverse();
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the field occured "
                      "unexpected error:%s", e.what() );
      }

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSELOGICOP, rc ) ;
      return rc;
   error:
      if ( isNew )
      {
         SDB_OSS_DEL( pPredicateSet );
         pPredicateSet = NULL;
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSECMPOP, "clsCatalogMatcher::parseCmpOp" )
   INT32 clsCatalogMatcher::parseCmpOp( const  BSONElement & beField,
                                        clsCatalogPredicateTree & predicateSet )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_PARSECMPOP ) ;
      const CHAR *pFieldName = NULL ;
      predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
      BSONObj boValue;
      try
      {
         pFieldName = beField.fieldName() ;
         BSONElement beTmp = _shardingKey.getField( pFieldName );
         if ( beTmp.eoo() )
         {
            goto done ;
         }
         if ( beField.type() == Object )
         {
            boValue = beField.embeddedObject() ;
            if ( _isExistUnreconigzeOp( boValue ) )
            {
               goto done ;
            }

            if ( isOpObj( boValue ) )
            {
               rc = parseOpObj( beField, predicateSet ) ;
            }
            else
            {
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               beField.getGtLtOp() ) ;
            }
         }
         else if ( RegEx == beField.type() &&
                   MTH_OPERATOR_EYECATCHER != pFieldName[0] )
         {
            rc = predicateSet.addPredicate( pFieldName, beField,
                                            BSONObj::opREGEX ) ;
         }
         else
         {
            rc = predicateSet.addPredicate( pFieldName, beField,
                                            beField.getGtLtOp() ) ;
         }
         PD_RC_CHECK( rc, PDERROR,
                     "failed to add predicate(rc=%d)", rc );
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR,
                     "failed to parse the field "
                     "occured unexpected error:%s",
                     e.what() );
      }
   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSECMPOP, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSEOPOBJ, "clsCatalogMatcher::parseOpObj" )
   INT32 clsCatalogMatcher::parseOpObj( const BSONElement & beField,
                                        clsCatalogPredicateTree & predicateSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_PARSEOPOBJ ) ;

      SDB_ASSERT( Object == beField.type(), "Should be an object" ) ;

      try
      {
         const CHAR *pFieldName = beField.fieldName() ;
         BSONObj boValue = beField.embeddedObject() ;
         BSONObjIterator i( boValue ) ;
         const CHAR *pRegex = NULL, *pOptions = NULL ;
         while( i.more() )
         {
            BSONElement beTmp = i.next() ;
            INT32 tmpOpType = beTmp.getGtLtOp() ;

            if ( tmpOpType == BSONObj::opREGEX )
            {
               PD_CHECK( String == beTmp.type() && NULL == pRegex,
                         SDB_INVALIDARG, error, PDERROR,
                         "Invalid regular expression operator" ) ;
               pRegex = beTmp.valuestr() ;
            }
            else if ( tmpOpType == BSONObj::opOPTIONS )
            {
               PD_CHECK( String == beTmp.type() && NULL == pOptions,
                         SDB_INVALIDARG, error, PDERROR,
                         "Invalid regular expression operator" ) ;
               pOptions = beTmp.valuestr() ;
            }
            else if ( pRegex )
            {
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               BSONObj::opREGEX ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "failed to add predicate(rc=%d)", rc );
               pRegex = NULL ;
            }
            else if ( pOptions )
            {
               PD_LOG( PDERROR, "Invalid regular expression operator" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            if ( pRegex && pOptions )
            {
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               BSONObj::opREGEX ) ;
               pRegex = NULL ;
               pOptions = NULL ;
            }
            else if ( pRegex || pOptions )
            {
               continue ;
            }
            else
            {
               rc = predicateSet.addPredicate( pFieldName, beTmp,
                                               beTmp.getGtLtOp() ) ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "failed to add predicate(rc=%d)", rc );
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR,
                     "failed to parse the field "
                     "occured unexpected error:%s",
                     e.what() );
      }
   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSEOPOBJ, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_ISEXISTUNRECONIGZEDOP, "clsCatalogMatcher::isExistUnreconigzeOp" )
   BOOLEAN clsCatalogMatcher::_isExistUnreconigzeOp( const bson::BSONObj obj )
   {
      BOOLEAN result = FALSE;
      PD_TRACE_ENTRY( SDB_CLSCATAMATCHER_ISEXISTUNRECONIGZEDOP ) ;
      try
      {
         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            const CHAR *pFieldName = beTmp.fieldName() ;
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               INT32 op = beTmp.getGtLtOp( -1 ) ;
               if ( op == -1 )
               {
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::opMOD && beTmp.isNumber() )
               {
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::opTYPE )
               {
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::opSIZE )
               {
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::Equality &&
                         pFieldName[1] == 'f' && pFieldName[2] == 'i' &&
                         pFieldName[3] == 'e' && pFieldName[4] == 'l' &&
                         pFieldName[5] == 'd' && pFieldName[6] == 0 )
               {
                  result = TRUE ;
                  break ;
               }
            }
            if ( beTmp.type() == Object )
            {
               result = _isExistUnreconigzeOp( beTmp.embeddedObject() ) ;
               if ( result )
               {
                  break ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "failed to check the obj occured unexpected "
                 "error:%s", e.what() ) ;
      }
      PD_TRACE_EXIT( SDB_CLSCATAMATCHER_ISEXISTUNRECONIGZEDOP ) ;
      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_ISOPOBJ, "clsCatalogMatcher::isOpObj" )
   BOOLEAN clsCatalogMatcher::isOpObj( const bson::BSONObj obj )
   {
      BOOLEAN result = FALSE;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_ISOPOBJ ) ;
      try
      {
         BSONObjIterator iter( obj );
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next();
            const CHAR *pFieldName = beTmp.fieldName();
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0])
            {
               result = TRUE;
               break;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "failed to check the obj occured unexpected "
                 "error:%s", e.what() ) ;
      }
      PD_TRACE_EXIT ( SDB_CLSCATAMATCHER_ISOPOBJ ) ;
      return result;
   }

   INT32 clsCatalogMatcher::matches( _clsCatalogItem * pCatalogItem,
                                     BOOLEAN & result )
   {
      return _predicateSet.matches( pCatalogItem, result ) ;
   }

}

