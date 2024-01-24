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

   Source File Name = authPrivilege.hpp

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

#ifndef AUTH_PRIVILEGE_HPP__
#define AUTH_PRIVILEGE_HPP__

#include "authResource.hpp"
#include "authActionSet.hpp"
#include <vector>

namespace engine
{
   class _authPrivilege : public SDBObject
   {
      public:
         _authPrivilege() {}
         _authPrivilege( const boost::shared_ptr< authResource > &resource,
                         const boost::shared_ptr< authActionSet > &actions );
         ~_authPrivilege();

         const boost::shared_ptr<authResource> &getResource() const;
         const boost::shared_ptr<authActionSet> &getActionSet() const;

      private:
         boost::shared_ptr<authResource> _resource;
         boost::shared_ptr<authActionSet> _actionSet;
   };
   typedef _authPrivilege authPrivilege;

   const authActionSet *authGetActionSetMask( RESOURCE_TYPE type );
} // namespace engine

#endif