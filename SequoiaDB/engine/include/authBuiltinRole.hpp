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

   Source File Name = authBuiltinRole.hpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   version of runtime component. This file contains code logic for
   data delete request from coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/27/2023  ZHY Initial Draft
   Last Changed =

*******************************************************************************/

#include "authPrivilege.hpp"

namespace engine
{
   class _authBuiltinRole
   {
   public:
      _authBuiltinRole();
      ~_authBuiltinRole();

      void set( authPrivilege *privileges, INT32 size, BOOLEAN needDelete );
      authPrivilege* getPrivileges();
      INT32 getSize();
      void toBSONObj( BOOLEAN showPrivileges, bson::BSONObjBuilder &builder );

   private:
      authPrivilege *_privileges;
      INT32 _size;
      BOOLEAN _needDelete;
   };
   typedef _authBuiltinRole authBuiltinRole;
}; // namespace engine