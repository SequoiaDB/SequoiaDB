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

   Source File Name = qgmOptiSplit.hpp

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

#ifndef QGMOPTISPLIT_HPP_
#define QGMOPTISPLIT_HPP_

#include "qgmOptiTree.hpp"

namespace engine
{
   class _qgmOptiSplit : public qgmOptiTreeNode
   {
   public:
      _qgmOptiSplit( _qgmPtrTable *table,
                     _qgmParamTable *param,
                     const _qgmDbAttr &split ) ;
      virtual ~_qgmOptiSplit() ;

   public:
      virtual INT32 outputStream( qgmOpStream &stream ) ;
   
      virtual string toString() const ;

      virtual BOOLEAN isEmpty() { return FALSE ;}

   public:
      qgmDbAttr _splitby ;    
   } ;
   typedef class _qgmOptiSplit qgmOptiSplit ;
}

#endif

