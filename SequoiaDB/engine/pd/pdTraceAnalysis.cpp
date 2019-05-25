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

   Source File Name = pdTraceAnalysis.hpp

   Descriptive Name = define the operation for analysis trace

   When/how to use: this program may be used to analyze trace file

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who            Description
   ====== =========== ===            ===========================================
          06/16/2017  Huangansheng   Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "pdTraceAnalysis.hpp"
#include "pd.hpp"
#include "../bson/bson.h"
#include <math.h>

using namespace bson ;
using namespace engine ;

#define PD_FUNC_RECORD_TITLE \
   "name, count, avgcost, min, maxIn2OutCost, maxCurrentCost, first, second, third, fourth, fifth"

/*
   _pdTraceParser implement
*/
_pdTraceParser::_pdTraceParser()
{
   _type = PD_TRACE_FORMAT_TYPE_FLOW ;
   _inOpen = FALSE ;
   _pFormatBuf = NULL ;
   _bufSize = 0 ;
   _pMaxRecordBuf = NULL ;
}

_pdTraceParser::~_pdTraceParser()
{
   if ( _inOpen )
   {
      ossClose( _inFile ) ;
      _inOpen = FALSE ;
   }
   if ( _pFormatBuf )
   {
      SDB_OSS_FREE( _pFormatBuf ) ;
   }
   _bufSize = 0 ;
   if ( _pMaxRecordBuf )
   {
      SDB_OSS_FREE( _pMaxRecordBuf ) ;
   }
}

