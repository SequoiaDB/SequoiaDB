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

   Source File Name = dpsArchiveMgr.cpp

   Descriptive Name = Data Protection Services Log Archive Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsArchiveMgr.hpp"
#include "dpsArchiveFile.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsLogFile.hpp"
#include "dpsMessageBlock.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "utilStr.hpp"
#include "dpsTrace.hpp"
#include <sstream>

namespace engine
{
   #define DPS_ARCHIVE_MAX_WAIT_TIME   (30 * 1000) // 30seconds
   #define DPS_ARCHIVE_ONE_WAIT_TIME   (100)       // 100ms
   #define DPS_ARCHIVE_EXPIRED_SECONDS (60 * 60)   // 1hour
   #define DPS_ARCHIVE_QUOTA_BYTES     (1024 * 1024 * 1024) // 1GB

   static DPS_LSN maxLSN( const DPS_LSN& left, const DPS_LSN& right,
                          BOOLEAN considerVersion = FALSE )
   {
      INT32 result = 0 ;

      if ( considerVersion )
      {
         result = left.compare( right ) ;
      }
      else
      {
         result = left.compareOffset( right ) ;
      }

      if ( result >= 0 )
      {
         return left ;
      }
      else
      {
         return right ;
      }
   }

   static DPS_LSN minLSN( const DPS_LSN& left, const DPS_LSN& right,
                          BOOLEAN considerVersion = FALSE )
   {
      INT32 result = 0 ;

      if ( considerVersion )
      {
         result = left.compare( right ) ;
      }
      else
      {
         result = left.compareOffset( right ) ;
      }

      if ( result <= 0 )
      {
         return left ;
      }
      else
      {
         return right ;
      }
   }

   enum dpsArchiveEventType
   {
      DPS_ARCHIVE_EVENT_INVALID           = 0,
      DPS_ARCHIVE_EVENT_ARCHIVE           = 1,
      DPS_ARCHIVE_EVENT_SWITCH_FILE       = 2,
      DPS_ARCHIVE_EVENT_GENERATE_ARCHIVE  = 3,
      DPS_ARCHIVE_EVENT_MAX
   } ;

   #define DPS_ARCHIVE_EVENT_IS_VALID(eventType) \
      ( ( (eventType) > DPS_ARCHIVE_EVENT_INVALID ) && \
        ( (eventType) < DPS_ARCHIVE_EVENT_MAX ) )

   class dpsArchiveEvent: public SDBObject
   {
   private:
      // disallow copy and assign
      dpsArchiveEvent( const dpsArchiveEvent& ) ;
      void operator=( const dpsArchiveEvent& );
   protected:
      dpsArchiveEvent( dpsArchiveEventType type )
         : _type( type ) {}
   public:
      virtual ~dpsArchiveEvent() {}

   public:
      dpsArchiveEventType type() { return _type ; }

   protected:
      dpsArchiveEventType _type ;
   } ;

   class dpsArchiveEventSwitchFile: public dpsArchiveEvent
   {
   public:
      dpsArchiveEventSwitchFile()
         : dpsArchiveEvent( DPS_ARCHIVE_EVENT_SWITCH_FILE )
      {
         preLogicalFileId = DPS_INVALID_LOG_FILE_ID ;
         preFileId = DPS_INVALID_LOG_FILE_ID ;
         curLogicalFileId = DPS_INVALID_LOG_FILE_ID ;
         curFileId = DPS_INVALID_LOG_FILE_ID ;
      }

      virtual ~dpsArchiveEventSwitchFile() {}

   public:
      UINT32 preLogicalFileId ;
      UINT32 preFileId ;
      UINT32 curLogicalFileId ;
      UINT32 curFileId ;
   } ;

   class dpsArchiveEventArchive: public dpsArchiveEvent
   {
   public:
      dpsArchiveEventArchive()
         : dpsArchiveEvent( DPS_ARCHIVE_EVENT_ARCHIVE )
      {
         logicalFileId = DPS_INVALID_LOG_FILE_ID ;
         isPartial = FALSE ;
      }

      virtual ~dpsArchiveEventArchive() {}

   public:
      UINT32   logicalFileId ;
      BOOLEAN  isPartial ;
      DPS_LSN  startLSN ;
      DPS_LSN  endLSN ; // not include
   } ;

   class dpsArchiveEventGenerateArchive: public dpsArchiveEvent
   {
   public:
      dpsArchiveEventGenerateArchive()
         : dpsArchiveEvent( DPS_ARCHIVE_EVENT_GENERATE_ARCHIVE )
      {
         allowPartial = FALSE ;
      }

      virtual ~dpsArchiveEventGenerateArchive() {}

   public:
      BOOLEAN allowPartial ;
   } ;

   dpsArchiveMgr::dpsArchiveMgr()
   {
      _dpsCB = NULL ;
      _logMgr = NULL ;
      ossGetCurrentTime( _lastActiveTime ) ;

      // set zero so that it will check if archive files expired immediately
      // when archive mgr start
      _lastExpiredTime.time = 0 ;
      _lastExpiredTime.microtm = 0 ;
      _archiveSize = 0 ;

      _isArchiving = FALSE ;
      _isDPSMoving = FALSE ;
      _inited = FALSE ;
   }

   dpsArchiveMgr::~dpsArchiveMgr()
   {
      _clearQueue() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR_INIT, "dpsArchiveMgr::init" )
   INT32 dpsArchiveMgr::init( _dpsLogWrapper* dpsCB, const CHAR* archivePath )
   {
      INT32 rc = SDB_OK ;
      dpsArchiveInfo info ;
      DPS_LSN startLSN ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR_INIT );
      SDB_ASSERT( dpsCB != NULL, "dpsCB must be not null" ) ;

      _dpsCB = dpsCB ;
      _logMgr = dpsCB->getLogMgr() ;
      _archivePath = string( archivePath ) ;
      if ( !utilStrEndsWith( _archivePath, OSS_FILE_SEP ) )
      {
         _archivePath += OSS_FILE_SEP ;
      }

      rc = _infoMgr.init( archivePath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init archive info mgr, rc=%d", rc ) ;
         goto error ;
      }

      info = _infoMgr.getInfo() ;
      startLSN = _calcStartLSN() ;
      if ( startLSN.compareOffset( info.startLSN.offset ) != 0 )
      {
         info.startLSN = startLSN ;
         rc = _infoMgr.updateInfo( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update archive info mgr, rc=%d", rc ) ;
            goto error ;
         }
      }

