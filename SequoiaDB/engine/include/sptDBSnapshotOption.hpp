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

   Source File Name = sptDBSnapshotOption.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          16/07/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_SNAPSHOTOPTION_HPP
#define SPT_DB_SNAPSHOTOPTION_HPP

#include "sptApi.hpp"
#include "sptDBOptionBase.hpp"

using namespace bson ;

namespace engine
{
   #define SPT_SNAPSHOTOPTION_NAME      "SdbSnapshotOption"

   /*
      _sptDBSnapshotOption define
   */
   class _sptDBSnapshotOption : public _sptDBOptionBase
   {
      JS_DECLARE_CLASS( _sptDBSnapshotOption )
   public:
      _sptDBSnapshotOption() ;
      virtual ~_sptDBSnapshotOption() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      static INT32 bsonToJSObj( sdbclient::sdb &db,
                                const BSONObj &data,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   } ;

   typedef _sptDBSnapshotOption sptDBSnapshotOption ;
}

#endif // SPT_DB_SNAPSHOTOPTION_HPP
