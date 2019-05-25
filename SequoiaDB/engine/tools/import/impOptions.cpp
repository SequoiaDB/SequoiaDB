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

   Source File Name = impOptions.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impOptions.hpp"
#include "impUtil.hpp"
#include "ossUtil.h"
#include "pd.hpp"
#include <iostream>
#include <sstream>

using namespace engine;
using namespace std;

namespace import
{
   #define IMP_OPTION_HELP              "help"
   #define IMP_OPTION_VERSION           "version"
   #define IMP_OPTION_HOSTNAME          "hostname"
   #define IMP_OPTION_SVCNAME           "svcname"
   #define IMP_OPTION_HOSTS             "hosts"
   #define IMP_OPTION_USER              "user"
   #define IMP_OPTION_PASSWORD          "password"
   #define IMP_OPTION_COLLECTSPACE      "csname"
   #define IMP_OPTION_COLLECTION        "clname"
   #define IMP_OPTION_DELCHAR           "delchar"
   #define IMP_OPTION_DELFIELD          "delfield"
   #define IMP_OPTION_DELRECORD         "delrecord"
   #define IMP_OPTION_FILENAME          "file"
   #define IMP_OPTION_EXTRA             "extra"
   #define IMP_OPTION_SPARSE            "sparse"
   #define IMP_OPTION_LINEPRIORITY      "linepriority"
   #define IMP_OPTION_FIELDS            "fields"
   #define IMP_OPTION_HEADERLINE        "headerline"
   #define IMP_OPTION_TYPE              "type"
   #define IMP_OPTION_BATCHSIZE         "insertnum"
   #define IMP_OPTION_ERRORSTOP         "errorstop"
   #define IMP_OPTION_FORCE             "force"
   #define IMP_OPTION_SSL               "ssl"
   #define IMP_OPTION_JOBS              "jobs"
   #define IMP_OPTION_BUFFERSIZE        "buffer"
   #define IMP_OPTION_DRYRUN            "dryrun"
   #define IMP_OPTION_VERBOSE           "verbose"
   #define IMP_OPTION_EXEC              "exec"
   #define IMP_OPTION_SHARDING          "sharding"
   #define IMP_OPTION_COORD             "coord"
   #define IMP_OPTION_TRANSACTION       "transaction"
   #define IMP_OPTION_ALLOWKEYDUP       "allowkeydup"
   #define IMP_OPTION_HELPFULL          "helpfull"
   #define IMP_OPTION_RECORDSMEM        "recordsmem"
   #define IMP_OPTION_CAST              "cast"
   #define IMP_OPTION_DATEFMT           "datefmt"
   #define IMP_OPTION_TIMESTAMPFMT      "timestampfmt"
   #define IMP_OPTION_TRIMSTRING        "trim"
   #define IMP_OPTION_IGNORENULL        "ignorenull"
   #define IMP_OPTION_STRICTFIELDNUM    "strictfieldnum"