      _fileMgr.setArchivePath( _archivePath ) ;
      rc = _fileMgr.getTotalSize( _archiveSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get archive total size, rc=%d", rc ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Log archive total size is %lld", _archiveSize ) ;

      if ( startLSN.compareOffset( _logMgr->expectLsn().offset ) != 0 )
      {
         SDB_ASSERT( startLSN.compare( _logMgr->expectLsn() ) < 0,
                     "next LSN should less than dps expect LSN" ) ;
         rc = _generateArchiveEvent( startLSN, _logMgr->expectLsn() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to generate archive event, rc=%d", rc ) ;
            goto error ;
         }
      }

      _dpsCB->regEventHandler( this ) ;
      _inited= TRUE ;

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR_FINI, "dpsArchiveMgr::fini" )
   INT32 dpsArchiveMgr::fini()
   {
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR_FINI ) ;
      if ( _inited )
      {
         SDB_ASSERT( NULL != _dpsCB, "_dpsCB can't be NULL" ) ;
         _dpsCB->unregEventHandler( this ) ;
      }
      PD_TRACE_EXIT ( SDB_DPSARCHIVEMGR_FINI ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR_RUN, "dpsArchiveMgr::run" )
   INT32 dpsArchiveMgr::run()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR_RUN ) ;
      dpsArchiveEvent* event = NULL;
      DPS_LSN lsn ;

      lsn = _getMoveLSN() ;
      if ( !lsn.invalid() )
      {
         rc = _move( lsn ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else if ( _isDPSMoving )
      {
         ossSleepmillis( 10 ) ;
         goto done ;
      }
      else if ( _queue.timed_wait_and_pop( event, OSS_ONE_SEC  ) )
      {
         SDB_ASSERT( NULL != event, "event can't be NULL" ) ;

         rc = _processLogEvent( event ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         SDB_ASSERT( !_isArchiving, "is archiveing?" ) ;

         rc = _checkArchiveTimeout() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _checkArchiveExpired() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _checkArchiveQuota() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR_RUN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // ensure log file can't be wrapped
   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR_CANASSIGNLOGPAGE, "dpsArchiveMgr::canAssignLogPage" )
   INT32 dpsArchiveMgr::canAssignLogPage( UINT32 reqLen, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR_CANASSIGNLOGPAGE ) ;
      UINT32 waitTime = 0 ;
      DPS_LSN_OFFSET safeOffset ;
      DPS_LSN_OFFSET unsafeOffset ;
      UINT32 logFileSize = _logMgr->getLogFileSz() ;

      // log file can't be wrapped if diff <= safeOffset
      safeOffset = ( _logMgr->getLogFileNum() - 1 ) * logFileSize ;

      // log file may be warpped if diff > safeOffset and <= unsafeOffset
      unsafeOffset = _logMgr->getLogFileNum() * logFileSize ;

      while ( PMD_IS_DB_AVAILABLE() )
      {
         dpsArchiveInfo info = _infoMgr.getInfo() ;
         DPS_LSN_OFFSET expectOffset = _logMgr->expectLsn().offset ;
         DPS_LSN_OFFSET nextOffset =
            ( info.startLSN.offset == DPS_INVALID_LSN_OFFSET ) ?
            0 : info.startLSN.offset ;
         DPS_LSN_OFFSET endOffset = expectOffset + reqLen ;
         DPS_LSN_OFFSET diff = endOffset - nextOffset ;

         if ( DPS_INVALID_LSN_OFFSET == expectOffset )
         {
            goto done ;
         }

         // archived LSN is ahead of end LSN or euqals, it's safe
         if ( nextOffset >= endOffset )
         {
            goto done ;
         }

         // log file can't be wrapped
         if ( diff <= safeOffset )
         {
            goto done ;
         }

         // log file may be wrapped, let's check it
         if ( diff <= unsafeOffset )
         {
            // LSNs are not in the same physical log file, so it's safe
            if ( _logMgr->calcFileID( endOffset ) != 
                 _logMgr->calcFileID( nextOffset ) )
            {
               goto done ;
            }

            // LSNs are in the same logical log file, so it's safe
            if ( _logMgr->calcLogicalFileID( endOffset ) ==
                 _logMgr->calcLogicalFileID( nextOffset ) )
            {
               goto done ;
            }
         }

         // log file will be wrapped in the next log, so wait for archiving
         if ( waitTime < DPS_ARCHIVE_MAX_WAIT_TIME )
         {
            ossSleep( DPS_ARCHIVE_ONE_WAIT_TIME ) ;
            waitTime += DPS_ARCHIVE_ONE_WAIT_TIME ;
         }
         else
         {
            // timeout
            rc = SDB_DPS_LOG_NOT_ARCHIVED ;
            PD_LOG( PDWARNING, "Replica log is not archived, " \
                    "expect LSN: %lld[file: %u(%u)], reqLen=%u, " \
                    "next archived LSN: %lld[file: %u(%u)]",
                    expectOffset,
                    _logMgr->calcFileID( expectOffset ),
                    _logMgr->calcLogicalFileID( expectOffset ),
                    reqLen,
                    nextOffset,
                    _logMgr->calcFileID( nextOffset ),
                    _logMgr->calcLogicalFileID( nextOffset ) ) ;

            (void)_buildGenerateArchiveEvent( TRUE ) ;

            break ;
         }

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR_CANASSIGNLOGPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR_ONSWITCHLOGFILE, "dpsArchiveMgr::onSwitchLogFile" )
   void dpsArchiveMgr::onSwitchLogFile( UINT32 preLogicalFileId,
                                        UINT32 preFileId,
                                        UINT32 curLogicalFileId,
                                        UINT32 curFileId )
   {
      dpsArchiveEventSwitchFile* event = NULL ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR_ONSWITCHLOGFILE ) ;
      PD_LOG( PDDEBUG, "Log archive on switch log file" ) ;

      event = SDB_OSS_NEW dpsArchiveEventSwitchFile() ;
      if ( NULL == event )
      {
         PD_LOG( PDERROR, "Failed to create new dpsLogEventSwitchFile, rc=%d",
                 SDB_OOM ) ;
         goto error ;
      }

      event->preLogicalFileId = preLogicalFileId ;
      event->preFileId = preFileId ;
      event->curLogicalFileId = curLogicalFileId ;
      event->curFileId = curFileId ;

      _queue.push( event ) ;

   done:
      PD_TRACE_EXIT ( SDB_DPSARCHIVEMGR_ONSWITCHLOGFILE ) ;
      return ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR_ONMOVELOG, "dpsArchiveMgr::onMoveLog" )
   void dpsArchiveMgr::onMoveLog( DPS_LSN_OFFSET moveToOffset,
                                  DPS_LSN_VER moveToVersion,
                                  DPS_LSN_OFFSET expectOffset,
                                  DPS_LSN_VER expectVersion,
                                  DPS_MOMENT moment,
                                  INT32 errcode )
   {
      DPS_LSN lsn ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR_ONMOVELOG ) ;
      lsn.set( moveToOffset, moveToVersion ) ;

      switch( moment )
      {
      case DPS_BEFORE:
         PD_LOG( PDEVENT, "before move to %lld", moveToOffset ) ;
         _beforeMove() ;
         break ;
      case DPS_AFTER:
         _afterMove( lsn, errcode ) ;
         PD_LOG( PDEVENT, "after move to %lld", moveToOffset ) ;
         break ;
      default:
         SDB_ASSERT( FALSE, "invalid moment value" ) ;
      }

      PD_TRACE_EXIT ( SDB_DPSARCHIVEMGR_ONMOVELOG ) ;
   }

   DPS_LSN dpsArchiveMgr::_calcStartLSN()
   {
      DPS_LSN infoStartLSN ;
      DPS_LSN dpsStartLSN ;
      DPS_LSN dpsNextLSN ;

      infoStartLSN = _infoMgr.getInfo().startLSN ;

      // dpsCB->getStartLsn() will return invalid LSN before init finished,
      // so we directly get start LSN from logMgr.
      dpsStartLSN = _logMgr->getStartLsn( FALSE ) ;
      dpsNextLSN = _logMgr->expectLsn() ;

      // no replica log yet
      if ( dpsNextLSN.offset == 0 )
      {
         return dpsNextLSN ;
      }

      DPS_LSN nextLSN = minLSN( maxLSN( infoStartLSN, dpsStartLSN ), dpsNextLSN ) ;
      return nextLSN ;
   }

   INT32 dpsArchiveMgr::_buildGenerateArchiveEvent( BOOLEAN allowPartial )
   {
      INT32 rc = SDB_OK ;
      dpsArchiveEventGenerateArchive* event = NULL ;

      PD_LOG( PDDEBUG, "Log archive is building dpsArchiveEventGenerateArchive" ) ;

      event = SDB_OSS_NEW dpsArchiveEventGenerateArchive() ;
      if ( NULL == event )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to create new dpsArchiveEventGenerateArchive, rc=%d",
                 rc ) ;
         goto error ;
      }

      event->allowPartial = allowPartial ;

      _queue.push( event ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__GENERATEARCHIVEEVENT, "dpsArchiveMgr::_generateArchiveEvent" )
   INT32 dpsArchiveMgr::_generateArchiveEvent( const DPS_LSN& startLSN, 
                                               const DPS_LSN& endLSN,
                                               BOOLEAN allowPartial )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__GENERATEARCHIVEEVENT ) ;
      DPS_LSN_OFFSET startOffset ;
      DPS_LSN_OFFSET endOffset ;
      UINT32 startFileId ;
      UINT32 endFileId ;
      UINT32 workFileId ;

      if ( startLSN.compareOffset( endLSN.offset ) >= 0 )
      {
         PD_LOG( PDDEBUG, "startLSN[%llu] is larger than or equals to endLSN[%llu]",
                 startLSN.offset, endLSN.offset ) ;
         goto done ;
      }

      startOffset = startLSN.offset ;
      endOffset = endLSN.offset ;
      startFileId = _logMgr->calcLogicalFileID( startOffset ) ;
      endFileId = _logMgr->calcLogicalFileID( endOffset ) ;
      workFileId = _logMgr->getLoggerLogicalWork() ;

      // use the smaller work file logical ID if needed, since the logger may
      // not flush
      if ( workFileId < endFileId )
      {
         PD_LOG( PDDEBUG, "Log archive expected end file LID %u is smaller "
                 "than current work file LID %u, use the work file LID",
                 endFileId, workFileId ) ;
      }

      SDB_ASSERT( startFileId <= endFileId, "invalid file id" ) ;

      for ( UINT32 i = startFileId; i <= endFileId && i <= workFileId ; i++ )
      {
         dpsArchiveEventArchive* event = NULL ;

         if ( _isDPSMoving )
         {
            break ;
         }

         if ( i == endFileId )
         {
            if ( !allowPartial )
            {
               break ;
            }

            // endOffset is the first physical LSN
            if ( startOffset == endOffset )
            {
               break ;
            }
         }

         event = SDB_OSS_NEW dpsArchiveEventArchive() ;
         if ( NULL == event )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create new dpsLogEventArchive" ) ;
            goto error ;
         }

         event->logicalFileId = i ;
         event->startLSN.version = startLSN.version ;
         event->endLSN.version = startLSN.version ;

         if ( _logMgr->calcFirstPhysicalLSNOfFile( i ) == startOffset &&
              i != endFileId )
         {
            // the whole log file
            event->isPartial = FALSE ;
            event->startLSN.offset = startOffset ;
            // go to the next log file
            startOffset = _logMgr->calcFirstPhysicalLSNOfFile( i + 1 ) ;
            event->endLSN.offset = startOffset ;
         }
         else
         {
            event->isPartial = TRUE ;
            event->startLSN.offset = startOffset ;

            if ( i != endFileId )
            {
               SDB_ASSERT( i == startFileId, "i != startFileId" ) ;
               // the first log file
               // until the end of the log file, so we go to the next log file
               startOffset = _logMgr->calcFirstPhysicalLSNOfFile( i + 1 ) ;
               event->endLSN.offset = startOffset ;
            }
            else
            {
               // the last log file
               event->endLSN.offset = endOffset ;
            }
         }

         SDB_ASSERT( event->startLSN.compareOffset( event->endLSN ) != 0,
                     "startLSN == endLSN" ) ;

         _queue.push( event ) ;
         PD_LOG( PDDEBUG, "Log archive generate archive event, " \
                 "logicalFileId=%u, isPartial=%d, " \
                 "startLSN=%lld, endLSN=%lld",
                 event->logicalFileId, event->isPartial,
                 event->startLSN.offset, event->endLSN.offset ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__GENERATEARCHIVEEVENT, rc ) ;
      return rc ;
   error:
       goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__PROCESSLOGEVENT, "dpsArchiveMgr::_processLogEvent" )
   INT32 dpsArchiveMgr::_processLogEvent( dpsArchiveEvent* event )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__PROCESSLOGEVENT ) ;
      SDB_ASSERT( NULL != event, "event can't be NULL" ) ;
      SDB_ASSERT( !_isArchiving, "is archiveing?" ) ;

      switch( event->type() )
      {
      case DPS_ARCHIVE_EVENT_ARCHIVE:
         {
            dpsArchiveEventArchive* archiveEvent =
               dynamic_cast<dpsArchiveEventArchive*>( event ) ;
            PD_LOG( PDDEBUG, "Log archive received archive event, " \
                    "logicalFileId=%u, isPartial=%d, startLSN=%lld, nextLSN=%lld",
                    archiveEvent->logicalFileId, archiveEvent->isPartial,
                    archiveEvent->startLSN.offset, archiveEvent->endLSN.offset ) ;
            rc = _archive( archiveEvent->logicalFileId, archiveEvent->isPartial,
                           archiveEvent->startLSN, archiveEvent->endLSN ) ;
         }
         break ;
      case DPS_ARCHIVE_EVENT_SWITCH_FILE:
         {
            dpsArchiveEventSwitchFile* switchEvent = 
               dynamic_cast<dpsArchiveEventSwitchFile*>( event ) ;
            DPS_LSN expectLSN = _logMgr->tryExpectLsn();
            PD_LOG( PDDEBUG, "Log archive received switch file event, " \
                    "preFileId=%u, preLogicalFileId=%u, " \
                    "curFileId=%u, curLogicalFileId=%u",
                    switchEvent->preFileId, switchEvent->preLogicalFileId,
                    switchEvent->curFileId, switchEvent->curLogicalFileId ) ;
            if ( expectLSN.invalid() )
            {
               PD_LOG( PDINFO, "Failed to try get expect lsn" ) ;
            }
            else
            {
               rc = _generateArchiveEvent( _infoMgr.getInfo().startLSN,
                                           expectLSN ) ;
            }
         }
         break ;
      case DPS_ARCHIVE_EVENT_GENERATE_ARCHIVE:
         {
            dpsArchiveEventGenerateArchive* generateEvent = 
               dynamic_cast<dpsArchiveEventGenerateArchive*>( event ) ;
            DPS_LSN expectLSN = _logMgr->tryExpectLsn();
            PD_LOG( PDDEBUG, "Log archive received generate archive event, " \
                    "allowPartial=%d",
                    generateEvent->allowPartial ) ;
            if ( expectLSN.invalid() )
            {
               PD_LOG( PDINFO, "Failed to try get expect lsn" ) ;
            }
            else
            {
               rc = _generateArchiveEvent( _infoMgr.getInfo().startLSN,
                                           expectLSN,
                                           generateEvent->allowPartial ) ;
            }
         }
         break ;
      default:
         SDB_ASSERT( FALSE, "invalid log event" ) ;
         PD_LOG( PDERROR, "Log archive received invalid log event [%d]",
                 event->type() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      SAFE_OSS_DELETE( event ) ;
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__PROCESSLOGEVENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__ARCHIVE, "dpsArchiveMgr::_archive" )
   INT32 dpsArchiveMgr::_archive( UINT32 logicalFileId,
                                  BOOLEAN isPartial,
                                  const DPS_LSN& startLSN,
                                  const DPS_LSN& endLSN )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__ARCHIVE ) ;
      dpsArchiveInfo info = _infoMgr.getInfo() ;
      DPS_LSN_OFFSET startOffset = startLSN.offset ;
      // endOffset is not archived at this time
      DPS_LSN_OFFSET endOffset = endLSN.offset ;

      SDB_ASSERT( startOffset != endOffset, "startOffset == endOffset" ) ;
      SDB_ASSERT( _logMgr->calcLogicalFileID( startOffset ) == logicalFileId,
                  "invalid logicalFieldId or startLSN" ) ;

      SDB_ASSERT( !_isArchiving, "is archiving" ) ;
      _isArchiving = TRUE ;

