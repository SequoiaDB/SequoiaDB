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

   Source File Name = impCataInfo.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impCataInfo.hpp"
#include "clsCatalogAgent.hpp"

using namespace bson;

namespace import
{
   CataInfo::CataInfo()
   {
      _cataSet = NULL;
   }

   CataInfo::~CataInfo()
   {
      _cataSet = NULL;
   }

   BOOLEAN CataInfo::isMainCL()
   {
      SDB_ASSERT(NULL != _cataSet, "must be inited");

      return _cataSet->isMainCL();
   }

   INT32 CataInfo::getGroupNum()
   {
      SDB_ASSERT(NULL != _cataSet, "must be inited");

      return _cataSet->getAllGroupID()->size();
   }

   INT32 CataInfo::getGroupByRecord(const char* bsonData, UINT32& groupId)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != _cataSet, "must be inited");
      SDB_ASSERT(NULL != bsonData, "bsonData can't be NULL");

      try
      {
         BSONObj obj(bsonData);

         rc = _cataSet->findGroupID(obj, groupId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get group by record, rc=%d", rc);
            goto error;
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "failed to get group by record,"
                "received unexcepted error:%s", e.what());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 CataInfo::getSubCLList(vector<string>& list)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != _cataSet, "must be inited");
      SDB_ASSERT(isMainCL(), "must be MainCL");

      rc = _cataSet->getSubCLList(list);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get subCL, rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 CataInfo::getSubCLNameByRecord(const char* bsonData,
                                                 string &subCLName)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != _cataSet, "must be inited");
      SDB_ASSERT(NULL != bsonData, "bsonData can't be NULL");

      try
      {
         BSONObj obj(bsonData);

         rc = _cataSet->findSubCLName(obj, subCLName);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find subCLName by record, rc=%d", rc);
            goto error;
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "failed to find subCLName by record,"
                "received unexcepted error:%s", e.what());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
