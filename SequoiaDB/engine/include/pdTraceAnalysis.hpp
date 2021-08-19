/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = pdTraceAnalysis.hpp

   Descriptive Name = define the operation for analysis trace

   When/how to use: this program may be used to analyze trace file

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who            Description
   ====== =========== ===            ==============================================
          06/16/2017  Huangansheng   Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PDTRACEANALYSIS_HPP__
#define PDTRACEANALYSIS_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pdTrace.hpp"
#include "ossIO.hpp"
#include <list>
#include <vector>
#include <map>
#include <set>
#include <stack>

#define  MAX_LENGTH_PRINT_LINE_EXCEPTION_RECORD    ( 40 )
#define  TRACE_RECORD_EXCEPTION_TIME_THRESHOLD     ( 1000 )

using namespace std ;

/*
   _pdTraceRecordIndex define
*/
struct _pdTraceRecordIndex
{
   UINT32  _sequenceNum;
   UINT64  _offset;

   _pdTraceRecordIndex()
   {
      _sequenceNum = 0 ;
      _offset = 0 ;
   }
   _pdTraceRecordIndex( UINT32 sequenceNum, UINT64 offset)
   {
      _sequenceNum = sequenceNum ;
      _offset = offset ;
   }
} ;
typedef _pdTraceRecordIndex pdTraceRecordIndex ;
typedef std::vector<pdTraceRecordIndex>                     PD_VEC_RECORD_IDX ;
typedef std::map<UINT32, PD_VEC_RECORD_IDX >                PD_MAP_TID_RECORDS ;
typedef PD_MAP_TID_RECORDS::iterator                        PD_MAP_TID_RECORDS_IT ;

/*
   _pdFunctionRecord define
*/
struct _pdFunctionRecord
{
   UINT32        _indexidx ;
   UINT32        _sequenceNum ;
   UINT32        _tid ;
   UINT32        _nChild ;
   UINT64        _cost ;
   UINT64        _totalCost ;
   UINT64        _maxTimeInterval ;
   UINT64        _functionID ;
   UINT64        _start ;

   _pdFunctionRecord()
   {
      _indexidx = 0 ;
      _sequenceNum = 0 ;
      _tid = 0 ;
      _nChild = 0 ;
      _cost = 0 ;
      _totalCost = 0 ;
      _maxTimeInterval = 0 ;
      _functionID = 0 ;
      _start = 0 ;
   }
   _pdFunctionRecord( UINT32 indexidx,
                      UINT32 sequence,
                      UINT32 tid,
                      UINT32 nchild,
                      UINT64 cost,
                      UINT64 totalCost,
                      UINT64  maxTimeInterval,
                      UINT64 funcID,
                      UINT64 starttime )
   {
      _indexidx      = indexidx ;
      _sequenceNum   = sequence ;
      _tid           = tid ;
      _nChild        = nchild ;
      _cost          = cost ;
      _totalCost     = totalCost ;
      _maxTimeInterval = maxTimeInterval ;
      _functionID    = funcID ;
      _start         = starttime ;
   }

   bool operator < ( const _pdFunctionRecord &another ) const
   {
      return _maxTimeInterval < another._maxTimeInterval ;
   }
} ;
typedef _pdFunctionRecord pdFunctionRecord ;

#define NUMBER_OF_FUNCTION_RECORD_RESERVATION         ( 5 )

/*
   _pdFunctionSummaryRecord define
*/
struct _pdFunctionSummaryRecord
{
   UINT32               _count ;
   UINT32               _minCost ;
   UINT64               _maxIn2OutCost ;
   UINT64               _maxCurrentCost ;
   UINT64               _totalCost ;
   double               _avgcost ;
   pdFunctionRecord     _reserveRecords[ NUMBER_OF_FUNCTION_RECORD_RESERVATION ] ;

   _pdFunctionSummaryRecord()
   {
      _count = 0 ;
      _minCost = 0 ;
      _totalCost = 0 ;
      _maxIn2OutCost = 0 ;
      _maxCurrentCost = 0 ;
   }

   void insert( const pdFunctionRecord &record )
   {
      INT8 idx = OSS_MIN( _count, NUMBER_OF_FUNCTION_RECORD_RESERVATION ) ;

      for ( idx = idx - 1 ; idx >= 0 ; --idx )
      {
         if ( record._maxTimeInterval <=
              _reserveRecords[ idx ]._maxTimeInterval )
         {
            break ;
         }

         if ( idx != NUMBER_OF_FUNCTION_RECORD_RESERVATION - 1 )
         {
            _reserveRecords[ idx + 1 ] = _reserveRecords[ idx ] ;
         }
      }

      //inset idx+1
      if ( idx + 1 < NUMBER_OF_FUNCTION_RECORD_RESERVATION )
      {
         _reserveRecords[ idx + 1 ] = record ;
      }

      if ( _count == 0 )
      {
         _minCost = (UINT32)record._cost ;
         _maxIn2OutCost = record._totalCost ;
         _maxCurrentCost = record._cost ;
      }
      else
      {
         _minCost = OSS_MIN ( (UINT32)record._cost, _minCost ) ;
         _maxIn2OutCost = OSS_MAX ( record._totalCost, _maxIn2OutCost ) ;
         _maxCurrentCost = OSS_MAX ( record._cost, _maxCurrentCost ) ;
      }

      ++_count ;
      _totalCost += record._cost ;
   }