   #define IMP_EXPLAIN_HELP             "print help information"
   #define IMP_EXPLAIN_VERSION          "print version"
   #define IMP_EXPLAIN_HOSTNAME         "host name, default: localhost"
   #define IMP_EXPLAIN_SVCNAME          "service name, default: 11810"
   #define IMP_EXPLAIN_HOSTS            "host addresses(hostname:svcname), separated by ',', such as 'localhost:11810,localhost:11910', default: 'localhost:11810'"
   #define IMP_EXPLAIN_USER             "username"
   #define IMP_EXPLAIN_PASSWORD         "password"
   #define IMP_EXPLAIN_DELCHAR          "string delimiter, default: '\"' ( csv only )"
   #define IMP_EXPLAIN_DELFIELD         "field delimiter, default: ',' ( csv only )"
   #define IMP_EXPLAIN_DELRECORD        "record delimiter, default: '\\n'"
   #define IMP_EXPLAIN_COLLECTSPACE     "collection space name"
   #define IMP_EXPLAIN_COLLECTION       "collection name"
   #define IMP_EXPLAIN_BATCHSIZE        "batch insert records number, minimun 1, maximum 100000, default: 100"
   #define IMP_EXPLAIN_FILENAME         "input files name, multiple files or directories must be separated by ',', don't support subdirectories recursively. use standard input if both --exec and --file are not specified"
   #define IMP_EXPLAIN_TYPE             "type of record to load, default: csv (json,csv)"
   #define IMP_EXPLAIN_FIELDS           "field name, separated by comma (',')(e.g. --fields \"name,age\"). "\
                                        "field type and default value can be specified for csv input (e.g. --fields \"name string,age int default 18\")"
   #define IMP_EXPLAIN_HEADERLINE       "for csv input, whether the first line defines field name. if --fields is defined, the first line will be ignored if this options is true"
   #define IMP_EXPLAIN_SPARSE           "for csv input, whether to add missing field, default: true"
   #define IMP_EXPLAIN_EXTRA            "for csv input, whether to add missing value, default: false"
   #define IMP_EXPLAIN_LINEPRIORITY     "reverse the priority for record and character delimiter, default: true"
   #define IMP_EXPLAIN_ERRORSTOP        "whether stop by hitting error, default: false"
   #define IMP_EXPLAIN_FORCE            "force to insert the records that are not in utf-8 format, default: false"
   #define IMP_EXPLAIN_SSL              "use SSL connection (arg: [true|false], e.g. --ssl true), default: false"
   #define IMP_EXPLAIN_JOBS             "importing job num at once, default: 1"
   #define IMP_EXPLAIN_BUFFER           "set buffer size(unit:MB), default: 64"
   #define IMP_EXPLAIN_DRYRUN           "only parse record, don't import to database"
   #define IMP_EXPLAIN_VERBOSE          "print run time details"
   #define IMP_EXPLAIN_EXEC             "execute external program to get data, the program should output data to standard outpupt"
   #define IMP_EXPLAIN_SHARDING         "repackage records by sharding, default: true"
   #define IMP_EXPLAIN_COORD            "find coordinators automatically, default: true"
   #define IMP_EXPLAIN_TRANSACTION      "enable transaction, default: false"
   #define IMP_EXPLAIN_ALLOWKEYDUP      "allow key duplication, default: true"
   #define IMP_EXPLAIN_HELPFULL         "print all options"
   #define IMP_EXPLAIN_RECORDSMEM       "the maximum memory size used by records, the unit is MB, range is [128~81920], default: 512"
   #define IMP_EXPLAIN_CAST             "allow type cast when lost precision, default: false"
   #define IMP_EXPLAIN_DATEFMT          "set date format, default: YYYY-MM-DD"
   #define IMP_EXPLAIN_TIMESTAMPFMT     "set timestamp format, default: YYYY-MM-DD-HH.mm.ss.ffffff"
   #define IMP_EXPLAIN_TRIMSTRING       "trim string (arg: [no|right|left|both]), default: no"
   #define IMP_EXPLAIN_IGNORENULL       "ignore null field, default: false"
   #define IMP_EXPLAIN_STRICTFIELDNUM   "report error if record fields num does not equal to fields definition, default: false"

   #define _TYPE(T) utilOptType(T)

   #define IMP_DEFAULT_HOSTNAME "localhost"
   #define IMP_DEFAULT_SVCNAME  "11810"
   #define IMP_DEFAULT_HOST     "localhost:11810"

   #define IMP_STR_TRIM_NO    "no"
   #define IMP_STR_TRIM_RIGHT "right"
   #define IMP_STR_TRIM_LEFT  "left"
   #define IMP_STR_TRIM_BOTH  "both"

   #define IMP_STR_TRIM_TYPE_EQ(str, type) \
      ((sizeof(type) - 1) == str.length() && ossStrncasecmp(str.c_str(), type, str.length()) == 0)

