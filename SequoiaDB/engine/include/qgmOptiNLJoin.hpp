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

   Source File Name = qgmOptiNLJoin.hpp

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

#ifndef QGMOPTINLJOIN_HPP_
#define QGMOPTINLJOIN_HPP_

#include "qgmOptiTree.hpp"
#include "qgmConditionNode.hpp"

namespace engine
{
   class _qgmOptiNLJoin : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiNLJoin( INT32 type, _qgmPtrTable *table,
                      _qgmParamTable *param ) ;
      virtual ~_qgmOptiNLJoin() ;

      virtual INT32        init () ;

      BOOLEAN              canSwapInnerOuter() const ;
      INT32                swapInnerOuter() ;
      BOOLEAN              needMakeCondition() const ;
      INT32                makeCondition() ;

   public:
      virtual INT32     outputSort( qgmOPFieldVec &sortFields ) ;
      virtual INT32     outputStream( qgmOpStream &stream ) ;
      virtual INT32     handleHints( QGM_HINS &hint ) ;
      virtual BOOLEAN   validateBeforeChange( QGM_OPTI_TYPE type ) const ;

      virtual string toString() const ;

      INT32 joinType() const { return _joinType ; }
      qgmOptiTreeNode* outer () { return *_outer ; }
      qgmOptiTreeNode* inner () { return *_inner ; }

   protected:
      virtual INT32 _pushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      virtual INT32 _removeOprUnit( qgmOprUnit *oprUnit ) ;
      virtual INT32 _updateChange( qgmOprUnit *oprUnit ) ;

      INT32   _makeCondVar( qgmConditionNode *cond ) ;
      INT32   _makeOuterInner () ;

      INT32   _createJoinUnit() ;

   private:
      virtual INT32 _extend( _qgmOptiTreeNode *&exNode ) ;

      INT32 _validate() ;

      INT32 _validateHint() ;

      INT32 _handleHints( _qgmOptiTreeNode *sub,
                          const QGM_HINS &hint ) ;

   public:
      INT32 _joinType ;
      qgmConditionNode *_condition ;
      qgmOptiTreeNode **_outer ;
      qgmOptiTreeNode **_inner ;
      QGM_VARLIST       _varList ;
      BOOLEAN           _hasMakeVar ;
      BOOLEAN           _hasPushSort ;
      qgmField          _uniqueNameR ;
      qgmField          _uniqueNameL ;
   } ;
   typedef class _qgmOptiNLJoin qgmOptiNLJoin ;
}

#endif

