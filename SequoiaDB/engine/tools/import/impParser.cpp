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
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impParser.hpp"
#include "impInputStream.hpp"
#include "impRecordScanner.hpp"
#include "impRecordReader.hpp"
#include "impRecordParser.hpp"
#include "impCSVRecordParser.hpp"
#include "../util/text.h"
#include "pd.hpp"
#include <sstream>
#include <iostream>

namespace import
{
   struct ParserArgs: public WorkerArgs
   {
      INT32   id ;
      Parser* self ;
   } ;

   static INT32 _copyCsvFieldsAndParse( CSVRecordParser* csvParser,
                                        CHAR*& fields, INT32& fieldsLength,
                                        const CHAR* src, INT32 srcLength,
                                        BOOLEAN isHeaderline )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != csvParser, "csvParser can't be NULL" ) ;
      SDB_ASSERT( NULL != src, "src can't be NULL" ) ;

      if ( fieldsLength < srcLength )
      {
         INT32 size = srcLength + 1 ;
         CHAR* tmp = fields ;

         fieldsLength = srcLength ;

         fields = (CHAR*)SDB_OSS_REALLOC( tmp, size ) ;
         if ( NULL == fields )
         {
            SAFE_OSS_FREE( tmp ) ;
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to malloc fields buffer, size=%d", size ) ;
            goto error ;
         }
      }

      SDB_ASSERT( NULL != fields, "fields can't be NULL" ) ;

      ossMemcpy( fields, src, srcLength ) ;
      fields[srcLength] = '\0' ;

      csvParser->reset() ;

      rc = csvParser->parseFields( fields, fieldsLength, isHeaderline ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse fields, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _parserRoutine( WorkerArgs* args )
   {
      INT32 rc = SDB_OK ;
      INT32 fileId       = 0 ;
      INT32 fieldsLength = 0 ;
      ParserArgs* parArgs = (ParserArgs*)args ;
      Parser* self = parArgs->self ;

      SDB_ASSERT( NULL != args, "arg can't be NULL" ) ;

      RecordParser* parser   = NULL ;
      const Options* options = self->_options ;
      DataQueue* dataQueue   = self->_dataQueue ;
      Packer* packer         = self->_packer ;
      LogFile* logFile       = &(self->_logFile) ;
      CHAR* fields           = NULL ;

      SDB_ASSERT( NULL != options, "options can't be NULL" ) ;
      SDB_ASSERT( NULL != logFile, "logFile can't be NULL" ) ;
      SDB_ASSERT( NULL != dataQueue, "dataQueue can't be NULL" ) ;
      SDB_ASSERT( NULL != packer, "packer can't be NULL" ) ;

      string inputString ;
      RecordData recordData ;
      bson obj ;

      bson_init( &obj ) ;

      {
         stringstream ss ;

         ss << "Parser [" << parArgs->id << "] started..." ;

         PD_LOG( PDEVENT, "%s", ss.str().c_str() ) ;

         if ( options->verbose() )
         {
            std::cout << ss.str() << std::endl ;
         }
      }

      rc = RecordParser::createInstance( options->inputFormat(),
                                         *options, parser ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create RecordParser object,"
                 "rc=%d, INPUT_FORMAT=%d", rc, options->inputFormat() ) ;
         goto error ;
      }

      if ( FORMAT_CSV == options->inputFormat() && !options->fields().empty() )
      {
         rc = _copyCsvFieldsAndParse( (CSVRecordParser*)parser,
                                      fields, fieldsLength,
                                      options->fields().c_str(),
                                      options->fields().length(),
                                      FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      while( !self->_stopped )
      {
         while( !dataQueue->pop( parArgs->id, recordData ) )
         {
            if( self->_stopped )
            {
               goto done ;
            }

            ossSleep( IMP_QUEUE_SLEEPTIME ) ;
         }

         if ( NULL == recordData.data || 0 == recordData.dataLen )
         {
            // stop signal
            break;
         }

         if ( fileId != recordData.fileId &&
              options->inputFormat() == FORMAT_CSV &&
              options->hasHeaderLine() &&
              options->fields().empty() )
         {
            // fields is not defined, so use file header line
            rc = _copyCsvFieldsAndParse( (CSVRecordParser*)parser,
                                         fields, fieldsLength,
                                         recordData.fields,
                                         recordData.fieldsLen,
                                         TRUE ) ;
            if ( rc )
            {
               goto error ;
            }

            fileId = recordData.fileId ;
         }

         rc = parser->parseRecord( recordData.data, recordData.dataLen, obj ) ;
         if ( rc )
         {
            bson_init_by_reset( &obj ) ;

            if ( SDB_DMS_EOC != rc )
            {
               self->_failedNum.add( 1 ) ;

               INT32 ret = logFile->write( recordData.data,
                                           recordData.dataLen ) ;
               if ( ret )
               {
                  PD_LOG( PDERROR, "Failed to log write record, rc=%d", ret ) ;
               }

               recordData.bufferBlock->release_r() ;

               PD_LOG( PDERROR, "Failed to parse record, rc=%d", rc ) ;

               if ( options->errorStop() )
               {
                  goto error ;
               }
            }
            else
            {
               recordData.bufferBlock->release_r() ;
            }

            continue ;
         }

         recordData.bufferBlock->release_r() ;

         rc = packer->packing( &obj ) ;

         bson_init_by_reset( &obj ) ;

         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to packing record, rc=%d", rc ) ;

            if ( options->errorStop() )
            {
               goto error ;
            }
         }

         self->_parsedNum.add( 1 ) ;
      }

   done:
      bson_destroy( &obj ) ;

      if ( NULL != parser )
      {
         RecordParser::releaseInstance( parser ) ;
         parser = NULL ;
      }

      {
         stringstream ss ;

         ss << "Parser [" << parArgs->id << "] stopped" ;

         PD_LOG( PDEVENT, "%s, rc=%d", ss.str().c_str(), rc ) ;

         if ( options->verbose() )
         {
            std::cout << ss.str() << std::endl ;
         }
      }

      SAFE_OSS_FREE( fields ) ;

      self->_livingNum.dec() ;

      return ;
   error:
      self->_stopped = TRUE ;
      goto done ;
   }

   Parser::Parser() : _inited( FALSE ),
                      _stopped( FALSE ),
                      _options( NULL ),
                      _dataQueue( NULL ),
                      _packer( NULL ),
                      _livingNum( 0 ),
                      _parsedNum( 0 ),
                      _failedNum( 0 )
   {
   }

   Parser::~Parser()
   {
      vector<Worker*>::iterator i = _workers.begin() ;

      for( ; i != _workers.end(); ++i )
      {
         Worker* worker = *i ;

         SDB_OSS_DEL( worker ) ;
      }
   }

   INT32 Parser::init( Options* options, DataQueue* dataQueue, Packer* packer,
                       INT32 workerNum )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != options, "options can't be NULL" ) ;
      SDB_ASSERT( NULL != dataQueue, "dataQueue can't be NULL" ) ;
      SDB_ASSERT( NULL != packer, "packer can't be NULL" ) ;

      string parserLogFile = makeRecordLogFileName( options->csname(),
                                                    options->clname(),
                                                    string( "parse" ) ) ;

      rc = _logFile.init( parserLogFile, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init parser log file, rc=%d", rc ) ;
         goto error ;
      }

      _options   = options ;
      _dataQueue = dataQueue ;
      _packer    = packer ;

      for ( INT32 i = 0; i < workerNum; ++i )
      {
         ParserArgs* args = NULL ;
         Worker* worker   = NULL ;

         args = SDB_OSS_NEW ParserArgs() ;
         if ( NULL == args )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create parser args, rc=%d", rc ) ;
            goto error ;
         }

         args->id = i ;
         args->self = this ;

         worker = SDB_OSS_NEW Worker( _parserRoutine, args, TRUE ) ;
         if ( NULL == worker )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create parser Worker object" ) ;
            goto error ;
         }

         _workers.push_back( worker ) ;
      }

      _inited = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Parser::start()
   {
      INT32 rc  = SDB_OK ;
      INT32 num = _workers.size() ;

      SDB_ASSERT( num > 0, "Must have woker" ) ;

      for ( INT32 i = 0; i < num; ++i )
      {
         Worker* worker = _workers[i] ;

         rc = worker->start() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to start parser" ) ;
            goto error ;
         }

         _livingNum.inc() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Parser::stop()
   {
      INT32 rc  = SDB_OK ;
      INT32 num = _workers.size() ;

      if ( 0 == num )
      {
         goto done ;
      }

      for( vector<Worker*>::iterator i = _workers.begin(); i != _workers.end();)
      {
         Worker* worker = *i ;

         rc = worker->waitStop() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to wait importer stop" ) ;
            ++i ;
            rc = SDB_OK ;
         }
         else
         {
            SDB_OSS_DEL( worker ) ;
            i = _workers.erase( i ) ;
         }
      }

   done:
      if ( _packer )
      {
         _packer->clearQueue() ;
      }
      return rc;
   }
}
