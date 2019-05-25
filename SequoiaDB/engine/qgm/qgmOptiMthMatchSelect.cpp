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

   Source File Name = qgmOptiMthMatchSelect.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmOptiMthMatchSelect.hpp"

using namespace bson;

namespace engine
{
   qgmOptiMthMatchSelect::qgmOptiMthMatchSelect( _qgmPtrTable *pTable,
                                                _qgmParamTable *pParam )
   :_qgmOptiSelect( pTable, pParam )
   {
   }

   qgmOptiMthMatchSelect::~qgmOptiMthMatchSelect()
   {
   }

   BOOLEAN qgmOptiMthMatchSelect::isEmpty()
   {
      return FALSE;
   }

   INT32 qgmOptiMthMatchSelect::fromBson( const BSONObj &matcher )
   {
      _matcher = matcher;
      return SDB_OK;
   }

   INT32 qgmOptiMthMatchSelect::_extend( _qgmOptiTreeNode *&exNode )
   {
      INT32 rc = SDB_OK;
      rc = this->qgmOptiSelect::_extend( exNode );
      PD_RC_CHECK( rc, PDERROR,
                  "extend failed(rc=%d)", rc );
      _type = QGM_OPTI_TYPE_SCAN == _type ?
               QGM_OPTI_TYPE_MTHMCHSCAN : QGM_OPTI_TYPE_MTHMCHFILTER ;
   done:
      return rc;
   error:
      goto done;
   }
}
