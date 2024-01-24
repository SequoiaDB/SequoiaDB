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

   Source File Name = mthMatchLogicNode.cpp

   Descriptive Name = Method MatchLogicNode

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for matcher, which
   indicates whether a record matches a given matching rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2016  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthMatchLogicNode.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   _mthMatchLogicNode::_mthMatchLogicNode( _mthNodeAllocator *allocator,
                                           const mthNodeConfig *config )
                      :_mthMatchNode( allocator, config )
   {
   }

   _mthMatchLogicNode::~_mthMatchLogicNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicNode::init( const CHAR *fieldName,
                                   const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      rc = _mthMatchNode::init( fieldName, element ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "setFieldName failed:fieldName=%s,rc=%d",
                 fieldName, rc ) ;
         goto error ;
      }

      rc = _init( fieldName, element ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_init failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   INT32 _mthMatchLogicNode::_init( const CHAR *fieldName,
                                    const BSONElement &element )
   {
      return SDB_OK ;
   }

   void _mthMatchLogicNode::clear()
   {
      _clear() ;
      _mthMatchNode::clear() ;
   }

   void _mthMatchLogicNode::_clear()
   {
      return ;
   }

   void _mthMatchLogicNode::setWeight( UINT32 weight )
   {
      _weight = weight ;
   }

   UINT32 _mthMatchLogicNode::getWeight()
   {
      return _weight ;
   }

   BOOLEAN _mthMatchLogicNode::isTotalConverted()
   {
      return FALSE ;
   }

   BSONObj _mthMatchLogicNode::toBson()
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( getOperatorStr() ) ) ;
      MATCHNODE_VECTOR::iterator iter = _children.begin() ;
      while ( iter != _children.end() )
      {
         sub << ( *iter )->toBson() ;
         iter++ ;
      }

      sub.doneFast() ;

      return builder.obj() ;
   }

   BSONObj _mthMatchLogicNode::toParamBson ( const rtnParamList &toParamBson )
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( getOperatorStr() ) ) ;
      MATCHNODE_VECTOR::iterator iter = _children.begin() ;
      while ( iter != _children.end() )
      {
         sub << ( *iter )->toParamBson( toParamBson ) ;
         iter++ ;
      }

      sub.doneFast() ;

      return builder.obj() ;
   }

   INT32 _mthMatchLogicNode::getName ( IXM_FIELD_NAME_SET& nameSet )
   {
      INT32 rc = SDB_OK ;
      try
      {
         // logic node have children node named a b and c, all of a b c should indexCover
         _mthMatchNodeIterator iter( this ) ;
         while ( iter.more() )
         {
            _mthMatchNode *node = iter.next() ;
            node->getName( nameSet ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed node to get nameSet, exception: %s, rc=%d",
                      e.what(), rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   //*******************_mthMatchLogicAndNode***********************
   _mthMatchLogicAndNode::_mthMatchLogicAndNode( _mthNodeAllocator *allocator,
                                                 const mthNodeConfig *config )
                         :_mthMatchLogicNode( allocator, config )
   {
   }

   _mthMatchLogicAndNode::~_mthMatchLogicAndNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicAndNode::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_LOGIC_AND ;
   }

   const CHAR* _mthMatchLogicAndNode::getOperatorStr()
   {
      return MTH_OPERATOR_STR_AND ;
   }

   INT32 _mthMatchLogicAndNode::execute( const BSONObj &obj,
                                         _mthMatchTreeContext &context,
                                         BOOLEAN &result )
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN tmpResult = TRUE ;

      result = TRUE ;
      _mthMatchNodeIterator iter( this ) ;
      while ( iter.more() )
      {
         _mthMatchNode *node = iter.next() ;
         rc = node->execute( obj, context, tmpResult ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "execute node failed:obj=%s,node=%s,rc=%d",
                    obj.toString().c_str(), node->toString().c_str(), rc ) ;
            goto error ;
         }

         if ( !tmpResult )
         {
            result = tmpResult ;
            if ( !context.isDollarListEnabled() )
            {
               break ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _mthMatchLogicAndNode::isTotalConverted()
   {
      //treat and as true
      return TRUE ;
   }

   void _mthMatchLogicAndNode::evalEstimation ( const optCollectionStat *pCollectionStat,
                                                double &selectivity,
                                                UINT32 &cpuCost )
   {
      selectivity = 1.0 ;
      cpuCost = 0 ;

      MATCHNODE_VECTOR::iterator iter = _children.begin() ;
      while ( iter != _children.end() )
      {
         double tmpSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
         UINT32 tmpCPUCost = OPT_MTH_DEFAULT_CPU_COST ;

         (*iter)->evalEstimation( pCollectionStat, tmpSelectivity, tmpCPUCost ) ;

         selectivity *= tmpSelectivity ;
         cpuCost += tmpCPUCost ;

         iter++ ;
      }

      selectivity = OPT_ROUND_SELECTIVITY( selectivity ) ;
      cpuCost += OPT_MTH_OPTR_BASE_CPU_COST ;
   }

   //*******************_mthMatchLogicAndNode***********************
   _mthMatchLogicOrNode::_mthMatchLogicOrNode( _mthNodeAllocator *allocator,
                                               const mthNodeConfig *config )
                        :_mthMatchLogicNode( allocator, config ),
                         _addedToPred( FALSE )
   {
   }

   _mthMatchLogicOrNode::~_mthMatchLogicOrNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicOrNode::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_LOGIC_OR ;
   }

   const CHAR* _mthMatchLogicOrNode::getOperatorStr()
   {
      return MTH_OPERATOR_STR_OR ;
   }

   INT32 _mthMatchLogicOrNode::execute( const BSONObj &obj,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result )
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN tmpResult = FALSE ;

      if ( getChildrenCount() == 0 )
      {
         result = TRUE ;
         goto done ;
      }

      {
         result = FALSE ;
         _mthMatchNodeIterator iter( this ) ;
         while ( iter.more() )
         {
            _mthMatchNode *node = iter.next() ;
            rc = node->execute( obj, context, tmpResult ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "execute node failed:obj=%s,node=%s,rc=%d",
                       obj.toString().c_str(), node->toString().c_str(), rc ) ;
               goto error ;
            }

            if ( tmpResult )
            {
               result = TRUE ;
               if ( !context.isDollarListEnabled() )
               {
                  break ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchLogicOrNode::calcPredicate( rtnPredicateSet &predicateSet,
                                              const rtnParamList * paramList )
   {
      INT32 rc = SDB_OK ;

      rtnPredicateSet curPredicateSet ;

      // if all children can generate a single predicate with the same field
      // we can generate predicate for $or
      // e.g. { $or : [ { a : 1 }, { a : 2 } ] } to bounds [ 1, 1 ], [ 2, 2 ]

      for ( MATCHNODE_VECTOR::iterator iter = _children.begin() ;
            iter != _children.end() ;
            ++ iter )
      {
         rtnPredicateSet subPredicateSet ;

         _mthMatchNode *child = *iter ;
         // NOTE: no parameters under $or
         rc = child->calcPredicate( subPredicateSet, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to calculate predicates in child "
                      "node [type: %d, field: %s], rc: %d",
                      child->getType(), child->getFieldName(), rc ) ;

         // check if predicates from child is empty or has multiple fields
         if ( 1 != subPredicateSet.getSize() )
         {
            goto done ;
         }

         if ( 0 == curPredicateSet.getSize() )
         {
            // first child, just add
            rc = curPredicateSet.addAndPredicates( subPredicateSet ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add first predicates by $or, "
                         "rc: %d", rc ) ;
         }
         else
         {
            // other children, add by $or merge
            rc = curPredicateSet.addOrPredicates( subPredicateSet ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add predicates by $or, "
                         "rc: %d", rc ) ;
         }

         // check if predicates after merge is empty or has multiple fields
         if ( 1 != curPredicateSet.getSize() )
         {
            goto done ;
         }
      }

      if ( 1 == curPredicateSet.getSize() )
      {
         rc = predicateSet.addAndPredicates( curPredicateSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to merge predicates, "
                      "rc: %d", rc ) ;
         _addedToPred = TRUE ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _mthMatchLogicOrNode::extraEqualityMatches( BSONObjBuilder &builder,
                                                     const rtnParamList *parameters )
   {
      // do not extraEqualityMatches in logic or.
      return SDB_OK ;
   }

   void _mthMatchLogicOrNode::evalEstimation ( const optCollectionStat *pCollectionStat,
                                               double &selectivity,
                                               UINT32 &cpuCost )
   {
      selectivity = 1.0 ;
      cpuCost = 0 ;

      MATCHNODE_VECTOR::iterator iter = _children.begin() ;
      while ( iter != _children.end() )
      {
         double tmpSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
         UINT32 tmpCPUCost = OPT_MTH_DEFAULT_CPU_COST ;

         (*iter)->evalEstimation( pCollectionStat, tmpSelectivity, tmpCPUCost ) ;

         selectivity *= ( 1.0 - tmpSelectivity ) ;
         cpuCost += tmpCPUCost ;

         iter++ ;
      }

      selectivity = 1.0 - OPT_ROUND_SELECTIVITY( selectivity ) ;
      cpuCost += OPT_MTH_OPTR_BASE_CPU_COST ;
   }

   //*******************_mthMatchLogicNotNode***************************
   _mthMatchLogicNotNode::_mthMatchLogicNotNode( _mthNodeAllocator *allocator,
                                                 const mthNodeConfig *config )
                         :_mthMatchLogicAndNode( allocator, config )
   {
   }

   _mthMatchLogicNotNode::~_mthMatchLogicNotNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicNotNode::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_LOGIC_NOT ;
   }

   const CHAR* _mthMatchLogicNotNode::getOperatorStr()
   {
      return MTH_OPERATOR_STR_NOT ;
   }

   INT32 _mthMatchLogicNotNode::execute( const BSONObj &obj,
                                         _mthMatchTreeContext &context,
                                         BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN tmpResult = FALSE ;

      if ( getChildrenCount() == 0 )
      {
         result = TRUE ;
         goto done ;
      }

      rc = _mthMatchLogicAndNode::execute( obj, context, tmpResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "execute _mthMatchLogicNotNode failed:obj=%s,rc=%d",
                 obj.toString().c_str(), rc ) ;
         goto error ;
      }

      result = !tmpResult ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchLogicNotNode::calcPredicate( rtnPredicateSet &predicateSet,
                                               const rtnParamList * paramList )
   {
      // Logic not do not have predicatekey.
      return SDB_OK ;
   }

   INT32 _mthMatchLogicNotNode::extraEqualityMatches( BSONObjBuilder &builder,
                                                      const rtnParamList *parameters )
   {
      // do not extraEqualityMatches in logic not.
      return SDB_OK ;
   }

   BOOLEAN _mthMatchLogicNotNode::isTotalConverted()
   {
      return FALSE ;
   }

   void _mthMatchLogicNotNode::evalEstimation ( const optCollectionStat *pCollectionStat,
                                                double &selectivity,
                                                UINT32 &cpuCost )
   {
      selectivity = 1.0 ;
      cpuCost = 0 ;

      MATCHNODE_VECTOR::iterator iter = _children.begin() ;
      while ( iter != _children.end() )
      {
         double tmpSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
         UINT32 tmpCPUCost = OPT_MTH_DEFAULT_CPU_COST ;

         (*iter)->evalEstimation( pCollectionStat, tmpSelectivity, tmpCPUCost ) ;

         selectivity *= ( 1.0 - tmpSelectivity ) ;
         cpuCost += tmpCPUCost ;

         iter++ ;
      }

      selectivity = OPT_ROUND_SELECTIVITY( selectivity ) ;
      cpuCost += OPT_MTH_OPTR_BASE_CPU_COST ;
   }
}


