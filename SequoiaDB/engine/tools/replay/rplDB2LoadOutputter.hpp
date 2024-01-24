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

   Source File Name = rplDB2LoadOutputter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_DB2LOAD_OUTPUTTER_HPP_
#define REPLAY_DB2LOAD_OUTPUTTER_HPP_

#include "oss.hpp"
#include "rplOutputter.hpp"
#include "rplMonitor.hpp"
#include "rplTableMapping.hpp"
#include "rplRecordWriter.hpp"
#include <string>
#include <map>

using namespace std ;
using namespace engine ;

namespace replay
{
   extern const CHAR RPL_OUTPUT_DB2LOAD[] ;
   class rplDB2LoadOutputter : public rplOutputter, public SDBObject {
   public:
      rplDB2LoadOutputter( Monitor *monitor) ;
      ~rplDB2LoadOutputter() ;

   public:
      INT32 init( const CHAR *confFile ) ;

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

      INT32 truncateCL( const CHAR *clFullName, UINT64 lsn ) ;

      INT32 pop( const CHAR *clFullName, UINT64 lsn, INT64 logicalID,
                 INT8 direction ) ;

      INT32 flush() ;

      BOOLEAN isNeedSubmit() ;

      /* rename tmp file to regular file */
      INT32 submit() ;

      /* get outputer's extra status */
      INT32 getExtraStatus( BSONObj &status ) ;

   private:
      INT32 _parseConf( const CHAR *confFile ) ;
      INT32 _generateRecord( const CHAR *clFullName, const CHAR *OP,
                             const UINT64 &opTimeMicroSecond,
                             const BSONObj &objIn, const CHAR **outDBName,
                             const CHAR **outTableName, string &strOut ) ;
      INT32 _parseSubmitTime( const CHAR *submitTime ) ;
      BOOLEAN _isNeedSumitPerDay() ;

   private:
      Monitor *_monitor ;
      rplTableMapping _tableMapping ;
      rplRecordWriter *_recordWriter ;

      string _prefix ;
      string _suffix ;
      string _outputDir ;
      string _delimiter ;

      // two submit mode:
      // 1. submit file at HH:MM per day
      INT32 _outputHour ;
      INT32 _outputMinute ;

      // 2. submit file per interval
      INT64 _submitInterval ; // microseconds
   } ;
}

#endif  /* REPLAY_DB2LOAD_OUTPUTTER_HPP_ */

