/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pd.hpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PDTRACE_HPP__
#define PDTRACE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossAtomic.hpp"
#include "sdbInterface.hpp"
#include "ossIO.hpp"
#include <list>
#include <vector>

#ifdef SDB_ENGINE
#include "pdTrace.h"
#include "ossLatch.hpp"
#endif // SDB_ENGINE

#define PD_TRACE_MAX_BP_NUM                  10
#define PD_TRACE_MAX_MONITORED_THREAD_NUM    10

/*
 * Slots and chunks
 *
 * The trace buffer is divided into slots and chunks, slots are the smallest
 * element in the buffer. Multiple slots ( TRACE_SLOTS_PER_CHUNK ) are grouped
 * into a single chunk, and multiple chunks are grouped into the entire trace
 * buffer
 * for Performance reason, we should never write a single slot into file,
 * instead we should flush file chunk by chunk.
 *
 * Each log record start from one slot, and may sit in one slot or cross
 * multiple slots.
 */

/*
   TRACE_RECORD_MAX_SIZE must <= 65536 and
                              <= TRACE_CHUNK_SIZE
   TRACE_CHUNK_SIZE must <= 1048576 and
                         >= 4096
*/

#define TRACE_CHUNK_SIZE      ( 65536 )      /* bytes */

#if TRACE_CHUNK_SIZE > 1048576 || TRACE_CHUNK_SIZE < 4096
   #error "TRACE_CHUNK_SIZE is invalid"
#endif

#define TRACE_RECORD_MAX_SIZE ( 65536 )      /* bytes */

#if TRACE_RECORD_MAX_SIZE > TRACE_CHUNK_SIZE || TRACE_RECORD_MAX_SIZE > 65536
   #error "TRACE_RECORD_MAX_SIZE can't more than TRACE_CHUNK_SIZE"
#endif

/*
 * Trace buffer should always be power of 2 and multiple of chunk size
 * Even thou there's no physical limitation of upper limit of buffer size, but
 * we should still limit it to a practical number, let's say 1GB
 */
#define TRACE_MIN_BUFFER_SIZE ( 1 )                   /* mega bytes */
#define TRACE_MAX_BUFFER_SIZE ( 1 * 1024 )            /* mega bytes */
#define TRACE_DFT_BUFFER_SIZE ( 256 )                 /* mega bytes */

/*
 * Put \n ( newline ) as eye catcher. We put this one in trace file header,
 * so that if the trace file was FTP'd using ASCII mode between UNIX machine and
 * PC, this eye catcher will
 * be corrupted so that we can easily detect whether a trace file is valid or
 * not
 */

#define TRACE_EYE_CATCHER           "TR"
#define TRACECB_EYE_CATCHER         "TB"
#define TRACE_EYE_CATCHER_SIZE      ( 2 )
#define TRACECB_EYE_CATCHER_SIZE    ( 2 )

/*
   Max component number
*/
const INT32 _pdTraceComponentNum = 28 ;

#define PD_TRACE_COMPONENT_AUTH    0x00000001
#define PD_TRACE_COMPONENT_BPS     0x00000002
#define PD_TRACE_COMPONENT_CAT     0x00000004
#define PD_TRACE_COMPONENT_CLS     0x00000008
#define PD_TRACE_COMPONENT_DPS     0x00000010
#define PD_TRACE_COMPONENT_MIG     0x00000020
#define PD_TRACE_COMPONENT_MSG     0x00000040
#define PD_TRACE_COMPONENT_NET     0x00000080
#define PD_TRACE_COMPONENT_OSS     0x00000100
#define PD_TRACE_COMPONENT_PD      0x00000200
#define PD_TRACE_COMPONENT_RTN     0x00000400
#define PD_TRACE_COMPONENT_SQL     0x00000800
#define PD_TRACE_COMPONENT_TOOL    0x00001000
#define PD_TRACE_COMPONENT_BAR     0x00002000
#define PD_TRACE_COMPONENT_CLIENT  0x00004000
#define PD_TRACE_COMPONENT_COORD   0x00008000
#define PD_TRACE_COMPONENT_DMS     0x00010000
#define PD_TRACE_COMPONENT_IXM     0x00020000
#define PD_TRACE_COMPONENT_MON     0x00040000
#define PD_TRACE_COMPONENT_MTH     0x00080000
#define PD_TRACE_COMPONENT_OPT     0x00100000
#define PD_TRACE_COMPONENT_PMD     0x00200000
#define PD_TRACE_COMPONENT_REST    0x00400000
#define PD_TRACE_COMPONENT_SPT     0x00800000
#define PD_TRACE_COMPONENT_UTIL    0x01000000
#define PD_TRACE_COMPONENT_AGGR    0x02000000
#define PD_TRACE_COMPONENT_SPD     0x04000000
#define PD_TRACE_COMPONENT_QGM     0x08000000

