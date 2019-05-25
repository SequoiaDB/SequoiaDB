/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnContext.cpp

   Descriptive Name = Runtime Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context helper
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnContext.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   #define RTN_CONTEXT_MAX_BUFF_SIZE         ( 5 * RTN_RESULTBUFFER_SIZE_MAX )
   #define RTN_CTX_PREPARE_MORE_DATA_INIT    (1024 * 4)     /* 4KB */
   #define RTN_CTX_PREPARE_MORE_DATA_MAX     (1024 * 512)   /* 512KB */

   _rtnContextStoreBuf::_rtnContextStoreBuf()
   {
      _buffer = NULL ;
      _numRecords = 0 ;
      _bufferSize = 0 ;
      _readOffset = 0 ;
      _writeOffset = 0 ;
      _countOnly = FALSE ;
   }

   _rtnContextStoreBuf::~_rtnContextStoreBuf()
   {
      release() ;
   }

   INT32 _rtnContextStoreBuf::_ensureBufferSize( INT32 ensuredSize )
   {
      INT32 rc = SDB_OK ;
      CHAR* oldBuffer = _buffer ;
      INT32 newSize ;

      if ( ensuredSize <= _bufferSize )
      {
         goto done ;
      }

      newSize = ( _bufferSize == 0 ) ? RTN_DFT_BUFFERSIZE : _bufferSize ;

      while ( newSize < ensuredSize )
      {
         if ( newSize >= RTN_CONTEXT_MAX_BUFF_SIZE )
         {
            PD_LOG ( PDERROR, "Result buffer is greater than %d bytes",
                     RTN_CONTEXT_MAX_BUFF_SIZE ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         newSize = newSize << 1 ;
         if (newSize > RTN_CONTEXT_MAX_BUFF_SIZE )
         {
            newSize = RTN_CONTEXT_MAX_BUFF_SIZE ;
         }
      }

      if ( NULL == _buffer )
      {
         _buffer = ( CHAR* )SDB_OSS_MALLOC(
                                 RTN_BUFF_TO_PTR_SIZE( newSize ) ) ;
      }
      else
      {
         _buffer = (CHAR*)SDB_OSS_REALLOC(
                                 RTN_BUFF_TO_REAL_PTR( _buffer ),
                                 RTN_BUFF_TO_PTR_SIZE( newSize ) ) ;
      }

      if ( NULL == _buffer )
      {
         PD_LOG ( PDERROR, "Unable to allocate buffer for %d bytes",
                  newSize ) ;
         _buffer = oldBuffer ;
         rc = SDB_OOM ;
         goto error ;
      }

      _buffer = RTN_REAL_PTR_TO_BUFF( _buffer ) ;
      _bufferSize = newSize ;

      if ( NULL == oldBuffer )
      {
         *RTN_GET_REFERENCE( _buffer ) = 0 ;
         *RTN_GET_CONTEXT_FLAG( _buffer ) = 1 ;
      }

   done:
      return rc;
   error:
      goto done ;
   }

   INT32 _rtnContextStoreBuf::append( const BSONObj& obj )
   {
      INT32 rc = SDB_OK ;

      if ( !isCountMode() )
      {
         _writeOffset = ossAlign4( (UINT32)_writeOffset ) ;
         if ( _writeOffset + obj.objsize () > _bufferSize )
         {
            rc = _ensureBufferSize ( _writeOffset + obj.objsize() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to reallocate buffer for context, rc: "
                        "%d", rc ) ;
               goto error ;
            }
         }

         ossMemcpy ( &(_buffer[_writeOffset]), obj.objdata(), obj.objsize() ) ;
         _writeOffset += obj.objsize() ;
      }

      _numRecords++ ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextStoreBuf::appendObjs( const CHAR* objBuf,
                                             INT32 len,
                                             INT32 num,
                                             BOOLEAN needAligned )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( len >= 0, "len should >= 0" ) ;
      SDB_ASSERT( num >= 0, "num should >= 0" ) ;

      if ( !isCountMode() )
      {
         if ( len > 0 )
         {
            if ( needAligned )
            {
               _writeOffset = ossAlign4( (UINT32)_writeOffset ) ;
            }

            if ( _writeOffset + len > _bufferSize )
            {
               rc = _ensureBufferSize ( _writeOffset + len ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG ( PDERROR, "Failed to reallocate buffer for context, "
                           "rc: %d", rc ) ;
                  goto error ;
               }
            }

            ossMemcpy ( &(_buffer[_writeOffset]), objBuf, len ) ;
            _writeOffset += len ;
         }
      }

      _numRecords += num ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextStoreBuf::get( INT32 maxNumToReturn,
                                   rtnContextBuf& buf )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( !isEmpty(), "inner buf is empty" ) ;

      buf.release() ;

      if ( !isCountMode() )
      {
         _readOffset = ossAlign4( (UINT32)_readOffset ) ;
         buf._pOrgBuff = _buffer ;
         buf._pBuff = &_buffer[ _readOffset ] ;

         if ( maxNumToReturn < 0 )
         {
            buf._buffSize = _writeOffset - _readOffset ;
            buf._recordNum = _numRecords ;
            _readOffset = _writeOffset ;
            _numRecords = 0 ;
         }
         else
         {
            INT32 prevCurOffset = _readOffset ;
            while ( _readOffset < _writeOffset &&
                    maxNumToReturn > 0 )
            {
               try
               {
                  BSONObj obj( &_buffer[_readOffset] ) ;
                  _readOffset += ossAlign4( (UINT32)obj.objsize() ) ;
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "Can't convert into BSON object: %s",
                          e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               buf._recordNum++ ;
               _numRecords-- ;
               maxNumToReturn-- ;
            } // end while

            if ( _readOffset > _writeOffset )
            {
               _readOffset = _writeOffset ;
               SDB_ASSERT( 0 == _numRecords, "buffer num records must "
                           " be zero" ) ;
            }
            buf._buffSize = _readOffset - prevCurOffset ;
         }
      }
      else
      {
         if ( maxNumToReturn < 0 || maxNumToReturn >= _numRecords )
         {
            buf._recordNum = _numRecords ;
            _numRecords = 0 ;
         }
         else
         {
            buf._recordNum = maxNumToReturn ;
            _numRecords -= maxNumToReturn ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextStoreBuf::release()
   {
      if ( NULL != _buffer )
      {
         *RTN_GET_CONTEXT_FLAG( _buffer ) = 0 ;

         if ( *RTN_GET_REFERENCE( _buffer ) == 0 )
         {
            SDB_OSS_FREE( RTN_BUFF_TO_REAL_PTR( _buffer ) ) ;
            _buffer = NULL ;
         }
      }
   }

   /*
      Functions
   */
   const CHAR* getContextTypeDesp( RTN_CONTEXT_TYPE type )
   {
      const _rtnContextInfo* info = sdbGetRTNContextBuilder()->find( type ) ;
      if ( NULL != info )
      {
         SDB_ASSERT( type == info->type, "invalid context info" ) ;
         SDB_ASSERT( NULL != info->newFunc, "null pointer of newFunc" ) ;

         return info->name.c_str() ;
      }
      else
      {
         return "UNKNOW" ;
      }
   }

   /*
      _rtnContextBase implement
   */
   _rtnContextBase::_rtnContextBase( INT64 contextID, UINT64 eduID )
   : _waitPrefetchNum( 0 )
   {
      _contextID           = contextID ;
      _eduID               = eduID ;

      _totalRecords        = 0 ;

      _hitEnd              = TRUE ;
      _isOpened            = FALSE ;

      _prefetchID          = 0 ;
      _isInPrefetch        = FALSE ;
      _prefetchRet         = SDB_OK ;
      _pPrefWatcher        = NULL ;
      _pMonAppCB           = NULL ;

      _countOnly           = FALSE ;
      _pDpsCB              = NULL ;
      _w                   = 1 ;

      _canPrepareMore      = FALSE ;
      _prepareMoreDataLimit = RTN_CTX_PREPARE_MORE_DATA_INIT ;

      _enableMonContext    = FALSE ;
      _enableQueryActivity = FALSE ;

      _monCtxCB.setContextID( contextID ) ;
   }

   _rtnContextBase::~_rtnContextBase()
   {
      _close() ;

      if ( _buffer.hasMem() )
      {
         *( _buffer.getContextFlag() ) = 0 ;
         if ( _buffer.getRefCount() != 0 )
         {
            _dataLock.release_r() ;
         }
      }

      _prefetchLock.lock_w() ;
      _prefetchLock.release_w() ;

      _pPrefWatcher = NULL ;

      SDB_ASSERT( 0 == _waitPrefetchNum.peek(), "Has wait prefetch jobs" ) ;
      SDB_ASSERT( FALSE == _isInPrefetch, "Has prefetch job run" ) ;
   }

   void _rtnContextBase::waitForPrefetch()
   {
      _close() ;

      if ( _canPrefetch() )
      {
         _dataLock.lock_r() ;
         _dataLock.release_r() ;
      }
   }

   void _rtnContextBase::setWriteInfo( SDB_DPSCB *dpsCB, INT16 w )
   {
      _pDpsCB  = dpsCB ;
      _w       = w ;
   }

   INT32 _rtnContextBase::getReference() const
   {
      return _buffer.getRefCount() ;
   }

   void _rtnContextBase::enablePrefetch( _pmdEDUCB * cb,
                                         rtnPrefWatcher *pWatcher )
   {
      _prefetchID = 1 ;
      _pPrefWatcher = pWatcher ;
      _pMonAppCB = cb->getMonAppCB() ;
   }

   string _rtnContextBase::toString()
   {
      stringstream ss ;

      ss << "IsOpened:" << ( _isOpened ? 1 : 0 )
         << ",HitEnd:" << ( _hitEnd ? 1 : 0 )
         << ",BufferSize:" << _buffer.bufferSize() ;

      if ( _totalRecords > 0 )
      {
         ss << ",TotalRecordNum:" << _totalRecords ;
      }

      if ( _buffer.numRecords() > 0 )
      {
         ss << ",BufferRecordNum:" << _buffer.numRecords() ;
      }

      if ( isOpened() )
      {
         _toString( ss ) ;
      }

      return ss.str() ;
   }

   INT32 _rtnContextBase::append( const BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      if ( !_isOpened )
      {
         _totalRecords = 0 ;
         _isOpened = TRUE ;
      }

      rc = _buffer.append( result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to append obj to context buffer, rc: "
                           "%d", rc ) ;
         goto error ;
      }

      _totalRecords++ ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::appendObjs( const CHAR * pObjBuff, INT32 len,
                                      INT32 num, BOOLEAN needAliened )
   {
      INT32 rc = SDB_OK ;

      if ( !_isOpened )
      {
         _isOpened = TRUE ;
      }

      rc = _buffer.appendObjs( pObjBuff, len, num, needAliened ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to append objs to context buffer, rc: "
                           "%d", rc ) ;
         goto error ;
      }

      _totalRecords += num ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextBase::_onDataEmpty ()
   {
      if ( _canPrefetch() && 0 != _prefetchID )
      {
         SDB_BPSCB *bpsCB = pmdGetKRCB()->getBPSCB() ;
         if ( bpsCB->isPrefetchEnabled() )
         {
            _prefetchLock.lock_r() ;
            if ( SDB_OK == bpsCB->sendPrefechReq( bpsDataPref( _prefetchID,
                                                               this ) ) )
            {
               _waitPrefetchNum.inc() ;
            }
            else
            {
               _prefetchLock.release_r() ;
            }
         }
      }
   }

   INT32 _rtnContextBase::prefetch( pmdEDUCB * cb, UINT32 prefetchID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN againTry = FALSE ;
      UINT32 timeout = 0 ;

      while ( timeout < OSS_ONE_SEC )
      {
         if ( prefetchID != _prefetchID )
         {
            goto done ;
         }
         rc = _dataLock.lock_w( 100 ) ;
         if ( SDB_OK == rc )
         {
            locked = TRUE ;
            _isInPrefetch = TRUE ;
            if ( _pPrefWatcher )
            {
               _pPrefWatcher->ntyBegin() ;
            }
            break ;
         }
         else if ( rc && SDB_TIMEOUT != rc )
         {
            goto error ;
         }
         timeout += 100 ;
      }

      if ( FALSE == locked )
      {
         goto error ;
      }

      if ( prefetchID != _prefetchID )
      {
         goto done ;
      }

      if ( !isOpened() || eof() || !isEmpty() )
      {
         goto done ;
      }

      if ( _pMonAppCB && cb->getID() != eduID() )
      {
         cb->getMonAppCB()->reset() ;
      }
      rc = _prepareDataMonitor( cb ) ;
      _prefetchRet = rc ;
      if ( rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDWARNING, "Prepare data failed, rc: %d", rc ) ;
      }

      if ( _pMonAppCB && cb->getID() != eduID() )
      {
         *_pMonAppCB += *cb->getMonAppCB() ;
         _monCtxCB.monDataReadInc( cb->getMonAppCB()->totalDataRead ) ;
         cb->getMonAppCB()->reset() ;
      }

      if ( SDB_OK == rc && isEmpty() && isOpened() && !eof() &&
           SDB_OK == pmdGetKRCB()->getBPSCB()->sendPrefechReq(
                     bpsDataPref( ++_prefetchID, this ), TRUE ) )
      {
         _waitPrefetchNum.inc() ;
         againTry = TRUE ;
      }

   done:
      pmdGetKRCB()->getBPSCB()->_idlePrefAgentNum.inc() ;
      if ( locked )
      {
         _isInPrefetch = FALSE ;
         if ( _pPrefWatcher )
         {
            _pPrefWatcher->ntyEnd() ;
         }
         _dataLock.release_w() ;
      }
      _waitPrefetchNum.dec() ;
      if ( FALSE == againTry )
      {
         _prefetchLock.release_r() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::_prepareMoreData( _pmdEDUCB *cb )
   {
      const UINT32 PREPARE_TIMEOUT = 1000 ; // 1ms

      INT32 rc = SDB_OK ;
      UINT64 beginTime ;

      SDB_ASSERT( isEmpty(), "buf is not empty" ) ;

      beginTime = ossGetCurrentMicroseconds() ;

      while ( !eof() )
      {
         INT32 startOffset = _buffer.writeOffset() ;
         INT32 currentPreparedSize ;
         UINT64 currentTime ;

         rc = _prepareDataMonitor( cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         currentPreparedSize = _buffer.writeOffset() - startOffset ;

         if ( _buffer.writeOffset() + currentPreparedSize >= _prepareMoreDataLimit )
         {
            break ;
         }

         currentTime = ossGetCurrentMicroseconds() ;
         if ( currentTime - beginTime >= PREPARE_TIMEOUT )
         {
            break ;
         }
      }

      if ( _prepareMoreDataLimit < RTN_CTX_PREPARE_MORE_DATA_MAX )
      {
         _prepareMoreDataLimit *= 2 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::_prepareDataMonitor ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( enabledMonContext() )
      {
         pmdKRCB *krcb = pmdGetKRCB() ;
         ossTick startTime = krcb->getCurTime() ;

         rc = _prepareData( cb ) ;

         ossTick endTime = krcb->getCurTime() ;
         _monCtxCB.monQueryTimeInc( startTime, endTime ) ;
      }
      else
      {
         rc = _prepareData( cb ) ;
      }

      return rc ;
   }

   INT32 _rtnContextBase::_getBuffer ( INT32 maxNumToReturn,
                                       rtnContextBuf& buf )
   {
      return _buffer.get( maxNumToReturn, buf ) ;
   }

   INT32 _rtnContextBase::getMore( INT32 maxNumToReturn,
                                   rtnContextBuf &buffObj,
                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      buffObj.release() ;

      if ( !isOpened() )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      else if ( eof() && isEmpty() )
      {
         rc = SDB_DMS_EOC ;
         _isOpened = FALSE ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = _dataLock.lock_r( OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            locked = TRUE ;
            break ;
         }
         else if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      if ( 0 != _prefetchID )
      {
         ++_prefetchID ;
      }
      if ( _prefetchRet && SDB_DMS_EOC != _prefetchRet )
      {
         rc = _prefetchRet ;
         PD_LOG( PDWARNING, "Occur error in prefetch, rc: %d", rc ) ;
         goto error ;
      }

      if ( isEmpty() && !eof() )
      {
         UINT64 tmpTotalRead = cb->getMonAppCB()->totalDataRead ;

         if ( _canPrepareMoreData() )
         {
            rc = _prepareMoreData( cb ) ;
         }
         else
         {
            rc = _prepareDataMonitor( cb ) ;
         }

         if ( rc && SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare data, rc: %d", rc ) ;
            goto error ;
         }

         _monCtxCB.monDataReadInc( cb->getMonAppCB()->totalDataRead -
                                   tmpTotalRead ) ;
      }

      if ( !isEmpty() )
      {
         INT64 numRecords = _buffer.numRecords() ;

         _monCtxCB.monReturnInc( 1, numRecords ) ;

         rc = _buffer.get( maxNumToReturn, buffObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get objs from context buffer: %d", rc ) ;
            goto error ;
         }

         if ( !isCountMode() )
         {
            buffObj._startFrom = _totalRecords - numRecords ;
            buffObj._reference( _buffer.getRefCountPointer(), &_dataLock ) ;
            locked = FALSE ;

            if ( isEmpty() && !eof() )
            {
               _buffer.empty() ;
               _onDataEmpty() ;
            }
         }
      }
      else
      {
         rc = SDB_DMS_EOC ;
         _isOpened = FALSE ;
      }

   done:
      if ( locked )
      {
         _dataLock.release_r() ;
      }
      if ( _hitEnd || SDB_DMS_EOC == rc )
      {
         setQueryActivity( TRUE ) ;
      }

      return rc ;
   error:
      goto done ;
   }

   UINT32 _rtnContextBase::getCachedRecordNum()
   {
      return _buffer.numRecords() ;
   }

   _rtnContextAssit::_rtnContextAssit( RTN_CONTEXT_TYPE type,
                                            std::string name,
                                            RTN_CTX_NEW_FUNC func )
   {
      SDB_ASSERT( NULL != func, "func is null" ) ;
      SDB_ASSERT( !name.empty(), "name is empty" ) ;

      sdbGetRTNContextBuilder()->_register( type, name, func ) ;
   }

   _rtnContextAssit::~_rtnContextAssit()
   {
   }

   _rtnContextBuilder::_rtnContextBuilder()
   {
   }

   _rtnContextBuilder::~_rtnContextBuilder()
   {
      _releaseContextInfos() ;
   }

   _rtnContextBase* _rtnContextBuilder::create ( RTN_CONTEXT_TYPE type,
                                                   INT64 contextId,
                                                   EDUID eduId )
   {
      const _rtnContextInfo* info = find( type ) ;
      if ( NULL != info )
      {
         SDB_ASSERT( type == info->type, "invalid context info" ) ;
         SDB_ASSERT( NULL != info->newFunc, "null pointer of newFunc" ) ;

         _rtnContextBase* ctx = (*(info->newFunc))( contextId, eduId ) ;
         SDB_ASSERT( ctx->name() == info->name, "name is wrong" ) ;
         SDB_ASSERT( ctx->getType() == info->type, "type is wrong" ) ;
         return ctx ;
      }
      else
      {
         SDB_ASSERT( FALSE, "unknown RTN_CONTEXT_TYPE" ) ;
         return NULL ;
      }
   }

   void _rtnContextBuilder::release ( _rtnContextBase* context )
   {
      if ( NULL != context)
      {
         SDB_OSS_DEL context ;
      }
   }

   const _rtnContextInfo* _rtnContextBuilder::find( RTN_CONTEXT_TYPE type ) const
   {
      ctx_info_iterator it = _contextInfoMap.find( type ) ;
      if ( it != _contextInfoMap.end() )
      {
         return (*it).second ;
      }
      else
      {
         return NULL ;
      }
   }

   INT32 _rtnContextBuilder::_register( RTN_CONTEXT_TYPE type,
                                       std::string name,
                                       RTN_CTX_NEW_FUNC func )
   {
      INT32 rc = SDB_OK ;
      _rtnContextInfo* newInfo = NULL ;

      const _rtnContextInfo* info = find( type ) ;
      if ( NULL != info )
      {
         PD_LOG( PDERROR, "RTN context info is registered: type=%d, name=%s",
                 info->type, info->name.c_str() ) ;
         SDB_ASSERT( FALSE, "RTN context info is registered" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      newInfo = SDB_OSS_NEW _rtnContextInfo() ;
      if ( NULL == newInfo )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      newInfo->type = type ;
      newInfo->name = name ;
      newInfo->newFunc = func ;

      rc = _insert( newInfo ) ;
      if ( SDB_OK != rc )
      {
         SDB_OSS_DEL newInfo ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBuilder::_insert( _rtnContextInfo* contextInfo )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != contextInfo, "contextInfo is null") ;

      try
      {
         _contextInfoMap.insert( pair_type( contextInfo->type, contextInfo ) ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG(PDERROR, "unexpected error happened: %s", e.what());
         rc = SDB_SYS ;
      }

      return rc ;
   }

   void _rtnContextBuilder::_releaseContextInfos()
   {
      for ( ctx_info_iterator it = _contextInfoMap.begin() ;
            it != _contextInfoMap.end() ;
            it++ )
      {
         _rtnContextInfo* info = (*it).second ;
         SDB_ASSERT( NULL != info, "info is null" ) ;
         SDB_OSS_DEL info ;
      }

      _contextInfoMap.clear() ;
   }

   _rtnContextBuilder* sdbGetRTNContextBuilder()
   {
      static _rtnContextBuilder ctxBuilder ;
      return &ctxBuilder ;
   }

   /*
      _rtnSubContextHolder implement
    */
   _rtnSubContextHolder::_rtnSubContextHolder ()
   : _subCB( NULL ),
     _subContext( NULL ),
     _subContextID( -1 )
   {
   }

   _rtnSubContextHolder::~_rtnSubContextHolder ()
   {
      _deleteSubContext() ;
   }

   void _rtnSubContextHolder::_deleteSubContext ()
   {
      if ( -1 != _subContextID )
      {
         sdbGetRTNCB()->contextDelete( _subContextID, _subCB ) ;
         _subContext = NULL ;
         _subCB = NULL ;
         _subContextID = -1 ;
      }
   }

   void _rtnSubContextHolder::_setSubContext ( rtnContext *subContext,
                                               pmdEDUCB *subCB )
   {
      _deleteSubContext() ;
      if ( NULL != subContext )
      {
         _subContext = subContext ;
         _subContextID = subContext->contextID() ;
         _subCB = subCB ;
      }
   }

}
