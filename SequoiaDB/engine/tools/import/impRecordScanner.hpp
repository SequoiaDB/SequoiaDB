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

   Source File Name = impRecordScanner.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_SCANNER_HPP_
#define IMP_RECORD_SCANNER_HPP_

#include "impOptions.hpp"
#include "core.hpp"
#include "oss.hpp"
#include <string>

using namespace std;

namespace import
{
   class RecordScanner: public SDBObject
   {
   public:
      RecordScanner(const string& recordDelimiter,
                    const string& stringDelimiter,
                    INPUT_FORMAT format,
                    BOOLEAN linePriority);
      ~RecordScanner();
      inline INPUT_FORMAT format() { return _format; }
      INT32 scan(const CHAR* data, INT32 length, BOOLEAN final,
                 INT32& recordLength);

   private:
      INT32 _scanCSV(const CHAR* data, INT32 length, BOOLEAN final,
                     INT32& recordLength);
      INT32 _scanJSON(const CHAR* data, INT32 length, BOOLEAN final,
                      INT32& recordLength);

   private:
      string         _recordDelimiter;
      string         _stringDelimiter;
      INPUT_FORMAT   _format;
      BOOLEAN        _linePriority;
   };
}

#endif /* IMP_RECORD_SCANNER_HPP_ */