/*
   _pdTraceFormatType define
*/
enum _pdTraceFormatType
{
   PD_TRACE_FORMAT_TYPE_FLOW = 0,
   PD_TRACE_FORMAT_TYPE_FORMAT
} ;
typedef _pdTraceFormatType pdTraceFormatType ;

/*
   _pdTraceArgumentType define
*/
enum _pdTraceArgumentType
{
   PD_TRACE_ARGTYPE_NONE = 0,
   PD_TRACE_ARGTYPE_NULL,
   PD_TRACE_ARGTYPE_CHAR,
   PD_TRACE_ARGTYPE_BYTE,
   PD_TRACE_ARGTYPE_SHORT,
   PD_TRACE_ARGTYPE_USHORT,
   PD_TRACE_ARGTYPE_INT,
   PD_TRACE_ARGTYPE_UINT,
   PD_TRACE_ARGTYPE_LONG,
   PD_TRACE_ARGTYPE_ULONG,
   PD_TRACE_ARGTYPE_FLOAT,
   PD_TRACE_ARGTYPE_DOUBLE,
   PD_TRACE_ARGTYPE_STRING,
   PD_TRACE_ARGTYPE_RAW,
   PD_TRACE_ARGTYPE_BSONRAW
} ;
typedef _pdTraceArgumentType pdTraceArgumentType ;

#pragma pack(4)

class _pdTraceArgument : public SDBObject
{
private :
   UINT8                _argumentType ;
   UINT8                _pad ;
   UINT16               _argumentSize ;

public:
   _pdTraceArgument()
   {
      _argumentType = 0 ;
      _pad = 0 ;
      _argumentSize = sizeof( _pdTraceArgument ) ;
   }

   CHAR* argData()
   {
      return ( CHAR* )this + sizeof( _pdTraceArgument ) ;
   }

   UINT16 dataSize() const
   {
      return _argumentSize - sizeof( _pdTraceArgument ) ;
   }

   void setDataSize( UINT16 size )
   {
      _argumentSize = sizeof( _pdTraceArgument ) + size ;
   }

   UINT16 argSize() const
   {
      return _argumentSize ;
   }

   UINT16 headerSize() const
   {
      return sizeof( _pdTraceArgument ) ;
   }

   pdTraceArgumentType getType() const
   {
      return ( pdTraceArgumentType )_argumentType ;
   }

   void setType( pdTraceArgumentType type )
   {
      _argumentType = (UINT8)type ;
   }
} ;
typedef class _pdTraceArgument pdTraceArgument ;

/*
   PD record flag define
*/
#define PD_TRACE_RECORD_FLAG_NORMAL 0
#define PD_TRACE_RECORD_FLAG_ENTRY  1
#define PD_TRACE_RECORD_FLAG_EXIT   2

/*
   Max argument number
*/
#define PD_TRACE_MAX_ARG_NUM        9

/*
   _pdTraceRecord define
*/
struct _pdTraceRecord 
{
   CHAR           _eyeCatcher[ TRACE_EYE_CATCHER_SIZE ] ;
   UINT8          _flag ;
   UINT8          _numArgs ;
   UINT32         _functionID ;
   UINT32         _tid ;
   UINT16         _line ;
   UINT16         _recordSize ;
   UINT64         _timestamp ;

   _pdTraceRecord()
   {
      ossMemset( this, 0, sizeof( _pdTraceRecord ) ) ;
      ossMemcpy( _eyeCatcher, TRACE_EYE_CATCHER, TRACE_EYE_CATCHER_SIZE ) ;
      _flag                = PD_TRACE_RECORD_FLAG_NORMAL ;
      _recordSize          = sizeof( _pdTraceRecord ) ;
   }

