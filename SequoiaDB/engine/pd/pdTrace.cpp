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

   Source File Name = pdTrace.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"

#if defined (SDB_ENGINE)
#include "pmd.hpp"
#include "pmdDef.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#endif //SDB_ENGINE

#include <sstream>

using namespace engine ;

BOOLEAN g_isTraceStarted = FALSE ;


void pdTraceFunc ( UINT64 funcCode, INT32 type,
                   const CHAR* file, UINT32 line,
                   pdTraceArgTuple *tuple )
{
   pdTraceCB *pdCB = sdbGetPDTraceCB() ;
   BOOLEAN hasStarted = FALSE ;

   if ( !pdCB->isStarted() )
   {
      return ;
   }

   pdCB->startWrite() ;
   hasStarted = TRUE ;

   if ( !pdCB->isStarted() || !pdCB->checkMask( funcCode ) )
   {
      goto done ;
   }

   {
      UINT32 tid = ossGetCurrentThreadID() ;
      UINT32 code = (UINT32)funcCode & 0xFFFFFFFF ;

      if ( !pdCB->checkThread( tid ) )
      {
         goto done ;
      }
      else
      {
         pdTraceRecord record ;
         CHAR *pBuffer = NULL ;
         UINT16 lastSize = TRACE_RECORD_MAX_SIZE - record._recordSize ;

         record.saveCurTime() ;
         record._functionID = code ;
         record._flag = (UINT8)type ;
         record._tid = tid ;
         record._line = (UINT16)line ;

         for ( INT8 i = 0 ; i < PD_TRACE_MAX_ARG_NUM ; ++i )
         {
            if ( PD_TRACE_ARGTYPE_NONE != tuple[i]._arg.getType() )
            {
               if ( tuple[i]._arg.argSize() > lastSize )
               {
                  tuple[i]._arg.setType( PD_TRACE_ARGTYPE_NONE ) ;
                  break ;
               }

               ++record._numArgs ;
               record._recordSize += tuple[i]._arg.argSize() ;
               lastSize -= tuple[i]._arg.argSize() ;
            }
            else
            {
               break ;
            }
         }

         pBuffer = pdCB->reserveMemory( record._recordSize ) ;
         if ( !pBuffer )
         {
            goto done ;
         }

         pBuffer = pdCB->fillIn( pBuffer, (const CHAR*)&record,
                                 sizeof( record ) ) ;

         for ( INT8 i = 0 ; i < PD_TRACE_MAX_ARG_NUM ; ++i )
         {
            if ( PD_TRACE_ARGTYPE_NONE != tuple[i]._arg.getType() )
            {
               pBuffer = pdCB->fillIn ( pBuffer,
                                        (const CHAR*)(&tuple[i]._arg),
                                        tuple[i]._arg.headerSize() ) ;
               pBuffer = pdCB->fillIn ( pBuffer,
                                        (const CHAR*)(tuple[i].y),
                                        tuple[i]._arg.dataSize() ) ;
            }
            else
            {
               break ;
            }
         }
      }

      pdCB->finishWrite() ;
      hasStarted = FALSE ;

#if defined (SDB_ENGINE)
      if ( pdCB->getBPNum() > 0 )
      {
         pdCB->pause( code ) ;
      }
#endif
   }
done :
   if ( hasStarted )
   {
      pdCB->finishWrite() ;
   }
}

/*
   _pdTraceCB implement
*/
_pdTraceCB::_pdTraceCB()
:_ptr( (ossValuePtr)0 ), _alloc( 0 ),
#ifdef _DEBUG
 _padSize( 0 ),
#endif // _DEBUG
 _metaOpr( 0 ), _currentWriter(0)
{
   _ptr2 = NULL ;
   _size = 0 ;
   _pBuffer = NULL ;

   _traceStarted = FALSE ;
   _componentMask = 0xFFFFFFFF ;

   _nMonitoredNum = 0 ;
   ossMemset( _monitoredThreads, 0, sizeof( _monitoredThreads ) ) ;

   _numBP = 0 ;
   ossMemset( _bpList, 0, sizeof( _bpList ) ) ;

   _reset() ;
}

_pdTraceCB::~_pdTraceCB()
{
   _reset() ;
}

