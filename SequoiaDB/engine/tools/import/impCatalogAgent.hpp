/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
