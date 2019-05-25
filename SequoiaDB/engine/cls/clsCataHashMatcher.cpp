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

   Source File Name = clsCataHashMatcher.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2014-02-13  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsCataHashMatcher.hpp"
#include "clsCatalogAgent.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "mthCommon.hpp"

using namespace bson;
namespace engine
{

   clsCataHashPredTree::clsCataHashPredTree( BSONObj shardingKey )
   : _logicType( CLS_CATA_LOGIC_INVALID ),
   _shardingKey( shardingKey ),
   _hasPred( FALSE ),
   _isNull( FALSE )
   {
   }

   clsCataHashPredTree::~clsCataHashPredTree()
   {
      clear() ;
   }

   void clsCataHashPredTree::upgradeToUniverse()
   {
      clear() ;
   }

   BOOLEAN clsCataHashPredTree::isNull()
   {
      return _isNull ;
   }

   BOOLEAN clsCataHashPredTree::isUniverse()
   {
      BOOLEAN result = TRUE ;
      if ( _isNull )
      {
         result = FALSE ;
         goto done ;
      }
      if ( _children.size() != 0 )
      {
         result = FALSE ;
         goto done ;
      }
      if ( _fieldSet.size() != 0 )
      {
         result = FALSE ;
         goto done ;
      }
   done:
      return result ;
   }

   CLS_CATA_LOGIC_TYPE clsCataHashPredTree::getLogicType()
   {
      return _logicType ;
   }

   void clsCataHashPredTree::setLogicType( CLS_CATA_LOGIC_TYPE type )
   {
      _logicType = type ;
   }

   void clsCataHashPredTree::addChild( clsCataHashPredTree *&pChild )
   {
      if ( !pChild->isNull() && !pChild->isUniverse() )
      {
         _children.push_back( pChild );
      }
      else
      {
         if ( pChild->isNull() )
         {
            if ( CLS_CATA_LOGIC_AND == _logicType )
            {
               _isNull = TRUE ;
            }
         }
         else
         {
            if ( _logicType != CLS_CATA_LOGIC_AND )
            {
               upgradeToUniverse();
            }
         }
         SDB_OSS_DEL( pChild ) ;
         pChild = NULL ;
      }
   }