INT32 _pdTraceParser::init( const CHAR *pInputFileName,
                            const CHAR *pOutputFileName,
                            pdTraceFormatType type )
{
   INT32 rc = SDB_OK ;
   CHAR   filePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   if ( !pInputFileName || !pOutputFileName ||
        !(*pInputFileName) || !(*pOutputFileName) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   _type = type ;
   _inFilePath = pInputFileName ;
   ossStrncpy( filePath, pOutputFileName, OSS_MAX_PATHSIZE ) ; 
   _removeSuffix( filePath ) ;

   _pFormatBuf = ( CHAR* )SDB_OSS_MALLOC( 10 * TRACE_CHUNK_SIZE + 1 ) ;
   if ( !_pFormatBuf )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( _pFormatBuf, 0, 10 * TRACE_CHUNK_SIZE + 1 ) ;
   _bufSize = 10 * TRACE_CHUNK_SIZE ;

   _pMaxRecordBuf = ( CHAR* )SDB_OSS_MALLOC( TRACE_RECORD_MAX_SIZE ) ;
   if ( !_pMaxRecordBuf )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( type == PD_TRACE_FORMAT_TYPE_FORMAT )
   {
      _fmtFilePath = string(filePath) + string(".fmt") ;
   }
   else
   {
      _flwFilePath = string(filePath) + string(".flw") ;
      _exceptFilePath = string(filePath) + string(".except") ;
      _sumFilePath = string(filePath) + string(".sumary") ;
      _errFilePath = string(filePath) + string(".error") ;
      _funcFilePath = string(filePath) + string(".funcRecord.csv") ;
   }

   rc = _loadDumpFile( pInputFileName, &_traceHeader ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Load dump file[%s] failed, rc: %d",
              pInputFileName, rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _pdTraceParser::parse()
{
   INT32 rc = SDB_OK ;

   rc = _parseTraceDumpFile( &_inFile, &_traceHeader, _fmtFilePath.c_str() ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Parse file[%s] failed, rc: %d",
              _inFilePath.c_str(), rc ) ;
      goto error ;
   }

   if ( _type != PD_TRACE_FORMAT_TYPE_FORMAT )
   {
      rc = _analysisTraceRecords( &_inFile, &_traceHeader ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Analysis file[%s] failed, rc: %d",
                 _inFilePath.c_str(), rc ) ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

void _pdTraceParser::_removeSuffix( CHAR *path )
{
   INT32 idx = ossStrlen( path ) - 1 ;
   while( idx >= 0 && path[idx] != OSS_FILE_SEP_CHAR )
   {
      if ( path[ idx ] == '.' )
      {
         path[ idx ] = 0 ;
         break ;
      }
      --idx ;
   }

   if ( path[ossStrlen(path)-1] == OSS_FILE_SEP_CHAR )
   {
      ossStrncat( path, "trace", ossStrlen("trace") ) ;
   }
}

INT32 _pdTraceParser::_loadDumpFile( const CHAR *pInputFileName,
                                     pdTraceHeader *pHeader )
{
   INT32 rc                = SDB_OK ;
   INT64 fileSize          = 0 ;
   SINT64 byteRead         = 0 ;

   rc = ossOpen( pInputFileName, OSS_READONLY|OSS_SHAREREAD, 0, _inFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Open file[%s] failed, rc: %d",
              pInputFileName, rc ) ;
      goto error ;
   }
   _inOpen = TRUE ;

   rc = ossGetFileSize( &_inFile, &fileSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to get trace file[%s] size, rc: %d",
              pInputFileName, rc ) ;
      goto error ;
   }

   if ( fileSize < (INT64)sizeof( pdTraceHeader ) )
   {
      PD_LOG( PDERROR, "File[%s] size[%lld] is less than TRACEHEADER size",
              pInputFileName, fileSize ) ;
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }

   rc = ossReadN( &_inFile, sizeof( pdTraceHeader ),
                  ( CHAR* )pHeader, byteRead ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Read trace header from file[%s] failed, rc: %d",
              pInputFileName, rc ) ;
      goto error ;
   }

   if ( 0 != ossMemcmp( pHeader->_eyeCatcher, TRACECB_EYE_CATCHER,
                        TRACECB_EYE_CATCHER_SIZE ) )
   {
      PD_LOG( PDERROR, "Invalid eye catcher" ) ;
      rc = SDB_PD_TRACE_FILE_INVALID ;
      goto error ;
   }
   if ( pHeader->_version != PD_TRACE_VERSION_CUR )
   {
      PD_LOG( PDERROR, "Version[%d] is not compatible", pHeader->_version ) ;
      rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      goto error ;
   }
   if ( sizeof( pdTraceHeader ) != pHeader->_headerSize )
   {
      PD_LOG( PDERROR, "Header size[%u] is invalid",
              pHeader->_headerSize ) ;
      rc = SDB_PD_TRACE_FILE_INVALID ;
      goto error ;
   }
   if ( pHeader->_bufSize + pHeader->_headerSize != (UINT64)fileSize )
   {
      PD_LOG( PDERROR, "BufSize[%llu] + HeaderSize[%u] is not the same with "
              "FileSize[%llu]", pHeader->_bufSize, pHeader->_headerSize,
              fileSize ) ;
      rc = SDB_PD_TRACE_FILE_INVALID ;
      goto error ;
   }
   if ( pHeader->_bufHeader > pHeader->_bufSize ||
        pHeader->_bufTail > pHeader->_bufSize )
   {
      PD_LOG( PDERROR, "Buffer info[Header:%llu, Tail:%llu] is more than "
              "buffSize[%llu]", pHeader->_bufHeader, pHeader->_bufTail,
              pHeader->_bufSize ) ;
      rc = SDB_PD_TRACE_FILE_INVALID ;
      goto error ;
   }

done :
   return rc ;
error :
   if ( _inOpen )
   {
      ossClose( _inFile ) ;
      _inOpen = FALSE ;
   }
   goto done ;
}

INT32 _pdTraceParser::_parseTraceDumpFile( OSSFILE *file, 
                                           const pdTraceHeader *pHeader,
                                           const CHAR *fmtFilePath )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isOpen = FALSE ;
   OSSFILE fmtFile ;
   UINT64 cursor = 0 ;
   UINT64 endPos = 0 ;
   UINT64 readLen = 0 ;
   CHAR *pTmpBuf = NULL ;
   UINT32 sequenceNum = 0 ;
   pdTraceRecordIndex recIdx ;
   const pdTraceRecord *record = NULL ;
   UINT32 dataOffset = 0 ;

   pTmpBuf = (CHAR*)SDB_OSS_MALLOC( TRACE_CHUNK_SIZE ) ;
   if ( !pTmpBuf )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( fmtFilePath && 0 != fmtFilePath[0] )
   {
      rc = ossOpen( fmtFilePath, OSS_REPLACE|OSS_READWRITE,
                    OSS_DEFAULTFILE, fmtFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open file[%s] failed, rc: %d",
                 fmtFilePath, rc ) ;
         goto error ;
      }
      isOpen = TRUE ;
   }

   cursor = pHeader->_bufHeader + pHeader->_headerSize ;
   endPos = pHeader->_bufTail + pHeader->_headerSize ;

   if ( cursor >= pHeader->_bufSize + pHeader->_headerSize )
   {
      cursor = pHeader->_headerSize ;
   }

   rc = ossSeek( file, (INT64)cursor, OSS_SEEK_SET ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Seek to %llu failed, rc: %d", cursor, rc ) ;
      goto error ;
   }

   while ( /*cursor != endPos &&*/ readLen < pHeader->_bufSize )
   {
      INT64 hasRead = 0 ;
      rc = ossReadN( file, TRACE_CHUNK_SIZE, pTmpBuf, hasRead ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Read trunk failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( hasRead != TRACE_CHUNK_SIZE )
      {
         PD_LOG( PDERROR, "Read size[%llu] is not the same with %u",
                 hasRead, TRACE_CHUNK_SIZE ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      readLen += hasRead ;

      dataOffset = 0 ;
      while( cursor + dataOffset != endPos && dataOffset < TRACE_CHUNK_SIZE )
      {
         record = ( const pdTraceRecord* )( pTmpBuf + dataOffset ) ;
         recIdx = pdTraceRecordIndex( ++sequenceNum,
                                      cursor + dataOffset ) ;
         if ( 0 != ossMemcmp( record->_eyeCatcher, TRACE_EYE_CATCHER,
                              TRACE_EYE_CATCHER_SIZE ) ||
              record->_recordSize < sizeof( pdTraceRecord ) ||
              record->_recordSize > TRACE_RECORD_MAX_SIZE )
         {
            break ;
         }

         if ( isOpen )
         {
            rc = _outputTraceRecordByFMT( &fmtFile, record, sequenceNum ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Trace record to fmt file failed, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
         else
         {
            _tid2recordsMap[ record->_tid ].push_back( recIdx ) ;
         }
         dataOffset += record->_recordSize ;
      }

      cursor += hasRead ;
      if ( cursor >= pHeader->_bufSize + pHeader->_headerSize )
      {
         cursor = pHeader->_headerSize ;
         rc = ossSeek( file, (INT64)cursor, OSS_SEEK_SET ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Seek to %llu failed, rc: %d",
                    cursor, rc ) ;
            goto error ;
         }
      }
   }

done :
   if ( pTmpBuf )
   {
      SDB_OSS_FREE( pTmpBuf ) ;
   }
   if ( isOpen )
   {
      ossClose( fmtFile ) ;
   }
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceParser::_analysisTraceRecords( OSSFILE *file,
                                             const pdTraceHeader *pHeader )
{
   INT32 rc = SDB_OK ;
   OSSFILE errFile ;
   BOOLEAN errOpen = FALSE ;
   OSSFILE funcRecFile ;
   BOOLEAN funcRecOpen = FALSE ;
   OSSFILE flwFile ;
   BOOLEAN flwOpen = FALSE ;
   OSSFILE exceptFile ;
   BOOLEAN exceptOpen = FALSE ;

   PD_MAP_FUNCS errFunctions ;
   PD_MAP_RECORD summaryRecords ;

   rc = ossOpen( _errFilePath.c_str(), OSS_REPLACE|OSS_READWRITE,
                 OSS_DEFAULTFILE, errFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Create file[%s] failed, rc: %d",
              _errFilePath.c_str(), rc ) ;
      goto error ;
   }
   errOpen = TRUE ;

   rc = ossOpen( _funcFilePath.c_str(), OSS_REPLACE|OSS_READWRITE,
                 OSS_DEFAULTFILE, funcRecFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Create file[%s] failed, rc: %d",
              _funcFilePath.c_str(), rc ) ;
      goto error ;
   }
   funcRecOpen = TRUE ;

   rc = ossOpen( _flwFilePath.c_str(), OSS_REPLACE|OSS_READWRITE,
                 OSS_DEFAULTFILE, flwFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Create file[%s] failed, rc: %d",
              _flwFilePath.c_str(), rc ) ;
      goto error ;
   }
   flwOpen = TRUE ;

   rc = ossOpen( _exceptFilePath.c_str(), OSS_REPLACE|OSS_READWRITE,
                 OSS_DEFAULTFILE, exceptFile ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Create file[%s] failed, rc: %d",
              _exceptFilePath.c_str(), rc ) ;
      goto error ;
   }
   exceptOpen = TRUE ;

   for ( PD_MAP_TID_RECORDS_IT it = _tid2recordsMap.begin() ;
         it != _tid2recordsMap.end() ;
         ++it )
   {
      rc = _analysisRecordsByThread( it->first, pHeader, it->second, 
                                     file, &flwFile,
                                     errFunctions,
                                     summaryRecords ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Analysis record by thread failed, rc: %d", rc ) ;
         goto error ;
      }
   }

   rc = _dealWithExceptRecords( pHeader, file, &funcRecFile,
                                &exceptFile, summaryRecords ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Deal with except records failed, rc: %d", rc ) ;
      goto error ;
   }

   _outputErrorFunctions( errFunctions, &errFile ) ;

done :
   if ( errOpen )
   {
      ossClose( errFile ) ;
   }
   if ( funcRecOpen )
   {
      ossClose( funcRecFile ) ;
   }
   if ( flwOpen )
   {
      ossClose( flwFile ) ;
   }
   if ( exceptOpen )
   {
      ossClose( exceptFile ) ;
   }
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceParser::_analysisRecordsByThread( UINT32 tid,
                                                const pdTraceHeader *pHeader,
                                                const PD_VEC_RECORD_IDX &vecRecordIdx,
                                                OSSFILE *inFile,
                                                OSSFILE *flwFile,
                                                PD_MAP_FUNCS &errFunctions,
                                                PD_MAP_RECORD &summaryRecords )
{
   INT32 rc = SDB_OK ;
   INT32 numIndent = 0 ;
   UINT64 preTimestamp = 0 ;
   UINT64 curTimeStamp = 0 ;
   UINT64 timeInterval = 0 ;
   UINT64 cursor = 0 ;
   UINT32 idx = 0 ;
   UINT32 length = 0 ;
   const pdTraceRecord *pRecord = NULL ;
   PD_STACK_FUNCS funcStack ;

   length = ossSnprintf( _pFormatBuf, _bufSize,
                         OSS_NEWLINE OSS_NEWLINE"tid: %u"OSS_NEWLINE,
                         tid ) ;
   rc = ossWriteN( flwFile, _pFormatBuf, length ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write to flw file failed, rc: %d", rc ) ;
      goto error ;
   }

   for ( ; idx < vecRecordIdx.size() ; ++idx )
   {
      cursor = vecRecordIdx[idx]._offset ;
      rc = _readTraceRecord( inFile, cursor, _pMaxRecordBuf ) ;
      if ( rc )
      {
         goto error ;
      }

      pRecord = ( const pdTraceRecord* )_pMaxRecordBuf ;
      curTimeStamp = pRecord->_timestamp ;

      if ( preTimestamp > 0 )
      {
         timeInterval = curTimeStamp - preTimestamp ;
      }

      _analysisFunctionStack( funcStack, idx,
                              vecRecordIdx[idx]._sequenceNum, 
                              timeInterval, pRecord,
                              errFunctions,
                              summaryRecords ) ;

      rc = _outputTraceRecordByThread( flwFile, vecRecordIdx[idx]._sequenceNum, 
                                       numIndent, timeInterval,
                                       _pMaxRecordBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Write info to flw file failed, rc: %d", rc ) ;
         goto error ;
      }

      preTimestamp = curTimeStamp ;
   }

done :
   return rc ;
error :
   goto done ;
}


INT32 _pdTraceParser::_dealWithExceptRecords( const pdTraceHeader *pHeader,
                                              OSSFILE *inFile,
                                              OSSFILE *funcRecFile,
                                              OSSFILE *exceptFile,
                                              PD_MAP_RECORD &summaryRecords )
{
   INT32 rc = SDB_OK ;
   PD_SET_FUNCS exceptRecords ;
   UINT32 length = 0 ;

   length += ossSnprintf( _pFormatBuf, _bufSize, PD_FUNC_RECORD_TITLE ) ;
   rc = ossWriteN( funcRecFile, _pFormatBuf, length ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write func record file failed, rc: %d", rc ) ;
      goto error ;
   }

   for ( PD_MAP_RECORD::iterator it = summaryRecords.begin() ; 
         it != summaryRecords.end() ; 
         ++it )
   {
      rc = _selectExceptRecords( it->second, exceptRecords ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Select except records failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _outputFunctionSummaryRecord( it->first, it->second, funcRecFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Output to function record file failed, rc: %d",
                 rc ) ;
         goto error ;
      }
   }

   rc = _outputExceptionReport( inFile, exceptFile, exceptRecords ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Output exception record failed, rc: %d", rc ) ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32  _pdTraceParser::_outputErrorFunctions( PD_MAP_FUNCS &errFunctions, 
                                              OSSFILE *errFile )
{
   INT32 nCount = 0 ;
   UINT32 length = 0 ;

   for ( PD_MAP_FUNCS::iterator it = errFunctions.begin() ;
         it != errFunctions.end() ;
         ++it )
   {
      length = ossSnprintf( _pFormatBuf, _bufSize,
                            "%d: %s "OSS_NEWLINE"       ( ",
                            nCount, pdGetTraceFunction( it->first ) ) ;

      for ( UINT32 item = 0 ; item < it->second.size() ; ++item )
      {
         if ( item > 0 )
         {
            length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                                   ", %d", it->second[ item ] ) ;
         }
         else
         {
            length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                                   "%d", it->second[ item ] ) ;
         }
      }

      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             " )"OSS_NEWLINE ) ;

      ossWriteN( errFile, _pFormatBuf, length ) ;

      ++nCount ;
   }

   return SDB_OK ;
}

