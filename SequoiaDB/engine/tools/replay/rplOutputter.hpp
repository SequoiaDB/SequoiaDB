/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

      virtual INT32 truncateCL( const CHAR *clFullName, UINT64 lsn,
                                  const UINT64 &opTimeMicroSecond ) = 0 ;

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