   void saveCurTime()
   {
      ossTimestamp tmp ;
      ossGetCurrentTime( tmp ) ;
      _timestamp = tmp.time * 1000000L + tmp.microtm ;
   }

   ossTimestamp getCurTime() const
   {
      ossTimestamp tm ;
      tm.time = _timestamp / 1000000 ;
      tm.microtm = _timestamp % 1000000 ;
      return tm ;
   }

   pdTraceArgument* getArg( UINT32 id ) const
   {
      CHAR *pArg = NULL ;

      pArg = ( CHAR* )this + sizeof( _pdTraceRecord ) ;

      while ( id > 0 )
      {
         pArg = pArg + ( ( pdTraceArgument* )pArg )->argSize() ;
         --id ;
      }

      return ( pdTraceArgument* )pArg ;
   }
} ;
typedef struct _pdTraceRecord pdTraceRecord ;

/*
   Current Version
*/
#define PD_TRACE_VERSION_CUR        1

/*
   _pdTraceHeader define
*/
struct _pdTraceHeader
{
   CHAR     _eyeCatcher[ TRACECB_EYE_CATCHER_SIZE ] ;
   UINT16   _headerSize ;      // size of header
   UINT8    _version ;
   UINT8    _pad[ 3 ] ;

   UINT64   _bufSize ;
   UINT64   _bufHeader ;
   UINT64   _bufTail ;

   UINT32   _pad1[ 10 ] ;

   _pdTraceHeader()
   {
      reset() ;
   }

   void savePosition( UINT64 current, UINT64 bufSize )
   {
      _bufTail = current % bufSize ;

      if ( current > bufSize )
      {
         _bufSize = bufSize ;
         _bufHeader = ( ( ( ( current + TRACE_CHUNK_SIZE - 1 ) /
                          TRACE_CHUNK_SIZE ) *
                        TRACE_CHUNK_SIZE ) % _bufSize ) ;
         
      }
      else
      {
         _bufHeader = 0 ;
         _bufSize = _bufTail ;
         _bufSize = ossRoundUpToMultipleX ( _bufSize, TRACE_CHUNK_SIZE ) ;
      }
   }

   void reset()
   {
      ossMemset( this, 0, sizeof( _pdTraceHeader ) ) ;
      ossMemcpy( _eyeCatcher, TRACECB_EYE_CATCHER, TRACECB_EYE_CATCHER_SIZE ) ;
      _headerSize    = sizeof( _pdTraceHeader ) ;
      _version       = PD_TRACE_VERSION_CUR ;
   }

} ;
typedef struct _pdTraceHeader pdTraceHeader ;

/*
   _pdAllocPair define
*/
struct _pdAllocPair
{
   ossAtomic64       _b ;
   volatile UINT64   _e ;

   _pdAllocPair()
   :_b( 0 ), _e( TRACE_CHUNK_SIZE )
   {
   }
   void reset()
   {
      _b.swap( 0 ) ;
      _e = TRACE_CHUNK_SIZE ;
   }
} ;
typedef _pdAllocPair pdAllocPair ;

/*
   _pdTraceCB define
*/
class _pdTraceCB : public SDBObject
{
public:
   _pdTraceCB() ;
   ~_pdTraceCB() ;

   CHAR*          reserveMemory( UINT32 size ) ;
   CHAR*          fillIn ( CHAR *pPos, const CHAR *pInput, INT32 size ) ;

   void           startWrite() ;
   void           finishWrite() ;

   void           setMask( UINT32 mask ) ;
   UINT32         getMask() const { return _componentMask ; }

   INT32          start ( UINT64 size,
                          UINT32 mask,
                          std::vector<UINT64> *funcCode,
                          std::vector<UINT32> *tids ) ;

   INT32          start ( UINT64 size, UINT32 mask ) ;
   INT32          start ( UINT64 size ) ; // size for trace buffer size on bytes
   void           stop () ; // stop trace but keep memory available

   INT32          dump ( OSSFILE *outFile ) ;
   void           destroy () ; // stop trace and destroy memory