UINT64 _pdTraceCB::getFreeSize()
{
   pdAllocPair *pAllocPair = NULL ;
   pAllocPair = ( pdAllocPair* )_ptr.fetch() ;
   UINT64 begin = pAllocPair->_b.fetch() ;
   UINT64 free = 0 ;

   if ( begin < _size )
   {
      free = _size - begin ;
   }
   return free ;
}

CHAR* _pdTraceCB::reserveMemory( UINT32 size )
{
   CHAR *ptr = NULL ;
   UINT64 end = 0 ;
   UINT64 begin = 0 ;
   UINT64 cur = 0 ;
   pdAllocPair *pAllocPair = NULL ;

   if ( size > TRACE_CHUNK_SIZE )
   {
      goto error ;
   }

retry:
   pAllocPair = ( pdAllocPair* )_ptr.fetch() ;
   end = pAllocPair->_e ;
   begin = pAllocPair->_b.add( size ) ;
   cur = begin + size ;

   if ( cur > end )
   {
      if ( begin < end )
      {
         _pBuffer[ begin % _size ] = '\0' ;
#ifdef _DEBUG
         _padSize.add( end - begin ) ;
#endif // _DEBUG
      }

      if ( _alloc.compareAndSwap( 0, 1 ) )
      {
         if ( _ptr.compare( (ossValuePtr)pAllocPair ) )
         {
            _ptr2->_b.swap( end ) ;
            _ptr2->_e = end + TRACE_CHUNK_SIZE ;
            _ptr2 = ( pdAllocPair* )_ptr.swap( (ossValuePtr)_ptr2 ) ;
         }
         _alloc.swap( 0 ) ;
      }
      else
      {
         while( _alloc.compare( 1 ) )
         {
            ossYield() ;
         }
      }
      goto retry ;
   }
   ptr = _pBuffer + ( begin % _size ) ;

done:
   return ptr ;
error:
   goto done ;
}

CHAR* _pdTraceCB::fillIn( CHAR *pPos, const CHAR *pInput, INT32 size )
{
   CHAR *pRetAddr        = NULL ;

   SDB_ASSERT( pPos && pInput, "pos and input can't be NULL" ) ;
   SDB_ASSERT( pPos >= _pBuffer, "pos can't be smaller than buffer" ) ;
   SDB_ASSERT( pPos + size <= _pBuffer + _size, "end pos over the buffer" ) ;

   if ( size < 0 || size >= TRACE_RECORD_MAX_SIZE )
   {
      pRetAddr = (CHAR*)pPos ;
      goto done ;
   }

   ossMemcpy( pPos, pInput, size ) ;
   pRetAddr = ( CHAR* )( pPos + size ) ;

done :
   return pRetAddr ;
}

void _pdTraceCB::startWrite()
{
   _currentWriter.inc() ;
}

void _pdTraceCB::finishWrite()
{
   _currentWriter.dec() ;
}

void _pdTraceCB::setMask( UINT32 mask )
{
   _componentMask = mask ;
}

INT32 _pdTraceCB::start ( UINT64 size )
{
   return start ( size, 0xFFFFFFFF, NULL, NULL ) ;
}

INT32 _pdTraceCB::start ( UINT64 size, UINT32 mask )
{
   return start ( size, mask, NULL, NULL ) ;
}

