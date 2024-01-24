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
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impScanner.hpp"
#include "impParser.hpp"
#include "impImporter.hpp"

using namespace std;

namespace import
{
   class Routine
   {
   public:
      Routine( Options& options ) ;

      ~Routine() ;

      INT32 run() ;

      void printStatistics() ;

   private:
      INT32 _init() ;

      INT32 _startScanner() ;
      INT32 _stopScanner() ;

      INT32 _startParser() ;
      INT32 _waitParserStop() ;

      INT32 _startImporter() ;
      INT32 _stopImporter() ;

   private:
      BOOLEAN           _isInit ;
      INT32             _dataQueueNum ;

      DataQueue*        _dataQueue ;
      BsonPageQueue*    _freeQueue ;
      PageQueue*        _importQueue ;

      Options&          _options ;

      PageQueueBuffer   _importQueueBuffer ;

      Scanner           _scanner ;
      Parser            _parser ;
      Importer          _importer ;
      Packer            _packer ;
   };
}

#endif /* IMP_ROUTINE_HPP_ */
