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

   Source File Name = impParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/02/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "impScanner.hpp"
#include "impInputStream.hpp"
#include "impRecordScanner.hpp"
#include "impRecordReader.hpp"
#include "impRecordParser.hpp"
#include "impCSVRecordParser.hpp"
#include "impUtil.hpp"
#include "../util/text.h"
#include "pd.hpp"
#include <sstream>
#include <iostream>

namespace import
{
   #define IMP_SCANNER_WAIT_TIMEOUT (1000)

   inline BOOLEAN _waitBufferBlock( Scanner* self, BOOLEAN useFirstBlock,
                                    impBufferBlock*& block )
   {
      BOOLEAN result = TRUE ;
      impBufferBlock *bufferBlock = useFirstBlock ?
            &( self->_bufferBlock1 ) : &( self->_bufferBlock2 ) ;

      while( bufferBlock->lock_w( IMP_SCANNER_WAIT_TIMEOUT ) )
      {
         if( self->_stopped )
         {
            result = FALSE ;
            goto done ;
         }
      }

      bufferBlock->release_w() ;

      block = bufferBlock ;

   done:
      return result ;
   }

   void _scannerRoutine( WorkerArgs* args )
   {
      INT32 rc = SDB_OK ;
      INT32 fileId     = 0 ;
      INT32 fieldsLen  = 0 ;
      INT32 bufferSize = 0 ;
      BOOLEAN isFirst  = TRUE ;
      BOOLEAN useFirstBlock = FALSE ;

      SDB_ASSERT( NULL != args, "arg can't be NULL" ) ;

      Scanner* self        = (Scanner*)args ;
      CHAR* buffer         = NULL ;
      CHAR* pFields        = NULL ;
      InputStream* input   = NULL ;
      impBufferBlock* bufferBlock = NULL ;

      SDB_ASSERT( NULL != self, "self can't be NULL" ) ;

      const Options* options       = self->_options ;
      DataQueue* dataQueue         = self->_dataQueue ;
      RecordParser* parser         = self->_parser ;

      SDB_ASSERT( NULL != options, "options can't be NULL" ) ;
      SDB_ASSERT( NULL != dataQueue, "dataQueue can't be NULL" ) ;

      INT64 scanNum = 0 ;

      RecordScanner scanner( options->recordDelimiter(),
                             options->stringDelimiter(),
                             options->inputFormat(),
                             options->linePriority() ) ;

      RecordReader recordReader ;
      string inputString ;

      {
         CHAR* str = "Scanner started..." ;

         PD_LOG( PDEVENT, "%s", str ) ;

         if ( options->verbose() )
         {
            std::cout << str << std::endl ;
         }
      }

   begin:
      if ( INPUT_FILE == options->inputType() )
      {
         if ( NULL != input )
         {
            InputStream::releaseInstance( input ) ;
            input = NULL;
         }

         // maybe multiple files
         if ( fileId < (INT32)options->files().size() )
         {
            useFirstBlock = !useFirstBlock ;

            scanNum = 0 ;
            isFirst = TRUE ;
            inputString = options->files()[fileId] ;
            ++fileId ;
            PD_LOG( PDINFO, "Read data from file [%s]", inputString.c_str() ) ;

            if ( options->verbose() )
            {
               std::cout << "Read data from file [" 
                         << inputString << "]"
                         << std::endl ;
            }
         }
         else
         {
            goto done ;
         }
      }

      rc = InputStream::createInstance( options->inputType(),
                                        inputString, *options, input ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create InputStream object,"
                 "rc=%d, INPUT_TYPE=%d", rc, options->inputType() ) ;
         goto error ;
      }

      if ( !_waitBufferBlock( self, useFirstBlock, bufferBlock ) )
      {
         goto done ;
      }

      buffer = bufferBlock->getBuffer() ;
      bufferSize = bufferBlock->getSize() ;

      recordReader.reset( buffer, bufferSize,
                          input, &scanner,
                          options->recordDelimiter().length() ) ;

