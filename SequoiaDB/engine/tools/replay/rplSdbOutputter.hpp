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

   Source File Name = rplSdbOutputter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_SDBOUTPUTTER_HPP_
#define REPLAY_SDBOUTPUTTER_HPP_

#include "oss.hpp"
#include "ossFile.hpp"
#include "../client/client.hpp"
#include "rplOutputter.hpp"
#include <string>

using namespace std;
using namespace sdbclient;

namespace replay
{
   extern const CHAR RPL_OUTPUT_SEQUOIADB[] ;
   class rplSdbOutputter : public rplOutputter, public SDBObject {
   public:
      rplSdbOutputter( BOOLEAN updateWithShardingKey,
                       BOOLEAN isKeepShardingKey ) ;
      ~rplSdbOutputter() ;

   public:
      INT32 init( const CHAR *hostName, const CHAR *svcName, const CHAR *user,
                  const CHAR *passwd, BOOLEAN useSSL ) ;

   public:
      string getType() ;

      INT32 insertRecord( const CHAR *clFullName, UINT64 lsn,
                          const BSONObj &obj,
                          const UINT64 &opTimeMicroSecond ) ;

      INT32 deleteRecord( const CHAR *clFullName, UINT64 lsn,
                          const BSONObj &oldObj,
                          const UINT64 &opTimeMicroSecond ) ;

      INT32 updateRecord( const CHAR *clFullName, UINT64 lsn,
                          const BSONObj &matcher,
                          const BSONObj &newModifier,
                          const BSONObj &shardingKey,
                          const BSONObj &oldModifier,
                          const UINT64 &opTimeMicroSecond,
                          const UINT32 &logWriteMod ) ;

      INT32 truncateCL( const CHAR *clFullName, UINT64 lsn,
                        const UINT64 &opTimeMicroSecond ) ;

      INT32 pop( const CHAR *clFullName, UINT64 lsn, INT64 logicalID,
                 INT8 direction ) ;

      /* sdb is independent in every operation, no need to flush */
      INT32 flush() { return SDB_OK ; }

      /* sdb is independent in every operation, no need to submit */
      BOOLEAN isNeedSubmit() { return FALSE ; }

      /* sdb is independent in every operation, no need to submit */
      INT32 submit() { return SDB_OK ; }

      /* sdb is independent in every operation, no need to save extra status */
      INT32 getExtraStatus( BSONObj &status )
      {
         status = BSONObj() ;
         return SDB_OK ;
      }

   private:
      sdb* _sdb ;
      BOOLEAN _updateWithShardingKey ;
      INT32 _updateFlag ;
   } ;
}

#endif  /* REPLAY_SDBOUTPUTTER_HPP_ */


