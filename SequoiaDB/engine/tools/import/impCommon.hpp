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

   Source File Name = impCommon.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/17/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_COMMON_HPP__
#define IMP_COMMON_HPP__

#include "ossUtil.h"
#include "ossQueue.hpp"
#include "ossRWMutex.hpp"
#include "utilCircularQueue.hpp"
#include "pd.hpp"
#include "msg.h"
#include "common.h"

namespace import
{

   INT32 reallocBuffer ( CHAR **ppBuffer, INT32 *bufferSize, INT32 newSize ) ;

   class _impBufferBlock : public SDBObject
   {
   public:
      _impBufferBlock() ;
      ~_impBufferBlock() ;

      INT32 init( INT32 bufferSize ) ;

      inline INT32 getSize()
      {
         return _bufferSize ;
      }

      inline CHAR* getBuffer( INT32 offset = 0 )
      {
         SDB_ASSERT( 0 <= offset,
                     "offset must be greater than or equal to 0" ) ;
         SDB_ASSERT( _bufferSize > offset,
                     "offset must be less than _bufferSize" ) ;
         SDB_ASSERT( TRUE == _inited, "bufferPage must call init" ) ;

         return _buffer + offset ;
      }

      inline INT32 lock_r( INT32 millisec = -1 )
      {
         return _mutex.lock_r( millisec ) ;
      }

      inline INT32 lock_w( INT32 millisec = -1 )
      {
         return _mutex.lock_w( millisec ) ;
      }

      inline INT32 release_r()
      {
         return _mutex.release_r() ;
      }

      inline INT32 release_w()
      {
         return _mutex.release_w() ;
      }

      inline BOOLEAN try_lock_r()
      {
         return _mutex.try_lock_r() ;
      }

      inline BOOLEAN try_lock_w()
      {
         return _mutex.try_lock_w() ;
      }

   private:
      BOOLEAN    _inited ;
      INT32      _bufferSize ;
      CHAR*      _buffer ;
      engine::ossRWMutex _mutex ;
   } ;
   typedef class _impBufferBlock impBufferBlock ;

   struct _RecordData : public SDBObject
   {
      INT32             fileId ;
      INT32             dataLen ;
      INT32             fieldsLen ;
      CHAR*             data ;
      const CHAR*       fields ;
      impBufferBlock*   bufferBlock ;

      _RecordData()
      {
         fileId      = 0 ;
         dataLen     = 0 ;
         fieldsLen   = 0 ;
         data        = NULL ;
         fields      = NULL ;
         bufferBlock = NULL ;
      }
   } ;
   typedef struct _RecordData RecordData ;

   #define IMP_QUEUE_SLEEPTIME 10
   #define IMP_QUEUE_CAPACITY 16384

   typedef class engine::utilSPSCQueue<RecordData> RecordDataQueue ;

   /* The _DataQueue class provides a composite
      single-writer/multiple-reader fifo queue */
   class _DataQueue : public SDBObject
   {
   public:
      _DataQueue() ;

      ~_DataQueue() ;

      INT32 init( INT32 queueNum ) ;

      inline void push( RecordData& data )
      {
         while( !_queue[_cur]->push( data ) )
         {
            _cur = ( _cur + 1 ) % _queueNum ;
         }

         _cur = ( _cur + 1 ) % _queueNum ;
      }

      inline void wait_and_push( RecordData& data )
      {
         while( !_queue[_cur]->push( data ) )
         {
            ossSleep( IMP_QUEUE_SLEEPTIME ) ;
         }

         _cur = ( _cur + 1 ) % _queueNum ;
      }

      inline BOOLEAN pop( INT32 id, RecordData& data )
      {
         SDB_ASSERT( 0 <= id && id < _queueNum,
                     "id must be between 0 and _queueNum" ) ;

         return _queue[id]->pop( data ) ;
      }

   private:
      INT32                _cur ;
      INT32                _queueNum ;
      RecordDataQueue**    _queue ;
   } ;
   typedef class _DataQueue DataQueue ;

   class _BsonPage : public SDBObject
   {
   public:
      _BsonPage( CHAR* buffer = NULL, INT32 size = 0 ) ;
      ~_BsonPage() ;

      INT32 init( INT32 size ) ;

      INT32 write( const CHAR *data, INT32 size ) ;

      inline void reset()
      {
         _used = 0 ;
         _recordsSize = 0 ;
         _cur  = _buffer ;
         _next = NULL ; ;
      }

      inline CHAR* getBuffer( INT32 offset = 0 )
      {
         SDB_ASSERT( 0 <= offset,
                     "offset must be greater than or equal to 0" ) ;
         SDB_ASSERT( _bufferSize > offset,
                     "offset must be less than _bufferSize" ) ;

         return _buffer + offset ;
      }

