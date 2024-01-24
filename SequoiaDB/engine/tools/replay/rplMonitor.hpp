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

   Source File Name = rplMonitor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_MONITOR_HPP_
#define REPLAY_MONITOR_HPP_

#include "oss.hpp"
#include "dpsDef.hpp"
#include "../bson/bson.hpp"
#include <ctime>
#include <string>
#include <map>

using namespace std;
using namespace bson;

namespace replay
{
   class Monitor: public SDBObject
   {
   public:
      Monitor();
      ~Monitor();
      void setIsLoadFromFile( BOOLEAN isLoadFromFile ) ;
      void  opCount(UINT16 type);
      INT64 opTypeNum(UINT8 type) const;
      void  setNextLSN(DPS_LSN_OFFSET lsn);
      void  setLastLSN(DPS_LSN_OFFSET lsn);
      void  setNextFileId(UINT32 fileId);
      void  setLastFileId(UINT32 fileId);
      void  setLastFileTime(time_t lastTime);
      void  setLastMovedFileTime(time_t lastTime);
      void  setOutputType( const string &type ) ;
      void  setSerial(UINT64 serial) ;
      void setSubmitTime(UINT64 microSeconds) ;
      void setExtraInfo(BSONObj extraInfo) ;
      OSS_INLINE BOOLEAN isLoadFromFile() { return _isLoadFromFile; }
      OSS_INLINE INT64 opTotalNum() const { return _opTotalNum; }
      OSS_INLINE DPS_LSN_OFFSET getNextLSN() const { return _nextLSN; }
      OSS_INLINE DPS_LSN_OFFSET getLastLSN() const { return _lastLSN; }
      OSS_INLINE UINT32 getNextFileId() const { return _nextFileId; }
      OSS_INLINE UINT32 getLastFileId() const { return _lastFileId; }
      OSS_INLINE time_t getLastFileTime() const { return _lastFileTime; }
      OSS_INLINE time_t getLastMovedFileTime() const { return _lastMovedFileTime; }
      OSS_INLINE string getOutputType() const { return _outputType ; }
      OSS_INLINE UINT64 getSerial() const { return _serial ; }
      OSS_INLINE UINT64 getSubmitTime() const { return _summitTime ; }
      OSS_INLINE BSONObj getExtraInfo() const { return _extraInfo ; }
      string dump();

   private:
      BOOLEAN              _isLoadFromFile;
      INT64                _opTotalNum;
      map<UINT16, INT64>   _opTypeNum;
      DPS_LSN_OFFSET       _nextLSN;
      DPS_LSN_OFFSET       _lastLSN;
      UINT32               _nextFileId;
      UINT32               _lastFileId;
      time_t               _lastFileTime;
      time_t               _lastMovedFileTime;
      string               _outputType ;
      UINT64               _serial ;
      UINT64               _summitTime ;  // microSeconds
      BSONObj              _extraInfo ;
   };
}

#endif /* REPLAY_MONITOR_HPP_ */

