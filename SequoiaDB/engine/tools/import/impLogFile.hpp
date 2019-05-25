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

   Source File Name = impLogFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_LOG_FILE_HPP_
#define IMP_LOG_FILE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "oss.h"
#include "../client/bson/bson.h"
#include <string>

using namespace std;

namespace import
{
   class LogFile: public SDBObject
   {
   public:
      LogFile();
      ~LogFile();
      INT32 init(const string& fileName, BOOLEAN threadSafe = FALSE);
      INT32 write(const CHAR* buf, INT32 length);
      INT32 write(const bson* obj);

      inline const string& fileName() const { return _fileName; }

   private:
      INT32 _open();
      inline void _beginWrite()
      {
         if (_threadSafe)
         {
            ossMutexLock(&_fileLock);
         }
      }

      inline void _endWrite()
      {
         if (_threadSafe)
         {
            ossMutexUnlock(&_fileLock);
         }
      }

   private:
      string      _fileName;
      OSSFILE     _file;
      BOOLEAN     _isOpened;
      BOOLEAN     _threadSafe;
      ossMutex    _fileLock;
      BOOLEAN     _inited;
   };

   string makeRecordLogFileName(const string& csname, 
                                const string& clname,
                                const string& custom);
}

#endif /* IMP_LOG_FILE_HPP_ */
