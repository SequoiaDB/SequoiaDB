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

   Source File Name = sptDBSequence.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/09/2020  LSQ Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_SEQ_HPP
#define SPT_DB_SEQ_HPP
#include "client.hpp"
#include "sptApi.hpp"
namespace engine
{
   #define SPT_SEQ_NAME_FIELD         "_name"
   #define SPT_SEQ_CONN_FIELD         "_conn"
   #define SPT_SEQ_NEXT_VALUE_FIELD   "NextValue"
   #define SPT_SEQ_RETURN_NUM_FIELD   "ReturnNum"
   #define SPT_SEQ_INCREMENT_FIELD    "Increment"

   class _sptDBSequence : public SDBObject
   {
   JS_DECLARE_CLASS( _sptDBSequence )
   public:
      _sptDBSequence( sdbclient::_sdbSequence *pSequence = NULL ) ;
      ~_sptDBSequence() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 setAttributes( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      INT32 getNextValue( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      INT32 getCurrentValue( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      INT32 setCurrentValue( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      INT32 fetch( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 restart( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      static INT32 cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg ) ;
      static INT32 fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg ) ;
      static INT32 bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail ) ;
   private:
      sdbclient::sdbSequence _sequence ;
   } ;
   typedef _sptDBSequence sptDBSequence ;
}

#endif
