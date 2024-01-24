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

   Source File Name = impRecordParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_PARSER_HPP_
#define IMP_RECORD_PARSER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "../client/bson/bson.h"
#include "jstobs.h"
#include <string>

using namespace std;

namespace import
{
   #define IMP_MAX_BSON_SIZE (1024 * 1024 * 16)

   // abstract class
   class RecordParser: public SDBObject
   {
   private:
      // disallow copy and assign
      RecordParser(const RecordParser&);
      void operator=(const RecordParser&);

   protected:
      RecordParser(const string& fieldDelimiter,
                   const string& stringDelimiter,
                   BOOLEAN autoAddField = TRUE,
                   BOOLEAN autoAddValue = FALSE,
                   BOOLEAN autoAddStrDel = FALSE,
                   BOOLEAN mustHasIDField = TRUE);

   public:
      virtual ~RecordParser() {}
      virtual INT32 parseRecord(const CHAR* data, INT32 length, bson& obj) = 0;
      virtual void  reset() {}

   protected:
      string   _fieldDelimiter;
      string   _stringDelimiter;
      BOOLEAN  _autoAddField;
      BOOLEAN  _autoAddValue;
      BOOLEAN  _autoAddStrDel;
      BOOLEAN  _mustHasIDField;

   public:
      static INT32 createInstance(INPUT_FORMAT format, const Options& options,
                                  RecordParser*& parser);
      static void  releaseInstance(RecordParser* parser);
   };

   class JSONRecordParser: public RecordParser
   {
   public:
      JSONRecordParser( BOOLEAN isUnicode, DECIMAL_TO_TYPE decimalto, BOOLEAN mustHasIDField );
      ~JSONRecordParser();
      INT32 init() ;
      INT32 parseRecord(const CHAR* data, INT32 length, bson& obj);

   private:
      BOOLEAN _isUnicode ;
      INT32   _decimalto ;
      CJSON_MACHINE *_pMachine ;
   };
}

#endif /* IMP_RECORD_PARSER_HPP_ */
