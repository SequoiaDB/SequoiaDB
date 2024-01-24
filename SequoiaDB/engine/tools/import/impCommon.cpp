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

   Source File Name = impCommon.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/17/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#include "impCommon.hpp"

namespace import
{

   INT32 reallocBuffer ( CHAR **ppBuffer, INT32 *bufferSize, INT32 newSize )
   {
      INT32 rc = SDB_OK ;

      if ( newSize > *bufferSize )
      {
         CHAR *newBuff = (CHAR *)SDB_OSS_REALLOC ( *ppBuffer, newSize ) ;
         if ( !newBuff )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         *ppBuffer   = newBuff ;
         *bufferSize = newSize ;
      }

      // SEQUOIADBMAINSTREAM-1916
      if ( NULL == *ppBuffer )
      {
         rc = SDB_OOM ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   _impBufferBlock::_impBufferBlock() : _inited( FALSE ),
                                        _bufferSize( 0 ),
                                        _buffer( NULL ),
                                        _mutex( RW_EXCLUSIVEWRITE )
   {
   }

   _impBufferBlock::~_impBufferBlock()
   {
      SAFE_OSS_FREE( _buffer ) ;
   }

   INT32 _impBufferBlock::init( INT32 bufferSize )
   {
      INT32 rc = SDB_OK ;

      _bufferSize = bufferSize ;

      _buffer = (CHAR*)SDB_OSS_MALLOC( _bufferSize ) ;
      if ( NULL == _buffer )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      _inited = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _DataQueue::_DataQueue() : _cur( 0 ),
                              _queueNum( 0 ),
                              _queue( NULL )
   {
   }

   _DataQueue::~_DataQueue()
   {
      if ( NULL != _queue )
      {
         for ( INT32 i = 0; i < _queueNum; ++i )
         {
            SAFE_OSS_DELETE( _queue[i] ) ;
         }

         SDB_OSS_FREE( _queue ) ;
         _queue = NULL ;
      }
   }

   INT32 _DataQueue::init( INT32 queueNum )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( 0 < queueNum, "queueNum must be greater than 0" ) ;

      _queueNum  = queueNum ;

      _queue = (RecordDataQueue**)SDB_OSS_MALLOC(
                                    _queueNum * sizeof( RecordDataQueue* ) ) ;
      if ( NULL == _queue )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemset( _queue, 0, _queueNum * sizeof( RecordDataQueue* ) ) ;

      for ( INT32 i = 0; i < _queueNum; ++i )
      {
         _queue[i] = SDB_OSS_NEW RecordDataQueue() ;
         if ( NULL == _queue[i] )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         rc = _queue[i] -> init( IMP_QUEUE_CAPACITY ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _BsonPage::_BsonPage( CHAR* buffer, INT32 size ) : _ownmem( TRUE ),
                                                      _bufferSize( size ),
                                                      _used( 0 ),
                                                      _recordsSize( 0 ),
                                                      _buffer( buffer ),
                                                      _cur( buffer ),
                                                      _next( NULL )
   {
      SDB_ASSERT( ( buffer && size > 0 ) || ( NULL == buffer && 0 == size),
                  "Invalid buffer or size" ) ;
      SDB_ASSERT( size % 4 == 0, "size must be in multiples of 4" ) ;

      _ownmem = NULL == _buffer ? TRUE : FALSE ;
   }

   _BsonPage::~_BsonPage()
   {
      if( _ownmem && _buffer )
      {
         SDB_OSS_FREE( _buffer ) ;
      }
   }

   INT32 _BsonPage::init( INT32 size )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( size % 4 == 0, "size must be in multiples of 4" ) ;

      if( _ownmem )
      {
         if ( _buffer )
         {
            SDB_OSS_FREE( _buffer ) ;
         }

         _buffer = (CHAR*)SDB_OSS_MALLOC( size ) ;
         if( NULL == _buffer )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _bufferSize = size ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _BsonPage::write( const CHAR *data, INT32 size )
   {
      INT32 free = 0 ;
      INT32 tmp  = 0 ;
      BOOLEAN isFull = FALSE ;
      SDB_ASSERT( NULL != data, "data can't be NULL" ) ;

      free   = _bufferSize - _used ;
      isFull = free <= size ;
      tmp    = isFull ? free : size ;

      if ( 0 < tmp )
      {
         INT32 alignSize = isFull ? tmp : ossRoundUpToMultipleX( tmp, 4 ) ;

         ossMemcpy( _cur, data, tmp ) ;

         _recordsSize = _used + tmp ;

         _cur  += alignSize ;
         _used += alignSize ;
      }

      return tmp ;
   }

   _BsonPageHeader::_BsonPageHeader() : _totalSize( 0 ),
                                        _totalRecord( 0 ),
                                        _first( NULL ),
                                        _last( NULL ),
                                        _cur( NULL ),
                                        _mutex( RW_EXCLUSIVEWRITE )
   {
   }

   _BsonPageHeader::~_BsonPageHeader()
   {
   }

   INT32 _BsonPageHeader::append( BsonPage* pages )
   {
      BsonPage* page = pages ;
      SDB_ASSERT( NULL != pages, "pages can't be NULL" ) ;

      while( page )
      {
         if ( NULL == _first )
         {
            _first = page ;
         }

         if ( NULL == _cur )
         {
            _cur = _first ;
         }

         if ( _last )
         {
            _last->setNext( page ) ;
         }

         _last = page ;

         _totalSize += _last->getSize() ;

         page = page->getNext() ;
      }

      return SDB_OK ;
   }

   INT32 _BsonPageHeader::getFreeSize()
   {
      INT32 freeSize = 0 ;
      BsonPage* next = _cur ;

      while( next )
      {
         freeSize += next->getFreeSize() ;
         next = next->getNext() ;
      }

      return freeSize ;
   }

   INT32 _BsonPageHeader::getUsedSize()
   {
      INT32 usedSize = 0 ;
      BsonPage* next = _first ;

      while( next && FALSE == next->isEmpty() )
      {
         usedSize += next->getUsedSize() ;
         next = next->getNext() ;
      }

      return usedSize ;
   }

   INT32 _BsonPageHeader::write( const CHAR *data, INT32 size )
   {
      INT32 writeSize = 0 ;
      SDB_ASSERT( NULL != data, "data can't be NULL" ) ;

      while( writeSize < size )
      {
         /*
         When the data page is full,
         need to align the last record with 4 bytes.
         */
         _cur->alignRecords() ;

         writeSize += _cur->write( data + writeSize, size - writeSize ) ;

         if ( NULL == _cur->getNext() )
         {
            break ;
         }

         _cur = _cur->getNext() ;
      }

      ++_totalRecord ;

      return writeSize ;
   }

   _BsonPageQueue::_BsonPageQueue() : _pageNum( 0 ),
                                      _pageSize( 0 ),
                                      _useNum( 0 ),
                                      _bufferSize( 0 ),
                                      _buffer( NULL ),
                                      _mutex( RW_EXCLUSIVEWRITE )
   {
   }

   _BsonPageQueue::~_BsonPageQueue()
   {
      BsonPage** tmp = _pageQueue.getFront() ;

      while( tmp )
      {
         BsonPage* page = *tmp ;

         SAFE_OSS_DELETE( page ) ;

         _pageQueue.popFront() ;

         tmp = _pageQueue.getFront() ;
      }

      _pageQueue.finiBuffer() ;

      SAFE_OSS_FREE( _buffer ) ;
   }

   INT32 _BsonPageQueue::init( INT32 pageNum, INT32 pageSize )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 0 < pageNum, "pageNum must be greater then 0" ) ;
      SDB_ASSERT( 0 < pageSize, "pageSize must be greater then 0" ) ;

      _pageNum  = pageNum ;
      _pageSize = pageSize ;
      _bufferSize = _pageNum * _pageSize ;

      if ( FALSE == _pageQueue.initBuffer( _pageNum ) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate queue buffer with [%u], rc=%d",
                 _pageNum, rc ) ;
         goto error ;
      }

      _buffer = (CHAR*)SDB_OSS_MALLOC( _bufferSize ) ;
      if ( NULL == _buffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate buffer, size=%d, rc=%d",
                 _bufferSize, rc ) ;
         goto error ;
      }

      for( INT32 i = 0; i < _pageNum; ++i )
      {
         BsonPage *page = SDB_OSS_NEW BsonPage( _buffer + ( i * _pageSize ),
                                                _pageSize ) ;

         if ( NULL == page )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to allocate BsonPage, rc=%d", rc ) ;
            goto error ;
         }

         page->init( _pageSize ) ;

         _pageQueue.pushBack( page ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _BsonPageQueue::pushPages( BsonPage* pages )
   {
      BsonPage* page = pages ;

      _mutex.lock_w() ;

      while( page )
      {
         BsonPage* next = page->getNext() ;

         SDB_ASSERT( _pageQueue.pushBack( page ) == TRUE, "queue is full" ) ;

         page->reset() ;

         page = next ;

         --_useNum ;
      }

      _mutex.release_w() ;
   }

   INT32 _BsonPageQueue::popPages( INT32 size, BsonPage*& pages )
   {
      INT32 rc = SDB_OK ;
      INT32 getSize = 0 ;
      BsonPage* first = NULL ;
      BsonPage* cur   = NULL ;

      _mutex.lock_w() ;

      if ( ( _pageNum - _useNum ) * _pageSize < size )
      {
         //Out of memory pages
         rc = SDB_OSS_UP_TO_LIMIT ;
         goto error ;
      }

      while( getSize < size )
      {
         BsonPage* tmp = *( _pageQueue.getFront() ) ;

         _pageQueue.popFront() ;

         tmp->reset() ;

         if( NULL == first )
         {
            first = tmp ;
            cur   = tmp ;
         }
         else
         {
            cur->setNext( tmp ) ;
            cur = tmp ;
         }

         ++_useNum ;
         getSize += _pageSize ;
      }

      pages = first ;

   done:
      _mutex.release_w() ;
      return rc ;
   error:
      goto done ;
   }

   _sdbMsgConvertor::_sdbMsgConvertor() : _buff( NULL ),
                                          _buffSize( 0 )
   {
   }

   _sdbMsgConvertor::~_sdbMsgConvertor()
   {
      if ( _buff )
      {
         SDB_OSS_FREE( _buff ) ;
      }
      _buff     = NULL ;
      _buffSize = 0 ;
   }

   INT32 _sdbMsgConvertor::downgradeRequest( const CHAR *pRequestHeader,
                                             INT32 requestHeaderLength,
                                             CHAR *&outputData,
                                             INT32 &outputDataLength )
   {
      INT32 rc          = SDB_OK ;
      MsgHeader *header = (MsgHeader *)pRequestHeader ;
      MsgHeaderV1 newHeader ;

      clientMsgHeaderDowngrade( header, &newHeader ) ;
      // we don't calculate the exact buffer size, just make sure the buffer is
      // large enough, and that is ok
      rc = _ensureBuff( requestHeaderLength ) ;
      if ( rc )
      {
         goto error ;
      }
      // build a request header by using MsgHeader
      ossMemcpy( _buff, &newHeader, sizeof(MsgHeaderV1) ) ;
      ossMemcpy( _buff + sizeof(MsgHeaderV1), (CHAR *)header + sizeof(MsgHeader),
                 requestHeaderLength - sizeof(MsgHeader) ) ;
      // output the result
      outputData       = _buff ;
      outputDataLength =
         requestHeaderLength - ( sizeof(MsgHeader) - sizeof(MsgHeaderV1) ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbMsgConvertor::upgradeReply( const CHAR *replyMsg,
                                         CHAR *&outputData,
                                         INT32 &outputDataLength )
   {
      INT32 rc            = SDB_OK ;
      MsgOpReplyV1 *reply = (MsgOpReplyV1 *)replyMsg ;
      MsgOpReply newReply ;

      clientMsgReplyHeaderUpgrade( reply, &newReply ) ;
      rc = _ensureBuff( newReply.header.messageLength ) ;
      if ( rc )
      {
         goto error ;
      }
      // build a reply by using MsgOpReplyV1
      ossMemcpy( _buff, &newReply, sizeof(MsgOpReply) ) ;
      ossMemcpy( _buff + sizeof(MsgOpReply),
                (CHAR *)reply + sizeof(MsgOpReplyV1),
                reply->header.messageLength - sizeof(MsgOpReplyV1) ) ;
      // output the result
      outputData       = _buff ;
      outputDataLength = newReply.header.messageLength ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbMsgConvertor::_ensureBuff( INT32 size )
   {
      return reallocBuffer( &_buff, &_buffSize, size ) ;
   }
}
