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

   Source File Name = qgmOptiSelect.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  JHL  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMOPTIMTHMATCHSELECT_HPP_
#define QGMOPTIMTHMATCHSELECT_HPP_

#include "qgmOptiSelect.hpp"
#include "../bson/bsonobj.h"

namespace engine
{
   class qgmOptiMthMatchSelect : public _qgmOptiSelect
   {
   public:
      qgmOptiMthMatchSelect( _qgmPtrTable *pTable, _qgmParamTable *pParam );
      virtual ~qgmOptiMthMatchSelect();

      virtual BOOLEAN isEmpty();

      virtual INT32 init (){ return SDB_OK; }

      INT32 fromBson( const bson::BSONObj &matcher );

   protected:
      virtual INT32 _extend( _qgmOptiTreeNode *&exNode ) ;

   public:
      bson::BSONObj        _matcher;
   };
}

#endif
