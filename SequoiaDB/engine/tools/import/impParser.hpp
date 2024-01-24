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

   Source File Name = impParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_PARSER_HPP_
#define IMP_PARSER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impLogFile.hpp"
#include "impPacker.hpp"

namespace import
{
   class Parser: public WorkerArgs
   {
   public:
      Parser() ;

      ~Parser() ;

      INT32 init( Options* options, DataQueue* dataQueue, Packer* packer,
                  INT32 workerNum ) ;

      INT32 start() ;

      INT32 stop() ;

      inline BOOLEAN isStopped()
      {
         return 0 == _livingNum.fetch() ;
      }

      inline INT64 parsedNum()
      {
         return _parsedNum.fetch() ;
      }

      inline INT64 failedNum()
      {
         return _failedNum.fetch() ;
      }

      inline const string& logFileName() const
      {
         return _logFile.fileName() ;
      }

   private:
      BOOLEAN           _inited ;
      BOOLEAN           _stopped ;

      Options*          _options ;
      DataQueue*        _dataQueue ;
      Packer*           _packer ;

      LogFile           _logFile ;
      vector<Worker*>   _workers ;

      // statistics
      ossAtomicSigned32 _livingNum ;
      ossAtomicSigned64 _parsedNum ;
      ossAtomicSigned64 _failedNum ;

      friend void _parserRoutine( WorkerArgs* args ) ;
   };
}

#endif /* IMP_PARSER_HPP_ */
