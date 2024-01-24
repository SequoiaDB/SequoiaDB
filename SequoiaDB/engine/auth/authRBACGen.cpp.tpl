/******************************************************************************


   Copyright (C) 2011-2023 SequoiaDB Ltd.

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


******************************************************************************/

/* This list file is automatically generated, you shoud NOT modify this file anyway!*/

\#include "ossTypes.hpp"
\#include "authRBACGen.hpp"
\#include "authActionSet.hpp"
\#include "msgDef.h"
\#include <cstring>

namespace engine
{
#for $key, $value in $ACTION_TYPE_ENUM.values.items()
   const char ACTION_NAME_${key}[] = "$value";
#end for

   ACTION_TYPE_ENUM authActionTypeParse( const char *actionName )
   {
   #for $key, $value in $ACTION_TYPE_ENUM.values.items()
      if (strcmp(actionName, ACTION_NAME_${key}) == 0)
      {
         return ACTION_TYPE_${key};
      }
   #end for
      return ACTION_TYPE__invalid;
   }

   const char *authActionTypeSerializer( ACTION_TYPE_ENUM actionType )
   {
   #for $key, $value in $ACTION_TYPE_ENUM.values.items()
      if (actionType == ACTION_TYPE_${key})
      {
         return ACTION_NAME_${key};
      }
   #end for
      return ACTION_NAME__invalid;
   }

#for $key, $value in $RESOURCE_TYPE_ENUM.values.items()
   const char RESOURCE_NAME_${key}[] = "$value.value";
#end for

#for $tag in $tags
   static const authActionSet ${tag.tag_name}_ACTION_SETS_ARRAY[] = {
   #for $actionset in $tag.actionsets
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      #for num_index, num in enumerate($actionset)
   #if $num_index != $len($RESOURCE_BITSETS[$key]) - 1
      ${num}ULL,
   #else
      ${num}ULL
   #end if
   #end for
      )),
   #end for
   };
   static const authRequiredActionSets ${tag.tag_name}_SETS( RESOURCE_TYPE_${tag.resource_type} , ${tag.tag_name}_ACTION_SETS_ARRAY, $len($tag.actionsets), 
   authRequiredActionSets::SOURCE_OBJ_${tag.from.obj.upper()}, $tag.from.key
   );

#end for


   const authRequiredActionSets* authGetCMDActionSetsByTag( AUTH_CMD_ACTION_SETS_TAG tag )
   {
   #for $tag in $tags
      if (tag == ${tag.tag_name})
      {
         return &${tag.tag_name}_SETS;
      }
   #end for
      return NULL;
   }

   #for $cmd, $privileges in $CMD_PRIVILEGE_MAP.values.items()
   static const AUTH_CMD_ACTION_SETS_TAG ${cmd}_TAGS_ARRAY[] = {
   #for $privilege in $privileges
      #if $isinstance($privilege.resource, dict)
         #if $privilege.resource.has_key("tag")
            AUTH_${cmd}_$privilege.resource.tag,
         #else
             AUTH_${cmd}_default,
         #end if
      #else
         AUTH_${cmd}_default,
      #end if
   #end for
   };
   static const CMD_TAGS_ARRAY ${cmd}_TAGS( ${cmd}_TAGS_ARRAY, $len($privileges));
   #end for

   const CMD_TAGS_ARRAY *authGetCMDActionSetsTags( const char *cmd )
   {
   #for $cmd in $CMD_PRIVILEGE_MAP.values
      if (strcmp(cmd, $cmd) == 0)
      {
         return &${cmd}_TAGS;
      }
   #end for
      return NULL;
   }

   // Built-in roles
#for $key, $role in $BUILTIN_ROLES.items()
   // $key
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA${key}[BUILTIN_ROLE_DATA_SIZE${key}] = {
   #for $privilege in $role.privileges
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_${privilege.resource.type}, ACTION_SET_NUMBER_ARRAY(
      #for num_index, num in enumerate($privilege.actions)
      #if $num_index != $len($privilege.actions) - 1
         ${num}ULL,
      #else
         ${num}ULL
      #end if
      #end for
      )),
   #end for   
   };

#end for
}