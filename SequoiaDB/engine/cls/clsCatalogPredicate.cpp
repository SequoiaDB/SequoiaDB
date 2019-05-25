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

   Source File Name = clsCatalogPredicats.cpp

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

#include "core.hpp"
#include "pd.hpp"
#include "clsCatalogPredicate.hpp"
#include "clsCatalogAgent.hpp"
#include "clsTrace.hpp"
#include "pdTrace.hpp"

using namespace bson;
namespace engine
{
   clsCatalogPredicateTree::clsCatalogPredicateTree( BSONObj shardingKey ) :
   _logicType( CLS_CATA_LOGIC_INVALID ),
   _shardingKey( shardingKey )
   {
   }

   clsCatalogPredicateTree::~clsCatalogPredicateTree()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ADDCHILD, "clsCatalogPredicateTree::addChild" )
   void clsCatalogPredicateTree::addChild( clsCatalogPredicateTree * pChild )
   {
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ADDCHILD ) ;
      SDB_ASSERT ( pChild, "pchild can't be null" ) ;
      pChild->adjustByShardingKey() ;

      if ( FALSE == pChild->isUniverse() )
      {
         _children.push_back( pChild ) ;
         goto done ;
      }

      if ( CLS_CATA_LOGIC_OR == _logicType )
      {
         upgradeToUniverse() ;
      }
      SDB_OSS_DEL( pChild ) ;

   done:
      PD_TRACE_EXIT ( SDB_CLSCATAPREDICATETREE_ADDCHILD );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_CLEAR, "clsCatalogPredicateTree::clear" )
   void clsCatalogPredicateTree::clear()
   {
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_CLEAR ) ;
      clsCatalogPredicateTree *pTmp = NULL ;
      while ( !_children.empty() )
      {
         pTmp = _children.back() ;
         _children.pop_back() ;
         SDB_OSS_DEL( pTmp ) ;
      }

      _predicateSet.clear() ;

      PD_TRACE_EXIT ( SDB_CLSCATAPREDICATETREE_CLEAR );
   }

   void clsCatalogPredicateTree::upgradeToUniverse()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ADDPREDICATE, "clsCatalogPredicateTree::addPredicate" )
   INT32 clsCatalogPredicateTree::addPredicate( const CHAR *pFieldName,
                                                BSONElement beField,
                                                INT32 opType )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ADDPREDICATE ) ;

      rc = _predicateSet.addPredicate( pFieldName, beField, opType, FALSE,
                                       TRUE, FALSE, -1, -1 ) ;

      PD_TRACE_EXITRC ( SDB_CLSCATAPREDICATETREE_ADDPREDICATE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ADJUSTBYSHARDINGKEY, "clsCatalogPredicateTree::adjustByShardingKey" )
   void clsCatalogPredicateTree::adjustByShardingKey()
   {
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ADJUSTBYSHARDINGKEY ) ;
      try
      {
         const CHAR *pFirstKeyName = _shardingKey.firstElementFieldName() ;
         const RTN_PREDICATE_MAP &mapPredicate = _predicateSet.predicates() ;

         if ( _logicType != CLS_CATA_LOGIC_AND ||
              mapPredicate.size() == 0 ||
              _children.size() != 0 )
         {
            goto done ;
         }
         if ( 0 == pFirstKeyName[0] )
         {
            goto done;
         }
         if ( mapPredicate.find( pFirstKeyName ) != mapPredicate.end() )
         {
            goto done ;
         }
         _predicateSet.clear() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to adjust the obj occured unexpected "
                 "error:%s", e.what() ) ;
      }

   done :
      PD_TRACE_EXIT ( SDB_CLSCATAPREDICATETREE_ADJUSTBYSHARDINGKEY );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ISUNIVERSE, "clsCatalogPredicateTree::isUniverse" )
   BOOLEAN clsCatalogPredicateTree::isUniverse()
   {
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ISUNIVERSE ) ;
      BOOLEAN result = TRUE;
      if ( _children.size() != 0 )
      {
         result = FALSE;
         goto done;
      }
      if ( _predicateSet.predicates().size() != 0 )
      {
         result = FALSE;
         goto done;
      }
   done:
      PD_TRACE_EXIT ( SDB_CLSCATAPREDICATETREE_ISUNIVERSE );
      return result;
   }

   void clsCatalogPredicateTree::setLogicType( CLS_CATA_LOGIC_TYPE type )
   {
      _logicType = type;
   }

   CLS_CATA_LOGIC_TYPE clsCatalogPredicateTree::getLogicType()
   {
      return _logicType;
   }

   string clsCatalogPredicateTree::toString() const
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

      if ( _predicateSet.predicates().size() > 0 )
      {
         buf << _predicateSet.toString() ;
      }

      for ( UINT32 i = 0 ; i < _children.size() ; ++i )
      {
         buf << _children[ i ]->toString() ;
      }

      buf << " ]" ;
      return buf.str() ;
   }

   INT32 clsCatalogPredicateTree::_matches ( BSONObjIterator itrSK,
                                             BSONObjIterator itrLB,
                                             BSONObjIterator itrUB,
                                             BOOLEAN &result,
                                             BOOLEAN isCloseInterval,
                                             INT32 compareLU )
   {
      INT32 rc = SDB_OK ;
      UINT32 ssKeyPos = 0 ;
      INT32 rsCmp = 0 ;
      INT32 director = 1 ;
      BSONElement lowBound ;
      BSONElement upBound ;
      BSONElement beShardingKey ;
      const RTN_PREDICATE_MAP &predicates = _predicateSet.predicates() ;
      RTN_PREDICATE_MAP::const_iterator itr ;

      if ( !itrSK.more() )
      {
         result = isCloseInterval ? TRUE : FALSE ;
         goto done ;
      }

      SDB_ASSERT( itrLB.more() && itrUB.more(), "Invalid catalog bound" ) ;

      beShardingKey = itrSK.next() ;
      director = beShardingKey.numberInt() > 0 ? 1 : -1 ;
      ssKeyPos = 0 ;

      lowBound = itrLB.next() ;
      upBound = itrUB.next() ;

      itr = predicates.find( beShardingKey.fieldName() ) ;
      if ( itr == predicates.end() )
      {
         result = TRUE ;
         goto done ;
      }

   retry:
      if ( ssKeyPos >= itr->second._startStopKeys.size() )
      {
         result = FALSE ;
         goto done ;
      }

      {
         const rtnStartStopKey &matcherBound =
            itr->second._startStopKeys[ ssKeyPos ] ;
         const rtnKeyBoundary *pStartKey = NULL ;
         const rtnKeyBoundary *pStopKey = NULL ;

         if ( director > 0 )
         {
            pStartKey = &(matcherBound._startKey) ;
            pStopKey = &(matcherBound._stopKey) ;
         }
         else
         {
            pStartKey = &(matcherBound._stopKey) ;
            pStopKey = &(matcherBound._startKey) ;
         }

         if ( compareLU <= 0 )
         {
            rsCmp = rtnKeyCompare( lowBound, pStopKey->_bound ) ;
            rsCmp *= director ;
            if ( rsCmp > 0 || ( rsCmp == 0 && !pStopKey->_inclusive ) )
            {
               ++ssKeyPos ;
               goto retry ;
            }
            else if ( rsCmp == 0 )
            {
               rc = _matches( itrSK, itrLB, itrUB, result, TRUE, -1 ) ;
               if ( compareLU < 0 )
               {
                  goto done ;
               }
            }
         }

         if ( 0 <= compareLU )
         {
            rsCmp = rtnKeyCompare( upBound, pStartKey->_bound ) ;
            rsCmp *= director ;
            if ( rsCmp < 0 || ( rsCmp == 0 && !pStartKey->_inclusive ) )
            {
               ++ssKeyPos ;
               goto retry ;
            }
            else if ( rsCmp == 0 )
            {
               rc = _matches( itrSK, itrLB, itrUB, result, isCloseInterval, 1 ) ;
               goto done ;
            }
         }
      }

      result = TRUE ;

   done:
      return rc ;
   }

   INT32 clsCatalogPredicateTree::matches( _clsCatalogItem * pCatalogItem,
                                           BOOLEAN & result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN rsTmp = TRUE ;

      if ( isUniverse() )
      {
         goto done ;
      }

      try
      {
         BSONObjIterator itrSK( _shardingKey ) ;
         BSONObjIterator itrLB( pCatalogItem->getLowBound() ) ;
         BSONObjIterator itrUB( pCatalogItem->getUpBound() ) ;

         rc = _matches( itrSK, itrLB, itrUB, rsTmp, pCatalogItem->isLast(), 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to match catalog item, rc: %d",
                      rc ) ;

         if ( ( rsTmp && CLS_CATA_LOGIC_OR == _logicType ) ||
              ( !rsTmp && CLS_CATA_LOGIC_AND == _logicType ) )
         {
            goto done ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception on match catalog item: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < _children.size(); i++ )
      {
         rc = _children[i]->matches( pCatalogItem, rsTmp ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to match the catalog item(rc=%d)",
                      rc );
         if ( ( !rsTmp && CLS_CATA_LOGIC_AND == _logicType ) ||
              ( rsTmp && CLS_CATA_LOGIC_OR == _logicType ) )
         {
            break ;
         }
      }
   done:
      result = rsTmp ;
      return rc ;
   error:
      goto done ;
   }

/*
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_MATCHES, "clsCatalogPredicateTree::matches" )
   INT32 clsCatalogPredicateTree::matches( _clsCatalogItem * pCatalogItem,
                                           BOOLEAN & result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN rsTmp = TRUE ;

      const map<string, rtnPredicate> &predicates = _predicateSet.predicates() ;
      if ( isUniverse() )
      {
         goto done ;
      }
      if ( predicates.size() > 0 )
      {
         try
         {
            BSONObjIterator iterSK( _shardingKey );
            BSONObjIterator iterLB( pCatalogItem->getLowBound() );
            BSONObjIterator iterUB( pCatalogItem->getUpBound() );
            while ( iterSK.more() )
            {
               BSONElement beShardingKey = iterSK.next() ;
               SDB_ASSERT ( beShardingKey.isNumber(),
                            "Invalid sharding-key!" ) ;
               map<string, rtnPredicate>::const_iterator iterMap =
                                 predicates.find( beShardingKey.fieldName() );
               if ( predicates.end() == iterMap )
               {
                  rsTmp = TRUE ;
                  goto check_children ;
               }

               if ( iterMap->second._startStopKeys.size() != 1 )
               {
                  rsTmp = TRUE ;
                  goto check_children ;
               }

               rtnStartStopKey matcherBound = iterMap->second._startStopKeys[0];
               BSONElement lowBound ;
               BSONElement upBound ;
               INT32 rsCmp = 0 ;

               if ( beShardingKey.numberInt() >= 0 )
               {
                  if ( iterLB.more() )
                  {
                     lowBound = iterLB.next();
                     rsCmp = rtnKeyCompare( lowBound,
                                            matcherBound._stopKey._bound );
                     if ( rsCmp > 0 )
                     {
                        rsTmp = FALSE;
                        goto check_children;
                     }
                     else if ( 0 == rsCmp )
                     {
                        if ( !matcherBound._stopKey._inclusive )
                        {
                           rsTmp = FALSE;
                           goto check_children;
                        }
                        if ( !iterSK.more() )
                        {
                           rsTmp = TRUE;
                           goto check_children;
                        }
                        if ( iterUB.more() )
                        {
                           upBound = iterUB.next();
                        }
                        continue;
                     }
                  }
                  if ( iterUB.more() )
                  {
                     upBound = iterUB.next();
                     rsCmp = rtnKeyCompare( upBound,
                                            matcherBound._startKey._bound );
                     if ( rsCmp < 0 )
                     {
                        rsTmp = FALSE;
                        goto check_children;
                     }
                     else if ( 0 == rsCmp )
                     {
                        if ( !matcherBound._startKey._inclusive ||
                             !iterSK.more() )
                        {
                           rsTmp = FALSE;
                           goto check_children;
                        }
                        continue;
                     }
                  }
                  rsTmp = TRUE;
                  goto check_children;
               }
               else // lowBound > upBound
               {
                  if ( iterLB.more() )
                  {
                     upBound = iterLB.next();
                     rsCmp = rtnKeyCompare( upBound,
                                            matcherBound._startKey._bound );
                     if ( rsCmp < 0 || ( 0 == rsCmp &&
                          !matcherBound._startKey._inclusive ) )
                     {
                        rsTmp = FALSE;
                        goto check_children;
                     }
                     else if ( 0 == rsCmp )
                     {
                        if ( !matcherBound._startKey._inclusive )
                        {
                           rsTmp = FALSE;
                           goto check_children;
                        }
                        if ( !iterSK.more() )
                        {
                           rsTmp = TRUE;
                           goto check_children;
                        }
                        if ( iterUB.more() )
                        {
                           lowBound = iterUB.next();
                        }
                        continue;
                     }
                  }
                  if ( iterUB.more() )
                  {
                     lowBound = iterUB.next();
                     rsCmp = rtnKeyCompare( lowBound,
                                            matcherBound._stopKey._bound );
                     if ( rsCmp > 0 )
                     {
                        rsTmp = FALSE;
                        goto check_children;
                     }
                     else if ( 0 == rsCmp )
                     {
                        if ( !matcherBound._stopKey._inclusive ||
                             !iterSK.more() )
                        {
                           rsTmp = FALSE;
                           goto check_children;
                        }
                        continue;
                     }
                  }
                  rsTmp = TRUE;
                  goto check_children;
               }
            }
            if ( !iterSK.more() )
            {
               rsTmp = FALSE;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR, "occured unexpected error:%s", e.what() );
            goto error ;
         }
      }

   check_children:
      if ( _children.size() > 0 && ( predicates.size() == 0 ||
           TRUE == rsTmp || CLS_CATA_LOGIC_OR == _logicType ) )
      {
         UINT32 i = 0;
         for ( ; i < _children.size(); i++ )
         {
            rc = _children[i]->matches( pCatalogItem, rsTmp );
            PD_RC_CHECK( rc, PDERROR, "Failed to match the shardingKey(rc=%d)",
                         rc );
            if ( ( !rsTmp && CLS_CATA_LOGIC_AND == _logicType ) ||
                 ( rsTmp && CLS_CATA_LOGIC_OR == _logicType ) )
            {
               break;
            }
         }
      }

   done:
      result = rsTmp;
      return rc;
   error:
      goto done;
   }
*/

}