   #define IMP_GENERAL_OPTIONS \
      (IMP_OPTION_HELP",h",             /* no arg */     IMP_EXPLAIN_HELP) \
      (IMP_OPTION_VERSION",V",          /* no arg */     IMP_EXPLAIN_VERSION) \
      (IMP_OPTION_HOSTNAME",s",        _TYPE(string),    IMP_EXPLAIN_HOSTNAME) \
      (IMP_OPTION_SVCNAME",p",         _TYPE(string),    IMP_EXPLAIN_SVCNAME) \
      (IMP_OPTION_HOSTS,               _TYPE(string),    IMP_EXPLAIN_HOSTS) \
      (IMP_OPTION_USER",u",            _TYPE(string),    IMP_EXPLAIN_USER) \
      (IMP_OPTION_PASSWORD",w",        _TYPE(string),    IMP_EXPLAIN_PASSWORD) \
      (IMP_OPTION_COLLECTSPACE",c",    _TYPE(string),    IMP_EXPLAIN_COLLECTSPACE) \
      (IMP_OPTION_COLLECTION",l",      _TYPE(string),    IMP_EXPLAIN_COLLECTION) \
      (IMP_OPTION_ERRORSTOP,           _TYPE(string),    IMP_EXPLAIN_ERRORSTOP) \
      (IMP_OPTION_SSL,                 _TYPE(string),    IMP_EXPLAIN_SSL) \
      (IMP_OPTION_VERBOSE",v",          /* no arg */     IMP_EXPLAIN_VERBOSE) \

   #define IMP_IMPORT_OPTIONS \
      (IMP_OPTION_BATCHSIZE",n",       _TYPE(INT32),     IMP_EXPLAIN_BATCHSIZE) \
      (IMP_OPTION_JOBS",j",            _TYPE(INT32),     IMP_EXPLAIN_JOBS) \
      (IMP_OPTION_COORD,               _TYPE(string),    IMP_EXPLAIN_COORD) \
      (IMP_OPTION_SHARDING,            _TYPE(string),    IMP_EXPLAIN_SHARDING) \
      (IMP_OPTION_TRANSACTION,         _TYPE(string),    IMP_EXPLAIN_TRANSACTION) \
      (IMP_OPTION_ALLOWKEYDUP,         _TYPE(string),    IMP_EXPLAIN_ALLOWKEYDUP) \

   #define IMP_INPUT_OPTIONS \
      (IMP_OPTION_FILENAME,            _TYPE(string),    IMP_EXPLAIN_FILENAME) \
      (IMP_OPTION_EXEC,                _TYPE(string),    IMP_EXPLAIN_EXEC) \
      (IMP_OPTION_TYPE,                _TYPE(string),    IMP_EXPLAIN_TYPE) \
      (IMP_OPTION_LINEPRIORITY,        _TYPE(string),    IMP_EXPLAIN_LINEPRIORITY) \
      (IMP_OPTION_DELRECORD",r",       _TYPE(string),    IMP_EXPLAIN_DELRECORD) \
      (IMP_OPTION_FORCE,               _TYPE(string),    IMP_EXPLAIN_FORCE) \

   #define IMP_CSV_OPTIONS \
      (IMP_OPTION_DELCHAR",a",         _TYPE(string),    IMP_EXPLAIN_DELCHAR) \
      (IMP_OPTION_DELFIELD",e",        _TYPE(string),    IMP_EXPLAIN_DELFIELD) \
      (IMP_OPTION_FIELDS,              _TYPE(string),    IMP_EXPLAIN_FIELDS) \
      (IMP_OPTION_DATEFMT,             _TYPE(string),    IMP_EXPLAIN_DATEFMT) \
      (IMP_OPTION_TIMESTAMPFMT,        _TYPE(string),    IMP_EXPLAIN_TIMESTAMPFMT) \
      (IMP_OPTION_TRIMSTRING,          _TYPE(string),    IMP_EXPLAIN_TRIMSTRING) \
      (IMP_OPTION_HEADERLINE,          _TYPE(string),    IMP_EXPLAIN_HEADERLINE) \
      (IMP_OPTION_SPARSE,              _TYPE(string),    IMP_EXPLAIN_SPARSE) \
      (IMP_OPTION_EXTRA,               _TYPE(string),    IMP_EXPLAIN_EXTRA) \
      (IMP_OPTION_CAST,                _TYPE(string),    IMP_EXPLAIN_CAST) \
      (IMP_OPTION_STRICTFIELDNUM,      _TYPE(string),    IMP_EXPLAIN_STRICTFIELDNUM) \

