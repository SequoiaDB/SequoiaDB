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

   Source File Name = optQgmOptimizer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "optQgmOptimizer.hpp"
#include "qgmOprUnit.hpp"
#include "qgmOptiNLJoin.hpp"

namespace engine
{

   _optQgmOptimizer::_optQgmOptimizer()
   {
   }

   _optQgmOptimizer::~_optQgmOptimizer()
   {
   }

   INT32 _optQgmOptimizer::optimize( qgmOptTree & orgTree )
   {
      return SDB_OK ;
   }

   INT32 _optQgmOptimizer::adjust( qgmOptTree & orgTree )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN adjust = TRUE ;
      DEL_NODES  mapDelNodes ;

      rc = _prepareAdjust( orgTree ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Prepare adjust failed, rc: %d", rc ) ;
         return rc ;
      }

      while ( adjust || mapDelNodes.size() > 0 )
      {
         adjust = FALSE ;
         mapDelNodes.clear() ;

         rc = _downAdjust( orgTree, adjust, mapDelNodes ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "down adjust failed, rc: %d", rc ) ;
            break ;
         }

         rc = _upAdjust( orgTree, adjust, mapDelNodes ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "up adjust failed, rc: %d", rc ) ;
            break ;
         }
      }

      if ( SDB_OK == rc )
      {
         rc = _endAdjust( orgTree ) ;
      }

      return rc ;
   }

   INT32 _optQgmOptimizer::_prepareAdjust( qgmOptTree &orgTree )
   {
      INT32 rc = SDB_OK ;
      qgmOptiTreeNode *treeNode = NULL ;

      qgmOptTree::reverse_iterator rit = orgTree.rbegin() ;
      while ( rit != orgTree.rend() )
      {
         treeNode = *rit ;

         rc = treeNode->init() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Init treeNode[%s] failed, rc: %d",
                    treeNode->toString().c_str(), rc ) ;
            goto error ;
         }
         ++rit ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_endAdjust( qgmOptTree & orgTree )
   {
      qgmOptTree::iterator it = orgTree.begin() ;
      while ( it != orgTree.end() )
      {
         it->done() ;
         if ( it->isEmpty() )
         {
            it = orgTree.erase( it ) ;
            continue ;
         }
         ++it ;
      }
      return SDB_OK ;
   }

   INT32 _optQgmOptimizer::_downAdjust( qgmOptTree &orgTree, BOOLEAN &adjust,
                                        DEL_NODES  &delNodes )
   {
      INT32 rc = SDB_OK ;
      qgmOptiTreeNode *pTreeNode = NULL ;
      qgmOprUnitPtrVec oprUnitVec ;

      qgmOptTree::iterator it = orgTree.begin() ;
      while ( it != orgTree.end() )
      {
         pTreeNode = *it ;
         pTreeNode->getOprUnits( oprUnitVec ) ;
         if ( oprUnitVec.size() > 0 )
         {
            rc = _adjustByOprUnit( &oprUnitVec, orgTree, pTreeNode, adjust ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "adjust by oprUnit failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         if ( pTreeNode->isEmpty() )
         {
            PD_CHECK( 1 == pTreeNode->getSubNodeCount(), SDB_SYS, error,
                      PDERROR, "Empty node[%s] sub node num[%d] must be 1",
                      pTreeNode->toString().c_str(),
                      pTreeNode->getSubNodeCount() ) ;

            PD_LOG( PDDEBUG, "node[%s] is empty, delete from tree[%s]",
                    pTreeNode->toString().c_str(), orgTree.treeName() ) ;

            if ( pTreeNode->validSelfAlias() )
            {
               pTreeNode->getSubNode( 0 )->setAlias( pTreeNode->getAlias() ) ;
            }

            delNodes.insert( DEL_NODES::value_type( pTreeNode->getNodeID(), 1 ) ) ;
            it = orgTree.erase( it ) ;
            continue ;
         }

         if ( QGM_OPTI_TYPE_JOIN == pTreeNode->getType() )
         {
            qgmOptiNLJoin *joinNode = (qgmOptiNLJoin*)pTreeNode ;
            if ( joinNode->needMakeCondition() )
            {
               rc = joinNode->makeCondition() ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "JoinNode[%s] make condition failed, rc: %d",
                          joinNode->toString().c_str(), rc ) ;
                  goto error ;
               }
               adjust = TRUE ;
            }
         }

         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_adjustByOprUnit( qgmOprUnitPtrVec * oprUnitVec,
                                             qgmOptTree & orgTree,
                                             qgmOptiTreeNode * curNode,
                                             BOOLEAN & adjust )
   {
      INT32 rc = SDB_OK ;
      OPT_QGM_SS_RESULT result = OPT_SS_REFUSE ;
      qgmOptiTreeNode *subNode = NULL ;
      qgmOptiTreeNode *indexNode = NULL ;
      UINT32 index = 0 ;

      qgmOprUnitPtrVec::iterator itUnit = oprUnitVec->begin() ;
      while ( itUnit != oprUnitVec->end() )
      {
         index = 0 ;
         indexNode = NULL ;
         subNode = NULL ;
         result = OPT_SS_REFUSE ;
         OPT_QGM_SS_RESULT tmpResult = OPT_SS_REFUSE ;

         while ( NULL != ( indexNode = curNode->getSubNode( index++ ) ) &&
                 result > OPT_SS_ACCEPT )
         {
            if ( !(*itUnit)->getDispatchAlias().empty() &&
                 indexNode->getAlias( TRUE ) != (*itUnit)->getDispatchAlias() )
            {
               continue ;
            }

            rc = _onStrategy( *itUnit, curNode, indexNode, tmpResult ) ;
            PD_RC_CHECK( rc, PDERROR, "onStrategy failed, rc: %d", rc ) ;

            if ( !subNode || tmpResult < result )
            {
               result = tmpResult ;
               subNode = indexNode ;
            }
         }

         rc = _processSSResult( orgTree, *itUnit, curNode, subNode,
                                result, adjust ) ;
         PD_RC_CHECK( rc, PDERROR, "process strategy result[%d] failed,rc: %d",
                      result, rc ) ;

         ++itUnit ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_onStrategy( qgmOprUnit * oprUnit,
                                        qgmOptiTreeNode * curNode,
                                        qgmOptiTreeNode * subNode,
                                        OPT_QGM_SS_RESULT & result )
   {
      INT32 rc = SDB_OK ;
      optQgmStrategyTable *pStrategyTable = getQgmStrategyTable() ;
      optQgmStrategyBase *pStrategy = pStrategyTable->getStrategy(
                                      oprUnit->getType(),
                                      subNode->getType() ) ;
      PD_CHECK( pStrategy, SDB_INVALIDARG, error, PDERROR,
                "Can't find the stragegy, oprUnit: %s, subNode: %s",
                oprUnit->toString().c_str(), subNode->toString().c_str() ) ;

      rc = pStrategy->calcResult( oprUnit, curNode, subNode, result ) ;
      PD_RC_CHECK( rc, PDERROR, "strategy[%s] calcResult[oprUnit: %s, "
                   "subNode: %s] failed, rc: %d", pStrategy->strategyName(),
                   oprUnit->toString().c_str(), subNode->toString().c_str(),
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_processSSResult( qgmOptTree &orgTree,
                                             qgmOprUnit * oprUnit,
                                             qgmOptiTreeNode * curNode,
                                             qgmOptiTreeNode * subNode,
                                             OPT_QGM_SS_RESULT result,
                                             BOOLEAN &adjust )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( subNode, "SubNode can't be NULL" ) ;

      if ( OPT_SS_PROCESSED == result )
      {
         PD_LOG( PDDEBUG, "oprUnit[%s] processed, removed. curNode[%s], subNode"
                 "[%s]", oprUnit->toString().c_str(),
                 curNode->toString().c_str(), subNode->toString().c_str() ) ;

         curNode->removeOprUnit( oprUnit, TRUE, TRUE ) ;
      }
      else if ( OPT_SS_ACCEPT == result || OPT_SS_TAKEOVER == result )
      {
         curNode->removeOprUnit( oprUnit, FALSE, TRUE ) ;

         if ( OPT_SS_ACCEPT == result )
         {
            oprUnit->resetNodeID() ;
            if ( !oprUnit->isOptional() )
            {
               adjust = TRUE ;
            }
         }
         else if ( !oprUnit->isNodeIDValid() )
         {
            oprUnit->setNodeID( curNode->getNodeID() ) ;
         }

         PD_LOG( PDDEBUG, "oprUnit[%s] process result[%s]. curNode[%s], subNode"
                 "[%s]", oprUnit->toString().c_str(),
                 result == OPT_SS_ACCEPT ? "accept" : "takeover",
                 curNode->toString().c_str(), subNode->toString().c_str() ) ;

         rc = subNode->pushOprUnit( oprUnit, curNode, qgmOptiTreeNode::FROM_UP ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Push down oprUnit[%s] to node[%s] failed, rc: %d",
                    oprUnit->toString().c_str(), subNode->toString().c_str(),
                    rc ) ;
            goto error ;
         }
      }
      else if ( oprUnit->isOptional() )
      {
         PD_LOG( PDDEBUG, "oprUnit[%s] process refused, released. curNode[%s], "
                 "subNode[%s]", oprUnit->toString().c_str(),
                 curNode->toString().c_str(), subNode->toString().c_str() ) ;

         curNode->removeOprUnit( oprUnit, TRUE, FALSE ) ;
      }
      else if ( !curNode->validateBeforeChange( oprUnit->getType() ) &&
                !oprUnit->isNodeIDValid() )
      {
         adjust = TRUE ;

         curNode->removeOprUnit( oprUnit, FALSE, TRUE ) ;

         PD_LOG( PDDEBUG, "oprUnit[%s] process refused, createNew. curNode[%s], "
                 "subNode[%s]", oprUnit->toString().c_str(),
                 curNode->toString().c_str(), subNode->toString().c_str() ) ;

         rc = _formNewNode( orgTree, oprUnit, curNode, subNode,
                            qgmOptiTreeNode::FROM_UP ) ;
         PD_RC_CHECK( rc, PDERROR, "form new node failed, rc: %d" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_upAdjust( qgmOptTree & orgTree, BOOLEAN & adjust,
                                      DEL_NODES  &delNodes )
   {
      INT32 rc = SDB_OK ;
      qgmOptiTreeNode *pTreeNode = NULL ;
      qgmOprUnitPtrVec oprUnitVec ;

      qgmOptTree::reverse_iterator rit = orgTree.rbegin() ;
      while ( rit != orgTree.rend() )
      {
         pTreeNode = *rit ;
         pTreeNode->getOprUnits( oprUnitVec ) ;

         qgmOprUnit *oprUnit = NULL ;
         qgmOprUnitPtrVec::iterator it = oprUnitVec.begin() ;
         while ( it != oprUnitVec.end() )
         {
            oprUnit = *it ;
            if ( !oprUnit->isNodeIDValid() )
            {
               ++it ;
               continue ;
            }
            rc = _upbackOprUnit( oprUnit, orgTree, pTreeNode, adjust, delNodes ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Upback oprUnit[%s] failed, rc: %d",
                       oprUnit->toString().c_str(), rc ) ;
               goto error ;
            }
            ++it ;
         }

         if ( pTreeNode->isEmpty() )
         {
            PD_CHECK( 1 == pTreeNode->getSubNodeCount(), SDB_SYS, error,
                      PDERROR, "Empty node[%s] sub node num[%d] must be 1",
                      pTreeNode->toString().c_str(),
                      pTreeNode->getSubNodeCount() ) ;

            if ( pTreeNode->validSelfAlias() )
            {
               pTreeNode->getSubNode( 0 )->setAlias( pTreeNode->getAlias() ) ;
            }

            PD_LOG( PDDEBUG, "node[%s] is empty, delete from tree[%s]",
                    pTreeNode->toString().c_str(), orgTree.treeName() ) ;

            delNodes.insert( DEL_NODES::value_type(pTreeNode->getNodeID(), 1) ) ;
            rit = orgTree.erase( rit ) ;
            continue ;
         }

         ++rit ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_upbackOprUnit( qgmOprUnit * oprUnit,
                                           qgmOptTree & orgTree,
                                           qgmOptiTreeNode * curNode,
                                           BOOLEAN & adjust,
                                           DEL_NODES  &delNodes )
   {
      INT32 rc = SDB_OK ;
      qgmOptiTreeNode *parent = curNode->getParent() ;

      if ( QGM_OPTI_TYPE_FILTER != oprUnit->getType() &&
           parent && parent->getNodeID() > oprUnit->getNodeID() &&
           QGM_OPTI_TYPE_JOIN != parent->getType() )
      {
         curNode->removeOprUnit( oprUnit, FALSE, FALSE ) ;

         PD_LOG( PDDEBUG, "oprUnit[%s] take back to node[%s] from Node[%s]",
                 oprUnit->toString().c_str(), parent->toString().c_str(),
                 curNode->toString().c_str() ) ;

         rc = parent->pushOprUnit( oprUnit, curNode, qgmOptiTreeNode::FROM_DOWN ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Push up oprUnit[%s] to [%s] failed, rc: %d",
                    oprUnit->toString().c_str(), parent->toString().c_str(),
                    rc ) ;
            goto error ;
         }
      }
      else if ( oprUnit->getType() != curNode->getType() )
      {
         curNode->removeOprUnit( oprUnit, FALSE, FALSE ) ;

         DEL_NODES::iterator itFind = delNodes.find( oprUnit->getNodeID() ) ;
         if ( itFind != delNodes.end() )
         {
            delNodes.erase( itFind ) ;
         }

         PD_LOG( PDDEBUG, "oprUnit[%s] take back createNew from Node[%s]",
                 oprUnit->toString().c_str(), curNode->toString().c_str() ) ;

         rc = _formNewNode( orgTree, oprUnit, parent, curNode,
                            qgmOptiTreeNode::FROM_DOWN ) ;
         PD_RC_CHECK( rc, PDERROR, "Form new node failed, rc: %d", rc ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "oprUnit[%s] take back self from Node[%s]",
                 oprUnit->toString().c_str(), curNode->toString().c_str() ) ;

         oprUnit->resetNodeID() ;
         curNode->updateChange( oprUnit ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _optQgmOptimizer::_formNewNode( qgmOptTree & orgTree,
                                         qgmOprUnit * oprUnit,
                                         qgmOptiTreeNode * curNode,
                                         qgmOptiTreeNode * subNode,
                                         qgmOptiTreeNode::PUSH_FROM from )
   {
      INT32 rc = SDB_OK ;
      qgmOptiTreeNode *newNode = orgTree.createNode( oprUnit->getType() );
      if ( !newNode )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      rc = orgTree.insertBetween( curNode, subNode, newNode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Insert new node[%s] to tree[%s] failed, rc: %d",
                 newNode->toString().c_str(), orgTree.treeName(), rc ) ;
         goto error ;
      }

      SDB_ASSERT( !oprUnit->isOptional(), "impossible" ) ;

      if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
      {
         qgmFilterUnit *filterUnit = (qgmFilterUnit*)oprUnit ;
         if ( filterUnit->isFieldsOptional() )
         {
            filterUnit->getFields()->clear() ;
         }

         newNode->setAlias( newNode->getSubNode(0)->getAlias(TRUE) ) ;
      }

      rc = newNode->pushOprUnit( oprUnit, subNode, from ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Push oprUnit[%s] to node[%s] failed, rc: %d",
                 oprUnit->toString().c_str(), newNode->toString().c_str(),
                 rc ) ;
         goto error ;
      }
      oprUnit->resetNodeID() ;
      newNode->updateChange( oprUnit ) ;

   done:
      return rc ;
   error:
      if ( newNode )
      {
         SDB_OSS_DEL newNode ;
      }
      goto done ;
   }


   optQgmOptimizer* getQgmOptimizer()
   {
      static optQgmOptimizer s_qgmOptimizer ;
      return &s_qgmOptimizer ;
   }

}
