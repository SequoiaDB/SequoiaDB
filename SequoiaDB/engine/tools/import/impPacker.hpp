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
          06/11/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_PACKER_HPP_
#define IMP_PACKER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCommon.hpp"
#include "impOptions.hpp"
#include "impRecordSharding.hpp"
#include "impLogFile.hpp"

namespace import
{
   typedef map<UINT32, BsonPageHeader*> PageMap ;

   class Packer : public SDBObject
   {
   public:
      Packer() ;

      ~Packer() ;

      INT32 init( Options* options, BsonPageQueue* freeQueue,
                  PageQueue* importQueue ) ;

      INT32 packing( bson* record ) ;

      BOOLEAN clearQueue( BOOLEAN allClear = TRUE ) ;

      inline void stop()
      {
         _stopped = TRUE ;
      }

      inline INT64 shardingNum()
      {
         return _shardingNum.fetch() ;
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
      INT32 _getHeaderId( bson* record, UINT32& headerId ) ;

      INT32 _initShardingMap() ;

      INT32 _initPageMap() ;

      void _pushToImportQueue( BsonPageHeader* pageHeader ) ;

   private:
      INT32             _mapNum ;
      BOOLEAN           _inited ;
      BOOLEAN           _stopped ;
      BOOLEAN           _needSharding ;

      Options*          _options ;
      BsonPageQueue*    _freeQueue ;
      PageQueue*        _importQueue ;

      ossAtomicSigned64 _shardingNum ;
      ossAtomicSigned64 _failedNum ;

      PageMap           _pageMap ;
      RecordSharding    _sharding ;
      LogFile           _logFile ;
   } ;
}

#endif /* IMP_PACKER_HPP_ */