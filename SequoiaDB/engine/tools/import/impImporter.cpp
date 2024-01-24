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

   Source File Name = impImporter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impImporter.hpp"
#include "impRecordImporter.hpp"
#include "pd.hpp"
#include <sstream>
#include <iostream>

namespace import
{
   struct ImporterArgs: public WorkerArgs
   {
      INT32          id;
      string         hostname;
      string         svcname;
      Importer*      self;

      ImporterArgs()
      {
         id = -1;
         self = NULL;
      }
   };

   inline BsonPage* _pageMemCopy( void* dest, BsonPage* src,
                                  INT32& offset, INT32 size )
   {
      INT32 copiedSize = 0 ;
      CHAR* buffer = NULL ;

      while( 0 < size )
      {
         SDB_ASSERT( NULL != src, "src can't be NULL" ) ;

         INT32 bufSize = src->getRecordsSize() - offset ;
         INT32 tmp     = bufSize > size ? size : bufSize ;

         if ( 0 == bufSize )
         {
            src = src->getNext() ;
            offset = 0 ;
            continue ;
         }

         SDB_ASSERT( 0 < tmp, "tmp can't be less than 0" ) ;

         buffer = src->getBuffer() ;

         ossMemcpy( (CHAR*)dest + copiedSize, buffer + offset, tmp ) ;

         size -= tmp ;
         copiedSize += tmp ;
         offset += tmp ;
      }

      return src ;
   }

   inline void _pageMemCopyPrepareForBson( INT32& offset )
   {
      offset = ossRoundUpToMultipleX( offset, 4 ) ;
   }

   inline INT32 _pageMemCopyBson( LogFile* logFile, bson* obj,
                                  BsonPage*& page, INT32& offset )
   {
      SDB_ASSERT( obj, "obj can't be null" ) ;

      INT32 rc  = SDB_OK ;
      INT32 ret = SDB_OK ;
      INT32 recordSize = 0 ;

      //get bson record size
      page = _pageMemCopy( &recordSize, page, offset, sizeof( recordSize ) );

      if ( NULL == obj->data || bson_buffer_size( obj ) < recordSize )
      {
         bson_init_size( obj, recordSize ) ;
         if ( NULL == obj->data )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to init bson size, rc=%d", rc ) ;
            goto error ;
         }
      }

      //get bson record
      ossMemcpy( obj->data, &recordSize, sizeof( recordSize ) ) ;
      page = _pageMemCopy( obj->data + sizeof( recordSize ), page, offset,
                           recordSize - sizeof( recordSize ) ) ;
      obj->cur = obj->data + recordSize - 1 ;
      obj->finished = 1 ;

      ret = logFile->write( obj ) ;
      if ( ret )
      {
         PD_LOG( PDERROR, "Failed to log write records, rc=%d", ret ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _writeRecords( LogFile* logFile, BsonPage* pages,
                               INT32 recordNum )
   {
      INT32 rc       = SDB_OK ;
      INT32 offset   = 0 ;
      BsonPage* page = pages ;
      bson obj ;

      bson_init( &obj ) ;

      SDB_ASSERT( NULL != page, "page can't be NULL" ) ;

      while( 0 < recordNum )
      {
         _pageMemCopyPrepareForBson( offset ) ;

         rc = _pageMemCopyBson( logFile, &obj, page, offset ) ;
         if ( rc )
         {
            goto error ;
         }

         --recordNum ;
      }

   done:
      bson_destroy( &obj ) ;
      return rc ;
   error:
      goto done ;
   }

   void _importerRoutine( WorkerArgs* args )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN dryRun = FALSE ;
      ImporterArgs* impArgs = (ImporterArgs*)args ;

      SDB_ASSERT( NULL != args, "arg can't be NULL" ) ;

      Importer* self   = impArgs->self ;
      Options* options = self->_options ;
      LogFile* logFile = &(self->_logFile) ;
      BsonPageQueue* freeQueue = self->_freeQueue ;
      PageQueue* importQueue   = self->_importQueue ;

      RecordImporter importer( impArgs->hostname,
                               impArgs->svcname,
                               options->user(),
                               options->password(),
                               options->csname(),
                               options->clname(),
                               options->useSSL(),
                               options->enableTransaction(),
                               options->allowKeyDuplication(),
                               options->replaceKeyDuplication(),
                               options->allowIDKeyDuplication(),
                               options->replaceIDKeyDuplication(),
                               options->mustHasIDField(),
                               options->batchSize() ) ;

      SDB_ASSERT( NULL != freeQueue, "freeQueue can't be NULL" ) ;
      SDB_ASSERT( NULL != importQueue, "importQueue can't be NULL" ) ;
      SDB_ASSERT( NULL != logFile, "logFile can't be NULL" ) ;

      /* dryrun indicates the test purpose and does not insert data.
         Dryrun means tryrun, corresponding to the
         hidden input parameter --dryrun */
      dryRun = options->dryRun() ;

      {
         stringstream ss ;

         ss << "Importer [" << impArgs->id << "] with " <<
               impArgs->hostname << ":" << impArgs->svcname << " started..." ;

         PD_LOG( PDEVENT, "%s", ss.str().c_str() ) ;

         if ( options->verbose() )
         {
            std::cout << ss.str() << std::endl ;
         }
      }

      rc = importer.connect() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to connect, rc=%d", rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         PageInfo pageInfo ;

         importQueue->wait_and_pop( pageInfo ) ;
         if ( 0 == pageInfo.recordNum )
         {
            // stop signal
            break ;
         }

         if ( !dryRun )
         {
            rc = importer.import( &pageInfo ) ;
            if ( rc )
            {
               _writeRecords( logFile, pageInfo.pages, pageInfo.recordNum ) ;

               freeQueue->pushPages( pageInfo.pages ) ;

               self->_failedNum.add( pageInfo.recordNum ) ;

               PD_LOG( PDERROR, "Failed to import records, rc=%d", rc ) ;
               continue ;
            }
         }

         freeQueue->pushPages( pageInfo.pages ) ;

         self->_importedNum.add( pageInfo.recordNum ) ;
         self->_duplicatedNum.add( pageInfo.duplicatedNum ) ;
      }

   done:
      {
         stringstream ss ;

         ss << "Importer [" << impArgs->id << "] stop" ;

         PD_LOG( PDEVENT, "%s", ss.str().c_str() ) ;

         if ( options->verbose() )
         {
            std::cout << ss.str() << std::endl ;
         }
      }
      self->_livingNum.dec() ;
      return ;
   error:
      goto done ;
   }

