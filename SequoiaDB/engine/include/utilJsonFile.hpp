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

   Source File Name = utilJsonFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_JSON_FILE_HPP_
#define UTIL_JSON_FILE_HPP_

#include "oss.hpp"
#include "ossFile.hpp"
#include "../bson/bsonobj.h"

namespace engine
{
   class utilJsonFile: public SDBObject
   {
   public:
      static INT32 read( ossFile& file, bson::BSONObj& data ) ;
      static INT32 write( ossFile& file, bson::BSONObj& data ) ;
   } ;
}

#endif /* UTIL_JSON_FILE_HPP_ */

