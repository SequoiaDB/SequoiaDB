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

   Source File Name = qgmOptiInsert.hpp

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

#ifndef QGMOPTIINSERT_HPP_
#define QGMOPTIINSERT_HPP_

#include "qgmOptiTree.hpp"

namespace engine
{
   class _qgmOptiInsert : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiInsert( _qgmPtrTable *table,
                      _qgmParamTable *param ) ;
      virtual ~_qgmOptiInsert() ;

   public:
      virtual INT32 outputStream( qgmOpStream &stream ) ;

      virtual string toString() const ;

   private:
      virtual INT32 _checkPrivileges( ISession *session ) const ;

   public:
      qgmOpField     _collection ;
      BSONObj        _insertor ;
   } ;
   typedef class _qgmOptiInsert qgmOptiInsert ;
}

#endif