#ifdef _DEBUG
      UINT32 endLogicalFileId = _logMgr->calcLogicalFileID( endOffset ) ;
      if ( isPartial )
      {
         if ( _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId ) ==
              startOffset )
         {
            // if startLSN is the first LSN of current file,
            // then endLSN can't be LSN of next file.
            SDB_ASSERT( endLogicalFileId == logicalFileId,
                        "startLSN and endLSN should be in the same log file" ) ;
            SDB_ASSERT(
               _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId + 1 ) >
               endOffset,
               "endLSN should be less than the first LSN of next file" ) ;
         }
         else if ( _logMgr->calcFirstPhysicalLSNOfFile( endLogicalFileId ) ==
                   endOffset )
         {
            // if endLSN is the first LSN of log file,
            // it should be in the next file,
            // and startLSN can't be the first LSN of current file.
            SDB_ASSERT( endLogicalFileId == logicalFileId + 1,
                        "endLSN should be in the next log file" ) ;
            SDB_ASSERT(
               _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId ) <
               startOffset,
               "startLSN must be greater than the first LSN of current file" ) ;
         }
         else
         {
            // startLSN and endLSN are in current log file
            SDB_ASSERT( endLogicalFileId == logicalFileId,
                     "startLSN and endLSN are in current log file" ) ;
            SDB_ASSERT(
               _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId ) <
               startOffset,
               "startLSN must be greater than the first LSN of current file" ) ;
            SDB_ASSERT(
               _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId + 1 ) >
               endOffset,
               "endLSN should be less than the first LSN of next file" ) ;
         }
      }
      else
      {
         SDB_ASSERT(
            _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId ) == startOffset,
            "startLSN should be the first LSN of current file" ) ;
         SDB_ASSERT( endLogicalFileId == logicalFileId + 1,
                     "endLSN should be the first LSN of next file" ) ;
         SDB_ASSERT(
            _logMgr->calcFirstPhysicalLSNOfFile( endLogicalFileId ) == endOffset,
            "endLSN should be the first LSN of next file" ) ;
      }
