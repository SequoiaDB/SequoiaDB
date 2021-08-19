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

   Source File Name = rplDB2LoadOutputter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rplDB2LoadOutputter.hpp"
#include "rplConfParser.hpp"
#include "rplConfDef.hpp"
#include "rplUtil.hpp"
#include "ossFile.hpp"
#include "ossPath.hpp"
#include "ossUtil.hpp"
#include "pcrecpp.h"
#include "dms.hpp"
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

using namespace std ;
using namespace engine ;
using namespace bson ;
namespace fs = boost::filesystem ;

namespace replay
{
   const CHAR RPL_DB2LOAD_OP_INSERT[] = "I" ;
   const CHAR RPL_DB2LOAD_OP_DELETE[] = "D" ;
   const CHAR RPL_DB2LOAD_OP_UPDATE_BEFORE[] = "B" ;
   const CHAR RPL_DB2LOAD_OP_UPDATE_AFTER[] = "A" ;

   static INT32 _ossFilterFiles( const string &dirPath, vector<string> &vecFiles,
                                 pcrecpp::RE &re, UINT32 deep ) ;

   rplDB2LoadOutputter::rplDB2LoadOutputter( Monitor *monitor )
   {
      _monitor = monitor ;
      _recordWriter = NULL ;
      _outputHour = 21 ;
      _outputMinute = 0 ;
      _submitInterval = -1 ;
      _delimiter = RPL_CONF_DEFAULT_DELIMITER ;
   }

   rplDB2LoadOutputter::~rplDB2LoadOutputter()
   {
      SAFE_OSS_DELETE( _recordWriter ) ;
      _monitor = NULL ;
   }