   #define IMP_HELPFUL_OPTIONS \
      (IMP_OPTION_HELPFULL,             /* no arg */     IMP_EXPLAIN_HELPFULL) \
      (IMP_OPTION_BUFFERSIZE,          _TYPE(INT32),     IMP_EXPLAIN_BUFFER) \
      (IMP_OPTION_DRYRUN,               /* no arg */     IMP_EXPLAIN_DRYRUN) \
      (IMP_OPTION_RECORDSMEM,          _TYPE(INT32),     IMP_EXPLAIN_RECORDSMEM) \
      (IMP_OPTION_IGNORENULL,          _TYPE(string),    IMP_EXPLAIN_IGNORENULL) \

   static INT32 _convertAsciiEscapeChar(const string& in, string& out)
   {
      INT32 rc = SDB_OK;
      stringstream ss;
      const CHAR* str = in.c_str();
      INT32 len = in.length();

      while (len > 0)
      {
         CHAR ch = *str;

         if ('\\' == ch)
         {
            CHAR nextCh = *(str + 1);
            str++;
            len--;

            if (isdigit(nextCh))
            {
               INT64 c = 0;

               while (len > 0 && isdigit(*str))
               {
                  c = c * 10 + (*str - '0');
                  if (c < 0 || c > 127)
                  {
                     rc = SDB_INVALIDARG;
                     goto error;
                  }
                  str++;
                  len--;
               }

               ss << (CHAR)c;
               continue;
            }
            else if ('n' == nextCh)
            {
               str++;
               len--;
               ss << '\n';
               continue;
            }
            else if ('r' == nextCh)
            {
               str++;
               len--;
               ss << '\r';
               continue;
            }
            else if ('t' == nextCh)
            {
               str++;
               len--;
               ss << '\t';
               continue;
            }
            else if ('\\' == nextCh)
            {
               str++;
               len--;
               ss << '\\';
               continue;
            }

            rc = SDB_INVALIDARG;
            goto error;
         }

         ss << ch;
         str++;
         len--;
      }

      out = ss.str();

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _checkDateTimeFormat(const string& format)
   {
      INT32 rc = SDB_OK;
      const CHAR* fmt = format.c_str();
      INT32 len = format.length();
      BOOLEAN hasYear = FALSE;
      BOOLEAN hasMonth = FALSE;
      BOOLEAN hasDay = FALSE;
      BOOLEAN hasHour = FALSE;
      BOOLEAN hasMinute = FALSE;
      BOOLEAN hasSecond = FALSE;
      BOOLEAN hasMillisecond = FALSE;
      BOOLEAN hasMicrosecond = FALSE;

      while (len > 0)
      {
         switch(*fmt)
         {
         case 'Y':
            if (hasYear)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('Y' != fmt[1] ||
                'Y' != fmt[2] ||
                'Y' != fmt[3])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasYear = TRUE;
            fmt += 4;
            len -= 4;
            break;
         case 'M':
            if (hasMonth)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('M' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMonth = TRUE;
            fmt += 2;
            len -= 2;
            break;
         case 'D':
            if (hasDay)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('D' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasDay = TRUE;
            fmt += 2;
            len -= 2;
            break;
         case 'H':
            if (hasHour)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('H' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasHour = TRUE;
            fmt += 2;
            len -= 2;
            break;
         case 'm':
            if (hasMinute)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('m' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMinute = TRUE;
            fmt += 2;
            len -= 2;
            break;
         case 's':
            if (hasSecond)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('s' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasSecond = TRUE;
            fmt += 2;
            len -= 2;
            break;
         case 'S':
            if (hasMillisecond || hasMicrosecond)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('S' != fmt[1] ||
                'S' != fmt[2])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMillisecond = TRUE;
            fmt += 3;
            len -= 3;
            break;
         case 'f':
            if (hasMillisecond || hasMicrosecond)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('f' != fmt[1] ||
                'f' != fmt[2] ||
                'f' != fmt[3] ||
                'f' != fmt[4] ||
                'f' != fmt[5])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMicrosecond = TRUE;
            fmt += 6;
            len -= 6;
            break;
         case '*':
         default:
            fmt++;
            len--;
            break;
         }
      }

      if (!hasYear || !hasMonth || !hasDay)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   Options::Options()
   {
      _parsed = FALSE;
      _hostname = IMP_DEFAULT_HOSTNAME;
      _svcname = IMP_DEFAULT_SVCNAME;
      _hostsString = IMP_DEFAULT_HOST;
      _recordDelimiter = "\n";
      _inputType = INPUT_STDIN;
      _inputFormat = FORMAT_CSV;
      _linePriority = TRUE;
      _errorStop = FALSE;
      _force = FALSE;
      _useSSL = FALSE;
      _verbose = FALSE;

      _batchSize = 100;
      _jobs = 1;
      _enableSharding = TRUE;
      _enableCoord = TRUE;
      _enableTransaction = FALSE;
      _allowKeyDuplication = TRUE;

      _stringDelimiter = "\"";
      _fieldDelimiter = ",";
      _dateFormat = "YYYY-MM-DD";
      _timestampFormat = "YYYY-MM-DD-HH.mm.ss.ffffff";
      _trimString = STR_TRIM_NO;
      _hasHeaderLine = FALSE;
      _autoAddField = TRUE;
      _autoCompletion = FALSE;
      _cast = FALSE;
      _strictFieldNum = FALSE;

      _bufferSize = 64;
      _dryRun = FALSE;
      _recordsMem = (INT64)1024 * 1024 * 512; // 512MB
      _ignoreNull = FALSE;
   }

   Options::~Options()
   {
      _parsed = FALSE;
   }

   INT32 Options::parse(INT32 argc, CHAR* argv[])
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(!_parsed, "can't parse again");

      addOptions("General Options")
         IMP_GENERAL_OPTIONS
      ;

      addOptions("Input Options")
         IMP_INPUT_OPTIONS
      ;

      addOptions("CSV Options")
         IMP_CSV_OPTIONS
      ;

      addOptions("Import Options")
         IMP_IMPORT_OPTIONS
      ;

      addOptions("Helpfull Options", TRUE)
         IMP_HELPFUL_OPTIONS
      ;

      rc = utilOptions::parse(argc, argv);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _parsed = TRUE;

      if (has(IMP_OPTION_HELP) ||
          has(IMP_OPTION_VERSION) ||
          has(IMP_OPTION_HELPFULL))
      {
         goto done;
      }

      rc = setOptions();
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN Options::hasHelp()
   {
      return has(IMP_OPTION_HELP);
   }

   BOOLEAN Options::hasVersion()
   {
      return has(IMP_OPTION_VERSION);
   }

   BOOLEAN Options::hasHelpfull()
   {
      return has(IMP_OPTION_HELPFULL);
   }

   void Options::printHelpInfo()
   {
      SDB_ASSERT(_parsed, "must be parsed");
      print();
   }

   void Options::printHelpfullInfo()
   {
      print(TRUE);
   }

   INT32 Options::setOptions()
   {
      INT32 rc = SDB_OK;

      if (has(IMP_OPTION_HOSTS))
      {
         _hostsString = get<string>(IMP_OPTION_HOSTS);
      }

      if (has(IMP_OPTION_HOSTNAME))
      {
         _hostname = get<string>(IMP_OPTION_HOSTNAME);
      }

      if (has(IMP_OPTION_SVCNAME))
      {
         _svcname = get<string>(IMP_OPTION_SVCNAME);
      }

      if (has(IMP_OPTION_HOSTNAME) || has(IMP_OPTION_SVCNAME))
      {
         if (has(IMP_OPTION_HOSTS))
         {
            _hostsString += "," + _hostname + ":" + _svcname;
         }
         else
         {
            _hostsString = _hostname + ":" + _svcname;
         }
      }

      rc = Hosts::parse(_hostsString, _hosts);
      if (SDB_OK != rc)
      {
         std::cerr << "invalid host"  << std::endl;
         rc = SDB_INVALIDARG;
         goto error;
      }
      Hosts::removeDuplicate(_hosts);

      if (has(IMP_OPTION_USER))
      {
         _user = get<string>(IMP_OPTION_USER);
      }

      if (has(IMP_OPTION_PASSWORD))
      {
         _password = get<string>(IMP_OPTION_PASSWORD);
      }

      if (has(IMP_OPTION_COLLECTSPACE))
      {
         _csName = get<string>(IMP_OPTION_COLLECTSPACE);
      }

      if (_csName.empty())
      {
         std::cerr << IMP_OPTION_COLLECTSPACE " must be specified"  << std::endl;
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (has(IMP_OPTION_COLLECTION))
      {
         _clName = get<string>(IMP_OPTION_COLLECTION);
      }

      if (_clName.empty())
      {
         std::cerr << IMP_OPTION_COLLECTION " must be specified" << std::endl;
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (has(IMP_OPTION_FILENAME) && has(IMP_OPTION_EXEC))
      {
         std::cerr << IMP_OPTION_FILENAME 
                   << " can't be specified with " 
                   << IMP_OPTION_EXEC
                   << std::endl;
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (has(IMP_OPTION_FILENAME))
      {
         _inputType = INPUT_FILE;
         string fileList = get<string>(IMP_OPTION_FILENAME);

         rc = parseFileList(fileList, _files);
         if (SDB_OK != rc)
         {
            std::cerr << "invalid " << IMP_OPTION_FILENAME 
                      << std::endl;
            goto error;
         }
      }
      else if (has(IMP_OPTION_EXEC))
      {
         _inputType = INPUT_EXEC;
         _exec = get<string>(IMP_OPTION_EXEC);
      }

      if (has(IMP_OPTION_TYPE))
      {
         string type = get<string>(IMP_OPTION_TYPE);
         if ("csv" == type)
         {
            _inputFormat = FORMAT_CSV;
         }
         else if ("json" == type)
         {
            _inputFormat = FORMAT_JSON;
         }
         else
         {
            std::cerr << "invalid argument of [" IMP_OPTION_TYPE "]: "
                      << type
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_BATCHSIZE))
      {
         _batchSize = get<INT32>(IMP_OPTION_BATCHSIZE);
         if (_batchSize <= 0 || _batchSize > 100000)
         {
            std::cerr << IMP_OPTION_BATCHSIZE " is out of range [1-100000]: "
                      << _batchSize
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_LINEPRIORITY))
      {
         string linePriority = get<string>(IMP_OPTION_LINEPRIORITY);
         ossStrToBoolean(linePriority.c_str(), &_linePriority);
      }

      if (has(IMP_OPTION_ERRORSTOP))
      {
         string errorStop = get<string>(IMP_OPTION_ERRORSTOP);
         ossStrToBoolean(errorStop.c_str(), &_errorStop);
      }

      if (has(IMP_OPTION_FORCE))
      {
         string force = get<string>(IMP_OPTION_FORCE);
         ossStrToBoolean(force.c_str(), &_force);
      }

      if (has(IMP_OPTION_JOBS))
      {
         _jobs = get<INT32>(IMP_OPTION_JOBS);
         if (_jobs <= 0 || _jobs > 1000)
         {
            std::cerr << IMP_OPTION_JOBS " is out of range [1, 1000]: "
                      << _jobs
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_SSL))
      {
         string ssl = get<string>(IMP_OPTION_SSL);
         ossStrToBoolean(ssl.c_str(), &_useSSL);
      }

      if (has(IMP_OPTION_DELCHAR))
      {
         _stringDelimiterIn = get<string>(IMP_OPTION_DELCHAR);
         if (_stringDelimiterIn.empty())
         {
            std::cerr << IMP_OPTION_DELCHAR << " can't be empty"
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = _convertAsciiEscapeChar(_stringDelimiterIn, _stringDelimiter);
         if (SDB_OK != rc)
         {
            std::cerr << "invalid " << IMP_OPTION_DELCHAR
                      << std::endl;
            goto error;
         }
      }

      if (has(IMP_OPTION_DELRECORD))
      {
         _recordDelimiterIn = get<string>(IMP_OPTION_DELRECORD);
         if (_recordDelimiterIn.empty())
         {
            std::cerr << IMP_OPTION_DELRECORD << " can't be empty"
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = _convertAsciiEscapeChar(_recordDelimiterIn, _recordDelimiter);
         if (SDB_OK != rc)
         {
            std::cerr << "invalid " << IMP_OPTION_DELRECORD
                      << std::endl;
            goto error;
         }
      }

      if (has(IMP_OPTION_DELFIELD))
      {
         _fieldDelimiterIn = get<string>(IMP_OPTION_DELFIELD);
         if (_fieldDelimiterIn.empty())
         {
            std::cerr << IMP_OPTION_DELFIELD << " can't be empty"
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = _convertAsciiEscapeChar(_fieldDelimiterIn, _fieldDelimiter);
         if (SDB_OK != rc)
         {
            std::cerr << "invalid " << IMP_OPTION_DELFIELD
                      << std::endl;
            goto error;
         }
      }

      if (FORMAT_CSV == _inputFormat)
      {
         if (_stringDelimiter == _recordDelimiter)
         {
            std::cerr << IMP_OPTION_DELCHAR << " can't be same with "
                      << IMP_OPTION_DELRECORD << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (_stringDelimiter == _fieldDelimiter)
         {
            std::cerr << IMP_OPTION_DELCHAR << " can't be same with "
                      << IMP_OPTION_DELFIELD << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (_recordDelimiter == _fieldDelimiter)
         {
            std::cerr << IMP_OPTION_DELRECORD << " can't be same with "
                      << IMP_OPTION_DELFIELD << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_FIELDS))
      {
         _fields = get<string>(IMP_OPTION_FIELDS);
      }

      if (has(IMP_OPTION_HEADERLINE))
      {
         string headerline = get<string>(IMP_OPTION_HEADERLINE);
         ossStrToBoolean(headerline.c_str(), &_hasHeaderLine);
      }

      if (FORMAT_CSV == _inputFormat)
      {
         if (_fields.empty() && !_hasHeaderLine)
         {
            std::cerr << IMP_OPTION_FIELDS " or " IMP_OPTION_HEADERLINE
                      << " must be specified when type is csv"
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_SPARSE))
      {
         string sparse = get<string>(IMP_OPTION_SPARSE);
         ossStrToBoolean(sparse.c_str(), &_autoAddField);
      }

      if (has(IMP_OPTION_EXTRA))
      {
         string extra = get<string>(IMP_OPTION_EXTRA);
         ossStrToBoolean(extra.c_str(), &_autoCompletion);
      }

      if (has(IMP_OPTION_CAST))
      {
         string cast = get<string>(IMP_OPTION_CAST);
         ossStrToBoolean(cast.c_str(), &_cast);
      }

      if (has(IMP_OPTION_STRICTFIELDNUM))
      {
         string strict = get<string>(IMP_OPTION_STRICTFIELDNUM);
         ossStrToBoolean(strict.c_str(), &_strictFieldNum);
      }

      if (has(IMP_OPTION_DATEFMT))
      {
         string datefmt = get<string>(IMP_OPTION_DATEFMT);
         rc = _checkDateTimeFormat(datefmt);
         if (SDB_OK != rc)
         {
            std::cerr << "invalid " << IMP_OPTION_DATEFMT
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         _dateFormat = datefmt;
      }

      if (has(IMP_OPTION_TIMESTAMPFMT))
      {
         string tsfmt = get<string>(IMP_OPTION_TIMESTAMPFMT);
         rc = _checkDateTimeFormat(tsfmt);
         if (SDB_OK != rc)
         {
            std::cerr << "invalid " << IMP_OPTION_TIMESTAMPFMT
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         _timestampFormat = tsfmt;
      }

      if (has(IMP_OPTION_TRIMSTRING))
      {
         string trim = get<string>(IMP_OPTION_TRIMSTRING);
         if (IMP_STR_TRIM_TYPE_EQ(trim, IMP_STR_TRIM_NO))
         {
            _trimString = STR_TRIM_NO;
         }
         else if (IMP_STR_TRIM_TYPE_EQ(trim, IMP_STR_TRIM_RIGHT))
         {
            _trimString = STR_TRIM_RIGHT;
         }
         else if (IMP_STR_TRIM_TYPE_EQ(trim, IMP_STR_TRIM_LEFT))
         {
            _trimString = STR_TRIM_LEFT;
         }
         else if (IMP_STR_TRIM_TYPE_EQ(trim, IMP_STR_TRIM_BOTH))
         {
            _trimString = STR_TRIM_BOTH;
         }
         else
         {
            std::cerr << "invalid " << IMP_OPTION_TRIMSTRING
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_BUFFERSIZE))
      {
         _bufferSize = get<INT32>(IMP_OPTION_BUFFERSIZE);
         if (_bufferSize < 32 || _bufferSize > 2048)
         {
            std::cerr << IMP_OPTION_BUFFERSIZE " is out of range [32, 2048]: "
                      << _bufferSize
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (has(IMP_OPTION_DRYRUN))
      {
         _dryRun = TRUE;
      }

      if (has(IMP_OPTION_VERBOSE))
      {
         _verbose = TRUE;
      }

      if (has(IMP_OPTION_SHARDING))
      {
         string sharding = get<string>(IMP_OPTION_SHARDING);
         ossStrToBoolean(sharding.c_str(), &_enableSharding);
      }

      if (has(IMP_OPTION_COORD))
      {
         string coord = get<string>(IMP_OPTION_COORD);
         ossStrToBoolean(coord.c_str(), &_enableCoord);
      }

      if (has(IMP_OPTION_TRANSACTION))
      {
         string tx = get<string>(IMP_OPTION_TRANSACTION);
         ossStrToBoolean(tx.c_str(), &_enableTransaction);
      }

      if (has(IMP_OPTION_ALLOWKEYDUP))
      {
         string allowKeyDup = get<string>(IMP_OPTION_ALLOWKEYDUP);
         ossStrToBoolean(allowKeyDup.c_str(), &_allowKeyDuplication);
      }

      if (has(IMP_OPTION_RECORDSMEM))
      {
         INT64 recordsMem = get<INT32>(IMP_OPTION_RECORDSMEM);
         if (recordsMem < 128 || recordsMem > 81920)
         {
            std::cerr << IMP_OPTION_RECORDSMEM " is out of range [128, 81920]: "
                      << recordsMem
                      << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         _recordsMem = recordsMem * 1024 * 1024; // convert MB to Byte
      }

      if (has(IMP_OPTION_IGNORENULL))
      {
         string ignoreNull = get<string>(IMP_OPTION_IGNORENULL);
         ossStrToBoolean(ignoreNull.c_str(), &_ignoreNull);
      }

   done:
      return rc;
   error:
      goto done;
   }
}