      while( !self->_stopped )
      {
         INT32 recordLength = 0 ;
         CHAR* record       = NULL ;
         RecordData recordData ;

         rc = recordReader.read( record, recordLength, FALSE ) ;
         if ( SDB_OSS_UP_TO_LIMIT == rc )
         {
            //buffer is full, change another buff
            rc = SDB_OK ;
            useFirstBlock = !useFirstBlock ;

            if ( !_waitBufferBlock( self, useFirstBlock, bufferBlock ) )
            {
               goto done ;
            }

            buffer = bufferBlock->getBuffer() ;
            bufferSize = bufferBlock->getSize() ;

            recordReader.rotationBuffer( buffer, bufferSize ) ;

            continue ;
         }
         else if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;

            if ( INPUT_FILE == options->inputType() )
            {
               std::stringstream ss;

               ss << "Scan records of file ["
                  << inputString << "]: "
                  << scanNum
                  << std::endl;

               PD_LOG( PDINFO, "%s", ss.str().c_str() ) ;
               // maybe multiple files
               goto begin ;
            }

            break ;
         }
         else if ( rc )
         {
            std::cerr << "ERROR: Read record error!" << std::endl ;
            PD_LOG( PDERROR, "Failed to read record" ) ;
            goto error ;
         }

         if ( 0 == recordLength )
         {
            if ( isFirst && FORMAT_CSV == options->inputFormat() &&
                 options->hasHeaderLine() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "The headerline is empty" ) ;
               std::cerr << "ERROR: The headerline is empty" << std::endl ;
               goto error ;
            }

