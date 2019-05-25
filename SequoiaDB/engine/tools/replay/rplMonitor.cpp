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

   Source File Name = rplMonitor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplMonitor.hpp"
#include "rplUtil.hpp"
#include "dpsLogFile.hpp"
#include <sstream>

using namespace engine;

namespace replay
{
   Monitor::Monitor()
   {
      _opTotalNum = 0;
      _nextLSN = DPS_INVALID_LSN_OFFSET;
      _lastLSN = DPS_INVALID_LSN_OFFSET;
      _nextFileId = DPS_INVALID_LOG_FILE_ID;
      _lastFileId = DPS_INVALID_LOG_FILE_ID;
      _lastFileTime = 0;
      _lastMovedFileTime = 0;
   }

   Monitor::~Monitor()
   {
   }

   void Monitor::opCount(UINT16 type)
   {
      map<UINT16, INT64>::iterator it = _opTypeNum.find(type);
      if (_opTypeNum.end() == it)
      {
         _opTypeNum.insert(make_pair(type, 1));
      }
      else
      {
         it->second++;
      }

      _opTotalNum++;
   }

   INT64 Monitor::opTypeNum(UINT8 type) const
   {
      map<UINT16, INT64>::const_iterator it = _opTypeNum.find(type);
      if (_opTypeNum.end() == it)
      {
         return 0;
      }
      else
      {
         return it->second;
      }
   }

   void Monitor::setNextLSN(DPS_LSN_OFFSET lsn)
   {
      _nextLSN = lsn;
   }

   void Monitor::setLastLSN(DPS_LSN_OFFSET lsn)
   {
      _lastLSN = lsn;
   }

   void Monitor::setNextFileId(UINT32 fileId)
   {
      _nextFileId = fileId;
   }

   void Monitor::setLastFileId(UINT32 fileId)
   {
      _lastFileId = fileId;
   }

   void Monitor::setLastFileTime(time_t lastTime)
   {
      _lastFileTime = lastTime;
   }

   void Monitor::setLastMovedFileTime(time_t lastTime)
   {
      _lastMovedFileTime = lastTime;
   }

   string Monitor::dump()
   {
      stringstream ss;

      if (DPS_INVALID_LSN_OFFSET != _nextLSN)
      {
         ss << "Next LSN: " << _nextLSN << std::endl;
      }

      if (DPS_INVALID_LOG_FILE_ID != _nextFileId)
      {
         ss << "Next FileId: " << _nextFileId << std::endl;
      }

      if (DPS_INVALID_LSN_OFFSET != _lastLSN)
      {
         ss << "Last LSN: " << _lastLSN << std::endl;
      }

      if (DPS_INVALID_LOG_FILE_ID != _lastFileId)
      {
         ss << "Last FileId: " << _lastFileId << std::endl;
      }

      if (0 != _lastMovedFileTime)
      {
         ss << "Last Moved File Time: " << _lastMovedFileTime << std::endl;
      }

      ss << "Total OP num: " << _opTotalNum << std::endl;

      map<UINT16, INT64>::const_iterator it = _opTypeNum.begin();
      for (; it != _opTypeNum.end(); it++)
      {
         UINT8 type = it->first;

         ss << "OP " << getOPName(type) << " num: " << it->second << std::endl;
      }

      return ss.str();
   }
}