   INT32 rplDB2LoadOutputter::init( const CHAR *confFile )
   {
      INT32 rc = SDB_OK ;
      if ( NULL == confFile )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "OutputDir is NULL, rc = %d", rc ) ;
      }

      if ( NULL == _monitor )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Monitor is NULL, rc = %d", rc ) ;
      }

      rc = _parseConf( confFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse conf file(%s), rc = %d",
                   confFile, rc ) ;

      _recordWriter = SDB_OSS_NEW rplRecordWriter( _monitor, _outputDir.c_str(),
                                                   _prefix.c_str(),
                                                   _suffix.c_str() ) ;
      if ( NULL == _recordWriter )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to new rplRecordWriter, rc = %d", rc ) ;
         goto error ;
      }

      rc = _recordWriter->init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init _recordWriter, rc = %d", rc ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _recordWriter ) ;
      goto done ;
   }

   INT32 rplDB2LoadOutputter::_parseConf( const CHAR *confFile )
   {
      INT32 rc = SDB_OK ;
      string submitTime ;
      BSONElement submitIntervalEle ;
      BSONObj conf ;
      rplConfParser confParser ;

      rc = confParser.init( confFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse conf file(%s), rc = %d",
                   confFile, rc ) ;

      if ( getType() != confParser.getType() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Config type(%s) is not %s, rc = %d",
                      confParser.getType().c_str(), getType().c_str(), rc ) ;
      }

      conf = confParser.getConf() ;
      _outputDir = conf.getStringField( RPL_CONF_NAME_OUTPUT_DIR ) ;
      if ( _outputDir.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "OutputDir(%s) is empty, rc = %d",
                      _outputDir.c_str(), rc ) ;
      }

      _prefix = conf.getStringField( RPL_CONF_NAME_PREFIX ) ;
      _suffix = conf.getStringField( RPL_CONF_NAME_SUFFIX ) ;
      if ( _prefix.empty() && _suffix.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Configure %s or %s must exist at least "
                      "one configure, rc = %d", RPL_CONF_NAME_PREFIX,
                      RPL_CONF_NAME_SUFFIX, rc ) ;
      }

      _delimiter = conf.getStringField( RPL_CONF_NAME_DELIMITER ) ;
      if ( _delimiter.empty() )
      {
         _delimiter = RPL_CONF_DEFAULT_DELIMITER ;
      }

      submitTime = conf.getStringField( RPL_CONF_SUBMIT_TIME ) ;
      submitIntervalEle = conf.getField( RPL_CONF_SUBMIT_INTERVAL ) ;
      if ( !submitIntervalEle.eoo() )
      {
         PD_CHECK( submitIntervalEle.type() == NumberInt
                   || submitIntervalEle.type() == NumberLong , SDB_INVALIDARG,
                   error, PDERROR, "%s must be int or long format",
                   RPL_CONF_SUBMIT_INTERVAL ) ;

         _submitInterval = submitIntervalEle.numberLong() ;
         PD_CHECK( _submitInterval > 0, SDB_INVALIDARG, error, PDERROR,
                   "%s must be greater 0", RPL_CONF_SUBMIT_INTERVAL ) ;
         PD_LOG( PDEVENT, "Output File every %lld munites", _submitInterval ) ;

         _submitInterval = _submitInterval * 60 * 1000000L ;
      }
      else if ( !submitTime.empty() )
      {

         rc = _parseSubmitTime( submitTime.c_str() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse submitTime(%s), rc = %d",
                      submitTime.c_str(), rc ) ;

         PD_LOG( PDEVENT, "Output File every %d:%d per day", _outputHour,
                 _outputMinute ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "%s or %s must be specified, rc = %d",
                      RPL_CONF_SUBMIT_TIME, RPL_CONF_SUBMIT_INTERVAL, rc ) ;
      }

      rc = _tableMapping.init( conf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse conf, rc = %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplDB2LoadOutputter::_parseSubmitTime( const CHAR *submitTime )
   {
      static const INT32 TIME_LEN = 5 ;

      INT32 rc = SDB_OK ;
      CHAR *p = NULL ;
      CHAR tempTime[ TIME_LEN + 1 ] = "" ;

      if ( ( INT32 )ossStrlen( submitTime ) != TIME_LEN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossStrcpy( tempTime, submitTime ) ;
      tempTime[TIME_LEN] = '\0' ;

      //TODO: ubuntu compile error because of redefine strchr in cstring
      p = ossStrchr( (CHAR *)submitTime, ':' ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      *p = '\0' ;
      _outputHour = ossAtoi( tempTime ) ;
      if ( _outputHour < 0 || _outputHour > 23 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _outputMinute = ossAtoi( p + 1 ) ;
      if ( _outputMinute < 0 || _outputMinute > 59 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string rplDB2LoadOutputter::getType()
   {
      return RPL_OUTPUT_DB2LOAD ;
   }

   INT32 rplDB2LoadOutputter::insertRecord( const CHAR *clFullName, UINT64 lsn,
                                            const BSONObj &obj,
                                            const UINT64 &opTimeMicroSecond )
   {
      INT32 rc = SDB_OK ;
      string strOut ;
      const CHAR *dbName = NULL ;
      const CHAR *tableName = NULL ;

      rc = _generateRecord( clFullName, RPL_DB2LOAD_OP_INSERT,
                            opTimeMicroSecond, obj,
                            &dbName, &tableName, strOut ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate insert record(%s), rc = %d",
                   obj.toString().c_str(), rc ) ;

      rc = _recordWriter->writeRecord( dbName, tableName, lsn, strOut.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write record(%s), rc = %d",
                   strOut.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplDB2LoadOutputter::_generateRecord( const CHAR *clFullName,
                                               const CHAR *OP,
                                               const UINT64 &opTimeMicroSecond,
                                               const BSONObj &objIn,
                                               const CHAR **outDBName,
                                               const CHAR **outTableName,
                                               string &strOut )
   {
      INT32 rc = SDB_OK ;
      FIELD_VECTOR * fieldVector = NULL ;
      stringstream ss ;
      rplFieldMapping *fieldMapping = NULL ;
      FIELD_VECTOR::iterator iter ;

      fieldMapping = _tableMapping.getFieldMapping( clFullName ) ;
      if ( NULL == fieldMapping )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Tablemapping(%s) is not exist, rc = %d",
                      clFullName, rc ) ;
      }

      fieldVector = fieldMapping->getFieldVector() ;
      iter = fieldVector->begin() ;
      while ( iter != fieldVector->end() )
      {
         if ( iter != fieldVector->begin() )
         {
            ss << _delimiter ;
         }

         rplField *info = *iter ;

         if ( info->isNeedDoubleQuote() )
         {
            ss << "\"" ;
         }

         if ( info->getFieldType() == AUTO_OP )
         {
            ss << OP ;
         }
         else if ( info->getFieldType() == ORIGINAL_TIME )
         {
            string timeStr ;
            ossTimestamp timestamp = ossMicrosecondsToTimestamp(
                                                          opTimeMicroSecond ) ;
            rplTimestampToString( timestamp, timeStr ) ;
            ss << timeStr ;
         }
         else
         {
            string value ;
            try
            {
               rc = info->getValue( objIn, value ) ;
            }
            catch( std::exception& e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
               goto error ;
            }

            PD_RC_CHECK( rc, PDERROR, "Failed to get value(%s), rc = %d",
                         objIn.toString().c_str(), rc ) ;
            ss << value ;
         }

         if ( info->isNeedDoubleQuote() )
         {
            ss << "\"" ;
         }

         iter++ ;
      }

      *outDBName = fieldMapping->getTargetDBName() ;
      *outTableName = fieldMapping->getTargetTableName() ;
      strOut = ss.str() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplDB2LoadOutputter::deleteRecord( const CHAR *clFullName, UINT64 lsn,
                                            const BSONObj &oldObj,
                                            const UINT64 &opTimeMicroSecond )
   {
      INT32 rc = SDB_OK ;
      string strOut ;
      const CHAR *dbName = NULL ;
      const CHAR *tableName = NULL ;

      rc = _generateRecord( clFullName, RPL_DB2LOAD_OP_DELETE,
                            opTimeMicroSecond, oldObj, &dbName, &tableName,
                            strOut ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate insert record(%s), rc = %d",
                   oldObj.toString().c_str(), rc ) ;

      rc = _recordWriter->writeRecord( dbName, tableName, lsn, strOut.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write record(%s), rc = %d",
                   strOut.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplDB2LoadOutputter::updateRecord( const CHAR *clFullName, UINT64 lsn,
                                            const BSONObj &matcher,
                                            const BSONObj &newModifier,
                                            const BSONObj &shardingKey,
                                            const BSONObj &oldModifier,
                                            const UINT64 &opTimeMicroSecond,
                                            const UINT32 &logWriteMod )
   {
      INT32 rc = SDB_OK ;
      string strOut ;
      const CHAR *dbName = NULL ;
      const CHAR *tableName = NULL ;

      if ( DMS_LOG_WRITE_MOD_FULL != logWriteMod )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "LogWriteMod(%d) is invalid, rc = %d",
                      logWriteMod, rc ) ;
      }

      rc = _generateRecord( clFullName, RPL_DB2LOAD_OP_UPDATE_BEFORE,
                            opTimeMicroSecond, oldModifier, &dbName, &tableName,
                            strOut ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate update record(%s), rc = %d",
                   oldModifier.toString().c_str(), rc ) ;

      rc = _recordWriter->writeRecord( dbName, tableName, lsn, strOut.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write record(%s), rc = %d",
                   strOut.c_str(), rc ) ;

      rc = _generateRecord( clFullName, RPL_DB2LOAD_OP_UPDATE_AFTER,
                            opTimeMicroSecond, newModifier, &dbName, &tableName,
                            strOut ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate update record(%s), rc = %d",
                   newModifier.toString().c_str(), rc ) ;

      rc = _recordWriter->writeRecord( dbName, tableName, lsn, strOut.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write record(%s), rc = %d",
                   strOut.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplDB2LoadOutputter::truncateCL( const CHAR *clFullName, UINT64 lsn )
   {
      PD_LOG( PDERROR, "Truncate cl(%s) is not supported", clFullName ) ;
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 rplDB2LoadOutputter::pop( const CHAR *clFullName, UINT64 lsn,
                                   INT64 logicalID, INT8 direction )
   {
      PD_LOG( PDERROR, "pop cl(%s) is not supported", clFullName ) ;
      return SDB_OPTION_NOT_SUPPORT ;
   }

   INT32 rplDB2LoadOutputter::flush()
   {
      INT32 rc = SDB_OK ;

      rc = _recordWriter->flush() ;
      PD_RC_CHECK( rc, PDERROR, "Failed flush writer, rc = %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplDB2LoadOutputter::getExtraStatus( BSONObj &status )
   {
      return _recordWriter->getStatus( status ) ;
   }

   BOOLEAN rplDB2LoadOutputter::_isNeedSumitPerDay()
   {
      UINT64 currentTime = ossGetCurrentMicroseconds() ;
      UINT64 lastSubmitTime = _monitor->getSubmitTime() ;

      if ( isSameDay( currentTime, lastSubmitTime ) )
      {
         // today have already submitted.
         return FALSE ;
      }

      UINT64 checkTime = replaceAndGetTime( currentTime, _outputHour,
                                            _outputMinute, 0 ) ;
      if ( checkTime <= currentTime )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN rplDB2LoadOutputter::isNeedSubmit()
   {
      if ( _submitInterval <= 0 )
      {
         return _isNeedSumitPerDay() ;
      }
      else
      {
         UINT64 lastSubmitTime = _monitor->getSubmitTime() ;
         UINT64 currentTime = ossGetCurrentMicroseconds() ;
         if ( currentTime - lastSubmitTime > (UINT64)_submitInterval )
         {
            return TRUE ;
         }

         return FALSE ;
      }
   }

   INT32 rplDB2LoadOutputter::submit()
   {
      INT32 rc = SDB_OK ;
      UINT64 currentTime = 0 ;

      rc = _recordWriter->submit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed flush writer, rc = %d", rc ) ;

      currentTime = ossGetCurrentMicroseconds() ;
      _monitor->setSubmitTime( currentTime ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _ossFilterFiles( const string &dirPath, vector<string> &vecFiles,
                          pcrecpp::RE &re, UINT32 deep )
   {
      INT32 rc = SDB_OK ;

      try
      {
         fs::path dbDir ( dirPath ) ;
         fs::directory_iterator end_iter ;

         if ( 0 == deep )
         {
            goto done ;
         }

         if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
         {
            for ( fs::directory_iterator dir_iter ( dbDir );
                  dir_iter != end_iter; ++dir_iter )
            {
               try
               {
                  if ( fs::is_regular_file ( dir_iter->status() ) )
                  {
                     const std::string fileName =
                        dir_iter->path().filename().string() ;
                     if ( re.FullMatch( fileName ) )
                     {
                        vecFiles.push_back( dir_iter->path().string() ) ;
                     }
                  }
                  else if ( fs::is_directory( dir_iter->path() ) && deep > 1 )
                  {
                     _ossFilterFiles( dir_iter->path().string(), vecFiles,
                                      re, deep - 1 ) ;
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDWARNING, "File or dir[%s] occur exception: %s",
                          dir_iter->path().string().c_str(),
                          e.what() ) ;
                  /// skip the file or dir
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

