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

   Source File Name = impRoutine.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRoutine.hpp"
#include "impHosts.hpp"
#include "impUtil.hpp"
#include <iostream>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace pt = boost::posix_time;

namespace import
{
   #define IMP_QUEUE_PAGE_SIZE (16 * 1024)

   Routine::Routine( Options& options ) : _isInit( FALSE ),
                                          _dataQueueNum( 0 ),
                                          _dataQueue( NULL ),
                                          _freeQueue( NULL ),
                                          _importQueue( NULL ),
                                          _options( options )
   {
   }

   Routine::~Routine()
   {
      PageInfo pageInfo ;

      // If an error occurs during import, there may be residual data
      // in the _importQueue.
      if ( _importQueue )
      {
         while ( _importQueue->try_pop( pageInfo ) )
         {
            SAFE_OSS_DELETE( pageInfo.pages ) ;
         }
      }

      SAFE_OSS_DELETE( _dataQueue ) ;
      SAFE_OSS_DELETE( _freeQueue ) ;
      SAFE_OSS_DELETE( _importQueue ) ;

      _importQueueBuffer.finiBuffer() ;
   }

   INT32 Routine::_init()
   {
      INT32 rc = SDB_OK ;
      INT32 pageSize  = IMP_QUEUE_PAGE_SIZE ;
      INT32 pageNum   = 0 ;

      rc = _options.check();
      if (SDB_OK != rc)
      {
         goto error;
      }

      pageNum = _options.recordsMem() / pageSize ;
      _dataQueueNum = _options.parsers() ;

      rc = _scanner.initParser( &_options ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init parser, rc=%d", rc ) ;
         goto error ;
      }

      if ( FALSE == _importQueueBuffer.initBuffer( pageNum ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate import queue buffer with [%u], "
                          "rc=%d",
                 pageNum, rc ) ;
         goto error ;
      }

      _importQueue = SDB_OSS_NEW PageQueue(
                                 PageQueueContainer( &_importQueueBuffer ) ) ;
      if ( NULL == _importQueue )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate import queue with [%u], rc=%d",
                 pageNum, rc ) ;
         goto error ;
      }

      _dataQueue = SDB_OSS_NEW DataQueue() ;
      if ( NULL == _dataQueue )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to create data queue, rc=%d", rc ) ;
         goto error ;
      }

      rc = _dataQueue->init( _dataQueueNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init data queue, rc=%d", rc ) ;
         goto error ;
      }

      _freeQueue = SDB_OSS_NEW BsonPageQueue() ;
      if ( NULL == _freeQueue )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to create bson page queue, rc=%d", rc ) ;
         goto error ;
      }

      rc = _freeQueue->init( pageNum, pageSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init bson page queue, rc=%d", rc ) ;
         goto error ;
      }

      rc = _packer.init( &_options, _freeQueue, _importQueue ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init packer, rc=%d", rc ) ;
         goto error ;
      }

      _isInit = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Routine::run()
   {
      INT32 rc = SDB_OK ;
      INT32 ret = SDB_OK ;
      pt::ptime startTime ;
      pt::ptime endTime ;

      PD_LOG( PDINFO, "Begin importing" ) ;

      startTime = pt::second_clock::universal_time() ;

      rc = _init() ;
      if ( rc )
      {
         std::cerr << "Error: initialization error, please verify log "
                   << getDialogName()
                   << std::endl ;
         goto done ;
      }

      rc = _startScanner() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start scanner, rc=%d", rc ) ;
         goto stop ;
      }

      rc = _startParser() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start parser, rc=%d", rc ) ;
         goto stop ;
      }

      rc = _startImporter() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start importers, rc=%d", rc ) ;
         goto stop ;
      }

      while ( !_parser.isStopped() && !_importer.isStopped() )
      {
         ossSleep( 100 ) ;
      }

   stop:
      ret = _stopScanner() ;
      if ( ret )
      {
         PD_LOG( PDERROR, "Failed to stop scanner, rc=%d", ret ) ;
      }

      ret = _waitParserStop() ;
      if ( ret )
      {
         PD_LOG( PDERROR, "Failed to wait parser stop, rc=%d", ret ) ;
      }

      ret = _stopImporter();
      if ( ret )
      {
         PD_LOG(PDERROR, "Failed to stop importers, rc=%d", ret ) ;
      }

      if ( rc )
      {
         std::cerr << "Error: import failed, please verify log "
                   << getDialogName()
                   << std::endl ;
      }

      endTime = pt::second_clock::universal_time() ;

      if ( SDB_OK == rc && _importer.importedNum() > 0 )
      {
         INT64 sec = 0 ;
         pt::time_duration time = endTime - startTime ;
         stringstream ss ;

         sec = time.total_seconds() ;

         ss << "Import " << _importer.importedNum() <<
               " records in " << sec << " second(s)" ;

         if ( sec > 0 )
         {
            ss << ", average " << _importer.importedNum() / sec << " records/s";
         }

         PD_LOG( PDINFO, "%s", ss.str().c_str() ) ;
      }

   done:
      PD_LOG( PDINFO, "Finished importing" ) ;
      return rc ;
   }

   INT32 Routine::_startScanner()
   {
      INT32 rc = SDB_OK ;
      INT32 workerNum = _options.parsers() ;

      rc = _scanner.init( &_options, _dataQueue, workerNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init scanner, rc=%d", rc ) ;
         goto error ;
      }

      rc = _scanner.start() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start scanner, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Routine::_stopScanner()
   {
      INT32 rc = SDB_OK ;

      rc = _scanner.stop() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to wait the scanner stop" ) ;
      }

      return rc ;
   }


   INT32 Routine::_startParser()
   {
      INT32 rc = SDB_OK ;
      INT32 workerNum = _options.parsers() ;

      rc = _parser.init( &_options, _dataQueue, &_packer, workerNum ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init parser, rc=%d", rc ) ;
         goto error ;
      }

      rc = _parser.start() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start parser, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Routine::_waitParserStop()
   {
      INT32 rc = SDB_OK ;

      _packer.stop() ;

      rc = _parser.stop() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to wait the parser stop" ) ;
      }

      return rc ;
   }

   INT32 Routine::_startImporter()
   {
      INT32 rc = SDB_OK;
      INT32 workerNum = _options.jobs() ;

      rc = _importer.init( &_options, _freeQueue, _importQueue, workerNum ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init importer, rc=%d", rc ) ;
         goto error ;
      }

      rc = _importer.start() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start importer, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Routine::_stopImporter()
   {
      INT32 rc = SDB_OK;

      rc = _importer.stop();
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to stop importer, rc=%d", rc ) ;
      }

      return rc;
   }

   void Routine::printStatistics()
   {
      if ( _isInit )
      {
         stringstream ss;

         ss << "Parsed records: " << _parser.parsedNum() << std::endl
            << "Parsed failure: " << _parser.failedNum() << std::endl;

         ss << "Sharding records: " << _packer.shardingNum() << std::endl
            << "Sharding failure: " << _packer.failedNum() << std::endl;

         ss << "Imported records: " << _importer.importedNum() << std::endl
            << "Imported failure: " << _importer.failedNum() << std::endl;

         ss << "Duplicated records: " << _importer.duplicatedNum() << std::endl;

         if ( _parser.failedNum() > 0 )
         {
            ss << "See " << _parser.logFileName()
               << " for parse failure records" << std::endl ;
         }

         if ( _packer.failedNum() > 0 )
         {
            ss << "See " << _packer.logFileName()
               << " for sharding failure records" << std::endl ;
         }

         if ( _importer.failedNum() > 0 )
         {
            ss << "See " << _importer.logFileName()
               << " for import failure records" << std::endl ;
         }

         string stat = ss.str() ;

         std::cout << stat ;
         PD_LOG( PDEVENT, stat.c_str() ) ;
      }
   }
}

