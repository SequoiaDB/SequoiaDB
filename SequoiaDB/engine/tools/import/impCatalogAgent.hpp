/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = impCatalogAgent.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_CATALOG_AGENT_HPP_
#define IMP_CATALOG_AGENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCataInfo.hpp"
#include <string>

using namespace std;

namespace engine
{
   class _clsCatalogAgent;
}

namespace import
{
   // define CatalogAgent to avoid bson namespace conflict between c and C++ bson API
   class CatalogAgent: public SDBObject
   {
   public:
      CatalogAgent();
      ~CatalogAgent();
      INT32 updateCatalog(const char* bsonData);
      INT32 getCataInfo(const string& collectionName, CataInfo& cataInfo);

   private:
      engine::_clsCatalogAgent*     _cataAgent;
   };
}

#endif /* IMP_CATALOG_AGENT_HPP_ */
