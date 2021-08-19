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

   Source File Name = rplConfParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_CONFPARSER_HPP_
#define REPLAY_CONFPARSER_HPP_

#include "utilJsonFile.hpp"
#include "../bson/bson.hpp"
#include "oss.hpp"

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   class rplConfParser : public SDBObject
   {
   public:
      rplConfParser() ;
      ~rplConfParser() ;

   public:
      INT32 init( const CHAR *confFile ) ;
      string getType() const ;
      BSONObj getConf() const ;

   private:
      BSONObj _conf ;
   } ;
}

#endif  /* REPLAY_CONFPARSER_HPP_ */


