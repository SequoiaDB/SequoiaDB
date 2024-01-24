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

   Source File Name = authBuiltinRole.cpp

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

#include "authBuiltinRole.hpp"
#include "authDef.hpp"

namespace engine
{
   using namespace bson;
   _authBuiltinRole::_authBuiltinRole()
   {
      _privileges = NULL;
      _size = 0;
      _needDelete = FALSE;
   }

   _authBuiltinRole::~_authBuiltinRole()
   {
      if ( _needDelete )
      {
         delete[] _privileges;
      }
   }

   void _authBuiltinRole::set( authPrivilege *privileges, INT32 size, BOOLEAN needDelete )
   {
      if ( _needDelete )
      {
         delete[] _privileges;
      }
      _privileges = privileges;
      _size = size;
      _needDelete = needDelete;
   }

   authPrivilege* _authBuiltinRole::getPrivileges()
   {
      return _privileges;
   }

   INT32 _authBuiltinRole::getSize()
   {
      return _size;
   }

   void _authBuiltinRole::toBSONObj( BOOLEAN showPrivileges, bson::BSONObjBuilder &builder )
   {
      
      builder.appendArray( AUTH_FIELD_NAME_ROLES, BSONArray() );
      builder.appendArray( AUTH_FIELD_NAME_INHERITED_ROLES, BSONArray() );
      if ( showPrivileges )
      {
         BSONArrayBuilder privilegesBuilder;
         for ( INT32 i = 0; i < _size; ++i )
         {
            BSONObjBuilder privBuilder( privilegesBuilder.subobjStart() );
            BSONObjBuilder resourceBuilder(
               privBuilder.subobjStart( AUTH_FIELD_NAME_RESOURCE ) );
            _privileges[ i ].getResource()->toBSONObj( resourceBuilder );
            BSONArrayBuilder actionsBuilder(
               privBuilder.subarrayStart( AUTH_FIELD_NAME_ACTIONS ) );
            _privileges[ i ].getActionSet()->toBSONArray( actionsBuilder );
            privBuilder.doneFast();
         }
         builder.appendArray( AUTH_FIELD_NAME_PRIVILEGES, privilegesBuilder.done() );
         builder.appendArray( AUTH_FIELD_NAME_INHERITED_PRIVILEGES,
                              privilegesBuilder.done() );
      }
   }
}; // namespace engine