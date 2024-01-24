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
      SDB_ASSERT(NULL != _cataSet, "Must be inited");

      return _cataSet->isMainCL();
   }

   INT32 CataInfo::getGroupNum()
   {
      SDB_ASSERT(NULL != _cataSet, "Must be inited");

      return _cataSet->getAllGroupID()->size();
   }

   INT32 CataInfo::getGroupByRecord(const char* bsonData, UINT32& groupId)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != _cataSet, "Must be inited");
      SDB_ASSERT(NULL != bsonData, "BsonData can't be NULL");

      try
      {
         BSONObj obj(bsonData);

         rc = _cataSet->findGroupID(obj, groupId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get group by record, rc=%d", rc);
            goto error;
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Failed to get group by record,"
                "received unexcepted error:%s", e.what());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 CataInfo::getAllGroupID( vector<UINT32>& list )
   {
      INT32 rc = SDB_OK ;
      INT32 size = 0 ;
      engine::VEC_GROUP_ID tmpList ;
      engine::VEC_GROUP_ID::iterator it ;

      SDB_ASSERT( NULL != _cataSet, "Must be inited" ) ;

      size = _cataSet->getAllGroupID( tmpList );
      if ( 0 > size )
      {
         rc = size ;
         PD_LOG( PDERROR, "Failed to get all group id, rc=%d", rc ) ;
         goto error ;
      }

      try
      {
         for ( it = tmpList.begin(); it != tmpList.end(); ++it )
         {
            list.push_back( *it ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 CataInfo::getSubCLList(vector<string>& list)
   {
      INT32 rc = SDB_OK;

      engine::CLS_SUBCL_LIST tmpList ;
      engine::CLS_SUBCL_LIST_IT itList ;
      SDB_ASSERT(NULL != _cataSet, "Must be inited");
      SDB_ASSERT(isMainCL(), "Must be MainCL");

      rc = _cataSet->getSubCLList( tmpList );
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get subCL, rc=%d", rc);
         goto error;
      }

      try
      {
         itList = tmpList.begin() ;
         while( itList != tmpList.end() )
         {
            list.push_back( *itList ) ;
            ++itList ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
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

      SDB_ASSERT(NULL != _cataSet, "Must be inited");
      SDB_ASSERT(NULL != bsonData, "BsonData can't be NULL");

      try
      {
         BSONObj obj(bsonData);

         rc = _cataSet->findSubCLName(obj, subCLName);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to find subCLName by record, rc=%d", rc);
            goto error;
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Failed to find subCLName by record,"
                "received unexcepted error:%s", e.what());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
