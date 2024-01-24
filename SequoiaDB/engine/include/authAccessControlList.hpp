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

   Source File Name = authAccessControlList.hpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef AUTH_ACL_HPP__
#define AUTH_ACL_HPP__

#include "ossTypes.hpp"
#include "authPrivilege.hpp"
#include "utilSharedPtrHelper.hpp"

namespace engine
{
   class _authAccessControlList : public SDBObject
   {
      typedef ossPoolMap< boost::shared_ptr< authResource >,
                          boost::shared_ptr< authActionSet >,
                          SHARED_TYPE_LESS< authResource > >
         DATA_TYPE;

   public:
      _authAccessControlList() {}
      ~_authAccessControlList() {}

      INT32 addPrivilege( const authPrivilege &p );
      INT32 addPrivilege( const bson::BSONObj & );

      INT32 removePrivilege( const authPrivilege &p );
      INT32 removePrivilege( const bson::BSONObj & );

      BOOLEAN isAuthorizedForPrivilege( const authPrivilege &p ) const;
      BOOLEAN isAuthorizedForActionsOnResource( const authResource &res,
                                                const authActionSet &actions ) const;

      void toBSONArray( bson::BSONArrayBuilder &builder ) const;

   private:
      DATA_TYPE _data;
   };
   typedef _authAccessControlList authAccessControlList;
} // namespace engine

#endif