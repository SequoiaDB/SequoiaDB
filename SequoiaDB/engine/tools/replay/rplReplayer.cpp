/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rplReplayer.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplReplayer.hpp"
#include "pd.hpp"
#include "dpsArchiveFile.hpp"
#include "dpsOp2Record.hpp"
#include "oss.h"
#include "ossEDU.hpp"
#include "../bson/bsonobj.h"
#include "utilJsonFile.hpp"
#include "ixm.hpp"
#include "dms.hpp"
#include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;

using namespace engine;
using namespace bson;
using namespace sdbclient;
using namespace std;

namespace replay
{
   #define RPL_DUMP_FIELD_WIDTH 8

   #define RPL_WATCH_INTERVAL (10 * 1000) // seconds

   #define RPL_STATUS_INTERVAL (1000)

   #define RPL_STATUS_NEXT_LSN               "nextLSN"
   #define RPL_STATUS_NEXT_FILEID            "nextFileId"
   #define RPL_STATUS_LAST_LSN               "lastLSN"
   #define RPL_STATUS_LAST_FILEID            "lastFileId"
   #define RPL_STATUS_LAST_MOVED_FILETIME    "lastMovedFileTime"

   #define RPL_DEFLATE_SUFFIX                ".deflate"
   #define RPL_INFLATE_SUFFIX                ".inflate"

   static volatile BOOLEAN _isRunning = FALSE;

   static void _stop(INT32 sigNum)
   {
      if (0 != sigNum)
      {
         PD_LOG(PDEVENT, "Recieved signal[%d], stop...", sigNum);
      }

      _isRunning = FALSE;
   }

   static INT32 _regSignalHandler()
   {
      INT32 rc = SDB_OK;
#if defined (_LINUX)
      ossSigSet sigSet ;
      sigSet.sigAdd(SIGHUP);
      sigSet.sigAdd(SIGINT);
      sigSet.sigAdd(SIGTERM);
      sigSet.sigAdd(SIGPWR);
      rc = ossRegisterSignalHandle(sigSet, (SIG_HANDLE)(&_stop));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to register signals, rc=%d",
                rc);
         goto error;
      }

   done:
      return rc ;
   error:
      goto done ;
#else
      return rc ;
