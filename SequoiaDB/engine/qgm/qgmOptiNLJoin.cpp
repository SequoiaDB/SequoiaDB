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

   Source File Name = qgmOptiNLJoin.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmOptiNLJoin.hpp"
#include "pd.hpp"
#include "qgmConditionNodeHelper.hpp"
#include "qgmOprUnit.hpp"
#include "qgmUtil.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "qgmHintDef.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "qgmDef.hpp"

namespace engine
{
   _qgmOptiNLJoin::_qgmOptiNLJoin( INT32 type, _qgmPtrTable *table,
                                   _qgmParamTable *param )
   :_qgmOptiTreeNode( QGM_OPTI_TYPE_JOIN, table, param ),
    _joinType( type ),
    _condition( NULL )
   {
      _outer = NULL ;
      _inner = NULL ;
      _hasMakeVar = FALSE ;
      _hasPushSort = FALSE ;
   }

   _qgmOptiNLJoin::~_qgmOptiNLJoin()
   {
      SAFE_OSS_DELETE( _condition ) ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN__MAKECONDVAR, "_qgmOptiNLJoin::_makeCondVar" )
   INT32 _qgmOptiNLJoin::_makeCondVar( qgmConditionNode * cond )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN__MAKECONDVAR ) ;
      INT32 rc = SDB_OK ;
      BOOLEAN existed = FALSE ;
      varItem item ;

      if ( cond->type == SQL_GRAMMAR::OR )
      {
         PD_LOG_MSG( PDERROR, "Join condition not suport or" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( cond->type == SQL_GRAMMAR::NOT )
      {
         PD_LOG_MSG( PDERROR, "Join condition not suport not" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( cond->type == SQL_GRAMMAR::AND )
      {
         rc = _makeCondVar( cond->left ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _makeCondVar( cond->right ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         if ( cond->left->value.relegation() == outer()->getAlias( TRUE ) )
         {
            // swap left and right
            _qgmConditionNode *tmp = cond->left ;
            cond->left = cond->right ;
            cond->right = tmp ;
            if ( SQL_GRAMMAR::LT == cond->type )
            {
               cond->type = SQL_GRAMMAR::GT ;
            }
            else if ( SQL_GRAMMAR::GT == cond->type )
            {
               cond->type = SQL_GRAMMAR::LT ;
            }
            else if ( SQL_GRAMMAR::GTE == cond->type )
            {
               cond->type = SQL_GRAMMAR::LTE ;
            }
            else if ( SQL_GRAMMAR::LTE == cond->type )
            {
               cond->type = SQL_GRAMMAR::GTE ;
            }
            else
            {
               /// when it is eg or ne, do nothing.
            }
         }
#ifdef _DEBUG
         if ( cond->right->type != SQL_GRAMMAR::DBATTR ||
              cond->right->value.relegation() != outer()->getAlias( TRUE ) )
         {
            PD_LOG_MSG( PDERROR, "Join condition[%s] invalid",
                        qgmConditionNodeHelper(cond).toJson().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
#endif //_DEBUG

         item._fieldName = cond->right->value ;
         item._varName = cond->right->value ;
         item._varName.relegation() = _uniqueNameR ;
         rc = _param->addVar( item._varName, cond->right->var, &existed ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Add var[%s, %s] failed, rc: %d",
                    item._fieldName.toString().c_str(),
                    item._varName.toString().c_str(), rc ) ;
            goto error ;
         }
         if ( !existed )
         {
            _varList.push_back( item ) ;
         }

         if ( _hints.empty() )
         {
            cond->right->type = SQL_GRAMMAR::SQLMAX + 1 ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN__MAKECONDVAR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _qgmOptiNLJoin::canSwapInnerOuter() const
   {
      // if is not inner join, can't swap
      // if has make condition, can't swap
      // if has push sort, can't swap
      if ( SQL_GRAMMAR::INNERJOIN != _joinType ||
           _hasMakeVar || _hasPushSort )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   INT32 _qgmOptiNLJoin::swapInnerOuter()
   {
      if ( !canSwapInnerOuter() )
      {
         return SDB_SYS ;
      }

      qgmOptiTreeNode *tmp = *_outer ;
      *_outer = *_inner ;
      *_inner = tmp ;

      return SDB_OK ;
   }

   BOOLEAN _qgmOptiNLJoin::needMakeCondition() const
   {
      // if has condition, and not make
      if ( _condition && !_hasMakeVar )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN_MAKECONDITION, "_qgmOptiNLJoin::makeCondition" )
   INT32 _qgmOptiNLJoin::makeCondition()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN_MAKECONDITION ) ;
      INT32 rc = SDB_OK ;
      qgmFilterUnit *condUnit = NULL ;

      if ( !needMakeCondition() )
      {
         goto done ; ;
      }

      rc = _makeCondVar( _condition ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _createJoinUnit() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( _hints.empty() )
      {
         // create a condtion oprUnit
         condUnit = SDB_OSS_NEW qgmFilterUnit( QGM_OPTI_TYPE_FILTER ) ;
         if ( !condUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         condUnit->setCondition( _condition ) ;
         _condition = NULL ;
         condUnit->setDispatchAlias( inner()->getAlias( TRUE ) ) ;
         _oprUnits.push_back( condUnit ) ;
         condUnit = NULL ;
      }
      else
      {
         _varList.clear() ;
      }

      _hasMakeVar = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN_MAKECONDITION, rc ) ;
      return rc ;
   error:
      if ( condUnit )
      {
         SDB_OSS_DEL condUnit ;
      }
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN_INIT, "_qgmOptiNLJoin::init" )
   INT32 _qgmOptiNLJoin::init()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN_INIT ) ;
      INT32 rc = SDB_OK ;

      _table->getUniqueTableAlias( _uniqueNameR ) ;
      if ( !_hints.empty() )
      {
         _table->getUniqueTableAlias( _uniqueNameL) ;
      }

      rc = _makeOuterInner() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // if not inner join, can make condition in init function
      if ( _condition && SQL_GRAMMAR::INNERJOIN != _joinType )
      {
         rc = makeCondition() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN__CRTJOINUNIT, "_qgmOptiNLJoin::_createJoinUnit" )
   INT32 _qgmOptiNLJoin::_createJoinUnit()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN__CRTJOINUNIT ) ;
      INT32 rc = SDB_OK ;
      qgmOprUnit *joinUnit = NULL ;
      QGM_VARLIST::iterator it ;

      if ( _hints.empty() )
      {
         if ( 0 == _varList.size() )
         {
            goto done ;
         }

         // create join unit
         joinUnit = SDB_OSS_NEW qgmOprUnit( QGM_OPTI_TYPE_JOIN ) ;
         if ( !joinUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         it = _varList.begin() ;
         while ( it != _varList.end() )
         {
            qgmDbAttr &fieldName = (*it)._fieldName ;
            joinUnit->addOpField( qgmOpField( fieldName, SQL_GRAMMAR::DBATTR ),
                                  FALSE ) ;
            ++it ;
         }
         joinUnit->setDispatchAlias( outer()->getAlias(TRUE) ) ;
         _oprUnits.push_back( joinUnit ) ;
         joinUnit = NULL ;
      }
      else
      {
         SDB_ASSERT( NULL != _condition &&
                     NULL != _condition->left &&
                     NULL != _condition->right, "can not be NULL") ;
         SDB_ASSERT( SQL_GRAMMAR::DBATTR == _condition->left->type &&
//                     SQL_GRAMMAR::SQLMAX <  _condition->right->type,
                     SQL_GRAMMAR::DBATTR == _condition->right->type,
                     "impossible" ) ;
         /// here we reset right type with dbattr.
//         _condition->right->type = SQL_GRAMMAR::DBATTR ;

         joinUnit = SDB_OSS_NEW qgmOprUnit( QGM_OPTI_TYPE_JOIN ) ;
         if ( !joinUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         joinUnit->setDispatchAlias( outer()->getAlias(TRUE) ) ;
         joinUnit->addOpField( qgmOpField(_condition->right->value,
                                          SQL_GRAMMAR::DBATTR),
                               FALSE) ;
         _oprUnits.push_back( joinUnit ) ;
         joinUnit = NULL ;

         joinUnit = SDB_OSS_NEW qgmOprUnit( QGM_OPTI_TYPE_JOIN_CONDITION ) ;
         if ( !joinUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         joinUnit->setDispatchAlias( inner()->getAlias(TRUE) ) ;
         joinUnit->addOpField( qgmOpField(_condition->left->value,
                                          SQL_GRAMMAR::DBATTR),
                               FALSE) ;
         _oprUnits.push_back( joinUnit ) ;
         joinUnit = NULL ;
      }


   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN__CRTJOINUNIT, rc ) ;
      return rc ;
   error:
      if ( joinUnit )
      {
         SDB_OSS_DEL joinUnit ;
      }
      goto done ;
   }

   INT32 _qgmOptiNLJoin::_makeOuterInner ()
   {
      if ( _children.size() != 2 )
      {
         return SDB_SYS ;
      }

      if ( SQL_GRAMMAR::R_OUTERJOIN == _joinType )
      {
         _outer = &_children[1] ;
         _inner = &_children[0] ;
         _joinType = SQL_GRAMMAR::L_OUTERJOIN ;
      }
      else
      {
         _outer = &_children[0] ;
         _inner = &_children[1] ;
      }

      return SDB_OK ;
   }

   string _qgmOptiNLJoin::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() ;
      // join type

      if ( SQL_GRAMMAR::INNERJOIN == _joinType )
      {
         ss << ",JoinType:" << "inner" ;
      }
      else if ( SQL_GRAMMAR::L_OUTERJOIN == _joinType )
      {
         ss << ",JoinType:" << "left" ;
      }
      else if ( SQL_GRAMMAR::R_OUTERJOIN == _joinType )
      {
         ss << ",JoinType:" << "right" ;
      }
      else
      {
         ss << ",JoinType:" << "full" ;
      }

      if ( NULL != _condition )
      {
         _qgmConditionNodeHelper condition( _condition ) ;
         ss << ",condition:" << condition.toJson() ;
      }

      ss << "}" ;
      return ss.str() ;
   }

   INT32 _qgmOptiNLJoin::outputSort( qgmOPFieldVec & sortFields )
   {
      return outer()->outputSort( sortFields ) ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN_OUTPUTSTREAM, "_qgmOptiNLJoin::outputStream" )
   INT32 _qgmOptiNLJoin::outputStream( qgmOpStream &stream )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN_OUTPUTSTREAM ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 2 == _children.size(), "impossible" ) ;

      _qgmOptiTreeNode *left = _children.at( 0 ) ;
      _qgmOptiTreeNode *right = _children.at( 1 ) ;

      rc = left->outputStream( stream ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      stream.next = SDB_OSS_NEW qgmOpStream() ;
      if ( NULL == stream.next )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = right->outputStream( *(stream.next) ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN_OUTPUTSTREAM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN__PUSHOPRUNIT, "_qgmOptiNLJoin::_pushOprUnit" )
   INT32 _qgmOptiNLJoin::_pushOprUnit( qgmOprUnit * oprUnit, PUSH_FROM from )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN__PUSHOPRUNIT ) ;
      INT32 rc = SDB_OK ;
      qgmFilterUnit *filterUnit = NULL ;
      qgmFilterUnit *outerUnit = NULL ;
      qgmFilterUnit *innerUnit = NULL ;

      // if not make condition, need to make
      if ( needMakeCondition() )
      {
         rc = makeCondition() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Node[%s] make condition failed, rc: %d",
                    toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
      {
         // push to outer
         oprUnit->setDispatchAlias( outer()->getAlias( TRUE ) ) ;
         oprUnit->resetNodeID() ;
         _oprUnits.insert( _oprUnits.begin(), oprUnit ) ;
         _hasPushSort = TRUE ;
      }
      else if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
      {
         qgmConditionNodePtrVec conds ;
         qgmConditionNodePtrVec::iterator itCond ;
         BOOLEAN outerChange = FALSE ;
         BOOLEAN innerChange = FALSE ;

         outerUnit = SDB_OSS_NEW qgmFilterUnit( QGM_OPTI_TYPE_FILTER ) ;
         innerUnit = SDB_OSS_NEW qgmFilterUnit( QGM_OPTI_TYPE_FILTER ) ;
         if ( !outerUnit || !innerUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         outerUnit->setOptional( TRUE ) ;
         innerUnit->setOptional( TRUE ) ;

         filterUnit = (qgmFilterUnit*)oprUnit ;
         qgmOPFieldVec *fields = filterUnit->getFields() ;
         for ( UINT32 index = 0 ; index < fields->size() ; index++ )
         {
            qgmOpField &field = (*fields)[index] ;
            if ( field.value.relegation() == outer()->getAlias( TRUE ) )
            {
               outerUnit->addOpField( field ) ;
               outerChange = TRUE ;
            }
            else if ( field.value.relegation() == inner()->getAlias( TRUE ) )
            {
               innerUnit->addOpField( field ) ;
               innerChange = TRUE ;
            }
            else
            {
               PD_LOG_MSG( PDERROR,
                           "oprUnit[%s] field[%s] is not in outer[%s] or "
                           "inner[%s]", oprUnit->toString().c_str(),
                           field.toString().c_str(),
                           outer()->getAlias( TRUE ).toString().c_str(),
                           inner()->getAlias( TRUE ).toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // condition
         conds = filterUnit->getConditions() ;
         itCond = conds.begin() ;
         while ( itCond != conds.end() )
         {
            qgmConditionNode *condNode = *itCond ;
            while ( condNode->left )
            {
               condNode = condNode->left ;
            }

            // const value, add to outer and inner
            if ( condNode->type != SQL_GRAMMAR::DBATTR )
            {
               outerUnit->addCondition( *itCond ) ;
               innerUnit->addCondition( SDB_OSS_NEW qgmConditionNode( *itCond ) ) ;
               outerChange = TRUE ;
               innerChange = TRUE ;
            }
            else if ( condNode->value.relegation() == outer()->getAlias( TRUE ) )
            {
               outerUnit->addCondition( *itCond ) ;
               outerChange = TRUE ;
            }
            else if ( condNode->value.relegation() == inner()->getAlias( TRUE ) )
            {
               innerUnit->addCondition( *itCond ) ;
               innerChange = TRUE ;
            }
            else
            {
               PD_LOG_MSG( PDERROR,
                           "oprUnit[%s] condition attr[%s] is not in "
                           "outer[%s] or inner[%s]", oprUnit->toString().c_str(),
                           condNode->value.toString().c_str(),
                           outer()->getAlias( TRUE ).toString().c_str(),
                           inner()->getAlias( TRUE ).toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            ++itCond ;
         }

         if ( !outerChange && _varList.size() == 0 )
         {
            qgmOpField dummyField ;
            dummyField.value.relegation() = outer()->getAlias( TRUE ) ;
            _table->getUniqueFieldAlias( dummyField.value.attr() ) ;
            dummyField.type = SQL_GRAMMAR::DBATTR ;
            outerUnit->addOpField( dummyField ) ;
         }

         // if varList is not in select fields, add
         QGM_VARLIST::iterator itVar = _varList.begin() ;
         while ( itVar != _varList.end() )
         {
            if ( !isFromOne( (*itVar)._fieldName, *(outerUnit->getFields()),
                             FALSE ) &&
                 ((*itVar)._fieldName.relegation() ==
                 outer()->getAlias()) )
            {
               outerUnit->addOpField( qgmOpField( (*itVar)._fieldName,
                                                  SQL_GRAMMAR::DBATTR) ) ;
            }
            ++itVar ;
         }

         /// if it is a hash join, check all the condition fields exist in sub.
         if ( !_hints.empty() )
         {
            SDB_ASSERT( _varList.empty(), "must be empty" ) ;
            _qgmConditionNodeHelper ctree( _condition ) ;
            qgmDbAttrPtrVec attrVec ;
            ctree.getAllAttr( attrVec ) ;
            qgmDbAttrPtrVec::const_iterator itr = attrVec.begin() ;
            for ( ; itr != attrVec.end(); itr++ )
            {
               if ( !isFromOne( **itr, *(outerUnit->getFields()),
                             FALSE ) &&
                    ((*itr)->relegation() ==
                    outer()->getAlias()) )
               {
                  outerUnit->addOpField( qgmOpField( **itr,
                                         SQL_GRAMMAR::DBATTR) ) ;
               }
               else if ( !isFromOne( **itr, *(innerUnit->getFields()),
                             FALSE ) &&
                        ((*itr)->relegation() ==
                        inner()->getAlias()))
               {
                  innerUnit->addOpField( qgmOpField(**itr,
                                      SQL_GRAMMAR::DBATTR) ) ;
               }
            }
         }

         outerUnit->setDispatchAlias( outer()->getAlias( TRUE ) ) ;
         _oprUnits.push_back( outerUnit ) ;
         outerUnit = NULL ;

         if ( !innerChange )
         {
            qgmOpField dummyField ;
            dummyField.value.relegation() = inner()->getAlias( TRUE ) ;
            _table->getUniqueFieldAlias( dummyField.value.attr() ) ;
            dummyField.type = SQL_GRAMMAR::DBATTR ;
            innerUnit->addOpField( dummyField ) ;
         }

         innerUnit->setDispatchAlias( inner()->getAlias( TRUE ) ) ;
         _oprUnits.push_back( innerUnit ) ;
         innerUnit = NULL ;

         filterUnit->emptyCondition() ;
         // delete
         SDB_OSS_DEL filterUnit ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN__PUSHOPRUNIT, rc ) ;
      return rc ;
   error:
      if ( outerUnit )
      {
         SDB_OSS_DEL outerUnit ;
      }
      if ( innerUnit )
      {
         SDB_OSS_DEL innerUnit ;
      }
      goto done ;
   }

   INT32 _qgmOptiNLJoin::_removeOprUnit( qgmOprUnit * oprUnit )
   {
      return SDB_OK ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN__UPDATECHANGE, "_qgmOptiNLJoin::_updateChange" )
   INT32 _qgmOptiNLJoin::_updateChange( qgmOprUnit * oprUnit )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN__UPDATECHANGE ) ;
      INT32 rc = SDB_OK ;

      if ( QGM_OPTI_TYPE_JOIN != oprUnit->getType() &&
           QGM_OPTI_TYPE_JOIN_CONDITION != oprUnit->getType() )
      {
         goto done ;
      }

      if ( _hints.empty() )
      {
         if ( oprUnit->getFields()->size() != _varList.size() )
         {
            PD_LOG( PDERROR, "Node[%s] joinUnit[%s] field num is not with the"
                    "varList", toString().c_str(), oprUnit->toString().c_str() ) ;
            SDB_ASSERT( FALSE , "JoinUnit field num is not with varList" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         qgmOPFieldVec *fields = oprUnit->getFields() ;
         UINT32 count = _varList.size() ;
         UINT32 index = 0 ;
         while ( index < count )
         {
            _varList[index]._fieldName = (*fields)[index].value ;
            ++index ;
         }
      }
      else
      {
         qgmOPFieldVec *fields = oprUnit->getFields() ;
         SDB_ASSERT( 1 == fields->size(), "size must be one") ;
         if ( oprUnit->getDispatchAlias() == inner()->getAlias() )
         {
            _condition->left->value = fields->at( 0 ).value ;
         }
         else
         {
            _condition->right->value = fields->at( 0 ).value ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN__UPDATECHANGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN_HANDLEHINTS, "_qgmOptiNLJoin::handleHints" )
   INT32 _qgmOptiNLJoin::handleHints( QGM_HINS &hints )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN_HANDLEHINTS ) ;
      INT32 rc = SDB_OK ;
      if ( SQL_GRAMMAR::INNERJOIN != _joinType)
      {
         goto done ;
      }
      {
      QGM_HINS::iterator itr = hints.begin() ;
      for ( ; itr != hints.end(); itr++ )
      {
         if ( 0 == ossStrncmp( itr->value.begin(),
                               QGM_HINT_HASHJOIN,
                               itr->value.size() ))
         {
            /// TODO: judgement should be done in optimize.
            if ( NULL != _condition &&
                 SQL_GRAMMAR::EG == _condition->type )
            {
               SDB_ASSERT( NULL != _condition->left &&
                           NULL != _condition->right, "impossible") ;
               if ( SQL_GRAMMAR::DBATTR == _condition->left->type &&
                    SQL_GRAMMAR::DBATTR == _condition->right->type )
               {
                  qgmHint hint = *itr ;
                  _hints.push_back( hint ) ;
               }
            }
         }
      }
      }

      if ( 2 != _children.size() )
      {
         goto done ;
      }

       /// _outer has not been initialized yet.
      if ( QGM_OPTI_TYPE_SCAN == _children[0]->getType() )
      {
         rc = _handleHints( _children[0], hints ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "sub node failed to handle hint:%d", rc ) ;
            goto error ;
         }
      }

      if ( QGM_OPTI_TYPE_SCAN == _children[1]->getType() )
      {
         rc = _handleHints( _children[1], hints ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "sub node failed to handle hint:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN_HANDLEHINTS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmOptiNLJoin::_handleHints( _qgmOptiTreeNode *sub,
                                       const QGM_HINS &hint )
   {
      INT32 rc = SDB_OK ;
      QGM_HINS copy ;
      const qgmField &alias = sub->getAlias() ;
      QGM_HINS::const_iterator itr = hint.begin() ;
      for ( ; itr != hint.end(); itr++ )
      {
         if ( 0 == ossStrncmp( itr->value.begin(),
                               QGM_HINT_USEINDEX,
                               itr->value.size() ) &&
              2 == itr->param.size() )
         {
            const qgmField &tName = itr->param.begin()->value.attr() ;
            if ( alias == tName )
            {
               copy.push_back( *itr ) ;
               break ;
            }
         }
      }

      if ( copy.empty() )
      {
         goto done ;
      }

      rc = sub->handleHints( copy ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to handle hint in sub node:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmOptiNLJoin::_extend( _qgmOptiTreeNode *&exNode )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 2 == _children.size(), "impossible" ) ;

      rc = _validate() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      exNode = this ;

   done:
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTINLJOIN__VALIDATE, "_qgmOptiNLJoin::_validate" )
   INT32 _qgmOptiNLJoin::_validate()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTINLJOIN__VALIDATE ) ;
      SDB_ASSERT( 2 == _children.size(), "impossible" ) ;
      INT32 rc = SDB_OK ;
      _qgmOptiTreeNode *left = _children.at( 0 ) ;
      _qgmOptiTreeNode *right = _children.at( 1 ) ;

      if ( left->_alias == right->_alias )
      {
         PD_LOG_MSG( PDERROR, "same alias:%s",
                     left->_alias.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( NULL != _condition )
      {
         qgmOpStream lstream, rstream ;
         qgmDbAttrPtrVec conditionFields ;
         qgmDbAttrPtrVec::iterator citr ;
         _qgmConditionNodeHelper cTree( _condition ) ;

         cTree.getAllAttr( conditionFields ) ;
         rc = left->outputStream( lstream ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = right->outputStream( rstream ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         for ( citr = conditionFields.begin()
               ; citr != conditionFields.end()
               ; citr++ )
         {
            if ( lstream.find( *(*citr) ) )
            {
               continue ;
            }
            else if ( !rstream.find( *(*citr) ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR,
                           "condition field[%s] not found in sub output.",
                           (*citr)->toString().c_str() ) ;
               goto error ;
            }
            else
            {
               continue ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTINLJOIN__VALIDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _qgmOptiNLJoin::validateBeforeChange( QGM_OPTI_TYPE type ) const
   {
      return QGM_OPTI_TYPE_JOIN == type ||
             QGM_OPTI_TYPE_JOIN_CONDITION == type ;
   }
}
