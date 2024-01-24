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
          06/11/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "impPacker.hpp"
#include "ossUtil.hpp"

namespace import
{
   #define IMP_ONE_BULKINSERT_MAX_SIZE  (32 * 1024 * 1024)

   Packer::Packer() : _mapNum( 0 ),
                      _inited( FALSE ),
                      _stopped( FALSE ),
                      _needSharding( FALSE ),
                      _options( NULL ),
                      _freeQueue( NULL ),
                      _importQueue( NULL ),
                      _shardingNum( 0 ),
                      _failedNum( 0 )
   {
   }

   Packer::~Packer()
   {
      PageMap::iterator it ;

      for ( it = _pageMap.begin(); it != _pageMap.end(); ++it )
      {
         BsonPageHeader* pageHeader = it->second ;

         SAFE_OSS_DELETE( pageHeader ) ;
      }
   }

   INT32 Packer::init( Options* options, BsonPageQueue* freeQueue,
                       PageQueue* importQueue )
   {
      INT32 rc = SDB_OK ;
      string shardingLogFile ;

      SDB_ASSERT( NULL != options, "options can't be NULL" ) ;
      SDB_ASSERT( NULL != freeQueue, "freeQueue can't be NULL" ) ;
      SDB_ASSERT( NULL != importQueue, "importQueue can't be NULL" ) ;

      _options     = options ;
      _freeQueue   = freeQueue ;
      _importQueue = importQueue ;

      if ( !_options->enableSharding() ||
           _options->batchSize() <= 1 )
      {
         rc = _initPageMap() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init page map, rc=%d", rc ) ;
            goto error ;
         }

         _needSharding = FALSE ;
         _inited = TRUE ;
         goto done ;
      }

      rc = _sharding.init( _options->hosts(),
                           _options->user(), _options->password(),
                           _options->csname(), _options->clname(),
                           _options->useSSL() );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init sharding, rc=%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( _sharding.getGroupNum() >= 0,
                  "groupNum must be greater than or equals 0" ) ;

      if ( _sharding.getGroupNum() <= 1 )
      {
         rc = _initPageMap() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init page map, rc=%d", rc ) ;
            goto error ;
         }

