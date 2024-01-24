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

   Source File Name = rplReplayer.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_REPLAYER_HPP_
#define REPLAY_REPLAYER_HPP_

#include "oss.hpp"
#include "rplOptions.hpp"
#include "rplFilter.hpp"
#include "rplMonitor.hpp"
#include "rplOutputter.hpp"
#include "../client/client.hpp"
#include "dpsArchiveFileMgr.hpp"
#include "dpsLogFile.hpp"
#include "ossFile.hpp"
#include "rplMonitorStore.hpp"
#include <string>
#include <map>

using namespace std;

namespace replay
{
   struct ReplicaFileInfo
   {
      string name;
      UINT32 fileSize;
      UINT32 fileNum;
      UINT32 fileId;
   };

   typedef map<UINT32, ReplicaFileInfo> REPLICA_FILE_MAP;

   class Replayer: public SDBObject
   {
   public:
      Replayer();
      ~Replayer();
      INT32 init(Options& options);
      INT32 run();

   private:
      INT32 _initMonitorStore( Monitor *monitor ) ;
      INT32 _initOutputter();
      INT32 _replayFile();
      INT32 _replayDir();
      INT32 _isDpsLogFile(const string& path, BOOLEAN& isArchive,
                               UINT32& fileSize, UINT32& fileNum, UINT32& fileId);
      INT32 _replayArchiveFile(const string& file);
      INT32 _replayReplicaFile(const string& file, UINT32 fileSize, UINT32 fileNum);
      INT32 _replayLogFile(engine::dpsLogFile& logFile,
                           DPS_LSN_OFFSET startLSN, DPS_LSN_OFFSET endLSN);
      INT32 _replayLog(const CHAR* log);
      void  _dumpArchiveFileHeader(engine::dpsArchiveFile& archiveFile);
      void  _dumpReplicaFileHeader(engine::dpsLogFile& logFile);
      void  _dumpLog(const engine::dpsLogRecord& log);
      INT32 _replayArchiveDir();
      INT32 _scanArchiveDir(UINT32& minFileId, UINT32& maxFileId);
      INT32 _replayArchiveFiles(UINT32 minFileId, UINT32 maxFileId);
      INT32 _replayReplicaDir();
      BOOLEAN _isLogFileName(const string& fileName);
      INT32 _scanReplicaDir(REPLICA_FILE_MAP& replicaFiles, UINT32& minFileId, UINT32& maxFileId);
      INT32 _replayReplicaFiles(REPLICA_FILE_MAP& replicaFiles, UINT32 minFileId, UINT32 maxFileId);
      INT32 _ensureFileSize(const string& filePath, INT64 fileSize);
      INT32 _ensureBufSize(UINT32 size);
      INT32 _setLastFileTime(const string& filePath);
      INT32 _replayInsert(const CHAR* log);
      INT32 _replayUpdate(const CHAR* log);
      INT32 _replayDelete(const CHAR* log);
      INT32 _replayTruncateCL(const CHAR* log);
      INT32 _replayPop(const CHAR *log) ;
      INT32 _move(UINT32 startFileId);
      INT32 _rollbackLogFile(engine::dpsLogFile& logFile,
                           DPS_LSN_OFFSET startLSN, DPS_LSN_OFFSET endLSN);
      INT32 _findLastLSN(engine::dpsLogFile& logFile,
                             DPS_LSN_OFFSET startLSN, DPS_LSN_OFFSET endLSN,
                             DPS_LSN_OFFSET& lastLSN);
      INT32 _rollbackLog(const CHAR* log);
      void  _dumpRollbackLog(const engine::dpsLogRecord& log);
      INT32 _rollbackInsert(const CHAR* log);
      INT32 _rollbackUpdate(const CHAR* log);
      INT32 _rollbackDelete(const CHAR* log);
      INT32 _rollbackTruncateCL(const CHAR* log);
      INT32 _rollbackPop(const CHAR* log);
      INT32 _deflateFile(const string& file);
      INT32 _inflateFile(const string& file);
      INT32 _checkSubmit();

      void _removeTmpFile();

   private:
      string                     _tmpFile;
      Options*                   _options;
      Filter                     _filter;
      Monitor                    _monitor;
      engine::ossFile            _status;
      rplMonitorStore*           _monitorStore;
      string                     _path;
      rplOutputter*              _outputter;
      engine::dpsArchiveFileMgr  _archiveFileMgr;
      CHAR*                      _buf;
      UINT32                     _bufSize;
   };
}

#endif /* REPLAY_REPLAYER_HPP_ */

