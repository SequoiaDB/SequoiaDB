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

   Source File Name = impRecordImporter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_IMPORTER_HPP_
#define IMP_RECORD_IMPORTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impRecordQueue.hpp"
#include "../client/client.h"
#include <string>

using namespace std;

namespace import
{
   class RecordImporter: public SDBObject
   {
   public:
      RecordImporter(const string& hostname,
                     const string& svcname,
                     const string& user,
                     const string& password,
                     const string& csname,
                     const string& clname,
                     BOOLEAN useSSL = FALSE,
                     BOOLEAN enableTransaction = FALSE,
                     BOOLEAN allowKeyDuplication = TRUE);
      ~RecordImporter();
      INT32 connect();
      void disconnect();
      INT32 import(bson* objs[], INT32 num);
      INT32 import(RecordArray* array);

   private:
      string   _hostname;
      string   _svcname;
      string   _user;
      string   _password;
      string   _csname;
      string   _clname;
      BOOLEAN  _useSSL;
      BOOLEAN  _enableTransaction;
      BOOLEAN  _allowKeyDuplication;

      // db handle
      sdbConnectionHandle  _connection;
      sdbCSHandle          _collectionSpace;
      sdbCollectionHandle  _collection;
   };
}

#endif /* IMP_RECORD_IMPORTER_HPP_ */