         _needSharding = FALSE ;
         _inited = TRUE ;
         goto done ;
      }

      shardingLogFile = makeRecordLogFileName( _options->csname(),
                                               _options->clname(),
                                               string( "sharding" ) ) ;

      rc = _logFile.init( shardingLogFile, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init sharding log file" ) ;
         goto error ;
      }

      {
         stringstream ss ;

         ss << "Sharding started with " << _sharding.getGroupNum() <<
               " groups..." << std::endl ;

         PD_LOG( PDEVENT, "%s", ss.str().c_str() ) ;

         if ( _options->verbose() )
         {
            std::cout << ss.str() ;
         }
      }

      rc = _initShardingMap() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init page map, rc=%d", rc ) ;
         goto error ;
      }

      _needSharding = TRUE ;
      _inited = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Packer::packing( bson* record )
   {
      INT32 rc = SDB_OK ;
      INT32 freeSize  = 0 ;
      INT32 writeSize = 0 ;
      INT32 dataSize  = 0 ;
      UINT32 headerId = 0 ;
      const CHAR* data = NULL ;
      BsonPageHeader* pageHeader = NULL ;
      PageMap::iterator it ;

      SDB_ASSERT( NULL != record, "record can't be NULL" ) ;

      rc = _getHeaderId( record, headerId ) ;
      if ( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      data     = bson_data( record ) ;
      dataSize = bson_size( record ) ;

      it = _pageMap.find( headerId ) ;
      SDB_ASSERT( it != _pageMap.end(), "headerId not found" ) ;

      pageHeader = it->second ;

   begin:
      pageHeader->lock() ;

      freeSize = pageHeader->getFreeSize() ;

      if ( freeSize < dataSize )
      {
         INT32 size = dataSize - freeSize ;
         BsonPage* page = NULL ;

         while ( TRUE )
         {
            rc = _freeQueue->popPages( size, page ) ;
            if ( SDB_OSS_UP_TO_LIMIT == rc )
            {
               rc = SDB_OK ;

               if ( _stopped )
               {
                  goto done ;
               }

               if ( !clearQueue( TRUE ) && pageHeader->getTotalRecord() > 0 )
               {
                  _pushToImportQueue( pageHeader ) ;

                  pageHeader->release() ;

                  goto begin ;
               }

               ossSleep( 10 ) ;
               continue ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get page, rc=%d", rc ) ;
               goto error ;
            }

            break ;
         }

         pageHeader->append( page ) ;
      }

      writeSize = pageHeader->write( data, dataSize ) ;
      SDB_ASSERT( writeSize == dataSize, "Page buffer not enough space" ) ;

      if ( _needSharding )
      {
         _shardingNum.add( 1 ) ;
      }

      if ( pageHeader->getTotalRecord() >= _options->batchSize() ||
           pageHeader->getUsedSize() >= IMP_ONE_BULKINSERT_MAX_SIZE )
      {
         _pushToImportQueue( pageHeader ) ;
      }

   done:
      if ( pageHeader )
      {
         pageHeader->release() ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN Packer::clearQueue( BOOLEAN allClear )
   {
      BOOLEAN isClear = FALSE ;
      PageMap::iterator it ;

      for ( it = _pageMap.begin(); it != _pageMap.end(); ++it )
      {
         BsonPageHeader* pageHeader = it->second ;

         if ( !pageHeader->try_lock() )
         {
            continue ;
         }

         if ( pageHeader->getTotalRecord() > 0 )
         {
            _pushToImportQueue( pageHeader ) ;

            isClear = TRUE ;
         }

         pageHeader->release() ;

         if ( !allClear && isClear )
         {
            break ;
         }
      }

      return isClear ;
   }

   INT32 Packer::_initShardingMap()
   {
      INT32 rc = SDB_OK ;
      vector<UINT32> list ;
      vector<UINT32>::iterator it ;

      rc = _sharding.getAllGroupID( list ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get all group id, rc=%d", rc ) ;
         goto error ;
      }

      for ( it = list.begin(); it != list.end(); ++it )
      {
         UINT32 mapID = *it ;
         BsonPageHeader* pageHeader = NULL ;

         pageHeader = SDB_OSS_NEW BsonPageHeader() ;
         if ( NULL == pageHeader )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to allocate PageHeader, rc=%d", rc ) ;
            goto error ;
         }

         _pageMap[mapID] = pageHeader ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 Packer::_initPageMap()
   {
      INT32 rc = SDB_OK ;

      _mapNum = _options->parsers() ;

      if ( _mapNum <= 1 )
      {
         _mapNum = 1 ;
      }
      else if ( _mapNum > 1 )
      {
         _mapNum = _mapNum * 2 ;
      }

      for ( INT32 i = 0; i < _mapNum; ++i )
      {
         UINT32 mapID = (UINT32)i ;
         BsonPageHeader* pageHeader = NULL ;

         pageHeader = SDB_OSS_NEW BsonPageHeader() ;
         if ( NULL == pageHeader )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to allocate PageHeader, rc=%d", rc ) ;
            goto error ;
         }

         _pageMap[mapID] = pageHeader ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void Packer::_pushToImportQueue( BsonPageHeader* pageHeader )
   {
      SDB_ASSERT( NULL != pageHeader, "pageHeader can't be NULL" ) ;

      PageInfo pageInfo( pageHeader->getTotalRecord(),
                         pageHeader->getFirstPage() ) ;

      _importQueue->push( pageInfo ) ;

      pageHeader->reset() ;
   }

   INT32 Packer::_getHeaderId( bson* record, UINT32& headerId )
   {
      INT32 rc = SDB_OK ;

      if ( _needSharding )
      {
         string collection ;

         rc = _sharding.getGroupByRecord( record, collection, headerId ) ;
         if ( rc )
         {
            INT32 ret = SDB_OK ;

            _failedNum.add( 1 ) ;

            PD_LOG( PDERROR, "Failed to get group by record, rc=%d", rc ) ;

            ret = _logFile.write( record ) ;
            if ( ret )
            {
               PD_LOG( PDERROR, "Failed to log record, rc=%d", ret ) ;
            }

            goto error ;
         }
      }
      else
      {
         headerId = (UINT32)(ossRand() % (UINT32)_mapNum) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}