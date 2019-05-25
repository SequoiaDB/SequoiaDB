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

   Source File Name = rplOptions.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplOptions.hpp"
#include "../util/fromjson.hpp"
#include "ossUtil.hpp"
#include "ossFile.hpp"
#include "utilStr.hpp"
#include "pd.hpp"
#include <iostream>

namespace replay
{
   #define RPL_OPTION_HELP              "help"
   #define RPL_OPTION_VERSION           "version"
   #define RPL_OPTION_HOST              "hostname"
   #define RPL_OPTION_SVC               "svcname"
   #define RPL_OPTION_USER              "user"
   #define RPL_OPTION_PASSWD            "password"
   #define RPL_OPTION_SSL               "ssl"
   #define RPL_OPTION_PATH              "path"
   #define RPL_OPTION_FILTER            "filter"
   #define RPL_OPTION_DUMP              "dump"
   #define RPL_OPTION_DUMPHEADER        "dumpheader"
   #define RPL_OPTION_DELETE            "delete"
   #define RPL_OPTION_WATCH             "watch"
   #define RPL_OPTION_DAEMON            "daemon"
   #define RPL_OPTION_STATUS            "status"
   #define RPL_OPTION_HELPFULL          "helpfull"
   #define RPL_OPTION_DEBUG             "debug"
   #define RPL_OPTION_DEFLATE           "deflate"
   #define RPL_OPTION_INFLATE           "inflate"
   #define RPL_OPTION_TYPE              "type"
   #define RPL_OPTION_UPDATE_WITH_SHARING_KEY "updatewithshardingkey"

   #define RPL_EXPLAIN_HELP             "print help information"
   #define RPL_EXPLAIN_VERSION          "print version"
   #define RPL_EXPLAIN_HOST             "sequoiadb hostname"
   #define RPL_EXPLAIN_SVC              "service name"
   #define RPL_EXPLAIN_USER             "username"
   #define RPL_EXPLAIN_PASSWD           "password"
   #define RPL_EXPLAIN_SSL              "use SSL connection (arg: [true|false], e.g. --ssl true), default: false"
   #define RPL_EXPLAIN_PATH             "archive or replica log directory or file path"
   #define RPL_EXPLAIN_FILTER           "log filtering rule, " \
                                        "e.g. --filter '{\"OP\": [\"insert\", \"update\"]}'"
   #define RPL_EXPLAIN_DUMP             "dump log only, default is false"
   #define RPL_EXPLAIN_DUMPHEADER       "dump archive header, default is false"
   #define RPL_EXPLAIN_DELETE           "delete log file after replay, " \
                                        "default is false"
   #define RPL_EXPLAIN_WATCH            "continuously watch path and replay log files, " \
                                        "valid when path is directory, default is false"
   #define RPL_EXPLAIN_DAEMON           "run in background, default is false, " \
                                        "the background process can be stopped elegantly " \
                                        "by \"kill -15 <pid>\""
   #define RPL_EXPLAIN_STATUS           "specify status file, " \
                                        "file will be created if it not exists, " \
                                        "replay will start according to the status if it exists"
   #define RPL_EXPLAIN_HELPFULL         "print all options"
   #define RPL_EXPLAIN_DEBUG            "log debug info"
   #define RPL_EXPLAIN_DEFLATE          "compress archive file, valid when path is file"
   #define RPL_EXPLAIN_INFLATE          "uncompress archive file, valid when path is file"
   #define RPL_EXPLAIN_TYPE             "indicate the type of file, " \
                                        "the value can be \"archive\" or \"replica\", " \
                                        "default is \"archive\""
   #define RPL_EXPLAIN_UPDATE_WITH_SHARDING_KEY "update record with sharding key when it exists, default is true"

   #define RPL_OPTION_TYPE_ARCHIVE      "archive"
   #define RPL_OPTION_TYPE_REPLICA      "replica"

   #define _TYPE(T) utilOptType(T)

