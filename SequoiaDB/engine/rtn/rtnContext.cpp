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
   // minimum timeout for prepare more 4ms
   #define RTN_CTX_PREPARE_MORE_TIME_INIT    ( 4000 )
   // maximum timeout for prepare more 512ms
   #define RTN_CTX_PREPARE_MORE_TIME_MAX     ( 512000 )

   _rtnContextStoreBuf::_rtnContextStoreBuf()
   {
      _buffer = NULL ;
      _numRecords = 0 ;
      _bufferSize = 0 ;
      _readOffset = 0 ;
      _writeOffset = 0 ;
      _countOnly = FALSE ;
      _contextValidator = NULL ;
   }

   _rtnContextStoreBuf::~_rtnContextStoreBuf()
   {
      release() ;
   }

   INT32 _rtnContextStoreBuf::_ensureBufferSize( INT32 ensuredSize )
   {
      INT32 rc = SDB_OK ;
      CHAR* oldBuffer = _buffer ;
      INT32 newSize = 0 ;
      BOOLEAN isReferenced = FALSE ;

      if ( ensuredSize <= _bufferSize )
      {
         goto done ;
      }

      newSize = ( _bufferSize == 0 ) ? RTN_DFT_BUFFERSIZE : _bufferSize ;

      // make sure we get enough memory in result buffer
      while ( newSize < ensuredSize )
      {
         // make sure we haven't hit max
         if ( newSize >= RTN_CONTEXT_MAX_BUFF_SIZE )
         {
            PD_LOG ( PDERROR, "Result buffer is greater than %d bytes",
                     RTN_CONTEXT_MAX_BUFF_SIZE ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         // double buffer size until hitting RTN_CONTEXT_MAX_BUFF_SIZE
         newSize = newSize << 1 ;
         if (newSize > RTN_CONTEXT_MAX_BUFF_SIZE )
         {
            newSize = RTN_CONTEXT_MAX_BUFF_SIZE ;
         }
      }

      if ( NULL == _buffer )
      {
         _buffer = ( CHAR* )SDB_THREAD_ALLOC(
                                 RTN_BUFF_TO_PTR_SIZE( newSize ) ) ;
      }
      else if ( 0 == *RTN_GET_REFERENCE( _buffer ) )
      {
         // reallocate memory
         _buffer = (CHAR*)SDB_THREAD_REALLOC(
                                 RTN_BUFF_TO_REAL_PTR( _buffer ),
                                 RTN_BUFF_TO_PTR_SIZE( newSize ) ) ;
      }
      else
      {
         // has reference, need leave the old buffer to referencer
         _buffer = ( CHAR* )SDB_THREAD_ALLOC(
                                 RTN_BUFF_TO_PTR_SIZE( newSize ) ) ;
         isReferenced = TRUE ;
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

      if ( NULL == oldBuffer )
      {
         *RTN_GET_REFERENCE( _buffer ) = 0 ;
         *RTN_GET_CONTEXT_FLAG( _buffer ) = 1 ;
      }
      else if ( isReferenced )
      {
         // copy old contents
         ossMemcpy( _buffer, oldBuffer, _bufferSize ) ;

         // transfer the ownerships of old buffer to referencer
         *RTN_GET_CONTEXT_FLAG( oldBuffer ) = 0 ;

         // reset new allocated buffer
         *RTN_GET_REFERENCE( _buffer ) = 0 ;
         *RTN_GET_CONTEXT_FLAG( _buffer ) = 1 ;
      }

      _bufferSize = newSize ;

   done:
      return rc;
   error:
      goto done ;
   }

   INT32 _rtnContextStoreBuf::append( const BSONObj& obj,
                                      const BSONObj *orgObj )
   {
      INT32 rc = SDB_OK ;

      if ( !isCountMode() )
      {
         if ( _contextValidator )
         {
            if ( !orgObj )
            {
               orgObj = &obj ;
            }
            rc = _contextValidator->validate( *orgObj ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to validate record, rc: %d", rc ) ;
               goto error ;
            }
         }

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

   INT32 _rtnContextStoreBuf::pushFronts( const CHAR *objBuf,
                                          INT32 len,
                                          INT32 num )
   {
      INT32 rc = SDB_OK ;

      if ( len < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !isCountMode() )
      {
         INT32 alignedSize = ossAlign4( len ) ;
         if ( _readOffset < alignedSize )
         {
            rc = _ensureBufferSize( _writeOffset + alignedSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reallocate buffer for context, rc: "
                       "%d", rc ) ;
               goto error ;
            }
            ossMemmove( &(_buffer[alignedSize]), &(_buffer[_readOffset]),
                        _writeOffset - _readOffset ) ;
            _readOffset = alignedSize ;
            _writeOffset += alignedSize ;
         }

         _readOffset -= alignedSize ;
         ossMemcpy( &(_buffer[_readOffset]), objBuf, len ) ;
      }

      _numRecords += num ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextStoreBuf::pushFront( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( !isCountMode() )
      {
         INT32 alignedSize = ossAlign4( (UINT32)obj.objsize() ) ;
         if ( _readOffset < alignedSize )
         {
            rc = _ensureBufferSize( _writeOffset + alignedSize ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reallocate buffer for context, rc: "
                       "%d", rc ) ;
               goto error ;
            }
            ossMemmove( &(_buffer[alignedSize]), &(_buffer[_readOffset]),
                        _writeOffset - _readOffset ) ;
            _readOffset = alignedSize ;
            _writeOffset += alignedSize ;
         }

         _readOffset -= alignedSize ;

         // if from the same position, no need to copy
         if ( (const CHAR *)( obj.objdata() ) != (const CHAR *)( &(_buffer[_readOffset]) ) )
         {
            ossMemcpy( &(_buffer[_readOffset]), obj.objdata(), obj.objsize() ) ;
         }
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
                                   rtnContextBuf& buf,
                                   BOOLEAN onlyPeek )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( !isEmpty(), "inner buf is empty" ) ;

      buf.release() ;

      if ( !isCountMode() )
      {
         _readOffset = ossAlign4( (UINT32)_readOffset ) ;
         buf._pOrgBuff = _buffer ;
         buf._pBuff = &_buffer[ _readOffset ] ;
         //buf._startFrom = _totalRecords - _numRecords ;

         // return current all records
         if ( maxNumToReturn < 0 )
         {
            buf._buffSize = _writeOffset - _readOffset ;
            buf._recordNum = _numRecords ;

            if ( !onlyPeek )
            {
               _readOffset = _writeOffset ;
               _numRecords = 0 ;
            }
         }
         else
         {
            INT32 prevCurOffset = _readOffset ;
            INT32 tmpReadOffset = _readOffset ;
            while ( tmpReadOffset < _writeOffset &&
                    maxNumToReturn > 0 )
            {
               try
               {
                  BSONObj obj( &_buffer[_readOffset] ) ;
                  tmpReadOffset += ossAlign4( (UINT32)obj.objsize() ) ;
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "Can't convert into BSON object: %s",
                          e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               buf._recordNum++ ;
               maxNumToReturn-- ;

               if ( !onlyPeek )
               {
                  _readOffset = tmpReadOffset ;
                  _numRecords-- ;
               }
            } // end while

            if ( _readOffset > _writeOffset )
            {
               _readOffset = _writeOffset ;
               SDB_ASSERT( 0 == _numRecords, "buffer num records must "
                           " be zero" ) ;
            }
            buf._buffSize = tmpReadOffset - prevCurOffset ;
         }
      }
      else
      {
         if ( maxNumToReturn < 0 || maxNumToReturn >= _numRecords )
         {
            buf._recordNum = _numRecords ;
            if ( !onlyPeek )
            {
               _numRecords = 0 ;
            }
         }
         else
         {
            buf._recordNum = maxNumToReturn ;
            if ( !onlyPeek )
            {
               _numRecords -= maxNumToReturn ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextStoreBuf::pop( UINT32 num )
   {
      INT32 rc = SDB_OK ;

      if ( !isCountMode() )
      {
         try
         {
            while( _numRecords > 0 && num > 0 )
            {
               BSONObj obj( &_buffer[_readOffset] ) ;
               _readOffset += ossAlign4( (UINT32)obj.objsize() ) ;
               --_numRecords ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         if ( _numRecords > num )
         {
            _numRecords -= num ;
         }
         else
         {
            _numRecords = 0 ;
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
            CHAR *pRealPtr = RTN_BUFF_TO_REAL_PTR( _buffer ) ;
            SDB_THREAD_FREE( pRealPtr ) ;
            _buffer = NULL ;
         }
      }
   }

   void _rtnContextStoreBuf::setContextValidator(
                             _rtnContextValidator *contextValidator )
   {
      _contextValidator = contextValidator ;
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
      _opID                = 0 ;

      _totalRecords        = 0 ;

      _hitEnd              = TRUE ;
      _isOpened            = FALSE ;
      _preHitEnd           = FALSE ;

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
      _prepareMoreTimeLimit = RTN_CTX_PREPARE_MORE_TIME_INIT ;

      _enableMonContext    = FALSE ;
      _enableQueryActivity = FALSE ;

      _isTransCtx          = FALSE ;
      _monQueryCB          = NULL ;
      _monCtxCB.setContextID( contextID ) ;

      _isAffectGIndex      = FALSE ;

      _lastProcessTick     = pmdGetDBTick() ;
      _needTimeout         = TRUE ;
      _needCloseOnEOF      = FALSE ;

      _buffer.setContextValidator( this ) ;
   }

   _rtnContextBase::~_rtnContextBase()
   {
      _close() ;

      /// wait prefetch complete
      _prefetchID = 0 ;
      if ( _prefetchLock.get() )
      {
         _prefetchLock->lock_w() ;
         _prefetchLock->release_w() ;
      }
      _pPrefWatcher = NULL ;

      if ( _buffer.hasMem() )
      {
         *( _buffer.getContextFlag() ) = 0 ;
         if ( _buffer.getRefCount() != 0 )
         {
            _dataLock.release_r() ;
         }
      }

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

   void _rtnContextBase::setTransContext( BOOLEAN transCtx )
   {
      _isTransCtx = transCtx ;
   }

   BOOLEAN _rtnContextBase::isTransContext() const
   {
      return _isTransCtx ;
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
         << ",IsTrans:" << ( _isTransCtx ? 1 : 0 )
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

   INT32 _rtnContextBase::append( const BSONObj &result,
                                  const BSONObj *orgResult )
   {
      INT32 rc = SDB_OK ;

      if ( !_isOpened )
      {
         _totalRecords = 0 ;
         _isOpened = TRUE ;
      }

      rc = _buffer.append( result, orgResult ) ;
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
            if ( !_prefetchLock.get() )
            {
               ossRWMutex *pMutex = SDB_OSS_NEW ossRWMutex() ;
               if ( !pMutex )
               {
                  goto done ;
               }
               _prefetchLock = ctxMutexPtr( pMutex ) ;
            }

            _prefetchLock->lock_r() ;
            if ( SDB_OK == bpsCB->sendPrefechReq( bpsDataPref( _prefetchID,
                                                               this ) ) )
            {
               _waitPrefetchNum.inc() ;
            }
            else
            {
               _prefetchLock->release_r() ;
            }
         }
      }

   done:
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXBASE_PREFETCH, "_rtnContextBase::prefetch" )
   INT32 _rtnContextBase::prefetch( pmdEDUCB * cb, UINT32 prefetchID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXBASE_PREFETCH ) ;

      BOOLEAN locked = FALSE ;
      BOOLEAN againTry = FALSE ;
      UINT32 timeout = 0 ;
      monSvcTaskInfo *pOldInfo = NULL ;
      pdLogShield logShield ;

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
         /// save task info
         pOldInfo = cb->getMonAppCB()->getSvcTaskInfo() ;
         cb->getMonAppCB()->setSvcTaskInfo( _pMonAppCB->getSvcTaskInfo() ) ;
      }

      logShield.addRC( SDB_IXM_ADVANCE_EOC ) ;

      while ( TRUE )
      {
         if ( _canPrepareMoreData() )
         {
            rc = _prepareMoreData( cb ) ;
         }
         else
         {
            rc = _prepareDataMonitor( cb ) ;
         }

         // For Data node: cl.query.sort(...).hint("$Range":{ ... })
         if ( rc == SDB_IXM_ADVANCE_EOC )
         {
            pdClearLastError() ;
            rc = _prepareDoAdvance( cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               break ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Prepare do advance failed, rc: %d", rc ) ;
               break ;
            }
         }
         else
         {
            break ;
         }
      }

      _prefetchRet = rc ;
      if ( rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDWARNING, "Prepare data failed, rc: %d", rc ) ;
      }

      if ( _pMonAppCB && cb->getID() != eduID() )
      {
         // merge monitor counts from prefetch thread
         *_pMonAppCB += *cb->getMonAppCB() ;
         _monCtxCB.monDataReadInc( cb->getMonAppCB()->totalDataRead ) ;
         _monCtxCB.monIndexReadInc( cb->getMonAppCB()->totalIndexRead ) ;
         cb->getMonAppCB()->reset() ;
         /// restore task info
         cb->getMonAppCB()->setSvcTaskInfo( pOldInfo ) ;
      }

      if ( SDB_OK == rc && isEmpty() && isOpened() && !eof() &&
           SDB_OK == pmdGetKRCB()->getBPSCB()->sendPrefechReq(
                     bpsDataPref( ++_prefetchID, this ), TRUE ) )
      {
         _waitPrefetchNum.inc() ;
         againTry = TRUE ;
      }

   done:
      // inc idle
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
         ctxMutexPtr tmpMutexPtr( _prefetchLock ) ;
         tmpMutexPtr->release_r() ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCTXBASE_PREFETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::_prepareMoreData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT64 beginTime ;

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

         // assume next prepared size equals current,
         // so break if data size exceeds limit when prepare once more time
         if ( _buffer.writeOffset() + currentPreparedSize >= _prepareMoreDataLimit )
         {
            break ;
         }

         // prepare timeout
         currentTime = ossGetCurrentMicroseconds() ;
         if ( currentTime - beginTime >= (UINT64)_prepareMoreTimeLimit )
         {
            break ;
         }
      }

      if ( _prepareMoreDataLimit < RTN_CTX_PREPARE_MORE_DATA_MAX )
      {
         _prepareMoreDataLimit *= 2 ;
      }
      if ( _prepareMoreTimeLimit < RTN_CTX_PREPARE_MORE_TIME_MAX )
      {
         _prepareMoreTimeLimit *= 2 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::_prepareDataMonitor ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isSetAffect = FALSE ;

      if ( isAffectGIndex() && NULL != cb && !cb->isAffectGIndex() )
      {
         cb->setIsAffectGIndex( TRUE ) ;
         isSetAffect = TRUE ;
      }

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

      if ( isSetAffect )
      {
         cb->setIsAffectGIndex( FALSE ) ;
         isSetAffect = FALSE ;
      }

      return rc ;
   }

   INT32 _rtnContextBase::_getBuffer ( INT32 maxNumToReturn,
                                       rtnContextBuf& buf )
   {
      return _buffer.get( maxNumToReturn, buf ) ;
   }

   void _rtnContextBase::updateLastProcessTick()
   {
      // update last process time
      _lastProcessTick = pmdGetDBTick() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXBASE_GETMORE, "_rtnContextBase::getMore" )
   INT32 _rtnContextBase::getMore( INT32 maxNumToReturn,
                                   rtnContextBuf &buffObj,
                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      PD_TRACE_ENTRY ( SDB_RTNCTXBASE_GETMORE ) ;

      // release buff obj
      buffObj.release() ;

      if ( !isOpened() )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      else if ( eof() && isEmpty() )
      {
         _monCtxCB.monReturnInc( 1, 0 ) ;

         rc = SDB_DMS_EOC ;
         _isOpened = FALSE ;
         goto error ;
      }

      // need to get data lock
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
      // check prefetch has error
      if ( _prefetchRet && SDB_DMS_EOC != _prefetchRet )
      {
         rc = _prefetchRet ;
         PD_LOG( PDWARNING, "Occur error in prefetch, rc: %d", rc ) ;
         goto error ;
      }

      // need to get more datas
      if ( isEmpty() && !eof() )
      {
         UINT64 startDataRead = cb->getMonAppCB()->totalDataRead ;
         UINT64 startIndexRead = cb->getMonAppCB()->totalIndexRead ;
         UINT64 startDataWrite = cb->getMonAppCB()->totalDataWrite ;
         UINT64 startIndexWrite = cb->getMonAppCB()->totalIndexWrite ;

         pdLogShield logShield ;
         logShield.addRC( SDB_IXM_ADVANCE_EOC ) ;

         while ( TRUE )
         {
            if ( _canPrepareMoreData() )
            {
               rc = _prepareMoreData( cb ) ;
            }
            else
            {
               rc = _prepareDataMonitor( cb ) ;
            }

            // For Data node: cl.query.sort(...).hint("$Range":{ ... })
            if ( rc == SDB_IXM_ADVANCE_EOC )
            {
               pdClearLastError() ;
               rc = _prepareDoAdvance( cb ) ;
               if ( SDB_DMS_EOC == rc )
               {
                  break ;
               }
               else if ( rc )
               {
                  PD_LOG( PDERROR, "Prepare do advance failed, rc: %d", rc ) ;
                  goto error ;
               }
            }
            else
            {
               break;
            }
         }

         if ( rc && SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare data, rc: %d", rc ) ;
            goto error ;
         }

         _monCtxCB.monDataReadInc( cb->getMonAppCB()->totalDataRead -
                                   startDataRead ) ;
         _monCtxCB.monIndexReadInc( cb->getMonAppCB()->totalIndexRead -
                                    startIndexRead ) ;
         _monCtxCB.monDataWriteInc( cb->getMonAppCB()->totalDataWrite -
                                    startDataWrite ) ;
         _monCtxCB.monIndexWriteInc( cb->getMonAppCB()->totalIndexWrite -
                                     startIndexWrite ) ;
      }

      // if not empty, get current data
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

            // if get all data
            if ( isEmpty() && !eof() )
            {
               if ( _preHitEnd )
               {
                  _hitEnd = _preHitEnd ;
               }
               else
               {
                  _buffer.empty() ;
                  _onDataEmpty() ;
               }
            }
            else if ( !isEmpty() && eof() )
            {
               _preHitEnd = _hitEnd ;
               _hitEnd = FALSE ;
            }
         }
      }
      else
      {
         _monCtxCB.monReturnInc( 1, 0 ) ;

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

      updateLastProcessTick() ;

      PD_TRACE_EXITRC ( SDB_RTNCTXBASE_GETMORE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXBASE__ADVANCE, "_rtnContextBase::_advance" )
   INT32 _rtnContextBase::_advance( const BSONObj &arg,
                                    BOOLEAN isLocate,
                                    _pmdEDUCB *cb,
                                    const CHAR *pBackData,
                                    INT32 backDataSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXBASE__ADVANCE ) ;

      BOOLEAN locked = FALSE ;

      BSONObj orderby ;
      UINT32 tmpPrefetchID = 0 ;
      BOOLEAN finished = FALSE ;
      INT32 type = MSG_ADVANCE_TO_FIRST_IN_VALUE ;
      INT32 prefixNum = 0 ;
      INT32 orderbyFieldNum = 0 ;
      BSONObj keyVal ;
      BSONObj objValue ;
      ixmIndexKeyGen keyGen ;

      if ( !isOpened() )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( eof() && isEmpty() && backDataSize <= 0 )
      {
         rc = SDB_OK ;
         goto done ;
      }

      // not support for count mode
      if ( isCountMode() )
      {
         PD_LOG_MSG( PDERROR, "Context in count mode does not support advance" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }
      else if ( isWrite() )
      {
         PD_LOG_MSG( PDERROR, "Context in write mode does not support advance" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      // check args
      try
      {
         BSONElement eType   = arg.getField( FIELD_NAME_TYPE ) ;
         BSONElement eNum    = arg.getField( FIELD_NAME_PREFIX_NUM ) ;
         BSONElement eVal    = arg.getField( FIELD_NAME_INDEXVALUE ) ;

         if ( arg.isEmpty() )
         {
            PD_LOG_MSG( PDERROR, "Position is empty" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( NumberInt != eType.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Int", FIELD_NAME_TYPE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( NumberInt != eNum.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Int",
                        FIELD_NAME_PREFIX_NUM ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( Object != eVal.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Object",
                        FIELD_NAME_INDEXVALUE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         type = eType.numberInt() ;
         prefixNum = eNum.numberInt() ;
         objValue = eVal.embeddedObject() ;

         // check type
         if ( MSG_ADVANCE_TO_FIRST_IN_VALUE != type &&
              MSG_ADVANCE_TO_NEXT_OUT_VALUE != type )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] is invalid", FIELD_NAME_TYPE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // check prefix number
         else if ( prefixNum <= 0 )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] is invalid",
                        FIELD_NAME_PREFIX_NUM ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _getAdvanceOrderby( orderby ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         BSONObj convertedIndexValue ;

         try
         {
            convertedIndexValue = dotted2nested( objValue ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "An exception occurred when converting "
                    "IndexValue: %s, rc: %d", e.what(), rc ) ;
            goto error ;
         }

         orderbyFieldNum = orderby.nFields() ;
         if ( prefixNum > orderbyFieldNum )
         {
            PD_LOG ( PDWARNING, "PrefixNum[%d] is too long, truncate to "
                     "the same as the order by's field number", prefixNum ) ;
            prefixNum = orderbyFieldNum ;
         }

         /// generate keyVal
         rc = keyGen.setKeyPattern( orderby ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Set index generate pattern failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = keyGen.getKeys( convertedIndexValue, keyVal ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Generate key value failed, rc: %d", rc ) ;
            goto error ;
         }
      }

      // need to get data lock
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
         /// disable prefetch
         tmpPrefetchID = _prefetchID + 1 ;
         _prefetchID = 0 ;
      }
      // check prefetch has error
      if ( _prefetchRet && SDB_DMS_EOC != _prefetchRet )
      {
         rc = _prefetchRet ;
         PD_LOG( PDWARNING, "Occur error in prefetch, rc: %d", rc ) ;
         goto error ;
      }

      if ( locked )
      {
         _dataLock.release_r() ;
         locked = FALSE ;
      }

      /// process back data
      if ( pBackData && backDataSize > 0 )
      {
         rc = _advanceBackData( type, prefixNum, keyGen, keyVal, orderby,
                                pBackData, backDataSize,
                                finished ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( finished )
         {
            goto done ;
         }
      }

      /// process the buffer records
      if ( _buffer.numRecords() > 0 )
      {
         rc = _advanceRecords( type, prefixNum, keyGen, keyVal, orderby,
                               _buffer.numRecords(), cb, finished ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( finished )
         {
            goto done ;
         }
      }

      // First do advance
      rc = _doAdvance( type, prefixNum, keyVal, orderby, arg, isLocate, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( isLocate )
      {
         goto done ;
      }
      // get more
      else
      {
         /// save position
         _advancePosition = arg ;

         rc = _advanceRecords( type, prefixNum, keyGen, keyVal, orderby,
                               -1, cb, finished ) ;
         /// restore
         _advancePosition = BSONObj() ;

         /// check result
         if ( rc )
         {
            goto error ;
         }
         SDB_ASSERT( finished, "Finish must is invalid" ) ;
      }

   done:
      if ( 0 != tmpPrefetchID )
      {
         // enable pretch
         _prefetchID = tmpPrefetchID ;
      }
      if ( locked )
      {
         _dataLock.release_r() ;
      }
      updateLastProcessTick() ;
      PD_TRACE_EXITRC ( SDB_RTNCTXBASE__ADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::advance( const BSONObj &arg,
                                   const CHAR *pBackData,
                                   INT32 backDataSize,
                                   _pmdEDUCB *cb )
   {
      return _advance( arg, FALSE, cb, pBackData, backDataSize ) ;
   }

   INT32 _rtnContextBase::locate( const BSONObj &arg,
                                  _pmdEDUCB *cb )
   {
      return _advance( arg, TRUE, cb ) ;
   }

   INT32 _rtnContextBase::_advanceRecords( INT32 type,
                                           INT32 prefixNum,
                                           ixmIndexKeyGen &keyGen,
                                           const BSONObj &keyVal,
                                           const BSONObj &orderby,
                                           INT64 recordNum,
                                           _pmdEDUCB *cb,
                                           BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      rtnContextBuf buffObj ;
      BSONObj tmpObj ;
      BOOLEAN matched = FALSE ;
      BOOLEAN isEqual = FALSE ;

      while( 0 != recordNum  )
      {
         rc = getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _isOpened = TRUE ;
            finished = TRUE ;
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            goto error ;
         }

         rc = buffObj.nextObj( tmpObj ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = _checkAdvance( type, keyGen, prefixNum, keyVal,
                             tmpObj, orderby, matched, isEqual ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( matched )
         {
            /// push back current
            rc = _buffer.pushFront( tmpObj ) ;
            if ( rc )
            {
               goto error ;
            }
            finished = TRUE ;
            break ;
         }

         if ( recordNum > 0 )
         {
            --recordNum ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::_advanceBackData( INT32 type,
                                            INT32 prefixNum,
                                            ixmIndexKeyGen &keyGen,
                                            const BSONObj &keyVal,
                                            const BSONObj &orderby,
                                            const CHAR *pBackData,
                                            INT32 backDataSize,
                                            BOOLEAN &finished )
   {
      INT32 rc = SDB_OK ;

      INT32 offset = 0 ;
      INT32 recordNum = 0 ;
      BOOLEAN matched = FALSE ;
      BOOLEAN isEqual = FALSE ;

      if ( backDataSize < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         while( offset < backDataSize )
         {
            BSONObj obj( &pBackData[offset] ) ;
            rc = _checkAdvance( type, keyGen, prefixNum, keyVal,
                                obj, orderby, matched, isEqual ) ;
            if ( rc )
            {
               goto error ;
            }
            else if ( matched )
            {
               /// cacl record num
               INT32 alignSize = ossAlign4( backDataSize ) ;
               INT32 tmpOffset = offset ;

               while( tmpOffset < backDataSize )
               {
                  BSONObj tmpObj( &pBackData[tmpOffset] ) ;
                  ++recordNum ;
                  tmpOffset += ossAlign4 ( tmpObj.objsize() ) ;
               }

               if ( tmpOffset != alignSize )
               {
                  PD_LOG_MSG( PDERROR, "Back data length is invalid" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               if ( _buffer.isEmpty() )
               {
                  _buffer.empty() ;
                  rc = _buffer.appendObjs( &pBackData[offset],
                                           backDataSize - offset,
                                           recordNum ) ;
               }
               else
               {
                  /// push back current
                  rc = _buffer.pushFronts( &pBackData[offset],
                                           backDataSize - offset,
                                           recordNum ) ;
               }
               if ( rc )
               {
                  goto error ;
               }
               _hitEnd = FALSE ;
               finished = TRUE ;
               break ;
            }

            offset += ossAlign4( obj.objsize() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXBASE__CHECKADVANCE, "_rtnContextBase::_checkAdvance" )
   INT32 _rtnContextBase::_checkAdvance( INT32 type,
                                         ixmIndexKeyGen &keyGen,
                                         INT32 prefixNum,
                                         const BSONObj &keyVal,
                                         const BSONObj &curObj,
                                         const BSONObj &orderby,
                                         BOOLEAN &matched,
                                         BOOLEAN &isEqual )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXBASE__CHECKADVANCE ) ;

      BSONObj keyObj ;
      INT32 cmp = 0 ;

      try
      {
         rc = keyGen.getKeys( curObj, keyObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Generate key from obj(%s) failed, rc: %d",
                    curObj.toPoolString().c_str(), rc ) ;
            goto error ;
         }

         cmp = _woNCompare( keyVal, keyObj, FALSE, prefixNum, orderby ) ;
         if ( cmp < 0 )
         {
            matched = TRUE ;
         }
         else if ( MSG_ADVANCE_TO_FIRST_IN_VALUE == type && 0 == cmp )
         {
            matched = TRUE ;
         }

         isEqual = ( 0 == cmp ) ? TRUE : FALSE ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCTXBASE__CHECKADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextBase::_woNCompare( const BSONObj &l,
                                       const BSONObj &r,
                                       BOOLEAN compreFieldName,
                                       UINT32 keyNum,
                                       const BSONObj &keyPattern )
   {
      BSONObjIterator itrL( l ) ;
      BSONObjIterator itrR( r ) ;
      BSONObjIterator itrK( keyPattern ) ;
      UINT32 i = 0 ;
      INT32 cmp = 0 ;
      BOOLEAN ordered = !keyPattern.isEmpty() ;

      for ( i = 0 ; i < keyNum && itrL.more() && itrR.more() ; ++i )
      {
         BSONElement eL = itrL.next() ;
         BSONElement eR = itrR.next() ;

         BSONElement eK ;
         if ( ordered )
         {
            if ( itrK.more() )
            {
               eK = itrK.next() ;
            }
            else
            {
               SDB_ASSERT( FALSE, "Key pattern is invalid" ) ;
               ordered = FALSE ;
            }
         }

         cmp = eL.woCompare( eR, compreFieldName ) ;
         if ( 0 != cmp )
         {
            if ( ordered && eK.numberInt() < 0 )
            {
               cmp = -cmp ;
            }
            return cmp ;
         }
      }

      if ( i < keyNum )
      {
         if ( itrL.more() )
         {
            cmp = 1 ;
         }
         else
         {
            cmp = -1 ;
         }
      }
      return cmp ;
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
      : _contextInfoVector( RTN_CONTEXT_CAT_END )
   {
   }

   _rtnContextBuilder::~_rtnContextBuilder()
   {
      _releaseContextInfos() ;
   }

   rtnContextPtr _rtnContextBuilder::create ( RTN_CONTEXT_TYPE type,
                                              INT64 contextId,
                                              EDUID eduId )
   {
      rtnContextPtr ctx ;
      const _rtnContextInfo* info = find( type ) ;
      if ( NULL != info )
      {
         SDB_ASSERT( type == info->type, "invalid context info" ) ;
         SDB_ASSERT( NULL != info->newFunc, "null pointer of newFunc" ) ;

         ctx = (*(info->newFunc))( contextId, eduId ) ;
         if ( NULL != ctx.get() )
         {
            SDB_ASSERT( 0 == ossStrcmp( ctx->name(), info->name.c_str() ),
                        "name is wrong" ) ;
            SDB_ASSERT( ctx->getType() == info->type, "type is wrong" ) ;
         }
      }
      else
      {
         SDB_ASSERT( FALSE, "unknown RTN_CONTEXT_TYPE" ) ;
      }
      return ctx ;
   }

   const _rtnContextInfo* _rtnContextBuilder::find( RTN_CONTEXT_TYPE type ) const
   {
      return _contextInfoVector[type] ;
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
         _contextInfoVector[contextInfo->type] = contextInfo ;
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
      for ( ctx_info_iterator it = _contextInfoVector.begin() ;
            it != _contextInfoVector.end(); ++it )
      {
         _rtnContextInfo* info = *it ;
         if ( NULL != info )
         {
            SDB_OSS_DEL info ;
         }
      }

      _contextInfoVector.clear() ;
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
   : _subSULogicalID( DMS_INVALID_LOGICCSID ),
     _subCB( NULL )
   {
   }

   _rtnSubContextHolder::~_rtnSubContextHolder ()
   {
      _deleteSubContext() ;
   }

   void _rtnSubContextHolder::_deleteSubContext ()
   {
      if ( _subContext )
      {
         sdbGetRTNCB()->contextDelete( _subContext->contextID(), _subCB ) ;
         _subContext.release() ;
         _subSULogicalID = DMS_INVALID_LOGICCSID ;
      }
   }

   void _rtnSubContextHolder::_setSubContext ( rtnContextPtr &subContext,
                                               pmdEDUCB *subCB )
   {
      SDB_ASSERT( NULL == _subContext.get(),
                  "should not have sub context" ) ;
      if ( subContext )
      {
         _subContext = subContext ;
         _subCB = subCB ;
         _subSULogicalID = subContext->getSULogicalID() ;
      }
   }

}
