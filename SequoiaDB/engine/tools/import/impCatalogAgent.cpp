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

   Source File Name = impCatalogAgent.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impCatalogAgent.hpp"
#include "clsCatalogAgent.hpp"

using namespace bson;

namespace import
{
   CatalogAgent::CatalogAgent()
   {
      _cataAgent = NULL;
   }

   CatalogAgent::~CatalogAgent()
   {
      SAFE_OSS_DELETE(_cataAgent);
   }

   INT32 CatalogAgent::updateCatalog(const char* bsonData)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != bsonData, "bsonData can't be NULL");

      if (NULL == _cataAgent)
      {
         _cataAgent = SDB_OSS_NEW engine::_clsCatalogAgent();
         if (NULL == _cataAgent)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to alloc _clsCatalogAgent");
            goto error;
         }
      }

      try
      {
         BSONObj obj(bsonData);

         rc = _cataAgent->updateCatalog(0, 0, bsonData, obj.objsize());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update _cataAgent from bson");
            goto error;
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "failed to update _cataAgent from bson,"
                "received unexcepted error:%s", e.what());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 CatalogAgent::getCataInfo(const string& collectionName, CataInfo& cataInfo)
   {
      INT32 rc = SDB_OK;
      engine::_clsCatalogSet* cataSet = NULL;

      SDB_ASSERT(NULL != _cataAgent, "_cataAgent can't be NULL");
      SDB_ASSERT(NULL == cataInfo._cataSet, "cataInfo must be empty");

      cataSet = _cataAgent->collectionSet(collectionName.c_str());
      if (NULL == cataSet)
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to find catalog of collection %s",
                collectionName.c_str());
         goto error;
      }

      cataInfo._cataSet = cataSet;
      cataInfo._collectionName = collectionName;

   done:
      return rc;
   error:
      goto done;
   }
}
