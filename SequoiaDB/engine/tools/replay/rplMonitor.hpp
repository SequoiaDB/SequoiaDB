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
#include <ctime>
#include <string>
#include <map>

using namespace std;

namespace replay
{
   class Monitor: public SDBObject
   {
   public:
      Monitor();
      ~Monitor();
      void  opCount(UINT16 type);
      INT64 opTypeNum(UINT8 type) const;
      void  setNextLSN(DPS_LSN_OFFSET lsn);
      void  setLastLSN(DPS_LSN_OFFSET lsn);
      void  setNextFileId(UINT32 fileId);
      void  setLastFileId(UINT32 fileId);
      void  setLastFileTime(time_t lastTime);
      void  setLastMovedFileTime(time_t lastTime);
      OSS_INLINE INT64 opTotalNum() const { return _opTotalNum; }
      OSS_INLINE DPS_LSN_OFFSET getNextLSN() const { return _nextLSN; }
      OSS_INLINE DPS_LSN_OFFSET getLastLSN() const { return _lastLSN; }
      OSS_INLINE UINT32 getNextFileId() const { return _nextFileId; }
      OSS_INLINE UINT32 getLastFileId() const { return _lastFileId; }
      OSS_INLINE time_t getLastFileTime() const { return _lastFileTime; }
      OSS_INLINE time_t getLastMovedFileTime() const { return _lastMovedFileTime; }
      string dump();

   private:
      INT64                _opTotalNum;
      map<UINT16, INT64>   _opTypeNum;
      DPS_LSN_OFFSET       _nextLSN;
      DPS_LSN_OFFSET       _lastLSN;
      UINT32               _nextFileId;
      UINT32               _lastFileId;
      time_t               _lastFileTime;
      time_t               _lastMovedFileTime;
   };
}

#endif /* REPLAY_MONITOR_HPP_ */

