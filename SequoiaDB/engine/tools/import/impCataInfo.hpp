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

   Source File Name = impCataInfo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_CATA_INFO_HPP_
#define IMP_CATA_INFO_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <string>
#include <vector>

using namespace std;

namespace engine
{
   class _clsCatalogSet;
}

namespace import
{
   class CataInfo: public SDBObject
   {
   public:
      CataInfo();
      ~CataInfo();
      INT32 getGroupNum();
      BOOLEAN isMainCL();
      INT32 getSubCLList(vector<string>& list);
      INT32 getGroupByRecord(const char* bsonData, UINT32& groupId);
      INT32 getSubCLNameByRecord(const char* bsonData, string &subCLName);

   private:
      string                     _collectionName;
      engine::_clsCatalogSet*    _cataSet;

   friend class CatalogAgent;
   };
}

#endif /* IMP_CATA_INFO_HPP_ */
