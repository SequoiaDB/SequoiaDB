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

   Source File Name = impRecordReader.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_READER_HPP_
#define IMP_RECORD_READER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impInputStream.hpp"
#include "impRecordScanner.hpp"

namespace import
{
   class RecordReader: public SDBObject
   {
   public:
      RecordReader();
      ~RecordReader();
      void reset(CHAR* buffer, INT32 bufferSize,
                 InputStream* input, RecordScanner* scanner,
                 INT32 recordDelimiterLength);
      INT32 read(CHAR*& record, INT32& recordLength);

   private:
      void _clear();

   private:
      BOOLEAN        _set;
      InputStream*   _input;
      RecordScanner* _scanner;
      CHAR*          _buffer;
      INT32          _bufferSize;
      INT32          _recDelLen;

      CHAR*          _data;
      INT32          _dataLength;
      INT32          _remainSize;
      INT64          _readSize;
      INT64          _totalSize;
      BOOLEAN        _final;
   };
}

#endif /* IMP_RECORD_READER_HPP_ */
