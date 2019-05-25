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

   Source File Name = impRecordReader.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordReader.hpp"
#include "ossUtil.h"
#include "pd.hpp"

#define IMP_RECORD_MAX_LENGTH (1024 * 1024 * 16) // 16MB

namespace import
{
   RecordReader::RecordReader()
   {
      _clear();
   }

   RecordReader::~RecordReader()
   {
      _clear();
   }

   void RecordReader::_clear()
   {
      _set = FALSE;
      _input = NULL;
      _scanner = NULL;
      _buffer = NULL;
      _bufferSize = 0;
      _recDelLen = 0;
      _data = NULL;
      _dataLength = 0;
      _remainSize = 0;
      _readSize = 0;
      _totalSize = 0;
      _final = FALSE;
   }

   void RecordReader::reset(CHAR* buffer, INT32 bufferSize,
                            InputStream* input, RecordScanner* scanner,
                            INT32 recordDelimiterLength)
   {
      _clear();

      SDB_ASSERT(NULL != buffer, "buffer can't be NULL");
      SDB_ASSERT(bufferSize > 0, "bufferSize must be greater than 0");
      SDB_ASSERT(NULL != input, "input can't be NULL");
      SDB_ASSERT(NULL != scanner, "scanner can't be NULL");
      SDB_ASSERT(recordDelimiterLength > 0,
                 "recordDelimiterLength must be greater than 0");

      _input = input;
      _scanner = scanner;
      _buffer = buffer;
      _bufferSize = bufferSize;
      _recDelLen = recordDelimiterLength;
   }

   INT32 RecordReader::read(CHAR*& record, INT32& recordLength)
   {
      INT32 rc = SDB_OK;

   readData:
      if (_dataLength <= 0)
      {
         rc = _input->read(_buffer + _remainSize,
                           _bufferSize - _remainSize, _readSize);
         if (SDB_OK != rc)
         {
            if (SDB_EOF != rc)
            {
               PD_LOG(PDERROR, "failed to read from InputStream, rc=%d", rc);
               goto error;
            }

            if (0 == _remainSize)
            {
               goto done;
            }

            _readSize = 0;
            _final = TRUE;
         }

         _totalSize += _readSize;
         _data = _buffer;
         _dataLength = _remainSize + _readSize;
      }

      rc = _scanner->scan(_data, _dataLength, _final, recordLength);
      if (SDB_OK != rc)
      {
         if (SDB_EOF == rc)
         {
            if (_dataLength > IMP_RECORD_MAX_LENGTH)
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "the remain data is out of length [0-%d]: %d",
                               IMP_RECORD_MAX_LENGTH, _dataLength);
               goto error;
            }
            ossMemmove(_buffer, _data, _dataLength);
            _remainSize = _dataLength;
            _dataLength = 0;
            goto readData;
         }

         PD_LOG(PDERROR, "failed to scan record, rc=%d", rc);
         goto error;
      }

      record = _data;

      if (FORMAT_CSV == _scanner->format())
      {
         _data += (recordLength + _recDelLen);
         _dataLength -= (recordLength + _recDelLen);
      }
      else // JSON
      {
         _data += recordLength;
         _dataLength -= recordLength;
      }

      if (_dataLength <= 0)
      {
         _remainSize = 0;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