#endif

      if ( _isDPSMoving )
      {
         PD_LOG( PDEVENT, "dps is moving, stop archiving" ) ;
         goto done ;
      }

      if ( startOffset != info.startLSN.offset )
      {
         PD_LOG( PDWARNING, "Log archive is not continuous" ) ;
      }

      if ( endOffset <= info.startLSN.offset )
      {
         PD_LOG( PDWARNING, "Log is archived, startLSN=%lld, endLSN=%lld, " \
                 "next archived LSN=%lld",
                 startOffset, endOffset, info.startLSN.offset ) ;
         goto done ;
      }

      rc = _fileMgr.deleteTmpFile() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete tmp file, rc=%d", rc ) ;
         goto error ;
      }

      if ( !isPartial )
      {
         rc = _archiveFull( _logMgr->calcFileID( startOffset ),
                            logicalFileId ) ;
      }
      else
      {
         rc = _archivePartial( logicalFileId, startOffset, endOffset ) ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

#ifdef _DEBUG
      {
         INT64 totalSize = 0 ;
         rc = _fileMgr.getTotalSize( totalSize ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "After archive, archiveSize=%lld, totalSize=%lld",
                    _archiveSize, totalSize ) ;
         }
      }
#endif

      ossGetCurrentTime( _lastActiveTime ) ; // update time

      info.startLSN = endLSN ;
      rc = _infoMgr.updateInfo( info ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to update archive info, rc=%d", rc ) ;
         rc = SDB_OK ;
      }

      PD_LOG( PDEVENT, "Archive done, next archived LSN: %lld", endLSN.offset ) ;

   done:
      _isArchiving = FALSE ;
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__ARCHIVE, rc ) ;
      return rc ;
   error:
      // current archive failed,
      // so we should re-archive current startLSN,
      // but the queue may have archive event after startLSN,
      // so here clear the queue,
      // the archive event of current startLSN will be re-created
      // by next switch event or timeout.
      _clearQueue() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__ARCHIVEFULL, "dpsArchiveMgr::_archiveFull" )
   INT32 dpsArchiveMgr::_archiveFull( UINT32 fileId, UINT32 logicalFileId )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__ARCHIVEFULL ) ;
      dpsArchiveFile archiveFile ;
      _dpsLogFile* logFile = NULL ;
      string path = _fileMgr.getFullFilePath( logicalFileId ) ;
      BOOLEAN compress = pmdGetKRCB()->getOptionCB()->archiveCompressOn() ;

