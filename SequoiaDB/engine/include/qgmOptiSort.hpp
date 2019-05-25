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

   Source File Name = qgmOptiSort.hpp

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

#ifndef QGMOPTISORT_HPP_
#define QGMOPTISORT_HPP_

#include "qgmOptiTree.hpp"

namespace engine
{
   class _qgmOptiSort : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiSort( _qgmPtrTable *table, _qgmParamTable *param ) ;
      virtual ~_qgmOptiSort() ;

      virtual INT32        init () ;
      qgmOPFieldVec* getOrderby() ;

   public:
      virtual INT32 outputStream( qgmOpStream &stream ) ;
      virtual BOOLEAN   isEmpty() ;

      virtual string toString() const ;

   protected:
      virtual INT32 _pushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      virtual INT32 _removeOprUnit( qgmOprUnit *oprUnit ) ;
      virtual INT32 _updateChange( qgmOprUnit *oprUnit ) ;

   public:
      INT32 append( const qgmOPFieldVec &field,
                    BOOLEAN keepRelegation = TRUE ) ;

   public:
      qgmOPFieldVec      _orderby ;

   } ;
   typedef class _qgmOptiSort qgmOptiSort ;

}

#endif

