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
#include "utilPasswdTool.hpp"
#include "utilParam.hpp"
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
   #define IMP_OPTION_CIPHERFILE        "cipherfile"
   #define IMP_OPTION_CIPHER            "cipher"
   #define IMP_OPTION_TOKEN             "token"
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
   #define IMP_OPTION_UNICODE           "unicode"
   #define IMP_OPTION_CHECKDELIMETER    "checkdelimeter"
   #define IMP_OPTION_DECIMALTO         "decimalto"

   #define IMP_EXPLAIN_HELP             "print help information"
   #define IMP_EXPLAIN_VERSION          "print version"
   #define IMP_EXPLAIN_HOSTNAME         "host name, default: localhost"
   #define IMP_EXPLAIN_SVCNAME          "service name, default: 11810"
   #define IMP_EXPLAIN_HOSTS            "host addresses(hostname:svcname), separated by ',', such as 'localhost:11810,localhost:11910', default: 'localhost:11810'"
   #define IMP_EXPLAIN_USER             "username"
   #define IMP_EXPLAIN_PASSWORD         "password"
   #define IMP_EXPLAIN_CIPHERFILE       "cipher file location, default ~/sequoiadb/passwd"
   #define IMP_EXPLAIN_CIPHER           "input password using a cipher file"
   #define IMP_EXPLAIN_TOKEN            "password encryption token"
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
   #define IMP_EXPLAIN_LINEPRIORITY     "reverse the priority for record and character delimiter (arg: [auto|true|false]), default: auto"
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
   #define IMP_EXPLAIN_UNICODE          "whether to escape Unicode encoding, default: true"
   #define IMP_EXPLAIN_CHECKDEL         "whether to check delimeter strictly, default: true"
   #define IMP_EXPLAIN_DECIMALTO        "decimal type cast (arg: [double|string], e.g. --decimalto double), default: \"\""

   #define _TYPE(T) utilOptType(T)
   #define _IMPLICIT_TYPE(T,V) implicit_value<T>(V)

   #define IMP_DEFAULT_HOSTNAME "localhost"
   #define IMP_DEFAULT_SVCNAME  "11810"
   #define IMP_DEFAULT_HOST     "localhost:11810"

   #define IMP_STR_TRIM_NO    "no"
   #define IMP_STR_TRIM_RIGHT "right"
   #define IMP_STR_TRIM_LEFT  "left"
   #define IMP_STR_TRIM_BOTH  "both"

   #define IMP_STR_LINEPRIORITY_AUTO   "auto"

   #define IMP_INT_DECIMALTO_DOUBLE    "double"
   #define IMP_INT_DECIMALTO_STRING    "string"

   #define IMP_STR_TRIM_TYPE_EQ(str, type) \
      ((sizeof(type) - 1) == str.length() && ossStrncasecmp(str.c_str(), type, str.length()) == 0)

   #define IMP_GENERAL_OPTIONS \
      (IMP_OPTION_HELP",h",             /* no arg */     IMP_EXPLAIN_HELP) \
      (IMP_OPTION_VERSION",V",          /* no arg */     IMP_EXPLAIN_VERSION) \
      (IMP_OPTION_HOSTNAME",s",        _TYPE(string),    IMP_EXPLAIN_HOSTNAME) \
      (IMP_OPTION_SVCNAME",p",         _TYPE(string),    IMP_EXPLAIN_SVCNAME) \
      (IMP_OPTION_HOSTS,               _TYPE(string),    IMP_EXPLAIN_HOSTS) \
      (IMP_OPTION_USER",u",            _TYPE(string),    IMP_EXPLAIN_USER) \
      (IMP_OPTION_PASSWORD",w", _IMPLICIT_TYPE(string, ""), IMP_EXPLAIN_PASSWORD) \
      (IMP_OPTION_CIPHERFILE,          _TYPE(string),    IMP_EXPLAIN_CIPHERFILE) \
      (IMP_OPTION_CIPHER,              _TYPE(bool),      IMP_EXPLAIN_CIPHER) \
      (IMP_OPTION_TOKEN,               _TYPE(string),    IMP_EXPLAIN_TOKEN) \
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

   #define IMP_JSON_OPTIONS \
      (IMP_OPTION_UNICODE,             _TYPE(bool),      IMP_EXPLAIN_UNICODE) \
      (IMP_OPTION_DECIMALTO,           _TYPE(string),    IMP_EXPLAIN_DECIMALTO) \

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
      (IMP_OPTION_CHECKDELIMETER,      _TYPE(bool),      IMP_EXPLAIN_CHECKDEL) \

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

            // escape ascii char
            if (isdigit(nextCh))
            {
               INT64 c = 0;

               while (len > 0 && isdigit(*str))
               {
                  c = c * 10 + (*str - '0');
                  // the max ascii is 127
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

      _isUnicode = TRUE ;
      _decimalto = DECIMALTO_DEFAULT ;

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
      _strictCheckDel = TRUE;

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

      addOptions("JSON Options")
         IMP_JSON_OPTIONS
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

   BOOLEAN Options::_checkDelimeters(string &stringDelimiter,
                                     string &fieldDelimiter,
                                     string &recordDelimiter)
   {
      if (stringDelimiter.find( fieldDelimiter ) != string::npos)
      {
         std::cerr << IMP_OPTION_DELCHAR << " can't contain "
                   << IMP_OPTION_DELFIELD << std::endl;
         return FALSE;
      }

      if (stringDelimiter.find( recordDelimiter ) != string::npos)
      {
         std::cerr << IMP_OPTION_DELCHAR << " can't contain "
                   << IMP_OPTION_DELRECORD << std::endl;
         return FALSE;
      }

      if (fieldDelimiter.find( stringDelimiter ) != string::npos)
      {
         std::cerr << IMP_OPTION_DELFIELD << " can't contain "
                   << IMP_OPTION_DELCHAR << std::endl;
         return FALSE;
      }

      if (fieldDelimiter.find( recordDelimiter ) != string::npos)
      {
         std::cerr << IMP_OPTION_DELFIELD << " can't contain "
                   << IMP_OPTION_DELRECORD << std::endl;
         return FALSE;
      }

      if (recordDelimiter.find( stringDelimiter ) != string::npos)
      {
         std::cerr << IMP_OPTION_DELRECORD << " can't contain "
                   << IMP_OPTION_DELCHAR << std::endl;
         return FALSE;
      }

      if (recordDelimiter.find( fieldDelimiter ) != string::npos)
      {
         std::cerr << IMP_OPTION_DELRECORD << " can't contain "
                   << IMP_OPTION_DELFIELD << std::endl;
         return FALSE;
      }

      return TRUE;
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

      // add hostname & svcname to hostsString,
      // so we can process them in one time
      if (has(IMP_OPTION_HOSTNAME) || has(IMP_OPTION_SVCNAME))
      {
         // it's ok if there are duplicate hostsString, it'll be processed.
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

      if (has(IMP_OPTION_CIPHERFILE))
      {
         _cipherfile = get<string>(IMP_OPTION_CIPHERFILE);
      }

      if (has(IMP_OPTION_TOKEN))
      {
         _token = get<string>(IMP_OPTION_TOKEN);
      }

      if (has(IMP_OPTION_USER))
      {
         _user = get<string>(IMP_OPTION_USER) ;

         if ( has(IMP_OPTION_PASSWORD) )
         {
            string passwd = get<string>(IMP_OPTION_PASSWORD) ;
            if ( "" == passwd )
            {
               passwd = passwd::utilPasswordTool::interactivePasswdInput() ;
            }
            _password = passwd ;
         }
         else
         {
            passwd::utilPasswordTool passwdTool ;

            if ( has(IMP_OPTION_CIPHER) && get<bool>(IMP_OPTION_CIPHER) )
            {
               rc = passwdTool.getPasswdByCipherFile( _user, _token,
                                                      _cipherfile,
                                                      _password ) ;
               if ( SDB_OK != rc )
               {
                  std::cerr << "Failed to get user[" << _user.c_str()
                            << "] password from cipher file"
                            << "[" << _cipherfile.c_str() << "], rc: " << rc
                            << std::endl ;
                  PD_LOG( PDERROR, "Failed to get user[%s] password from cipher"
                          " file[%s], rc: %d", _user.c_str(),
                          _cipherfile.c_str(), rc ) ;
                  goto error ;
               }
               _user = passwd::utilGetUserShortNameFromUserFullName( _user ) ;
            }
            else
            {
               if ( has(IMP_OPTION_TOKEN) || has(IMP_OPTION_CIPHERFILE) )
               {
                  std::cout << "If you want to use cipher text, you should use"
                            << " \"--cipher true\"" << std::endl ;
               }
            }
         }
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

         if( 0 == ossStrncasecmp( linePriority.c_str(),
                                  IMP_STR_LINEPRIORITY_AUTO,
                                  linePriority.length() ) )
         {
            if( FORMAT_CSV == _inputFormat )
            {
               _linePriority = TRUE ;
            }
            else if( FORMAT_JSON == _linePriority )
            {
               _linePriority = FALSE ;
            }
         }
         else
         {
            ossStrToBoolean(linePriority.c_str(), &_linePriority);
         }
      }
      else
      {
         if( FORMAT_CSV == _inputFormat )
         {
            _linePriority = TRUE ;
         }
         else if( FORMAT_JSON == _linePriority )
         {
            _linePriority = FALSE ;
         }
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

      if (has(IMP_OPTION_CHECKDELIMETER))
      {
         _strictCheckDel = get<bool>(IMP_OPTION_CHECKDELIMETER);
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

         if (_fieldDelimiter.find( _recordDelimiter ) != string::npos)
         {
            std::cerr << IMP_OPTION_DELFIELD << " can't contain "
                      << IMP_OPTION_DELRECORD << std::endl;
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (_strictCheckDel)
         {
            if (!_checkDelimeters(_stringDelimiter, _fieldDelimiter,
                                  _recordDelimiter))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
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

      if (has(IMP_OPTION_UNICODE))
      {
         _isUnicode = get<bool>(IMP_OPTION_UNICODE) ? TRUE : FALSE ;
      }

      if( has( IMP_OPTION_DECIMALTO ) )
      {
         string decimalto = get<string>( IMP_OPTION_DECIMALTO ) ;

         if( decimalto.empty() )
         {
            _decimalto = DECIMALTO_DEFAULT ;
         }
         else if( IMP_STR_TRIM_TYPE_EQ( decimalto, IMP_INT_DECIMALTO_DOUBLE ) )
         {
            _decimalto = DECIMALTO_DOUBLE ;
         }
         else if( IMP_STR_TRIM_TYPE_EQ( decimalto, IMP_INT_DECIMALTO_STRING ) )
         {
            _decimalto = DECIMALTO_STRING ;
         }
         else
         {
            std::cerr << "invalid argument of [" IMP_OPTION_DECIMALTO "]: "
                      << decimalto << std::endl ;
            rc = SDB_INVALIDARG;
            goto error;
         }
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
         rc = checkDateTimeFormat(datefmt);
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
         rc = checkDateTimeFormat(tsfmt);
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
