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

\#ifndef AUTH_ACTION_TYPE_GEN_HPP__
\#define AUTH_ACTION_TYPE_GEN_HPP__
\#include <vector>
namespace engine
{
   enum ACTION_TYPE_ENUM
   {
   #for $index, $key in enumerate($ACTION_TYPE_ENUM.values)
      #if $index == 0
      ACTION_TYPE_${key} = -1,
      #else
      ACTION_TYPE_${key},
      #end if
   #end for
   };

   const int ACTION_TYPE_NUM_GEN = $len($ACTION_TYPE_ENUM.values);
   const int ACTION_TYPE_VALID_NUM_GEN = ${len($ACTION_TYPE_ENUM.values) - 1};


   ACTION_TYPE_ENUM authActionTypeParse( const char *actionName );
   const char *authActionTypeSerializer( ACTION_TYPE_ENUM actionType );

   enum RESOURCE_TYPE_ENUM
   {
   #for $index, $key in enumerate($RESOURCE_TYPE_ENUM.values)
      RESOURCE_TYPE_${key},
   #end for
   };

   const int ACTION_SET_NUMBERS = $actionset_numbers ;
   struct ACTION_SET_NUMBER_ARRAY
   {
      unsigned long long numbers[ACTION_SET_NUMBERS];
      ACTION_SET_NUMBER_ARRAY(
      #for $index in range($actionset_numbers)
         #if $index != $actionset_numbers - 1
         unsigned long long n${index},
         #else
         unsigned long long n${index}
         #end if
      #end for
      )
      {
      #for $index in range($actionset_numbers)
         numbers[${index}] = n${index};
      #end for
      }
   };

#for $index, $key in enumerate($RESOURCE_BITSETS):
   // $key: $RESOURCE_TYPE_ENUM.values[$key].extra_data
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_${key}_BITSET_NUMBERS(
   #for num_index, num in enumerate($RESOURCE_BITSETS[$key])
   #if $num_index != $len($RESOURCE_BITSETS[$key]) - 1
      ${num}ULL,
   #else
      ${num}ULL
   #end if
   #end for
   );

#end for

   // Built-in roles
#for $key, $role in $BUILTIN_ROLES.items()
   // $key
   const int BUILTIN_ROLE_DATA_SIZE${key} = $len($role.privileges);
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA${key}[];

#end for

   enum AUTH_CMD_ACTION_SETS_TAG
   {
   #for $tag in $tags
      $tag.tag_name,
   #end for
   };

   typedef class _authActionSet authActionSet;
   typedef const authActionSet *ACTION_SETS_PTR;
   class authRequiredActionSets
   {
      public:
         enum SOURCE_OBJ
         {
            SOURCE_OBJ_NONE,
            SOURCE_OBJ_QUERY,
            SOURCE_OBJ_SELECTOR,
            SOURCE_OBJ_ORDERBY,
            SOURCE_OBJ_HINT
         };

         struct SOURCE
         {
            SOURCE( SOURCE_OBJ obj, const char *key ) : obj( obj ), key( key ) {}
            SOURCE_OBJ obj;
            const char *key;
         };
      public:
         authRequiredActionSets( RESOURCE_TYPE_ENUM t, ACTION_SETS_PTR sets, unsigned int size, SOURCE_OBJ obj, const char *key )
            : _t( t ), _sets( sets ), _size( size ), _source( obj, key ) {}
         RESOURCE_TYPE_ENUM getResourceType() const { return _t; }
         ACTION_SETS_PTR getActionSets() const { return _sets; }
         unsigned int getSize() const { return _size; }
         SOURCE getSource() const { return _source; }
      private:
         RESOURCE_TYPE_ENUM _t;
         ACTION_SETS_PTR _sets;
         unsigned int _size;
         SOURCE _source;
   };
   typedef const std::pair< const AUTH_CMD_ACTION_SETS_TAG*, unsigned int > CMD_TAGS_ARRAY;
   const CMD_TAGS_ARRAY* authGetCMDActionSetsTags( const char *cmd );
   const authRequiredActionSets* authGetCMDActionSetsByTag( AUTH_CMD_ACTION_SETS_TAG tag );
}

\#endif