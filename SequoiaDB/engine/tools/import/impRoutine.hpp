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

   Source File Name = impRoutine.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_ROUTINE_HPP_
#define IMP_ROUTINE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "impRecordQueue.hpp"
#include "impParser.hpp"
#include "impImporter.hpp"
#include "impSharding.hpp"

using namespace std;

namespace import
{
   class Routine
   {
   public:
      Routine(Options& options);
      ~Routine();
      INT32 run();
      void printStatistics();

   private:
      INT32 _startParser();
      INT32 _waitParserStop();
      INT32 _startImporter(INT32 workerNum);
      INT32 _stopImporter();
      INT32 _startSharding();
      INT32 _stopSharding();

   private:
      Options&          _options;
      RecordQueue       _parsedQueue;
      RecordQueue       _shardingQueue;

      Parser            _parser;
      Importer          _importer;
      Sharding          _sharding;
   };
}

#endif /* IMP_ROUTINE_HPP_ */
