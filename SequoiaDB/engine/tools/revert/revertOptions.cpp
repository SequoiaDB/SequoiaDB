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

   Source File Name = revertOptions.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#include "revertOptions.hpp"
#include "revertCommon.hpp"
#include "utilPasswdTool.hpp"
#include "ossErr.h"
#include "../util/fromjson.hpp"
#include "utilStr.hpp"
#include "ossUtil.hpp"
#include <iostream>
#include <vector>

using namespace std ;

namespace sdbrevert
{
   // option name
   #define SDB_REVERT_OPTION_HELP              "help"
   #define SDB_REVERT_OPTION_VERSION           "version"
   #define SDB_REVERT_OPTION_HOSTNAME          "hostname"
   #define SDB_REVERT_OPTION_SVC               "svcname"
   #define SDB_REVERT_OPTION_HOSTS             "hosts"
   #define SDB_REVERT_OPTION_USER              "user"
   #define SDB_REVERT_OPTION_PASSWD            "password"
   #define SDB_REVERT_OPTION_CIPHERFile        "cipherfile"
   #define SDB_REVERT_OPTION_TOKEN             "token"
   #define SDB_REVERT_OPTION_SSL               "ssl"
   #define SDB_REVERT_OPTION_TARGET_CL         "targetcl"
   #define SDB_REVERT_OPTION_LOG_PATH          "logpath"
   #define SDB_REVERT_OPTION_DATA_TYPE         "datatype"
   #define SDB_REVERT_OPTION_MATCHER           "matcher"
   #define SDB_REVERT_OPTION_START_TIME        "starttime"
   #define SDB_REVERT_OPTION_END_TIME          "endtime"
   #define SDB_REVERT_OPTION_START_LSN         "startlsn"
   #define SDB_REVERT_OPTION_END_LSN           "endlsn"
   #define SDB_REVERT_OPTION_OUTPUT_CL         "output"
   #define SDB_REVERT_OPTION_LABEL             "label"
   #define SDB_REVERT_OPTION_JOBS              "jobs"
   #define SDB_REVERT_OPTION_STOP              "stop"
   #define SDB_REVERT_OPTION_FILL_LOB_HOLE     "fillhole"
   #define SDB_REVERT_OPTION_TMP_PATH          "tmppath"


   // option description
   #define SDB_REVERT_EXPLAIN_HELP              "print help information"
   #define SDB_REVERT_EXPLAIN_VERSION           "print version"
   #define SDB_REVERT_EXPLAIN_HOSTNAME          "host name, default: localhost"
   #define SDB_REVERT_EXPLAIN_SVC               "service name, default: 11810"
   #define SDB_REVERT_EXPLAIN_HOSTS             "host addresses(hostname:svcname), separated by ',', such as 'localhost:11810,localhost:11910'"
   #define SDB_REVERT_EXPLAIN_USER              "username"
   #define SDB_REVERT_EXPLAIN_PASSWD            "password"
   #define SDB_REVERT_EXPLAIN_CIPHERFile        "cipher file location, default ~/sequoiadb/passwd. If '--password' is specified, it will be used '--password' in preference"
   #define SDB_REVERT_EXPLAIN_TOKEN             "password encryption token"
   #define SDB_REVERT_EXPLAIN_SSL               "use SSL connection, default: false"
   #define SDB_REVERT_EXPLAIN_TARGET_CL         "the target collection for which data needs to be reverted"
   #define SDB_REVERT_EXPLAIN_LOG_PATH          "log path or log file, support archivelog and replicalog"
   #define SDB_REVERT_EXPLAIN_DATA_TYPE         "the type of data that needs to be reverted, the value is 'doc', 'lob', 'all', default is 'all'"
   #define SDB_REVERT_EXPLAIN_MATCHER           "conditions for filtering data, json formate. eg: { \"_id\": 1 }"
   #define SDB_REVERT_EXPLAIN_START_TIME        "start time to revert data, format: YYYY-MM-DD-HH:mm:ss, default is 1970-01-01-00:00:00"
   #define SDB_REVERT_EXPLAIN_END_TIME          "end time to revert data, format: YYYY-MM-DD-HH:mm:ss, default is 2037-12-31-23:59:59"
   #define SDB_REVERT_EXPLAIN_START_LSN         "start lsn to revert data, default is 0"
   #define SDB_REVERT_EXPLAIN_END_LSN           "end lsn to revert data"
   #define SDB_REVERT_EXPLAIN_OUTPUT_CL         "temporary collection to hold reverted data"
   #define SDB_REVERT_EXPLAIN_LABEL             "custom label to identify doc data"
   #define SDB_REVERT_EXPLAIN_JOBS              "the number of concurrent threads to revert data"
   #define SDB_REVERT_EXPLAIN_STOP              "whether the sdbrevert process stops executing when a single thread fails to execute, default is false, which means no stop"
   #define SDB_REVERT_EXPLAIN_FILL_LOB_HOLE     "fill lob holes with 0, default is false, which means no filling"
   #define SDB_REVERT_EXPLAIN_TMP_PATH          "the temporary path to store decompressed temporary archive log files"

