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

   enum DECIMAL_TO_TYPE
   {
      DECIMALTO_DEFAULT = 0,
      DECIMALTO_DOUBLE,
      DECIMALTO_STRING
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
      INT32 check();

      /* general */
      inline const string& cipherfile()   const { return _cipherfile; }
      inline const string& hostname() const { return _hostname; }
      inline const string& svcname() const { return _svcname; }
      inline const string& hostsString() const { return _hostsString; }
      inline vector<Host>& hosts() { return _hosts; }
      inline const string& user() const { return _user; }
      inline const string& password() const { return _password; }
      inline const string& token() const { return _token; }
      inline const string& csname() const { return _csName; }
      inline const string& clname() const { return _clName; }
      inline BOOLEAN errorStop() const { return _errorStop; }
      inline BOOLEAN useSSL() const { return _useSSL; }
      inline BOOLEAN verbose() const { return _verbose; }

      /* import */
      inline INT32 batchSize() const { return _batchSize; }
      inline INT32 jobs() const { return _jobs; }
      inline INT32 parsers() const { return _parsers ; }
      inline BOOLEAN enableSharding() const { return _enableSharding; }
      inline BOOLEAN enableCoord() const { return _enableCoord; }
      inline BOOLEAN enableTransaction() const { return _enableTransaction; }
      inline BOOLEAN allowKeyDuplication() const { return _allowKeyDuplication; }
      inline BOOLEAN replaceKeyDuplication() const { return _replaceKeyDuplication ; }
      inline BOOLEAN allowIDKeyDuplication() const { return _allowIDKeyDuplication; }
      inline BOOLEAN replaceIDKeyDuplication() const { return _replaceIDKeyDuplication ; }
      inline BOOLEAN mustHasIDField() const { return _mustHasIDField ; }

      /* input */
      inline const vector<string>& files() const { return _files; }
      inline const string& exec() const { return _exec; }
      inline INPUT_TYPE inputType() const { return _inputType; }
      inline INPUT_FORMAT inputFormat() const { return _inputFormat; }
      inline BOOLEAN linePriority() const { return _linePriority; }
      inline const string& recordDelimiter() const { return _recordDelimiter; }
      inline BOOLEAN force() const { return _force; }

      /* json */
      inline BOOLEAN isUnicode() const { return _isUnicode; }
      inline const DECIMAL_TO_TYPE decimalto() const { return _decimalto ; }

      /* csv */
      inline const string& stringDelimiter() const { return _stringDelimiter; }
      inline BOOLEAN autoAddStrDel() const { return _autoAddStrDel; }
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
      INT32   setOptions( INT32 argc );
      BOOLEAN _checkDelimeters(string &stringDelimiter, string &fieldDelimiter,
                               string &recordDelimiter);

   private:
      BOOLEAN        _parsed;

      /* general */
      string         _hostname;
      string         _svcname;
      string         _user;
      string         _hostsString;
      vector<Host>   _hosts;
      string         _password;
      string         _cipherfile;
      string         _token;
      string         _csName;
      string         _clName;
      BOOLEAN        _errorStop;
      BOOLEAN        _useSSL;
      BOOLEAN        _verbose;

      /* import */
      INT32          _batchSize;
      INT32          _jobs;
      INT32          _parsers ;
      BOOLEAN        _enableSharding;
      BOOLEAN        _enableCoord;
      BOOLEAN        _enableTransaction;
      BOOLEAN        _allowKeyDuplication;
      BOOLEAN        _replaceKeyDuplication ;
      BOOLEAN        _allowIDKeyDuplication ;
      BOOLEAN        _replaceIDKeyDuplication ;
      // it must be TRUE to ensure that every record contains the '_id' field.
      BOOLEAN        _mustHasIDField ; 

      /* input */
      vector<string> _files;
      string         _exec;
      INPUT_TYPE     _inputType;
      INPUT_FORMAT   _inputFormat;
      BOOLEAN        _linePriority;
      string         _recordDelimiterIn;
      string         _recordDelimiter;
      BOOLEAN        _force;

      /* json */
      BOOLEAN         _isUnicode ;
      DECIMAL_TO_TYPE _decimalto ;

      /* csv */
      string         _stringDelimiterIn;
      BOOLEAN        _autoAddStrDel;
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
      BOOLEAN        _strictCheckDel;

      /* helpfull */
      BOOLEAN        _dryRun;
      INT32          _bufferSize;
      INT64          _recordsMem; // the records used momory threshold
      BOOLEAN        _ignoreNull;

   };
}

#endif /* IMP_OPTIONS_HPP_ */