   FLOAT64 avgCost() const
   {
      if ( _count > 0 )
      {
         return ( FLOAT64 )_totalCost / _count ;
      }
      return 0.0 ;
   }
} ;
typedef _pdFunctionSummaryRecord pdFunctionSummaryRecord ;

typedef std::map<UINT64, std::vector<UINT32> >     PD_MAP_FUNCS ;
typedef std::map<UINT64, pdFunctionSummaryRecord>  PD_MAP_RECORD ;
typedef std::stack<pdFunctionRecord>               PD_STACK_FUNCS ;
typedef std::set<pdFunctionRecord>                 PD_SET_FUNCS ;

/*
   _pdTraceParser define
*/
class _pdTraceParser
{
   public:
      _pdTraceParser() ;
      ~_pdTraceParser() ;

   public:

      INT32    init( const CHAR *pInputFileName,
                     const CHAR *pOutputFileName,
                     pdTraceFormatType type,
                     BOOLEAN useFunctionListFile ) ;

      INT32    parse() ;

   protected:

      void     _removeSuffix( CHAR *path ) ;
      INT32    _loadDumpFile( const CHAR *pInputFileName,
                              pdTraceHeader *pHeader ) ;

      INT32    _parseTraceDumpFile( OSSFILE *file,
                                    const pdTraceHeader *pHeader,
                                    const CHAR *fmtFilePath ) ;

      INT32    _analysisTraceRecords( OSSFILE *file,
                                      const pdTraceHeader *pHeader ) ;

      // 1 output program execution sequence
      // 2 calculate function execution time
      // 3 analyze exception record
      INT32    _analysisRecordsByThread( UINT32 tid,
                                         const pdTraceHeader *pHeader,
                                         const PD_VEC_RECORD_IDX &vecRecordIdx,
                                         OSSFILE *inFile,
                                         OSSFILE *flwFile,
                                         PD_MAP_FUNCS &errFunctions,
                                         PD_MAP_RECORD &summaryRecords ) ;

      // 1 select exception
      // 2 output exception
      INT32 _dealWithExceptRecords( const pdTraceHeader *pHeader,
                                    OSSFILE *inFile,
                                    OSSFILE *funcRecFile,
                                    OSSFILE *exceptFile,
                                    PD_MAP_RECORD &summaryRecords ) ;

      INT32 _outputErrorFunctions( PD_MAP_FUNCS &errFunctions,
                                   OSSFILE *errFile ) ;

      INT32 _readTraceRecord( OSSFILE *file,
                              UINT64 cursor,
                              CHAR *tempBuf ) ;

      // 1 calculate function execution time
      // 2 maxtimeInterval
      // 3 handing error records
      void  _analysisFunctionStack( PD_STACK_FUNCS &funStack,
                                    UINT32 recdIndexIdx,
                                    UINT32 sequenceNum,
                                    UINT64 timeInterval,
                                    const pdTraceRecord *pRecord,
                                    PD_MAP_FUNCS &errFunctions,
                                    PD_MAP_RECORD &summaryRecords ) ;

      INT32 _outputTraceRecordByThread( OSSFILE *flwFile,
                                        UINT32 sequence,
                                        INT32 &numIndent,
                                        UINT64 timeInterval,
                                        const CHAR *recordBuf ) ;

      INT32 _selectExceptRecords( pdFunctionSummaryRecord &record,
                                  PD_SET_FUNCS &exceptRecords ) ;

      INT32 _outputFunctionSummaryRecord( UINT64 funcId,
                                          pdFunctionSummaryRecord &record,
                                          OSSFILE *funcFile ) ;

      INT32 _outputExceptionReport( OSSFILE *inFile,
                                    OSSFILE *exceptFile,
                                    PD_SET_FUNCS &exceptRecords ) ;

      UINT32   _outputTraceRecordFormat( CHAR *pBuffer,
                                         UINT32 bufSize,
                                         const pdTraceRecord *record,
                                         UINT32 sequence,
                                         BOOLEAN isChild ) ;

      INT32 _outputTraceRecordByFMT( OSSFILE *out,
                                     const pdTraceRecord *pRecord,
                                     UINT32 sequenceNum ) ;

      INT32 _outputDBVersionInfo( const CHAR *versionFilePath ) ;

      const CHAR  *_getFunctionName( UINT32 functionID ) ;

      INT32 _dumpFunctionList( OSSFILE *file );

   private:
      pdTraceFormatType _type ;
      OSSFILE  _inFile ;
      BOOLEAN  _inOpen ;
      BOOLEAN  _useFunctionListFile ;

      string   _inFilePath ;
      string   _versionFilePath ;
      string   _fmtFilePath ;
      string   _flwFilePath ;
      string   _sumFilePath ;
      string   _exceptFilePath ;
      string   _errFilePath ;
      string   _funcFilePath ;

      pdTraceHeader        _traceHeader ;
      PD_MAP_TID_RECORDS   _tid2recordsMap ;

      CHAR     *_pFormatBuf ;
      UINT32    _bufSize ;

      CHAR     *_pMaxRecordBuf ;

      UINT32    _functionsCount ;
      CHAR     *_funcNames ;
      UINT32   *_funcOffsetList ;

} ;
typedef _pdTraceParser pdTraceParser ;

#endif // PDTRACEANALYSIS_HPP__

