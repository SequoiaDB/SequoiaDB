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

   Source File Name = optQgmSpecStrategy.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "optQgmSpecStrategy.hpp"
#include "qgmOprUnit.hpp"
#include "qgmOptiAggregation.hpp"
#include "qgmOptiNLJoin.hpp"
#include "qgmOptiSort.hpp"
#include "qgmOptiSelect.hpp"
#include "qgmUtil.hpp"

namespace engine
{


   static BOOLEAN isCondNotInAggrFunc( qgmConditionNode *condNode,
                                       qgmOptiAggregation *aggrNode )
   {
      qgmConditionNodeHelper condTree( condNode ) ;
      qgmDbAttrPtrVec attrs ;
      condTree.getAllAttr( attrs ) ;
      qgmDbAttrPtrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         if ( aggrNode->isInAggrFieldAlias( *(*it) ) )
         {
            return FALSE ;
         }
         ++it ;
      }
      return TRUE ;
   }

   static INT32 getCondAttrMoreFields( qgmConditionNode *condNode,
                                       const qgmOPFieldVec &fields,
                                       qgmOPFieldVec &more )
   {
      if ( 0 == fields.size() )
      {
         return SDB_OK ;
      }

      qgmConditionNodeHelper condTree( condNode ) ;
      qgmDbAttrPtrVec attrs ;
      condTree.getAllAttr( attrs ) ;
      qgmDbAttrPtrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         if ( isFromOne( *(*it), more, FALSE ) )
         {
            ++it ;
            continue ;
         }

         if ( !isFromOne( *(*it), fields, TRUE ) )
         {
            more.push_back( qgmOpField( *(*it), SQL_GRAMMAR::DBATTR ) ) ;
         }
         ++it ;
      }
      return SDB_OK ;
   }


   static INT32 findSameSortField( const qgmOpField &field,
                                   const qgmOPFieldVec &fieldVec )
   {
      UINT32 endPos = fieldVec.size() ;
      UINT32 index = 0 ;
      while ( index < endPos )
      {
         if ( field.value.attr() == fieldVec[index].value.attr() )
         {
            return index ;
         }
         ++index ;
      }
      return -1 ;
   }

   INT32 _optQgmSortAggrSty::calcResult( qgmOprUnit * oprUnit,
                                         qgmOptiTreeNode * curNode,
                                         qgmOptiTreeNode * subNode,
                                         OPT_QGM_SS_RESULT & result )
   {

      INT32 rc = SDB_OK ;
      qgmOPFieldVec outSorts ;
      INT32 getSortRC = subNode->outputSort( outSorts ) ;
      qgmOPFieldVec *sortFields = oprUnit->getFields() ;
      UINT32 count = sortFields->size() ;

      if ( isWildCard( outSorts ) )
      {
         result = OPT_SS_PROCESSED ;
         goto done ;
      }
      else if ( SDB_OK != getSortRC || count > outSorts.size() )
      {
         result = OPT_SS_REFUSE ;
         goto done ;
      }
      else
      {
         UINT32 index = 0 ;
         INT32  findPos = -1 ;
         BOOLEAN otherAllSame = TRUE ;
         BOOLEAN nameAllSame = TRUE ;

         while ( index < count )
         {
            findPos = findSameSortField( (*sortFields)[index], outSorts ) ;
            if ( -1 == findPos )
            {
               nameAllSame = FALSE ;
               otherAllSame = FALSE ;
               break ;
            }

            if ( otherAllSame && (UINT32)findPos != index )
            {
               otherAllSame = FALSE ;
            }
            else if ( otherAllSame &&
                      (*sortFields)[index].type != outSorts[findPos].type )
            {
               otherAllSame = FALSE ;
            }
            ++index ;
         }

         if ( !nameAllSame )
         {
            result = OPT_SS_REFUSE ;
            goto done ;
         }
         else if ( otherAllSame )
         {
            result = OPT_SS_PROCESSED ;
            goto done ;
         }

         result = OPT_SS_ACCEPT ;
      }

   done:
      return rc ;
   }

   const CHAR* _optQgmSortAggrSty::strategyName() const
   {
      return "SortAggr-Strategy" ;
   }

   INT32 _optQgmSortJoinSty::calcResult( qgmOprUnit * oprUnit,
                                         qgmOptiTreeNode * curNode,
                                         qgmOptiTreeNode * subNode,
                                         OPT_QGM_SS_RESULT & result )
   {
      INT32 rc = SDB_OK ;
      qgmOptiNLJoin *joinNode = ( qgmOptiNLJoin* )subNode ;
      qgmOPFieldVec *sortFields = oprUnit->getFields() ;
      qgmOptiTreeNode *pNode = joinNode->outer() ;

      if ( SQL_GRAMMAR::INNERJOIN == joinNode->joinType() ||
           SQL_GRAMMAR::L_OUTERJOIN == joinNode->joinType() )
      {
         BOOLEAN allFind = TRUE ;
         BOOLEAN hasSwap = FALSE ;

         while ( pNode )
         {
            const qgmField &alias = pNode->getAlias( TRUE ) ;
            qgmOPFieldVec::iterator it = sortFields->begin() ;
            while ( it != sortFields->end() )
            {
               if ( (*it).value.relegation() != alias )
               {
                  allFind = FALSE ;
                  break ;
               }
               ++it ;
            }

            if ( allFind )
            {
               if ( hasSwap )
               {
                  rc = joinNode->swapInnerOuter() ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Swap node[%s] inner-outer failed, rc: %d",
                             joinNode->toString().c_str(), rc ) ;
                     goto error ;
                  }
               }
               result = OPT_SS_TAKEOVER ;
               goto done ;
            }

            if ( SQL_GRAMMAR::INNERJOIN == joinNode->joinType() &&
                 joinNode->canSwapInnerOuter() && !hasSwap )
            {
               allFind = TRUE ;
               hasSwap = TRUE ;
               pNode = joinNode->inner() ;
            }
            else
            {
               pNode = NULL ;
            }
         }
      }

      result = OPT_SS_REFUSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _optQgmSortJoinSty::strategyName() const
   {
      return "SortJoin-Strategy" ;
   }

   INT32 _optQgmSortFilterSty::calcResult( qgmOprUnit * oprUnit,
                                           qgmOptiTreeNode * curNode,
                                           qgmOptiTreeNode * subNode,
                                           OPT_QGM_SS_RESULT & result )
   {
      qgmOptiSelect *filter = (qgmOptiSelect*)subNode ;

      if ( filter->hasConstraint() )
      {
         qgmOPFieldVec outSort ;
         INT32 getSortRC = filter->outputSort( outSort ) ;
         qgmOPFieldVec *fields = oprUnit->getFields() ;
         UINT32 count = fields->size() ;

         if ( isWildCard( outSort ) )
         {
            result = OPT_SS_PROCESSED ;
         }
         else if ( SDB_OK != getSortRC || count > outSort.size() )
         {
            result = OPT_SS_REFUSE ;
         }
         else
         {
            UINT32 index = 0 ;
            BOOLEAN allSame = TRUE ;
            while ( index < count )
            {
               if ( (*fields)[index].value.attr() != outSort[index].value.attr()
                    || (*fields)[index].type != outSort[index].type )
               {
                  allSame = FALSE ;
                  break ;
               }
               ++index ;
            }

            result = allSame ? OPT_SS_PROCESSED : OPT_SS_REFUSE ;
         }
      }
      else
      {
         result = OPT_SS_TAKEOVER ;
      }

      return SDB_OK ;
   }

   const CHAR* _optQgmSortFilterSty::strategyName() const
   {
      return "SortFilter-Strategy" ;
   }

   INT32 _optQgmFilterSortSty::calcResult( qgmOprUnit * oprUnit,
                                           qgmOptiTreeNode * curNode,
                                           qgmOptiTreeNode * subNode,
                                           OPT_QGM_SS_RESULT & result )
   {
      INT32 rc = SDB_OK ;
      qgmFilterUnit *filterUnit = ( qgmFilterUnit* )oprUnit ;
      qgmFilterUnit *newUnit = NULL ;

      qgmOPFieldVec *fields = oprUnit->getFields() ;
      qgmOptiSort *sortNode = ( qgmOptiSort* )subNode ;
      qgmOPFieldVec *orderby = sortNode->getOrderby() ;
      qgmOPFieldVec::iterator itSort ;
      qgmOPFieldVec sortMore ;

      if ( fields->size() > 0 )
      {
         itSort = orderby->begin() ;
         while ( itSort != orderby->end() )
         {
            if ( !isFromOne( *itSort, *fields, TRUE ) )
            {
               sortMore.push_back( *itSort ) ;
            }
            ++itSort ;
         }
      }

      if ( 0 == sortMore.size() )
      {
         result = OPT_SS_TAKEOVER ;
         goto done ;
      }
      else
      {
         if ( !oprUnit->isOptional() && curNode->getParent() )
         {
            qgmOpStream outputStream ;
            qgmOptiTreeNode *rootNode = curNode->getParent() ;
            while ( rootNode->getParent() )
            {
               rootNode = rootNode->getParent() ;
            }
            rootNode->outputStream( outputStream ) ;
            if ( outputStream.stream.size() != 0 && !outputStream.isWildCard() )
            {
               result = OPT_SS_TAKEOVER ;
               goto done ;
            }
         }

         newUnit = SDB_OSS_NEW qgmFilterUnit( *filterUnit,
          filterUnit->isFieldsOptional() ? FALSE : QGM_OPR_FILTER_COPY_FLAG ) ;
         if ( !newUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         newUnit->setNodeID( curNode->getNodeID() ) ;

         if ( filterUnit->hasCondition() )
         {
            qgmConditionNodePtrVec subConds = filterUnit->getConditions() ;
            newUnit->addCondition( subConds ) ;
            filterUnit->emptyCondition() ;
         }

         PD_LOG( PDDEBUG, "Create a optional unit[%s], curNode[%s], subNode[%s]",
                 newUnit->toString().c_str(), curNode->toString().c_str(),
                 subNode->toString().c_str() ) ;

         rc = subNode->pushOprUnit( newUnit, curNode, qgmOptiTreeNode::FROM_UP ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Push oprUnit[%s] to node[%s] failed, rc: %d",
                    newUnit->toString().c_str(), subNode->toString().c_str(),
                    rc ) ;
            goto error ;
         }
      }

      result = OPT_SS_REFUSE ;

   done:
      return rc ;
   error:
      if ( newUnit )
      {
         SDB_OSS_DEL newUnit ;
      }
      goto done ;
   }

   const CHAR* _optQgmFilterSortSty::strategyName() const
   {
      return "FilterSort-Strategy" ;
   }

   INT32 _optQgmFilterFilterSty::calcResult( qgmOprUnit * oprUnit,
                                             qgmOptiTreeNode * curNode,
                                             qgmOptiTreeNode * subNode,
                                             OPT_QGM_SS_RESULT & result )
   {
      qgmOptiSelect *filter = (qgmOptiSelect*)subNode ;
      qgmFilterUnit *filterUnit = (qgmFilterUnit*)oprUnit ;
      qgmFilterUnit *newCopy = NULL ;
      INT32 rc = SDB_OK ;

      if ( filter->hasExpr() ||
           oprUnit->hasExpr() )
      {
         result = OPT_SS_REFUSE ;
         goto done ;
      }

      if ( filter->hasConstraint() && FILTER_SEC != filterUnit->filterType() )
      {
         result = OPT_SS_REFUSE ;

         qgmOPFieldVec *fields = filterUnit->getFields() ;

         if ( fields->size() > 0 && !oprUnit->isWildCardField() )
         {
            qgmDbAttrPtrVec attrs ;
            qgmDbAttrPtrVec::iterator itAttr ;

            newCopy = SDB_OSS_NEW qgmFilterUnit( *filterUnit,
             filterUnit->isFieldsOptional() ? FALSE : QGM_OPR_FILTER_COPY_FLAG ) ;
            if ( !newCopy )
            {
               rc = SDB_OOM ;
               goto error ;
            }

            filterUnit->getCondFields( attrs ) ;
            itAttr = attrs.begin() ;
            while ( itAttr != attrs.end() )
            {
               newCopy->addOpField( qgmOpField( *(*itAttr), SQL_GRAMMAR::DBATTR ),
                                    TRUE ) ;
               ++itAttr ;
            }

            rc = subNode->pushOprUnit( newCopy, curNode,
                                       qgmOptiTreeNode::FROM_UP ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "push oprUnit[%s] to subNode[%s] from node[%s] "
                       "failed, rc: %d", newCopy->toString().c_str(),
                       subNode->toString().c_str(), rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         result = OPT_SS_ACCEPT ;
      }

   done:
      return rc ;
   error:
      if ( newCopy )
      {
         SDB_OSS_DEL newCopy ;
      }
      goto done ;
   }

   const CHAR* _optQgmFilterFilterSty::strategyName() const
   {
      return "FilterFilter-Strategy" ;
   }

   INT32 _optQgmFilterAggrSty::calcResult( qgmOprUnit * oprUnit,
                                           qgmOptiTreeNode * curNode,
                                           qgmOptiTreeNode * subNode,
                                           OPT_QGM_SS_RESULT & result )
   {
      INT32 rc = SDB_OK ;
      qgmFilterUnit *filterUnit = ( qgmFilterUnit* )oprUnit ;
      qgmFilterUnit *newUnit = NULL ;
      qgmOptiAggregation *aggrNode = ( qgmOptiAggregation* )subNode ;
      qgmOPFieldVec* fields = filterUnit->getFields() ;

      BOOLEAN fieldNotInAggrFunc = TRUE ;
      BOOLEAN attrNotInAggrFunc = TRUE ;
      qgmOPFieldVec moreField ;
      BOOLEAN change = FALSE ;
      qgmConditionNodePtrVec pushConds ;
      BOOLEAN delDup = QGM_OPR_FILTER_COPY_FLAG ;

      qgmOPFieldVec::iterator it = fields->begin() ;

      if ( oprUnit->hasExpr() ||
           aggrNode->hasExpr() )
      {
         result = OPT_SS_REFUSE ;
         goto done ;
      }

      while ( it != fields->end() )
      {
         qgmOpField &field = *it ;
         if ( field.type == SQL_GRAMMAR::WILDCARD ||
              aggrNode->isInAggrFieldAlias( field.value ) )
         {
            fieldNotInAggrFunc = FALSE ;
            break ;
         }
         ++it ;
      }

      if ( filterUnit->hasCondition() )
      {
         qgmConditionNodePtrVec subConds = filterUnit->getConditions() ;
         qgmConditionNodePtrVec::iterator itSub = subConds.begin() ;
         while ( itSub != subConds.end() )
         {
            if ( isCondNotInAggrFunc( *itSub, aggrNode ) )
            {
               pushConds.push_back( *itSub ) ;
               filterUnit->removeCondition( *itSub ) ;
            }
            else
            {
               getCondAttrMoreFields( *itSub, *fields, moreField ) ;
               attrNotInAggrFunc = FALSE ;
            }

            ++itSub ;
         }
      }

      if ( fieldNotInAggrFunc && attrNotInAggrFunc )
      {
         filterUnit->addCondition( pushConds ) ;
         result = OPT_SS_TAKEOVER ;
         goto done ;
      }
      else if ( attrNotInAggrFunc || FILTER_SEC == filterUnit->filterType() )
      {
         filterUnit->addCondition( pushConds ) ;
         result = OPT_SS_TAKEOVER ;
         goto done ;
      }
      else if ( filterUnit->isWildCardField() && 0 == pushConds.size() )
      {
         result = OPT_SS_REFUSE ;
         goto done ;
      }

      if ( 0 == moreField.size() ||
           filterUnit->isFieldsOptional() )
      {
         delDup = FALSE ;
      }

      newUnit = SDB_OSS_NEW qgmFilterUnit( *filterUnit, delDup ) ;
      if ( !newUnit )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      newUnit->setNodeID( curNode->getNodeID() ) ;

      if ( moreField.size() == 0 )
      {
         fields->clear() ;
         change = TRUE ;
      }
      else
      {
         newUnit->addOpField( moreField, FALSE ) ;
      }

      newUnit->addCondition( pushConds ) ;

      PD_LOG( PDDEBUG, "Create a optional unit[%s], curNode[%s], subNode[%s]",
              newUnit->toString().c_str(), curNode->toString().c_str(),
              subNode->toString().c_str() ) ;

      rc = subNode->pushOprUnit( newUnit, curNode, qgmOptiTreeNode::FROM_UP ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Push oprUnit[%s] to node[%s] failed, rc: %d",
                 newUnit->toString().c_str(), subNode->toString().c_str(),
                 rc ) ;
         goto error ;
      }

      if ( change )
      {
         curNode->updateChange( filterUnit ) ;
      }

      result = OPT_SS_REFUSE ;

   done:
      return rc ;
   error:
      if ( newUnit )
      {
         SDB_OSS_DEL newUnit ;
      }
      goto done ;
   }

   const CHAR* _optQgmFilterAggrSty::strategyName() const
   {
      return "FilterAggr-Strategy" ;
   }

   INT32 _optQgmFilterJoinSty::calcResult( qgmOprUnit * oprUnit,
                                           qgmOptiTreeNode * curNode,
                                           qgmOptiTreeNode * subNode,
                                           OPT_QGM_SS_RESULT & result )
   {
      INT32 rc = SDB_OK ;
      qgmFilterUnit *newUnit = NULL ;
      qgmFilterUnit *filterUnit = (qgmFilterUnit*)oprUnit ;

      BOOLEAN       fieldReleValid = TRUE ;
      BOOLEAN       condReleSame = TRUE ;

      qgmConditionNodePtrVec pushConds ;
      qgmOPFieldVec moreField ;

      if ( oprUnit->isWildCardField() )
      {
         fieldReleValid = FALSE ;
      }
      else
      {
         qgmOPFieldVec *fields = oprUnit->getFields() ;
         qgmOPFieldVec::iterator itField = fields->begin() ;
         while ( itField != fields->end() )
         {
            if ( (*itField).value.relegation().empty() )
            {
               fieldReleValid = FALSE ;
               break ;
            }
            ++itField ;
         }
      }

      qgmConditionNodePtrVec subConds = filterUnit->getConditions() ;
      qgmConditionNodePtrVec::iterator itSub = subConds.begin() ;
      while ( itSub != subConds.end() )
      {
         if ( isCondSameRele( *itSub, FALSE ) )
         {
            pushConds.push_back( *itSub ) ;
            filterUnit->removeCondition( *itSub ) ;
         }
         else
         {
            condReleSame = FALSE ;
            getCondAttrMoreFields( *itSub, *(filterUnit->getFields()),
                                   moreField ) ;
         }
         ++itSub ;
      }

      if ( FILTER_CON == filterUnit->filterType() && condReleSame )
      {
         filterUnit->addCondition( pushConds ) ;
         result = OPT_SS_TAKEOVER ;
         goto done ;
      }

      if ( fieldReleValid )
      {
         qgmOPFieldVec::iterator itField = moreField.begin() ;
         while ( itField != moreField.end() )
         {
            if ( (*itField).value.relegation().empty() )
            {
               fieldReleValid = FALSE ;
               break ;
            }
            ++itField ;
         }
      }

      if ( !fieldReleValid && 0 == pushConds.size() )
      {
         result = OPT_SS_REFUSE ;
         goto done ;
      }

      newUnit = SDB_OSS_NEW qgmFilterUnit( QGM_OPTI_TYPE_FILTER ) ;
      if ( !newUnit )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      newUnit->setOptional( TRUE ) ;

      if ( fieldReleValid )
      {
         newUnit->addOpField( *oprUnit->getFields(),
          filterUnit->isFieldsOptional() ? FALSE : QGM_OPR_FILTER_COPY_FLAG ) ;
         newUnit->addOpField( moreField, FALSE ) ;
      }
      newUnit->setNodeID( curNode->getNodeID() ) ;

      if ( pushConds.size() > 0 )
      {
         newUnit->addCondition( pushConds ) ;
      }

      PD_LOG( PDDEBUG, "Create a optional unit[%s], curNode[%s], subNode[%s]",
              newUnit->toString().c_str(), curNode->toString().c_str(),
              subNode->toString().c_str() ) ;

      rc = subNode->pushOprUnit( newUnit, curNode, qgmOptiTreeNode::FROM_UP ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Push oprUnit[%s] to node[%s] failed, rc: %d",
                 newUnit->toString().c_str(), subNode->toString().c_str(),
                 rc ) ;
         goto error ;
      }

      result = OPT_SS_REFUSE ;

   done:
      return rc ;
   error:
      if ( newUnit )
      {
         SDB_OSS_DEL newUnit ;
      }
      goto done ;
   }

   const CHAR* _optQgmFilterJoinSty::strategyName() const
   {
      return "FilterJoin-Strategy" ;
   }

   INT32 _optQgmAggrFilterSty::calcResult( qgmOprUnit * oprUnit,
                                           qgmOptiTreeNode * curNode,
                                           qgmOptiTreeNode * subNode,
                                           OPT_QGM_SS_RESULT & result )
   {
      INT32 rc = SDB_OK ;
      qgmFilterUnit *filterUnit = (qgmFilterUnit*)
                           subNode->getOprUnitByType( QGM_OPTI_TYPE_FILTER ) ;

      result = OPT_SS_REFUSE ;

      if ( oprUnit->hasExpr() ||
           (( qgmOptiSelect * )subNode)->hasExpr() )
      {
         goto done ;
      }

      if ( !filterUnit || filterUnit->hasCondition() )
      {
         goto done ;
      }
      else
      {
         /*qgmOPFieldPtrVec aliases ;
         qgmOptiAggregation *aggrNode = (qgmOptiAggregation*)curNode ;

         if ( filterUnit->getFieldAlias( aliases ) > 0 )
         {
         }

         if ( subNode->validSelfAlias() )
         {
         }*/
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _optQgmAggrFilterSty::strategyName() const
   {
      return "AggrFilter-Strategy" ;
   }


   INT32  _optQgmFilterScanSty::calcResult( qgmOprUnit *oprUnit,
                                            qgmOptiTreeNode *curNode,
                                            qgmOptiTreeNode *subNode,
                                            OPT_QGM_SS_RESULT &result )
   {
      INT32 rc = SDB_OK ;
      qgmOptiSelect *filter = ( qgmOptiSelect * )subNode ;
      result = ( oprUnit->hasExpr() || filter->hasExpr() ) ?
                 OPT_SS_REFUSE : OPT_SS_ACCEPT ;

      return rc ;
   }

   const CHAR* _optQgmFilterScanSty::strategyName() const
   {
      return "FilterScan-Strategy" ;
   }

   BOOLEAN isCondSameRele( qgmConditionNode * condNode, BOOLEAN allowEmpty )
   {
      if ( condNode->type != SQL_GRAMMAR::AND &&
           condNode->type != SQL_GRAMMAR::OR &&
           condNode->type != SQL_GRAMMAR::NOT )
      {
         if ( !allowEmpty && condNode->left->type == SQL_GRAMMAR::DBATTR &&
              condNode->left->value.relegation().empty() )
         {
            return FALSE ;
         }

         if ( condNode->right->type == SQL_GRAMMAR::DBATTR &&
              condNode->left->type == SQL_GRAMMAR::DBATTR &&
              condNode->right->value.relegation() !=
              condNode->left->value.relegation() )
         {
            return FALSE ;
         }
      }
      else
      {
         vector < qgmDbAttr * > condFields ;
         qgmConditionNodeHelper condTree( condNode ) ;
         condTree.getAllAttr( condFields ) ;
         vector < qgmDbAttr * >::iterator itCond = condFields.begin() ;
         while ( itCond != condFields.end() )
         {
            qgmDbAttr &attr = *(*itCond) ;
            if ( !allowEmpty && attr.relegation().empty() )
            {
               return FALSE ;
            }

            if ( attr.relegation() != condFields[0]->relegation() )
            {
               return FALSE ;
            }
            ++itCond ;
         }
      }
      return TRUE ;
   }

   BOOLEAN isCondIncludedNull( qgmConditionNode * condNode )
   {
      if ( !condNode )
      {
         return FALSE ;
      }
      else if ( SQL_GRAMMAR::AND == condNode->type ||
                SQL_GRAMMAR::OR == condNode->type )
      {
         if ( isCondIncludedNull( condNode->left ) )
         {
            return TRUE ;
         }
         return isCondIncludedNull( condNode->right ) ;
      }
      else if ( SQL_GRAMMAR::NOT == condNode->type )
      {
         return isCondIncludedNull( condNode->left ) ;
      }
      else if ( condNode->right &&
                SQL_GRAMMAR::NULLL == condNode->right->type )
      {
         return TRUE ;
      }
      return FALSE ;
   }

}


