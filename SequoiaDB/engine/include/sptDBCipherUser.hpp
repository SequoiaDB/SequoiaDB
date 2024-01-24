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

   Source File Name = sptDBCipherUser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          26/05/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_CIPHER_USER_HPP
#define SPT_DB_CIPHER_USER_HPP

#include "sptApi.hpp"

namespace engine
{
   #define SPT_CIPHERUSER_NAME                      "CipherUser"
   #define SPT_CIPHERUSER_USER_FIELD                "_user"
   #define SPT_CIPHERUSER_CLUSTER_NAME_FIELD        "_clusterName"
   #define SPT_CIPHERUSER_CIPHER_FILE_FIELD         "_cipherFile"

   #define SPT_CIPHERUSER_FIELD_NAME_CLUSTER_NAME   "ClusterName"
   #define SPT_CIPHERUSER_FIELD_NAME_CIPHER_FILE    "CipherFile"

   class _sptDBCipherUser : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBCipherUser )
   public:
      _sptDBCipherUser() ;
      virtual ~_sptDBCipherUser() ;
   public:
      INT32          construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;
      INT32          destruct() ;
      INT32          setToken( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;
      const CHAR*    getToken() ;
      static INT32   cvtToBSON( const CHAR* key,
                                const sptObject &value,
                                BOOLEAN isSpecialObj,
                                BSONObjBuilder& builder,
                                string &errMsg ) ;
      static INT32   fmpToBSON( const sptObject &value,
                                BSONObj &retObj,
                                string &errMsg ) ;
      static INT32   bsonToJSObj( sdbclient::sdb &db,
                                  const BSONObj &data,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;
      static INT32   help( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;
   private:
      string _token ;
   } ;

   typedef _sptDBCipherUser sptDBCipherUser ;

}

#endif

