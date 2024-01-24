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

   Source File Name = sptDBQueryOption.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/07/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_QUERYOPTION_HPP
#define SPT_DB_QUERYOPTION_HPP

#include "sptDBOptionBase.hpp"

using namespace bson ;

namespace engine
{
   #define SPT_QUERYOPTION_NAME                "SdbQueryOption"
   #define SPT_QUERYOPTION_OPTIONS_FIELD       "_options"

   /*
      _sptDBQueryOption define
   */
   class _sptDBQueryOption : public _sptDBOptionBase
   {
      JS_DECLARE_CLASS( _sptDBQueryOption )
   public:
      _sptDBQueryOption() ;
      virtual ~_sptDBQueryOption() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;

      static INT32 cvtToBSON( const CHAR* key,
                              const sptObject &value,
                              BOOLEAN isSpecialObj,
                              BSONObjBuilder& builder,
                              string &errMsg ) ;

      static INT32 fmpToBSON( const sptObject &value,
                              BSONObj &retObj,
                              string &errMsg ) ;

      static INT32 bsonToJSObj( sdbclient::sdb &db,
                                const BSONObj &data,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   protected:
      static void _setReturnVal( const BSONObj &data,
                                 _sptReturnVal &rval ) ;

   } ;
   typedef _sptDBQueryOption sptDBQueryOption ;
}

#endif //SPT_DB_QUERYOPTION_HPP
