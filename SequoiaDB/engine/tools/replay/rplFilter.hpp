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

   Source File Name = rplFilter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_FILTER_HPP_
#define REPLAY_FILTER_HPP_

#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "dpsDef.hpp"
#include "dpsLogFile.hpp"
#include "dpsLogRecord.hpp"
#include "dpsArchiveFile.hpp"
#include <set>

using namespace std;
using namespace bson;

namespace replay
{
   class Filter: public SDBObject
   {
   public:
      Filter();
      ~Filter();
      INT32 init(const BSONObj& filterObj);
      BOOLEAN isFiltered(engine::dpsArchiveFile& file);
      BOOLEAN isFiltered(engine::dpsLogFile& file);
      BOOLEAN isFiltered(const engine::dpsLogRecord& log, BOOLEAN dump = FALSE);
      BOOLEAN lessThanMinLSN(DPS_LSN_OFFSET lsn);
      BOOLEAN largerThanMaxLSN(DPS_LSN_OFFSET lsn);

   private:
      INT32 _checkFilterField(const BSONObj& filterObj);
      INT32 _checkFilterOP(const set<string>& ops);
      INT32 _readStringArray(const BSONObj& filterObj,
                             const string& fieldName,
                             set<string>& out);
      INT32 _readInt64(const BSONObj& filterObj,
                         const string& fieldName,
                         INT64& out);
      BOOLEAN _isFileFiltered(const string& fileName);
      BOOLEAN _isCSFiltered(const string& csName);
      BOOLEAN _isCLFiltered(const string& fullName);
      BOOLEAN _isOPFiltered(const string& op);
      BOOLEAN _isLSNFiltered(DPS_LSN_OFFSET lsn);

   private:
      set<string>       _files;
      set<string>       _exclFiles;
      set<string>       _cs;
      set<string>       _exclCS;
      set<string>       _cl;
      set<string>       _exclCL;
      set<string>       _op;
      set<string>       _exclOP;
      DPS_LSN_OFFSET    _minLSN;
      DPS_LSN_OFFSET    _maxLSN;
   };
}

#endif /* REPLAY_FILTER_HPP_ */