INT32 _pdTraceCB::start ( UINT64 size,
                          UINT32 mask,
                          std::vector<UINT64> *funcCode,
                          std::vector<UINT32> *tids )
{
   INT32 rc = SDB_OK ;
   std::stringstream tidTextss ;
   std::stringstream funcTextss ;

   while( !_metaOpr.compareAndSwap( 0, 1 ) )
   {
      ossSleep( 100 ) ;
   }

   if ( _traceStarted )
   {
      PD_LOG ( PDWARNING, "Trace is already started" ) ;
      rc = SDB_PD_TRACE_IS_STARTED ;
      goto error ;
   }

   _reset() ;

   if ( funcCode && funcCode->size() > 0 )
   {
      funcTextss << "[" ;
      for ( UINT32 i = 0; i < funcCode->size() ; ++i )
      {
         rc = _addBreakPoint ( (*funcCode)[i] ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add break point, rc: %d", rc ) ;
            removeAllBreakPoint() ;
            goto error ;
         }
         if ( i > 0 )
         {
            funcTextss << ", " ;
         }
         funcTextss << pdGetTraceFunction( (*funcCode)[i] ) ;
      }
      funcTextss << "]" ;
   }

   if ( tids && tids->size() > 0 )
   {
      tidTextss << "[" ;
      for ( UINT32 i = 0 ; i < tids->size() ; ++i )
      {
         rc = _addTidFilter( (*tids)[i] ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to add tid filter, rc: %d", rc ) ;
            _removeAllTidFilter() ;
            goto error ;
         }
         if ( i > 0 )
         {
            tidTextss << ", " ;
         }
         tidTextss << (*tids)[i] ;
      }
      tidTextss << "]" ;
   }

   _componentMask = mask ;
   size           = OSS_MAX ( size, TRACE_MIN_BUFFER_SIZE ) ;
   size           = OSS_MIN ( size, TRACE_MAX_BUFFER_SIZE ) ;
   size           *= ( 1024 * 1024 ) ;
   size           = ossRoundUpToMultipleX ( size, TRACE_CHUNK_SIZE ) ;

   PD_LOG ( PDEVENT, "Trace starts, buffersize = %llu, mask = 0x%x, "
            "funcCodes = %s, tids = %s", size, mask,
            funcTextss.str().c_str(), tidTextss.str().c_str() ) ;

   _size = size ;

   _pBuffer       = (CHAR*)SDB_OSS_MALLOC ( size ) ;
   if ( !_pBuffer )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory for trace "
               "buffer: %lld bytes", size ) ;
      rc = SDB_OOM ;
      goto error ;
   }

   _traceStarted = TRUE ;
   g_isTraceStarted = TRUE ;

done :
   _metaOpr.swap( 0 ) ;
   return rc ;
error :
   goto done ;
}

void _pdTraceCB::stop()
{
   while( !_metaOpr.compareAndSwap( 0, 1 ) )
   {
      ossSleep( 100 ) ;
   }

   if ( _traceStarted )
   {
      _traceStarted = FALSE ;
      g_isTraceStarted = FALSE ;
      PD_LOG ( PDEVENT, "Trace stops" ) ;

      while ( !_currentWriter.compare( 0 ) )
      {
         ossSleep( 1 ) ;
      }
   }

   _metaOpr.swap( 0 ) ;
}

void _pdTraceCB::destroy()
{
   while( !_metaOpr.compareAndSwap( 0, 1 ) )
   {
      ossSleep( 100 ) ;
   }

   if ( _traceStarted )
   {
      _traceStarted = FALSE ;
      g_isTraceStarted = FALSE ;
      PD_LOG ( PDEVENT, "Trace stops" ) ;

      while ( !_currentWriter.compare( 0 ) )
      {
         ossSleep( 1 ) ;
      }
   }

   _reset() ;

   _metaOpr.swap( 0 ) ;
}

