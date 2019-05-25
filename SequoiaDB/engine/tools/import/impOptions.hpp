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

   Source File Name = impOptions.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_OPTIONS_HPP_
#define IMP_OPTIONS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impHosts.hpp"
#include "utilOptions.hpp"

using namespace std;

namespace import
{

   enum INPUT_TYPE
   {
      INPUT_FILE = 0,
      INPUT_STDIN,
      INPUT_EXEC
   };

   enum INPUT_FORMAT
   {
      FORMAT_CSV = 0,
      FORMAT_JSON
   };

   enum STR_TRIM_TYPE
   {
      STR_TRIM_NO = 0,
      STR_TRIM_RIGHT,
      STR_TRIM_LEFT,
      STR_TRIM_BOTH
   };

   class Options: public engine::utilOptions
   {
   public:
      Options();
      ~Options();
      INT32 parse(INT32 argc, CHAR* argv[]);
      void printHelpInfo();
      void printHelpfullInfo();
      BOOLEAN hasHelp();
      BOOLEAN hasVersion();
      BOOLEAN hasHelpfull();

      /* general */
      inline const string& hostname() const { return _hostname; }
      inline const string& svcname() const { return _svcname; }
      inline const string& hostsString() const { return _hostsString; }
      inline vector<Host>& hosts() { return _hosts; }
      inline const string& user() const { return _user; }
      inline const string& password() const { return _password; }
      inline const string& csname() const { return _csName; }
      inline const string& clname() const { return _clName; }
      inline BOOLEAN errorStop() const { return _errorStop; }
      inline BOOLEAN useSSL() const { return _useSSL; }
      inline BOOLEAN verbose() const { return _verbose; }

      /* import */
      inline INT32 batchSize() const { return _batchSize; }
      inline INT32 jobs() const { return _jobs; }
      inline BOOLEAN enableSharding() const { return _enableSharding; }
      inline BOOLEAN enableCoord() const { return _enableCoord; }
      inline BOOLEAN enableTransaction() const { return _enableTransaction; }
      inline BOOLEAN allowKeyDuplication() const { return _allowKeyDuplication; }

      /* input */
      inline const vector<string>& files() const { return _files; }
      inline const string& exec() const { return _exec; }
      inline INPUT_TYPE inputType() const { return _inputType; }
      inline INPUT_FORMAT inputFormat() const { return _inputFormat; }
      inline BOOLEAN linePriority() const { return _linePriority; }
      inline const string& recordDelimiter() const { return _recordDelimiter; }
      inline BOOLEAN force() const { return _force; }

      /* csv */
      inline const string& stringDelimiter() const { return _stringDelimiter; }
      inline const string& fieldDelimiter() const { return _fieldDelimiter; }
      inline const string& fields() const { return _fields; }
      inline const string& dateFormat() const { return _dateFormat; }
      inline const string& timestampFormat() const { return _timestampFormat; }
      inline const STR_TRIM_TYPE trimString() const { return _trimString; }
      inline BOOLEAN hasHeaderLine() const { return _hasHeaderLine; }
      inline BOOLEAN autoAddField() const { return _autoAddField; }
      inline BOOLEAN autoCompletion() const { return _autoCompletion; }
      inline BOOLEAN cast() const { return _cast; }
      inline BOOLEAN strictFieldNum() const { return _strictFieldNum; }

      /* helpful */
      inline INT32 bufferSize() const { return _bufferSize; }
      inline BOOLEAN dryRun() const { return _dryRun; }
      inline INT64 recordsMem() const { return _recordsMem; }
      inline BOOLEAN ignoreNull() const { return _ignoreNull; }

   private:
      INT32 setOptions();

   private:
      BOOLEAN        _parsed;

      /* general */
      string         _hostname;
      string         _svcname;
      string         _user;
      string         _hostsString;
      vector<Host>   _hosts;
      string         _password;
      string         _csName;
      string         _clName;
      BOOLEAN        _errorStop;
      BOOLEAN        _useSSL;
      BOOLEAN        _verbose;

      /* import */
      INT32          _batchSize;
      INT32          _jobs;
      BOOLEAN        _enableSharding;
      BOOLEAN        _enableCoord;
      BOOLEAN        _enableTransaction;
      BOOLEAN        _allowKeyDuplication;

      /* input */
      vector<string> _files;
      string         _exec;
      INPUT_TYPE     _inputType;
      INPUT_FORMAT   _inputFormat;
      BOOLEAN        _linePriority;
      string         _recordDelimiterIn;
      string         _recordDelimiter;
      BOOLEAN        _force;

      /* csv */
      string         _stringDelimiterIn;
      string         _fieldDelimiterIn;
      string         _stringDelimiter;
      string         _fieldDelimiter;
      string         _fields;
      string         _dateFormat;
      string         _timestampFormat;
      STR_TRIM_TYPE  _trimString;
      BOOLEAN        _hasHeaderLine;
      BOOLEAN        _autoAddField;
      BOOLEAN        _autoCompletion;
      BOOLEAN        _cast;
      BOOLEAN        _strictFieldNum;

      /* helpfull */
      BOOLEAN        _dryRun;
      INT32          _bufferSize;
      INT64          _recordsMem; // the records used momory threshold 
      BOOLEAN        _ignoreNull;

   };
}

#endif /* IMP_OPTIONS_HPP_ */
