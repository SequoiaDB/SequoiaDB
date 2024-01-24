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
   /*
      clsCatalogMatcher implement
   */
   clsCatalogMatcher::clsCatalogMatcher( const BSONObj &shardingKey,
                                         BOOLEAN isHashShard )
   :_isHashShard( isHashShard ),
    _predicateSet( shardingKey, _isHashShard ),
    _shardingKey( shardingKey )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_LOADPATTERN, "clsCatalogMatcher::loadPattern" )
   INT32 clsCatalogMatcher::loadPattern( const BSONObj &matcher )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_LOADPATTERN ) ;

      try
      {
         _matcher = matcher.getOwned() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = parseAnObj( _matcher, _predicateSet ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Parse matcher failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _predicateSet.done() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Done predicate failed, rc: %d", rc ) ;
         goto error ;
      }

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

      try
      {
         BSONObjIterator i( matcher ) ;
         while ( i.more() )
         {
            BSONElement beTmp = i.next() ;
            const CHAR *pFieldName = beTmp.fieldName() ;

            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               rc = parseLogicOp( beTmp, predicateSet ) ;
            }
            else
            {
               rc = parseCmpOp( beTmp, predicateSet ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the field, rc: %d",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the matcher(%s), "
                      "occured unexpected error:%s",
                      matcher.toString().c_str(),
                      e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSEANOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSELOGICOP, "clsCatalogMatcher::parseLogicOp" )
   INT32 clsCatalogMatcher::parseLogicOp( const BSONElement &beField,
                                          clsCatalogPredicateTree &predicateSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_PARSELOGICOP ) ;
      clsCatalogPredicateTree *pPredicateSet = NULL ;
      BOOLEAN isNew = FALSE ;
      clsCatalogPredicateTree *pSubPredicate = NULL ;
      BOOLEAN isSubNew = FALSE ;
      CLS_CATA_LOGIC_TYPE logicType = CLS_CATA_LOGIC_INVALID ;

      try
      {
         const CHAR *pFieldName = beField.fieldName() ;
         if ( beField.type() == Array )
         {
            if ( 'a' == pFieldName[1] && 'n' == pFieldName[2] &&
                 'd' == pFieldName[3] && 0 == pFieldName[4] )
            {
               // parse "$and"
               logicType = CLS_CATA_LOGIC_AND ;
               predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
               pPredicateSet = &predicateSet ;
            }
            else if( 'o' == pFieldName[1] && 'r' == pFieldName[2] &&
                     0 == pFieldName[3] )
            {
               /// when there is one element, change to $and
               if ( beField.embeddedObject().nFields() <= 1 )
               {
                  predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
                  pPredicateSet = &predicateSet ;
               }
               // parse "$or"
               else if ( predicateSet.getLogicType() != CLS_CATA_LOGIC_INVALID )
               {
                  pPredicateSet = SDB_OSS_NEW
                     clsCatalogPredicateTree( _shardingKey, _isHashShard ) ;
                  PD_CHECK( pPredicateSet != NULL, SDB_OOM, error, PDERROR,
                            "Malloc failed" ) ;
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
               // now "$not" is regarded as universe set
            }

            if ( logicType != CLS_CATA_LOGIC_INVALID )
            {
               BSONObjIterator iter( beField.embeddedObject() ) ;
               while ( iter.more() )
               {
                  BSONElement beTmp = iter.next() ;
                  if ( Object == beTmp.type() )
                  {
                     BSONObj boTmp = beTmp.embeddedObject() ;

                     /// make sure sub predicate
                     if ( pPredicateSet->getLogicType() == CLS_CATA_LOGIC_OR )
                     {
                        pSubPredicate = SDB_OSS_NEW
                           clsCatalogPredicateTree( _shardingKey, _isHashShard ) ;
                        PD_CHECK( pSubPredicate != NULL, SDB_OOM, error, PDERROR,
                                  "malloc failed" ) ;
                        isSubNew = TRUE ;
                     }
                     else
                     {
                        pSubPredicate = pPredicateSet ;
                        isSubNew = FALSE ;
                     }

                     rc = parseAnObj( boTmp, *pSubPredicate ) ;
                     PD_RC_CHECK( rc, PDERROR,
                                  "Failed to parse the field, rc: %d", rc ) ;

                     if ( isSubNew )
                     {
                        rc = pPredicateSet->addChild( pSubPredicate ) ;
                        if ( rc )
                        {
                           goto error ;
                        }
                        isSubNew = FALSE ;
                     }
                     if ( pPredicateSet->getLogicType() == CLS_CATA_LOGIC_OR &&
                          pPredicateSet->isUniverse() )
                     {
                        // $or: ignore all predicatesets in universe set
                        break ;
                     }
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
                  rc = predicateSet.addChild( pPredicateSet ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
                  isNew = FALSE ;
               }
               goto done ;
            }
         }

         // the regular expresion is regarded as universe set.
         // if it is in the $or then upgrade to universe set
         // and ignore the remaining elements.
         // if it is in the $and then ignore this element.
         if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_OR )
         {
            // clear all predicates and upgrade to universe set
            predicateSet.upgradeToUniverse() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the field occured "
                      "unexpected error: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSELOGICOP, rc ) ;
      return rc;
   error:
      if ( isSubNew )
      {
         SDB_OSS_DEL pSubPredicate ;
         pSubPredicate = NULL ;
      }
      if ( isNew )
      {
         SDB_OSS_DEL pPredicateSet ;
         pPredicateSet = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSECMPOP, "clsCatalogMatcher::parseCmpOp" )
   INT32 clsCatalogMatcher::parseCmpOp( const BSONElement &beField,
                                        clsCatalogPredicateTree &predicateSet )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_PARSECMPOP ) ;
      const CHAR *pFieldName = NULL ;
      predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
      BSONObj boValue ;

      try
      {
         pFieldName = beField.fieldName() ;
         BSONElement beTmp = _shardingKey.getField( pFieldName ) ;
         if ( beTmp.eoo() )
         {
            // ignore the field which is not sharding-key
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
               // Object contains match operators
               rc = parseOpObj( beField, predicateSet ) ;
            }
            else
            {
               // Just a simple object
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               beField.getGtLtOp() ) ;
            }
         }
         else if ( RegEx == beField.type() &&
                   MTH_OPERATOR_EYECATCHER != pFieldName[0] )
         {
            if ( !_isHashShard )
            {
               // It is a { 'xx': { $regex: 'xxx', $options: 'xxx' } }
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               BSONObj::opREGEX ) ;
            }
         }
         else
         {
            rc = predicateSet.addPredicate( pFieldName, beField,
                                            beField.getGtLtOp() ) ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add predicate, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse the field occured unexpected error: %s",
                      e.what() ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSECMPOP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_PARSEOPOBJ, "clsCatalogMatcher::parseOpObj" )
   INT32 clsCatalogMatcher::parseOpObj( const BSONElement &beField,
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
               // It is a { $regex:'xxx', ... } case
               // Put the original element to predicate, it will parse
               // the regex operator
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               BSONObj::opREGEX ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to add predicate, rc: %d", rc ) ;
               pRegex = NULL ;
            }
            else if ( pOptions )
            {
               // Put the original element to predicate, it will parse
               // the regex operator
               PD_LOG( PDERROR, "Invalid regular expression operator" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            if ( pRegex && pOptions )
            {
               // It is a { $regex:'xxx', $options:'xxx', ... }
               // Put the original element to predicate, it will parse
               // the regex operator
               rc = predicateSet.addPredicate( pFieldName, beField,
                                               BSONObj::opREGEX ) ;
               pRegex = NULL ;
               pOptions = NULL ;
            }
            else if ( pRegex || pOptions )
            {
               // Delay until we parse the next element
               continue ;
            }
            else
            {
               // It is a match operator
               rc = predicateSet.addPredicate( pFieldName, beTmp,
                                               beTmp.getGtLtOp() ) ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to add predicate, rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse the field occured unexpected error: %s",
                      e.what() ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAMATCHER_PARSEOPOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN clsCatalogMatcher::_isExistUnreconigzeOp( const BSONObj &obj ) const
   {
      if ( _isHashShard )
      {
         return _isHashExistUnreconigzeOp( obj ) ;
      }
      return _isRangeExistUnreconigzeOp( obj ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_ISHASHEXISTUNRECONIGZEDOP, "clsCatalogMatcher::_isHashExistUnreconigzeOp" )
   BOOLEAN clsCatalogMatcher::_isHashExistUnreconigzeOp( const BSONObj &obj ) const
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY( SDB_CLSCATAMATCHER_ISHASHEXISTUNRECONIGZEDOP ) ;

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
               if ( BSONObj::Equality == op )
               {
                  if ( pFieldName[1] == 'f' && pFieldName[2] == 'i' &&
                       pFieldName[3] == 'e' && pFieldName[4] == 'l' &&
                       pFieldName[5] == 'd' && pFieldName[6] == 0 )
                  {
                     result = TRUE ;
                     break ;
                  }
               }
               else if ( BSONObj::opIN == op )
               {
                  /// do nothing
               }
               else if ( BSONObj::opISNULL == op )
               {
                  if ( !beTmp.isNumber() || 1 != beTmp.numberInt() )
                  {
                     result = TRUE ;
                     break ;
                  }
               }
               else if ( BSONObj::opEXISTS == op )
               {
                  if ( !beTmp.isNumber() || 0 != beTmp.numberInt() )
                  {
                     result = TRUE ;
                     break ;
                  }
               }
               else
               {
                  result = TRUE ;
                  break ;
               }
            }

            if ( beTmp.type() == Object )
            {
               // Recursively check inner object
               result = _isHashExistUnreconigzeOp( beTmp.embeddedObject() ) ;
               if ( result )
               {
                  break ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to check the obj occured unexpected "
                 "error: %s", e.what() ) ;
      }

      PD_TRACE_EXIT( SDB_CLSCATAMATCHER_ISHASHEXISTUNRECONIGZEDOP ) ;
      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_ISRANGEEXISTUNRECONIGZEDOP, "clsCatalogMatcher::_isRangeExistUnreconigzeOp" )
   BOOLEAN clsCatalogMatcher::_isRangeExistUnreconigzeOp( const BSONObj &obj ) const
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY( SDB_CLSCATAMATCHER_ISRANGEEXISTUNRECONIGZEDOP ) ;

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
                  // $mod:num is a function
                  // Note: $mod:[num,num] is a recognized operator
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::opTYPE )
               {
                  // $type is a function now
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::opSIZE )
               {
                  // $size is a function now
                  result = TRUE ;
                  break ;
               }
               else if ( op == BSONObj::Equality &&
                         pFieldName[1] == 'f' && pFieldName[2] == 'i' &&
                         pFieldName[3] == 'e' && pFieldName[4] == 'l' &&
                         pFieldName[5] == 'd' && pFieldName[6] == 0 )
               {
                  // $field should not be used to generate predicate
                  result = TRUE ;
                  break ;
               }
            }
            if ( beTmp.type() == Object )
            {
               // Recursively check inner object
               result = _isRangeExistUnreconigzeOp( beTmp.embeddedObject() ) ;
               if ( result )
               {
                  break ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to check the obj occured unexpected "
                 "error: %s", e.what() ) ;
      }
      PD_TRACE_EXIT( SDB_CLSCATAMATCHER_ISRANGEEXISTUNRECONIGZEDOP ) ;
      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAMATCHER_ISOPOBJ, "clsCatalogMatcher::isOpObj" )
   BOOLEAN clsCatalogMatcher::isOpObj( const BSONObj &obj ) const
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY ( SDB_CLSCATAMATCHER_ISOPOBJ ) ;

      try
      {
         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            const CHAR *pFieldName = beTmp.fieldName();
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               result = TRUE ;
               break ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to check the obj occured unexpected "
                 "error: %s", e.what() ) ;
      }
      PD_TRACE_EXIT ( SDB_CLSCATAMATCHER_ISOPOBJ ) ;
      return result ;
   }

   INT32 clsCatalogMatcher::calc( const _clsCatalogSet *pSet,
                                  CLS_SET_CATAITEM &setItem )
   {
      return _predicateSet.calc( pSet, setItem ) ;
   }

   BOOLEAN clsCatalogMatcher::isUniverse() const
   {
      return _predicateSet.isUniverse() ;
   }

   BOOLEAN clsCatalogMatcher::isNull() const
   {
      return _predicateSet.isNull() ;
   }

}