   #define _TYPE(T) utilOptType(T)

   vector<string> passwdVec ;

   revertOptions::revertOptions()
   {
      _userName    = "" ;
      _password    = "" ;
      _useSSL      = FALSE ;
      _dataType    = SDB_REVERT_ALL ;
      _startTime   = 0 ;  // 1970-01-01
      _endTime     = 0 ;
      _startLSN    = 0 ;
      _endLSN      = OSS_UINT64_MAX ;
      _label       = "" ;
      _jobs        = 1 ;
      _stop        = FALSE ;
      _fillLobHole = FALSE ;
      _tmpPath     = "" ;
   }

   revertOptions::~revertOptions()
   {
   }

   INT32 revertOptions::parse( INT32 argc, CHAR* argv[] )
   {
      INT32 rc = SDB_OK ;

      addOptions( "Options" )
         ( SDB_REVERT_OPTION_HELP",h",       /* no arg */             SDB_REVERT_EXPLAIN_HELP )
         ( SDB_REVERT_OPTION_VERSION",V",    /* no arg */             SDB_REVERT_EXPLAIN_VERSION )
         ( SDB_REVERT_OPTION_HOSTNAME,       _TYPE(string ),          SDB_REVERT_EXPLAIN_HOSTNAME )
         ( SDB_REVERT_OPTION_SVC,            _TYPE(string ),          SDB_REVERT_EXPLAIN_SVC )
         ( SDB_REVERT_OPTION_HOSTS,          _TYPE(string ),          SDB_REVERT_EXPLAIN_HOSTS )
         ( SDB_REVERT_OPTION_USER,           _TYPE(string ),          SDB_REVERT_EXPLAIN_USER )
         ( SDB_REVERT_OPTION_PASSWD,         po::value< vector<string> >(&passwdVec)->multitoken()->zero_tokens(),    SDB_REVERT_EXPLAIN_PASSWD )
         ( SDB_REVERT_OPTION_CIPHERFile,     _TYPE( string ),         SDB_REVERT_EXPLAIN_CIPHERFile )
         ( SDB_REVERT_OPTION_TOKEN ,         _TYPE( string ),         SDB_REVERT_EXPLAIN_TOKEN )
         ( SDB_REVERT_OPTION_SSL,            _TYPE( string ),        SDB_REVERT_EXPLAIN_SSL )
         ( SDB_REVERT_OPTION_TARGET_CL,      _TYPE( string ),         SDB_REVERT_EXPLAIN_TARGET_CL )
         ( SDB_REVERT_OPTION_LOG_PATH,       _TYPE( string ),         SDB_REVERT_EXPLAIN_LOG_PATH )
         ( SDB_REVERT_OPTION_DATA_TYPE,      _TYPE( string ),         SDB_REVERT_EXPLAIN_DATA_TYPE )
         ( SDB_REVERT_OPTION_MATCHER,        _TYPE( string ),         SDB_REVERT_EXPLAIN_MATCHER )
         ( SDB_REVERT_OPTION_START_TIME,     _TYPE( string ),         SDB_REVERT_EXPLAIN_START_TIME )
         ( SDB_REVERT_OPTION_END_TIME,       _TYPE( string ),         SDB_REVERT_EXPLAIN_END_TIME )
         ( SDB_REVERT_OPTION_START_LSN,      _TYPE( DPS_LSN_OFFSET ), SDB_REVERT_EXPLAIN_START_LSN )
         ( SDB_REVERT_OPTION_END_LSN ,       _TYPE( DPS_LSN_OFFSET ), SDB_REVERT_EXPLAIN_END_LSN )
         ( SDB_REVERT_OPTION_OUTPUT_CL,      _TYPE( string ),         SDB_REVERT_EXPLAIN_OUTPUT_CL )
         ( SDB_REVERT_OPTION_LABEL,          _TYPE( string ),         SDB_REVERT_EXPLAIN_LABEL )
         ( SDB_REVERT_OPTION_JOBS,           _TYPE( INT32 ),          SDB_REVERT_EXPLAIN_JOBS )
         ( SDB_REVERT_OPTION_STOP,           _TYPE( string ),         SDB_REVERT_EXPLAIN_STOP )
         ( SDB_REVERT_OPTION_FILL_LOB_HOLE,  _TYPE( string ),         SDB_REVERT_EXPLAIN_FILL_LOB_HOLE )
         ( SDB_REVERT_OPTION_TMP_PATH,       _TYPE( string ),         SDB_REVERT_EXPLAIN_TMP_PATH )
         ;

      rc = engine::utilOptions::parse( argc, argv ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( has( SDB_REVERT_OPTION_HELP ) ||
           has( SDB_REVERT_OPTION_VERSION ) )
      {
         goto done ;
      }

      rc = _setOptions( argc ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _check() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN revertOptions::hasHelp()
   {
      return has(SDB_REVERT_OPTION_HELP) ;
   }

   BOOLEAN revertOptions::hasVersion()
   {
      return has(SDB_REVERT_OPTION_VERSION) ;
   }

   void revertOptions::printHelpInfo()
   {
      print() ;
   }

   INT32 revertOptions::_setOptions( INT32 argc )
   {
      INT32 rc        = SDB_OK ;
      string hostName = "localhost" ;
      string svcName  = "11810" ;
      string tmpStr = "" ;

      if ( has( SDB_REVERT_OPTION_HOSTNAME ) )
      {
         hostName = get<string>( SDB_REVERT_OPTION_HOSTNAME ) ;
      }

      if ( has( SDB_REVERT_OPTION_SVC ) )
      {
         svcName = get<string>( SDB_REVERT_OPTION_SVC ) ;
      }

      if ( has( SDB_REVERT_OPTION_HOSTS ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_HOSTS ) ;
         _hostList = splitString( tmpStr, SDB_REVERT_PARAM_DELIMITER ) ;

         if ( has( SDB_REVERT_OPTION_HOSTNAME ) || has( SDB_REVERT_OPTION_SVC ) )
         {
            tmpStr = hostName + ":" + svcName ;
            _hostList.push_back( tmpStr ) ;
         }
      }
      else
      {
         tmpStr = hostName + ":" + svcName ;
         _hostList.push_back( tmpStr ) ;
      }

      if ( has( SDB_REVERT_OPTION_USER ) )
      {
         _userName = get<string>( SDB_REVERT_OPTION_USER ) ;

         if ( has( SDB_REVERT_OPTION_PASSWD ) )
         {
            string  passwd ;
            BOOLEAN isNormalInput = FALSE ;

            if ( 0 == passwdVec.size() )
            {
               isNormalInput = utilPasswordTool::interactivePasswdInput( passwd ) ;
            }
            else
            {
               isNormalInput = TRUE ;
               passwd = passwdVec[0] ;
            }

            if ( !isNormalInput )
            {
               rc = SDB_APP_INTERRUPT ;
               cerr << getErrDesp( rc ) << ", rc= " << rc << endl ;
               goto error ;
            }

            _password = passwd ;
         }
         else if ( has( SDB_REVERT_OPTION_CIPHERFile ) )
         {
            utilPasswordTool passwdTool ;

            _cipherfile = get<string>( SDB_REVERT_OPTION_CIPHERFile ) ;

            if ( has( SDB_REVERT_OPTION_TOKEN ) )
            {
               _token = get<string>( SDB_REVERT_OPTION_TOKEN ) ;
            }

            rc = passwdTool.getPasswdByCipherFile( _userName, _token,
                                                   _cipherfile,
                                                   _password ) ;
            if ( SDB_OK != rc )
            {
               cerr << "Failed to get user[" << _userName.c_str()
                           << "] password from cipher file"
                           << "[" << _cipherfile.c_str() << "], rc= " << rc
                           << endl ;
               PD_LOG( PDERROR, "Failed to get user[%s] password from cipher"
                        " file[%s], rc: %d", _userName.c_str(),
                        _cipherfile.c_str(), rc ) ;
               goto error ;
            }
            _userName = utilGetUserShortNameFromUserFullName( _userName ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            cerr << "No password or cipher file specified, rc= " << rc << endl ;
            goto error ;
         }
      }
      else if ( has( SDB_REVERT_OPTION_PASSWD ) ||  has( SDB_REVERT_OPTION_CIPHERFile ) )
      {
            rc = SDB_INVALIDARG ;
            cerr << "No user name specified, rc= " << rc << endl ;
            goto error ;
      }

      if ( has( SDB_REVERT_OPTION_SSL ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_SSL ) ;
         ossStrToBoolean( tmpStr.c_str(), &_useSSL ) ;
      }

      if ( has( SDB_REVERT_OPTION_TARGET_CL ) )
      {
         _targetClName = get<string>( SDB_REVERT_OPTION_TARGET_CL ) ;
      }

      if ( has( SDB_REVERT_OPTION_LOG_PATH ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_LOG_PATH ) ;
         _logPathList = splitString( tmpStr, SDB_REVERT_PARAM_DELIMITER ) ;
      }

      if ( has( SDB_REVERT_OPTION_DATA_TYPE ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_DATA_TYPE ) ;

         if ( "doc" == tmpStr )
         {
            _dataType = SDB_REVERT_DOC ;
         }
         else if ( "lob" == tmpStr )
         {
            _dataType = SDB_REVERT_LOB ;
         }
         else if ( "all" == tmpStr )
         {
            _dataType = SDB_REVERT_ALL ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            cerr << "Invalid argument: '--" << SDB_REVERT_OPTION_DATA_TYPE << "', value= " << tmpStr << endl ;
            goto error ;
         }
      }

      if ( has( SDB_REVERT_OPTION_MATCHER ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_MATCHER ) ;
         rc = bson::fromjson( tmpStr, _matcher ) ;
         if (SDB_OK != rc)
         {
            rc = SDB_INVALIDARG ;
            cerr << "Invalid argument: '--" << SDB_REVERT_OPTION_MATCHER << "', value= " << tmpStr << endl ;
            goto error ;
         }
      }

      if ( has( SDB_REVERT_OPTION_START_TIME ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_START_TIME ) ;
         rc = engine::utilStr2TimeT( tmpStr.c_str(), _startTime, NULL ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            cerr << "Invalid argument: '--" << SDB_REVERT_OPTION_START_TIME
                 << "', format= " << "YYYY-MM-DD-HH:mm:ss"
                 << ", value= " << tmpStr << endl ;
            goto error ;
         }
      }

      if ( has( SDB_REVERT_OPTION_END_TIME ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_END_TIME ) ;
         rc = engine::utilStr2TimeT( tmpStr.c_str(), _endTime, NULL ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            cerr << "Invalid argument: '--" << SDB_REVERT_OPTION_END_TIME
                 << "', format= " << "YYYY-MM-DD-HH:mm:ss"
                 << ", value= " << tmpStr << endl ;
            goto error ;
         }
      }
      else
      {
         tmpStr = SDB_REVERT_MAX_TIME ;
         rc = engine::utilStr2TimeT( tmpStr.c_str(), _endTime, NULL ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            cerr << "Error default value=" << SDB_REVERT_MAX_TIME
                 << " for " << SDB_REVERT_OPTION_END_TIME << endl ;
            goto error ;
         }
      }

      if ( has( SDB_REVERT_OPTION_START_LSN ) )
      {
         _startLSN = get<DPS_LSN_OFFSET>( SDB_REVERT_OPTION_START_LSN ) ;
      }

      if ( has( SDB_REVERT_OPTION_END_LSN ) )
      {
         _endLSN = get<DPS_LSN_OFFSET>( SDB_REVERT_OPTION_END_LSN ) ;
      }

      if ( has( SDB_REVERT_OPTION_OUTPUT_CL ) )
      {
         _outputClName = get<string>( SDB_REVERT_OPTION_OUTPUT_CL ) ;
      }

      if ( has( SDB_REVERT_OPTION_LABEL ) )
      {
         _label = get<string>( SDB_REVERT_OPTION_LABEL ) ;
      }

      if ( has( SDB_REVERT_OPTION_JOBS ) )
      {
         _jobs = get<INT32>( SDB_REVERT_OPTION_JOBS ) ;
      }

      if ( has( SDB_REVERT_OPTION_STOP ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_STOP ) ;
         ossStrToBoolean( tmpStr.c_str(), &_stop ) ;
      }

      if ( has( SDB_REVERT_OPTION_FILL_LOB_HOLE ) )
      {
         tmpStr = get<string>( SDB_REVERT_OPTION_FILL_LOB_HOLE ) ;
         ossStrToBoolean( tmpStr.c_str(), &_fillLobHole ) ;
      }

      if ( has( SDB_REVERT_OPTION_TMP_PATH ) )
      {
         _tmpPath = get<string>( SDB_REVERT_OPTION_TMP_PATH ) ;
      }
      
   done:
      return rc ;
   error:
      goto done ;   
   }

   INT32 revertOptions::_check()
   {
      INT32 rc                                          = SDB_OK ;
      CHAR startTimeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { '\0' } ;
      CHAR endTimeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ]   = { '\0' } ;

      if ( _targetClName.empty() )
      {
         rc = SDB_INVALIDARG ;
         cerr << "Missing required parameter '--" << SDB_REVERT_OPTION_TARGET_CL << "'" << endl ;
         goto error ;
      }

      if ( _outputClName.empty() )
      {
         rc = SDB_INVALIDARG ;
         cerr << "Missing required parameter '--" << SDB_REVERT_OPTION_OUTPUT_CL << "'" << endl ;
         goto error ;
      }

      if ( _logPathList.empty() )
      {
         rc = SDB_INVALIDARG ;
         cerr << "Missing required parameter '--" << SDB_REVERT_OPTION_LOG_PATH << "'" << endl ;
         goto error ;
      }

      engine::utilAscTime( _startTime, startTimeStr, OSS_TIMESTAMP_STRING_LEN ) ;
      engine::utilAscTime( _endTime, endTimeStr, OSS_TIMESTAMP_STRING_LEN ) ;
      if ( _startTime >= _endTime )
      {
         rc = SDB_INVALIDARG ;
         cerr << "Start time '" << startTimeStr << "' greater than or equal to end time '"
              << endTimeStr << "'" << endl ;
         goto error ;
      }

      if ( _startLSN >= _endLSN )
      {
         rc = SDB_INVALIDARG ;
         cerr << "Start lsn '" << _startLSN << "' greater than or equal to end lsn '"
              << _endLSN << "'" << endl ;
         goto error ;
      }

      if ( _jobs < 1 )
      {
         rc = SDB_INVALIDARG ;
         cerr << "Invalid parameter: '--" << SDB_REVERT_OPTION_JOBS << "', value= " << _jobs << endl ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;   
   }
}
