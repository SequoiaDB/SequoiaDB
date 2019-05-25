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