   INT32 clsCataHashPredTree::addPredicate( const CHAR *pFieldName,
                                            BSONElement beField )
   {
      INT32 rc = SDB_OK ;
      MAP_CLSCATAHASHPREDFIELDS::iterator iter ;

      if ( CLS_CATA_LOGIC_OR == _logicType && _fieldSet.size() >= 1 )
      {
         clsCataHashPredTree *pChild = NULL ;
         pChild = SDB_OSS_NEW clsCataHashPredTree( _shardingKey ) ;
         PD_CHECK( pChild != NULL, SDB_OOM, error, PDERROR,
                   "malloc failed!" ) ;
         pChild->setLogicType( CLS_CATA_LOGIC_AND ) ;
         rc = pChild->addPredicate( pFieldName, beField ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add predicate(rc=%d)", rc ) ;
         addChild( pChild ) ;
         goto done ;
      }
      iter = _fieldSet.find( pFieldName ) ;
      if ( iter != _fieldSet.end() )
      {
         if ( 0 != beField.woCompare( iter->second, FALSE ) )
         {
            clear() ;
            _isNull = TRUE ;
         }
         goto done ;
      }
      _fieldSet[ pFieldName ] = beField ;
   done:
      return rc ;
   error:
      goto done ;
   }

   void clsCataHashPredTree::clear()
   {
      clsCataHashPredTree *pTmp ;
      UINT32 i = 0 ;
      for ( ; i < _children.size() ; i++ )
      {
         pTmp = _children[i] ;
         SDB_OSS_DEL( pTmp ) ;
      }
      _children.clear() ;
      _fieldSet.clear() ;
   }

   INT32 clsCataHashPredTree::generateHashPredicate( UINT32 partitionBit,
                                                     UINT32 internalV )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BOOLEAN includeAllKey = TRUE ;
         UINT32 i = 0 ;
         BSONObjBuilder bobKey ;
         BSONObj objKey ;
         BSONElement eleShard ;
         BSONObjIterator iterKey( _shardingKey ) ;
         if ( CLS_CATA_LOGIC_AND == _logicType && isNull() )
         {
            goto done ;
         }
         while( iterKey.more() )
         {
            BSONElement beTmp = iterKey.next() ;
            const CHAR *pFieldName = beTmp.fieldName() ;
            MAP_CLSCATAHASHPREDFIELDS::iterator iterField ;
            iterField = _fieldSet.find( pFieldName ) ;
            if ( _fieldSet.end() == iterField )
            {
               includeAllKey = FALSE ;
               if ( _logicType != CLS_CATA_LOGIC_AND )
               {
                  upgradeToUniverse() ;
                  goto done ;
               }
               _fieldSet.clear() ;
               break ;
            }
            else if ( Array == iterField->second.type() )
            {
               BSONObj tmpObj( iterField->second.embeddedObject() ) ;
               if ( tmpObj.nFields() > 1 )
               {
                  includeAllKey = FALSE ;
                  if ( _logicType != CLS_CATA_LOGIC_AND )
                  {
                     upgradeToUniverse() ;
                     goto done ;
                  }
                  _fieldSet.clear() ;
                  break ;
               }
               eleShard = tmpObj.firstElement() ;
            }
            else
            {
               eleShard = iterField->second ;
            }

            bobKey.appendAs( eleShard, "" ) ;
         }

         if ( includeAllKey )
         {
            objKey = bobKey.obj() ;
            _hashVal = clsPartition( objKey,
                                     partitionBit,
                                     internalV ) ;
            _hasPred = TRUE ;
         }

         for ( i = 0 ; i < _children.size() ; i++ )
         {
            rc = _children[i]->generateHashPredicate( partitionBit, internalV ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to generate hash predicate "
                         "for children(rc=%d)", rc ) ;
            if ( _logicType != CLS_CATA_LOGIC_AND &&
                 _children[i]->isUniverse() )
            {
               upgradeToUniverse() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed generate hash predicate occured "
                 "unexpected error:%s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 clsCataHashPredTree::matches( _clsCatalogItem* pCatalogItem,
                                       BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN rsTmp = TRUE ;
      UINT32 i = 0 ;

      if ( isNull() )
      {
         rsTmp = FALSE ;
         goto done ;
      }
      if ( isUniverse() )
      {
         goto done ;
      }
      try
      {
         if ( _hasPred )
         {
            INT32 hashLB ;
            INT32 hashUB ;
            BSONElement beLB = pCatalogItem->getLowBound().firstElement() ;
            BSONElement beUB = pCatalogItem->getUpBound().firstElement() ;
            PD_CHECK( beLB.isNumber()&& beUB.isNumber(), SDB_SYS, error,
                      PDERROR, "Found invalid item, match failed!" ) ;
            hashLB = beLB.numberInt() ;
            hashUB = beUB.numberInt() ;
            if ( _hashVal < hashLB || _hashVal >= hashUB )
            {
               rsTmp = FALSE ;
               if ( CLS_CATA_LOGIC_AND == _logicType )
               {
                  goto done ;
               }
            }
            else
            {
               if ( CLS_CATA_LOGIC_AND != _logicType )
               {
                  goto done ;
               }
            }
         }
         for ( i = 0 ; i < _children.size() ; i++ )
         {
            rc = _children[i]->matches( pCatalogItem, rsTmp ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to match the children!" ) ;
            if ( !rsTmp && CLS_CATA_LOGIC_AND == _logicType )
            {
               goto done ;
            }
            else if ( rsTmp && CLS_CATA_LOGIC_AND != _logicType )
            {
               goto done ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Match failed, occured unexpected error:%s",
                 e.what() ) ;
         goto error ;
      }
   done:
      result = rsTmp ;
      return rc ;
   error:
      goto done ;
   }

   string clsCataHashPredTree::toString() const
   {
      StringBuilder buf ;
      buf << "[ " ;
      if ( CLS_CATA_LOGIC_INVALID == _logicType )
      {
         buf << "$invalid: " ;
      }
      else if ( CLS_CATA_LOGIC_AND == _logicType )
      {
         buf << "$and: " ;
      }
      else if ( CLS_CATA_LOGIC_OR == _logicType )
      {
         buf << "$or: " ;
      }
      else
      {
         buf << _logicType << ": " ;
      }

      if ( _fieldSet.size() > 0 )
      {
         MAP_CLSCATAHASHPREDFIELDS::const_iterator cit = _fieldSet.begin() ;
         while ( cit != _fieldSet.end() )
         {
            buf << "{ " << cit->first << ": "
                << cit->second.toString( false, false ) << " }" ;
            ++cit ;
         }
      }

      if ( _hasPred )
      {
         buf << "{ hashVal: " << _hashVal << " }" ;
      }

      if ( _isNull )
      {
         buf << "{ isNull: true }" ;
      }

      for ( UINT32 i = 0 ; i < _children.size() ; ++i )
      {
         buf << _children[ i ]->toString() ;
      }

      buf << " ]" ;
      return buf.str() ;
   }

   clsCataHashMatcher::clsCataHashMatcher( const BSONObj &shardingKey )
   :_predicateSet( shardingKey ),
   _shardingKey( shardingKey )
   {
   }

   INT32 clsCataHashMatcher::loadPattern( const BSONObj &matcher,
                                          UINT32 partitionBit,
                                          UINT32 internalV )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _matcher = matcher.copy() ;
         rc = parseAnObj( _matcher, _predicateSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load pattern(rc=%d)", rc ) ;

         rc = _predicateSet.generateHashPredicate( partitionBit, internalV ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate hash value(rc=%d)",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the matcher(%s), "
                      "occured unexpected error:%s",
                      matcher.toString().c_str(), e.what() ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_CLSCATAHASHMATCHER_PARSEANOBJ, "clsCataHashMatcher::parseAnObj" )
   INT32 clsCataHashMatcher::parseAnObj( const BSONObj &matcher,
                                         clsCataHashPredTree &predicateSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAHASHMATCHER_PARSEANOBJ ) ;
      try
      {
         if ( matcher.nFields() > 1 &&
              predicateSet.getLogicType() == CLS_CATA_LOGIC_INVALID )
         {
            predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
         }
         BSONObjIterator iter( matcher ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            const CHAR *pFieldName = beTmp.fieldName();
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               rc = parseLogicOp( beTmp, predicateSet ) ;
            }
            else
            {
               rc = parseCmpOp( beTmp, predicateSet ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the field(rc=%d)",
                         rc ) ;

            if ( ( predicateSet.isUniverse() &&
                   predicateSet.getLogicType() != CLS_CATA_LOGIC_AND ) ||
                 ( predicateSet.isNull() &&
                   predicateSet.getLogicType() == CLS_CATA_LOGIC_AND ) )
            {
               break ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the matcher(%s), "
                      "occured unexpected error:%s",
                      matcher.toString().c_str(), e.what() ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAHASHMATCHER_PARSEANOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_CLSCATAHASHMATCHER_PARSELOGICOP, "clsCataHashMatcher::parseLogicOp" )
   INT32 clsCataHashMatcher::parseLogicOp( const bson::BSONElement &beField,
                                           clsCataHashPredTree &predicateSet )
   {
      INT32 rc = SDB_OK ;
      clsCataHashPredTree *pPredicateSet = NULL ;
      BOOLEAN isNewChild = FALSE ;
      PD_TRACE_ENTRY ( SDB_CLSCATAHASHMATCHER_PARSELOGICOP ) ;
      try
      {
         const CHAR *pFieldName = beField.fieldName() ;

         if ( beField.type() != Array )
         {
            if ( predicateSet.getLogicType() != CLS_CATA_LOGIC_AND )
            {
               predicateSet.upgradeToUniverse();
            }
            goto done ;
         }

         if ( 'a' == pFieldName[1] && 'n' == pFieldName[2] &&
              'd' == pFieldName[3] && 0 == pFieldName[4] )
         {
            if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_OR )
            {
               pPredicateSet = SDB_OSS_NEW clsCataHashPredTree( _shardingKey ) ;
               PD_CHECK( pPredicateSet != NULL, SDB_OOM, error, PDERROR,
                         "malloc failed!" ) ;
               pPredicateSet->setLogicType( CLS_CATA_LOGIC_AND ) ;
               isNewChild = TRUE ;
            }
            else
            {
               pPredicateSet = &predicateSet ;
               if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_INVALID )
               {
                  predicateSet.setLogicType( CLS_CATA_LOGIC_AND ) ;
               }
            }
         }
         else if ( 'o' == pFieldName[1] && 'r' == pFieldName[2] &&
                   0 == pFieldName[3] )
         {
            if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_AND )
            {
               pPredicateSet = SDB_OSS_NEW clsCataHashPredTree( _shardingKey ) ;
               PD_CHECK( pPredicateSet != NULL, SDB_OOM, error, PDERROR,
                         "malloc failed!" ) ;
               pPredicateSet->setLogicType( CLS_CATA_LOGIC_OR ) ;
               isNewChild = TRUE ;
            }
            else
            {
               pPredicateSet = &predicateSet ;
               if ( predicateSet.getLogicType() == CLS_CATA_LOGIC_INVALID )
               {
                  predicateSet.setLogicType( CLS_CATA_LOGIC_OR ) ;
               }
            }
         }
         else
         {
            if ( predicateSet.getLogicType() != CLS_CATA_LOGIC_AND )
            {
               predicateSet.upgradeToUniverse();
            }
            goto done ;
         }

         {
            BSONObjIterator iter( beField.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONObj boTmp ;
               BSONElement beTmp = iter.next() ;
               PD_CHECK( beTmp.type() == Object, SDB_INVALIDARG, error,
                         PDERROR, "Failed to parse logic-operation field, "
                         "the field type must be Object!" ) ;
               boTmp = beTmp.embeddedObject() ;
               rc = parseAnObj( boTmp, *pPredicateSet );
               PD_RC_CHECK( rc, PDERROR, "Failed to parse the field"
                            "( field:%s, rc=%d )", beTmp.toString().c_str(),
                            rc ) ;
            }
            if ( isNewChild )
            {
               predicateSet.addChild( pPredicateSet );
               isNewChild = FALSE ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the field, occured "
                      "unexpected error:%s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAHASHMATCHER_PARSELOGICOP, rc ) ;
      return rc ;
   error:
      if ( isNewChild )
      {
         SDB_OSS_DEL( pPredicateSet ) ;
         pPredicateSet = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_CLSCATAHASHMATCHER_PARSECMPOP, "clsCataHashMatcher::parseCmpOp" )
   INT32 clsCataHashMatcher::parseCmpOp( const bson::BSONElement &beField,
                                         clsCataHashPredTree &predicateSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAHASHMATCHER_PARSECMPOP ) ;
      try
      {
         const CHAR *pFieldName = NULL ;
         pFieldName = beField.fieldName() ;
         BSONElement beTmp = _shardingKey.getField( pFieldName ) ;
         if ( beTmp.eoo() )
         {
            goto done ;
         }
         if ( beField.type() == Object )
         {
            INT32 opType ;
            BSONObj boValue = beField.embeddedObject() ;
            rc = checkOpObj( boValue, opType ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the field(rc=%d)",
                         rc ) ;
            if ( PREDICATE_OBJ_TYPE_OP_NOT_EQ == opType )
            {
               if ( predicateSet.getLogicType() != CLS_CATA_LOGIC_AND )
               {
                  predicateSet.upgradeToUniverse() ;
               }
               goto done;
            }
            else if ( PREDICATE_OBJ_TYPE_OP_EQ == opType )
            {
               BSONObjIterator iter( boValue ) ;
               while( iter.more() )
               {
                  BSONElement beTmp = iter.next() ;
                  rc = predicateSet.addPredicate( pFieldName, beTmp ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to add predicate(rc=%d)",
                               rc ) ;
               }
               goto done ;
            }
         }
         else if ( beField.type() == RegEx )
         {
            if ( predicateSet.getLogicType() != CLS_CATA_LOGIC_AND )
            {
               predicateSet.upgradeToUniverse() ;
            }
            goto done ;
         }
         rc = predicateSet.addPredicate( pFieldName, beField ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add predicate(rc=%d)", rc ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse the field, occured "
                      "unexpected error:%s", e.what() ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAHASHMATCHER_PARSECMPOP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAHASHMATCHER_CHECKOPOBJ, "clsCataHashMatcher::checkOpObj" )
   INT32 clsCataHashMatcher::checkOpObj( const bson::BSONObj obj,
                                         INT32 &result )
   {
      INT32 rc = SDB_OK ;
      result = PREDICATE_OBJ_TYPE_NOT_OP ;
      PD_TRACE_ENTRY ( SDB_CLSCATAHASHMATCHER_CHECKOPOBJ ) ;
      try
      {
         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            const CHAR *pFieldName = beTmp.fieldName() ;
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               if ( 'e' == pFieldName[1] && 't' == pFieldName[2] &&
                    0 == pFieldName[3] )
               {
                  if ( iter.more() )
                  {
                     result = PREDICATE_OBJ_TYPE_OP_NOT_EQ ;
                  }
                  else
                  {
                     if ( beTmp.type() == Object )
                     {
                        INT32 tmpResult = PREDICATE_OBJ_TYPE_NOT_OP ;
                        rc = checkETInnerObj( beTmp.embeddedObject(),
                                              tmpResult ) ;
                        PD_RC_CHECK( rc, PDERROR, "Failed to parse object "
                                     "inside $et, rc: %d", rc ) ;
                        if ( tmpResult == PREDICATE_OBJ_TYPE_NOT_OP )
                        {
                           result = PREDICATE_OBJ_TYPE_OP_EQ ;
                        }
                        else
                        {
                           result = PREDICATE_OBJ_TYPE_OP_NOT_EQ ;
                        }
                     }
                     else
                     {
                        result = PREDICATE_OBJ_TYPE_OP_EQ ;
                     }
                  }
               }
               else
               {
                  result = PREDICATE_OBJ_TYPE_OP_NOT_EQ ;
               }
            }
            break ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to check the obj occured unexpected error:%s",
                 e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXIT ( SDB_CLSCATAHASHMATCHER_CHECKOPOBJ ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAHASHMATCHER_CHECKETINNER, "clsCataHashMatcher::checkETInnerObj" )
   INT32 clsCataHashMatcher::checkETInnerObj( const bson::BSONObj obj,
                                              INT32 &result )
   {
      INT32 rc = SDB_OK ;

      result = PREDICATE_OBJ_TYPE_NOT_OP ;

      PD_TRACE_ENTRY ( SDB_CLSCATAHASHMATCHER_CHECKETINNER ) ;

      try
      {
         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            const CHAR *pFieldName = beTmp.fieldName() ;
            if ( MTH_OPERATOR_EYECATCHER == pFieldName[0] )
            {
               INT32 opType = beTmp.getGtLtOp() ;
               if ( opType != BSONObj::opREGEX &&
                    opType != BSONObj::opOPTIONS )
               {
                  result = PREDICATE_OBJ_TYPE_OP_NOT_EQ ;
                  break ;
               }
               else
               {
                  PD_CHECK( beTmp.type() == String, SDB_INVALIDARG, error,
                            PDERROR, "Failed to parse regex operator" ) ;
               }
            }
            else if ( beTmp.type() == Object )
            {
               rc = checkETInnerObj( beTmp.embeddedObject(), result ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse inner object, "
                            "rc: %d", rc ) ;
               if ( result != PREDICATE_OBJ_TYPE_NOT_OP )
               {
                  break ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to check the obj occured unexpected error:%s",
                 e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXIT ( SDB_CLSCATAHASHMATCHER_CHECKETINNER ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAHASHMATCHER_MATCHES, "clsCataHashMatcher::matches" )
   INT32 clsCataHashMatcher::matches( _clsCatalogItem* pCatalogItem,
                                      BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAHASHMATCHER_MATCHES ) ;
      rc = _predicateSet.matches( pCatalogItem, result ) ;
      PD_RC_CHECK( rc, PDERROR, "match failed(rc=%d)", rc ) ;
   done:
      PD_TRACE_EXIT ( SDB_CLSCATAHASHMATCHER_MATCHES ) ;
      return rc ;
   error:
      goto done ;
   }

}
