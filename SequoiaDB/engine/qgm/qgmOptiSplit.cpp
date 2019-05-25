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

   Source File Name = qgmOptiSplit.cpp

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

#include "qgmOptiSplit.hpp"

namespace engine
{
   _qgmOptiSplit::_qgmOptiSplit( _qgmPtrTable *table,
                                 _qgmParamTable *param,
                                 const qgmDbAttr &splitby )
   :_qgmOptiTreeNode(QGM_OPTI_TYPE_SPLIT, table, param ),
    _splitby(splitby)
   {
      
   }

   _qgmOptiSplit::~_qgmOptiSplit()
   {

   }

   INT32 _qgmOptiSplit::outputStream( qgmOpStream &stream )
   {
      SDB_ASSERT( 1 == _children.size(), "impossible" ) ;
      return (*(_children.begin()))->outputStream( stream ) ;
   }

   string _qgmOptiSplit::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() ;
      ss << ", split by[" << _splitby.toString() << "]}" ;
      return ss.str() ;
   }
}