INT32 _pdTraceCB::dump( OSSFILE *outFile )
{
   INT32 rc = SDB_OK ;
   pdAllocPair *pAllocPair = NULL ;

   while( !_metaOpr.compareAndSwap( 0, 1 ) )
   {
      ossSleep( 100 ) ;
   }

   if ( _traceStarted )
   {
      rc = SDB_PD_TRACE_IS_STARTED ;
      PD_LOG( PDWARNING, "Trace must be stopped before dumping" ) ;
      goto error ;
   }

   if ( !_pBuffer || _size == 0 )
   {
      rc = SDB_PD_TRACE_HAS_NO_BUFFER ;
      PD_LOG( PDWARNING, "Trace buffer does not exist, trace must be captured "
              "before dump" ) ;
      goto error ;
   }

   pAllocPair = ( pdAllocPair* )_ptr.fetch() ;
   _header.savePosition( pAllocPair->_b.fetch(), _size ) ;
   if ( _header._bufTail % TRACE_CHUNK_SIZE )
   {
      _pBuffer[_header._bufTail] = 0;
   }

   rc = ossWriteN( outFile, ( const CHAR* )&_header, sizeof( _header ) ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write header to file failed, rc: %d", rc ) ;
      goto error ;
   }

   rc = ossWriteN( outFile, _pBuffer, _header._bufSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write buffer to file failed, rc: %d", rc ) ;
      goto error ;
   }

done :
   _metaOpr.swap( 0 ) ;
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceCB::_addBreakPoint( UINT64 functionCode )
{
   INT32 rc = SDB_OK ;
   for ( UINT32 i = 0; i < _numBP; ++i )
   {
      if ( functionCode == _bpList[i] )
      {
         goto done ;
      }
   }
   if ( _numBP >= PD_TRACE_MAX_BP_NUM )
   {
      rc = SDB_TOO_MANY_TRACE_BP ;
      goto error ;
   }
   _bpList[_numBP] = functionCode ;
   ++_numBP ;

done :
   return rc ;
error :
   goto done ;
}

void _pdTraceCB::removeAllBreakPoint()
{
   _numBP = 0 ;
   ossMemset( &_bpList[0], 0, sizeof(_bpList) ) ;
}

INT32 _pdTraceCB::_addTidFilter( UINT32 tid )
{
   INT32 rc = SDB_OK ;
   for ( UINT32 i = 0; i < _nMonitoredNum ; ++i )
   {
      if ( tid == _monitoredThreads[i] )
      {
         goto done ;
      }
   }
   if ( _numBP >= PD_TRACE_MAX_MONITORED_THREAD_NUM )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   _monitoredThreads[_nMonitoredNum] = tid ;
   ++_nMonitoredNum ;

done :
   return rc ;
error :
   goto done ;
}

void _pdTraceCB::_removeAllTidFilter()
{
   _nMonitoredNum = 0 ;
   ossMemset( &_monitoredThreads[0], 0, sizeof(_monitoredThreads) ) ;
}

void _pdTraceCB::_reset ()
{
   _traceStarted = FALSE ;
   g_isTraceStarted = FALSE ;

   if ( _pBuffer )
   {
      SDB_OSS_FREE ( _pBuffer ) ;
      _pBuffer = NULL ;
   }
   _size         = 0 ;
   _header.reset() ;

   _componentMask = 0xFFFFFFFF ;
   removeAllBreakPoint() ;
   _removeAllTidFilter() ;

   _info1.reset() ;
   _info2.reset() ;

   _ptr.swap( (ossValuePtr)(&_info1) ) ;
   _ptr2 = &_info2 ;

   SDB_ASSERT( 0 == _alloc.fetch(), "Alloc must be zero" ) ;

   resumePausedEDUs() ;
}

void _pdTraceCB::addPausedEDU( engine::IExecutor *cb )
{
#ifdef SDB_ENGINE
   _pmdEDUCBLatch.get() ;
   _pmdEDUCBList.push_back ( cb ) ;
   _pmdEDUCBLatch.release () ;
#endif // SDB_ENGINE
}

void _pdTraceCB::resumePausedEDUs ()
{
#ifdef SDB_ENGINE
   _pmdEDUCBLatch.get() ;
   pmdEDUEvent event( PMD_EDU_EVENT_BP_RESUME ) ;
   while ( !_pmdEDUCBList.empty() )
   {
      pmdEDUCB *cb = ( pmdEDUCB* )_pmdEDUCBList.front() ;
      cb->postEvent( event ) ;
      _pmdEDUCBList.pop_front() ;
   }
   _pmdEDUCBLatch.release() ;
#endif // SDB_ENGINE
}

void _pdTraceCB::pause( UINT64 funcCode )
{
#ifdef SDB_ENGINE
   pmdEDUCB *educb = pmdGetThreadEDUCB() ;
   pmdEDUEvent event ;

   for ( UINT32 i = 0 ; i < _numBP ; ++i )
   {
      if ( _bpList[i] == funcCode )
      {
         addPausedEDU( educb ) ;

         educb->waitEvent( engine::PMD_EDU_EVENT_BP_RESUME, event, -1 ) ;
         break ;
      } // if ( _bpList[i] == funcCode )
   } // for ( i = 0; i < _numBP; ++i )
#endif // SDB_ENGINE
}

BOOLEAN _pdTraceCB::isWrapped()
{
   if ( _pBuffer && _size > 0 )
   {
      pdAllocPair *pAllocPair = NULL ;
      pAllocPair = ( pdAllocPair* )_ptr.fetch() ;
      return pAllocPair->_b.fetch() > _size ? TRUE : FALSE ;
   }
   return FALSE ;
}

/*
   get global pdtrace cb
*/
pdTraceCB* sdbGetPDTraceCB ()
{
   static pdTraceCB s_pdTraceCB ;
   return &s_pdTraceCB ;
}