   void           resumePausedEDUs() ;
   void           addPausedEDU( engine::IExecutor *cb ) ;
   void           pause ( UINT64 funcCode ) ;

   const UINT64*  getBPList() const { return _bpList ; }
   UINT32         getBPNum() const { return _numBP ; }

   UINT32         getThreadFilterNum() const { return _nMonitoredNum ; }
   const UINT32*  getThreadFilterList() const { return _monitoredThreads ; }

   BOOLEAN        isWrapped() ;

   UINT64         getSize() const { return _size ; }
   BOOLEAN        isStarted() const { return _traceStarted ; }

   BOOLEAN        checkMask( UINT64 funcCode ) const
   {
      UINT32 component = (UINT32)( funcCode >> 32 ) ;
      return ( component & _componentMask ) ? TRUE : FALSE ;
   }

   BOOLEAN        checkThread( UINT32 tid ) const
   {
      BOOLEAN result = FALSE ;
      if ( _nMonitoredNum > 0 )
      {
         for ( UINT32 i = 0 ; i < _nMonitoredNum ; ++i )
         {
            if ( _monitoredThreads[ i ] == tid )
            {
               result = TRUE ;
               break ;
            }
         }
      }
      else
      {
         result = TRUE ;
      }
      return result ;
   }

   void           removeAllBreakPoint() ;

#ifdef _DEBUG
   UINT64         getPadSize() { return _padSize.fetch() ; }
#endif // _DEBUG

   UINT64         getFreeSize() ;

protected:
   INT32          _addBreakPoint( UINT64 functionCode ) ;
   INT32          _addTidFilter( UINT32 tid ) ;
   void           _removeAllTidFilter() ;

   void           _reset() ;

private :
   pdTraceHeader        _header ;

   pdAllocPair          _info1 ;
   pdAllocPair          _info2 ;
   ossAtomicPtr         _ptr ;
   pdAllocPair          *_ptr2 ;
   ossAtomic32          _alloc ;
   UINT64               _size ;
   CHAR                 *_pBuffer ;

#ifdef _DEBUG
   ossAtomic64          _padSize ;
#endif // _DEBUG

   ossAtomic32          _metaOpr ;
   volatile BOOLEAN     _traceStarted ; // whether trace is started or not
   ossAtomic32          _currentWriter ;

   UINT32               _componentMask ;   // each bit represent one component
   UINT8                _nMonitoredNum ;
   UINT32               _monitoredThreads[ PD_TRACE_MAX_MONITORED_THREAD_NUM ] ;
   UINT32               _numBP ;    // num of break points
   UINT64               _bpList [ PD_TRACE_MAX_BP_NUM ] ; // break point list

#if defined (SDB_ENGINE)
   ossSpinXLatch                 _pmdEDUCBLatch ;     // paused EDU latch
   std::list<engine::IExecutor*> _pmdEDUCBList ;      // EDU CB list that pause
#endif // SDB_ENGINE

} ;
typedef class _pdTraceCB pdTraceCB ;

/*
   _pdTraceArgTuple define
*/
struct _pdTraceArgTuple
{
   pdTraceArgument _arg ;
   const void *y ;
   _pdTraceArgTuple ( _pdTraceArgumentType a,
                      const void *b, INT32 c )
   {
      _arg.setType( a );
      _arg.setDataSize( (UINT16)c ) ;
      y = b ;
   }
   _pdTraceArgTuple () {}
   _pdTraceArgTuple &operator=(const _pdTraceArgTuple &right)
   {
      _arg.setType( right._arg.getType() ) ;
      _arg.setDataSize( right._arg.dataSize() ) ;
      y = right.y ;
      return *this ;
   }
} ;
typedef struct _pdTraceArgTuple pdTraceArgTuple ;

#pragma pack()

#ifndef SDB_ENGINE

