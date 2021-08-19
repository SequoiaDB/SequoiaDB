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

   Source File Name = rplOutputter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_OUTPUTTER_HPP_
#define REPLAY_OUTPUTTER_HPP_

#include "oss.hpp"
#include "ossFile.hpp"
#include "../bson/bsonobj.h"
#include <string>
#include <vector>

using namespace std;
using namespace bson;

namespace replay
{
   class rplOutputter {
   public:
      virtual ~rplOutputter(){} ;
   public:
      virtual string getType() = 0 ;

      virtual INT32 insertRecord( const CHAR *clFullName, UINT64 lsn,
                                  const BSONObj &obj,
                                  const UINT64 &opTimeMicroSecond ) = 0;

      virtual INT32 deleteRecord( const CHAR *clFullName, UINT64 lsn,
                                  const BSONObj &oldObj,
                                  const UINT64 &opTimeMicroSecond ) = 0 ;

      virtual INT32 updateRecord( const CHAR *clFullName, UINT64 lsn,
                                  const BSONObj &matcher,
                                  const BSONObj &newModifier,
                                  const BSONObj &shardingKey,
                                  const BSONObj &oldModifier,
                                  const UINT64 &opTimeMicroSecond,
                                  const UINT32 &logWriteMod ) = 0 ;

      virtual INT32 truncateCL( const CHAR *clFullName, UINT64 lsn ) = 0 ;

      virtual INT32 pop( const CHAR *clFullName, UINT64 lsn, INT64 logicalID,
                         INT8 direction ) = 0 ;

      /* flush all previous operations( such as flush file ) */
      virtual INT32 flush() = 0 ;

      virtual BOOLEAN isNeedSubmit() = 0 ;

      /* submit all previous operations( such as rename tmp file to regular file)*/
      virtual INT32 submit() = 0 ;

      /* get outputer's extra status */
      virtual INT32 getExtraStatus( BSONObj &status ) = 0 ;
   } ;
}

#endif  /* REPLAY_OUTPUTTER_HPP_ */

