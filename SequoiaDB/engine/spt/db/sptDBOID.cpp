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

   Source File Name = sptDBOID.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBOID.hpp"
#include "utilStr.hpp"
using namespace bson ;
namespace engine
{
   #define SPT_OID_NAME       "ObjectId"
   #define SPT_OID_STR_FIELD  "_str"
   #define SPT_OID_SPECIAL_FIELD  "$oid"
   #define SPT_OID_STR_LENGTH 24

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBOID, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBOID, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBOID, help )

   JS_BEGIN_MAPPING( _sptDBOID, SPT_OID_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_OID_SPECIAL_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBOID::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBOID::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBOID::bsonToJSObj )
   JS_MAPPING_END()
   _sptDBOID::_sptDBOID()
   {
   }

   _sptDBOID::~_sptDBOID()
   {
   }

   INT32 _sptDBOID::construct( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oidStr ;

      rc = arg.getString( 0, oidStr ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         OID oid = OID::gen() ;
         oidStr = oid.str() ;
         rc = SDB_OK ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "ObjectId argument must be String" ) ;
         goto error ;
      }
      else
      {
         if( SPT_OID_STR_LENGTH != oidStr.size() )
         {
            stringstream ss ;
            ss << "The length of ObjectId argument is not equal " << SPT_OID_STR_LENGTH ;
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << ss.str() ) ;
            goto error ;
         }

         if( !utilIsValidOID( oidStr.c_str() ) )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "invalid ObjectId value: " + oidStr ) ;
            goto error ;
         }
      }
      rval.addSelfProperty( SPT_OID_STR_FIELD )->setValue( oidStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBOID::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBOID::cvtToBSON( const CHAR* key, const sptObject &value,
                               BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                               string &errMsg )
   {
      string data ;
      string fieldName ;
      INT32 rc = SDB_OK ;
      if( isSpecialObj )
      {
         fieldName = SPT_OID_SPECIAL_FIELD ;
      }
      else
      {
         fieldName = SPT_OID_STR_FIELD ;
      }
      rc = value.getStringField( fieldName, data ) ;
      if( SDB_OK != rc )
      {
         errMsg = "ObjectId oid value must be String" ;
         goto error ;
      }
      if( SPT_OID_STR_LENGTH != data.size() )
      {
         stringstream ss ;
         ss << "The length of oid value is not equal " << SPT_OID_STR_LENGTH ;
         errMsg = ss.str() ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( !utilIsValidOID( data.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         errMsg = "invalid ObjectId value: " + data ;
         goto error ;
      }

      {
         OID oid( data ) ;
         builder.appendOID( key, &oid ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBOID::fmpToBSON( const sptObject &value, BSONObj &retObj,
                               string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string data ;
      rc = value.getStringField( SPT_OID_STR_FIELD, data ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get oid _str field" ;
         goto error ;
      }
      retObj = BSON( SPT_OID_STR_FIELD << data ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBOID::bsonToJSObj( sdbclient::sdb &db,const BSONObj &data,
                                 _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oidStr ;
      oidStr = data.getStringField( SPT_OID_STR_FIELD ) ;
      sptDBOID *pOid = SDB_OSS_NEW sptDBOID() ;
      if( NULL == pOid )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBOID obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBOID >( pOid ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      rval.addReturnValProperty( SPT_OID_STR_FIELD )->setValue( oidStr ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pOid ) ;
      goto done ;
   }

   INT32 _sptDBOID::help( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          BSONObj &detail )
   {
      stringstream ss ;
      ss << "--Constructor methods for class ObjectId : " << endl ;
      ss << "   { \"$oid\": <data> }   " << endl ;
      ss << "   ObjectId( [data] )     "
         << "-- Data type: object id( OID )" << endl ;
      ss << "--Static methods for class ObjectId : " << endl ;
      ss << "--Instance methods for class ObjectId : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}
