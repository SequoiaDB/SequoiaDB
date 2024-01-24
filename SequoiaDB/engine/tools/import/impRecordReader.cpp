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
      _set     = FALSE ;
      _input   = NULL ;
      _scanner = NULL ;
      _buffer  = NULL ;
      _bufferSize  = 0 ;
      _recDelLen   = 0 ;
      _data        = NULL ;
      _dataLength  = 0 ;
      _remainSize  = 0 ;
      _readSize    = 0 ;
      _totalSize   = 0 ;
      _final       = FALSE ;
      _isFull      = FALSE ;
   }

   void RecordReader::reset( CHAR* buffer, INT32 bufferSize,
                             InputStream* input, RecordScanner* scanner,
                             INT32 recordDelimiterLength )
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

   void RecordReader::rotationBuffer( CHAR* buffer, INT32 bufferSize )
   {
      SDB_ASSERT( NULL != buffer, "buffer can't be NULL" ) ;
      SDB_ASSERT( bufferSize > 0, "bufferSize must be greater than 0" ) ;

      if ( 0 < _dataLength )
      {
         ossMemcpy( buffer, _data, _dataLength ) ;
      }

      _buffer     = buffer ;
      _bufferSize = bufferSize ;
      _remainSize = _dataLength ;
      _dataLength = 0 ;
      _isFull     = FALSE ;
   }

   INT32 RecordReader::read( CHAR*& record, INT32& recordLength,
                             BOOLEAN isReuseBuffer )
   {
      INT32 rc = SDB_OK;

   readData:
      if ( 0 >= _dataLength )
      {
         if( _isFull && FALSE == isReuseBuffer )
         {
            rc = SDB_OSS_UP_TO_LIMIT ;
            _dataLength = 0 ;
            goto done ;
         }

         rc = _input->read( _buffer + _remainSize,
                            _bufferSize - _remainSize, _readSize ) ;
         if ( rc )
         {
            if ( SDB_EOF != rc )
            {
               PD_LOG( PDERROR, "Failed to read from InputStream, rc=%d", rc ) ;
               goto error ;
            }

            if ( 0 == _remainSize )
            {
               // real EOF
               goto done;
            }

            // the final record
            _readSize = 0 ;
            _final = TRUE ;
         }

         _totalSize += _readSize ;
         _data = _buffer ;
         _dataLength = _remainSize + _readSize ;
      }

      rc = _scanner->scan( _data, _dataLength, _final, recordLength ) ;
      if ( rc )
      {
         if ( SDB_EOF == rc )
         {
            if ( _dataLength >= _bufferSize )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "The remain data is out of length [0-%d]: %d",
                       _bufferSize, _dataLength ) ;
               goto error ;
            }

            if ( isReuseBuffer )
            {
               ossMemmove( _buffer, _data, _dataLength ) ;
               _remainSize = _dataLength ;
               _dataLength = 0 ;
               goto readData ;
            }

            rc = SDB_OSS_UP_TO_LIMIT ;
            goto done ;
         }

         PD_LOG( PDERROR, "Failed to scan record, rc=%d", rc ) ;
         goto error ;
      }

      record = _data ;

      if ( FORMAT_CSV == _scanner->format() )
      {
         _data += ( recordLength + _recDelLen ) ;
         _dataLength -= ( recordLength + _recDelLen ) ;
      }
      else // JSON
      {
         _data += recordLength ;
         _dataLength -= recordLength ;
      }

      if ( _dataLength <= 0 )
      {
         _remainSize = 0 ;
         _isFull = TRUE ;
      }

      if ( 0 < recordLength && 0 == record[recordLength-1] )
      {
         --recordLength ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
