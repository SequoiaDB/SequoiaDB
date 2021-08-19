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

   Source File Name = sptDBOptionBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/07/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBOptionBase.hpp"
#include "sptBsonobj.hpp"

using namespace bson ;

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBOptionBase, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBOptionBase, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBOptionBase, help )

   JS_BEGIN_MAPPING( _sptDBOptionBase, SPT_OPTIONBASE_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBOptionBase::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBOptionBase::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBOptionBase::bsonToJSObj )
      JS_ADD_STATIC_FUNC( "help", help )
   JS_MAPPING_END()

   _sptDBOptionBase::_sptDBOptionBase()
   {
   }

   _sptDBOptionBase::~_sptDBOptionBase()
   {
   }

   INT32 _sptDBOptionBase::construct( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      rval.addSelfProperty( SPT_OPTIONBASE_COND_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( BSONObj() ) ;
      rval.addSelfProperty( SPT_OPTIONBASE_SEL_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( BSONObj() ) ;
      rval.addSelfProperty( SPT_OPTIONBASE_SORT_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( BSONObj() ) ;
      rval.addSelfProperty( SPT_OPTIONBASE_HINT_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( BSONObj() ) ;
      rval.addSelfProperty( SPT_OPTIONBASE_SKIP_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( 0 ) ;
      rval.addSelfProperty( SPT_OPTIONBASE_LIMIT_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( -1 ) ;
      rval.addSelfProperty( SPT_OPTIONBASE_FLAGS_FIELD,
                            SPT_PROP_ENUMERATE )->setValue( 0 ) ;

      return SDB_OK ;
   }

   INT32 _sptDBOptionBase::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBOptionBase::cvtToBSON( const CHAR* key,
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

   INT32 _sptDBOptionBase::fmpToBSON( const sptObject &value,
                                      BSONObj &retObj,
                                      string &errMsg )
   {
      INT32 rc = SDB_OK ;
      sptObjectPtr ptr ;
      sptBsonobj *pBsonObj = NULL ;

      BSONObj cond ;
      BSONObj sel ;
      BSONObj sort ;
      BSONObj hint ;
      INT32 skip = 0 ;
      INT32 limit = -1 ;
      INT32 flags = 0 ;

      if ( value.isFieldExist( SPT_OPTIONBASE_COND_FIELD ) )
      {
         rc = value.getObjectField( SPT_OPTIONBASE_COND_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get cond data field" ;
            goto error ;
         }

         rc = ptr->getUserObj( _sptBsonobj::__desc,
                               (const void **)&pBsonObj ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to contruct Bson from cond data field" ;
            goto error ;
         }
         cond = pBsonObj->getBson() ;
      }

      if ( value.isFieldExist( SPT_OPTIONBASE_SEL_FIELD ) )
      {
         rc = value.getObjectField( SPT_OPTIONBASE_SEL_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get sel type field" ;
            goto error ;
         }

         rc = ptr->getUserObj( _sptBsonobj::__desc,
                               (const void **)&pBsonObj ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to contruct Bson from sel data field" ;
            goto error ;
         }
         sel = pBsonObj->getBson() ;
      }

      if ( value.isFieldExist( SPT_OPTIONBASE_SORT_FIELD ) )
      {
         rc = value.getObjectField( SPT_OPTIONBASE_SORT_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get sort type field" ;
            goto error ;
         }

         rc = ptr->getUserObj( _sptBsonobj::__desc,
                               (const void **)&pBsonObj ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to contruct Bson from sort data field" ;
            goto error ;
         }
         sort = pBsonObj->getBson() ;
      }

      if ( value.isFieldExist( SPT_OPTIONBASE_HINT_FIELD ) )
      {
         rc = value.getObjectField( SPT_OPTIONBASE_HINT_FIELD, ptr ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get hint type field" ;
            goto error ;
         }

         rc = ptr->getUserObj( _sptBsonobj::__desc,
                               (const void **)&pBsonObj ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to contruct Bson from hint data field" ;
            goto error ;
         }
         hint = pBsonObj->getBson() ;
      }

      if ( value.isFieldExist( SPT_OPTIONBASE_SKIP_FIELD ) )
      {
         rc = value.getIntField( SPT_OPTIONBASE_SKIP_FIELD, skip ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get SdbSnapshotOption skip field" ;
            goto error ;
         }
      }

      if ( value.isFieldExist( SPT_OPTIONBASE_LIMIT_FIELD ) )
      {
         rc = value.getIntField( SPT_OPTIONBASE_LIMIT_FIELD, limit ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get SdbSnapshotOption limit field" ;
            goto error ;
         }
      }

      if ( value.isFieldExist( SPT_OPTIONBASE_FLAGS_FIELD ) )
      {
         rc = value.getIntField( SPT_OPTIONBASE_FLAGS_FIELD, flags ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to get SdbSnapshotOption flags field" ;
            goto error ;
         }
      }

      retObj = BSON( SPT_OPTIONBASE_COND_FIELD << cond <<
                     SPT_OPTIONBASE_SEL_FIELD << sel <<
                     SPT_OPTIONBASE_SORT_FIELD << sort <<
                     SPT_OPTIONBASE_HINT_FIELD << hint <<
                     SPT_OPTIONBASE_SKIP_FIELD << skip <<
                     SPT_OPTIONBASE_LIMIT_FIELD << limit <<
                     SPT_OPTIONBASE_FLAGS_FIELD << flags ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBOptionBase::bsonToJSObj( sdbclient::sdb &db,
                                        const BSONObj &data,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBOptionBase *pOptionBase = NULL ;

      pOptionBase = SDB_OSS_NEW sptDBOptionBase() ;
      if( NULL == pOptionBase )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBOptionBase obj" ) ;
         goto error ;
      }

      rc = rval.setUsrObjectVal< sptDBOptionBase >( pOptionBase ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set ret obj" ) ;
         goto error ;
      }

      _setReturnVal( data, rval ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pOptionBase ) ;
      goto done ;
   }

   void _sptDBOptionBase::_setReturnVal( const BSONObj &data,
                                         _sptReturnVal &rval )
   {
      BSONObj obj ;

      /// _cond
      obj = data.getObjectField( SPT_OPTIONBASE_COND_FIELD ) ;
      rval.addReturnValProperty( SPT_OPTIONBASE_COND_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( obj ) ;
      /// _sel
      obj = data.getObjectField( SPT_OPTIONBASE_SEL_FIELD ) ;
      rval.addReturnValProperty( SPT_OPTIONBASE_SEL_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( obj ) ;

      /// _sort
      obj = data.getObjectField( SPT_OPTIONBASE_SORT_FIELD ) ;
      rval.addReturnValProperty( SPT_OPTIONBASE_SORT_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( obj ) ;

      /// _hint
      obj = data.getObjectField( SPT_OPTIONBASE_HINT_FIELD ) ;
      rval.addReturnValProperty( SPT_OPTIONBASE_HINT_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( obj ) ;

      /// _skip
      INT32   skip = 0 ;
      if ( data.hasField( SPT_OPTIONBASE_SKIP_FIELD ) )
      {
         skip = data.getIntField( SPT_OPTIONBASE_SKIP_FIELD ) ;
      }
      rval.addReturnValProperty( SPT_OPTIONBASE_SKIP_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( skip ) ;

      /// _limit
      INT32   limit = -1 ;
      if ( data.hasField( SPT_OPTIONBASE_LIMIT_FIELD ) )
      {
         limit = data.getIntField( SPT_OPTIONBASE_LIMIT_FIELD ) ;
      }
      rval.addReturnValProperty( SPT_OPTIONBASE_LIMIT_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( limit ) ;

      /// _flags
      INT32   flags = 0 ;
      if ( data.hasField( SPT_OPTIONBASE_FLAGS_FIELD ) )
      {
         flags = data.getIntField( SPT_OPTIONBASE_FLAGS_FIELD ) ;
      }
      rval.addReturnValProperty( SPT_OPTIONBASE_FLAGS_FIELD,
                                 SPT_PROP_ENUMERATE )->setValue( flags ) ;
   }

   INT32 _sptDBOptionBase::help( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      stringstream ss ;
      ss << "--Constructor methods for class SdbOptionBase : " << endl ;
      ss << "   SdbOptionBase[.cond(<cond>)]" << endl ;
      ss << "                [.sel(<sel>)]" << endl ;
      ss << "                [.sort(<sort>)]" << endl ;
      ss << "                [.options(<options>)]" << endl ;
      ss << "                [.skip(<skipNum>)]" << endl ;
      ss << "                [.limit(<retNum>)]   "
         << "-- Create a SdbOptionBase object" << endl ;
      ss << "--Static methods for class SdbOptionBase : " << endl ;
      ss << "--Instance methods for class SdbOptionBase : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}

