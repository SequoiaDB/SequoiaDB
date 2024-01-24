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

   Source File Name = qgmBuilder.hpp

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

#ifndef QGMBUILDER_HPP_
#define QGMBUILDER_HPP_

#include "sqlGrammar.hpp"
#include "qgmOptiTree.hpp"

using namespace bson ;

namespace engine
{
   class _qgmOptiSelect ;
   class _qgmOptiInsert ;
   class _qgmOptiNLJoin ;
   struct _qgmConditionNode ;
   class _qgmOptiSort ;
   class _qgmPlan ;
   class _qgmOptiCommand ;
   class _qgmOptiDelete ;
   class  _qgmPtrTable ;
   class _qgmParamTable ;
   class _qgmOptiUpdate ;
   class _qgmOptiAggregation ;
   class qgmOptiMthMatchSelect;
   class _qgmSelectorExprNode ;

   /// qgmBuilder can not be freed before sql operation done.
   class _qgmBuilder : public SDBObject
   {
   public:
      _qgmBuilder( _qgmPtrTable *ptrT, _qgmParamTable *paramT ) ;
      virtual ~_qgmBuilder() ;

   public:
      static BSONObj buildOrderby( const qgmOPFieldVec &orderby ) ;

   public:
      INT32 build( const SQL_CONTAINER &tree,
                   _qgmOptiTreeNode *&node ) ;

      INT32 build( _qgmOptiTreeNode *logicalTree,
                   _qgmPlan *&physicalTree) ;

   private:
      INT32 _build( const SQL_CON_ITR &root,
                    _qgmOptiTreeNode *&node ) ;

      INT32 _buildPhysicalNode( _qgmOptiTreeNode *logicalTree,
                                _qgmPlan *&physicalTree ) ;

      INT32 _crtPhyFilter( _qgmOptiSelect *s,
                           _qgmPlan *father,
                           _qgmPlan *&phy ) ;

      INT32 _crtMthMatcherFilter( qgmOptiMthMatchSelect *s,
                                 _qgmPlan *father,
                                 _qgmPlan *&phy ) ;

      INT32 _addPhyScan( _qgmOptiSelect *s,
                         _qgmPlan *father ) ;

      INT32 _addMthMatcherScan( qgmOptiMthMatchSelect *s,
                               _qgmPlan *father ) ;

      INT32 _addPhyAggr( _qgmOptiAggregation *aggr,
                         _qgmPlan *father,
                         _qgmPlan *&pyh ) ;

      INT32 _crtPhyJoin( _qgmOptiNLJoin *join,
                         _qgmPlan *father,
                         _qgmPlan *&pyh ) ;

      INT32 _crtPhySort( _qgmOptiSort *sort,
                         _qgmPlan *father,
                         _qgmPlan *&phy ) ;

      INT32 _addPhyCommand( _qgmOptiCommand *command,
                            _qgmPlan *&father );

      INT32 _buildSelect( const SQL_CON_ITR &root,
                          _qgmOptiSelect *node,
                          const CHAR *alias = NULL,
                          UINT32 len = 0 ) ;

      INT32 _buildInsert( const SQL_CON_ITR &root,
                          _qgmOptiInsert *node ) ;

      INT32 _buildUpdate( const SQL_CON_ITR &root,
                          _qgmOptiUpdate *update ) ;

      INT32 _addSet( const SQL_CON_ITR &root,
                     BSONObjBuilder &builder ) ;

      INT32 _buildDelete( const SQL_CON_ITR &root,
                          _qgmOptiDelete *node ) ;

      INT32 _addColumns( const SQL_CON_ITR &root,
                         qgmOPFieldVec &columns ) ;

      INT32 _addValues( const SQL_CON_ITR &root,
                        qgmOPFieldVec &values ) ;

      INT32 _addSelector( const SQL_CON_ITR &root,
                          _qgmOptiSelect *node,
                          const CHAR *alias = NULL,
                          UINT32 len = 0 ) ;

      INT32 _addFrom( const SQL_CON_ITR &root,
                      _qgmOptiSelect *node,
                      const CHAR *alias = NULL,
                      UINT32 len = 0 ) ;

      INT32 _buildJoin( const SQL_CON_ITR &root,
                        _qgmOptiNLJoin *node,
                        const CHAR *alias = NULL,
                        UINT32 len = 0 ) ;

      INT32 _buildCrtCS( const SQL_CON_ITR &root,
                          _qgmOptiCommand *node ) ;

      INT32 _buildDropCS( const SQL_CON_ITR &root,
                          _qgmOptiCommand *node ) ;

      INT32 _buildCrtCL( const SQL_CON_ITR &root,
                         _qgmOptiCommand *node ) ;

      INT32 _buildDropCL( const SQL_CON_ITR &root,
                          _qgmOptiCommand *node ) ;

      INT32 _buildCrtIndex( const SQL_CON_ITR &root,
                            _qgmOptiCommand *node ) ;

      INT32 _buildDropIndex( const SQL_CON_ITR &root,
                            _qgmOptiCommand *node ) ;

      INT32 _buildCondition( const SQL_CON_ITR &root,
                             _qgmConditionNode *&condition ) ;

      INT32 _addGroupBy( const SQL_CON_ITR &root,
                         qgmOPFieldVec &groupby ) ;

      INT32 _addSplitBy( const SQL_CON_ITR &root,
                         _qgmOptiSelect *node ) ;

      INT32 _addHint( const SQL_CON_ITR &root,
                      _qgmOptiUpdate *node ) ;

      INT32 _addHint( const SQL_CON_ITR &root,
                      _qgmOptiSelect *node ) ;

      INT32 _addOrderBy( const SQL_CON_ITR &root,
                         qgmOPFieldVec &order ) ;

      INT32 _addLimit( const SQL_CON_ITR &root,
                       _qgmOptiSelect *node ) ;

      INT32 _addSkip( const SQL_CON_ITR &root,
                      _qgmOptiSelect *node ) ;

      INT32 _buildInCondition( const SQL_CON_ITR &root,
                               _qgmConditionNode *&condition ) ;

      INT32 _addSelectorFromExpr( const SQL_CON_ITR &root,
                                  _qgmOptiSelect *node,
                                  const CHAR *alias,
                                  UINT32 len ) ;

      INT32 _buildExprTree( const SQL_CON_ITR &root,
                            _qgmSelectorExprNode *exprNode,
                            qgmDbAttr &attr ) ;

   private:
      _qgmPtrTable *_table ;
      _qgmParamTable *_param ;
   } ;
   typedef class _qgmBuilder qgmBuilder ;
}

#endif

