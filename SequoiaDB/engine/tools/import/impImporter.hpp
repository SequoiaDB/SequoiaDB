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

   Source File Name = impImporter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_IMPORTER_HPP_
#define IMP_IMPORTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impCoord.hpp"
#include "impRecordQueue.hpp"
#include "impLogFile.hpp"
#include "ossAtomic.hpp"
#include <vector>

using namespace std;

namespace import
{
   class Importer: public SDBObject
   {
   public:
      Importer();
      ~Importer();
      INT32 init(Options* options,
                 RecordQueue* workQueue,
                 INT32 workerNum);
      INT32 start();
      INT32 stop();
      inline BOOLEAN isStopped()
      {
         return 0 == _livingNum.fetch();
      }
      inline INT64 importedNum() { return _importedNum.fetch(); }
      inline INT64 failedNum() { return _failedNum.fetch(); }
      inline const string& logFileName() const
      {
         return _logFile.fileName();
      }

   private:
      Options*          _options;
      RecordQueue*      _workQueue;
      BOOLEAN           _inited;

      Coords            _coords;
      UINT32            _refCount;
      vector<Worker*>   _workers;
      ossAtomicSigned32 _livingNum;
      LogFile           _logFile;

      // statistics
      ossAtomicSigned64 _importedNum;
      ossAtomicSigned64 _failedNum;

      friend void _importerRoutine(WorkerArgs* args);
   };
}

#endif /* IMP_IMPORTER_HPP_ */
