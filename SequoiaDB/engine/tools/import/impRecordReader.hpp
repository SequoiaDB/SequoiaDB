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
   class RecordReader : public SDBObject
   {
   public:
      RecordReader() ;

      ~RecordReader() ;

      void reset( CHAR* buffer, INT32 bufferSize,
                  InputStream* input, RecordScanner* scanner,
                  INT32 recordDelimiterLength ) ;

      void rotationBuffer( CHAR* buffer, INT32 bufferSize ) ;

      INT32 read( CHAR*& record, INT32& recordLength,
                  BOOLEAN isReuseBuffer = TRUE ) ;

   private:
      void _clear() ;

   private:
      BOOLEAN        _set;
      InputStream*   _input;
      RecordScanner* _scanner;
      CHAR*          _buffer;
      INT32          _bufferSize;
      INT32          _recDelLen;

      // runtime
      CHAR*          _data;
      INT32          _dataLength;
      INT32          _remainSize;
      INT64          _readSize;
      INT64          _totalSize;
      BOOLEAN        _final;
      BOOLEAN        _isFull ;
   };
}

#endif /* IMP_RECORD_READER_HPP_ */