#define PD_TRACE_DECLARE_FUNCTION(x,y) 
#define PD_TRACE_ENTRY(funcCode) 
#define PD_TRACE_EXIT(funcCode) 
#define PD_TRACE_EXITRC(funcCode,rc) 
#define PD_TRACE1(funcCode,pack0) 
#define PD_TRACE2(funcCode,pack0,pack1) 
#define PD_TRACE3(funcCode,pack0,pack1,pack2) 
#define PD_TRACE4(funcCode,pack0,pack1,pack2,pack3) 
#define PD_TRACE5(funcCode,pack0,pack1,pack2,pack3,pack4) 
#define PD_TRACE6(funcCode,pack0,pack1,pack2,pack3,pack4,pack5) 
#define PD_TRACE7(funcCode,pack0,pack1,pack2,pack3,pack4,pack5,pack6) 
#define PD_TRACE8(funcCode,pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7) 
#define PD_TRACE9(funcCode,pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7,pack8) 

#define PD_PACK_NONE 
#define PD_PACK_NULL 
#define PD_PACK_CHAR(x) 
#define PD_PACK_BYTE(x) 
#define PD_PACK_SHORT(x) 
#define PD_PACK_USHORT(x) 
#define PD_PACK_INT(x) 
#define PD_PACK_UINT(x) 
#define PD_PACK_LONG(x) 
#define PD_PACK_ULONG(x) 
#define PD_PACK_FLOAT(x) 
#define PD_PACK_DOUBLE(x) 
#define PD_PACK_STRING(x) 
#define PD_PACK_BSON(x) 

#else

#define PD_TRACE_DECLARE_FUNCTION(x,y) \

#define PD_PACK_NONE      _pdTraceArgTuple ( PD_TRACE_ARGTYPE_NONE, NULL, 0 )
#define PD_PACK_NULL      _pdTraceArgTuple ( PD_TRACE_ARGTYPE_NULL, NULL, 0 )
#define PD_PACK_CHAR(x)   _pdTraceArgTuple ( PD_TRACE_ARGTYPE_CHAR, &x, 1 )
#define PD_PACK_BYTE(x)   _pdTraceArgTuple ( PD_TRACE_ARGTYPE_BYTE, &x, 1 )
#define PD_PACK_SHORT(x)  _pdTraceArgTuple ( PD_TRACE_ARGTYPE_SHORT, &x, 2 )
#define PD_PACK_USHORT(x) _pdTraceArgTuple ( PD_TRACE_ARGTYPE_USHORT, &x, 2 )
#define PD_PACK_INT(x)    _pdTraceArgTuple ( PD_TRACE_ARGTYPE_INT, &x, 4 )
#define PD_PACK_UINT(x)   _pdTraceArgTuple ( PD_TRACE_ARGTYPE_UINT, &x, 4 )
#define PD_PACK_LONG(x)   _pdTraceArgTuple ( PD_TRACE_ARGTYPE_LONG, &x, 8 )
#define PD_PACK_ULONG(x)  _pdTraceArgTuple ( PD_TRACE_ARGTYPE_ULONG, &x, 8 )
#define PD_PACK_FLOAT(x)  _pdTraceArgTuple ( PD_TRACE_ARGTYPE_FLOAT, &x, 4 )
#define PD_PACK_DOUBLE(x) _pdTraceArgTuple ( PD_TRACE_ARGTYPE_DOUBLE, &x, 8 )
#define PD_PACK_STRING(x) _pdTraceArgTuple ( PD_TRACE_ARGTYPE_STRING, x, ossStrlen(x)+1 )
#define PD_PACK_RAW(x,y)  _pdTraceArgTuple ( PD_TRACE_ARGTYPE_RAW, x, y )
#define PD_PACK_BSON(x)   _pdTraceArgTuple ( PD_TRACE_ARGTYPE_BSONRAW, x.objdata(), x.objsize() )

extern BOOLEAN g_isTraceStarted ;

#define PD_TRACE_ENTRY(funcCode)                                    \
   do {                                                             \
      if ( g_isTraceStarted )                                       \
      {                                                             \
         pdTraceArgTuple argTuple[PD_TRACE_MAX_ARG_NUM] ;           \
         ossMemset ( &argTuple[0], 0, sizeof(argTuple) ) ;          \
         pdTraceFunc ( funcCode, PD_TRACE_RECORD_FLAG_ENTRY,        \
                       __FILE__, __LINE__, &argTuple[0] ) ;         \
      }                                                             \
   } while ( FALSE )

