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

   Source File Name = sptConvertorHelper.cpp

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

#include "sptConvertorHelper.hpp"
#include "sptConvertor.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "../client/client.h"
#include "../client/client_internal.h"

extern JSBool jsobj_is_query( JSContext *cx, JSObject *obj ) ;
extern JSBool jsobj_is_sdbobj( JSContext *cx, JSObject *obj ) ;
extern void *jsobj_get_cursor_private( JSContext *cx, JSObject *obj ) ;
extern JSBool jsobj_is_bsonobj( JSContext *cx, JSObject *obj) ;
extern JSBool jsobj_is_cursor( JSContext *cx, JSObject *obj ) ;
extern JSBool jsobj_is_cs( JSContext *cx, JSObject *obj ) ;
extern JSBool jsobj_is_cl( JSContext *cx, JSObject *obj ) ;
extern JSBool jsobj_is_rn( JSContext *cx, JSObject *obj ) ;
extern JSBool jsobj_is_rg( JSContext *cx, JSObject *obj ) ;

INT32 JSObj2BsonRaw( JSContext *cx, JSObject *obj, CHAR **raw )
{
   INT32 rc = SDB_OK ;
   sptConvertor convertor( cx ) ;
   bson *bs = NULL ;
   rc = convertor.toBson( obj, &bs ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   bs->ownmem = FALSE ;
   *raw = bs->data ;
done:
   if ( NULL != bs )
   {
      bson_dispose( bs ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 JSVal2String( JSContext *cx, const jsval &val, std::string &str )
{
   return sptConvertor::toString( cx, val, str ) ;
}

CHAR *convertJsvalToString ( JSContext *cx , jsval val )
{
   JSString *  str   = NULL ;
   CHAR *      cstr  = NULL ;

   str = JS_ValueToString ( cx , val ) ;
   if ( ! str )
      goto error ;

   cstr = JS_EncodeString ( cx , str ) ;
   if ( ! cstr )
      goto error ;

done :
   return cstr ;
error :
   goto done ;
}

BOOLEAN JSObjIsQuery( JSContext *cx, JSObject *obj )
{
   return jsobj_is_query( cx, obj ) ;
}

BOOLEAN JSObjIsCursor( JSContext *cx, JSObject *obj )
{
   return jsobj_is_cursor( cx, obj ) ;
}

BOOLEAN JSObjIsSdbObj( JSContext *cx, JSObject *obj )
{
   return jsobj_is_sdbobj( cx, obj ) ;
}

BOOLEAN JSObjIsBsonobj( JSContext *cx, JSObject *obj )
{
   return jsobj_is_bsonobj( cx, obj ) ;
}

INT32 JSObj2Cursor( JSContext *cx, JSObject *obj, void **cursor )
{
   INT32 rc = SDB_OK ;

   *cursor = jsobj_get_cursor_private( cx, obj ) ;
   if ( NULL == *cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 cursorNextRaw( void *cursor, CHAR **raw )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle *handle = ( sdbCursorHandle * )cursor ;
   bson bs ;
   bson_init( &bs ) ;
   rc = sdbNext( *handle, &bs ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   else
   {
      bs.ownmem = FALSE ;
      *raw = bs.data ;
   }
done:
   bson_destroy( &bs ) ;
   return rc ;
error:
   goto done ;
}

INT32 getBsonRawFromBsonClass( JSContext *cx, JSObject *obj, CHAR **raw )
{
   INT32 rc = SDB_OK ;
   bson *bs = (bson *)JS_GetPrivate( cx, obj ) ;
   if ( NULL == bs )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   *raw = bs->data ;
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN JSObjIsCS( JSContext *cx, JSObject *obj )
{
   return jsobj_is_cs( cx, obj ) ;
}

BOOLEAN JSObjIsCL( JSContext *cx, JSObject *obj )
{
   return jsobj_is_cl( cx, obj ) ;
}

BOOLEAN JSObjIsRN( JSContext *cx, JSObject *obj )
{
   return jsobj_is_rn( cx, obj ) ;
}

BOOLEAN JSObjIsRG( JSContext *cx, JSObject *obj )
{
   return jsobj_is_rg( cx, obj ) ;
}

INT32 getCSNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **csName )
{
   INT32 rc = SDB_OK ;
   jsval val = JSVAL_VOID ;
   JSString *jsstr = NULL ;

   if ( !JS_GetProperty(cx, obj, "_name", &val ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_STRING( val ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   jsstr = JSVAL_TO_STRING( val ) ;
   *csName = JS_EncodeString( cx, jsstr ) ;
   if ( NULL == *csName )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   if ( NULL != *csName )
   {
      JS_free( cx, *csName ) ;
      *csName = NULL ;
   }
   goto done ;
}

INT32 getCLNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **clName )
{
   INT32 rc = SDB_OK ;
   jsval valCLName = JSVAL_VOID ;
   jsval valCS = JSVAL_VOID ;
   jsval valCSName = JSVAL_VOID ;
   JSString *clJSStr = NULL ;
   JSString *csJSStr = NULL ;
   CHAR *collection = NULL ;
   CHAR *cs = NULL ;

   if ( !JS_GetProperty( cx, obj, "_name", &valCLName ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_STRING( valCLName) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   clJSStr = JSVAL_TO_STRING( valCLName ) ;
   collection = JS_EncodeString( cx, clJSStr ) ;
   if ( NULL == collection )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JS_GetProperty( cx, obj, "_cs", &valCS ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_OBJECT( valCS ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JS_GetProperty(cx, JSVAL_TO_OBJECT( valCS ),
                        "_name", &valCSName  ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_STRING( valCSName ))
   {
      rc = SDB_SYS ;
      goto error ;
   }

   csJSStr = JSVAL_TO_STRING( valCSName ) ;
   cs = JS_EncodeString( cx, csJSStr ) ;
   if ( NULL == cs )
   {
      rc = SDB_OK ;
      goto error ;
   }

   {
   UINT32 clLen = ossStrlen( collection ) ;
   UINT32 csLen = ossStrlen( cs ) ;
   *clName = ( CHAR * )SDB_OSS_MALLOC( clLen + csLen  + 2 ) ; // +2 for '.' and '\0'
   if ( NULL == *clName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossSnprintf( *clName, clLen + csLen + 2, "%s.%s",
                cs, collection ) ;
   }

done:
   if ( NULL != collection )
   {
      JS_free( cx, collection ) ;
   }
   if ( NULL != cs )
   {
      JS_free( cx, cs ) ;
   }
   return rc ;
error:
   if ( NULL != *clName )
   {
      SDB_OSS_FREE( *clName ) ;
      *clName = NULL ;
   }
   goto done ;
}

INT32 getRGNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **rgName )
{
   INT32 rc = SDB_OK ;
   jsval val = JSVAL_VOID ;
   JSString *jsstr = NULL ;

   if ( !JS_GetProperty(cx, obj, "_name", &val ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_STRING( val ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   jsstr = JSVAL_TO_STRING( val ) ;
   *rgName = JS_EncodeString( cx, jsstr ) ;
   if ( NULL == *rgName )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   if ( NULL != *rgName )
   {
      JS_free( cx, *rgName ) ;
      *rgName = NULL ;
   }
   goto done ;
}

INT32 getRNNameFromObj( JSContext *cx, JSObject *obj,
                        CHAR **rnName )
{
   INT32 rc = SDB_OK ;
   jsval valName = JSVAL_VOID ;
   jsval valRG = JSVAL_VOID ;
   jsval valRGName = JSVAL_VOID ;
   JSString *jsNodeName = NULL ;
   JSString *jsRGName = NULL ;
   CHAR *nodeName = NULL ;
   CHAR *rgName = NULL ;

   if ( !JS_GetProperty( cx, obj, "_nodename", &valName ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_STRING( valName ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   jsNodeName = JSVAL_TO_STRING( valName ) ;
   nodeName = JS_EncodeString( cx, jsNodeName ) ;
   if ( NULL == nodeName )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( !JS_GetProperty( cx, obj, "_rg", &valRG ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JSVAL_IS_OBJECT(valRG) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !JS_GetProperty( cx, JSVAL_TO_OBJECT( valRG ),
                         "_name", &valRGName ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( !JSVAL_IS_STRING( valRGName ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   jsRGName = JSVAL_TO_STRING( valRGName ) ;
   rgName = JS_EncodeString( cx, jsRGName ) ;
   if ( NULL == rgName )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   {
   UINT32 nodeLen = ossStrlen( nodeName ) ;
   UINT32 rgLen = ossStrlen( rgName ) ;
   *rnName = ( CHAR * )SDB_OSS_MALLOC( nodeLen + rgLen + 2 ) ;
   if ( NULL == *rnName )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossSnprintf( *rnName, nodeLen + rgLen + 2, "%s:%s",
                rgName, nodeName ) ;
   }

done:
   if ( NULL != nodeName )
   {
      JS_free( cx, nodeName ) ;
   }
   if ( NULL != rgName )
   {
      JS_free( cx, rgName ) ;
   }
   return rc ;
error:
   if ( NULL != *rnName )
   {
      SDB_OSS_FREE( *rnName ) ;
      *rnName = NULL ;
   }
   goto done ;
}

