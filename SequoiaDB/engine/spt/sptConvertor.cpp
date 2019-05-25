/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "utilStr.hpp"
#include "../client/base64c.h"
#include "../client/jstobs.h"
#include <boost/lexical_cast.hpp>
#include "sptSPDef.hpp"

#define SPT_CONVERTOR_SPE_OBJSTART '$'
#define SPT_SPEOBJ_MINKEY "$minKey"
#define SPT_SPEOBJ_MAXKEY "$maxKey"
#define SPT_SPEOBJ_TIMESTAMP "$timestamp"
#define SPT_SPEOBJ_DATE "$date"
#define SPT_SPEOBJ_REGEX "$regex"
#define SPT_SPEOBJ_OPTION "$options"
#define SPT_SPEOBJ_BINARY "$binary"
#define SPT_SPEOBJ_TYPE "$type"
#define SPT_SPEOBJ_OID "$oid"
#define SPT_SPEOBJ_NUMBERLONG "$numberLong"
#define SPT_SPEOBJ_DECIMAL "$decimal"
#define SPT_SPEOBJ_PRESICION "$precision"
#define SPT_AGGREGATE_MATCHER "$match"

/*
#define SDB_DATE_TYPE_CHECK_BOUND(tm)                 \
   do {                                               \
      if( (INT64)tm/1000 < TIME_STAMP_DATE_MIN ||     \
          (INT64)tm/1000 > TIME_STAMP_DATE_MAX ) {    \
         rc = SDB_INVALIDARG ;                        \
         goto error ;                                 \
      }                                               \
   } while( 0 )
*/
#define SDB_TIMESTAMP_TYPE_CHECK_BOUND(tm)            \
   do {                                               \
      if ( !ossIsTimestampValid( ( INT64 )tm ) ) {   \
           rc = SDB_INVALIDARG ;                      \
           goto error ;                               \
      }                                               \
   } while( 0 )

extern JSBool is_objectid( JSContext *, JSObject * ) ;
extern JSBool is_bindata( JSContext *, JSObject * ) ;
extern JSBool is_jsontypes( JSContext *, JSObject * ) ;
extern JSBool is_timestamp( JSContext *, JSObject * ) ;
extern JSBool is_regex( JSContext *, JSObject * ) ;
extern JSBool is_minkey( JSContext *, JSObject * ) ;
extern JSBool is_maxkey( JSContext *, JSObject * ) ;
extern JSBool is_numberlong( JSContext *, JSObject * ) ;
extern JSBool is_sdbdate( JSContext *, JSObject * ) ;
extern JSBool jsobj_is_sdbobj( JSContext *cx, JSObject *obj ) ;

INT32 sptConvertor::toBson( JSObject *obj , bson **bs )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != _cx && NULL != bs, "can not be NULL" ) ;

   _hasSetErrMsg = FALSE ;

   *bs = bson_create() ;
   if ( NULL == *bs )
   {
      _setErrorMsg( "Failed to allocate memory", FALSE ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   bson_init( *bs ) ;

   rc = _traverse( obj, *bs ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to call traverse", FALSE ) ;
      goto error ;
   }

   rc = bson_finish( *bs ) ;
   if ( rc )
   {
      _setErrorMsg( "Failed to build bson", FALSE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   if ( NULL != *bs )
   {
      bson_dispose( *bs ) ;
      *bs = NULL ;
   }
   goto done ;
}

INT32 sptConvertor::toBson( JSObject *obj, bson *bs )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != obj && NULL != bs, "can not be NULL" ) ;

   _hasSetErrMsg = FALSE ;

   rc = _traverse( obj, bs ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to call traverse", FALSE ) ;
      goto error ;
   }

   rc = bson_finish( bs ) ;
   if ( rc )
   {
      _setErrorMsg( "Failed to build bson", FALSE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

const CHAR* sptConvertor::getErrorMsg()
{
   return _errorMsg.c_str() ;
}

INT32 sptConvertor::_traverse( JSObject *obj , bson *bs )
{
   INT32 rc = SDB_OK ;
   JSIdArray *properties = NULL ;
   if ( NULL == obj )
   {
      goto done ;
   }

   properties = JS_Enumerate( _cx, obj ) ;
   if ( NULL == properties )
   {
      _setErrorMsg( "System error", FALSE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   for ( jsint i = 0; i < properties->length; i++ )
   {
      jsid id = properties->vector[i] ;
      jsval fieldName, fieldValue ;
      std::string name ;
      if ( !JS_IdToValue( _cx, id, &fieldName ))
      {
         _setErrorMsg( "Failed to get object field name", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( fieldName, name ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion field name", FALSE ) ;
         goto error ;
      }

      if ( !JS_GetProperty( _cx, obj, name.c_str(), &fieldValue ))
      {
         _setErrorMsg( "Failed to get object field value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _appendToBson( name, fieldValue, bs ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to call appendToBson", FALSE ) ;
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

INT32 sptConvertor::_addObjectId( JSObject *obj,
                                  const CHAR *key,
                                  bson *bs )
{
   INT32 rc = SDB_OK ;
   std::string strValue ;
   jsval value ;
   if ( !_getProperty( obj, "_str", JSTYPE_STRING, value ))
   {
      _setErrorMsg( "ObjectId argument must be a string", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   rc = _toString( value, strValue ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion ObjectId property", FALSE ) ;
      goto error ;
   }

   if ( 24 != strValue.length() )
   {
      _setErrorMsg( "The length of ObjectId is not equal 24", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   bson_oid_t oid ;
   bson_oid_from_string( &oid, strValue.c_str() ) ;
   bson_append_oid( bs, key, &oid ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addBinData( JSObject *obj,
                                 const CHAR *key,
                                 bson *bs )
{
   INT32 rc = SDB_OK ;
   std::string typeName ;
   std::string strBin, strType ;
   jsval jsBin, jsType ;
   CHAR *decode = NULL ;
   INT32 decodeSize = 0 ;
   UINT32 binType = 0 ;

   if ( !_getProperty( obj, "_data",
                       JSTYPE_STRING, jsBin ))
   {
      _setErrorMsg( "BinData 1st argument must be a string", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !_getProperty( obj, "_type",
                       JSTYPE_STRING, jsType ))
   {
      _setErrorMsg( "BinData 2nd argument must be a string", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   rc = _toString( jsBin, strBin ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion BinData bindata", FALSE ) ;
      goto error ;
   }

   rc = _toString( jsType, strType ) ;
   if ( SDB_OK != rc || strType.empty())
   {
      _setErrorMsg( "Failed to conversion BinData type", FALSE ) ;
      goto error ;
   }

   try
   {
      binType = boost::lexical_cast<INT32>( strType.c_str() ) ;
      if ( binType > 255 )
      {
         _setErrorMsg( "Bad type for binary", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   catch ( std::bad_cast &e )
   {
      _setErrorMsg( "Bad type for binary", FALSE ) ;
      PD_LOG( PDERROR, "bad type for binary:%s", strType.c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   decodeSize = getDeBase64Size( strBin.c_str() ) ;
   if ( decodeSize < 0 )
   {
      _setErrorMsg( "Invalid bindata", FALSE ) ;
      PD_LOG( PDERROR, "invalid bindata %s", strBin.c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if( decodeSize > 0 )
   {
      decode = ( CHAR * )SDB_OSS_MALLOC( decodeSize ) ;
      if ( NULL == decode )
      {
         _setErrorMsg( "Failed to allocate memory", FALSE ) ;
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      memset ( decode, 0, decodeSize ) ;
      if ( base64Decode( strBin.c_str(), decode, decodeSize ) < 0 )
      {
         _setErrorMsg( "Failed to decode base64 code", FALSE ) ;
         PD_LOG( PDERROR, "failed to decode base64 code" ) ;
         rc = SDB_INVALIDARG ;
         SDB_OSS_FREE( decode ) ;
         goto error ;
      }
      bson_append_binary( bs, key, binType,
                          decode, decodeSize - 1 ) ;
   }
   else
   {
      bson_append_binary( bs, key, binType,
                          "", 0 ) ;
   }
done:
   SDB_OSS_FREE( decode ) ;
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addTimestamp( JSObject *obj,
                                   const CHAR *key,
                                   bson *bs )
{
   INT32 rc = SDB_OK ;
   std::string strValue ;
   jsval value ;
   time_t tm ;
   UINT64 usec = 0 ;
   bson_timestamp_t btm ;
   if ( !_getProperty( obj, "_t", JSTYPE_STRING, value ))
   {
      _setErrorMsg( "Timestamp 1st argument must be a string", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   rc = _toString( value, strValue ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion Timestamp property", FALSE ) ;
      goto error ;
   }

   rc = engine::utilStr2TimeT( strValue.c_str(),
                               tm,
                               &usec ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion Timestamp", FALSE ) ;
      goto error ;
   }

   SDB_TIMESTAMP_TYPE_CHECK_BOUND( tm ) ;
   btm.t = tm;
   btm.i = usec ;
   bson_append_timestamp( bs, key, &btm ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addRegex( JSObject *obj,
                               const CHAR *key,
                               bson *bs )
{
   INT32 rc = SDB_OK ;
   std::string optionName ;
   std::string strRegex, strOption ;
   jsval jsRegex, jsOption ;

   if ( !_getProperty( obj, "_regex",
                       JSTYPE_STRING, jsRegex ))
   {
      _setErrorMsg( "Regex 1st argument must be a string", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !_getProperty( obj, "_option",
                       JSTYPE_STRING, jsOption ))
   {
      _setErrorMsg( "Regex 2nd argument must be a string", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   rc = _toString( jsRegex, strRegex ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion Regex regex", FALSE ) ;
      goto error ;
   }

   rc = _toString( jsOption, strOption ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion Regex options", FALSE ) ;
      goto error ;
   }

   bson_append_regex( bs, key, strRegex.c_str(), strOption.c_str() ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addMinKey( JSObject *obj,
                                const CHAR *key,
                                bson *bs )
{
   bson_append_minkey( bs, key ) ;
   return SDB_OK ;
}

INT32 sptConvertor::_addMaxKey( JSObject *obj,
                                const CHAR *key,
                                bson *bs )
{
   bson_append_maxkey( bs, key ) ;
   return SDB_OK ;
}

INT32 sptConvertor::_getNumberLongValue( JSObject *obj, INT64 &value )
{
   INT32 rc = SDB_OK ;
   jsval jsV = JSVAL_VOID ;
   FLOAT64 fv = 0 ;
   string strv ;

   if ( !_getProperty( obj, "_v",
                       JSTYPE_NUMBER, jsV ))
   {
      if ( !_getProperty( obj, "_v",
                          JSTYPE_STRING, jsV ) )
      {
         _setErrorMsg( "NumberLong 1st argument must be a string or a number",
                       FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsV, strv ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
         goto error ;
      }

      try
      {
         value = boost::lexical_cast<INT64>( strv ) ;
      }
      catch ( std::bad_cast &e )
      {
         _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = _toDouble( jsV, fv ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
         goto error ;
      }
      value = fv ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addNumberLong( JSObject *obj,
                                    const CHAR *key,
                                    bson *bs )
{
   INT32 rc = SDB_OK ;
   INT64 n = 0 ;

   rc = _getNumberLongValue( obj, n ) ;
   if ( rc )
   {
      goto error ;
   }
   bson_append_long( bs, key, n ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addSdbDate( JSObject *obj,
                                 const CHAR *key,
                                 bson *bs )
{
   INT32 rc = SDB_OK ;
   std::string strValue ;
   jsval value ;
   UINT64 tm = 0 ;
   bson_date_t datet ;
   if ( _getProperty( obj, "_d", JSTYPE_STRING, value ) )
   {
      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Date", FALSE ) ;
         goto error ;
      }

      rc = engine::utilStr2Date( strValue.c_str(), tm ) ;
      if ( SDB_OK != rc )
      {  // maybe the format is {dateKey:SdbDate("-30610252800000")}
         try
         {
            tm = boost::lexical_cast<UINT64>( strValue.c_str() ) ;
         }
         catch( boost::bad_lexical_cast &e )
         {
            _setErrorMsg( "Failed to conversion Date", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
   }
   else if (  _getProperty( obj, "_d", JSTYPE_NUMBER, value ) )
   {
      FLOAT64 fv = 0 ;
      rc = _toDouble( value, fv ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Date", FALSE ) ;
         goto error ;
      }
      tm = fv ;
   }
   else
   {
      _setErrorMsg( "Date 1st argument must be a string or a number", FALSE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   datet = tm ;
   bson_append_date( bs, key, datet ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addJsonTypes( JSObject *obj,
                                   const CHAR *key,
                                   bson *bs )
{
   INT32 rc = SDB_OK ;
   if ( is_objectid( _cx, obj ) )
   {
      rc = _addObjectId( obj, key, bs ) ;
   }
   else if ( is_bindata( _cx, obj ) )
   {
      rc = _addBinData( obj, key, bs ) ;
   }
   else if ( is_timestamp( _cx, obj ) )
   {
      rc = _addTimestamp( obj, key, bs ) ;
   }
   else if ( is_regex( _cx, obj ) )
   {
      rc = _addRegex( obj, key, bs ) ;
   }
   else if ( is_minkey( _cx, obj ) )
   {
      rc = _addMinKey( obj, key, bs ) ;
   }
   else if ( is_maxkey( _cx, obj ) )
   {
      rc = _addMaxKey( obj, key, bs ) ;
   }
   else if ( is_numberlong( _cx, obj ) )
   {
      rc = _addNumberLong( obj, key, bs ) ;
   }
   else if ( is_sdbdate( _cx, obj ) )
   {
      rc = _addSdbDate( obj, key, bs ) ;
   }
   else
   {
      _setErrorMsg( "Unknow js object type", FALSE ) ;
      rc = SDB_INVALIDARG ;
   }

   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_getDecimalPrecision( const CHAR *precisionStr,
                                          INT32 *precision, INT32 *scale )
{
   BOOLEAN isFirst = TRUE ;
   INT32 rc        = SDB_OK ;
   const CHAR *p   = precisionStr ;
   while ( NULL != p && '\0' != *p )
   {
      if ( ' ' == *p )
      {
         p++ ;
         continue ;
      }

      if ( ',' == *p )
      {
         if ( !isFirst )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         isFirst = FALSE ;
         p++ ;
         continue ;
      }

      if ( *p < '0' || *p > '9' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      p++ ;
   }

   rc = sscanf ( precisionStr, "%d,%d", precision, scale ) ;
   if ( 2 != rc )
   {
      _setErrorMsg( "Invalid decimal", FALSE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = SDB_OK ;

done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_addSpecialObj( JSObject *obj,
                                    const CHAR *key,
                                    bson *bs )
{
   INT32 rc = SDB_OK ;
   JSIdArray *properties = JS_Enumerate( _cx, obj ) ;
   if ( NULL == properties || 0 == properties->length )
   {
      rc = SDB_SPT_NOT_SPECIAL_JSON ;
      goto error ;
   }

   {
   jsid id = properties->vector[0] ;
   jsval fieldName ;
   std::string name ;
   if ( !JS_IdToValue( _cx, id, &fieldName ))
   {
      _setErrorMsg( "Failed to get object field name", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   rc = _toString( fieldName, name ) ;
   if ( SDB_OK != rc )
   {
      _setErrorMsg( "Failed to conversion object field name", FALSE ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   if ( name.length() <= 1 )
   {
      rc = SDB_SPT_NOT_SPECIAL_JSON ;
      goto error ;
   }

   if ( SPT_CONVERTOR_SPE_OBJSTART != name.at(0) )
   {
      rc = SDB_SPT_NOT_SPECIAL_JSON ;
      goto error ;
   }

   if ( 0 == name.compare( SPT_SPEOBJ_MINKEY ) &&
        1 == properties->length )
   {
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_NUMBER, value ) )
      {
         _setErrorMsg( "MinKey value must be a number", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bson_append_minkey( bs, key ) ;
   }
   else if ( 0 == name.compare(SPT_SPEOBJ_MAXKEY) &&
             1 == properties->length )
   {
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_NUMBER, value ) )
      {
         _setErrorMsg( "MaxKey value must be a number", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bson_append_maxkey( bs, key ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_OID ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         _setErrorMsg( "ObjectId value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion ObjectId value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !engine::utilIsValidOID( strValue.c_str() ) )
      {
         _setErrorMsg( "The length of ObjectId is not equal 24", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bson_oid_t oid ;
      bson_oid_from_string( &oid, strValue.c_str() ) ;
      bson_append_oid( bs, key, &oid ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_TIMESTAMP ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      time_t tm ;
      UINT64 usec = 0 ;
      bson_timestamp_t btm ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         _setErrorMsg( "Timestamp value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Timestamp value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_OK != engine::utilStr2TimeT( strValue.c_str(),
                                            tm,
                                            &usec ))
      {
         _setErrorMsg( "Failed to conversion Timestamp", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      SDB_TIMESTAMP_TYPE_CHECK_BOUND( tm ) ;
      btm.t = tm;
      btm.i = usec ;
      bson_append_timestamp( bs, key, &btm ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_NUMBERLONG ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      if ( !_getProperty( obj, name.c_str(), JSTYPE_STRING, value ))
      {
         _setErrorMsg( "NumberLong value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( value, strValue ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion NumberLong value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_isValidNumberLong(strValue.c_str()) )
      {
         _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bson_append_long( bs, key, ossAtoll(strValue.c_str()) ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_DATE ) &&
             1 == properties->length )
   {
      std::string strValue ;
      jsval value ;
      UINT64 tm ;
      bson_date_t datet ;
      if ( TRUE == _getProperty( obj, name.c_str(), JSTYPE_STRING, value ) )
      {
         rc = _toString( value, strValue ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion Date", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         rc = engine::utilStr2Date( strValue.c_str(), tm ) ;
         if ( SDB_OK != rc )
         {
            try
            {
               tm = boost::lexical_cast<UINT64>( strValue.c_str() ) ;
            }
            catch( boost::bad_lexical_cast &e )
            {
               _setErrorMsg( "Failed to conversion Date", FALSE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      else if ( TRUE == _getProperty( obj, name.c_str(), JSTYPE_OBJECT, value ) )
      {
         JSObject *tmpObj = JSVAL_TO_OBJECT( value ) ;
         jsval tmpValue ;
         if ( NULL == tmpObj )
         {
            _setErrorMsg( "Failed to get $numberLong", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( TRUE == _getProperty( tmpObj, "$numberLong",
                                    JSTYPE_STRING, tmpValue ) )
         {
            rc = _toString( tmpValue, strValue ) ;
            if ( SDB_OK != rc )
            {
               _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            try
            {
               tm = boost::lexical_cast<UINT64>( strValue ) ;
            }
            catch( boost::bad_lexical_cast &e )
            {
               _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( TRUE == _getProperty( tmpObj, "$numberLong",
                                         JSTYPE_NUMBER, tmpValue ) )
         {
            FLOAT64 fv = 0 ;
            rc = _toDouble( tmpValue, fv ) ;
            if ( SDB_OK != rc )
            {
               _setErrorMsg( "Failed to conversion NumberLong", FALSE ) ;
               goto error ;
            }
            tm = fv ;
         }
         else if ( is_numberlong( _cx, tmpObj ) )
         {
            INT64 value = 0 ;
            rc = _getNumberLongValue( tmpObj, value ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            tm = value ;
         }
         else
         {
            _setErrorMsg( "$date 's value object is not support", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( TRUE == _getProperty( obj, name.c_str(), JSTYPE_NUMBER, value ) )
      {
         FLOAT64 fv = 0 ;
         rc = _toDouble( value, fv ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion Number", FALSE ) ;
            goto error ;
         }
         tm = fv ;
      }
      else
      {
         _setErrorMsg( "Invalid Date value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      datet = tm ;
      rc = bson_append_date( bs, key, datet ) ;
      if ( SDB_OK !=rc )
      {
         _setErrorMsg( "Failed to conversion Date", FALSE ) ;
         rc = SDB_DRIVER_BSON_ERROR ;
         goto  error ;
      }
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_REGEX ) &&
             2 == properties->length )
   {
      std::string optionName ;
      std::string strRegex, strOption ;
      jsval jsRegex, jsOption ;
      jsid optionid = properties->vector[1] ;
      jsval optionValName ;

      if ( !JS_IdToValue( _cx, optionid, &optionValName ))
      {
         _setErrorMsg( "Regex $options not found", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( optionValName, optionName ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Regex $options", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 != optionName.compare( SPT_SPEOBJ_OPTION ) )
      {
         if ( _inMatcher )
         {
            rc = SDB_SPT_NOT_SPECIAL_JSON ;
            goto error ;
         }
         else
         {
            _setErrorMsg( "Regex $options not found", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( !_getProperty( obj, name.c_str(),
                          JSTYPE_STRING, jsRegex ))
      {
         _setErrorMsg( "Regex $regex value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_getProperty( obj, optionName.c_str(),
                          JSTYPE_STRING, jsOption ))
      {
         _setErrorMsg( "Regex $options value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsRegex, strRegex ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Regex $regex value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsOption, strOption ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Regex $options value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bson_append_regex( bs, key, strRegex.c_str(), strOption.c_str() ) ;
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_BINARY ) &&
             2 == properties->length )
   {
      std::string typeName ;
      std::string strBin, strType ;
      jsval jsBin, jsType ;
      jsid typeId = properties->vector[1] ;
      jsval typeValName ;
      CHAR *decode = NULL ;
      INT32 decodeSize = 0 ;
      UINT32 binType = 0 ;

      if ( !JS_IdToValue( _cx, typeId, &typeValName ))
      {
         _setErrorMsg( "Binary $type not found", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( typeValName, typeName ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Binary $type", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 != typeName.compare( SPT_SPEOBJ_TYPE ) )
      {
         _setErrorMsg( "Binary $type not found", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_getProperty( obj, name.c_str(),
                          JSTYPE_STRING, jsBin ))
      {
         _setErrorMsg( "Binary $binary value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_getProperty( obj, typeName.c_str(),
                          JSTYPE_STRING, jsType ))
      {
         _setErrorMsg( "Binary $type value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsBin, strBin ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Binary $binary", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsType, strType ) ;
      if ( SDB_OK != rc || strType.empty())
      {
         _setErrorMsg( "Failed to conversion Binary $type", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         binType = boost::lexical_cast<INT32>( strType.c_str() ) ;
         if ( binType > 255 )
         {
            _setErrorMsg( "Bad type for binary", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( std::bad_cast &e )
      {
         _setErrorMsg( "Bad type for binary", FALSE ) ;
         PD_LOG( PDERROR, "bad type for binary:%s", strType.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      decodeSize = getDeBase64Size( strBin.c_str() ) ;
      if ( decodeSize < 0 )
      {
         _setErrorMsg( "Invalid binary code", FALSE ) ;
         PD_LOG( PDERROR, "invalid decode %s", strBin.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( decodeSize > 0 )
      {
         decode = ( CHAR * )SDB_OSS_MALLOC( decodeSize ) ;
         if ( NULL == decode )
         {
            _setErrorMsg( "Failed to allocate memory", FALSE ) ;
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         if ( base64Decode( strBin.c_str(), decode, decodeSize ) < 0 )
         {
            _setErrorMsg( "Invalid binary code", FALSE ) ;
            PD_LOG( PDERROR, "failed to decode base64 code" ) ;
            rc = SDB_INVALIDARG ;
            SDB_OSS_FREE( decode ) ;
            goto error ;
         }

         bson_append_binary( bs, key, binType,
                             decode, decodeSize - 1 ) ;
         SDB_OSS_FREE( decode ) ;
      }
      else
      {
         bson_append_binary( bs, key, binType,
                             "", 0 ) ;
      }
   }
   else if ( 0 == name.compare( SPT_SPEOBJ_DECIMAL ) )
   {
      std::string optionName ;
      std::string strDecimal, strOption ;
      jsval jsDecimal, jsOption ;
      jsval optionValName ;
      int precision ;
      int scale ;
      if ( 1 != properties->length && 2 != properties->length )
      {
         _setErrorMsg( "Invalid Decimal", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !_getProperty( obj, name.c_str(),
                          JSTYPE_STRING, jsDecimal ))
      {
         _setErrorMsg( "Decimal $decimal value must be a string", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _toString( jsDecimal, strDecimal ) ;
      if ( SDB_OK != rc )
      {
         _setErrorMsg( "Failed to conversion Decimal value", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 2 == properties->length )
      {
         jsid optionid = properties->vector[1] ;

         if ( !JS_IdToValue( _cx, optionid, &optionValName ))
         {
            _setErrorMsg( "Decimal $precision not found", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _toString( optionValName, optionName ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion Decimal $precision", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( 0 != optionName.compare( SPT_SPEOBJ_PRESICION ) )
         {
            _setErrorMsg( "Decimal $precision not found", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( !_getProperty( obj, optionName.c_str(),
                             JSTYPE_OBJECT, jsOption ))
         {
            _setErrorMsg( "Decimal $precision value must be a string", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _toString( jsOption, strOption ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion Decimal $precision value", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = _getDecimalPrecision( strOption.c_str(), &precision, &scale ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion Decimal", FALSE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = bson_append_decimal2( bs, key, strDecimal.c_str(),
                                    precision, scale ) ;
      }
      else
      {
         rc = bson_append_decimal3( bs, key, strDecimal.c_str() ) ;
      }

      if ( 0 != rc )
      {
         _setErrorMsg( "Failed to conversion Decimal", FALSE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_SPT_NOT_SPECIAL_JSON ;
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
                                   const jsval &val,
                                   bson *bs )
{
   INT32 rc = SDB_OK ;
   switch (JS_TypeOfValue( _cx, val ))
   {
      case JSTYPE_VOID :
      {
         bson_append_undefined( bs, name.c_str() ) ;
         break ;
      }
      case JSTYPE_NULL :
      {
         bson_append_null( bs, name.c_str() ) ;
         break ;
      }
      case JSTYPE_NUMBER :
      {
         if ( JSVAL_IS_INT( val ) )
         {
            INT32 iN = 0 ;
            rc = _toInt( val, iN ) ;
            if ( SDB_OK != rc )
            {
               _setErrorMsg( "Failed to conversion int", FALSE ) ;
               goto error ;
            }
            bson_append_int( bs, name.c_str(), iN ) ;
         }
         else
         {
            FLOAT64 fV = 0 ;
            rc = _toDouble( val, fV ) ;
            if ( SDB_OK != rc )
            {
               _setErrorMsg( "Failed to conversion double", FALSE ) ;
               goto error ;
            }
            bson_append_double( bs, name.c_str(), fV ) ;
         }
         break ;
      }
      case JSTYPE_STRING :
      {
         std::string str ;
         rc = _toString( val, str ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion string", FALSE ) ;
            goto error ;
         }
         bson_append_string( bs, name.c_str(), str.c_str() ) ;
         break ;
      }
      case JSTYPE_BOOLEAN :
      {
         BOOLEAN bL = TRUE ;
         rc = _toBoolean( val, bL ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion boolean", FALSE ) ;
            goto error ;
         }
         bson_append_bool( bs, name.c_str(), bL ) ;
         break ;
      }
      case JSTYPE_OBJECT :
      {
         if ( JSVAL_IS_NULL( val ) )
         {
            bson_append_null( bs, name.c_str() ) ;
         }
         else
         {
            JSObject *obj = JSVAL_TO_OBJECT( val ) ;
            if ( NULL == obj )
            {
               bson_append_null( bs, name.c_str() ) ;
            }
            else if( is_jsontypes( _cx, obj ) )
            {
               rc = _addJsonTypes( obj, name.c_str(), bs ) ;
               if ( SDB_OK != rc )
               {
                  _setErrorMsg( "Failed to call addJsonTypes", FALSE ) ;
                  goto error ;
               }
            }
            else
            {
               rc = _addSpecialObj( obj, name.c_str(), bs ) ;
               if ( SDB_SPT_NOT_SPECIAL_JSON == rc )
               {
                  BOOLEAN setMatcher = FALSE ;
                  bson *bsobj = NULL ;

                  if ( jsobj_is_sdbobj( _cx, obj ) )
                  {
                     _setErrorMsg( "Can not use sdbobj", FALSE ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }

                  if ( SPT_CONVERT_AGGREGATE == _mode &&
                       0 == name.compare( SPT_AGGREGATE_MATCHER ) &&
                       !_inMatcher )
                  {
                     _inMatcher = TRUE ;
                     setMatcher = TRUE ;
                  }

                  rc = toBson( obj, &bsobj ) ;
                  if ( SDB_OK != rc )
                  {
                     _setErrorMsg( "Failed to call toBson", FALSE ) ;
                     goto error ;
                  }

                  if ( setMatcher )
                  {
                     _inMatcher = FALSE ;
                  }

                  if ( JS_IsArrayObject( _cx, obj ) )
                  {
                     bson_append_array( bs, name.c_str(), bsobj ) ;
                  }
                  else
                  {
                     bson_append_bson( bs, name.c_str(), bsobj ) ;
                  }

                  bson_destroy( bsobj ) ;
               }
               else if ( SDB_OK != rc )
               {
                  _setErrorMsg( "Failed to call addSpecialObj", FALSE ) ;
                  goto error ;
               }
               else
               {
               }
            }
         }
         break ;
      }
      case JSTYPE_FUNCTION :
      {
         std::string str ;
         rc = _toString( val, str ) ;
         if ( SDB_OK != rc )
         {
            _setErrorMsg( "Failed to conversion function", FALSE ) ;
            goto error ;
         }

         bson_append_code( bs, name.c_str(), str.c_str() ) ;
         break ;
      }
      default :
      {
         _setErrorMsg( "unexpected type", FALSE ) ;
         SDB_ASSERT( FALSE, "unexpected type" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN sptConvertor::_getProperty( JSObject *obj,
                                    const CHAR *name,
                                    JSType type,
                                    jsval &val )
{
   if ( !JS_GetProperty( _cx, obj, name, &val ) )
   {
      return FALSE ;
   }
   else if ( type != JS_TypeOfValue( _cx, val ) )
   {
      return FALSE ;
   }
   else
   {
      return TRUE ;
   }
}

INT32 sptConvertor::toString( JSContext *cx,
                              const jsval &val,
                              std::string &str )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != cx, "impossible" ) ;
   size_t len = 0 ;
   JSString *jsStr = JS_ValueToString( cx, val ) ;
   if ( NULL == jsStr )
   {
      goto done ;
   }
   len = JS_GetStringLength( jsStr ) ;
   if ( 0 == len )
   {
      goto done ;
   }
   else
   {
/*      size_t cLen = len * 6 + 1 ;
      const jschar *utf16 = JS_GetStringCharsZ( cx, jsStr ) ; ;
      utf8 = (CHAR *)SDB_OSS_MALLOC( cLen ) ;
      if ( NULL == utf8 )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !JS_EncodeCharacters( cx, utf16, len, utf8, &cLen ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      str.assign( utf8, cLen ) ;
*/

      CHAR *p = JS_EncodeString ( cx , jsStr ) ;
      if ( NULL != p )
      {
         str.assign( p ) ;
         SAFE_JS_FREE( cx, p ) ;
      }
   }
done:
   return rc ;
}

INT32 sptConvertor::_toString( const jsval &val, std::string &str )
{
   return toString( _cx, val, str ) ;
}

INT32 sptConvertor::_toInt( const jsval &val, INT32 &iN )
{
   INT32 rc = SDB_OK ;
   int32 ip = 0 ;
   if ( !JS_ValueToInt32( _cx, val, &ip ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   iN = ip ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_toDouble( const jsval &val, FLOAT64 &fV )
{
   INT32 rc = SDB_OK ;
   jsdouble dp = 0 ;
   if ( !JS_ValueToNumber( _cx, val, &dp ))
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   fV = dp ;
done:
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor::_toBoolean( const jsval &val, BOOLEAN &bL )
{
   INT32 rc = SDB_OK ;
   JSBool bp = TRUE ;
   if ( !JS_ValueToBoolean( _cx, val, &bp ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   bL = bp ;
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN sptConvertor::_isValidNumberLong( const CHAR *value )
{
   UINT32 len = 0 ;
   UINT32 i = 0;
   if ( NULL == value  )
      return FALSE ;
   len = ossStrlen( value ) ;
   if ( len > 0 )
   {
      if ( value[0] == '-' )
      {
         ++i ;
      }
   }
   for ( ; i < len; ++i )
   {
      if ( ! ( value[i] >= '0' && value[i] <= '9' ) )
      {
         return FALSE ;
      }
   }
   return TRUE ;
}

void sptConvertor::_setErrorMsg( const CHAR *pErrMsg, BOOLEAN isReplace )
{
   if( _hasSetErrMsg == TRUE && isReplace == FALSE )
   {
      return ;
   }
   _hasSetErrMsg = TRUE ;
   _errorMsg = pErrMsg ;
}
