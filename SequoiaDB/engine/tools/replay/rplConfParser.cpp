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

   Source File Name = rplConfParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rplConfParser.hpp"
#include "rplTableMapping.hpp"
#include "ossFile.hpp"
#include "utilJsonFile.hpp"
#include "rplConfDef.hpp"
#include "../bson/bson.hpp"
#include "oss.hpp"
#include <string>

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   rplConfParser::rplConfParser()
   {
   }

   rplConfParser::~rplConfParser()
   {
   }

   INT32 rplConfParser::init( const CHAR *confFile )
   {
      INT32 rc = SDB_OK ;
      ossFile file ;

      rc = file.open( confFile, OSS_READONLY, OSS_DEFAULTFILE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open ini file[%s], rc=%d",
                   confFile, rc ) ;

      rc = utilJsonFile::read( file, _conf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read json from conf file[%s], "
                   "rc=%d", confFile, rc) ;
   done:
      if ( file.isOpened() )
      {
         file.close() ;
      }

      return rc ;
   error:
      goto done ;
   }

   BSONObj rplConfParser::getConf() const
   {
      return _conf ;
   }

   string rplConfParser::getType() const
   {
      return _conf.getStringField( RPL_CONF_OUTPUT_TYPE ) ;
   }
}

