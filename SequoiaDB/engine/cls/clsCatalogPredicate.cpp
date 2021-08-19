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
   /*
      clsCatalogPredicateTree implement
   */
   clsCatalogPredicateTree::clsCatalogPredicateTree( const BSONObj &shardingKey,
                                                     BOOLEAN isHashShard )
  :_logicType( CLS_CATA_LOGIC_INVALID ),
   _shardingKey( shardingKey ),
   _isNull( FALSE ),
   _isHashShard( isHashShard )
   {
   }

   clsCatalogPredicateTree::~clsCatalogPredicateTree()
   {
      _clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ADDCHILD, "clsCatalogPredicateTree::addChild" )
   INT32 clsCatalogPredicateTree::addChild( clsCatalogPredicateTree *pChild )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ADDCHILD ) ;
      SDB_ASSERT ( pChild, "pChild can't be NULL" ) ;

      pChild->_doneCheckNull() ;

      if ( pChild->isNull() )
      {
         if ( CLS_CATA_LOGIC_AND == _logicType )
         {
            upgradeToNull() ;
         }
         /// ignore $or
         SDB_OSS_DEL pChild ;
      }
      else if ( pChild->isUniverse() )
      {
         if ( CLS_CATA_LOGIC_AND != _logicType )
         {
            upgradeToUniverse() ;
         }
         /// ignore $and
         SDB_OSS_DEL pChild ;
      }
      else
      {
         try
         {
            _children.push_back( pChild ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSCATAPREDICATETREE_ADDCHILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void clsCatalogPredicateTree::_doneCheckNull()
   {
      if ( _predicateSet.getSize() > 0 )
      {
         const RTN_PREDICATE_MAP &mapPred = _predicateSet.predicates() ;
         RTN_PREDICATE_MAP::const_iterator cit = mapPred.begin() ;
         while( cit != mapPred.end() )
         {
            if ( cit->second.isEmpty() )
            {
               upgradeToNull() ;
               break ;
            }
            ++cit ;
         }
      }
   }

   void clsCatalogPredicateTree::_doneCheckUniverse()
   {
      const RTN_PREDICATE_MAP &mapPredicate = _predicateSet.predicates() ;

      if ( isNull() || mapPredicate.empty() )
      {
         goto done ;
      }

      if ( _isHashShard )
      {
         BSONObjIterator itr( _shardingKey ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( mapPredicate.find( e.fieldName() ) == mapPredicate.end() )
            {
               /// not found
               _predicateSet.clear() ;
               break ;
            }
         }
      }
      else
      {
         const CHAR *pName = _shardingKey.firstElementFieldName() ;
         if ( pName && pName[0] &&
              mapPredicate.find( pName ) == mapPredicate.end() )
         {
            _predicateSet.clear() ;
         }
      }

   done:
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE__CLEAR, "clsCatalogPredicateTree::_clear" )
   void clsCatalogPredicateTree::_clear()
   {
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE__CLEAR ) ;

      _clearChildren() ;

      // clear the predicateSet
      _predicateSet.clear() ;
      _isNull = FALSE ;

      PD_TRACE_EXIT ( SDB_CLSCATAPREDICATETREE__CLEAR ) ;
   }

   void clsCatalogPredicateTree::_clearChildren()
   {
      /// clear child
      VEC_CLSCATAPREDICATESET::iterator it = _children.begin() ;
      while ( it != _children.end() )
      {
         SDB_OSS_DEL ( *it ) ;
         ++it ;
      }
      _children.clear() ;
   }

   void clsCatalogPredicateTree::upgradeToUniverse()
   {
      _clear() ;
   }

   void clsCatalogPredicateTree::upgradeToNull()
   {
      _isNull = TRUE ;
      _clearChildren() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ADDPREDICATE, "clsCatalogPredicateTree::addPredicate" )
   INT32 clsCatalogPredicateTree::addPredicate( const CHAR *pFieldName,
                                                const BSONElement &beField,
                                                INT32 opType )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ADDPREDICATE ) ;

      // We don't know the setting of enableMixCmp in data groups, we need a
      // larger range to cover all cases, so use mix-compare mode
      rc = _predicateSet.addPredicate( pFieldName, beField, opType,
                                       FALSE, TRUE ) ;

      PD_TRACE_EXITRC ( SDB_CLSCATAPREDICATETREE_ADDPREDICATE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE__PUSHPREDSET, "clsCatalogPredicateTree::_pushPredset" )
   INT32 clsCatalogPredicateTree::_pushPredset( rtnPredicateSet &predset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE__PUSHPREDSET ) ;

      if ( CLS_CATA_LOGIC_AND == _logicType )
      {
         if ( isNull() )
         {
            /// When is null, do nothing
            goto done ;
         }

         try
         {
            RTN_PREDICATE_MAP &mapPred = _predicateSet.predicates() ;
            RTN_PREDICATE_MAP &mapPredPush = predset.predicates() ;
            RTN_PREDICATE_MAP::iterator itPred = mapPredPush.begin() ;

            while ( itPred != mapPredPush.end() )
            {
               rtnPredicate &pred = mapPred[ itPred->first ] ;
               pred &= itPred->second ;
               ++itPred ;

               if ( pred.isEmpty() )
               {
                  upgradeToNull() ;
                  goto done ;
               }
            }

            if ( _children.empty() )
            {
               /// Only the last level and predicate can check universe
               _doneCheckUniverse() ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         if ( !_children.empty() )
         {
            BOOLEAN isAllUniverse = TRUE ;
            /// push to child
            VEC_CLSCATAPREDICATESET::iterator it = _children.begin() ;
            while( it != _children.end() )
            {
               rc = (*it)->_pushPredset( _predicateSet ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( (*it)->isNull() )
               {
                  upgradeToNull() ;
                  goto done ;
               }
               else if ( isAllUniverse && !(*it)->isUniverse() )
               {
                  isAllUniverse = FALSE ;
               }
               ++it ;
            }
            /// clear self pred
            _predicateSet.clear() ;

            if ( isAllUniverse )
            {
               upgradeToUniverse() ;
            }
         }
      }
      else if ( CLS_CATA_LOGIC_OR == _logicType && !_children.empty() )
      {
         BOOLEAN isAllNull = TRUE ;
         /// push to child
         VEC_CLSCATAPREDICATESET::iterator it = _children.begin() ;
         while( it != _children.end() )
         {
            rc = (*it)->_pushPredset( predset ) ;
            if ( rc )
            {
               goto error ;
            }
            if ( (*it)->isUniverse() )
            {
               upgradeToUniverse() ;
               goto done ;
            }
            else if ( isAllNull && !(*it)->isNull() )
            {
               isAllNull = FALSE ;
            }
            ++it ;
         }

         if ( isAllNull )
         {
            upgradeToNull() ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_CLSCATAPREDICATETREE__PUSHPREDSET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_ISUNIVERSE, "clsCatalogPredicateTree::isUniverse" )
   BOOLEAN clsCatalogPredicateTree::isUniverse() const
   {
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_ISUNIVERSE ) ;
      BOOLEAN result = TRUE ;

      if ( _isNull )
      {
         result = FALSE ;
      }
      else if ( _children.size() != 0 )
      {
         result = FALSE ;
      }
      else if ( _predicateSet.predicates().size() != 0 )
      {
         result = FALSE ;
      }

      PD_TRACE_EXIT ( SDB_CLSCATAPREDICATETREE_ISUNIVERSE );
      return result ;
   }

   void clsCatalogPredicateTree::setLogicType( CLS_CATA_LOGIC_TYPE type )
   {
      _logicType = type ;
   }

   CLS_CATA_LOGIC_TYPE clsCatalogPredicateTree::getLogicType() const
   {
      return _logicType ;
   }

   ossPoolString clsCatalogPredicateTree::toString() const
   {
      StringBuilder buf ;
      buf << "{ " ;

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

      if ( isNull() )
      {
         buf << "Null" ;
      }
      else if ( isUniverse() )
      {
         buf << "Universe" ;
      }
      else
      {
         // predicate
         if ( _predicateSet.predicates().size() > 0 )
         {
            buf << _predicateSet.toString() ;
         }

         // sub
         for ( UINT32 i = 0 ; i < _children.size() ; ++i )
         {
            buf << _children[ i ]->toString() ;
         }
      }

      buf << " }" ;
      return buf.poolStr() ;
   }

   INT32 clsCatalogPredicateTree::done()
   {
      INT32 rc = SDB_OK ;

      _doneCheckNull() ;

      /// push predicate
      {
         rtnPredicateSet tmpPredset ;
         rc = _pushPredset( tmpPredset ) ;
         if ( rc )
         {
            goto error ;
         }
      
         if ( isNull() || isUniverse() )
         {
            goto done ;
         }
      }

      rc = _done() ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE__DONE, "clsCatalogPredicateTree::_done" )
   INT32 clsCatalogPredicateTree::_done()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE__DONE ) ;

      /// gen predicate list
      if ( _predicateSet.getSize() > 0 )
      {
         SDB_ASSERT( _children.empty(), "Children must be empty" ) ;

         UINT32 addedLevel = 0 ;
         rc = _predicateLst.initialize( _predicateSet, _shardingKey,
                                        1, addedLevel ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Initialize predicate list failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

      /// done children
      if ( !_children.empty() )
      {
         SDB_ASSERT( _predicateSet.getSize() == 0, "Predicates must be empty") ;

         VEC_CLSCATAPREDICATESET::iterator it = _children.begin() ;
         while( it != _children.end() )
         {
            rc = (*it)->_done() ;
            if ( rc )
            {
               goto error ;
            }
            ++it ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSCATAPREDICATETREE__DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE_CALC, "clsCatalogPredicateTree::calc" )
   INT32 clsCatalogPredicateTree::calc( const _clsCatalogSet *pSet,
                                        CLS_SET_CATAITEM &setItem )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE_CALC ) ;

      clsCatalogPredicateTree *pChild = NULL ;
      const _clsCatalogSet::MAP_CAT_ITEM *pCataItem = NULL ;
      pCataItem = pSet->getCataItem() ;

      if ( isNull() )
      {
         /// none
      }
      else if ( isUniverse() )
      {
         /// push all
         _clsCatalogSet::MAP_CAT_ITEM::const_iterator cit ;
         cit = pCataItem->begin() ;
         while( cit != pCataItem->end() )
         {
            setItem.insert( cit->second ) ;
            ++cit ;
         }
      }
      else if ( _predicateLst.isInitialized() )
      {
         try
         {
            VEC_INT32 vecPos( _predicateLst.size(), 0 ) ;
            BOOLEAN isEnd = FALSE ;

            while( !isEnd )
            {
               rc = _calc( pSet, vecPos, setItem ) ;
               if ( rc )
               {
                  goto error ;
               }
               isEnd = _calcNext( vecPos ) ? FALSE : TRUE ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      else
      {
         /// process children
         VEC_CLSCATAPREDICATESET::iterator it = _children.begin() ;
         BOOLEAN isFirst = TRUE ;
         while ( it != _children.end() )
         {
            pChild = *it ;

            if ( CLS_CATA_LOGIC_AND == _logicType )
            {
               if ( pChild->isNull() )
               {
                  setItem.clear() ;
                  break ;
               }
               CLS_SET_CATAITEM tmpItem ;
               rc = pChild->calc( pSet, isFirst ? setItem : tmpItem ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( !isFirst )
               {
                  _mergeAnd( setItem, tmpItem ) ;
               }
            }
            else
            {
               CLS_SET_CATAITEM tmpItem ;
               rc = pChild->calc( pSet, isFirst ? setItem : tmpItem ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( !isFirst )
               {
                  rc = _mergeOr( setItem, tmpItem ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
               }
               if ( pChild->isUniverse() )
               {
                  break ;
               }
            }

            ++it ;
            isFirst = FALSE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSCATAPREDICATETREE_CALC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSCATAPREDICATETREE__CALC, "clsCatalogPredicateTree::_calc" )
   INT32 clsCatalogPredicateTree::_calc( const _clsCatalogSet *pSet,
                                         VEC_INT32 &vecCur,
                                         CLS_SET_CATAITEM &setItem )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CLSCATAPREDICATETREE__CALC ) ;

      INT32 cmpResult = 0 ;
      BSONObj objStart ;
      BOOLEAN isEqual = FALSE ;
      const _clsCatalogItem *pItem = NULL ;
      const _clsCatalogSet::MAP_CAT_ITEM *pMapCata = NULL ;
      pMapCata = pSet->getCataItem() ;
      _clsCatalogSet::MAP_CAT_ITEM::const_iterator cit ;

      rc = _buildStartObj( vecCur, objStart, isEqual ) ;
      if ( rc )
      {
         goto error ;
      }
      else if ( pSet->isHashSharding() )
      {
         if ( !isEqual )
         {
            cit = pMapCata->begin() ;
            while( cit != pMapCata->end() )
            {
               setItem.insert( cit->second ) ;
               ++cit ;
            }
         }
         else
         {
            clsCataItemKey findKey( clsPartition( objStart,
                                                  pSet->getPartitionBit(),
                                                  pSet->getInternalV() ) ) ;
            cit = pMapCata->upper_bound( findKey ) ;
            if ( cit == pMapCata->end() )
            {
               if ( pSet->getLastItem() )
               {
                  setItem.insert( pSet->getLastItem() ) ;
               }
            }
            else
            {
               setItem.insert( cit->second ) ;
            }
         }
      }
      else
      {
         clsCataItemKey findKey( objStart.objdata(), pSet->getOrdering() ) ;
         cit = pMapCata->upper_bound( findKey ) ;
         /// When not found
         if ( cit == pMapCata->end() )
         {
            if ( pSet->getLastItem() )
            {
               setItem.insert( pSet->getLastItem() ) ;
            }
         }
         else if ( isEqual && pSet->isWholeRange() )
         {
            setItem.insert( cit->second ) ;
         }
         else
         {
            while ( cit != pMapCata->end() )
            {
               pItem = cit->second ;
               /// compare start with upBound
               cmpResult = _compareStartWithBound( vecCur,
                                                   pItem->getUpBound(),
                                                   pSet->getOrdering() ) ;
               if ( cmpResult >= 0 )
               {
                  ++cit ;
                  continue ;
               }
               /// compare stop with lowBound
               cmpResult = _compareStopWithBound( vecCur,
                                                  pItem->getLowBound(),
                                                  pSet->getOrdering() ) ;
               if ( cmpResult < 0 )
               {
                  break ;
               }

               setItem.insert( pItem ) ;
               ++cit ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CLSCATAPREDICATETREE__CALC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN clsCatalogPredicateTree::_calcNext( VEC_INT32 &vecCur ) const
   {
      BOOLEAN hasNext = TRUE ;
      const RTN_PREDICATE_LIST *pPredList = NULL ;
      pPredList = _predicateLst.getPredicateList() ;

      UINT32 size = vecCur.size() ;
      INT32 i = size - 1 ;
      for ( ; i >= 0 ; --i )
      {
         const rtnPredicate &pred = (*pPredList)[i] ;
         if ( vecCur[i] + 1 < (INT32)pred._startStopKeys.size() )
         {
            ++vecCur[i] ;
            break ;
         }
         else
         {
            vecCur[i] = 0 ;
         }
      }

      if ( i < 0 )
      {
         hasNext = FALSE ;
      }

      return hasNext ;
   }

   INT32 clsCatalogPredicateTree::_buildStartObj( VEC_INT32 &vecCur,
                                                  BSONObj &obj,
                                                  BOOLEAN &isEqual )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      const RTN_PREDICATE_LIST *pPredList = NULL ;
      pPredList = _predicateLst.getPredicateList() ;
      UINT32 size = vecCur.size() ;

      try
      {
         isEqual = TRUE ;
         for ( UINT32 i = 0 ; i < size ; ++i )
         {
            const rtnPredicate &pred = (*pPredList)[i] ;
            if ( isEqual && !pred._startStopKeys[vecCur[i]].isEquality() )
            {
               isEqual = FALSE ;
            }
            builder.appendAs( pred._startStopKeys[vecCur[i]]._startKey._bound,
                              "" ) ;
         }
         obj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void clsCatalogPredicateTree::_mergeAnd( CLS_SET_CATAITEM &setItem,
                                            const CLS_SET_CATAITEM &mergeItem )
   {
      CLS_SET_CATAITEM::const_iterator cit = mergeItem.begin() ;
      CLS_SET_CATAITEM::iterator it = setItem.begin() ;

      while ( cit != mergeItem.end() && it != setItem.end() )
      {
         if ( *cit == *it )
         {
            ++cit ;
            ++it ;
         }
         else if ( *cit < *it )
         {
            ++cit ;
         }
         else
         {
            setItem.erase( it++ ) ;
         }
      }

      /// remove the others
      while ( it != setItem.end() )
      {
         setItem.erase( it++ ) ;
      }
   }

   INT32 clsCatalogPredicateTree::_mergeOr( CLS_SET_CATAITEM &setItem,
                                            const CLS_SET_CATAITEM &mergeItem )
   {
      INT32 rc = SDB_OK ;

      CLS_SET_CATAITEM::const_iterator cit = mergeItem.begin() ;

      try
      {
         while ( cit != mergeItem.end() )
         {
            setItem.insert( *cit ) ;
            ++cit ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 clsCatalogPredicateTree::_compareStartWithBound( VEC_INT32 &vecCur,
                                                          const BSONObj &bound,
                                                          const Ordering *pOrder )
   {
      INT32 cmpResult = 0 ;
      const RTN_PREDICATE_LIST *pPredList = NULL ;
      pPredList = _predicateLst.getPredicateList() ;
      UINT32 size = vecCur.size() ;

      BSONObjIterator itr( bound ) ;

      for ( UINT32 i = 0 ; i < size ; ++i )
      {
         const rtnPredicate &pred = (*pPredList)[i] ;
         const rtnStartStopKey &startStop = pred._startStopKeys[ vecCur[i] ] ;
         BSONElement e = itr.next() ;

         cmpResult = startStop._startKey._bound.woCompare( e, false ) ;
         if ( 0 == cmpResult )
         {
            if ( !startStop._startKey._inclusive )
            {
               cmpResult = 1 ;
               break ;
            }
         }
         else
         {
            cmpResult *= pOrder->get( i ) ;
            break ;
         }
      }

      return cmpResult ;
   }

   INT32 clsCatalogPredicateTree::_compareStopWithBound( VEC_INT32 &vecCur,
                                                         const BSONObj &bound,
                                                         const Ordering *pOrder )
   {
      INT32 cmpResult = 0 ;
      const RTN_PREDICATE_LIST *pPredList = NULL ;
      pPredList = _predicateLst.getPredicateList() ;
      UINT32 size = vecCur.size() ;

      BSONObjIterator itr( bound ) ;

      for ( UINT32 i = 0 ; i < size ; ++i )
      {
         const rtnPredicate &pred = (*pPredList)[i] ;
         const rtnStartStopKey &startStop = pred._startStopKeys[ vecCur[i] ] ;
         BSONElement e = itr.next() ;

         cmpResult = startStop._stopKey._bound.woCompare( e, false ) ;
         if ( 0 == cmpResult )
         {
            if ( !startStop._stopKey._inclusive )
            {
               /// stop not include means <
               cmpResult = -1 ;
               break ;
            }
         }
         else
         {
            cmpResult *= pOrder->get( i ) ;
            break ;
         }
      }

      return cmpResult ;
   }

}

