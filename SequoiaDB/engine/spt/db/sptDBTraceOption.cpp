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

   Source File Name = sptDBTraceOption.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/03/2019  FJ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBTraceOption.hpp"
#include "sptBsonobj.hpp"
#include "msgDef.h"

using namespace bson ;

namespace engine
{

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBTraceOption, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBTraceOption, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBTraceOption, help )

   JS_BEGIN_MAPPING( _sptDBTraceOption, SPT_TRACEOPTION_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBTraceOption::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBTraceOption::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBTraceOption::bsonToJSObj )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
   JS_MAPPING_END()

   _sptDBTraceOption::_sptDBTraceOption()
   {
   }

   _sptDBTraceOption::~_sptDBTraceOption()
   {
   }

   INT32 _sptDBTraceOption::construct( const _sptArguments &arg,
                                       _sptReturnVal &rval,
                                       bson::BSONObj &detail )
   {
      std::vector<string> components ;
      std::vector<string> breakPoints ;
      std::vector<UINT32> tids ;
      std::vector<string> functionNames ;
      std::vector<string> threadTypes ;

      rval.addSelfProperty( SPT_TRACEOPTION_COMPONENTS_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( components ) ;
      rval.addSelfProperty( SPT_TRACEOPTION_BREAKPOINTS_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( breakPoints ) ;
      rval.addSelfProperty( SPT_TRACEOPTION_TIDS_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( tids ) ;
      rval.addSelfProperty( SPT_TRACEOPTION_FUNCTIONNAMES_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( functionNames ) ;
      rval.addSelfProperty( SPT_TRACEOPTION_THREADTYPES_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( threadTypes ) ;
      return SDB_OK ;
   }

   INT32 _sptDBTraceOption::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBTraceOption::cvtToBSON( const CHAR* key,
                                       const sptObject &value,
                                       BOOLEAN isSpecialObj,
                                       BSONObjBuilder& builder,
                                       string &errMsg )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = fmpToBSON( value, obj, errMsg ) ;
      if ( rc )
      {
         goto error ;
      }

      builder.append( key, obj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBTraceOption::fmpToBSON( const sptObject &value,
                                       BSONObj &retObj,
                                       string &errMsg )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder retBuilder ;
      sptObjectPtr ptr ;
      bson::BSONObj tmpObj ;

      // append component field
      if ( value.isFieldExist( SPT_TRACEOPTION_COMPONENTS_FIELD ) )
      {
         rc = value.getObjectField( SPT_TRACEOPTION_COMPONENTS_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get component data field" ;
            goto error ;
         }

         rc = ptr->toBSON( tmpObj, true ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert component data to BSON" ;
            goto error;
         }

         retBuilder.appendArray( FIELD_NAME_COMPONENTS, tmpObj ) ;
      }

      // append breakPoint field
      if ( value.isFieldExist( SPT_TRACEOPTION_BREAKPOINTS_FIELD ) )
      {
         rc = value.getObjectField( SPT_TRACEOPTION_BREAKPOINTS_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get breakPoint data field" ;
            goto error ;
         }

         rc = ptr->toBSON( tmpObj, true ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert breakPoint data to BSON" ;
            goto error;
         }

         retBuilder.appendArray( FIELD_NAME_BREAKPOINTS, tmpObj ) ;
      }

      // append tid field
      if ( value.isFieldExist( SPT_TRACEOPTION_TIDS_FIELD ) )
      {
         rc = value.getObjectField( SPT_TRACEOPTION_TIDS_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get tid data field" ;
            goto error ;
         }

         rc = ptr->toBSON( tmpObj, true ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert tid data to BSON" ;
            goto error;
         }

         retBuilder.appendArray( FIELD_NAME_THREADS, tmpObj ) ;
      }

      // append functionName field
      if ( value.isFieldExist( SPT_TRACEOPTION_FUNCTIONNAMES_FIELD ) )
      {
         rc = value.getObjectField( SPT_TRACEOPTION_FUNCTIONNAMES_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get functionNames data field" ;
            goto error ;
         }

         rc = ptr->toBSON( tmpObj, true ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert functionName data to BSON" ;
            goto error;
         }

         retBuilder.appendArray( FIELD_NAME_FUNCTIONNAMES, tmpObj ) ;
      }

      // append threadType field
      if ( value.isFieldExist( SPT_TRACEOPTION_THREADTYPES_FIELD ) )
      {
         rc = value.getObjectField( SPT_TRACEOPTION_THREADTYPES_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get threadTypes data field" ;
            goto error ;
         }

         rc = ptr->toBSON( tmpObj, true ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert threadType data to BSON" ;
            goto error;
         }

         retBuilder.appendArray( FIELD_NAME_THREADTYPES, tmpObj ) ;
      }

      retObj = retBuilder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBTraceOption::bsonToJSObj( sdbclient::sdb &db,
                                         const BSONObj &data,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sptDBTraceOption *pTraceOption = NULL ;

      pTraceOption = SDB_OSS_NEW _sptDBTraceOption() ;
      if( NULL == pTraceOption )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new _sptDBTraceOption obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< _sptDBTraceOption >( pTraceOption ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }

      _setReturnVal( data, rval ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pTraceOption ) ;
      goto done ;
   }

   void _sptDBTraceOption::_setReturnVal( const BSONObj &data,
                                          _sptReturnVal &rval )
   {
      BSONObj obj ;

      obj = data.getObjectField( FIELD_NAME_COMPONENTS ) ;
      rval.addReturnValProperty( SPT_TRACEOPTION_COMPONENTS_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue
                                 ( obj.toString( true, true ) ) ;

      obj = data.getObjectField( FIELD_NAME_BREAKPOINTS ) ;
      rval.addReturnValProperty( SPT_TRACEOPTION_BREAKPOINTS_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue
                                 ( obj.toString( true, true ) ) ;

      obj = data.getObjectField( FIELD_NAME_THREADS ) ;
      rval.addReturnValProperty( SPT_TRACEOPTION_TIDS_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue
                                 ( obj.toString( true, true ) ) ;

      obj = data.getObjectField( FIELD_NAME_FUNCTIONNAMES ) ;
      rval.addReturnValProperty( SPT_TRACEOPTION_FUNCTIONNAMES_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue
                                 ( obj.toString( true, true ) ) ;

      obj = data.getObjectField( FIELD_NAME_THREADTYPES ) ;
      rval.addReturnValProperty( SPT_TRACEOPTION_THREADTYPES_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue
                                 ( obj.toString( true, true ) ) ;
   }

   INT32 _sptDBTraceOption::help( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"SdbTraceOption\" : " << endl ;
      ss << "   SdbTraceOption[.components( <component1>"
         << "[, component2, ... ] )]" << endl ;
      ss << "                 [.breakPoints( <breakPoint1>"
         << "[, breakPoint2, ... ] )]      " << endl ;
      ss << "                 [.tids( <tid1>[, tid2, ... ] )]     " << endl ;
      ss << "                 [.functionNames( <functionName1>"
         << "[, functionName2, ... ] )]" << endl ;
      ss << "                 [.threadTypes( <threadType1>"
         << "[, threadType2, ... ] )]  " << endl << endl ;
      ss << "   SdbTraceOption[.components( [ <component1>"
         << ", <component2>, ... ] ] )]" << endl ;
      ss << "                 [.breakPoints( [ <breakPoint1>"
         << ", <breakPoint2>, ... ] ] )]      " << endl ;
      ss << "                 [.tids( [ <tid1>, <tid2>, ... ] ] )]" << endl ;
      ss << "                 [.functionNames( [ <functionName1>"
         << ", <functionName2>, ... ] ] )]" << endl ;
      ss << "                 [.threadTypes( [ <threadType1>"
         << ", <threadType2>, ... ] ] )]" << endl ;
      ss << endl ;
      ss << "                              "
         << "- Create a SdbTraceOption object" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"SdbTraceOption\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"SdbTraceOption\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

}
