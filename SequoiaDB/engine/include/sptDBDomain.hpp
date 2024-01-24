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

   Source File Name = sptDBDomain.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_DB_DOMAIN_HPP
#define SPT_DB_DOMAIN_HPP
#include "client.hpp"
#include "sptApi.hpp"
namespace engine
{
   #define SPT_DOMAIN_NAME_FIELD   "_domainname"
   class _sptDBDomain : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBDomain )
      public:
         _sptDBDomain( sdbclient::_sdbDomain *pDomain = NULL ) ;
         virtual ~_sptDBDomain() ;
      public:
         INT32 construct( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;
         INT32 destruct() ;

         INT32 alter( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

         INT32 listCL( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

         INT32 listCS( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

         INT32 listGroup( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

         INT32 addGroups( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

         INT32 removeGroups( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

         INT32 setGroups( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

         INT32 setActiveLocation( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

         INT32 setLocation( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

         INT32 setAttributes( const _sptArguments &arg,
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
         sdbclient::sdbDomain _domain ;
   };
   typedef _sptDBDomain sptDBDomain ;

   #define SPT_SET_DOMAIN_TO_RETURNVAL( pDomain )\
      do\
      {\
         sptDBDomain *__sptDomain__ = SDB_OSS_NEW sptDBDomain( pDomain ) ;\
         if( NULL == __sptDomain__ )\
         {\
            rc = SDB_OOM ;\
            detail = BSON( SPT_ERR << "Failed to alloc memory for sptDBDomain" ) ;\
            goto error ;\
         }\
         rc = rval.getReturnVal().assignUsrObject< sptDBDomain >( __sptDomain__ ) ;\
         if( SDB_OK != rc )\
         {\
            SAFE_OSS_DELETE( __sptDomain__ ) ;\
            pDomain = NULL ;\
            detail = BSON( SPT_ERR << "Failed to set return obj" ) ;\
            goto error ;\
         }\
      }while(0)

}
#endif