            continue ;
         }

         if ( !options->force() && !isValidUTF8WSize( record, recordLength ) )
         {
            rc = SDB_MIG_DATA_NON_UTF ;
            PD_LOG( PDERROR, "It is not utf-8 file, rc=%d", rc ) ;
            if ( options->errorStop() )
            {
               goto error ;
            }
         }

         if ( isFirst )
         {
            isFirst = FALSE;
            if ( FORMAT_CSV == options->inputFormat() &&
                 options->hasHeaderLine() )
            {
               SDB_ASSERT( NULL != parser, "parser can't be NULL" ) ;

               CSVRecordParser* csvParser = NULL ;
               string fields = string( record, recordLength ) ;

               if ( !options->fields().empty() )
               {
                  // fields is defined, so ignore the headerline
                  continue;
               }

               fieldsLen = recordLength ;

               PD_LOG( PDINFO, "File: %s, fields: %s", inputString.c_str(),
                       fields.c_str() ) ;
               if ( options->verbose() )
               {
                  std::cout << "File: " << inputString <<
                               ", fields: " << fields << std::endl;
               }

               //Check file first line field format
               csvParser = (CSVRecordParser*)parser ;
               csvParser->reset() ;
               rc = csvParser->parseFields( record, recordLength, TRUE ) ;
               if ( rc )
               {
                  std::cerr << "Failed to parse fields" << std::endl;
                  PD_LOG( PDERROR, "Failed to parse fields, rc = %d", rc ) ;
                  goto error ;
               }

               if ( options->verbose() )
               {
                  csvParser->printFieldsDef() ;
               }

               pFields = (CHAR*)SDB_OSS_MALLOC( fieldsLen + 1 ) ;
               if ( NULL == pFields )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Failed to malloc record buffer, size=%d",
                          fieldsLen + 1 ) ;
                  goto error ;
               }
               ossMemcpy( pFields, record, fieldsLen ) ;
               pFields[fieldsLen] = '\0' ;

               self->_fields.push_back( pFields ) ;

               continue;
            }
         }

         if ( FORMAT_CSV == options->inputFormat() )
         {
            recordData.fields    = pFields ;
            recordData.fieldsLen = fieldsLen ;
         }

         recordData.fileId  = fileId ;
         recordData.data    = record ;
         recordData.dataLen = recordLength ;
         recordData.bufferBlock = bufferBlock ;

         recordData.bufferBlock->lock_r() ;

         dataQueue->push( recordData ) ;

         ++scanNum ;
      }

   done:
      self->_stopped = TRUE ;

      //Stop parser signal
      for ( INT32 i = 0; i < self->_workerNum; ++i )
      {
         RecordData recordData ;

         dataQueue->wait_and_push( recordData ) ;
      }

      if ( NULL != input )
      {
         InputStream::releaseInstance( input ) ;
         input = NULL ;
      }

      {
         CHAR* str= "Scanner stopped";

         PD_LOG( PDEVENT, "%s, rc=%d", str, rc ) ;

         if ( options->verbose() )
         {
            std::cout << str << std::endl ;
         }
      }

      return ;
   error:
      goto done ;
   }

   Scanner::Scanner() : _workerNum( 0 ),
                        _inited( FALSE ),
                        _stopped( TRUE ),
                        _options( NULL ),
                        _dataQueue( NULL ),
                        _worker( NULL ),
                        _parser( NULL )
   {
   }

   Scanner::~Scanner()
   {
      vector<CHAR*>::iterator i = _fields.begin() ;

      for( ; i != _fields.end(); ++i )
      {
         CHAR* fields = *i ;

         SAFE_OSS_FREE( fields ) ;
      }

      if ( NULL != _parser )
      {
         RecordParser::releaseInstance( _parser ) ;
         _parser = NULL ;
      }

      SAFE_OSS_DELETE( _worker ) ;
   }

   INT32 Scanner::initParser( Options* options )
   {
      INT32 rc = SDB_OK ;

      if ( FORMAT_CSV == options->inputFormat() )
      {
         rc = RecordParser::createInstance( options->inputFormat(),
                                            *options, _parser ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to create RecordParser object,"
                    "rc=%d, INPUT_FORMAT=%d", rc, options->inputFormat() ) ;
            goto error ;
         }

         if ( !options->fields().empty() )
         {
            INT32 len = options->fields().length() ;
            const CHAR* str = options->fields().c_str() ;
            CSVRecordParser* csvParser = NULL ;

            PD_LOG( PDINFO, "Fields: %s", options->fields().c_str() ) ;

            if ( options->verbose() )
            {
               std::cout << "Fields: " << options->fields() << std::endl ;
            }

            csvParser = (CSVRecordParser*)_parser ;

            rc = csvParser->parseFields( str, len, FALSE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to parse fields, rc=%d", rc ) ;
               goto error ;
            }

            if ( options->verbose() )
            {
               csvParser->printFieldsDef() ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Scanner::init( Options* options, DataQueue* dataQueue,
                        INT32 workerNum )
   {
      INT32 rc = SDB_OK ;
      INT32 bufferSize = 0 ;

      SDB_ASSERT( NULL != options, "options can't be NULL" ) ;
      SDB_ASSERT( NULL != dataQueue, "dataQueue can't be NULL" ) ;

      if ( FORMAT_CSV == options->inputFormat() )
      {
         SDB_ASSERT( NULL != _parser, "_parser can't be NULL" ) ;
      }

      bufferSize = options->bufferSize() ;

      rc = _bufferBlock1.init( bufferSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init buffer page, rc=%d", rc ) ;
         goto error ;
      }

      rc = _bufferBlock2.init( bufferSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init buffer page, rc=%d", rc ) ;
         goto error ;
      }

      _options   = options ;
      _dataQueue = dataQueue ;
      _workerNum = workerNum ;

      _worker = SDB_OSS_NEW Worker( _scannerRoutine, this ) ;
      if ( NULL == _worker )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to create scanner Worker object" ) ;
         goto error ;
      }

      _inited = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Scanner::start()
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( _inited, "Must be inited" ) ;

      _stopped = FALSE ;

      rc = _worker->start() ;
      if ( rc )
      {
         _stopped = TRUE ;
         PD_LOG( PDERROR, "Failed to start scanner" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Scanner::stop()
   {
      INT32 rc = SDB_OK ;

      if( _inited )
      {
         SDB_ASSERT( NULL != _worker, "_worker can't be NULL" ) ;

         _stopped = TRUE ;

         rc = _worker->waitStop() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to wait the scanner stop" ) ;
         }

         SAFE_OSS_DELETE( _worker ) ;
      }

      return rc ;
   }
}
