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
#include "ossVer.h"
#if defined (SDB_ENGINE)
#include "pmd.hpp"
#include "pmdDef.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#endif // SDB_ENGINE

#include <sstream>

#define TRACE_FUNCTIONSNAMES_INIT_SIZE      ( 204800 )      /* bytes */

using namespace engine ;

BOOLEAN g_isTraceStarted = FALSE ;

// extract high 32 bit as function component mask, and OR with
// cb->_componentMask, if the result is 0 that means the component is not what
// we want

void pdTraceFunc ( UINT64 funcCode, INT32 type,
                   const CHAR* file, UINT32 line,
                   pdTraceArgTuple *tuple )
{
   pdTraceCB *pdCB = sdbGetPDTraceCB() ;
   BOOLEAN hasStarted = FALSE ;

   // make sure trace is turned on
   if ( !pdCB->isStarted() )
   {
      return ;
   }

   pdCB->startWrite() ;
   hasStarted = TRUE ;

   /// double check
   if ( !pdCB->isStarted() )
   {
      goto done ;
   }

   if ( !( pdCB->checkFunction( funcCode ) ) )
   {
      goto done ;
   }

   {
      UINT32 tid = ossGetCurrentThreadID() ;
      UINT32 code = (UINT32)(funcCode & 0xFFFFFFFF) ;
      INT32 threadType = 0 ;
#ifdef SDB_ENGINE
      pmdEDUCB *educb = pmdGetThreadEDUCB() ;
      if( !educb )
      {
         goto done ;
      }
      threadType = educb->getType() ;
#endif // SDB_ENGINE

      if ( !( pdCB->checkThread( tid, threadType ) ) )
      {
         goto done ;
      }

      pdTraceRecord record ;
      CHAR *pBuffer = NULL ;
      UINT16 lastSize = TRACE_RECORD_MAX_SIZE - record._recordSize ;
      UINT64 offset = 0 ;

      record.saveCurTime() ;
      record._functionID = code ;
      record._flag = (UINT8)type ;
      record._tid = tid ;
      record._line = (UINT16)line ;

      // parse arguments and calcualte the total size of buffer we need
      for ( INT8 i = 0 ; i < PD_TRACE_MAX_ARG_NUM ; ++i )
      {
         if ( PD_TRACE_ARGTYPE_NONE != tuple[i]._arg.getType() )
         {
            /// make sure size is not overflow
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

      pBuffer = pdCB->reserveMemory( record._recordSize, offset ) ;
      if ( !pBuffer )
      {
         goto done ;
      }

      pBuffer = pdCB->fillIn( pBuffer, (const CHAR*)&record,
                              sizeof( record ), &offset ) ;
      if ( !pBuffer )
      {
         goto done ;
      }

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
   _pdTraceHeader implement
*/
void _pdTraceHeader::reset()
{
   ossMemset( this, 0, sizeof( _pdTraceHeader ) ) ;
   ossMemcpy( _eyeCatcher, TRACECB_EYE_CATCHER, TRACECB_EYE_CATCHER_SIZE ) ;
   _headerSize          = sizeof( _pdTraceHeader ) ;
   _version             = PD_TRACE_VERSION_CUR ;
   _engineVersion       = SDB_ENGINE_VERISON_CURRENT ;
   _engineSubVersion    = SDB_ENGINE_SUBVERSION_CURRENT ;
#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   _engineFixVersion = SDB_ENGINE_FIXVERSION_CURRENT ;
#else
   _engineFixVersion = PD_TRACE_INVALID_FIXVERSION ;
#endif
   _release                = SDB_ENGINE_RELEASE_CURRENT ;
   _functionsSegmentSize   = 0 ;
   _functionsSegmentOffset = 0 ;
}

_pdTraceHeader::_pdTraceHeader()
{
   reset();
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
   _threadMonitoredNum = 0 ;
   _threadTypeMonitoredNum = 0 ;
   _functionMonitoredNum = 0 ;
   ossMemset( _monitoredThreads, 0, sizeof( _monitoredThreads ) ) ;

   _numBP = 0 ;
   ossMemset( _bpList, 0, sizeof( _bpList ) ) ;

   _reset() ;
}

// free memory
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

CHAR* _pdTraceCB::reserveMemory( UINT32 size, UINT64& offset )
{
   CHAR *ptr = NULL ;
   UINT64 end = 0 ;
   UINT64 tmpEnd = 0 ;
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

   // in case that end value is too old
   tmpEnd = pAllocPair->_e ;
   if ( ( tmpEnd - end ) >= ( TRACE_CHUNK_SIZE << 1 ) )
   {
      end = tmpEnd ;
   }
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
         // If the chunk has been switched 1 time, 2 times ..., just goto retry
         if ( end == ((pdAllocPair*)_ptr.fetch())->_e )
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

   // write "begin" to the first 8 bytes, fillIn() will check it
   *((UINT64*)ptr) = begin ;
   offset = begin ;

done:
   return ptr ;
error:
   goto done ;
}

CHAR* _pdTraceCB::fillIn( CHAR *pPos, const CHAR *pInput, INT32 size,
                          UINT64* pOffset )
{
   CHAR *pRetAddr        = NULL ;

   SDB_ASSERT( pPos && pInput, "pos and input can't be NULL" ) ;
   // target offset must be in valid range
   SDB_ASSERT( pPos >= _pBuffer, "pos can't be smaller than buffer" ) ;
   // end offset must be in valid range
   SDB_ASSERT( pPos + size <= _pBuffer + _size, "end pos over the buffer" ) ;

   // In reserveMemory(), "begin" is writen to the first 8 bytes. If find
   // an inconsistency now, the memory is reassigned (caused by a buffer
   // circle), we shouldn't write the memory.
   if ( pOffset )
   {
      if ( *pOffset != *((UINT64*)pPos) )
      {
         pRetAddr = NULL ;
         goto done ;
      }
   }

   // if we are asked to write too big data, let's just skip it
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
   return start ( size, 0xFFFFFFFF, NULL, NULL, NULL, NULL ) ;
}

INT32 _pdTraceCB::start ( UINT64 size, UINT32 mask )
{
   return start ( size, mask, NULL, NULL, NULL, NULL ) ;
}

INT32 _pdTraceCB::start ( UINT64 size,
                          UINT32 mask,
                          std::vector<UINT64> *breakPoint,
                          std::vector<UINT32> *tids,
                          std::vector<UINT64> *functionName,
                          std::vector<INT32> *threadType )
{
   INT32 rc = SDB_OK ;
   std::stringstream tidTextss ;
   std::stringstream funcTextss ;
   std::stringstream functionNameTextss ;
   std::stringstream threadTypeTextss ;

#if defined (SDB_ENGINE)
   pmdEPFactory &factory = pmdGetEPFactory() ;
#endif //SDB_ENGINE

   while( !_metaOpr.compareAndSwap( 0, 1 ) )
   {
      ossSleep( 100 ) ;
   }

   // sanity check, make sure we are not currently tracing anything
   if ( _traceStarted )
   {
      PD_LOG ( PDWARNING, "Trace is already started" ) ;
      rc = SDB_PD_TRACE_IS_STARTED ;
      goto error ;
   }

   _reset() ;

   // trace stop break
   if ( breakPoint && breakPoint->size() > 0 )
   {
      funcTextss << "[" ;
      for ( UINT32 i = 0; i < breakPoint->size() ; ++i )
      {
         rc = _addBreakPoint ( (*breakPoint)[i] ) ;
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
         funcTextss << pdGetTraceFunction( (*breakPoint)[i] ) ;
      }
      funcTextss << "]" ;
   }

   // trace tid filter
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

   // trace functionName filter
   if ( functionName && functionName->size() > 0 )
   {
      functionNameTextss << "[" ;
      for ( UINT32 i = 0 ; i < functionName->size() ; ++i )
      {
        rc = _addFunctionNameFilter( (*functionName)[i] ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to add functionName filter, rc: %d", rc ) ;
            _removeAllFunctionNameFilter() ;
            goto error ;
         }
         if ( i > 0 )
         {
            functionNameTextss << ", " ;
         }
         functionNameTextss << pdGetTraceFunction( (*functionName)[i] ) ;
      }
      functionNameTextss << "]" ;
   }

   // trace threadType filter
   if ( threadType && threadType->size() > 0 )
   {
      threadTypeTextss<< "[" ;
      for ( UINT32 i = 0 ; i < threadType->size() ; ++i )
      {
         rc = _addThreadTypeFilter( (*threadType)[i] ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to add threadType filter, rc: %d", rc ) ;
            _removeAllThreadTypeFilter() ;
            goto error ;
         }
#if defined (SDB_ENGINE)
         if ( i > 0 )
         {
            threadTypeTextss << ", " ;
         }
         threadTypeTextss << factory.type2Name( (*threadType)[i] ) ;
#endif //SDB_ENGINE
      }
      threadTypeTextss << "]" ;
   }

   _componentMask = mask ;

   if ( _componentMask == 0xFFFFFFFF &&
        functionName && functionName->size() > 0  )
   {
      _componentMask = 0 ;
   }

   size           = OSS_MAX ( size, TRACE_MIN_BUFFER_SIZE ) ;
   size           = OSS_MIN ( size, TRACE_MAX_BUFFER_SIZE ) ;
   size           *= ( 1024 * 1024 ) ;
   size           = ossRoundUpToMultipleX ( size, TRACE_CHUNK_SIZE ) ;

   // start trace
   PD_LOG ( PDEVENT, "Trace starts, buffersize = %llu, mask = 0x%x, "
            "funcCodes = %s, tids = %s, functionNames = %s, threadTypeS = %s ",
            size, mask, funcTextss.str().c_str(), tidTextss.str().c_str(),
            functionNameTextss.str().c_str(),
            threadTypeTextss.str().c_str() ) ;

   _size = size ;

   // memory will be freed in destructor or reset
   _pBuffer       = (CHAR*)SDB_OSS_MALLOC ( size ) ;
   if ( !_pBuffer )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory for trace "
               "buffer: %lld bytes", size ) ;
      rc = SDB_OOM ;
      goto error ;
   }

   /// set started
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

      // wait until there's no one write into the buffer
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

      // wait until there's no one write into the buffer
      while ( !_currentWriter.compare( 0 ) )
      {
         ossSleep( 1 ) ;
      }
   }

   _reset() ;

   _metaOpr.swap( 0 ) ;
}

INT32 _pdTraceCB::_appendFuncName( CHAR **ppBuffer, UINT32 &bufferSize,
                                   UINT32 &usedSize, UINT32 functionNameID )
{
   INT32 rc                = SDB_OK ;
   CHAR  *pNewBuffer       = NULL ;
   UINT32 tempBufferSize   = 0 ;
   UINT32 functionsNameLen = ossStrlen( pdGetTraceFunction( functionNameID ) )
                             + 1 ;
   if ( !( *ppBuffer ) )
   {
      tempBufferSize = TRACE_FUNCTIONSNAMES_INIT_SIZE ;
      *ppBuffer = ( CHAR* )SDB_OSS_MALLOC( tempBufferSize ) ;
      if ( !*ppBuffer )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = tempBufferSize ;
   }

   if ( ( usedSize + functionsNameLen ) > bufferSize )
   {
      tempBufferSize = ( usedSize + functionsNameLen ) > ( bufferSize * 2 ) ?
                       ( usedSize + functionsNameLen ) : ( bufferSize * 2 ) ;
      pNewBuffer = (CHAR*)SDB_OSS_REALLOC( *ppBuffer, tempBufferSize ) ;
      if ( !pNewBuffer )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      *ppBuffer = pNewBuffer ;
      bufferSize = tempBufferSize ;
   }

   ossMemcpy( *ppBuffer + usedSize,
              pdGetTraceFunction( functionNameID ), functionsNameLen ) ;
   usedSize += functionsNameLen ;

done :
   return rc ;
error :
   goto done ;
}

// This function can check trace file of the host
// where the monitored node is located
BOOLEAN _pdTraceCB::isTraceFile( const CHAR* filePath )
{
   INT32 rc = SDB_OK ;
   CHAR pHeader[TRACECB_EYE_CATCHER_SIZE] = { '\0' } ;
   SINT64 byteRead = 0 ;
   OSSFILE traceFile ;
   BOOLEAN isOpen = FALSE ;

   if( NULL == filePath )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = ossOpen( filePath, OSS_READONLY | OSS_SHAREREAD, 0, traceFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to open file[%s], rc: %d", filePath, rc ) ;
      goto error ;
   }
   isOpen = TRUE ;

   rc = ossReadN( &traceFile, TRACECB_EYE_CATCHER_SIZE,
                  ( CHAR* )pHeader, byteRead ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to read file[%s], rc: %d", filePath, rc ) ;
      goto error ;
   }

   /// check eye TRACECB_EYE_CATCHER
   if ( 0 != ossStrcmp( pHeader, TRACECB_EYE_CATCHER ) )
   {
      PD_LOG( PDERROR, "Invalid eye catcher" ) ;
      rc = SDB_PD_TRACE_FILE_INVALID ;
      goto error ;
   }

done :
   if( isOpen )
   {
      ossClose( traceFile ) ;
   }
   if( SDB_OK == rc )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
error :
   goto done ;
}

INT32 _pdTraceCB::dump( OSSFILE *outFile )
{
   INT32 rc = SDB_OK ;
   pdAllocPair *pAllocPair = NULL ;

   UINT32  readLen               = 0 ;
   UINT32  mallocSize            = 0 ;
   CHAR   *funcNamesBuf          = NULL ;
   UINT32 *funcOffsetListBuf     = NULL ;
   UINT32  funcOffsetListBufSize = 0 ;
   const UINT32 functionsCount   = pdGetTraceFunctionListNum() ;

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

   funcOffsetListBufSize = ( functionsCount ) * sizeof( UINT32 ) ;
   funcOffsetListBuf = ( UINT32* )SDB_OSS_MALLOC( funcOffsetListBufSize ) ;
   if ( !funcOffsetListBuf )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   //init funcNamesBuf and funcOffsetList
   for( UINT32 i = 0 ; i < functionsCount ; i++ )
   {
      funcOffsetListBuf[i] = readLen ;

      rc = _appendFuncName( &funcNamesBuf, mallocSize, readLen, i ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Append functionName[%d] failed, rc: %d", i ,rc ) ;
         goto error ;
      }
   }

   _header._functionsSegmentSize = sizeof( UINT32 ) + funcOffsetListBufSize
                                   + readLen ;

   // write header first
   pAllocPair = ( pdAllocPair* )_ptr.fetch() ;
   _header.savePosition( pAllocPair->_b.fetch(), _size ) ;

   if ( _header._bufTail % TRACE_CHUNK_SIZE )
   {
      _pBuffer[_header._bufTail] = 0;
   }

   _header._functionsSegmentOffset = sizeof(_header) + _header._bufSize ;

   rc = ossWriteN( outFile, ( const CHAR* )&_header, sizeof( _header ) ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write header to file failed, rc: %d", rc ) ;
      goto error ;
   }

   // write buffer
   rc = ossWriteN( outFile, _pBuffer, _header._bufSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write buffer to file failed, rc: %d", rc ) ;
      goto error ;
   }

   // write functionsSegment
   rc = ossWriteN( outFile, ( CHAR* )&functionsCount, sizeof( UINT32 ) ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write functionsCount to file failed, rc: %d", rc ) ;
      goto error ;
   }
   rc = ossWriteN( outFile, ( CHAR* )funcOffsetListBuf, funcOffsetListBufSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write funcOffsetList to file failed, rc: %d", rc ) ;
      goto error ;
   }
   rc = ossWriteN( outFile, funcNamesBuf, readLen ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write funcNamesBuf to file failed, rc: %d", rc ) ;
      goto error ;
   }

done :
   if ( funcNamesBuf )
   {
      SDB_OSS_FREE( funcNamesBuf ) ;
      funcNamesBuf = NULL ;
   }
   if ( funcOffsetListBuf )
   {
      SDB_OSS_FREE( funcOffsetListBuf ) ;
      funcOffsetListBuf = NULL ;
   }
   _metaOpr.swap( 0 ) ;
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceCB::_addBreakPoint( UINT64 functionCode )
{
   INT32 rc = SDB_OK ;
   // duplicate detection
   for ( UINT32 i = 0; i < _numBP; ++i )
   {
      if ( functionCode == _bpList[i] )
      {
         goto done ;
      }
   }
   // here we need to insert the function code into list
   // first we have to make sure we still have enough room
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
   // duplicate detection
   for ( UINT32 i = 0; i < _threadMonitoredNum ; ++i )
   {
      if ( tid == _monitoredThreads[i] )
      {
         goto done ;
      }
   }
   // here we need to insert the tid filter into list
   // first we have to make sure we still have enough room
   if ( _threadMonitoredNum >= PD_TRACE_MAX_MONITORED_THREAD_NUM )
   {
      rc = SDB_OSS_UP_TO_LIMIT ;
      goto error ;
   }
   _monitoredThreads[_threadMonitoredNum] = tid ;
   ++_threadMonitoredNum ;

done :
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceCB::_addFunctionNameFilter( UINT64 functionId )
{
   INT32 rc = SDB_OK ;
   // duplicate detection
   for ( UINT32 i = 0; i < _functionMonitoredNum ; ++i )
   {
      if ( functionId == _monitoredFunctionNamesId[i] )
      {
         goto done ;
      }
   }
   // here we need to insert the functionName filter into list
   // first we have to make sure we still have enough room
   if ( _functionMonitoredNum >= PD_TRACE_MAX_MONITORED_THREAD_NUM )
   {
      rc = SDB_OSS_UP_TO_LIMIT ;
      goto error ;
   }
   _monitoredFunctionNamesId[_functionMonitoredNum] = functionId ;
   ++_functionMonitoredNum ;

done :
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceCB::_addThreadTypeFilter( INT32 threadTypeId )
{
   INT32 rc = SDB_OK ;
   // duplicate detection
   for ( UINT32 i = 0; i < _threadTypeMonitoredNum ; ++i )
   {
      if ( threadTypeId == _monitoredThreadTypes[i] )
      {
         goto done ;
      }
   }
   // here we need to insert the threadType filter into list
   // first we have to make sure we still have enough room
   if ( _threadTypeMonitoredNum >= PD_TRACE_MAX_MONITORED_THREAD_NUM )
   {
      rc = SDB_OSS_UP_TO_LIMIT ;
      goto error ;
   }
   _monitoredThreadTypes[_threadTypeMonitoredNum] = threadTypeId ;
   ++_threadTypeMonitoredNum ;

done :
   return rc ;
error :
   goto done ;
}

void _pdTraceCB::_removeAllTidFilter()
{
   _threadMonitoredNum = 0 ;
   ossMemset( &_monitoredThreads[0], 0, sizeof(_monitoredThreads) ) ;
}

void _pdTraceCB::_removeAllThreadTypeFilter()
{
   _threadTypeMonitoredNum = 0 ;
   ossMemset( &_monitoredThreadTypes[0], 0, sizeof(_monitoredThreadTypes) ) ;
}

void _pdTraceCB::_removeAllFunctionNameFilter()
{
   _functionMonitoredNum = 0 ;
   ossMemset( &_monitoredFunctionNamesId[0], 0,
              sizeof(_monitoredFunctionNamesId) ) ;
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
   _removeAllThreadTypeFilter() ;
   _removeAllFunctionNameFilter() ;

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

   // compare each defined break points
   for ( UINT32 i = 0 ; i < _numBP ; ++i )
   {
      // if the break point matches our current function code
      if ( _bpList[i] == funcCode )
      {
         // put EDU into pause status
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