      inline _BsonPage* getNext()
      {
         return _next ;
      }

      inline void setNext( _BsonPage* next )
      {
         _next = next ;
      }

      inline INT32 getSize()
      {
         return _bufferSize ;
      }

      inline BOOLEAN isFull()
      {
         return _used == _bufferSize ;
      }

      inline INT32 getFreeSize()
      {
         return _bufferSize - _used ;
      }

      inline INT32 getUsedSize()
      {
         return _used ;
      }

      inline void alignRecords()
      {
         if ( isFull() )
         {
            _recordsSize = _used ;
         }
      }

      inline INT32 getRecordsSize()
      {
         return _recordsSize ;
      }

      inline BOOLEAN isEmpty()
      {
         return _used == 0 ? TRUE : FALSE ;
      }

   private:
      BOOLEAN     _ownmem ;
      INT32       _bufferSize ;
      INT32       _used ;
      INT32       _recordsSize ;
      CHAR*       _buffer ;
      CHAR*       _cur ;
      _BsonPage*  _next ;
   } ;
   typedef _BsonPage BsonPage ;

   class _BsonPageHeader : public SDBObject
   {
   public:
      _BsonPageHeader() ;
      ~_BsonPageHeader() ;

      INT32 append( BsonPage* pages ) ;

      INT32 getFreeSize() ;

      INT32 getUsedSize() ;

      INT32 write( const CHAR *data, INT32 size ) ;

      inline void reset()
      {
         _totalSize = 0 ;
         _totalRecord = 0 ;
         _first = NULL ;
         _last = NULL ;
         _cur = NULL ;
      }

      inline INT32 getTotalRecord()
      {
         return _totalRecord ;
      }

      inline BsonPage* getFirstPage()
      {
         return _first ;
      }

      inline INT32 lock( INT32 millisec = -1 )
      {
         return _mutex.lock_w( millisec ) ;
      }

      inline INT32 release()
      {
         return _mutex.release_w() ;
      }

      inline BOOLEAN try_lock()
      {
         return _mutex.try_lock_w() ;
      }

   private:
      INT32       _totalSize ;
      INT32       _totalRecord ;
      BsonPage*   _first ;
      BsonPage*   _last ;
      BsonPage*   _cur ;
      engine::ossRWMutex _mutex ;
   } ;
   typedef _BsonPageHeader BsonPageHeader ;

   struct _PageInfo : public SDBObject
   {
      INT32 recordNum ;
      INT32 duplicatedNum ;
      BsonPage *pages ;

      _PageInfo( INT32 num = 0, BsonPage* pPages = NULL, INT32 duplNum = 0 )
      {
         recordNum     = num ;
         pages         = pPages ;
         duplicatedNum = duplNum ;
      }
   } ;
   typedef struct _PageInfo PageInfo ;

   typedef engine::_utilCircularBuffer<PageInfo> PageQueueBuffer ;
   typedef engine::_utilCircularQueue<PageInfo> PageQueueContainer ;
   typedef class ossQueue<PageInfo, PageQueueContainer> PageQueue ;

   typedef engine::_utilCircularBuffer<BsonPage*> BsonPageBuffer ;

   class _BsonPageQueue : public SDBObject
   {
   public:
      _BsonPageQueue() ;
      ~_BsonPageQueue() ;

      INT32 init( INT32 pageNum, INT32 pageSize ) ;

      void pushPages( BsonPage* pages ) ;

      INT32 popPages( INT32 size, BsonPage*& pages ) ;

   private:
      INT32                _pageNum ;
      INT32                _pageSize ;
      INT32                _useNum ;
      INT32                _bufferSize ;
      CHAR*                _buffer ;
      engine::ossRWMutex   _mutex ;
      BsonPageBuffer       _pageQueue ;
   } ;
   typedef _BsonPageQueue BsonPageQueue ;

   class _sdbMsgConvertor
   {
   public:
      _sdbMsgConvertor() ;
      ~_sdbMsgConvertor() ;

   public:
      INT32 downgradeRequest( const CHAR *pRequestHeader,
                              INT32 requestHeaderLength,
                              CHAR *&outputData,
                              INT32 &outputDataLength ) ;
      INT32 upgradeReply( const CHAR *replyMsg,
                          CHAR *&outputData,
                          INT32 &outputDataLength ) ;

   private:
      INT32 _ensureBuff( INT32 size ) ;

   private:
      CHAR*       _buff ;
      INT32       _buffSize ;
   } ;
   typedef class _sdbMsgConvertor sdbMsgConvertor ;
}

#endif /* IMP_RECORD_QUEUE_HPP__ */