   Importer::Importer() : _inited( FALSE ),
                          _refCount( 0 ),
                          _options( NULL ),
                          _freeQueue( NULL ),
                          _importQueue( NULL ),
                          _livingNum( 0 ),
                          _importedNum( 0 ),
                          _failedNum( 0 ),
                          _duplicatedNum( 0 ) 
   {
   }

   Importer::~Importer()
   {
      vector<Worker*>::iterator i = _workers.begin() ;

      for( ; i != _workers.end(); ++i )
      {
         Worker* worker = *i ;

         SDB_OSS_DEL( worker ) ;
      }
   }

   INT32 Importer::init( Options* options, BsonPageQueue* freeQueue,
                         PageQueue* importQueue, INT32 workerNum )
   {
      INT32 rc = SDB_OK;
      string fileName ;
      SDB_ASSERT( NULL != options, "options can't be NULL" ) ;
      SDB_ASSERT( NULL != freeQueue, "freeQueue can't be NULL" ) ;
      SDB_ASSERT( NULL != importQueue, "importQueue can't be NULL" ) ;

      _options     = options ;
      _freeQueue   = freeQueue ;
      _importQueue = importQueue ;

      fileName = makeRecordLogFileName( _options->csname(),
                                        _options->clname(),
                                        string( "import" ) );

      rc = _logFile.init( fileName, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init importer log file, rc=%d", rc ) ;
         goto error ;
      }

      if ( _options->enableCoord() )
      {
         rc = _coords.init( _options->hosts(),
                            _options->user(),
                            _options->password(),
                            _options->useSSL() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init coords, rc=%d", rc ) ;
            goto error ;
         }
      }

      for ( INT32 i = 0; i < workerNum; ++i )
      {
         ImporterArgs* args = NULL ;
         string hostname ;
         string svcname ;

         if ( _options->enableCoord() )
         {
            rc = _coords.getRandomCoord( hostname, svcname ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get coord, rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = Coords::getRandomCoord( _options->hosts(), _refCount,
                                         hostname, svcname) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get coord, rc=%d", rc ) ;
               goto error ;
            }
         }

         args = SDB_OSS_NEW ImporterArgs() ;
         if ( NULL == args )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         args->id = i ;
         args->hostname = hostname ;
         args->svcname = svcname ;
         args->self = this ;

         Worker* worker = SDB_OSS_NEW Worker( _importerRoutine, args, TRUE ) ;
         if ( NULL == worker )
         {
            SDB_OSS_DEL( args ) ;
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create importer Worker object" ) ;
            goto error ;
         }

         _workers.push_back( worker ) ;
      }

      _inited = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Importer::start()
   {
      INT32 rc = SDB_OK;
      INT32 num = _workers.size();

      SDB_ASSERT(num > 0, "Must have woker");

      for (INT32 i = 0; i < num; i++)
      {
         Worker* worker = _workers[i] ;

         rc = worker->start();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to start importer");
            goto error;
         }

         _livingNum.inc();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Importer::stop()
   {
      INT32 rc = SDB_OK;
      INT32 num = _workers.size();

      if ( 0 == num )
      {
         goto done;
      }

      for ( INT32 i = 0; i < num; ++i )
      {
         // push empty pageInfo as stop signal
         PageInfo pageInfo ;

         _importQueue->push( pageInfo ) ;
      }

      for (vector<Worker*>::iterator i = _workers.begin(); i != _workers.end();)
      {
         Worker* worker = *i;

         rc = worker->waitStop();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to wait importer stop");
            i++;
            rc = SDB_OK;
         }
         else
         {
            SDB_OSS_DEL(worker);
            i = _workers.erase(i);
         }
      }

   done:
      return rc;
   }
}