#ifdef _DEBUG
      {
         BOOLEAN exist = FALSE ;
         rc = _fileMgr.fullFileExists( logicalFileId, exist ) ;
         SDB_ASSERT( SDB_OK == rc, "failed to access file" ) ;
         SDB_ASSERT( !exist, "full file exists" ) ;
      }
#endif

      logFile = _logMgr->getLogFile( fileId ) ;
      if ( NULL == logFile )
      {
         SDB_ASSERT( FALSE, "logFile can't be NULL" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get replica log file[%u], rc=%d",
                 fileId, rc ) ;
         goto error ;
      }

      if ( logFile->header()._logID != logicalFileId )
      {
         SDB_ASSERT( FALSE, "logicalFileId must be the same" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid logical file id" \
                 "(expect=%u, real=%u), FileId=%u, rc=%d",
                 logicalFileId, logFile->header()._logID, fileId, rc ) ;
         goto error ;
      }

      rc = _fileMgr.copyArchiveFile( logFile->path(),
              _fileMgr.getTmpFilePath(),
              compress ? DPS_ARCHIVE_COPY_COMPRESS : DPS_ARCHIVE_COPY_PLAIN,
              this ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to copy replica log file[%s(%lld)], rc=%d",
                 logFile->path().c_str(), logicalFileId, rc ) ;
         goto error ;
      }

      rc = archiveFile.init( _fileMgr.getTmpFilePath(), FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init archive file[%lld], rc=%d",
                 logicalFileId, rc ) ;
         goto error ;
      }

      {
         _dpsLogHeader& srcLogHeader = logFile->header() ;
         _dpsLogHeader* destLogHeader = archiveFile.getLogHeader() ;
         *destLogHeader = srcLogHeader ;

         dpsArchiveHeader* archiveHeader = archiveFile.getArchiveHeader() ;
         archiveHeader->init();
         archiveHeader->startLSN = srcLogHeader._firstLSN ;
         archiveHeader->endLSN.offset =
            _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId + 1 ) ;
         if ( compress )
         {
            archiveHeader->setFlag( DPS_ARCHIVE_COMPRESSED ) ;
         }

         rc = archiveFile.flushHeader() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to flush archive file header[%lld], rc=%d",
                    logicalFileId, rc ) ;
            goto error ;
         }

         archiveFile.close() ;
      }

      rc = ossRenamePath( _fileMgr.getTmpFilePath().c_str(), path.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to rename tmp archive to file[%s], rc: %d",
                 path.c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Full replica log file[%u(%u)] is archived",
              fileId, logicalFileId ) ;

      {
         INT64 fileSize = 0 ;
         rc = ossFile::getFileSize( path, fileSize ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDDEBUG, "Before archive size=%lld", _archiveSize ) ;
            _archiveSize += fileSize ;
            PD_LOG( PDDEBUG, "After archive size=%lld, fileSize=%lld",
                    _archiveSize, fileSize ) ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__ARCHIVEFULL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__ARCHIVEPARTIAL, "dpsArchiveMgr::_archivePartial" )
   INT32 dpsArchiveMgr::_archivePartial( UINT32 logicalFileId, 
                                         DPS_LSN_OFFSET startOffset,
                                         DPS_LSN_OFFSET endOffset )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__ARCHIVEPARTIAL ) ;
      dpsArchiveFile archiveFile ;
      _dpsLogFile* logFile = NULL ;
      string path = _fileMgr.getFullFilePath( logicalFileId ) ;
      string tmpPath = _fileMgr.getTmpFilePath() ;
      string partialPath = _fileMgr.getPartialFilePath( logicalFileId ) ;
      UINT32 fileId = _logMgr->calcFileID( startOffset ) ;
      BOOLEAN exist = FALSE ;

#ifdef _DEBUG
      {
         UINT32 startLogicalFileId = _logMgr->calcLogicalFileID( startOffset ) ;
         UINT32 endLogicalFileId = _logMgr->calcLogicalFileID( endOffset ) ;
         SDB_ASSERT( startLogicalFileId == endLogicalFileId ||
                     startLogicalFileId + 1 == endLogicalFileId,
                     "invalid partial archive" ) ;
         SDB_ASSERT( logicalFileId == startLogicalFileId,
                     "invalid partial archive" ) ;
      }
