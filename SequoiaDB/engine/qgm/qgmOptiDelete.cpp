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

   Source File Name = qgmOptiDelete.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2023  ZHY Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmOptiDelete.hpp"
#include "authAccessControlList.hpp"
#include "boost/exception/diagnostic_information.hpp"

namespace engine
{
   INT32 qgmOptiDelete::_checkPrivileges( ISession *session ) const
   {
      INT32 rc = SDB_OK;
      if ( !session->privilegeCheckEnabled() )
      {
         goto done;
      }

      {
         authActionSet actions;
         actions.addAction( ACTION_TYPE_remove );
         rc = session->checkPrivilegesForActionsOnExact( _collection.toString().c_str(), actions );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine