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

   Source File Name = sptConvertor.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  YW Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossUtil.hpp"
#include "sptConvertor.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "sptObjDesc.hpp"
#include "sptSPObject.hpp"
#include <typeinfo>
using namespace bson ;
using namespace engine ;

namespace engine
{

   #define SPT_SPTOBJ_DUMMY_FIELD         "__dummy__"

   /*
      sptConvertor implement
   */
   INT32 sptConvertor::toBson( JSObject *obj ,
                               bson::BSONObj &bsobj,
                               BOOLEAN *pIsArray )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder( 1024 ) ;
      sptSPVal spVal ;
      const sptObjDesc *pDesc = NULL ;

      SDB_ASSERT( NULL != obj, "can not be NULL" ) ;

      try
      {
         _hasSetErrMsg = FALSE ;

         if ( pIsArray && JS_IsArrayObject( _cx, obj ) )
         {
            *pIsArray = TRUE ;
         }

         spVal.reset( _cx, OBJECT_TO_JSVAL( obj ) ) ;

         /// When is spt object
         if ( spVal.isSPTObject( NULL, NULL, &pDesc ) &&
              pDesc->hasCVTToBSONFunc() )
         {
            string errMsg ;
            BSONElement e ;
            BSONObj tmpObj ;
            rc = pDesc->cvtToBSON( SPT_SPTOBJ_DUMMY_FIELD,
                                   sptSPObject( _cx, obj ),
                                   FALSE, builder, errMsg ) ;
            if ( rc )
            {
               _setErrMsg( errMsg, FALSE ) ;
               goto error ;
            }
            tmpObj = builder.obj() ;
            e = tmpObj.getField( SPT_SPTOBJ_DUMMY_FIELD ) ;
            if ( Object != e.type() )
            {
               rc = SDB_INVALIDARG ;
               _setErrMsg( "Specail object can't convert to BSONObj", FALSE ) ;
               goto error ;
            }
            bsobj = e.embeddedObject().copy() ;
         }
         else
         {
            rc = _traverse( obj, builder ) ;
            if ( SDB_OK != rc )
            {
               _setErrMsg( "Failed to call traverse", FALSE ) ;
               goto error ;
            }
            bsobj = builder.obj() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         _setErrMsg( e.what(), FALSE ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sptConvertor::toBson( const sptSPVal *pVal,
                               BSONObj &obj,
                               BOOLEAN *pIsArray )
   {
      INT32 rc = SDB_OK ;
      JSObject *pJSObj = NULL ;

      _hasSetErrMsg = FALSE ;
      if ( !pVal->isObject() )
      {
         _setErrMsg( "Value is not Object", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !JS_ValueToObject( _cx, *(pVal->valuePtr()), &pJSObj ) )
      {
         _setErrMsg( "Convert to JSObject failed", FALSE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = toBson( pJSObj, obj, pIsArray ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sptConvertor::appendToBson( const string &key,
                                     const sptSPVal &val,
                                     BSONObjBuilder &builder )
   {
      _hasSetErrMsg = FALSE ;
      return _appendToBson( key, val, builder ) ;
   }

   INT32 sptConvertor::toObjArray( JSObject *obj, vector< BSONObj > &bsArray )
   {
      INT32 rc = SDB_OK ;
      UINT32 length = 0 ;

      _hasSetErrMsg = FALSE ;

      if ( NULL == obj )
      {
         goto done ;
      }
      if( !JS_IsArrayObject( _cx, obj ) )
      {
         rc = SDB_INVALIDARG ;
         _setErrMsg( "Object is not an array obj", FALSE ) ;
         goto error ;
      }
      if( !JS_GetArrayLength( _cx, obj, &length ) )
      {
         rc = SDB_INVALIDARG ;
         _setErrMsg( "Failed to get array length", FALSE ) ;
         goto error ;
      }

      for ( UINT32 i = 0; i < length; i++ )
      {
         BSONObj tmpObj ;
         JSObject *eleObj = NULL ;
         jsval val ;
         if( !JS_GetElement( _cx, obj, i, &val ) )
         {
            rc = SDB_INVALIDARG ;
            _setErrMsg( "Failed to get array element", FALSE ) ;
            goto error ;
         }
         if( JSTYPE_OBJECT != JS_TypeOfValue( _cx, val ) )
         {
            rc = SDB_INVALIDARG ;
            _setErrMsg( "Invalid element type", FALSE ) ;
            goto error ;
         }
         eleObj = JSVAL_TO_OBJECT( val ) ;
         rc = toBson( eleObj, tmpObj ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
         bsArray.push_back( tmpObj ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sptConvertor::toStrArray( JSObject *obj, vector< string > &bsArray )
   {
      INT32 rc = SDB_OK ;
      UINT32 length = 0 ;
      sptSPVal value ;

      _hasSetErrMsg = FALSE ;

      if ( NULL == obj )
      {
         goto done ;
      }
      if( !JS_IsArrayObject( _cx, obj ) )
      {
         rc = SDB_INVALIDARG ;
         _setErrMsg( "Object is not an array obj", FALSE ) ;
         goto error ;
      }
      if( !JS_GetArrayLength( _cx, obj, &length ) )
      {
         rc = SDB_INVALIDARG ;
         _setErrMsg( "Failed to get array length", FALSE ) ;
         goto error ;
      }

      for ( UINT32 i = 0; i < length; i++ )
      {
         std::string str ;
         jsval val ;
         if( !JS_GetElement( _cx, obj, i, &val ) )
         {
            rc = SDB_INVALIDARG ;
            _setErrMsg( "Failed to get array element", FALSE ) ;
            goto error ;
         }
         if( JSTYPE_STRING != JS_TypeOfValue( _cx, val ) )
         {
            rc = SDB_INVALIDARG ;
            _setErrMsg( "Invalid element type", FALSE ) ;
            goto error ;
         }
         value.reset( _cx, val ) ;
         rc = value.toString( str ) ;
         if ( SDB_OK != rc )
         {
            _setErrMsg( "Failed to conversion string", FALSE ) ;
            goto error ;
         }
         bsArray.push_back( str ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sptConvertor::_traverse( JSObject *obj,
                                  BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      JSIdArray *properties = NULL ;
      sptSPVal key, value ;

      if ( NULL == obj )
      {
         goto done ;
      }

      properties = JS_Enumerate( _cx, obj ) ;
      if ( NULL == properties )
      {
         _setErrMsg( "System error", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( jsint i = 0; i < properties->length; i++ )
      {
         jsid id = properties->vector[i] ;
         jsval fieldName, fieldValue ;
         std::string name ;

         if ( !JS_IdToValue( _cx, id, &fieldName ) )
         {
            _setErrMsg( "Failed to get object field name", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         key.reset( _cx, fieldName ) ;
         rc = key.toString( name ) ;
         if ( SDB_OK != rc )
         {
            _setErrMsg( "Failed to conversion field name", FALSE ) ;
            goto error ;
         }

         if ( !JS_GetProperty( _cx, obj, name.c_str(), &fieldValue ) )
         {
            _setErrMsg( "Failed to get object field value", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         value.reset( _cx, fieldValue ) ;

         rc = _appendToBson( name, value, builder ) ;
         if ( SDB_OK != rc )
         {
            _setErrMsg( "Failed to call appendToBson", FALSE ) ;
            goto error ;
         }
      }

   done:
      if ( properties )
      {
         JS_DestroyIdArray( _cx, properties ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 sptConvertor::_appendToBson( const std::string &name,
                                      const sptSPVal &val,
                                      BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( val.isVoid() )
         {
         }
         else if ( val.isNull() )
         {
            builder.appendNull( name ) ;
         }
         else if ( val.isInt() )
         {
            INT32 v = 0 ;
            rc = val.toInt( v ) ;
            if ( rc )
            {
               _setErrMsg( "Failed to convert to int", FALSE ) ;
               goto error ;
            }
            builder.append( name, v ) ;
         }
         else if ( val.isDouble() )
         {
            FLOAT64 v = 0.0 ;
            rc = val.toDouble( v ) ;
            if ( rc )
            {
               _setErrMsg( "Failed to convert to double", FALSE ) ;
               goto error ;
            }
            builder.append( name, v ) ;
         }
         else if ( val.isString() )
         {
            std::string v ;
            rc = val.toString( v ) ;
            if ( rc )
            {
               _setErrMsg( "Failed to convert to string", FALSE ) ;
               goto error ;
            }
            builder.append( name, v ) ;
         }
         else if ( val.isBoolean() )
         {
            BOOLEAN v = FALSE ;
            rc = val.toBoolean( v ) ;
            if ( rc )
            {
               _setErrMsg( "Failed to convert to boolean", FALSE ) ;
               goto error ;
            }
            builder.appendBool( name, v ) ;
         }
         else if ( val.isFunctionObj() )
         {
            string v ;
            rc = val.toString( v ) ;
            if ( rc )
            {
               _setErrMsg( "Failed to convert to string", FALSE ) ;
               goto error ;
            }
            builder.appendCode( name, v ) ;
         }
         else if ( val.isObject() )
         {
            const sptObjDesc *desc = NULL ;
            BOOLEAN isSpecialObj = FALSE ;
            BOOLEAN hasDone = FALSE ;
            JSObject *obj = JSVAL_TO_OBJECT( *(val.valuePtr()) ) ;

            if ( NULL == obj )
            {
               _setErrMsg( "Failed to convert to object", FALSE ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( val.isSPTObject( &isSpecialObj, NULL, &desc ) )
            {
               string errMsg ;
               rc = desc->cvtToBSON( name.c_str(), sptSPObject( _cx, obj ),
                                     isSpecialObj, builder, errMsg ) ;
               if ( SDB_OK == rc )
               {
                  hasDone = TRUE ;
               }
               else if ( SDB_SPT_NOT_SPECIAL_JSON == rc )
               {
                  if ( _strict )
                  {
                     _setErrMsg( errMsg, FALSE ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
               }
               else
               {
                  _setErrMsg( errMsg, FALSE ) ;
                  goto error ;
               }
            }

            if ( !hasDone )
            {
               BSONObjBuilder subBuild( 1024 ) ;
               BOOLEAN isArray = JS_IsArrayObject( _cx, obj ) ;

               rc = _traverse( obj, subBuild ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( isArray )
               {
                  builder.appendArray( name, subBuild.obj() ) ;
               }
               else
               {
                  builder.append( name, subBuild.obj() ) ;
               }
            }
         }
         else
         {
            _setErrMsg( "unexpected type", FALSE ) ;
            SDB_ASSERT( FALSE, "unexpected type" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         _setErrMsg( e.what(), FALSE ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sptConvertor::toString( JSContext *cx,
                                 const jsval &val,
                                 std::string &str )
   {

      sptSPVal spVal( cx, val ) ;
      return spVal.toString( str ) ;
   }

   void sptConvertor::_setErrMsg( const string &msg, BOOLEAN isReplace )
   {
      if( _hasSetErrMsg == TRUE && isReplace == FALSE )
      {
         return ;
      }
      _hasSetErrMsg = TRUE ;
      _errMsg = msg ;
   }

   const string& sptConvertor::getErrMsg() const
   {
      return _errMsg ;
   }

}

