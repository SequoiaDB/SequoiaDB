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

   Source File Name = qgmOptiSelect.cpp

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

#include "qgmOptiSelect.hpp"
#include "qgmUtil.hpp"
#include "qgmConditionNodeHelper.hpp"
#include "qgmOptiAggregation.hpp"
#include "qgmOptiSort.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "qgmExtendSelectPlan.hpp"
#include "qgmOprUnit.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "qgmHintDef.hpp"
#include "auth.hpp"
#include "boost/exception/diagnostic_information.hpp"

using namespace bson ;

namespace engine
{
   _qgmOptiSelect::_qgmOptiSelect( _qgmPtrTable *table,
                                   _qgmParamTable *param )
   :_qgmOptiTreeNode( QGM_OPTI_TYPE_SELECT, table, param ),
    _condition( NULL ),
    _limit( -1 ),
    _skip( 0 ),
    _hasFunc( FALSE )
   {

   }

   _qgmOptiSelect::~_qgmOptiSelect()
   {
      SAFE_OSS_DELETE( _condition ) ;
   }

   BOOLEAN _qgmOptiSelect::isEmpty()
   {
      if ( QGM_OPTI_TYPE_SCAN == getType() ||
           -1 != _limit || 0 != _skip ||
           hasExpr() )
      {
         return FALSE ;
      }

      return getOprUnitCount() == 0 ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT_INIT, "_qgmOptiSelect::init" )
   INT32 _qgmOptiSelect::init()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT_INIT ) ;
      INT32 rc = SDB_OK ;
      qgmFilterUnit *filterUnit = NULL ;

      if ( _limit < -1 )
      {
         _limit = -1 ;
      }
      if ( _skip < 0 )
      {
         _skip = 0 ;
      }

      if ( QGM_OPTI_TYPE_SCAN == getType() )
      {
         goto done ;
      }
      else
      {
         // create a filterUnit
         filterUnit = SDB_OSS_NEW qgmFilterUnit( QGM_OPTI_TYPE_FILTER ) ;
         if ( !filterUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         filterUnit->addOpField( _selector ) ;
         if ( _condition )
         {
            filterUnit->addCondition( _condition ) ;
            _condition = NULL ;
         }
         _oprUnits.push_back( filterUnit ) ;
         filterUnit = NULL ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISELECT_INIT, rc ) ;
      return rc ;
   error:
      if ( filterUnit )
      {
         SDB_OSS_DEL filterUnit ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT_DONE, "_qgmOptiSelect::done" )
   INT32 _qgmOptiSelect::done()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT_DONE ) ;
      qgmFilterUnit *filterUnit = NULL ;

      filterUnit = (qgmFilterUnit*)getOprUnitByType( QGM_OPTI_TYPE_FILTER ) ;
      if ( filterUnit )
      {
         SDB_ASSERT( !filterUnit->isNodeIDValid(), "impossible" ) ;
         _condition = filterUnit->getMergeCondition() ;
         filterUnit->emptyCondition() ;
      }

      /// When the child node is scan/filter, can put down the limit and skip
      if ( QGM_OPTI_TYPE_FILTER == getType() &&
           ( QGM_OPTI_TYPE_SCAN == getSubNode(0)->getType() ||
             QGM_OPTI_TYPE_FILTER == getSubNode(0)->getType() ) )
      {
         qgmOptiSelect *pNode = (qgmOptiSelect*)getSubNode(0) ;

         if ( pNode->_limit >= 0 )
         {
            if ( pNode->_limit <= _skip )
            {
               pNode->_limit = 0 ;
               pNode->_skip = 0 ;
            }
            else
            {
               pNode->_limit -= _skip ;
               pNode->_skip += _skip ;
            }
         }
         else if ( _skip > 0 )
         {
            pNode->_skip += _skip ;
         }

         if ( _limit >= 0 )
         {
            if ( -1 == pNode->_limit || _limit < pNode->_limit )
            {
               pNode->_limit = _limit ;
            }
         }

         _limit = -1 ;
         _skip = 0 ;
      }

      if ( getParent() && QGM_OPTI_TYPE_FILTER == getType() )
      {
         qgmOPFieldPtrVec fieldAlias ;
         _getFieldAlias( fieldAlias, FALSE ) ;

         // if  sub node is not join, and selectors has no field alias,
         // the selectors can be empty
         if ( QGM_OPTI_TYPE_JOIN != getSubNode(0)->getType() &&
              0 == fieldAlias.size() && !hasExpr() )
         {
            _selector.clear() ;
         }
         // if the parent is aggr node, and the node has no condition and no
         // constraint, the node can be remove
         else if ( QGM_OPTI_TYPE_AGGR == getParent()->getType() &&
                   NULL == _condition && !hasConstraint() )
         {
            qgmFieldVec subAlias ;
            qgmAggrUnit *aggrUnit = ( qgmAggrUnit* )
               (getParent()->getOprUnitByType( QGM_OPTI_TYPE_AGGR )) ;

            if ( aggrUnit )
            {
               // step1: replace table alias
               if ( validSelfAlias() && 1 == _getSubAlias( subAlias ) )
               {
                  aggrUnit->replaceRele( subAlias[0] ) ;
               }

               if ( subAlias[0].empty() )
               {
                  fieldAlias.clear() ;
                  _getFieldAlias( fieldAlias, TRUE ) ;
               }

               // step2: replace feild alias
               aggrUnit->replaceFieldAlias( fieldAlias ) ;
               getParent()->updateChange( aggrUnit ) ;

               // step3: remove local filter unit
               if ( filterUnit )
               {
                  removeOprUnit( filterUnit, TRUE, FALSE ) ;
               }
            }
         }
      }

      PD_TRACE_EXIT( SDB__QGMOPTISELECT_DONE ) ;
      return qgmOptiTreeNode::done() ;
   }

   INT32 _qgmOptiSelect::outputSort( qgmOPFieldVec &sortFields )
   {
      if ( QGM_OPTI_TYPE_SCAN == getType() )
      {
         sortFields = _orderby ;
         if ( sortFields.size() > 0 )
         {
            _upFields( sortFields ) ;
         }
         return SDB_OK ;
      }
      else
      {
         return qgmOptiTreeNode::outputSort( sortFields ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT_OUTPUTSTREAM, "_qgmOptiSelect::outputStream" )
   INT32 _qgmOptiSelect::outputStream( qgmOpStream &stream )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT_OUTPUTSTREAM ) ;
      INT32 rc = SDB_OK ;

      qgmOPFieldVec::iterator itr = _selector.begin() ;
      for ( ; itr != _selector.end(); itr++ )
      {
         stream.stream.push_back( *itr ) ;
      }

      if ( !_alias.empty() )
      {
         stream.alias = _alias ;
      }

      PD_TRACE_EXITRC( SDB__QGMOPTISELECT_OUTPUTSTREAM, rc ) ;
      return rc ;
   }

   string _qgmOptiSelect::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() ;
      if ( !_selector.empty() )
      {
         ss << ",selector:[" ;
         qgmOPFieldVec::const_iterator itr = _selector.begin() ;
         for ( ; itr != _selector.end(); itr++ )
         {
            if ( SQL_GRAMMAR::WILDCARD == itr->type )
            {
               ss << "{value:*}," ;
               continue ;
            }
            ss << "{value:" << itr->value.toString() ;
            if ( !(itr->alias.empty()) )
            {
               ss << ",alias:" << itr->alias.toString() ;
            }

            if ( !( itr->expr.isEmpty() ) )
            {
               ss << ", expr:" << itr->expr.toString() ;
            }
            ss << "}," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]" ;
      }

      if ( !_orderby.empty() )
      {
         ss << ",orderby:[" ;
         qgmOPFieldVec::const_iterator itr = _orderby.begin() ;
         for ( ; itr != _orderby.end(); itr++ )
         {
            const CHAR *t = "" ;
            if ( SQL_GRAMMAR::DBATTR == itr->type
                 || SQL_GRAMMAR::ASC == itr->type )
            {
               t = "asc" ;
            }
            else
            {
               t = "desc" ;
            }
            ss << "{value:" << itr->value.toString()
               << ",type:" << t << "}," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]" ;
      }

      if ( !_groupby.empty() )
      {
         ss << ",groupby:[" ;
         qgmOPFieldVec::const_iterator itr = _groupby.begin() ;
         for ( ; itr != _groupby.end(); itr++ )
         {
            ss << "{value:" << itr->value.toString()
               << "}," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]" ;
      }

      if ( NULL != _condition )
      {
         _qgmConditionNodeHelper condition( _condition ) ;
         ss << ",condition:" << condition.toJson()  ;
      }

      if ( -1 != _limit)
      {
         ss << ",limit:" << _limit;
      }

      if ( 0 != _skip )
      {
         ss << ",skip:"<< _skip ;
      }

      if ( !_collection.empty() )
      {
         ss << ",collection:" << _collection.value.toString() ;
      }

      if ( !_hints.empty() )
      {
         ss << ", hint:" << qgmHintToString( _hints ) ;
      }

      ss << "}" ;
      return ss.str() ;

   }

   UINT32 _qgmOptiSelect::_getFieldAlias( qgmOPFieldPtrVec & fieldAlias,
                                          BOOLEAN getAll )
   {
      UINT32 count = 0 ;

      qgmOPFieldVec::iterator it = _selector.begin() ;
      while ( it != _selector.end() )
      {
         if ( (*it).type != SQL_GRAMMAR::WILDCARD &&
              ( getAll || !(*it).alias.empty() ) )
         {
            fieldAlias.push_back( &(*it) ) ;
            ++count ;
         }
         ++it ;
      }

      return count ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT__PUSHOPRUNIT, "_qgmOptiSelect::_pushOprUnit" )
   INT32 _qgmOptiSelect::_pushOprUnit( qgmOprUnit * oprUnit, PUSH_FROM from )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT__PUSHOPRUNIT ) ;
      INT32 rc = SDB_OK ;
      qgmOprUnit *typeUnit = getOprUnitByType( oprUnit->getType() ) ;

      if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
      {
         if ( QGM_OPTI_TYPE_FILTER == getType() ) // filter
         {
            if ( typeUnit )
            {
               removeOprUnit( typeUnit, TRUE, FALSE ) ;
            }
            // need push to the first
            _oprUnits.insert( _oprUnits.begin(), oprUnit ) ;
         }
         else // scan
         {
            _orderby = *(oprUnit->getFields()) ;
            SDB_OSS_DEL oprUnit ;
         }
      }
      else if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
      {
         // update local selector
         qgmOPFieldVec *fields = oprUnit->getFields() ;
         if ( !oprUnit->isWildCardField() && fields->size() != 0 &&
              ( QGM_OPTI_TYPE_SCAN == getType() || typeUnit ) )
         {
            _selector = *fields ;
         }

         qgmFilterUnit *filterUnit = (qgmFilterUnit*)oprUnit ;
         qgmFilterUnit *typeFilterUnit = (qgmFilterUnit*)typeUnit ;

         if ( QGM_OPTI_TYPE_FILTER == getType() ) // filter
         {
            if ( typeFilterUnit )
            {
               typeFilterUnit->setFields( _selector ) ;
               if ( filterUnit->hasCondition() )
               {
                  qgmConditionNodePtrVec conds = filterUnit->getConditions() ;
                  typeFilterUnit->addCondition( conds ) ;
                  filterUnit->emptyCondition() ;
               }
               SDB_OSS_DEL filterUnit ;
            }
            else
            {
               // push to oprUnit, not change optional status
               _oprUnits.push_back( filterUnit ) ;
            }
         }
         else // scan
         {
            if ( filterUnit->hasCondition() )
            {
               if ( _condition )
               {
                  filterUnit->addCondition( _condition ) ;
               }
               _condition = filterUnit->getMergeCondition() ;
               filterUnit->emptyCondition() ;
            }
            SDB_OSS_DEL oprUnit ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISELECT__PUSHOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT__RMOPRUNIT, "_qgmOptiSelect::_removeOprUnit" )
   INT32 _qgmOptiSelect::_removeOprUnit( qgmOprUnit * oprUnit )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT__RMOPRUNIT ) ;
      INT32 rc = SDB_OK ;

      if ( oprUnit->isNodeIDValid() || oprUnit->isOptional() )
      {
         goto done ;
      }

      if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
      {
         _selector.clear() ;
      }
      else
      {
         PD_LOG_MSG( PDERROR,"Node[%s] remove oprUnit[%s] type error",
                     toString().c_str(), oprUnit->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISELECT__RMOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmOptiSelect::_updateChange( qgmOprUnit * oprUnit )
   {
      INT32 rc = SDB_OK ;

      if ( oprUnit->isNodeIDValid() )
      {
         PD_LOG_MSG( PDERROR, "Node[%s] update oprUnit[%s] failed",
                     toString().c_str(), oprUnit->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( ((qgmFilterUnit*)oprUnit)->isFieldsOptional() )
      {
         goto done ;
      }

      _selector = *(oprUnit->getFields()) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT__EXTEND, "_qgmOptiSelect::_extend" )
   INT32 _qgmOptiSelect::_extend( _qgmOptiTreeNode *&exNode )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT__EXTEND ) ;
      INT32 rc = SDB_OK ;

      qgmOpStream s ;
      _qgmOptiTreeNode *exLocal = NULL ;
      _initFrom() ;
      _qgmExtendSelectPlan plan ;

      /// not a leaf node, validate.
      if ( NULL != _from )
      {
         rc = _from->outputStream( s ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _validateAndCrtPlan( s, &plan ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         _qgmOpField dummy ;
         dummy.type = SQL_GRAMMAR::WILDCARD ;
         s.stream.push_back( dummy ) ;
         s.alias = _collection.alias ;
         rc = _validateAndCrtPlan( s, &plan ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      rc = plan.extend( exLocal ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      exNode = exLocal ;
      _type = NULL == _from ?
              QGM_OPTI_TYPE_SCAN : QGM_OPTI_TYPE_FILTER ;

      rc = handleHints( _hints ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISELECT__EXTEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _qgmOptiSelect::_initFrom()
   {
      _from = _children.size() == 1 ?
              _children.at( 0 ) : NULL ;
      return ;
   }

   INT32 _qgmOptiSelect::handleHints( QGM_HINS &hints )
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _from &&
           QGM_OPTI_TYPE_JOIN == _from->getType() )
      {
         rc = _from->handleHints( hints ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to handle hints in sub node:%d",rc ) ;
            goto error ;
         }
      }
/*
      else if ( NULL != _from &&
                QGM_OPTI_TYPE_FILTER == _from->getType() )
      {
         rc = _from->handleHints( hints ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to handle hints in sub node:%d",rc ) ;
            goto error ;
         }
      }
*/
      else if ( NULL == _from )
      {
         _handleHint( hints ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _qgmOptiSelect::_handleHint( QGM_HINS &hints )
   {
      if ( &hints != &_hints )
      {
         QGM_HINS::const_iterator itr = hints.begin() ;
         for ( ; itr != hints.end(); ++itr )
         {
            if ( 0 == ossStrncmp( itr->value.begin(),
                                  QGM_HINT_USEINDEX,
                                  itr->value.size() ) ||
                 0 == ossStrncmp( itr->value.begin(),
                                  QGM_HINT_USEOPTION,
                                  itr->value.size() ) )
            {
               _hints.push_back( *itr ) ;
            }
         }
      }
   }

   BSONObj _qgmOptiSelect::getHint() const
   {
      BSONObj obj ;

      if ( !_objHint.isEmpty() )
      {
         obj = _objHint ;
      }
      else
      {
         try
         {
            BSONObjBuilder buildOpt ;
            BSONObjBuilder build ;
            QGM_HINS::const_iterator itr = _hints.begin() ;
            for( ; itr != _hints.end(); ++itr )
            {
               if ( 0 == ossStrncmp( itr->value.begin(),
                                     QGM_HINT_USEINDEX,
                                     itr->value.size() ) )
               {
                  qgmUseIndexHintToBson( *itr, build ) ;
               }
               else if ( 0 == ossStrncmp( itr->value.begin(),
                                          QGM_HINT_USEOPTION,
                                          itr->value.size() ) )
               {
                  qgmUseOptionToBson( *itr, buildOpt ) ;
               }
            }

            BSONObj objOpt = buildOpt.obj() ;
            if ( !objOpt.isEmpty() )
            {
               BSONObjBuilder sub( build.subobjStart( "$" FIELD_NAME_OPTIONS )  ) ;
               sub.appendElements( objOpt ) ;
               sub.done() ;         
            }
            obj = build.obj() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         }
      }

      return obj ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT__VALIDATEANDCRTPLAN, "_qgmOptiSelect::_validateAndCrtPlan" )
   INT32 _qgmOptiSelect::_validateAndCrtPlan( _qgmOpStream &stream,
                                              _qgmExtendSelectPlan *plan )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT__VALIDATEANDCRTPLAN ) ;
      INT32 rc = SDB_OK ;
      qgmOPFieldVec::iterator itr ;
      qgmField uniqueField ;
      BOOLEAN addSortFeild = FALSE ;
      BOOLEAN addRootFilter = FALSE ;

      if ( NULL != stream.next )
      {
         if ( 1 == _selector.size()
              && SQL_GRAMMAR::WILDCARD == _selector.begin()->type )
         {
            PD_LOG_MSG( PDERROR, "can not select * from a join operation." ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( _alias.empty() )
      {
         _table->getUniqueTableAlias( _alias ) ;
      }

      plan->insertPlan( QGM_EXTEND_LOCAL, this ) ;
      plan->pushAlias( _alias ) ;
      plan->_original = _selector ;

      // order by
      if ( !_orderby.empty() )
      {
         itr = _orderby.begin() ;
         for ( ; itr != _orderby.end(); itr++ )
         {
            BOOLEAN found = FALSE ;
            qgmOpField *sExist = NULL ;
            UINT32 pos = 0 ;
            if ( !stream.find( itr->value ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR,
                           "orderby field[%s] not found in sub output.",
                           itr->value.toString().c_str() ) ;
               goto error ;
            }

            rc = _paramExistInSelector( itr->value, found, sExist, &pos ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( !found )
            {
               qgmOpField field( itr->value, SQL_GRAMMAR::DBATTR ) ;
               _table->getUniqueFieldAlias( field.alias ) ;
               _selector.push_back( field ) ;

               itr->value.relegation().clear() ;
               itr->value.attr() = field.alias ;

               addSortFeild = TRUE ;
            }
            else if ( NULL != sExist && !sExist->alias.empty() )
            {
               itr->value.relegation().clear() ;
               if ( 0 == pos )
               {
                  itr->value.attr() = sExist->alias ;
               }
               else
               {
                  itr->value.attr().replace(0, pos, sExist->alias ) ;
               }
            }
         }

         plan->_orderby = _orderby ;
         plan->insertPlan( QGM_EXTEND_ORDERBY ) ;
         _orderby.clear() ;
      }

      if ( addSortFeild || hasConstraint() )
      {
         plan->insertPlan( QGM_EXTEND_ORDERFILTER ) ;
         _table->getUniqueTableAlias( uniqueField ) ;
         plan->pushAlias( uniqueField ) ;

         plan->_limit    = _limit ;
         plan->_skip     = _skip ;

         clearConstraint() ;
         addRootFilter = TRUE ;
      }

      if ( _hasFunc || _groupby.size() > 0 )
      {
         plan->insertPlan( QGM_EXTEND_AGGR ) ;
         _table->getUniqueTableAlias( uniqueField ) ;
         plan->pushAlias( uniqueField ) ;
      }

      itr = _selector.begin() ;
      for ( ; itr != _selector.end(); )
      {
         if ( SQL_GRAMMAR::FUNC == itr->type )
         {
            qgmAggrSelector func ;
            if ( itr->alias.empty() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "aggr func[%s] must have alias.",
                           (*itr).toString().c_str() ) ;
               goto error ;
            }
            rc = qgmFindFieldFromFunc( itr->value.attr().begin(),
                                       itr->value.attr().size(),
                                       func.value.value.attr(),
                                       func.param,
                                       _table, _from != NULL ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            {
            /// ensure that every param in func exist in stream.
            qgmOPFieldVec::const_iterator itrPara = func.param.begin() ;
            for ( ; itrPara != func.param.end(); itrPara++ )
            {
               if ( !stream.find( itrPara->value ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Aggr func param[%s] not found in sub output",
                              itrPara->value.toString().c_str() ) ;
                  goto error ;
               }
            }
            }

            if ( !(itr->alias.empty()))
            {
               func.value.alias = itr->alias ;
            }
            func.value.type = itr->type ;

            plan->_funcSelector.push_back( func ) ;

            itr = _selector.erase( itr ) ;
         }
         else if ( SQL_GRAMMAR::WILDCARD != itr->type )
         {
            if ( !stream.find( itr->value ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "selector[%s] not found in sub output",
                           itr->value.toString().c_str() ) ;
               goto error ;
            }

            if ( _hasFunc || _groupby.size() > 0 )
            {
               qgmAggrSelector f ;
               f.value.value = (*itr).value ;
               f.value.type = SQL_GRAMMAR::DBATTR ;

               /// to avoid the upper unable to find field.
               if ( !itr->alias.empty() )
               {
                  f.value.value.relegation().clear() ;
                  f.value.value.attr() = itr->alias ;
                  f.value.expr = itr->expr ;
               }
               else if ( f.value.value.attr().isDotted() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Aggr selector[%s] dotted format "
                              "must be used with alias",
                              itr->toString().c_str() ) ;
                  goto error ;
               }

               plan->_funcSelector.push_back( f ) ;
            }

            ++itr ;
         }
         else
         {
            qgmAggrSelector f ;
            f.value.type = SQL_GRAMMAR::WILDCARD ;
            plan->_funcSelector.push_back( f ) ;
            ++itr ;
         }
      }

      {
      qgmAggrSelectorVec &aggrSelector = plan->_funcSelector ;
      qgmAggrSelectorVec::iterator itrFunc = aggrSelector.begin() ;
      for ( ; itrFunc != aggrSelector.end(); itrFunc++ )
      {
         if ( SQL_GRAMMAR::FUNC != itrFunc->value.type )
         {
            continue ;
         }
         else
         {
            qgmOPFieldVec::iterator itrPara = itrFunc->param.begin() ;
            for ( ; itrPara != itrFunc->param.end(); itrPara++ )
            {
               BOOLEAN found ;
               qgmOpField *sExist = NULL ;
               UINT32 pos = 0 ;
               /// eg: select sum(T.a), T.a from T ;
               /// we do not need put T.a into selector.
               /// but if it is: select sum(T.a), T.b from T;
               /// we need push T.a into selector.
               rc = _paramExistInSelector( itrPara->value, found,
                                           sExist, &pos ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG ( PDERROR,
                           "Failed to check paramter existence, rc = %d",
                           rc  ) ;
                  goto error ;
               }

               if ( !found )
               {
                  qgmOpField field( itrPara->value, SQL_GRAMMAR::DBATTR ) ;
                  _table->getUniqueFieldAlias( field.alias) ;
                  itrPara->value.attr() = field.alias ;
                  itrPara->value.relegation().clear() ;
                  _selector.push_back( field ) ;
               }
               else if ( NULL != sExist && !sExist->alias.empty() )
               {
                  itrPara->value.relegation().clear() ;
                  if ( 0 == pos )
                  {
                     itrPara->value.attr() = sExist->alias ;
                  }
                  else
                  {
                     itrPara->value.attr().replace( 0, pos, sExist->alias ) ;
                  }
               }
            }
         }
      }
      }

      if ( !_groupby.empty() )
      {
         itr = _groupby.begin() ;
         for ( ; itr != _groupby.end(); itr++ )
         {
            if ( !stream.find( itr->value ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "groupby field[%s] not found in sub output.",
                           itr->value.toString().c_str() ) ;
               goto error ;
            }
         }
         /// do not mind if it is failed.
         plan->insertPlan( QGM_EXTEND_GROUPBY ) ;

         itr = _groupby.begin() ;
         for ( ; itr != _groupby.end(); itr++ )
         {
            qgmOpField *sExist = NULL ;
            BOOLEAN found = FALSE ;
            UINT32 pos = 0 ;
            rc = _paramExistInSelector( itr->value, found, sExist, &pos ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to check paramter existence, rc = %d",
                        rc  ) ;
               goto error ;
            }

            if ( found )
            {
               if( NULL != sExist && !sExist->alias.empty() )
               {
                  (*itr).value.relegation().clear() ;
                  if ( 0 == pos )
                  {
                     (*itr).value.attr() = sExist->alias ;
                  }
                  else
                  {
                     (*itr).value.attr().replace( 0, pos, sExist->alias ) ;
                  }
               }
            }
            else
            {
               qgmOpField field( itr->value, SQL_GRAMMAR::DBATTR ) ;
               _table->getUniqueFieldAlias( field.alias ) ;
               (*itr).value.attr() = field.alias ;
               (*itr).value.relegation().clear() ;
               _selector.push_back( field ) ;
            }
         }

         plan->_groupby = _groupby ;
         _groupby.clear() ;
      }

      if ( NULL != _condition )
      {
         qgmDbAttrPtrVec conditionFields ;
         qgmDbAttrPtrVec::iterator citr ;
         _qgmConditionNodeHelper cTree( _condition ) ;
         cTree.getAllAttr( conditionFields ) ;
         for ( citr = conditionFields.begin()
               ; citr != conditionFields.end()
               ; citr++ )
         {
            if ( !stream.find( *(*citr)) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "condition attr[%s] not found in sub output.",
                           (*(*citr)).toString().c_str() ) ;
               goto error ;
            }
         }
      }

      if ( !_splitby.empty() )
      {
         BOOLEAN found = FALSE ;
         qgmOpField *sExist = NULL ;
         UINT32 pos = 0 ;
         if ( !stream.find( _splitby ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR,
                        "splitby field[%s] not found in sub output.",
                        _splitby.toString().c_str() ) ;
            goto error ;
         }

         rc = _paramExistInSelector( _splitby, found, sExist, &pos ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( found )
         {
            if ( NULL != sExist && !sExist->alias.empty() )
            {
               _splitby.relegation().clear() ;
               if ( 0 == pos )
               {
                  _splitby.attr() = sExist->alias ;
               }
               else
               {
                  _splitby.attr().replace( 0, pos, sExist->alias ) ;
               }
            }
         }
         else
         {
            qgmOpField field( _splitby, SQL_GRAMMAR::DBATTR ) ;
            _table->getUniqueFieldAlias( field.alias ) ;
            _splitby.attr() = field.alias ;
            _splitby.relegation().clear() ;
            _selector.push_back( field ) ;
         }

         plan->insertPlan( QGM_EXTEND_SPLITBY ) ;
         plan->_splitby = _splitby ;

         if ( !found && !addRootFilter )
         {
            plan->insertPlan( QGM_EXTEND_ORDERFILTER ) ;
            _table->getUniqueTableAlias( uniqueField ) ;
            plan->pushAlias( uniqueField ) ;
            addRootFilter = TRUE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISELECT__VALIDATEANDCRTPLAN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT__PARAMEXISTINSELECOTR, "_qgmOptiSelect::_paramExistInSelector" )
   INT32 _qgmOptiSelect::_paramExistInSelector( const qgmDbAttr &field,
                                                BOOLEAN &found,
                                                qgmOpField *&selector,
                                                UINT32 *pos )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT__PARAMEXISTINSELECOTR ) ;
      INT32 rc = SDB_OK ;
      found = FALSE ;
      qgmOPFieldVec::iterator itr = _selector.begin() ;
      for ( ; itr != _selector.end(); itr++ )
      {
         if ( SQL_GRAMMAR::WILDCARD == itr->type )
         {
            found = TRUE ;
            selector = NULL ;
         }
         if ( SQL_GRAMMAR::DBATTR == itr->type )
         {
            if ( !field.relegation().empty()
                 && !itr->value.relegation().empty() )
            {
               if ( itr->value.relegation() == field.relegation() &&
                    field.attr().isSubfix( itr->value.attr(), TRUE, pos ) )
               {
                  found = TRUE ;
                  selector = &(*itr) ;
                  break ;
               }
            }
            else if ( field.attr().isSubfix( itr->value.attr(), TRUE, pos ) )
            {
               found = TRUE ;
               selector = &(*itr) ;
               break ;
            }
            else
            {
               continue ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__QGMOPTISELECT__PARAMEXISTINSELECOTR, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT_HASEXPR, "_qgmOptiSelect::hasExpr" )
   BOOLEAN _qgmOptiSelect::hasExpr() const
   {
      BOOLEAN r = FALSE ;
      qgmOPFieldVec::const_iterator itr = _selector.begin() ;
      for ( ; itr != _selector.end(); ++itr )
      {
         if ( !( itr->expr.isEmpty() ) )
         {
            r = TRUE ;
            break ;
         }
      }

      return r ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTISELECT__CHECKPRIVILEGES, "_qgmOptiSelect::_checkPrivileges" )
   INT32 _qgmOptiSelect::_checkPrivileges( ISession *session ) const
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__QGMOPTISELECT__CHECKPRIVILEGES );
      if ( !session->privilegeCheckEnabled() )
      {
         goto done;
      }

      // pass the privilege check if select from a nested select statement
      if ( _from )
      {
         goto done;
      }

      // example: select * from $LIST_CS. The collection name is $LIST_CS.
      // We need to check privileges for $LIST_CS like a command
      if ( _collection.value.attr().begin() &&
           0 == ossStrncmp( CMD_ADMIN_PREFIX, _collection.value.attr().begin(),
                            ossStrlen( CMD_ADMIN_PREFIX ) ) )
      {
         ossPoolString cmdName = _collection.value.toString();
         rc = session->checkPrivilegesForCmd( cmdName.c_str() + 1, NULL, NULL,
                                              NULL, NULL );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges for command: %s",
                      cmdName.c_str() + 1 );
      }
      else
      {
         authActionSet actions;
         actions.addAction( ACTION_TYPE_find );
         rc = session->checkPrivilegesForActionsOnExact( _collection.value.toString().c_str(),
                                                         actions );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTISELECT__CHECKPRIVILEGES, rc );
      return rc;
   error:
      goto done;
   }
}
