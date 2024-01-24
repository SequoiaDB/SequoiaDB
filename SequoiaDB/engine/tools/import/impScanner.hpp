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
          06/02/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_SCANNER_HPP_
#define IMP_SCANNER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impRecordParser.hpp"

namespace import
{
   class Scanner : public WorkerArgs
   {
   public:
      Scanner() ;
      ~Scanner() ;
      INT32 initParser( Options* options ) ;
      INT32 init( Options* options, DataQueue* dataQueue, INT32 workerNum ) ;
      INT32 start() ;
      INT32 stop() ;
      inline BOOLEAN isStopped() const { return _stopped ; }

   private:
      INT32             _workerNum ;
      BOOLEAN           _inited ;
      BOOLEAN           _stopped ;

      Options*          _options ;
      DataQueue*        _dataQueue ;
      Worker*           _worker ;
      RecordParser*     _parser ;

      impBufferBlock    _bufferBlock1 ;
      impBufferBlock    _bufferBlock2 ;

      vector<CHAR*>     _fields ;

      friend BOOLEAN _waitBufferBlock( Scanner* self, BOOLEAN useFirstBlock,
                                       impBufferBlock*& block ) ;
      friend void _scannerRoutine( WorkerArgs* args ) ;
   };
}

#endif /* IMP_SCANNER_HPP_ */