#endif

      logFile = _logMgr->getLogFile( fileId ) ;
      if ( NULL == logFile )
      {
         SDB_ASSERT( FALSE, "logFile can't be NULL" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get replica log file[%u], rc=%d",
                 fileId, rc ) ;
         goto error ;
      }

      if ( logFile->header()._logID != logicalFileId &&
           logFile->header()._logID != DPS_INVALID_LOG_FILE_ID )
      {
         SDB_ASSERT( FALSE, "logicalFileId must be the same" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid logical file id" \
                 "(expect=%u, real=%u), FileId=%u, rc=%d",
                 logicalFileId, logFile->header()._logID, fileId, rc ) ;
         goto error ;
      }

      rc = ossFile::exists( partialPath, exist ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if archive file[%s] exists, rc=%d",
                 partialPath.c_str(), rc ) ;
         goto error ;
      }

      // if partial file does not exist,
      // create a temp file firstly,
      // then rename it to partial file after archiving,
      // so replay tool can't read a partial file with un-inited file header
      if ( !exist )
      {
         rc = _fileMgr.deleteTmpFile() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete tmp file, rc=%d", rc ) ;
            goto error ;
         }

         rc = archiveFile.init( tmpPath, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init archive file[%s], rc=%d",
                    tmpPath.c_str(), rc ) ;
            goto error ;
         }

         rc = archiveFile.extend( _logMgr->getLogFileSz() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend archive file[%s], rc=%d",
                    tmpPath.c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = archiveFile.init( partialPath, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init archive file[%s], rc=%d",
                    partialPath.c_str(), rc ) ;
            goto error ;
         }
      }

      {
         DPS_LSN lsn ;
         _dpsMessageBlock block( DPS_MSG_BLOCK_DEF_LEN ) ;
         UINT32 len = 0 ;
         INT64 offset = 0 ;
         DPS_LSN_OFFSET firstOffset =
            _logMgr->calcFirstPhysicalLSNOfFile( logicalFileId ) ;
         lsn.offset = startOffset ;

         while ( lsn.offset < endOffset )
         {
            if ( _isDPSMoving )
            {
               rc = SDB_INTERRUPT ;
               PD_LOG( PDWARNING,
                       "Partial archive is interrupted by DPS move" ) ;
               goto error ;
            }

            rc = _logMgr->search( lsn, &block, DPS_SEARCH_ALL, FALSE, &len ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to find LSN[%lld], rc=%d",
                       lsn.offset, rc ) ;
               if ( _isDPSMoving )
               {
                  PD_LOG( PDERROR,
                          "Failed to find LSN perhaps because DPS move" ) ;
               }
               goto error ;
            }

            SDB_ASSERT( block.length() == len, "invalid LSN length" ) ;
#ifdef _DEBUG
            {
               _dpsLogRecordHeader* record =
                  (_dpsLogRecordHeader*)block.startPtr() ;
               SDB_ASSERT( lsn.offset == record->_lsn, "corrupt LSN" ) ;
               SDB_ASSERT( len == record->_length, "invalid LSN length" ) ;
            }
#endif

            offset = (INT64)( lsn.offset - firstOffset ) + DPS_LOG_HEAD_LEN ;

            rc = archiveFile.write( offset, block.startPtr(), len ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to write to archive file[%s], rc: %d",
                       partialPath.c_str(), rc ) ;
               goto error ;
            }

            block.clear() ;
            lsn.offset += len ;
         }

         SDB_ASSERT( lsn.offset == endOffset, "corrupt LSN" ) ;
      }

      {
         _dpsLogHeader& srcLogHeader = logFile->header() ;
         _dpsLogHeader* destLogHeader = archiveFile.getLogHeader() ;

         // a new partial archive file, need to init file header
         if ( DPS_INVALID_LOG_FILE_ID == destLogHeader->_logID )
         {
            *destLogHeader = srcLogHeader ;
            destLogHeader->_firstLSN.offset = startOffset ;
            if ( DPS_INVALID_LSN_VERSION == destLogHeader->_firstLSN.version )
            {
               destLogHeader->_firstLSN.version = 1 ;
            }
            destLogHeader->_logID = logicalFileId ;
            destLogHeader->_fileNum = _logMgr->getLogFileNum() ;
            destLogHeader->_fileSize = _logMgr->getLogFileSz() ;
            archiveFile.getArchiveHeader()->init();
         }

         dpsArchiveHeader* archiveHeader = archiveFile.getArchiveHeader() ;
         if ( DPS_INVALID_LSN_OFFSET == archiveHeader->startLSN.offset )
         {
            archiveHeader->setFlag( DPS_ARCHIVE_PARTIAL ) ;
            archiveHeader->startLSN.set( startOffset, 1 ) ;
         }
         archiveHeader->endLSN.set( endOffset, 1 ) ;

         rc = archiveFile.flushHeader() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to flush archive file header[%s], rc=%d",
                    partialPath.c_str(), rc ) ;
            goto error ;
         }

         archiveFile.close() ;
      }

      // after archiving, rename tmp file to partial file
      if ( !exist )
      {
         rc = ossFile::rename( tmpPath, partialPath ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to rename file from [%s] to [%s], rc=%d",
                    tmpPath.c_str(), partialPath.c_str(), rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Before extend, archive size=%lld", _archiveSize ) ;
         _archiveSize += _logMgr->getLogFileSz() + DPS_LOG_HEAD_LEN ;
         PD_LOG( PDDEBUG, "After extend, archive size=%lld", _archiveSize ) ;
      }

      PD_LOG( PDEVENT, "Partial replica log file[%u(%u):%lld-%lld] is archived",
              fileId, logicalFileId, startOffset, endOffset ) ;

      // partial log file is full
      if ( _logMgr->calcLogicalFileID( endOffset ) == logicalFileId + 1 )
      {
         BOOLEAN compress = pmdGetKRCB()->getOptionCB()->archiveCompressOn() ;
         string srcPath = partialPath ;

         if ( compress )
         {
            rc = _fileMgr.copyArchiveFile( partialPath, tmpPath,
                                           DPS_ARCHIVE_COPY_COMPRESS, this ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to copy archive file[%s], rc=%d",
                       partialPath.c_str(), rc ) ;
               goto error ;
            }

            srcPath = tmpPath ;

            {
               INT64 fileSize = 0 ;
               rc = ossFile::getFileSize( tmpPath, fileSize ) ;
               if ( SDB_OK == rc )
               {
                  PD_LOG( PDDEBUG, "Before compress, archive size=%lld",
                          _archiveSize ) ;
                  _archiveSize += fileSize ;
                  PD_LOG( PDDEBUG, "After compress, archive size=%lld, " \
                          "fileSize=%lld",
                          _archiveSize, fileSize ) ;
               }
            }
         }

         rc = archiveFile.init( srcPath, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init archive file[%s], rc=%d",
                    srcPath.c_str(), rc ) ;
            goto error ;
         }

         {
            dpsArchiveHeader* archiveHeader = archiveFile.getArchiveHeader() ;
            archiveHeader->unsetFlag( DPS_ARCHIVE_PARTIAL ) ;
            if (compress)
            {
               archiveHeader->setFlag( DPS_ARCHIVE_COMPRESSED ) ;
            }
         }

         rc = archiveFile.flushHeader() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to flush archive file header[%s], rc=%d",
                    srcPath.c_str(), rc ) ;
            goto error ;
         }

         archiveFile.close() ;

         rc = ossFile::rename( srcPath, path ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to rename file from [%s] to [%s], rc: %d",
                    srcPath.c_str(), path.c_str(), rc ) ;
            goto error ;
         }

         if ( compress )
         {
            INT64 fileSize = 0 ;
            (void)ossFile::getFileSize( partialPath, fileSize ) ;

            rc = ossFile::deleteFile( partialPath ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to delete partial file[%s], rc=%d",
                       partialPath.c_str(), rc ) ;
               goto error ;
            }

            PD_LOG( PDDEBUG, "Before delete file, archive size=%lld",
                    _archiveSize ) ;
            _archiveSize -= fileSize ;
            PD_LOG( PDDEBUG, "After delete file, archive size=%lld, fileSize=%lld",
                    _archiveSize, fileSize ) ;
         }

         PD_LOG( PDEVENT, "Partial archive file[%lld] is full",
                 logicalFileId ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__ARCHIVEPARTIAL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__MOVE, "dpsArchiveMgr::_move" )
   INT32 dpsArchiveMgr::_move( const DPS_LSN& lsn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__MOVE ) ;
      dpsArchiveInfo info = _infoMgr.getInfo() ;
      DPS_LSN startLSN = info.startLSN ;
      DPS_LSN_OFFSET moveOffset = lsn.offset ;
      INT32 result = 0 ;

      SDB_ASSERT( !lsn.invalid(), "invalid LSN" ) ;
      SDB_ASSERT( !_isArchiving, "is archiveing?" ) ;

      result = startLSN.compareOffset( moveOffset ) ;
      UINT32 moveFileId = _logMgr->calcLogicalFileID( moveOffset ) ;
      UINT32 curFileId = _logMgr->calcLogicalFileID( startLSN.offset ) ;

      if ( result > 0 ) // backward
      {
         SDB_ASSERT( moveFileId <= curFileId, "invalid move file id" ) ;
         PD_LOG( PDEVENT, "Move backward to LSN %lld, next archived LSN is %lld",
                 moveOffset, startLSN.offset ) ;

         for ( UINT32 fileId = curFileId ;
               fileId >= moveFileId && DPS_INVALID_LOG_FILE_ID != fileId ;
               fileId-- )
         {
            rc = _fileMgr.moveArchiveFile( fileId ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to move archive file[%u], rc=%d",
                       fileId, rc ) ;
               goto error ;
            }
         }

         // we have moved the whole file,
         // but if the moveLSN is not the first LSN of file,
         // we should archive logs before the moveLSN in the file again.
         if ( !_logMgr->isFirstPhysicalLSNOfFile( moveOffset ) )
         {
            moveOffset = _logMgr->calcFirstPhysicalLSNOfFile( moveFileId ) ;
         }
      }
      else if ( result < 0 ) // forward
      {
         PD_LOG( PDEVENT, "Move forward to LSN %lld, next archived LSN is %lld",
                 moveOffset, startLSN.offset ) ;

         if ( moveFileId == curFileId )
         {
            // if move in the same log file,
            // there will be a gap between moved LSN and current start LSN
            rc = _fileMgr.moveArchiveFile( curFileId, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to move archive file[%u], rc=%d",
                       curFileId, rc ) ;
               goto error ;
            }

            // we have moved the whole file,
            // but if the moveLSN is not the first LSN of file,
            // we should archive logs before the moveLSN in the file again.
            if ( !_logMgr->isFirstPhysicalLSNOfFile( moveOffset ) )
            {
               moveOffset = _logMgr->calcFirstPhysicalLSNOfFile( moveFileId ) ;
            }
         }
      }

      if ( info.startLSN.compareOffset( moveOffset ) != 0 )
      {
         info.startLSN.set( moveOffset, lsn.version );
         rc = _infoMgr.updateInfo( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update archive info, rc=%d", rc ) ;
            goto error ;
         }
      }

      _popMoveLSN( lsn ) ;
      _clearQueue() ;

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__MOVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__CHECKARCHIVETIMEOUT, "dpsArchiveMgr::_checkArchiveTimeout" )
   INT32 dpsArchiveMgr::_checkArchiveTimeout()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__CHECKARCHIVETIMEOUT ) ;
      ossTimestamp curTime ;
      UINT32 timeout ;
      DPS_LSN expectLSN ;

      timeout = pmdGetKRCB()->getOptionCB()->getArchiveTimeout() ;
      if ( 0 == timeout )
      {
         // zero means disable timeout check
         goto done ;
      }

      ossGetCurrentTime( curTime ) ;
      if ( curTime.time < _lastActiveTime.time )
      {
         // time is go back, so we set current time to it
         _lastActiveTime = curTime ;
      }
      if ( curTime.time - _lastActiveTime.time < timeout )
      {
         goto done ;
      }

      ossGetCurrentTime( _lastActiveTime ) ;
      PD_LOG( PDDEBUG, "Log archive timeout(%u seconds)",
              pmdGetKRCB()->getOptionCB()->getArchiveTimeout() ) ;

      expectLSN = _logMgr->tryExpectLsn() ;
      if ( expectLSN.invalid() )
      {
         PD_LOG( PDINFO, "Failed to try get expect lsn" ) ;
         goto done;
      }

      rc = _generateArchiveEvent( _infoMgr.getInfo().startLSN,
                                  expectLSN,
                                  TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to generate timeout archive event, rc=%d",
                 rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__CHECKARCHIVETIMEOUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__CHECKARCHIVEEXPIRED, "dpsArchiveMgr::_checkArchiveExpired" )
   INT32 dpsArchiveMgr::_checkArchiveExpired()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__CHECKARCHIVEEXPIRED ) ;
      ossTimestamp curTime ;
      UINT32 expired ;
      UINT32 minFileId = DPS_INVALID_LOG_FILE_ID ;
      UINT32 maxFileId = DPS_INVALID_LOG_FILE_ID ;

      expired = pmdGetKRCB()->getOptionCB()->getArchiveExpired() ;
      if ( 0 == expired )
      {
         // zero means disable expiration check
         goto done ;
      }
      // exipred unit is HOURS, here convert to SECONDS
      expired *= DPS_ARCHIVE_EXPIRED_SECONDS ;

      ossGetCurrentTime( curTime ) ;
      if ( curTime.time < _lastExpiredTime.time )
      {
         // time is go back, so we set current time to it
         _lastExpiredTime = curTime ;
      }
      if ( curTime.time - _lastExpiredTime.time < ( expired / 2 ) )
      {
         goto done ;
      }

      rc = _fileMgr.scanArchiveFiles( minFileId, maxFileId, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to scan archive file, rc=%d", rc ) ;
         goto error ;
      }

      if ( DPS_INVALID_LOG_FILE_ID == minFileId )
      {
         // no archive file
         goto done ;
      }

      SDB_ASSERT( DPS_INVALID_LOG_FILE_ID != maxFileId, "invalid max file id" ) ;

      PD_LOG( PDDEBUG, "Log archive min file id[%u], max file id[%u]",
              minFileId, maxFileId ) ;

      rc = _fileMgr.deleteFilesByTime( minFileId, maxFileId,
                                       curTime.time - expired ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete expired archive file, rc=%d", rc ) ;
         goto error ;
      }

      ossGetCurrentTime( _lastExpiredTime ) ;

      // stats archive total size again
      rc = _fileMgr.getTotalSize( _archiveSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get archive total size, rc=%d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__CHECKARCHIVEEXPIRED, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DPSARCHIVEMGR__CHECKARCHIVEQUOTA, "dpsArchiveMgr::_checkArchiveQuota" )
   INT32 dpsArchiveMgr::_checkArchiveQuota()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DPSARCHIVEMGR__CHECKARCHIVEQUOTA ) ;
      UINT32 minFileId = DPS_INVALID_LOG_FILE_ID ;
      UINT32 maxFileId = DPS_INVALID_LOG_FILE_ID ;
      UINT64 logFileSize = _logMgr->getLogFileSz() ;
      UINT64 quota ;

      quota = pmdGetKRCB()->getOptionCB()->getArchiveQuota() ;
      if ( 0 == quota )
      {
         // zero means disable quota check
         goto done ;
      }
      quota *= DPS_ARCHIVE_QUOTA_BYTES ;

      // at least 3 log file size for quota
      if ( quota < logFileSize * 3 )
      {
         quota = logFileSize * 3 ;
      }
      // keep one log file size for safty
      quota -= logFileSize ;

      if ( _archiveSize <= (INT64)quota )
      {
         goto done ;
      }

      rc = _fileMgr.scanArchiveFiles( minFileId, maxFileId, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to scan archive file, rc=%d", rc ) ;
         goto error ;
      }

      if ( DPS_INVALID_LOG_FILE_ID == minFileId )
      {
         PD_LOG( PDERROR, "Archive size = %lld, but no archive file found",
                 _archiveSize );
         rc = _fileMgr.getTotalSize( _archiveSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get archive total size, rc=%d", rc ) ;
            goto error ;
         }
         goto done ;
      }

      PD_LOG( PDEVENT, "Start to delete archive file, " \
              "quota=%lld, archive size=%lld",
              quota, _archiveSize ) ;

      rc = _fileMgr.deleteFilesBySize( minFileId, maxFileId,
                                       _archiveSize - (INT64)quota ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete archive file by quota, rc=%d", rc ) ;
         goto error ;
      }

      rc = _fileMgr.getTotalSize( _archiveSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get archive total size, rc=%d", rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Finish deleting archive file, archive size=%lld",
              _archiveSize ) ;

   done:
      PD_TRACE_EXITRC ( SDB_DPSARCHIVEMGR__CHECKARCHIVEQUOTA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void dpsArchiveMgr::_beforeMove()
   {
      // before move we just stop archiving
      // to avoid concurrency between DPS move and archive

      SDB_ASSERT( !_isDPSMoving, "is moving" ) ;

      // move operation is mutually exclusive in DPS,
      // so only one thread can modify _isDPSMoving,
      // set true until dps move is finished,
      // to stop archiving during move
      _isDPSMoving = TRUE ;
   }

   void dpsArchiveMgr::_afterMove( const DPS_LSN& lsn, INT32 errcode )
   {
      SDB_ASSERT( _isDPSMoving, "is not moving" ) ;

      if ( SDB_OK != errcode )
      {
         goto done ;
      }

      _setMoveLSN( lsn ) ;

   done:
      _isDPSMoving = FALSE ;
      return ;
   }

   void dpsArchiveMgr::_clearQueue()
   {
      dpsArchiveEvent* event = NULL;
      while( _queue.try_pop( event ) )
      {
         SAFE_OSS_DELETE( event ) ;
      }
   }

   DPS_LSN dpsArchiveMgr::_getMoveLSN()
   {
      DPS_LSN lsn ;
      _mutex.get() ;
      if ( _moveLSN.size() > 0 )
      {
         lsn = _moveLSN.front() ;
      }
      _mutex.release() ;
      return lsn ;
   }

   void dpsArchiveMgr::_setMoveLSN( const DPS_LSN& lsn )
   {
      _mutex.get() ;
      _moveLSN.push( lsn ) ;
      _mutex.release() ;
   }

   void dpsArchiveMgr::_popMoveLSN( const DPS_LSN& lsn )
   {
      DPS_LSN front ;
      SDB_ASSERT( !lsn.invalid(), "invalid lsn" ) ;
      _mutex.get() ;
      if ( _moveLSN.size() > 0 )
      {
         front = _moveLSN.front() ;
         SDB_ASSERT( front.compare( lsn ) == 0, "lsn is not the front LSN" ) ;
         if ( front.compare( lsn ) == 0 )
         {
            _moveLSN.pop() ;
         }
      }
      _mutex.release() ;
   }

   BOOLEAN dpsArchiveMgr::isInterrupted()
   {
      return _isDPSMoving ;
   }
}

