/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = revertCommon.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/

#include "revertCommon.hpp"
#include "pd.hpp"
#include "dpsMetaFile.hpp"
#include "ossUtil.hpp"
#include "ossFile.hpp"
#include "utilStr.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <sstream>

namespace fs = boost::filesystem ;
using namespace engine ;

namespace sdbrevert
{
   static INT32 _listLogFile( const string &filePath, logFileMgr &logFileMgr )
   {
      INT32 rc = SDB_OK ;

      if ( !fs::exists( filePath ) )
      {
         rc = SDB_FNE ;
         PD_LOG( PDERROR, "File or path is not existing, filePath= %s, rc= %d", filePath.c_str(), rc ) ;
         goto error ;
      }

      if ( fs::is_regular_file( filePath ) )
      {
         // sikp sequoiadbLog.mate
         if ( isReplicalogMeta( filePath ) )
         {
            goto done ;
         }

         // not support archivelog.<FileId>.m
         if ( isArchivelogM( filePath ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Not support archivelog.<FileId>.m, file[%s], rc= %d",
                    filePath.c_str(), rc ) ;
            goto error ;
         }

         if ( isArchivelog( filePath ) || isReplicalog( filePath ) )
         {
            logFileMgr.push( filePath ) ;
            goto done ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "File[%s] is not archivelog or replicalog, rc= ",
                    filePath.c_str(), rc ) ;
            goto error ;
         }
      }
      else if ( fs::is_directory( filePath ) )
      {
         string path ;
         fs::directory_iterator endIter ;
         for ( fs::directory_iterator dirIter( filePath ) ; dirIter != endIter ; ++dirIter )
         {
            if ( !fs::is_regular_file( dirIter->status() ) )
            {
               continue ;
            }

            path = dirIter->path().string() ;
            // sikp sequoiadbLog.mate
            if ( isReplicalogMeta( path ) )
            {
               continue ;
            }

            // skip archivelog.<FileId>.m
            if ( isArchivelogM( path ) )
            {
               PD_LOG( PDINFO, "Skip archivelog.<FileId>.m, file[%s]", filePath.c_str() ) ;
               continue ;
            }

            if ( isArchivelog( path ) || isReplicalog( path ) )
            {
               logFileMgr.push( path ) ;
            }
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Not File or directory, filePath= %s, rc= ", filePath.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 listAllLogFile( const vector<string> &pathList, logFileMgr &logFileMgr )
   {
      INT32 rc = SDB_OK ;

      for ( size_t i = 0 ; i < pathList.size() ; ++i )
      {
         rc = _listLogFile( pathList[i], logFileMgr ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN isArchivelog( const string &filePath )
   {
      string fileName = ossFile::getFileName( filePath ) ;
      return utilStrStartsWith( fileName, SDB_REVERT_ARCHIVELOG_PREFIX ) ;
   }

   BOOLEAN isArchivelogM( const string &filePath )
   {
      if ( !isArchivelog( filePath ) )
      {
         return FALSE ;
      }

      string fileName = ossFile::getFileName( filePath ) ;
      return utilStrEndsWith( fileName, SDB_REVERT_ARCHIVELOG_M_SUFFIX ) ;
   }

   BOOLEAN isReplicalog( const string &filePath )
   {
      string fileName = ossFile::getFileName( filePath ) ;
      return utilStrStartsWith( fileName, SDB_REVERT_REPLICALOG_PREFIX ) ;
   }

   BOOLEAN isReplicalogMeta( const string &filePath )
   {
      string fileName = ossFile::getFileName( filePath ) ;
      return ossStrcmp( fileName.c_str(), SDB_REVERT_REPLICALOG_META ) == 0 ;
   }

   vector<string> splitString( const string& str, char delimiter )
   {
      vector<string> substrings ;
      string::size_type start = 0 ;
      string::size_type end   = str.find( delimiter ) ;

      while ( end != string::npos )
      {
         substrings.push_back( str.substr( start, end - start ) ) ;
         start = end + 1 ;
         end = str.find( delimiter, start ) ;
      }

      substrings.push_back( str.substr( start ) ) ;

      return substrings ;
   }

   // orgObj contain matcherObj, return true
   BOOLEAN match( const bson::BSONObj &orgObj, const bson::BSONObj &matcherObj )
   {
      SDB_ASSERT( !orgObj.isEmpty(), "origin bson object can not be empty" ) ;

      if ( matcherObj.isEmpty() )
      {
         return TRUE ;
      }

      bson::BSONObjIterator matcherItr( matcherObj ) ;
      while ( matcherItr.more() )
      {
         bson::BSONElement matcherEle = matcherItr.next() ;

         bson::BSONElement orgEle = orgObj.getField( matcherEle.fieldName() ) ;
         if ( orgEle.ok() && orgEle == matcherEle )
         {
            continue ;
         }
         else
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   string joinPath( const string &path, const string &fileName )
   {
      string filePath = path ;
      if (!filePath.empty() && filePath[filePath.length() - 1] != '/')
      {
         filePath += "/" ;
      }

      filePath += fileName ;
      return filePath ;
   }


   // logFileMgr
   void logFileMgr::push( const string &filePath )
   {
      ossScopedLock lock( &_mutex ) ;
      _logFileList.push_back( filePath ) ;
   }

   const string logFileMgr::pop()
   {
      string logFile = "" ;

      ossScopedLock lock( &_mutex ) ;
      if ( !_logFileList.empty() )
      {
         logFile = _logFileList.back() ;
         _logFileList.pop_back() ;
      }
      return logFile ;
   }

   BOOLEAN logFileMgr::empty()
   {
      ossScopedLock lock( &_mutex, SHARED ) ;
      return _logFileList.empty() ;
   }


   // resultInfo
   resultInfo::resultInfo()
   {
      reset() ;
   }

   resultInfo::~resultInfo()
   {
   }

   void resultInfo::fileNumInc()
   {
      _fileNum++ ;
   }

   void resultInfo::append( const resultInfo &info )
   {
      if ( DPS_INVALID_LSN_OFFSET == _startLSN || info._startLSN < _startLSN )
      {
         _startLSN = info._startLSN ;
      }

      if ( DPS_INVALID_LSN_OFFSET == _endLSN || info._endLSN > _endLSN )
      {
         _endLSN = info._endLSN ;
      }

      _fileNum += info._fileNum ;
      _parsedLogs += info._parsedLogs ;
      _parsedDocs += info._parsedDocs ;
      _parsedLobPieces += info._parsedLobPieces ;
      _revertedDocs += info._revertedDocs ;
      _revertedLobPieces += info._parsedLobPieces ;
   }

   void resultInfo::setStartLSN( const DPS_LSN_OFFSET &lsn )
   {
      _startLSN = lsn ;
   }

   void resultInfo::setEndLSN( const DPS_LSN_OFFSET &lsn )
   {
      _endLSN = lsn ;
   }

   void resultInfo::parsedLogInc()
   {
      _parsedLogs++ ;
   }

   void resultInfo::parsedDocInc()
   {
      _parsedDocs++ ;
   }

   void resultInfo::parsedLobPiecesInc()
   {
      _parsedLobPieces++ ;
   }

   void resultInfo::revertedDocInc()
   {
      _revertedDocs++ ;
   }

   void resultInfo::revertedLobPiecesInc()
   {
      _revertedLobPieces++ ;
   }

   void resultInfo::appendRevertedDoc( INT32 value )
   {
      _revertedDocs += value ;
   }

   void resultInfo::appendFailedLogFile( const string &logFileName )
   {
      _failedLogFiles.push_back( logFileName ) ;
   }

   void resultInfo::reset()
   {
      _fileNum           = 0 ;
      _startLSN          = DPS_INVALID_LSN_OFFSET ;
      _endLSN            = DPS_INVALID_LSN_OFFSET ;
      _parsedLogs        = 0 ;
      _parsedDocs        = 0 ;
      _parsedLobPieces   = 0 ;
      _revertedDocs      = 0 ;
      _revertedLobPieces = 0 ;
   }

   const string resultInfo::toString() const
   {
      stringstream ss ;
      DPS_LSN_OFFSET start = DPS_INVALID_LSN_OFFSET == _startLSN ? 0 : _startLSN ;
      DPS_LSN_OFFSET end   = DPS_INVALID_LSN_OFFSET == _endLSN ? 0 : _endLSN ;

      ss << "*************Result info**************" << endl ;
      ss << "Processed file num: " << _fileNum << endl ;
      ss << "LSN scope: [ " << start << ", " << end << " )" << endl ;
      ss << "Parsed log num: " << _parsedLogs << endl ;
      ss << "Parsed doc num: " << _parsedDocs << endl ;
      ss << "Parsed lob pieces num: " << _parsedLobPieces << endl ; 
      ss << "Reverted doc num: " << _revertedDocs << endl ; 
      ss << "Reverted lob pieces num: " << _revertedLobPieces << endl ;

      if ( _failedLogFiles.size() > 0 )
      {
         ss << "Revert failed log file: " << endl ;
         for ( size_t i = 0 ; i < _failedLogFiles.size() ; ++i )
         {
            ss << "  " << i + 1 << ". " <<  _failedLogFiles[i] << endl ;
         }
      }

      return ss.str() ;
   }


   // globalInfoMgr
   globalInfoMgr::globalInfoMgr()
   :_gloablRc( 0 ), _runNum( 0 )
   {
   }

   globalInfoMgr::~globalInfoMgr()
   {
   }

   void globalInfoMgr::appendResultInfo( const resultInfo &info )
   {
      ossScopedLock lock( &_mutex ) ;
      _resultInfo.append( info ) ;
   }

   void globalInfoMgr::appendFailedLogFile( const string &logFileName )
   {
      ossScopedLock lock( &_mutex ) ;
      _resultInfo.appendFailedLogFile( logFileName ) ;
   }

   void globalInfoMgr::printResultInfo()
   {
      string info ;

      ossScopedLock lock( &_mutex ) ;
      info = _resultInfo.toString() ;

      cout << info << endl ;
      PD_LOG( PDEVENT, info ) ;
   }

   void globalInfoMgr::setGlobalRc( INT32 rc )
   {
      _gloablRc.init( rc ) ;
   }

   INT32 globalInfoMgr::getGlobalRc()
   {
      return _gloablRc.fetch() ;
   }

   void globalInfoMgr::runNumInc()
   {
      _runNum.inc() ;
   }

   void globalInfoMgr::runNumDec()
   {
      _runNum.dec() ;
   }

   INT32 globalInfoMgr::getRunNum()
   {
      return _runNum.fetch() ;
   }
}