#define PD_TRACE_EXIT(funcCode)                                     \
   do {                                                             \
      if ( g_isTraceStarted )                                       \
      {                                                             \
         pdTraceArgTuple argTuple[PD_TRACE_MAX_ARG_NUM] ;           \
         ossMemset ( &argTuple[0], 0, sizeof(argTuple) ) ;          \
         pdTraceFunc ( funcCode, PD_TRACE_RECORD_FLAG_EXIT,         \
                       __FILE__, __LINE__, &argTuple[0] ) ;         \
      }                                                             \
   } while ( FALSE )

#define PD_TRACE_EXITRC(funcCode,rc)                                \
   do {                                                             \
      if ( g_isTraceStarted )                                       \
      {                                                             \
         pdTraceArgTuple argTuple[PD_TRACE_MAX_ARG_NUM] ;           \
         ossMemset ( &argTuple[0], 0, sizeof(argTuple) ) ;          \
         argTuple[0] = PD_PACK_INT(rc) ;                            \
         pdTraceFunc ( funcCode, PD_TRACE_RECORD_FLAG_EXIT,         \
                       __FILE__, __LINE__, &argTuple[0] ) ;         \
      }                                                             \
   } while ( FALSE )

#define PD_TRACE_FUNC(funcCode,file,line,pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7,pack8) \
   do {                                                             \
      if ( g_isTraceStarted )                                       \
      {                                                             \
         pdTraceArgTuple argTuple[PD_TRACE_MAX_ARG_NUM] ;           \
         argTuple[0] = pack0 ;                                      \
         argTuple[1] = pack1 ;                                      \
         argTuple[2] = pack2 ;                                      \
         argTuple[3] = pack3 ;                                      \
         argTuple[4] = pack4 ;                                      \
         argTuple[5] = pack5 ;                                      \
         argTuple[6] = pack6 ;                                      \
         argTuple[7] = pack7 ;                                      \
         argTuple[8] = pack8 ;                                      \
         pdTraceFunc ( funcCode, PD_TRACE_RECORD_FLAG_NORMAL,       \
                       file, line,                                  \
                       &argTuple[0] ) ;                             \
      }                                                             \
   } while ( FALSE )

#define PD_TRACE0(funcCode)                               \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE1(funcCode,pack0)                         \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE2(funcCode,pack0,pack1)                   \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE3(funcCode,pack0,pack1,pack2)             \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE4(funcCode,pack0,pack1,pack2,pack3)       \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      pack3,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE5(funcCode,pack0,pack1,pack2,pack3,pack4) \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      pack3,                              \
                      pack4,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE6(funcCode,pack0,pack1,pack2,pack3,pack4,pack5) \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      pack3,                              \
                      pack4,                              \
                      pack5,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE7(funcCode,pack0,pack1,pack2,pack3,pack4,pack5,pack6) \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      pack3,                              \
                      pack4,                              \
                      pack5,                              \
                      pack6,                              \
                      PD_PACK_NONE,                       \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE8(funcCode,pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7) \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      pack3,                              \
                      pack4,                              \
                      pack5,                              \
                      pack6,                              \
                      pack7,                              \
                      PD_PACK_NONE ) ;                    \
   } while ( FALSE )

#define PD_TRACE9(funcCode,pack0,pack1,pack2,pack3,pack4,pack5,pack6,pack7,pack8) \
   do {                                                   \
      PD_TRACE_FUNC ( funcCode, __FILE__, __LINE__,       \
                      pack0,                              \
                      pack1,                              \
                      pack2,                              \
                      pack3,                              \
                      pack4,                              \
                      pack5,                              \
                      pack6,                              \
                      pack7,                              \
                      pack8 ) ;                           \
   } while ( FALSE )

#endif // SDB_ENGINE

const CHAR *pdGetTraceFunction ( UINT64 id ) ;
const CHAR *pdGetTraceComponent ( UINT32 id ) ;
void pdTraceFunc ( UINT64 funcCode, INT32 type,
                   const CHAR* file, UINT32 line,
                   pdTraceArgTuple *tuple ) ;

/*
   get global pdtrace cb
*/
pdTraceCB* sdbGetPDTraceCB () ;

#endif // PDTRACE_HPP__