#endif
   }

   Replayer::Replayer()
   {
      _options = NULL;
      _sdb = NULL;
      _buf = NULL;
      _bufSize = 0;
   }

   Replayer::~Replayer()
   {
      if (NULL != _sdb)
      {
         _sdb->disconnect();
         SAFE_OSS_DELETE(_sdb);
      }

      if (!_tmpFile.empty())
      {
         ossFile::deleteFile(_tmpFile);
         _tmpFile = "";
      }

      if (_bufSize > 0)
      {
         SAFE_OSS_FREE(_buf);
         _bufSize = 0;
      }
   }

   INT32 Replayer::init(Options& options)
   {
      INT32 rc = SDB_OK;

      _options = &options;

      rc = _filter.init(_options->filter());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init filter, rc=%d", rc);
         goto error;
      }

      _path = _options->path();
      SDB_ASSERT(SDB_OSS_FIL == _options->pathType() ||
                 SDB_OSS_DIR == _options->pathType(),
                 "path can only be file or directory");

      if (!_options->status().empty())
      {
         rc = _initStatus();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to init status, rc=%d", rc);
            goto error;
         }
      }

      rc = _connectSdb();
      if (SDB_OK != rc)
      {
         goto error;
      }

      {
         stringstream ss;
         ss << "sdbreplay.tmp."
            << ossGetCurrentProcessID();
         _tmpFile = ss.str();

         ossFile file;
         rc = file.open(_tmpFile,
                        OSS_CREATEONLY | OSS_READWRITE,
                        OSS_DEFAULTFILE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to open temporary file[%s], rc=%d",
                   _tmpFile.c_str(), rc);
            goto error;
         }
      }

      rc = _regSignalHandler();
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::run()
   {
      INT32 rc = SDB_OK;

      _isRunning = TRUE;

      if (SDB_OSS_FIL == _options->pathType())
      {
         rc = _replayFile();
      }
      else if (SDB_OSS_DIR == _options->pathType())
      {
         rc = _replayDir();
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid path type");
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid path type: %d", _options->pathType());
         goto error;
      }

   done:
      if (_status.isOpened())
      {
         rc = _writeStatus();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to write status, rc=%d", rc);
         }
      }
      if (!_options->inflate() && !_options->deflate())
      {
         PD_LOG(PDINFO, "Replay result:\n%s", _monitor.dump().c_str());
      }
      _isRunning = FALSE;
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayFile()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(SDB_OSS_FIL == _options->pathType(), "path is not file");

      if (_options->deflate())
      {
         rc = _deflateFile(_path);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to deflate file[%s], rc=%d",
                   _path.c_str(), rc);
            goto error;
         }
      }
      else if (_options->inflate())
      {
         rc = _inflateFile(_path);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to inflate file[%s], rc=%d",
                   _path.c_str(), rc);
            goto error;
         }
      }
      else
      {
         BOOLEAN isArchive = FALSE;
         UINT32 fileSize = 0;
         UINT32 fileNum = 0;
         UINT32 fileId = DPS_INVALID_LOG_FILE_ID;

         rc = _isDpsLogFile(_path, isArchive, fileSize, fileNum, fileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to check if file[%s] is log file, rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         if (!isArchive && !_options->isReplicaFile())
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "File[%s] is not archive file, rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         if (_options->isReplicaFile())
         {
            rc = _replayReplicaFile(_path, fileSize, fileNum);
         }
         else
         {
            rc = _replayArchiveFile(_path);
         }

         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to replay file[%s], rc=%d",
                   _path.c_str(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayDir()
   {
      INT32 rc = SDB_OK;

      if (_options->isReplicaFile())
      {
         rc = _replayReplicaDir();
      }
      else
      {
         _archiveFileMgr.setArchivePath(_path);

         rc = _replayArchiveDir();
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to replay log files in directory[%s], rc=%d",
                _path.c_str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_isDpsLogFile(const string& path, BOOLEAN& isArchive,
                                      UINT32& fileSize, UINT32& fileNum, UINT32& fileId)
   {
      INT32 rc = SDB_OK;
      BOOLEAN exist = FALSE;
      ossFile file;
      _dpsLogHeader* logHeader = NULL;
      dpsArchiveHeader* archiveHeader = NULL;
      INT64 readSize = 0;

      isArchive = FALSE;

      rc = ossFile::exists( path, exist );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check existence of file[%s], rc=%d",
                 path.c_str(), rc );
         goto error;
      }

      rc = file.open( path, OSS_READONLY, OSS_DEFAULTFILE );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc=%d",
                 path.c_str(), rc );
         goto error;
      }

      logHeader = SDB_OSS_NEW _dpsLogHeader();
      if ( NULL == logHeader )
      {
         rc = SDB_OOM;
         PD_LOG ( PDERROR, "Failed to create new _dpsLogHeader!" );
         goto error;
      }

      archiveHeader = ( dpsArchiveHeader* )
                      ( (CHAR*)logHeader + DPS_ARCHIVE_HEADER_OFFSET );

      rc = file.readN( (CHAR*)logHeader, DPS_LOG_HEAD_LEN, readSize );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read file header, rc=%d", rc ) ;
         goto error ;
      }

      if (readSize != DPS_LOG_HEAD_LEN)
      {
         rc = SDB_DPS_FILE_NOT_RECOGNISE ;
         PD_LOG( PDWARNING, "DPS file header length error, rc=%d", rc ) ;
         goto error ;
      }

      if (0 != ossStrncmp(logHeader->_eyeCatcher,
                          DPS_LOG_HEADER_EYECATCHER,
                          DPS_LOG_HEADER_EYECATCHER_LEN))
      {
         rc = SDB_DPS_FILE_NOT_RECOGNISE ;
         PD_LOG( PDWARNING, "DPS file eye catcher error, rc=%d", rc ) ;
         goto error ;
      }

      fileSize = logHeader->_fileSize;
      fileNum = logHeader->_fileNum;
      fileId = logHeader->_logID;

      if (0 == ossStrncmp(archiveHeader->eyeCatcher,
                          DPS_ARCHIVE_HEADER_EYECATCHER,
                          DPS_ARCHIVE_HEADER_EYECATCHER_LEN))
      {
         isArchive = TRUE;
      }

   done:
      SAFE_OSS_DELETE(logHeader);
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayArchiveFile(const string& file)
   {
      INT32 rc = SDB_OK;
      dpsArchiveHeader* archiveHeader = NULL;
      dpsLogHeader* logHeader = NULL;
      dpsArchiveFile archiveFile;
      dpsLogFile logFile;
      string filePath = file;

      PD_LOG(PDEVENT, "Begin to replay archive log file[%s]",
             file.c_str());

      rc = _setLastFileTime(file);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = archiveFile.init(filePath, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      archiveHeader = archiveFile.getArchiveHeader();
      logHeader = archiveFile.getLogHeader();

      if (_filter.isFiltered(archiveFile))
      {
         _monitor.setNextLSN(archiveHeader->endLSN.offset);
         PD_LOG(PDEVENT, "Archive log file[%s] is filtered",
                file.c_str());
         goto done;
      }

      if (_monitor.getNextFileId() != DPS_INVALID_LOG_FILE_ID)
      {
         if (logHeader->_logID < _monitor.getNextFileId())
         {
            PD_LOG(PDINFO, "Archive log file[%s] is already replayed",
                file.c_str());
            goto done;
         }
      }

      if (_options->dumpHeader())
      {
         _dumpArchiveFileHeader(archiveFile);

         if (!_options->dump())
         {
            goto done;
         }
      }

      if (archiveHeader->hasFlag(DPS_ARCHIVE_COMPRESSED))
      {
         ossFile::deleteFile(_tmpFile);
         rc = _archiveFileMgr.copyArchiveFile(file, _tmpFile,
                                              DPS_ARCHIVE_COPY_UNCOMPRESS);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to uncompress archive file[%s], rc=%d",
                   file.c_str(), rc);
            goto error;
         }

         archiveFile.close();
         archiveHeader = NULL;
         filePath = _tmpFile;
         rc = archiveFile.init(filePath, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                   filePath.c_str(), rc);
            goto error;
         }

         archiveHeader = archiveFile.getArchiveHeader();
         logHeader = archiveFile.getLogHeader();
         archiveHeader->unsetFlag(DPS_ARCHIVE_COMPRESSED);
      }

      rc = logFile.init(filePath.c_str(),
                        (UINT32)logHeader->_fileSize,
                        logHeader->_fileNum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init log file[%s], rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      rc = _replayLogFile(logFile,
                          archiveHeader->startLSN.offset,
                          archiveHeader->endLSN.offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to replay log file[%s], rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      PD_LOG(PDEVENT, "Replay archive log file[%s] successfully",
             file.c_str());

      if (_options->remove())
      {
         fs::path dir( file );
         string fileName = dir.filename().string();

         if (_archiveFileMgr.isFullFileName(fileName))
         {
            rc = ossFile::deleteFile(file);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to delete log file[%s], rc=%d",
                      file.c_str(), rc);
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayReplicaFile(const string& file,
                                           UINT32 fileSize, UINT32 fileNum)
   {
      INT32 rc = SDB_OK ;
      dpsLogFile logFile;
      _dpsLogHeader* logHeader = NULL;
      DPS_LSN_OFFSET startLSN = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET endLSN = DPS_INVALID_LSN_OFFSET;

      PD_LOG(PDEVENT, "Begin to replay replica log file[%s]",
             file.c_str());

      rc = _setLastFileTime(file);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = logFile.init(file.c_str(), fileSize, fileNum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init replica log file[%s], rc=%d",
                file.c_str(), rc);
         goto error;
      }

      logHeader = &(logFile.header());

      if (_filter.isFiltered(logFile))
      {
         DPS_LSN_OFFSET endLSN = (UINT64)fileSize * (logHeader->_logID + 1);
         _monitor.setNextLSN(endLSN);
         PD_LOG(PDEVENT, "Archive log file[%s] is filtered",
                file.c_str());
         goto done;
      }

      if (_monitor.getNextFileId() != DPS_INVALID_LOG_FILE_ID)
      {
         if (logHeader->_logID < _monitor.getNextFileId())
         {
            PD_LOG(PDINFO, "Replica log file[%s] is already replayed",
                file.c_str());
            goto done;
         }
      }

      if (_options->dumpHeader())
      {
         _dumpReplicaFileHeader(logFile);

         if (!_options->dump())
         {
            goto done;
         }
      }

      startLSN = logFile.getFirstLSN().offset;
      endLSN = startLSN + logFile.getLength();

      rc = _replayLogFile(logFile, startLSN, endLSN);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to replay replica log file[%s], rc=%d",
                file.c_str(), rc);
         goto error;
      }

      PD_LOG(PDEVENT, "Replay replica log file[%s] successfully",
             file.c_str());

      if (_options->remove())
      {
         rc = ossFile::deleteFile(file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to delete replica log file[%s], rc=%d",
                   file.c_str(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayLogFile(engine::dpsLogFile& logFile,
                                  DPS_LSN_OFFSET startLSN, DPS_LSN_OFFSET endLSN)
   {
      INT32 rc = SDB_OK;
      dpsLogRecord log;
      dpsLogRecordHeader& logHeader = log.head();
      DPS_LSN_OFFSET currentLSN;
      INT64 i = 0;

      SDB_ASSERT(startLSN <= endLSN, "invalid start LSN");

      currentLSN = logFile.getFirstLSN().offset;
      if (DPS_INVALID_LSN_OFFSET == currentLSN || currentLSN < startLSN)
      {
         rc = SDB_DPS_CORRUPTED_LOG;
         PD_LOG(PDERROR, "Invalid first LSN[%lld] of log file[%s]",
                currentLSN, logFile.path().c_str());
         goto error;
      }

      currentLSN = startLSN;

      if (_monitor.getNextLSN() != DPS_INVALID_LSN_OFFSET)
      {
         if (currentLSN > _monitor.getNextLSN())
         {
            rc = SDB_DPS_CORRUPTED_LOG;
            PD_LOG(PDERROR, "LSN is not continuous, " \
                   "startLSN[%lld] of log file[%s], expect nextLSN[%lld]",
                   currentLSN, logFile.path().c_str(), _monitor.getNextLSN());
            goto error;
         }

         if (currentLSN < _monitor.getNextLSN())
         {
            currentLSN = _monitor.getNextLSN();
         }
      }

      if (currentLSN >= endLSN)
      {
         PD_LOG(PDINFO, "No need to replay, currentLSN[%lld], endLSN[%lld]",
                currentLSN, endLSN);
         goto done;
      }

      PD_LOG(PDEVENT, "Replay from LSN[%lld] to LSN[%lld]",
             currentLSN, endLSN);

      while (currentLSN < endLSN)
      {
         i++;

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         rc = logFile.read(currentLSN,
                           sizeof(dpsLogRecordHeader),
                           (CHAR*)&logHeader);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to read log file[%s], lsn[%lld], rc:%d",
                   logFile.path().c_str(), currentLSN, rc);
            goto error;
         }

         if (logHeader._lsn != currentLSN)
         {
            rc = SDB_DPS_INVALID_LSN;
            PD_LOG(PDERROR, "Invalid LSN, expect[%lld], real[%lld]",
                   currentLSN, logHeader._lsn);
            goto error;
         }

         rc = _ensureBufSize(logHeader._length);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to ensure buf size, rc=%d", rc);
            goto error;
         }

         rc = logFile.read(currentLSN, logHeader._length, _buf);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to read log file[%s], lsn[%lld], rc=%d",
                   logFile.path().c_str(), currentLSN, rc);
            goto error;
         }

         log.clear();
         rc = log.load(_buf);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to load log, lsn[%lld], rc=%d",
                   currentLSN, rc);
            goto error;
         }

         if (_filter.isFiltered(log, _options->dump()))
         {
            _monitor.setLastLSN(currentLSN);
            currentLSN += logHeader._length;
            _monitor.setNextLSN(currentLSN);
            if (_filter.largerThanMaxLSN(logHeader._lsn))
            {
               PD_LOG(PDINFO, "Current LSN[%lld] is larger than max LSN",
                      logHeader._lsn);
               goto done;
            }
            continue;
         }

         if (_options->dump())
         {
            _dumpLog(log);
         }
         else
         {
            rc = _replayLog(_buf);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to replay log, lsn[%lld], rc=%d",
                      logHeader._lsn, rc);
               goto error;
            }
         }

         _monitor.setLastLSN(currentLSN);
         currentLSN += logHeader._length;
         _monitor.opCount(logHeader._type);
         _monitor.setNextLSN(currentLSN);

         if (0 == i%RPL_STATUS_INTERVAL)
         {
            if (_status.isOpened())
            {
               rc = _writeStatus();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to write status, rc=%d",
                         rc);
                  goto error;
               }
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayLog(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      UINT16 type = header._type;
      switch(type)
      {
      case LOG_TYPE_DATA_INSERT:
         rc = _replayInsert(log);
         break;
      case LOG_TYPE_DATA_UPDATE:
         rc = _replayUpdate(log);
         break;
      case LOG_TYPE_DATA_DELETE:
         rc = _replayDelete(log);
         break;
      case LOG_TYPE_CL_TRUNC:
         rc = _replayTruncateCL(log);
         break;
      case LOG_TYPE_DATA_POP:
         rc = _replayPop(log) ;
         break ;
      default:
         SDB_ASSERT(FALSE, "invalid log type");
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to replay log, type=%u, rc=%d",
                type, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayInsert(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      const CHAR* fullName = NULL;
      BSONObj obj;
      sdbCollection cl;

      SDB_ASSERT(LOG_TYPE_DATA_INSERT == header._type, "not data insert log");

      rc = dpsRecord2Insert(log, &fullName, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record, lsn[%lld], rc=%d",
                header._lsn, rc);
         goto error;
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection:%s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc ) ;
         goto error ;
      }

      rc = cl.insert(obj);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_DUP_KEY == rc)
         {
            /* If duplicate key was found, just skip. */
            rc = SDB_OK;
         }
         else
         {
            PD_LOG(PDERROR, "Failed to insert record(%s), lsn[%lld], rc=%d",
                   obj.toString(FALSE, TRUE).c_str(), header._lsn, rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayUpdate(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      sdbCollection cl;
      const CHAR *fullName = NULL;
      BSONObj match;
      BSONObj oldObj;
      BSONObj newMatch;
      BSONObj modifier;
      BSONObj oldShardingKey ;
      BSONObj hint = BSON( "" << "$id" );

      SDB_ASSERT(LOG_TYPE_DATA_UPDATE == header._type, "not data update log");

      rc = dpsRecord2Update(log, &fullName,
                            match, oldObj,
                            newMatch, modifier,
                            &oldShardingKey );
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record[%lld], rc:%d",
                header._lsn, rc);
         goto error;
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection:%s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc ) ;
         goto error ;
      }

      if (!oldShardingKey.isEmpty() && _options->updateWithShardingKey())
      {
         BSONObjBuilder builder ;
         builder.appendElements(match) ;
         builder.appendElements(oldShardingKey) ;
         match = builder.obj() ;
      }

      rc = cl.update(modifier, match, hint, UPDATE_KEEP_SHARDINGKEY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to update record[%s:%s], lsn[%lld], rc=%d",
                modifier.toString(FALSE, TRUE).c_str(),
                match.toString(FALSE, TRUE).c_str(), header._lsn, rc);
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayDelete(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      sdbCollection cl;
      const CHAR *fullName = NULL;
      BSONObj obj;
      BSONObj condition;
      BSONObj hint = BSON( "" << "$id" );

      SDB_ASSERT(LOG_TYPE_DATA_DELETE == header._type, "not data delete log");

      rc = dpsRecord2Delete(log, &fullName, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record[%lld], rc=%d",
                header._lsn, rc);
         goto error;
      }

      {
         BSONObjBuilder conditionBuilder;
         BSONElement idEle = obj.getField( DMS_ID_KEY_NAME ) ;
         if ( idEle.eoo() )
         {
            PD_LOG(PDWARNING, "Failed to parse oid from bson:[%s]",
                   obj.toString().c_str());
            rc = SDB_INVALIDARG;
            goto error;
         }

         conditionBuilder.append( idEle ) ;
         condition = conditionBuilder.obj() ;
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection: %s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc);
         goto error;
      }

      rc = cl.del(condition, hint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to delete record[%s], lsn[%lld], rc=%d",
                obj.toString(FALSE, TRUE).c_str(), header._lsn, rc);
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayTruncateCL(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      sdbCollection cl;
      const CHAR *fullName = NULL;

      SDB_ASSERT(LOG_TYPE_CL_TRUNC == header._type, "not trunc cl log");

      rc = dpsRecord2CLTrunc(log, &fullName);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record[%lld], rc=%d",
                header._lsn, rc);
         goto error;
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection: %s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc);
         goto error;
      }

      rc = cl.truncate();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to truncate collection[%s], lsn[%lld], rc=%d",
                fullName, header._lsn, rc);
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayPop( const CHAR *log )
   {
      INT32 rc = SDB_OK ;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log ;
      const CHAR *fullName = NULL ;
      sdbCollection cl ;
      INT64 logicalID = 0 ;
      INT8 direction = 1 ;

      rc = dpsRecord2Pop( log, &fullName, logicalID, direction ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse log record[%lld], rc=%d",
                 header._lsn, rc ) ;
         goto error ;
      }

      rc  = _sdb->getCollection( fullName, cl ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get collection: %s, lsn[%lld], rc=%d",
                 fullName, header._lsn, rc ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_LOGICAL_ID, logicalID ) ;
         builder.append( FIELD_NAME_DIRECTION, direction ) ;
         BSONObj option = builder.obj() ;

         rc = cl.pop( option ) ;
         if ( rc )
         {
            if ( SDB_INVALIDARG == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to do pop[option: %s] on collection[%s], "
                       "lsn[%lld], rc=%d", option.toString().c_str(), fullName,
                       header._lsn, rc);
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void Replayer::_dumpArchiveFileHeader(dpsArchiveFile& archiveFile)
   {
      dpsLogHeader* logHeader = archiveFile.getLogHeader();
      dpsArchiveHeader* archiveHeader = archiveFile.getArchiveHeader();
      stringstream ss;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "File" << ": "
         << archiveFile.path()
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "LogHead" << ": "
         << string(logHeader->_eyeCatcher, DPS_LOG_HEADER_EYECATCHER_LEN)
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "LogID" << ": "
         << logHeader->_logID
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "FirstLSN" << ": "
         << "0x" << hex << right << setfill('0') << setw(16) << logHeader->_firstLSN.offset
         << "(" << dec << logHeader->_firstLSN.offset << ")"
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "ArchHead" << ": "
         << string(archiveHeader->eyeCatcher, DPS_ARCHIVE_HEADER_EYECATCHER_LEN)
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "ArchFlag" << ": "
         << "0x" << hex << right << setfill('0') << setw(8) << archiveHeader->flag
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "StartLSN" << ": "
         << "0x" << hex << right << setfill('0') << setw(16) << archiveHeader->startLSN.offset
         << "(" << dec << archiveHeader->startLSN.offset << ")"
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "EndLSN" << ": "
         << "0x" << hex << right << setfill('0') << setw(16) << archiveHeader->endLSN.offset
         << "(" << dec << archiveHeader->endLSN.offset << ")"
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "FileSize" << ": "
         << logHeader->_fileSize
         << endl;

      string out = ss.str();

      cout << out << endl;
   }

   void Replayer::_dumpReplicaFileHeader(engine::dpsLogFile& logFile)
   {
      dpsLogHeader& logHeader = logFile.header();
      stringstream ss;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "File" << ": "
         << logFile.path()
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "LogHead" << ": "
         << string(logHeader._eyeCatcher, DPS_LOG_HEADER_EYECATCHER_LEN)
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "LogID" << ": "
         << logHeader._logID
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "FirstLSN" << ": "
         << "0x" << hex << right << setfill('0') << setw(16) << logHeader._firstLSN.offset
         << "(" << dec << logHeader._firstLSN.offset << ")"
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "FileSize" << ": "
         << logFile.size()
         << endl;

      ss << " " << left << setfill(' ') << setw(RPL_DUMP_FIELD_WIDTH) << "IdleSize" << ": "
         << logFile.getIdleSize()
         << endl;

      string out = ss.str();

      cout << out << endl;
   }

   void Replayer::_dumpLog(const engine::dpsLogRecord& log)
   {
      const INT32 len = 4096 ;
      static CHAR buf[len] = {0};

      log.dump(buf, len - 1, DPS_DMP_OPT_FORMATTED);
      std::cout << buf << std::endl;
   }

   INT32 Replayer::_replayArchiveDir()
   {
      INT32 rc = SDB_OK;

      for (;;)
      {
         UINT32 minFileId = DPS_INVALID_LOG_FILE_ID;
         UINT32 maxFileId = DPS_INVALID_LOG_FILE_ID;

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         if (DPS_INVALID_LOG_FILE_ID != _monitor.getLastFileId())
         {
            UINT32 lastFileId = _monitor.getLastFileId();
            BOOLEAN movedExist = FALSE;
            time_t movedTime = 0;

            string movedFile = _archiveFileMgr.getMovedFilePath(lastFileId);
            rc = ossFile::exists(movedFile, movedExist);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                      movedFile.c_str(), rc);
               goto error;
            }

            if (movedExist)
            {
               rc = ossFile::getLastWriteTime(movedFile, movedTime);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to get last write time of file[%s], rc=%d",
                         movedFile.c_str(), rc);
                  goto error;
               }

               if (_monitor.getLastMovedFileTime() != movedTime)
               {
                  PD_LOG(PDEVENT, "Detect move operation");

                  rc = _move(lastFileId);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "Failed to move from file id[%u], rc=%d",
                            lastFileId, rc);
                     goto error;
                  }
               }
            }
         }

         rc = _scanArchiveDir(minFileId, maxFileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to scan directory[%s], rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         if (DPS_INVALID_LOG_FILE_ID != minFileId)
         {
            rc = _replayArchiveFiles(minFileId, maxFileId);
            if (SDB_OK != rc)
            {
               goto error;
            }

            if (_monitor.getLastLSN() != DPS_INVALID_LSN_OFFSET &&
                _filter.largerThanMaxLSN(_monitor.getLastLSN()))
            {
               PD_LOG(PDDEBUG, "Last LSN[%lld] is over max LSN",
                      _monitor.getLastLSN());
               goto done;
            }
         }

         if (!_options->watch())
         {
            break;
         }

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         ossSleep(RPL_WATCH_INTERVAL);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_scanArchiveDir(UINT32& minFileId, UINT32& maxFileId)
   {
      INT32 rc = SDB_OK;

      rc = _archiveFileMgr.scanArchiveFiles(minFileId, maxFileId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to scan archive directory[%s], rc=%d",
                _path.c_str(), rc);
         goto error;
      }

      if (DPS_INVALID_LOG_FILE_ID == minFileId)
      {
         goto done;
      }

      SDB_ASSERT(DPS_INVALID_LOG_FILE_ID != maxFileId, "invalid max file id");

      if (DPS_INVALID_LOG_FILE_ID == _monitor.getNextFileId())
      {
         goto done;
      }

      if (maxFileId < _monitor.getNextFileId())
      {
         PD_LOG(PDDEBUG, "Max log file id[%u] is less than next file id[%u]",
                maxFileId, _monitor.getNextFileId());
         minFileId = DPS_INVALID_LOG_FILE_ID;
         maxFileId = DPS_INVALID_LOG_FILE_ID;
         goto done;
      }

      if (minFileId > _monitor.getNextFileId())
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "Find min file id[%u], but expect next file id[%u]",
                minFileId, _monitor.getNextFileId());
         goto error;
      }

      if (maxFileId == _monitor.getNextFileId())
      {
         BOOLEAN exist = FALSE;
         time_t maxFileTime = 0;

         string maxFilePath = _archiveFileMgr.getPartialFilePath(maxFileId);
         rc = ossFile::exists(maxFilePath, exist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   maxFilePath.c_str(), rc);
            goto error;
         }

         if (exist)
         {
            rc = ossFile::getLastWriteTime(maxFilePath, maxFileTime);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to get last write of file[%s], rc=%d",
                      maxFilePath.c_str(), rc);
               goto error;
            }

            PD_LOG(PDDEBUG, "Last wirte time of file[%s] is %d, last file time is %u",
                   maxFilePath.c_str(), maxFileTime, _monitor.getLastFileTime());

            if (maxFileTime == _monitor.getLastFileTime())
            {
               minFileId = DPS_INVALID_LOG_FILE_ID;
               maxFileId = DPS_INVALID_LOG_FILE_ID;
               goto done;
            }
         }

         maxFilePath = _archiveFileMgr.getFullFilePath(maxFileId);
         rc = ossFile::exists(maxFilePath, exist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   maxFilePath.c_str(), rc);
            goto error;
         }

         if (exist)
         {
            rc = ossFile::getLastWriteTime(maxFilePath, maxFileTime);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to get last write of file[%s], rc=%d",
                      maxFilePath.c_str(), rc);
               goto error;
            }

            PD_LOG(PDDEBUG, "Last wirte time of file[%s] is %d, last file time is %u",
                   maxFilePath.c_str(), maxFileTime, _monitor.getLastFileTime());

            if (maxFileTime == _monitor.getLastFileTime())
            {
               minFileId = DPS_INVALID_LOG_FILE_ID;
               maxFileId = DPS_INVALID_LOG_FILE_ID;
               goto done;
            }
         }
      }

      minFileId = _monitor.getNextFileId();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayArchiveFiles(UINT32 minFileId, UINT32 maxFileId)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(DPS_INVALID_LOG_FILE_ID != minFileId, "invalid min file id");
      SDB_ASSERT(DPS_INVALID_LOG_FILE_ID != maxFileId, "invalid max file id");

      for (UINT32 i = minFileId; i <= maxFileId; i++)
      {
         BOOLEAN fullExist = FALSE;
         BOOLEAN partExist = FALSE;
         BOOLEAN movedExist = FALSE;
         time_t movedTime = 0;
         string file;
         UINT32 nextFileId;
         UINT32 lastFileId;

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         string fullFile = _archiveFileMgr.getFullFilePath(i);

         rc = ossFile::exists(fullFile, fullExist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   fullFile.c_str(), rc);
            goto error;
         }

         if (fullExist)
         {
            file = fullFile;
            nextFileId = i + 1;
            lastFileId = i;
         }
         else
         {
            string partFile = _archiveFileMgr.getPartialFilePath(i);

            rc = ossFile::exists(partFile, partExist);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                      partFile.c_str(), rc);
               goto error;
            }

            if (!partExist)
            {
               rc = SDB_SYS;
               PD_LOG(PDERROR, "No archive file for file id[%u]",
                      i);
               goto error;
            }

            file = partFile;
            nextFileId = i;
            lastFileId = i;
         }

         string movedFile = _archiveFileMgr.getMovedFilePath(i);
         rc = ossFile::exists(movedFile, movedExist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   movedFile.c_str(), rc);
            goto error;
         }

         if (movedExist)
         {
            rc = ossFile::getLastWriteTime(movedFile, movedTime);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to get last write time of file[%s], rc=%d",
                      movedFile.c_str(), rc);
               goto error;
            }
         }

         _monitor.setLastMovedFileTime(movedTime);

         rc = _replayArchiveFile(file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to replay file[%s], rc=%d",
                   file.c_str(), rc);
            goto error;
         }

         _monitor.setNextFileId(nextFileId);
         _monitor.setLastFileId(lastFileId);
         if (_status.isOpened())
         {
            rc = _writeStatus();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to write status, rc=%d",
                      rc);
               goto error;
            }
         }
         PD_LOG(PDINFO, "current replay:\n%s", _monitor.dump().c_str());

         if (_monitor.getLastLSN() != DPS_INVALID_LSN_OFFSET &&
             _filter.largerThanMaxLSN(_monitor.getLastLSN()))
         {
            PD_LOG(PDDEBUG, "Last LSN[%lld] is over max LSN",
                   _monitor.getLastLSN());
            goto done;
         }

         movedExist = FALSE;
         rc = ossFile::exists(movedFile, movedExist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   movedFile.c_str(), rc);
            goto error;
         }

         if (movedExist)
         {
            time_t movedTime2 = 0;
            rc = ossFile::getLastWriteTime(movedFile, movedTime2);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to get last write time of file[%s], rc=%d",
                      movedFile.c_str(), rc);
               goto error;
            }

            if (movedTime2 != movedTime)
            {
               PD_LOG(PDEVENT, "Detect move operation");

               rc = _move(i);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to move from file id[%u], rc=%d",
                         i, rc);
                  goto error;
               }

               _monitor.setLastMovedFileTime(movedTime2);

               goto done;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayReplicaDir()
   {
      INT32 rc = SDB_OK;

      for (;;)
      {
         REPLICA_FILE_MAP replicaFiles;
         UINT32 minFileId = DPS_INVALID_LOG_FILE_ID;
         UINT32 maxFileId = DPS_INVALID_LOG_FILE_ID;

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         rc = _scanReplicaDir(replicaFiles, minFileId, maxFileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to scan directory[%s], rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         if (DPS_INVALID_LOG_FILE_ID != minFileId)
         {
            rc = _replayReplicaFiles(replicaFiles, minFileId, maxFileId);
            if (SDB_OK != rc)
            {
               goto error;
            }

            if (_monitor.getLastLSN() != DPS_INVALID_LSN_OFFSET &&
                _filter.largerThanMaxLSN(_monitor.getLastLSN()))
            {
               PD_LOG(PDDEBUG, "Last LSN[%lld] is over max LSN",
                      _monitor.getLastLSN());
               goto done;
            }
         }

         if (!_options->watch())
         {
            break;
         }

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         ossSleep(RPL_WATCH_INTERVAL);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_scanReplicaDir(REPLICA_FILE_MAP& replicaFiles,
                                        UINT32& minFileId, UINT32& maxFileId)
   {
      INT32 rc = SDB_OK;

      try
      {
         fs::path dir(_path);
         fs::directory_iterator endIter;

         for (fs::directory_iterator dirIter(dir);
              dirIter != endIter;
              ++dirIter)
         {
            const string filePath = dirIter->path().string();
            BOOLEAN isArchive = FALSE;
            UINT32 fileSize = 0;
            UINT32 fileNum = 0;
            UINT32 fileId = DPS_INVALID_LOG_FILE_ID;

            if (!fs::is_regular_file(dirIter->status()))
            {
               continue;
            }

            rc = _isDpsLogFile(filePath, isArchive, fileSize, fileNum, fileId);
            if (SDB_OK != rc)
            {
               if (SDB_DPS_FILE_NOT_RECOGNISE ==  rc)
               {
                  continue;
               }

               PD_LOG(PDERROR, "Failed to check if file[%s] is log file, rc=%d",
                      filePath.c_str(), rc);
               goto error;
            }

            if (DPS_INVALID_LOG_FILE_ID == fileId)
            {
               /*rc = SDB_SYS;
               PD_LOG(PDERROR, "Invalid file id of file[%s], rc=%d",
                      filePath.c_str(), rc);
               goto error;*/
               continue;
            }

            REPLICA_FILE_MAP::const_iterator fileIter = replicaFiles.find(fileId);
            if (fileIter != replicaFiles.end())
            {
               rc = SDB_SYS;
               PD_LOG(PDERROR, "Duplicate log file id[%u] of file[%s] and file[%s], rc=%d",
                      fileId, filePath.c_str(), fileIter->second.name.c_str(), rc);
               goto error;
            }

            ReplicaFileInfo fileInfo;
            fileInfo.name = filePath;
            fileInfo.fileSize = fileSize;
            fileInfo.fileNum = fileNum;
            fileInfo.fileId = fileId;
            replicaFiles.insert(REPLICA_FILE_MAP::value_type(fileId, fileInfo));

            if ( minFileId > fileId || DPS_INVALID_LOG_FILE_ID == minFileId )
            {
               minFileId = fileId ;
            }

            if ( maxFileId < fileId || DPS_INVALID_LOG_FILE_ID == maxFileId )
            {
               maxFileId = fileId ;
            }
         }
      }
      catch( fs::filesystem_error& e )
      {
         if ( e.code() == boost::system::errc::permission_denied ||
              e.code() == boost::system::errc::operation_not_permitted )
         {
            rc = SDB_PERM ;
         }
         else
         {
            rc = SDB_IO ;
         }
         goto error ;
      }
      catch( std::exception& e )
      {
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if (DPS_INVALID_LOG_FILE_ID == minFileId)
      {
         goto done;
      }

      SDB_ASSERT(DPS_INVALID_LOG_FILE_ID != maxFileId, "invalid max file id");

      if (DPS_INVALID_LOG_FILE_ID == _monitor.getNextFileId())
      {
         goto done;
      }

      if (maxFileId < _monitor.getNextFileId())
      {
         PD_LOG(PDDEBUG, "Max log file id[%u] is less than next file id[%u]",
                maxFileId, _monitor.getNextFileId());
         minFileId = DPS_INVALID_LOG_FILE_ID;
         maxFileId = DPS_INVALID_LOG_FILE_ID;
         replicaFiles.clear();
         goto done;
      }

      if (minFileId > _monitor.getNextFileId())
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "Find min file id[%u], but expect next file id[%u]",
                minFileId, _monitor.getNextFileId());
         goto error;
      }

      if (maxFileId == _monitor.getNextFileId())
      {
         BOOLEAN exist = FALSE;
         time_t maxFileTime = 0;

         string maxFilePath = replicaFiles.rbegin()->second.name;
         rc = ossFile::exists(maxFilePath, exist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   maxFilePath.c_str(), rc);
            goto error;
         }

         if (exist)
         {
            rc = ossFile::getLastWriteTime(maxFilePath, maxFileTime);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to get last write of file[%s], rc=%d",
                      maxFilePath.c_str(), rc);
               goto error;
            }

            PD_LOG(PDDEBUG, "Last wirte time of file[%s] is %d, last file time is %u",
                   maxFilePath.c_str(), maxFileTime, _monitor.getLastFileTime());

            if (maxFileTime == _monitor.getLastFileTime())
            {
               minFileId = DPS_INVALID_LOG_FILE_ID;
               maxFileId = DPS_INVALID_LOG_FILE_ID;
               replicaFiles.clear();
               goto done;
            }
         }
      }

      if (minFileId < _monitor.getNextFileId())
      {
         REPLICA_FILE_MAP::iterator it = replicaFiles.begin();
         while (it != replicaFiles.end())
         {
            if (it->first < _monitor.getNextFileId())
            {
               replicaFiles.erase(it++);
            }
            else
            {
               break;
            }
         }
         minFileId = _monitor.getNextFileId();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_replayReplicaFiles(REPLICA_FILE_MAP& replicaFiles,
                                            UINT32 minFileId, UINT32 maxFileId)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(DPS_INVALID_LOG_FILE_ID != minFileId, "invalid min file id");
      SDB_ASSERT(DPS_INVALID_LOG_FILE_ID != maxFileId, "invalid max file id");
      SDB_ASSERT(replicaFiles.begin()->first == minFileId, "invalid min file id");

      for (REPLICA_FILE_MAP::const_iterator it = replicaFiles.begin();
           it != replicaFiles.end();
           it++)
      {
         UINT32 fileId = it->first;
         const ReplicaFileInfo& fileInfo = it->second;
         BOOLEAN fileExist = FALSE;

         SDB_ASSERT( fileId == fileInfo.fileId, "invalid file id");

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         rc = ossFile::exists(fileInfo.name, fileExist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to access file[%s], rc=%d",
                   fileInfo.name.c_str(), rc);
            goto error;
         }

         if (!fileExist)
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "Replica file[%s] is not found, rc=%d",
                   fileInfo.name.c_str(), rc);
            goto error;
         }

         rc = _replayReplicaFile(fileInfo.name, fileInfo.fileSize, fileInfo.fileNum);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to replay file[%s], rc=%d",
                   fileInfo.name.c_str(), rc);
            goto error;
         }

         if (_monitor.getNextLSN() > (UINT64)fileInfo.fileSize * (fileInfo.fileId + 1))
         {
            _monitor.setNextFileId(fileId + 1);
         }
         else
         {
            _monitor.setNextFileId(fileId);
         }
         _monitor.setLastFileId(fileId);
         if (_status.isOpened())
         {
            rc = _writeStatus();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to write status, rc=%d",
                      rc);
               goto error;
            }
         }
         PD_LOG(PDINFO, "current replay:\n%s", _monitor.dump().c_str());

         if (_monitor.getLastLSN() != DPS_INVALID_LSN_OFFSET &&
             _filter.largerThanMaxLSN(_monitor.getLastLSN()))
         {
            PD_LOG(PDDEBUG, "Last LSN[%lld] is over max LSN",
                   _monitor.getLastLSN());
            goto done;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_ensureFileSize(const string& filePath, INT64 fileSize)
   {
      INT32 rc = SDB_OK;
      INT64 realSize = 0;

      rc = ossFile::getFileSize(filePath, realSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get file size[%s], rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      realSize -= DPS_LOG_HEAD_LEN;

      if (realSize < fileSize)
      {
         INT64 increment = fileSize - realSize;
         rc = ossFile::extend(filePath, increment);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to extend file[%s], rc=%d",
                   filePath.c_str(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_ensureBufSize(UINT32 size)
   {
      INT32 rc = SDB_OK;

      if (_bufSize >= size)
      {
         goto done;
      }

      if (0 != _bufSize)
      {
         SAFE_OSS_FREE(_buf);
         _bufSize = 0;
      }

      SDB_ASSERT(NULL == _buf, "_buf should be NULL");
      _buf = (CHAR*)SDB_OSS_MALLOC(size);
      if (NULL == _buf)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "Failed to malloc, size=%u", size);
         goto error;
      }
      _bufSize = size;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_setLastFileTime(const string& filePath)
   {
      time_t lastTime = 0;
      INT32 rc = SDB_OK;

      rc = ossFile::getLastWriteTime(filePath, lastTime);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get last wirte time of file[%s], rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      _monitor.setLastFileTime(lastTime);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_move(UINT32 startFileId)
   {
      INT32 rc = SDB_OK;

      for (INT64 i = (INT64)startFileId; i >= 0; i--)
      {
         BOOLEAN exist = FALSE;
         string movedFilePath = _archiveFileMgr.getMovedFilePath((UINT32)i);
         string filePath = movedFilePath;
         dpsArchiveFile archiveFile;
         dpsLogFile logFile;
         dpsArchiveHeader* archiveHeader = NULL;
         dpsLogHeader* logHeader = NULL;
         DPS_LSN_OFFSET lastLSN = DPS_INVALID_LSN_OFFSET;

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Rollback is interrupted");
            goto error;
         }

         rc = ossFile::exists(movedFilePath, exist);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to check if moved file[%s] exists, rc=%d",
                   movedFilePath.c_str(), rc);
            goto error;
         }

         if (!exist)
         {
            break;
         }

         PD_LOG(PDEVENT, "Begin to rollback archive log file[%s]",
                movedFilePath.c_str());

         rc = archiveFile.init(movedFilePath, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to init moved archive file[%s], rc=%d",
                   movedFilePath.c_str(), rc);
            goto error;
         }

         archiveHeader = archiveFile.getArchiveHeader();
         logHeader = archiveFile.getLogHeader();

         if (_filter.isFiltered(archiveFile))
         {
            PD_LOG(PDEVENT, "Archive log file[%s] is filtered",
                   movedFilePath.c_str());
            continue;
         }

         if (_options->dumpHeader())
         {
            _dumpArchiveFileHeader(archiveFile);

            if (!_options->dump())
            {
               continue;
            }
         }

         if (archiveHeader->hasFlag(DPS_ARCHIVE_COMPRESSED))
         {
            ossFile::deleteFile(_tmpFile);
            rc = _archiveFileMgr.copyArchiveFile(movedFilePath, _tmpFile,
                                                 DPS_ARCHIVE_COPY_UNCOMPRESS);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to uncompress archive file[%s], rc=%d",
                      movedFilePath.c_str(), rc);
               goto error;
            }

            archiveFile.close();
            archiveHeader = NULL;
            filePath = _tmpFile;
            rc = archiveFile.init(filePath, TRUE);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                      filePath.c_str(), rc);
               goto error;
            }

            archiveHeader = archiveFile.getArchiveHeader();
            logHeader = archiveFile.getLogHeader();
            archiveHeader->unsetFlag(DPS_ARCHIVE_COMPRESSED);
         }

         rc = logFile.init(filePath.c_str(),
                           (UINT32)logHeader->_fileSize,
                           logHeader->_fileNum);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to init log file[%s], rc=%d",
                   filePath.c_str(), rc);
            goto error;
         }

         rc = _findLastLSN(logFile,
                           archiveHeader->startLSN.offset,
                           archiveHeader->endLSN.offset,
                           lastLSN);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to find last log in file[%s], rc=%d",
                   filePath.c_str(), rc);
            goto error;
         }

         rc = _rollbackLogFile(logFile,
                               lastLSN,
                               archiveHeader->startLSN.offset);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to rollback log file[%s], rc=%d",
                   filePath.c_str(), rc);
            goto error;
         }

         _monitor.setNextFileId((UINT32)i);
         _monitor.setLastFileId((UINT32)(i - 1));
         _monitor.setLastMovedFileTime(0);
         if (_status.isOpened())
         {
            rc = _writeStatus();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to write status, rc=%d",
                      rc);
               goto error;
            }
         }

         PD_LOG(PDEVENT, "Rollback archive log file[%s] successfully",
                movedFilePath.c_str());

         if (_options->remove())
         {
            rc = ossFile::deleteFile(movedFilePath);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to delete log file[%s], rc=%d",
                      movedFilePath.c_str(), rc);
               goto error;
            }
         }
      }


   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackLogFile(engine::dpsLogFile& logFile,
                           DPS_LSN_OFFSET startLSN, DPS_LSN_OFFSET endLSN)
   {
      INT32 rc = SDB_OK;
      dpsLogRecord log;
      dpsLogRecordHeader& logHeader = log.head();
      DPS_LSN_OFFSET currentLSN;
      INT64 i = 0;

      SDB_ASSERT(startLSN >= endLSN, "invalid startLSN & endLSN");

      currentLSN = startLSN;

      if (_monitor.getLastLSN() != DPS_INVALID_LSN_OFFSET)
      {
         if (currentLSN > _monitor.getLastLSN())
         {
            currentLSN = _monitor.getLastLSN();
         }
      }

      PD_LOG(PDEVENT, "Rollback from LSN[%lld] to LSN[%lld]",
             currentLSN, endLSN);

      while (currentLSN >= endLSN && currentLSN != DPS_INVALID_LSN_OFFSET)
      {
         i++;

         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         rc = logFile.read(currentLSN,
                           sizeof(dpsLogRecordHeader),
                           (CHAR*)&logHeader);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to read log file[%s], lsn[%lld], rc:%d",
                   logFile.path().c_str(), currentLSN, rc);
            goto error;
         }

         if (logHeader._lsn != currentLSN)
         {
            rc = SDB_DPS_INVALID_LSN;
            PD_LOG(PDERROR, "Invalid LSN, expect[%lld], real[%lld]",
                   currentLSN, logHeader._lsn);
            goto error;
         }

         rc = _ensureBufSize(logHeader._length);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to ensure buf size, rc=%d", rc);
            goto error;
         }

         rc = logFile.read(currentLSN, logHeader._length, _buf);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to read log file[%s], lsn[%lld], rc=%d",
                   logFile.path().c_str(), currentLSN, rc);
            goto error;
         }

         log.clear();
         rc = log.load(_buf);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to load log, lsn[%lld], rc=%d",
                   currentLSN, rc);
            goto error;
         }

         if (_filter.isFiltered(log))
         {
            _monitor.setNextLSN(currentLSN);
            currentLSN = logHeader._preLsn;
            _monitor.setLastLSN(currentLSN);
            continue;
         }

         if (_options->dump())
         {
            _dumpRollbackLog(log);
         }
         else
         {
            rc = _rollbackLog(_buf);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to rollback log, lsn[%lld], rc=%d",
                      logHeader._lsn, rc);
               goto error;
            }
         }

         _monitor.setNextLSN(currentLSN);
         currentLSN = logHeader._preLsn;
         _monitor.opCount(logHeader._type);
         _monitor.setLastLSN(currentLSN);

         if (0 == i%RPL_STATUS_INTERVAL)
         {
            if (_status.isOpened())
            {
               rc = _writeStatus();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to write status, rc=%d",
                         rc);
                  goto error;
               }
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_findLastLSN(engine::dpsLogFile& logFile,
                                    DPS_LSN_OFFSET startLSN,
                                    DPS_LSN_OFFSET endLSN,
                                    DPS_LSN_OFFSET& lastLSN)
   {
      INT32 rc = SDB_OK;
      dpsLogRecordHeader logHeader;
      DPS_LSN_OFFSET currentLSN = startLSN;

      SDB_ASSERT(startLSN < endLSN, "invalid startLSN & endLSN");

      while (currentLSN < endLSN)
      {
         if (!_isRunning)
         {
            rc = SDB_INTERRUPT;
            PD_LOG(PDINFO, "Replay is interrupted");
            goto done;
         }

         rc = logFile.read(currentLSN,
                           sizeof(dpsLogRecordHeader),
                           (CHAR*)&logHeader);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to read log file[%s], lsn[%lld], rc:%d",
                   logFile.path().c_str(), currentLSN, rc);
            goto error;
         }

         if (logHeader._lsn != currentLSN)
         {
            rc = SDB_DPS_INVALID_LSN;
            PD_LOG(PDERROR, "Invalid LSN, expect[%lld], real[%lld]",
                   currentLSN, logHeader._lsn);
            goto error;
         }

         if (currentLSN + logHeader._length >= endLSN)
         {
            SDB_ASSERT(currentLSN + logHeader._length == endLSN, "corrupted LSN");
            break;
         }

         currentLSN += logHeader._length;
         logHeader.clear();
      }

      lastLSN = currentLSN;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackLog(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      UINT16 type = header._type;
      switch(type)
      {
      case LOG_TYPE_DATA_INSERT:
         rc = _rollbackInsert(log);
         break;
      case LOG_TYPE_DATA_UPDATE:
         rc = _rollbackUpdate(log);
         break;
      case LOG_TYPE_DATA_DELETE:
         rc = _rollbackDelete(log);
         break;
      case LOG_TYPE_CL_TRUNC:
         rc = _rollbackTruncateCL(log);
         break;
      case LOG_TYPE_DATA_POP:
         rc = _rollbackPop(log);
         break;
      default:
         SDB_ASSERT(FALSE, "invalid log type");
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to rollback log, type=%u, rc=%d",
                type, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackInsert(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      const CHAR* fullName = NULL;
      BSONObj obj;
      BSONObj selector;
      BSONObj hint;
      sdbCollection cl;

      SDB_ASSERT(LOG_TYPE_DATA_INSERT == header._type, "not data insert log");

      rc = dpsRecord2Insert(log, &fullName, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record, lsn[%lld], rc=%d",
                header._lsn, rc);
         goto error;
      }

      {
         BSONElement idEle = obj.getField( DMS_ID_KEY_NAME ) ;
         if ( idEle.eoo() )
         {
            PD_LOG(PDWARNING, "Failed to parse oid from bson:[%s]",
                   obj.toString().c_str()) ;
            rc = SDB_SYS;
            goto error;
         }

         try
         {
            BSONObjBuilder selectorBuilder ;
            selectorBuilder.append( idEle ) ;
            selector = selectorBuilder.obj() ;
            hint = BSON(""<<IXM_ID_KEY_NAME) ;
         }
         catch (std::exception& e)
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "Unexpected error happened: %s", e.what());
            goto error;
         }
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection:%s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc ) ;
         goto error ;
      }

      rc = cl.del(selector, hint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to rollback insert record(%s), lsn[%lld], rc=%d",
                obj.toString(FALSE, TRUE).c_str(), header._lsn, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackUpdate(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      sdbCollection cl;
      const CHAR *fullName = NULL;
      BSONObj match;
      BSONObj oldObj;
      BSONObj newMatch;
      BSONObj modifier;
      BSONObj newShardingKey ;
      BSONObj hint = BSON( "" << "$id" );

      SDB_ASSERT(LOG_TYPE_DATA_UPDATE == header._type, "not data update log");

      rc = dpsRecord2Update(log, &fullName,
                            match, oldObj,
                            newMatch, modifier,
                            NULL, &newShardingKey );
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record[%lld], rc:%d",
                header._lsn, rc);
         goto error;
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection:%s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc ) ;
         goto error ;
      }

      if (!newShardingKey.isEmpty() && _options->updateWithShardingKey())
      {
         BSONObjBuilder builder ;
         builder.appendElements( newMatch ) ;
         builder.appendElements( newShardingKey ) ;
         newMatch = builder.obj() ;
      }

      rc = cl.update(oldObj, newMatch, hint, UPDATE_KEEP_SHARDINGKEY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to rollback update record[%s:%s], lsn[%lld], rc=%d",
                oldObj.toString(FALSE, TRUE).c_str(),
                newMatch.toString(FALSE, TRUE).c_str(), header._lsn, rc);
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackDelete(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      sdbCollection cl;
      const CHAR *fullName = NULL;
      BSONObj obj;
      BSONObj hint = BSON( "" << "$id" );

      SDB_ASSERT(LOG_TYPE_DATA_DELETE == header._type, "not data delete log");

      rc = dpsRecord2Delete(log, &fullName, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record[%lld], rc=%d",
                header._lsn, rc);
         goto error;
      }

      rc = _sdb->getCollection(fullName, cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection: %s, lsn[%lld], rc=%d",
                fullName, header._lsn, rc);
         goto error;
      }

      rc = cl.insert(obj);
      if (SDB_OK != rc)
      {
         if (SDB_IXM_DUP_KEY == rc)
         {
            /* If duplicate key was found, just skip. */
            rc = SDB_OK;
         }
         else
         {
            PD_LOG(PDERROR, "Failed to rollback insert record(%s), lsn[%lld], rc=%d",
                   obj.toString(FALSE, TRUE).c_str(), header._lsn, rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackTruncateCL(const CHAR* log)
   {
      INT32 rc = SDB_OK;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log;
      const CHAR *fullName = NULL;

      SDB_ASSERT(LOG_TYPE_CL_TRUNC == header._type, "not trunc cl log");

      rc = dpsRecord2CLTrunc(log, &fullName);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to parse log record[%lld], rc=%d",
                header._lsn, rc);
         goto error;
      }

      rc = SDB_SYS;
      PD_LOG(PDERROR, "Truncate collection[%s] can't be rollbacked, " \
             "lsn[%lld], prelsn[%lld]",
             fullName, header._lsn, header._preLsn);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_rollbackPop( const CHAR* log )
   {
      INT32 rc = SDB_OK ;
      const dpsLogRecordHeader& header = *(const dpsLogRecordHeader*)log ;
      const CHAR *fullName = NULL;
      INT64 logicalID = 0 ;
      INT8 direction = 1 ;

      SDB_ASSERT( LOG_TYPE_DATA_POP == header._type, "not pop log" ) ;

      rc = dpsRecord2Pop( log, &fullName, logicalID, direction ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse log record[%lld], rc=%d",
                 header._lsn, rc ) ;
         goto error ;
      }

      rc = SDB_SYS ;
      PD_LOG( PDERROR, "Pop collection[%s] can't be rollbacked, lsn[%lld], "
              "prelsn[%lld]", fullName, header._lsn, header._preLsn ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void Replayer::_dumpRollbackLog(const engine::dpsLogRecord& log)
   {
      const INT32 len = 4096 ;
      static CHAR buf[len] = {0};

      log.dump(buf, len - 1, DPS_DMP_OPT_FORMATTED);
      std::cout << buf << "Rollback: true"  << std::endl;
   }

   INT32 Replayer::_connectSdb()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL == _sdb, "_sdb is not null");

      if (_options->dump() ||
          _options->dumpHeader() ||
          _options->deflate() ||
          _options->inflate())
      {
         goto done;
      }

      _sdb = new(std::nothrow) sdbclient::sdb(_options->useSSL());
      if (NULL == _sdb)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "Failed to new sdbclient::sdb, rc=%d", rc);
         goto error;
      }

      rc = _sdb->connect(_options->hostName().c_str(),
                        _options->serviceName().c_str(),
                        _options->user().c_str(),
                        _options->password().c_str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to connect to sdb[%s:%s], rc=%d",
                _options->hostName().c_str(),
                _options->serviceName().c_str(),
                rc);
         goto error;
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(_sdb);
      goto done;
   }

   INT32 Replayer::_initStatus()
   {
      INT32 rc = SDB_OK;
      string filePath;
      UINT32 mode;
      BOOLEAN exist = FALSE;

      SDB_ASSERT(NULL != _options, "_options can't be NULL");

      filePath = _options->status();
      SDB_ASSERT(!filePath.empty(), "status file is empty");

      rc = ossFile::exists(filePath, exist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to check if file[%s] exists, rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      if (exist)
      {
         mode = OSS_READWRITE;
      }
      else
      {
         mode = OSS_CREATEONLY | OSS_READWRITE;
      }

      rc = _status.open(filePath, mode, OSS_DEFAULTFILE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to open status file[%s], rc=%d",
                filePath.c_str(), rc);
         goto error;
      }

      rc = _readStatus();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to read status, rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_readStatus()
   {
      INT32 rc = SDB_OK;
      BSONObj data;
      BSONElement ele;

      SDB_ASSERT(_status.isOpened(), "_status is not opened");

      rc = engine::utilJsonFile::read(_status, data);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to read json from status file[%s], rc=%d",
                _status.getPath().c_str(), rc);
         goto error;
      }

      ele = data.getField(RPL_STATUS_NEXT_LSN) ;
      if (EOO != ele.type())
      {
         DPS_LSN_OFFSET nextLSN;

         if (NumberLong != ele.type() && NumberInt != ele.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid json field[%s]", RPL_STATUS_NEXT_LSN);
            goto error;
         }
         nextLSN = (DPS_LSN_OFFSET)ele.numberLong();
         if (nextLSN < 0)
         {
            rc = SDB_INVALIDARG ;
            PD_LOG(PDERROR, "Invalid value of field[%s]: %d",
                   RPL_STATUS_NEXT_LSN, nextLSN);
            goto error;
         }

         _monitor.setNextLSN(nextLSN);
      }

      ele = data.getField(RPL_STATUS_LAST_LSN) ;
      if (EOO != ele.type())
      {
         DPS_LSN_OFFSET lastLSN;

         if (NumberLong != ele.type() && NumberInt != ele.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid json field[%s]", RPL_STATUS_LAST_LSN);
            goto error;
         }
         lastLSN = (DPS_LSN_OFFSET)ele.numberLong();
         if (lastLSN < 0)
         {
            rc = SDB_INVALIDARG ;
            PD_LOG(PDERROR, "Invalid value of field[%s]: %d",
                   RPL_STATUS_LAST_LSN, lastLSN);
            goto error;
         }

         _monitor.setLastLSN(lastLSN);
      }

      ele = data.getField(RPL_STATUS_NEXT_FILEID);
      if (EOO != ele.type())
      {
         UINT32 nextFileId;

         if (NumberLong != ele.type() && NumberInt != ele.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid json field[%s]", RPL_STATUS_NEXT_FILEID);
            goto error;
         }
         nextFileId = (UINT32)ele.numberLong();
         _monitor.setNextFileId(nextFileId);
      }

      ele = data.getField(RPL_STATUS_LAST_FILEID);
      if (EOO != ele.type())
      {
         UINT32 lastFileId;

         if (NumberLong != ele.type() && NumberInt != ele.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid json field[%s]", RPL_STATUS_LAST_FILEID);
            goto error;
         }
         lastFileId = (UINT32)ele.numberLong();
         _monitor.setLastFileId(lastFileId);
      }

      ele = data.getField(RPL_STATUS_LAST_MOVED_FILETIME);
      if (EOO != ele.type())
      {
         time_t lastMovedFileTime;

         if (NumberLong != ele.type() && NumberInt != ele.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid json field[%s]", RPL_STATUS_LAST_MOVED_FILETIME);
            goto error;
         }
         lastMovedFileTime = (time_t)ele.numberLong();
         _monitor.setLastMovedFileTime(lastMovedFileTime);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_writeStatus()
   {
      INT32 rc = SDB_OK;
      bson::BSONObjBuilder builder;
      BSONObj data;

      SDB_ASSERT(_status.isOpened(), "_status is not opened");

      try
      {
         builder.append(RPL_STATUS_NEXT_LSN, (INT64)_monitor.getNextLSN());
         builder.append(RPL_STATUS_NEXT_FILEID, _monitor.getNextFileId());
         builder.append(RPL_STATUS_LAST_LSN, (INT64)_monitor.getLastLSN());
         builder.append(RPL_STATUS_LAST_FILEID, _monitor.getLastFileId());
         builder.append(RPL_STATUS_LAST_MOVED_FILETIME,
                        (INT64)_monitor.getLastMovedFileTime());
         data = builder.obj();
      }
      catch (std::exception& e)
      {
         rc = SDB_SYS;
         PD_LOG (PDERROR, "Failed to create BSON object: %s",
                 e.what()) ;
         goto error;
      }

      rc = engine::utilJsonFile::write(_status, data);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to write json to status file[%s], rc=%d",
                _status.getPath().c_str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Replayer::_deflateFile(const string& file)
   {
      INT32 rc = SDB_OK;
      dpsArchiveHeader* archiveHeader = NULL;
      dpsArchiveFile archiveFile;
      string deflateFile = file + RPL_DEFLATE_SUFFIX;

      rc = archiveFile.init(file, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                file.c_str(), rc);
         goto error;
      }

      archiveHeader = archiveFile.getArchiveHeader();

      if (archiveHeader->hasFlag(DPS_ARCHIVE_COMPRESSED))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDINFO, "Archive file[%s] is already compressed",
                file.c_str());
         goto error;
      }

      rc = ossFile::deleteFileIfExists(deflateFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to delete file[%s], rc=%d",
                deflateFile.c_str(), rc);
         goto error;
      }

      rc = _archiveFileMgr.copyArchiveFile(file, deflateFile,
                                           DPS_ARCHIVE_COPY_COMPRESS);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to compress archive file[%s], rc=%d",
                file.c_str(), rc);
         goto error;
      }

      archiveFile.close();
      archiveHeader = NULL;

      rc = archiveFile.init(deflateFile, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                deflateFile.c_str(), rc);
         goto error;
      }

      archiveHeader = archiveFile.getArchiveHeader();
      archiveHeader->setFlag(DPS_ARCHIVE_COMPRESSED);

      rc = archiveFile.flushHeader();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to flush header of archive log file[%s], rc=%d",
                deflateFile.c_str(), rc);
         goto error;
      }

      archiveFile.close();

   done:
      return rc;
   error:
      ossFile::deleteFileIfExists(deflateFile);
      goto done;
   }

   INT32 Replayer::_inflateFile(const string& file)
   {
      INT32 rc = SDB_OK;
      dpsArchiveHeader* archiveHeader = NULL;
      dpsArchiveFile archiveFile;
      string inflateFile = file + RPL_INFLATE_SUFFIX;

      rc = archiveFile.init(file, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                file.c_str(), rc);
         goto error;
      }

      archiveHeader = archiveFile.getArchiveHeader();

      if (!archiveHeader->hasFlag(DPS_ARCHIVE_COMPRESSED))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDINFO, "Archive file[%s] is not compressed",
                file.c_str());
         goto error;
      }

      rc = ossFile::deleteFileIfExists(inflateFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to delete file[%s], rc=%d",
                inflateFile.c_str(), rc);
         goto error;
      }

      rc = _archiveFileMgr.copyArchiveFile(file, inflateFile,
                                           DPS_ARCHIVE_COPY_UNCOMPRESS);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to uncompress archive file[%s], rc=%d",
                file.c_str(), rc);
         goto error;
      }

      archiveFile.close();
      archiveHeader = NULL;

      rc = archiveFile.init(inflateFile, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to init archive log file[%s], rc=%d",
                inflateFile.c_str(), rc);
         goto error;
      }

      archiveHeader = archiveFile.getArchiveHeader();
      archiveHeader->unsetFlag(DPS_ARCHIVE_COMPRESSED);

      rc = archiveFile.flushHeader();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to flush header of archive log file[%s], rc=%d",
                inflateFile.c_str(), rc);
         goto error;
      }

      archiveFile.close();

   done:
      return rc;
   error:
      ossFile::deleteFileIfExists(inflateFile);
      goto done;
   }
}

