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

   Source File Name = impInputStream.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impInputStream.hpp"
#include "ossUtil.h"
#include "ossProc.hpp"
#include <sstream>
#include <boost/program_options/parsers.hpp>

namespace import
{
   INT32 InputStream::createInstance(INPUT_TYPE inputType,
                                     const string& input,
                                     const Options& options,
                                     InputStream*& istream)
   {
      InputStream* upstream = NULL;
      INT32 rc = SDB_OK;

      if (INPUT_FILE == inputType)
      {
         SDB_ASSERT(!input.empty(), "input can't be empty");

         FileInputStream* fileStream =
            SDB_OSS_NEW FileInputStream(input);
         if (NULL == fileStream)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to create FileInputStream object, rc=%d",
                   rc);
            goto error;
         }

         rc = fileStream->init();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init FileInputStream, rc=%d", rc);
            SDB_OSS_DEL(fileStream);
            goto error;
         }

         upstream = fileStream;
      }
      else if (INPUT_STDIN == inputType)
      {
         StdinInputStream* stdinStream = SDB_OSS_NEW StdinInputStream();
         if (NULL == stdinStream)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to create StdinInputStream object, rc=%d",
                   rc);
            goto error;
         }

         upstream = stdinStream;
      }
      else if (INPUT_EXEC == inputType)
      {
         SubProcessInputStream* subProcessStream = SDB_OSS_NEW
            SubProcessInputStream(options.exec());
         if (NULL == subProcessStream)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR,
                   "failed to create SubProcessInputStream object, rc=%d", rc);
            goto error;
         }

         rc = subProcessStream->init();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init SubProcessInputStream, rc=%d", rc);
            SDB_OSS_DEL(subProcessStream);
            goto error;
         }

         upstream = subProcessStream;
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid input type");
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid input type, rc=%d", rc);
         goto error;
      }

      istream = SDB_OSS_NEW UTF8InputStream(upstream, TRUE);
      if (NULL == istream)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to create FileInputStream object, rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(upstream);
      goto done;
   }

   void InputStream::releaseInstance(InputStream* istream)
   {
      SDB_ASSERT(NULL != istream, "istream can't be NULL");

      SAFE_OSS_DELETE(istream);
   }

   FileInputStream::FileInputStream(const string& fileName)
   : _fileName(fileName)
   {
   }

   FileInputStream::~FileInputStream()
   {
      ossClose(_file);
   }

   INT32 FileInputStream::init()
   {
      INT32 rc = SDB_OK;

      rc = ossOpen(_fileName.c_str(),
                   OSS_READONLY | OSS_SHAREREAD,
                   OSS_DEFAULTFILE,
                   _file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file %s, rc=%d",
                _fileName.c_str(), rc);
      }

      return rc;
   }

   INT32 FileInputStream::read(CHAR* buf, INT64 bufSize, INT64& readSize)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != buf, "buf can't be NULL");
      SDB_ASSERT(bufSize > 0, "bufSize must be greater than 0");

      rc = ossReadN(&_file, bufSize, buf, readSize);
      if (SDB_OK != rc && SDB_EOF != rc)
      {
         PD_LOG(PDERROR, "failed to read from file %s, rc=%d",
                _fileName.c_str(), rc);
      }

      return rc;
   }

   StdinInputStream::StdinInputStream()
   {
   }

   StdinInputStream::~StdinInputStream()
   {
   }

   INT32 StdinInputStream::read(CHAR* buf, INT64 bufSize, INT64& readSize)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != buf, "buf can't be NULL");
      SDB_ASSERT(bufSize > 0, "bufSize must be greater than 0");

      readSize = fread(buf, 1, bufSize, stdin);
      if (readSize <= 0)
      {
         if (feof(stdin))
         {
            rc = SDB_EOF;
         }
         else if (ferror(stdin))
         {
            PD_LOG(PDERROR, "failed to read from stdin, errno=%d",
                   ossGetLastError());
            rc = SDB_IO;
         }
      }

      return rc;
   }

   UTF8InputStream::UTF8InputStream(InputStream* inputStream,
                                    BOOLEAN managed)
   : _upstream(inputStream),
     _managed(managed)
   {
      SDB_ASSERT(NULL != inputStream, "inputStream can't be NULL");
      _first = TRUE;
   }

   UTF8InputStream::~UTF8InputStream()
   {
      if (_managed)
      {
         SDB_OSS_DEL(_upstream);
      }
   }

   INT32 UTF8InputStream::read(CHAR* buf, INT64 bufSize, INT64& readSize)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != buf, "buf can't be NULL");
      SDB_ASSERT(bufSize > 0, "bufSize must be greater than 0");

      if (!_first)
      {
         rc = _upstream->read(buf, bufSize, readSize);
      }
      else
      {
         rc = _firstRead(buf, bufSize, readSize);
      }

      return rc;
   }

   INT32 UTF8InputStream::_firstRead(CHAR* buf, INT64 bufSize, INT64& readSize)
   {
      static const UINT8 UTF8_BOM[] = {0xEF, 0xBB, 0xBF};
      static const INT32 UTF8_BOM_SIZE =
         (INT32)(sizeof(UTF8_BOM)/sizeof(UTF8_BOM[0]));
      INT32 rc = SDB_OK;
      INT64 size = 0;

      SDB_ASSERT(_first, "must be first");

      rc = _upstream->read(buf, UTF8_BOM_SIZE, size);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read from upstream, rc=%d", rc);
         goto error;
      }

      if (size < UTF8_BOM_SIZE || 0 != ossMemcmp(UTF8_BOM, buf, UTF8_BOM_SIZE))
      {
         // not BOM, continue read
         buf += UTF8_BOM_SIZE;
         bufSize -= UTF8_BOM_SIZE;
         readSize = size;
      }
      else
      {
         // has BOM, override it
         readSize = 0;
      }

      size = 0;
      rc = _upstream->read(buf, bufSize, size);
      if (SDB_OK != rc)
      {
         if (SDB_EOF != rc)
         {
            PD_LOG(PDERROR, "failed to read from upstream, rc=%d", rc);
            goto error;
         }

         rc = SDB_OK;
      }

      readSize += size;
      _first = FALSE;
      goto done;

   done:
      return rc;
   error:
      goto done;
   }

   SubProcessInputStream::SubProcessInputStream(const string& subProcessCmd)
   : _subProcessCmd(subProcessCmd)
   {
   }

   SubProcessInputStream::~SubProcessInputStream()
   {
      if (OSS_INVALID_PID != _subPid)
      {
         ossCloseNamedPipe(_pipe);
         _subPid = OSS_INVALID_PID;
      }
   }

   INT32 SubProcessInputStream::init()
   {
      ossResultCode result;
      INT32 rc = SDB_OK;
      CHAR* arguments = NULL;
      INT32 flag = OSS_EXEC_NORESIZEARGV | OSS_EXEC_NODETACHED;
      const CHAR* cmd = _subProcessCmd.c_str();

      {
         std::list<const CHAR*> argvList;
         INT32 argLen = 0;

#if defined(_LINUX)
         std::vector<std::string> vecArgs;

         vecArgs = boost::program_options::split_unix(cmd);
         for (UINT32 i = 0; i < vecArgs.size(); ++i)
         {
            argvList.push_back(vecArgs[ i ].c_str());
         }
#else // _WINDOWS
         argvList.push_back(cmd);
#endif // _LINUX

         rc = ossBuildArguments(&arguments, argLen, argvList);
         if (SDB_OK != rc)
         {
            PD_LOG( PDERROR, "failed to build arguments, rc=%d", rc);
            goto error;
         }
      }

      rc = ossExec(arguments, arguments,
                   NULL, flag, _subPid, result, NULL, &_pipe);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to execute %s, rc=%d", cmd, rc);
         goto error;
      }
      else
      {
         PD_LOG(PDEVENT, "execute %s successfully", cmd);
      }

   done:
      SAFE_OSS_FREE(arguments);
      return rc;
   error:
      goto done;
   }

   INT32 SubProcessInputStream::read(CHAR* buf, INT64 bufSize, INT64& readSize)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != buf, "buf can't be NULL");
      SDB_ASSERT(bufSize > 0, "bufSize must be greater than 0");

      rc = ossReadNamedPipe(_pipe, buf, bufSize, &readSize);
      if (SDB_OK != rc && SDB_EOF != rc)
      {
         PD_LOG(PDERROR, "failed to read from sub process %s, rc=%d",
                _subProcessCmd.c_str(), rc);
      }

      return rc;
   }
}

