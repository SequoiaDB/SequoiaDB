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

   Source File Name = impLogFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impLogFile.hpp"
#include "ossUtil.h"
#include "pd.hpp"
#include <string>
#include <sstream>
#include <iostream>

namespace import
{
   LogFile::LogFile()
   {
      _isOpened = FALSE;
      _threadSafe = FALSE;
      _inited = FALSE;
      ossMutexInit(&_fileLock);
   }

   LogFile::~LogFile()
   {
      if (_isOpened)
      {
         ossClose(_file);
         _isOpened = FALSE;
      }

      ossMutexDestroy(&_fileLock);
   }

   INT32 LogFile::init(const string& fileName, BOOLEAN threadSafe)
   {
      _fileName = fileName;
      _threadSafe = threadSafe;
      _inited = TRUE;
      return SDB_OK;
   }

   INT32 LogFile::_open()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_inited, "must inited");
      SDB_ASSERT(!_isOpened, "can't open again");

      rc = ossOpen(_fileName.c_str(),
                   OSS_REPLACE | OSS_WRITEONLY | OSS_EXCLUSIVE,
                   OSS_RU | OSS_WU | OSS_RG,
                   _file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file %s, rc=%d",
                _fileName.c_str(), rc);
      }
      else
      {
         _isOpened = TRUE;
      }

      return rc;
   }

   INT32 LogFile::write(const CHAR* buf, INT32 length)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != buf, "buf can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      _beginWrite();

      if (!_isOpened)
      {
         rc = _open();
         if (SDB_OK != rc)
         {
            string s(buf, length);
            PD_LOG(PDERROR, "failed to open file %s, rc=%d",
                   _fileName.c_str(), rc, s.c_str());
            std::cout << "failed to open file " << _fileName << std::endl;
            goto error;
         }
      }

      rc = ossWriteN(&_file, buf, length);
      if (SDB_OK != rc)
      {
         string s(buf, length);
         PD_LOG(PDERROR, "failed to write to file %s, rc=%d, text:%s",
                _fileName.c_str(), rc, s.c_str());
      }
      else
      {
         rc = ossWriteN(&_file, "\n", 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write to file %s, rc=%d",
                   _fileName.c_str(), rc);
         }
      }

   done:
      _endWrite();
      return rc;
   error:
      goto done;
   }

   INT32 LogFile::write(const bson* obj)
   {
      INT32 rc = SDB_OK;
      INT32 len;
      CHAR* buffer = NULL;

      SDB_ASSERT(NULL != obj, "obj can't be NULL");

      len = bson_sprint_length(obj);
      if (0 == len)
      {
         goto done;
      }

      buffer = (CHAR*)SDB_OSS_MALLOC(len);
      if (NULL == buffer)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to malloc for bson sprint, size=%d", len);
         goto error;
      }

      if (!bson_sprint(buffer, len, obj))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "failed to bson sprint");
         goto error;
      }

      rc = write(buffer, ossStrlen(buffer));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write bson, rc=%d", rc);
         goto error;
      }

   done:
      SAFE_OSS_FREE(buffer);
      return rc;
   error:
      goto done;
   }

   string makeRecordLogFileName(const string& csname, 
                                const string& clname,
                                const string& custom)
   {
      std::stringstream ss;

      ss << csname << "_"
         << clname << "_"
         << custom << "_"
         << ossGetCurrentProcessID()
         << ".rec";

      return ss.str();
   }
}