INT32 _pdTraceParser::_readTraceRecord( OSSFILE *file,
                                        UINT64 cursor,
                                        CHAR *tempBuf )
{
   INT32 rc = SDB_OK ;
   UINT32 headerLen = sizeof(pdTraceRecord) ;
   const pdTraceRecord *record = NULL ;
   SINT64 hasRead = 0 ;

   rc = ossSeek( file, (INT64)cursor, OSS_SEEK_SET ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to seek to %llu, rc: %d", cursor, rc ) ;
      goto error ;
   }

   rc = ossReadN( file, headerLen, tempBuf, hasRead ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to read record header, rc: %d", rc ) ;
      goto error ;
   }

   record = ( const pdTraceRecord* )tempBuf ;
   if ( 0 != ossMemcmp( record->_eyeCatcher, TRACE_EYE_CATCHER,
                        TRACE_EYE_CATCHER_SIZE ) ||
        record->_recordSize < sizeof( pdTraceRecord ) ||
        record->_recordSize > TRACE_RECORD_MAX_SIZE )
   {
      PD_LOG( PDERROR, "Record in %llu is invalid", cursor ) ;
      rc = SDB_PD_TRACE_FILE_INVALID ;
      goto error ;
   }

   if ( record->_recordSize > headerLen )
   {
      rc = ossReadN( file, record->_recordSize - headerLen,
                     tempBuf + headerLen, hasRead ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to read record last data, rc: %d", rc ) ;
         goto error ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

void _pdTraceParser::_analysisFunctionStack( PD_STACK_FUNCS &funStack,
                                             UINT32 recdIndexIdx,
                                             UINT32 sequenceNum,
                                             UINT64 timeInterval,
                                             const pdTraceRecord *pRecord,
                                             PD_MAP_FUNCS &errFunctions, 
                                             PD_MAP_RECORD &summaryRecords )
{
   BOOLEAN isEmpty = funStack.empty() ;
   UINT64 curTimestamp = pRecord->_timestamp ;

   if ( !isEmpty )
   {
      pdFunctionRecord &topRecord = funStack.top() ;
      topRecord._maxTimeInterval = OSS_MAX( topRecord._maxTimeInterval,
                                            timeInterval ) ;
   }

   if ( pRecord->_flag == PD_TRACE_RECORD_FLAG_ENTRY )
   {
      if ( !isEmpty )
      {
         ++funStack.top()._nChild ;
      }
      funStack.push( pdFunctionRecord( recdIndexIdx, sequenceNum,
                                       pRecord->_tid,
                                       0, curTimestamp,
                                       0, 0,
                                       pRecord->_functionID, 
                                       curTimestamp ) ) ; 
   }
   else if ( !isEmpty && pRecord->_flag == PD_TRACE_RECORD_FLAG_EXIT )
   {
      pdFunctionRecord popRecord = funStack.top() ;
      funStack.pop() ;

      if ( popRecord._functionID == pRecord->_functionID )
      {
         popRecord._cost = curTimestamp - popRecord._cost ;
         popRecord._totalCost = curTimestamp - popRecord._start ;
         summaryRecords[ popRecord._functionID ].insert( popRecord ) ;

         if ( !funStack.empty() )
         {
            funStack.top()._cost += popRecord._totalCost ;
            ++funStack.top()._nChild ;
         }
      }
      else
      {
         PD_STACK_FUNCS nullStack ;
         errFunctions[ popRecord._functionID ].push_back( sequenceNum ) ;
         funStack = nullStack ;
      }
   }
   else if( !isEmpty && pRecord->_flag == PD_TRACE_RECORD_FLAG_NORMAL )
   {
      ++funStack.top()._nChild ;
   }
}

INT32 _pdTraceParser::_outputTraceRecordByThread( OSSFILE *flwFile, 
                                                  UINT32 sequence, 
                                                  INT32 &numIndent, 
                                                  UINT64 timeInterval, 
                                                  const CHAR *recordBuf )
{
   INT32 rc = SDB_OK ;
   CHAR timestamp[64] = { 0 } ;
   const pdTraceRecord *record = (const pdTraceRecord *)recordBuf ;
   UINT32 length = 0 ;
   ossTimestamp tmpTime ;

   length += ossSnprintf( _pFormatBuf, _bufSize, OSS_NEWLINE"%10u:  ",
                          sequence ) ;

   if ( record->_flag == PD_TRACE_RECORD_FLAG_EXIT )
   {
      --numIndent ;
      if ( numIndent < 0 )
      {
         numIndent = 0 ;
      }
   }
   for ( INT32 i = 0 ; i < numIndent ; ++i )
   {
      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             "| " ) ;
   }

   length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                          "%s", pdGetTraceFunction( record->_functionID ) ) ;

   if ( record->_flag == PD_TRACE_RECORD_FLAG_ENTRY )
   {
      length += ossSnprintf( _pFormatBuf + length , _bufSize - length,
                             " Entry" ) ;
      ++numIndent ;
   }
   else if ( record->_flag == PD_TRACE_RECORD_FLAG_EXIT )
   {
      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             " Exit" ) ;
      if ( 1 == record->_numArgs )
      {
         pdTraceArgument *arg = record->getArg( 0 ) ;
         if ( PD_TRACE_ARGTYPE_INT == arg->getType() )
         {
            INT32 retCode = *(INT32*)arg->argData() ;
            if ( SDB_OK != retCode )
            {
               length += ossSnprintf( _pFormatBuf + length,
                                      _bufSize - length,
                                      "[retCode=%d]",
                                      retCode ) ;
            }
         } // if ( PD_TRACE_ARGTYPE_INT == arg->getType() )
      } // if ( record->_numArgs == 1 )
   } // else if ( record->_flag == PD_TRACE_RECORD_FLAG_EXIT )

   tmpTime = record->getCurTime() ;
   ossTimestampToString ( tmpTime, timestamp ) ;

   length += ossSnprintf( _pFormatBuf + length,
                          _bufSize - length,
                          "(%u): %s",
                          record->_line,
                          timestamp ) ;

   if ( timeInterval > TRACE_RECORD_EXCEPTION_TIME_THRESHOLD )
   {
      length += ossSnprintf( _pFormatBuf + length,
                             _bufSize - length,
                             "  (%lld)",
                             timeInterval ) ;
   }

   rc = ossWriteN( flwFile, _pFormatBuf, length ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write to file failed, rc: %d", rc ) ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 _pdTraceParser::_selectExceptRecords( pdFunctionSummaryRecord &record, 
                                            PD_SET_FUNCS &exceptRecords )
{
   INT32 rc = SDB_OK ;

   if ( record._count > 0 &&
        record._reserveRecords[0]._maxTimeInterval >
        3 * TRACE_RECORD_EXCEPTION_TIME_THRESHOLD )
   {
      exceptRecords.insert( record._reserveRecords[0] ) ;
   }

   return rc ;
}

INT32 _pdTraceParser::_outputFunctionSummaryRecord( UINT64 funcId,
                                                    pdFunctionSummaryRecord &record, 
                                                    OSSFILE *funcFile )
{
   INT32 rc = SDB_OK ;
   UINT32 length = 0 ;
   UINT32 count = 0 ;

   length = ossSnprintf( _pFormatBuf, _bufSize,
                         OSS_NEWLINE"%s,%d,%.1f,%u,%lld,%lld",
                         pdGetTraceFunction(funcId),
                         record._count,
                         record.avgCost(),
                         record._minCost,
                         record._maxIn2OutCost,
                         record._maxCurrentCost ) ;

   count = OSS_MIN ( record._count, NUMBER_OF_FUNCTION_RECORD_RESERVATION ) ;
   for ( UINT32 idx = 0; idx < count ; ++idx )
   {
      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             ",\"(%u, %lld, %lld, %lld)\"",
                             record._reserveRecords[idx]._sequenceNum,
                             record._reserveRecords[idx]._totalCost,
                             record._reserveRecords[idx]._cost,
                             record._reserveRecords[idx]._maxTimeInterval ) ;
   }

   rc = ossWriteN( funcFile, _pFormatBuf, length ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write func file failed, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _pdTraceParser::_outputExceptionReport( OSSFILE *inFile, 
                                              OSSFILE *exceptFile, 
                                              PD_SET_FUNCS &exceptRecords )
{
   INT32 rc = SDB_OK ;
   UINT32 tid = 0 ;
   UINT64 cursor = 0 ;
   INT32  indent = 0 ;
   INT32  nScan = 0 ;
   BOOLEAN isChild = TRUE ;
   BOOLEAN isFinish = FALSE ;
   BOOLEAN isPrintAll = TRUE ;
   UINT32 length = 0 ;

   const pdTraceRecord *pRecord = NULL ;
   pdTraceRecord preRecord ;

   UINT64 preTimestamp = 0 ;
   UINT64 curTimestap = 0 ;
   UINT64 timeInterval = 0 ;

   for ( PD_SET_FUNCS::reverse_iterator rit = exceptRecords.rbegin() ; 
         rit != exceptRecords.rend() ;
         ++rit )
   {
      isPrintAll = TRUE ;
      indent = 0 ;
      nScan = 0 ;
      isFinish = FALSE ;
      preTimestamp = 0 ;
      tid = rit->_tid ;

      length = ossSnprintf( _pFormatBuf, _bufSize,
                            OSS_NEWLINE"------------------------------"
                            "-----------------------------------------"
                            "--"OSS_NEWLINE ) ;

      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             "%s "OSS_NEWLINE,
                             pdGetTraceFunction( rit->_functionID ) ) ;

      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             "sequence: %u tid: %u  cost: %ld (%ld) "
                             OSS_NEWLINE,
                             rit->_sequenceNum,
                             tid,
                             rit->_cost,
                             rit->_maxTimeInterval ) ;

      if ( rit->_nChild > MAX_LENGTH_PRINT_LINE_EXCEPTION_RECORD )
      {
         isPrintAll = FALSE ;
      }

      for ( UINT32 idx = rit->_indexidx ;
            idx < _tid2recordsMap[tid].size() && !isFinish ;
            ++idx )
      {
         timeInterval = 0 ; 
         isChild = TRUE ;

         cursor = _tid2recordsMap[tid][idx]._offset ;
         rc = _readTraceRecord( inFile, cursor, _pMaxRecordBuf ) ;
         if ( rc )
         {
            break ;
         }

         pRecord = (const pdTraceRecord*)_pMaxRecordBuf ;
         if ( pRecord->_flag == PD_TRACE_RECORD_FLAG_EXIT )
         {
            --indent ;
            if( indent > 1 )
            {
               continue ;
            }
            if( indent == 0 )
            {
               isFinish = TRUE ;
            }
         }
         else if ( pRecord->_flag == PD_TRACE_RECORD_FLAG_ENTRY )
         {
            ++indent ;
            if( indent > 2 )
            {
               continue ;
            }
         }
         else
         {
            if( indent > 1 )
            {
               continue ;
            }
         }

         curTimestap = pRecord->_timestamp ;
         if ( preTimestamp > 0 &&
              !( pRecord->_flag == PD_TRACE_RECORD_FLAG_EXIT &&
                 indent > 0 ) )
         {
            timeInterval = curTimestap - preTimestamp ;
         }

         if ( 0 == preTimestamp || isFinish )
         {
            isChild = FALSE ;
         }

         if ( isPrintAll )
         {
            length += _outputTraceRecordFormat( _pFormatBuf + length,
                                                _bufSize - length,
                                                pRecord,
                                                _tid2recordsMap[tid][idx]._sequenceNum,
                                                isChild ) ;
            if ( timeInterval > 0 )
            {
               length += ossSnprintf( _pFormatBuf + length,
                                      _bufSize - length,
                                      "      (%ld)",
                                      timeInterval ) ;
            }
         }
         else
         {
            if ( 0 == preTimestamp  )
            {
               length += _outputTraceRecordFormat( _pFormatBuf + length,
                                                   _bufSize - length,
                                                   pRecord,
                                                   _tid2recordsMap[tid][idx]._sequenceNum,
                                                   isChild ) ;
            }
            else if ( isFinish ||
                      timeInterval > TRACE_RECORD_EXCEPTION_TIME_THRESHOLD )
            {
               if( nScan > 2 )
               {
                  length += ossSnprintf( _pFormatBuf + length,
                                         _bufSize - length,
                                         OSS_NEWLINE"           . . . .  " ) ;
               }
               if( nScan > 1 )
               {
                  length += _outputTraceRecordFormat( _pFormatBuf + length,
                                                      _bufSize - length,
                                                      &preRecord,
                                                      _tid2recordsMap[tid][idx-1]._sequenceNum,
                                                      TRUE ) ;
               }
               length += _outputTraceRecordFormat( _pFormatBuf + length,
                                                   _bufSize - length,
                                                   pRecord,
                                                   _tid2recordsMap[tid][idx]._sequenceNum,
                                                   isChild ) ;
               if ( timeInterval > 0 )
               {
                  length += ossSnprintf( _pFormatBuf + length,
                                         _bufSize - length,
                                         "      (%ld)",
                                         timeInterval ) ;
               }
               nScan = 0 ;
            }
            preRecord = *pRecord ;
            ++nScan ;
         }

         preTimestamp = curTimestap ;
      }

      length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                             OSS_NEWLINE ) ;
      rc = ossWriteN( exceptFile, _pFormatBuf, length ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Write file failed, rc: %d", rc ) ;
         goto error ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

UINT32 _pdTraceParser::_outputTraceRecordFormat( CHAR *pBuffer,
                                                 UINT32 bufSize,
                                                 const pdTraceRecord *record, 
                                                 UINT32 sequence, 
                                                 BOOLEAN isChild )
{
   CHAR timestamp[ 64 ] = { 0 } ;
   UINT32 length = 0 ;
   ossTimestamp tmpTime ;

   length = ossSnprintf( pBuffer, bufSize,
                         OSS_NEWLINE"%10u:  ",
                         sequence ) ;

   if ( isChild )
   {
      length += ossSnprintf( &pBuffer[ length ],
                             bufSize - length,
                             "| " ) ;
   }

   length += ossSnprintf( &pBuffer[ length ],
                          bufSize - length,
                          "%s",
                          pdGetTraceFunction( record->_functionID ) ) ;

   if ( record->_flag == PD_TRACE_RECORD_FLAG_ENTRY )
   {
      length += ossSnprintf( &pBuffer[ length ],
                             bufSize - length,
                             " Entry" ) ;
   }
   else if ( record->_flag == PD_TRACE_RECORD_FLAG_EXIT )
   {
      length += ossSnprintf( &pBuffer[ length ],
                             bufSize - length,
                             " Exit" ) ;
   }

   tmpTime = record->getCurTime() ;
   ossTimestampToString ( tmpTime, timestamp ) ;
   length += ossSnprintf( &pBuffer[ length ], bufSize - length,
                          "(%u): %s", record->_line, timestamp ) ;

   return length ;
}

INT32 _pdTraceParser::_outputTraceRecordByFMT( OSSFILE *out, 
                                               const pdTraceRecord *pRecord, 
                                               UINT32 sequenceNum )
{
   INT32 rc = SDB_OK ;
   CHAR timestamp[ 64 ] = { 0 } ;
   pdTraceArgument *pArgs = NULL ;
   UINT32 length = 0 ;
   ossTimestamp tmpTime ;
   const CHAR *pTypeStr = "Normal" ;

   if ( PD_TRACE_RECORD_FLAG_EXIT == pRecord->_flag )
   {
      pTypeStr = "Exit" ;
   }
   else if ( PD_TRACE_RECORD_FLAG_ENTRY == pRecord->_flag )
   {
      pTypeStr = "Entry" ;
   }

   length = ossSnprintf( _pFormatBuf, _bufSize, "%u: ", sequenceNum ) ;

   tmpTime = pRecord->getCurTime() ;
   ossTimestampToString ( tmpTime, timestamp ) ;
   length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                          "%s %s(%u): %s"OSS_NEWLINE,
                          pdGetTraceFunction( pRecord->_functionID ),
                          pTypeStr,
                          pRecord->_line,
                          timestamp ) ;

   length += ossSnprintf( _pFormatBuf + length, _bufSize - length,
                          "tid: %u, numArgs: %u"OSS_NEWLINE,
                          pRecord->_tid,
                          pRecord->_numArgs ) ;

   pArgs = pRecord->getArg( 0 ) ;
   for ( UINT32 i = 0 ; i < pRecord->_numArgs ; ++i )
   {
      if ( ( const CHAR* )pArgs - ( const CHAR* )pRecord
           >= TRACE_RECORD_MAX_SIZE ||
           ( const CHAR* )pArgs - ( const CHAR* )pRecord <
           (INT32)sizeof( pdTraceRecord ) )
      {
         rc = SDB_PD_TRACE_FILE_INVALID ;
         PD_LOG( PDERROR, "Invalid argument offset" ) ;
         goto error ;
      }

      length += ossSnprintf( _pFormatBuf + length,
                             _bufSize - length,
                             "\targ%d:",
                             i ) ;

      switch ( pArgs->getType() )
      {
         case PD_TRACE_ARGTYPE_NULL :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\tNULL"OSS_NEWLINE ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_CHAR :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%c"OSS_NEWLINE,
                                   *(pArgs->argData()) ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_BYTE :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t0x%x"OSS_NEWLINE,
                                   (UINT32)(*(pArgs->argData())) ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_SHORT :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%d"OSS_NEWLINE,
                                   (INT32)(*(INT16*)pArgs->argData()) ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_USHORT :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%u"OSS_NEWLINE,
                                   (UINT32)(*(UINT16*)pArgs->argData()) ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_INT :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%d"OSS_NEWLINE,
                                   *(INT32*)pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_UINT :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%u"OSS_NEWLINE,
                                   *(UINT32*)pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_LONG :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%lld"OSS_NEWLINE,
                                   *(INT64*)pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_ULONG :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%llu"OSS_NEWLINE,
                                   *(UINT64*)pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_FLOAT :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%f"OSS_NEWLINE,
                                   *(FLOAT32*)pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_DOUBLE :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%f"OSS_NEWLINE,
                                   *(FLOAT64*)pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_STRING :
         {
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   "\t%s"OSS_NEWLINE,
                                   pArgs->argData() ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_RAW :
         {
            INT32 rawSize = pArgs->dataSize() ;
            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   OSS_NEWLINE ) ;

            length += ossHexDumpBuffer ( pArgs->argData(),
                                         rawSize,
                                         _pFormatBuf + length,
                                         _bufSize - length,
                                         NULL,
                                         OSS_HEXDUMP_INCLUDE_ADDR ) ;

            length += ossSnprintf( _pFormatBuf + length,
                                   _bufSize - length,
                                   OSS_NEWLINE ) ;
            break ;
         }
         case PD_TRACE_ARGTYPE_BSONRAW :
         {
            try
            {
               BSONObj argument( pArgs->argData() ) ;
               length += ossSnprintf( _pFormatBuf + length,
                                      _bufSize - length,
                                      "\t%s"OSS_NEWLINE,
                                      argument.toString( FALSE, TRUE ).c_str() ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG ( PDERROR, "Failed to print BSON object: %s", e.what() ) ;
            }
            break ;
         }
         case PD_TRACE_ARGTYPE_NONE :
         default :
            break ;
      }

      pArgs = ( pdTraceArgument* )( ( const CHAR* )pArgs +
                                    pArgs->argSize() ) ;
   }

   length += ossSnprintf( _pFormatBuf + length,
                          _bufSize - length,
                          OSS_NEWLINE ) ;

   rc = ossWriteN( out, _pFormatBuf, length ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Write file failed, rc: %d", rc ) ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