   Options::Options()
   {
      _pathType = SDB_OSS_UNK;
      _useSSL = FALSE;
      _dump = FALSE;
      _dumpHeader = FALSE;
      _delete = FALSE;
      _watch = FALSE;
      _daemon = FALSE;
      _debug = FALSE;
      _deflate = FALSE;
      _inflate = FALSE;
      _isReplicaFile = FALSE;
      _updateWithShardingKey = TRUE;
   }

   Options::~Options()
   {
   }

   INT32 Options::parse(INT32 argc, CHAR* argv[])
   {
      INT32 rc = SDB_OK;

      addOptions("General Options")
         (RPL_OPTION_HELP",h",       /* no arg */     RPL_EXPLAIN_HELP)
         (RPL_OPTION_VERSION",V",    /* no arg */     RPL_EXPLAIN_VERSION)
         (RPL_OPTION_HOST,          _TYPE(string),    RPL_EXPLAIN_HOST)
         (RPL_OPTION_SVC,           _TYPE(string),    RPL_EXPLAIN_SVC)
         (RPL_OPTION_USER,          _TYPE(string),    RPL_EXPLAIN_USER)
         (RPL_OPTION_PASSWD,        _TYPE(string),    RPL_EXPLAIN_PASSWD)
         (RPL_OPTION_SSL,           _TYPE(string),    RPL_EXPLAIN_SSL)
         (RPL_OPTION_PATH,          _TYPE(string),    RPL_EXPLAIN_PATH)
         (RPL_OPTION_FILTER,        _TYPE(string),    RPL_EXPLAIN_FILTER)
         (RPL_OPTION_DUMP,          _TYPE(string),    RPL_EXPLAIN_DUMP)
         (RPL_OPTION_DUMPHEADER,    _TYPE(string),    RPL_EXPLAIN_DUMPHEADER)
         (RPL_OPTION_DELETE,        _TYPE(string),    RPL_EXPLAIN_DELETE)
         (RPL_OPTION_WATCH,         _TYPE(string),    RPL_EXPLAIN_WATCH)
         (RPL_OPTION_DAEMON,        _TYPE(string),    RPL_EXPLAIN_DAEMON)
         (RPL_OPTION_STATUS,        _TYPE(string),    RPL_EXPLAIN_STATUS)
         (RPL_OPTION_TYPE,          _TYPE(string),    RPL_EXPLAIN_TYPE)
      ;

      addOptions("Helpfull Options", TRUE)
         (RPL_OPTION_HELPFULL,       /* no arg */     RPL_EXPLAIN_HELPFULL)
         (RPL_OPTION_DEBUG,          /* no arg */     RPL_EXPLAIN_DEBUG)
         (RPL_OPTION_DEFLATE,       _TYPE(string),    RPL_EXPLAIN_DEFLATE)
         (RPL_OPTION_INFLATE,       _TYPE(string),    RPL_EXPLAIN_INFLATE)
         (RPL_OPTION_UPDATE_WITH_SHARING_KEY, _TYPE(string), RPL_EXPLAIN_UPDATE_WITH_SHARDING_KEY)
      ;

      rc = engine::utilOptions::parse(argc, argv);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (has(RPL_OPTION_HELP) ||
          has(RPL_OPTION_VERSION) ||
          has(RPL_OPTION_HELPFULL))
      {
         goto done;
      }

      rc = setOptions() ;
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void Options::printHelp()
   {
      print();
   }

   void Options::printHelpfull()
   {
      print(TRUE);
   }

   BOOLEAN Options::hasHelp()
   {
      return has(RPL_OPTION_HELP);
   }

   BOOLEAN Options::hasVersion()
   {
      return has(RPL_OPTION_VERSION);
   }

   BOOLEAN Options::hasHelpfull()
   {
      return has(RPL_OPTION_HELPFULL);
   }

   string Options::buildPrintableCmd(INT32 argc, CHAR* argv[])
   {
      stringstream ss;

      for (INT32 i = 0; i < argc; i++)
      {
         if (argv[i] == string("--"RPL_OPTION_PASSWD))
         {
            i++; // ignore password
            continue;
         }

         if (i > 0 && argv[i - 1] == string("--"RPL_OPTION_FILTER))
         {
            ss << "'" << argv[i] << "'" << " ";
         }
         else
         {
            ss << argv[i] << " ";
         }
      }

      return ss.str();
   }

   string Options::buildBackgroundCmd(INT32 argc, CHAR* argv[])
   {
      stringstream ss;

      for (INT32 i = 0; i < argc; i++)
      {
         if (argv[i] == string("--"RPL_OPTION_DAEMON))
         {
            i++; // ignore daemon value
            continue;
         }

         if (i > 0 && argv[i - 1] == string("--"RPL_OPTION_FILTER))
         {
            ss << "'" << argv[i] << "'" << " ";
         }
         else
         {
            ss << argv[i] << " ";
         }
      }

      return ss.str();
   }

   INT32 Options::setOptions()
   {
      INT32 rc = SDB_OK;

      if (has(RPL_OPTION_DUMP))
      {
         string dump = get<string>(RPL_OPTION_DUMP);
         ossStrToBoolean(dump.c_str(), &_dump);
      }

      if (has(RPL_OPTION_DUMPHEADER))
      {
         string dumpHeader = get<string>(RPL_OPTION_DUMPHEADER);
         ossStrToBoolean(dumpHeader.c_str(), &_dumpHeader);
      }

      if (has(RPL_OPTION_DEFLATE))
      {
         string deflate = get<string>(RPL_OPTION_DEFLATE);
         ossStrToBoolean(deflate.c_str(), &_deflate);
      }

      if (has(RPL_OPTION_INFLATE))
      {
         string inflate = get<string>(RPL_OPTION_INFLATE);
         ossStrToBoolean(inflate.c_str(), &_inflate);
      }

      if (_deflate && _inflate)
      {
         rc = SDB_INVALIDARG;
         std::cerr << "conflict arguments: " << RPL_OPTION_DEFLATE
                   << " and " << RPL_OPTION_INFLATE
                   << " can't be specified at the same time" << std::endl;
         PD_LOG( PDERROR, "%s and %s can't be specified at the same time, rc=%d",
                 RPL_OPTION_DEFLATE, RPL_OPTION_INFLATE, rc ) ;
         goto error ;
      }

      if (has(RPL_OPTION_HOST))
      {
         _hostName = get<string>(RPL_OPTION_HOST);
      }
      else if (!_dump && !_dumpHeader && !_deflate && !_inflate)
      {
         std::cerr << "Missing argument: " << RPL_OPTION_HOST << std::endl;
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Missing argument: %s", RPL_OPTION_HOST);
         goto error;
      }

      if (has(RPL_OPTION_SVC))
      {
         _serviceName = get<string>(RPL_OPTION_SVC);
         if (!engine::utilStrIsDigit(_serviceName))
         {
            std::cerr << "Invalid argument: " << RPL_OPTION_SVC << std::endl;
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid argument: %s[%s]",
                   RPL_OPTION_SVC, _serviceName.c_str());
            goto error;
         }
      }
      else if (!_dump && !_dumpHeader && !_deflate && !_inflate)
      {
         std::cerr << "Missing argument: " << RPL_OPTION_SVC << std::endl;
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Missing argument: %s", RPL_OPTION_SVC);
         goto error;
      }

      if (has(RPL_OPTION_USER))
      {
         _user = get<string>(RPL_OPTION_USER);
      }

      if (has(RPL_OPTION_PASSWD))
      {
         _password = get<string>(RPL_OPTION_PASSWD);
      }

      if (has(RPL_OPTION_SSL))
      {
         string ssl = get<string>(RPL_OPTION_SSL);
         ossStrToBoolean(ssl.c_str(), &_useSSL);
      }

      if (has(RPL_OPTION_PATH))
      {
         _path = get<string>(RPL_OPTION_PATH);

         BOOLEAN exist = FALSE;
         rc = engine::ossFile::exists(_path, exist);
         if (SDB_OK != rc)
         {
            std::cerr << "Failed to access path: " << _path << std::endl;
            PD_LOG(PDERROR, "Failed to access path[%s], rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         if (!exist)
         {
            rc = SDB_FNE;
            std::cerr << "Path is not existing" << std::endl;
            PD_LOG(PDERROR, "Path[%s] is not existing, rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         rc = ossGetPathType(_path.c_str(), &_pathType);
         if (SDB_OK != rc)
         {
            std::cerr << "Failed to get path type" << std::endl;
            PD_LOG(PDERROR, "Failed to get path[%s] type, rc=%d",
                   _path.c_str(), rc);
            goto error;
         }

         if (SDB_OSS_FIL != _pathType && SDB_OSS_DIR != _pathType)
         {
            rc = SDB_INVALIDARG;
            std::cerr << "Path is not file or directory" << std::endl;
            PD_LOG(PDERROR, "Path[%s] is not file or directory, rc=%d",
                   _path.c_str(), rc);
            goto error;
         }
      }
      else
      {
         std::cerr << "Missing argument: " << RPL_OPTION_PATH << std::endl;
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Missing argument: %s", RPL_OPTION_PATH);
         goto error;
      }

      if (has(RPL_OPTION_FILTER))
      {
         string filter = get<string>(RPL_OPTION_FILTER);
         rc = bson::fromjson(filter, _filter) ;
         if (SDB_OK != rc)
         {
            std::cerr << "invalid argument: " << RPL_OPTION_FILTER << std::endl;
            PD_LOG( PDERROR, "Failed to convert json[%s] to bson, rc=%d",
                    filter.c_str(), rc ) ;
            goto error ;
         }
      }

      if (has(RPL_OPTION_DELETE))
      {
         string del = get<string>(RPL_OPTION_DELETE);
         ossStrToBoolean(del.c_str(), &_delete);
      }

      if (has(RPL_OPTION_WATCH))
      {
         string hold = get<string>(RPL_OPTION_WATCH);
         ossStrToBoolean(hold.c_str(), &_watch);
      }

      if (has(RPL_OPTION_DAEMON))
      {
         string daemon = get<string>(RPL_OPTION_DAEMON);
         ossStrToBoolean(daemon.c_str(), &_daemon);
      }

      if (has(RPL_OPTION_STATUS))
      {
         _status = get<string>(RPL_OPTION_STATUS);
      }

      if (has(RPL_OPTION_DEBUG))
      {
         _debug = TRUE;
      }

      if (has(RPL_OPTION_TYPE))
      {
         string type = get<string>(RPL_OPTION_TYPE);
         if (type == RPL_OPTION_TYPE_ARCHIVE)
         {
            _isReplicaFile = FALSE;
         }
         else if (type == RPL_OPTION_TYPE_REPLICA)
         {
            _isReplicaFile = TRUE;
         }
         else
         {
            rc = SDB_INVALIDARG;
            std::cerr << "invalid argument: " << RPL_OPTION_TYPE << std::endl;
            PD_LOG( PDERROR, "invalid argument of %s: %s, rc=%d",
                    RPL_OPTION_TYPE, type.c_str(), rc ) ;
            goto error ;
         }
      }

      if (_isReplicaFile && (_inflate || _deflate))
      {
         rc = SDB_INVALIDARG;
         std::cerr << RPL_OPTION_INFLATE << " and " << RPL_OPTION_DEFLATE
                   << " can't run with replica file"
                   << std::endl;
         PD_LOG( PDERROR, "%s and %s can't run with replica file, rc=%d",
                 RPL_OPTION_INFLATE, RPL_OPTION_DEFLATE, rc ) ;
         goto error ;
      }

      if (has(RPL_OPTION_UPDATE_WITH_SHARING_KEY))
      {
         string withShardingKey = get<string>(RPL_OPTION_UPDATE_WITH_SHARING_KEY);
         ossStrToBoolean(withShardingKey.c_str(), &_updateWithShardingKey);
      }

   done: 
      return rc;
   error:
      goto done;
   }
}
