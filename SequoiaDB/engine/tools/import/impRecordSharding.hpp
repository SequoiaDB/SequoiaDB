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

   Source File Name = impRecordSharding.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_SHARDING_HPP_
#define IMP_RECORD_SHARDING_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCataInfo.hpp"
#include "impCatalogAgent.hpp"
#include "impHosts.hpp"
#include "../client/bson/bson.h"
#include <string>
#include <map>

using namespace std;

namespace import
{
   class RecordSharding: public SDBObject
   {
   public:
      RecordSharding();
      ~RecordSharding();
      INT32 init(const vector<Host>& hosts,
                 const string& user,
                 const string& password,
                 const string& csname,
                 const string& clname,
                 BOOLEAN useSSL);
      INT32 getGroupNum() const { return _groupNum; }
      INT32 getGroupByRecord(bson* record, string& collection, UINT32& groupId);

   private:
      INT32 _getCatalogInfo( BOOLEAN getSubCL, INT32 &cataCount ) ;

   private:
      const vector<Host>*     _hosts;
      string                  _user;
      string                  _password;
      string                  _csname;
      string                  _clname;
      BOOLEAN                 _useSSL;
      BOOLEAN                 _inited;

      CatalogAgent            _cataAgent;
      CataInfo                _cataInfo;
      string                  _collectionName;
      BOOLEAN                 _isMainCL;
      INT32                   _groupNum;
      map<string, CataInfo>   _subCataInfo;
   };
}

#endif /* IMP_RECORD_SHARDING_HPP_ */
