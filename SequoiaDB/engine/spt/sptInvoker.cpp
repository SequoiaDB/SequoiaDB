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

   Source File Name = sptInvoker.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptInvoker.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"
#include "jscntxt.h"
#include "jsscript.h"
using namespace bson ;

namespace engine
{

   /*
      _sptInvoker implement
   */
   INT32 _sptInvoker::_getValFromProperty( JSContext *cx,
                                           const sptProperty &pro,
                                           jsval &val )
   {
      INT32 rc = SDB_OK ;
      if ( String == pro.getType() )
      {
         JSString *jsstr = JS_NewStringCopyN( cx, pro.getString(),
                                              ossStrlen( pro.getString() ) ) ;
         if ( NULL == jsstr )
         {
            ossPrintf( "%s\n", pro.getString() ) ;
            PD_LOG( PDERROR, "failed to create a js string" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         val = STRING_TO_JSVAL( jsstr ) ;
      }
      else if ( Bool == pro.getType() )
      {
         BOOLEAN v = TRUE ;
         rc = pro.getNative( Bool, &v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         val = BOOLEAN_TO_JSVAL( v ) ;
      }
      else if ( NumberInt == pro.getType() )
      {
         INT32 v = 0 ;
         rc = pro.getNative( NumberInt, &v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         val = INT_TO_JSVAL( v ) ;
      }
      else if ( NumberDouble == pro.getType() )
      {
         FLOAT64 v = 0 ;
         rc = pro.getNative( NumberDouble, &v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         val = DOUBLE_TO_JSVAL( v ) ;
      }
      else
      {
         PD_LOG( PDERROR, "the type %d is not surpported yet.",
                 pro.getType() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptInvoker::_callbackDone( JSContext *cx, JSObject *obj,
                                     _sptReturnVal &rval,
                                     const bson::BSONObj &detail,
                                     jsval *rvp )
   {
      INT32 rc = SDB_OK ;
      sptProperty &rpro = rval.getReturnVal() ;
      jsval val = JSVAL_VOID ;

      if( rpro.isRawData() )
      {
         sptResultVal *pResultVal ;
         rc = rpro.getResultVal( &pResultVal ) ;
         if( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get result value" ) ;
            goto error ;
         }
         val = *( jsval* )pResultVal->rawPtr() ;
      }
      else
      {
         if ( EOO == rpro.getType() )
         {
            *rvp = JSVAL_VOID ;
            goto done ;
         }
         else if ( rpro.isObject() )
         {
            JSObject *jsObj = JS_NewObject ( cx,
                                             (JSClass *)(rpro.getObjDesc()->getClassDef()),
                                             0 , 0 ) ;
            if ( NULL == jsObj )
            {
               PD_LOG( PDERROR, "failed to new js object" ) ;
               rc = SDB_OOM ;
               goto error ;
            }

            if( !JS_SetPrivate( cx, jsObj, rpro.getValue() ) )
            {
               PD_LOG( PDERROR, "failed to set object to js object" ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            /*
               Add the fixed property
            */
            rval.addReturnValProperty( SPT_OBJ_CNAME_PROPNAME,
                                       SPT_PROP_READONLY|
                                       SPT_PROP_PERMANENT )->setValue(
                                       rpro.getObjDesc()->getJSClassName() ) ;
            rval.addReturnValProperty( SPT_OBJ_ID_PROPNAME,
                                       SPT_PROP_READONLY|
                                       SPT_PROP_PERMANENT )->setValue(
                                       sdbGetGlobalID() ) ;

            rpro.takeoverObject() ;

            if ( !rval.getReturnValProperties().empty() )
            {
               rc = _sptInvoker::setProperty( cx, jsObj,
                                              rval.getReturnValProperties() ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }

            val = OBJECT_TO_JSVAL( jsObj ) ;
         }
         else if ( rpro.isArray() )
         {
            const SPT_PROPERTIES &elem = rpro.getArray() ;
            if ( elem.size() > 0 )
            {
               JSObject *jsObj = JS_NewArrayObject( cx, elem.size(), NULL ) ;
               if ( NULL == jsObj )
               {
                  PD_LOG( PDERROR, "Failed to new js Array" ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
               rc = setArrayElems( cx, jsObj, elem ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to set the array's elem "
                          "failed, rc: %d", rc ) ;
                  goto error ;
               }

               val = OBJECT_TO_JSVAL( jsObj ) ;
            }
         }
         else
         {
            rc = _getValFromProperty( cx, rpro, val ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         if ( obj && !rval.getSelfProperties().empty() )
         {
            rc = _sptInvoker::setProperty( cx, obj, rval.getSelfProperties() ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         if ( obj && !rpro.getName().empty() )
         {
            if ( rpro.isNeedDelete() )
            {
               JSBool found = JS_FALSE ;
               uintN attr = 0 ;
               if ( JS_GetPropertyAttributes( cx, obj, rpro.getName().c_str(),
                                              &attr, &found ) &&
                    found &&
                    ( attr & SPT_PROP_PERMANENT ) )
               {
                  attr &= ~SPT_PROP_PERMANENT ;
                  JS_SetPropertyAttributes( cx, obj, rpro.getName().c_str(),
                                            attr, &found ) ;
               }

               if ( found )
               {
                  jsval dval = JSVAL_VOID ;
                  JS_DeleteProperty2( cx, obj, rpro.getName().c_str(), &dval ) ;
               }
            }
            else if ( !JS_DefineProperty( cx, obj, rpro.getName().c_str(),
                                          val, 0, 0, rpro.getAttr() ) )
            {
               PD_LOG( PDERROR, "failed to set obj to parent obj" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
      *rvp = val ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptInvoker::setProperty( JSContext *cx,
                                   JSObject *obj,
                                   const SPT_PROPERTIES &properties )
   {
      INT32 rc = SDB_OK ;
      SPT_PROPERTIES::const_iterator itr = properties.begin() ;

      for ( ; itr != properties.end(); itr++ )
      {
         sptProperty *prop = (sptProperty*)(*itr) ;
         SDB_ASSERT( !prop->getName().empty(), "name can not be empty" ) ;

         if ( prop->getName().empty() )
         {
            continue ;
         }
         else if ( prop->isNeedDelete() )
         {
            JSBool found = JS_FALSE ;
            uintN attr = 0 ;
            if ( JS_GetPropertyAttributes( cx, obj, prop->getName().c_str(),
                                           &attr, &found ) &&
                 found &&
                 ( attr & SPT_PROP_PERMANENT ) )
            {
               attr &= ~SPT_PROP_PERMANENT ;
               JS_SetPropertyAttributes( cx, obj, prop->getName().c_str(),
                                         attr, &found ) ;
            }

            if ( found )
            {
               jsval dval = JSVAL_VOID ;
               JS_DeleteProperty2( cx, obj, prop->getName().c_str(), &dval ) ;
            }
         }
         else if ( bson::EOO == prop->getType() )
         {
            continue ;
         }

         jsval val = JSVAL_VOID ;

         if ( prop->isObject() )
         {
            JSObject *jsObj = JS_NewObject ( cx,
                                             (JSClass *)(prop->getObjDesc()->getClassDef()),
                                             0 , 0 ) ;
            if ( NULL == jsObj )
            {
               PD_LOG( PDERROR, "failed to new js object" ) ;
               rc = SDB_OOM ;
               goto error ;
            }

            if( !JS_SetPrivate( cx, jsObj, prop->getValue() ) )
            {
               PD_LOG( PDERROR, "failed to set private to js object" ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            prop->takeoverObject() ;

            val = OBJECT_TO_JSVAL( jsObj ) ;
         }
         else if ( prop->isArray() )
         {
            const SPT_PROPERTIES &elem = prop->getArray() ;
            if ( 0 == elem.size() )
            {
               continue ;
            }
            JSObject *jsObj = JS_NewArrayObject( cx, elem.size(), NULL ) ;
            if ( NULL == jsObj )
            {
               PD_LOG( PDERROR, "Failed to new js Array" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            rc = setArrayElems( cx, jsObj, elem ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to set the array's elem "
                       "failed, rc: %d", rc ) ;
               goto error ;
            }

            val = OBJECT_TO_JSVAL( jsObj ) ;
         }
         else
         {
            rc = _getValFromProperty( cx, *prop, val ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         if ( !JS_DefineProperty( cx, obj, prop->getName().c_str(),
                                  val, 0, 0, prop->getAttr() ) )
         {
            PD_LOG( PDERROR, "failed to set property of obj" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptInvoker::setArrayElems( JSContext *cx,
                                     JSObject *obj,
                                     const SPT_PROPERTIES &properties )
   {
      INT32 rc = SDB_OK ;
      UINT32 index = 0 ;
      SPT_PROPERTIES::const_iterator itr = properties.begin() ;

      for ( ; itr != properties.end(); itr++ )
      {
         sptProperty *prop = (sptProperty*)(*itr) ;

         if ( bson::EOO == prop->getType() )
         {
            continue ;
         }

         jsval val = JSVAL_VOID ;

         if ( prop->isObject() )
         {
            JSObject *jsObj = JS_NewObject ( cx,
                                             (JSClass *)(prop->getObjDesc()->getClassDef()),
                                             0 , 0 ) ;
            if ( NULL == jsObj )
            {
               PD_LOG( PDERROR, "faile to new js object" ) ;
               rc = SDB_OOM ;
               goto error ;
            }

            JS_SetPrivate( cx, jsObj, prop->getValue() ) ;

            prop->takeoverObject() ;

            val = OBJECT_TO_JSVAL( jsObj ) ;
         }
         else if ( prop->isArray() )
         {
            const SPT_PROPERTIES &elem = prop->getArray() ;
            if ( 0 == elem.size() )
            {
               continue ;
            }
            JSObject *jsObj = JS_NewArrayObject( cx, elem.size(), NULL ) ;
            if ( NULL == jsObj )
            {
               PD_LOG( PDERROR, "Failed to new js Array" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            rc = setArrayElems( cx, jsObj, elem ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to set the array's array elem "
                       "failed, rc: %d", rc ) ;
               goto error ;
            }

            val = OBJECT_TO_JSVAL( jsObj ) ;
         }
         else
         {
            rc = _getValFromProperty( cx, *prop, val ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         if ( !JS_DefineElement( cx, obj, index,
                                 val, 0, 0, prop->getAttr() ) )
         {
            PD_LOG( PDERROR, "Failed to add the %d item of array", index ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         ++index ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptInvoker::_reportError( JSContext *cx,
                                   INT32 rc,
                                   const bson::BSONObj &detail )
   {
      if ( SDB_OK != rc && !JS_IsExceptionPending( cx ) )
      {
         stringstream ss ;
         BSONObjIterator itr( detail) ;
         INT32 fieldNum = detail.nFields() ;
         INT32 count = 0 ;
         while ( itr.more() )
         {
            if ( count > 0 )
            {
               ss << ", " ;
            }
            BSONElement e = itr.next() ;
            if ( fieldNum > 1 ||
                 0 != ossStrcmp( SPT_ERR, e.fieldName() ) )
            {
               ss << e.fieldName() << ": " ;
            }

            if ( String == e.type() )
            {
               ss << e.valuestr() ;
            }
            else if ( NumberInt == e.type() )
            {
               ss << e.numberInt() ;
            }
            else if ( NumberLong == e.type() )
            {
               ss << e.numberLong() ;
            }
            else if ( NumberDouble == e.type() )
            {
               ss << e.numberDouble() ;
            }
            else if ( Bool == e.type() )
            {
               ss << ( e.boolean() ? "true" : "false" ) ;
            }
            else
            {
               ss << e.toString( false, false ) ;
            }
            ++count ;
         }

         string strMsg = ss.str() ;
         if ( sdbIsErrMsgEmpty() )
         {
            sdbSetErrMsg( strMsg.empty() ? getErrDesp(rc) : strMsg.c_str(),
                          FALSE ) ;
         }
         if ( SDB_OK == sdbGetErrno() )
         {
            sdbSetErrno( rc, FALSE ) ;
         }

         {
            sptPrivateData *privateData = ( sptPrivateData* )
                                            JS_GetContextPrivate( cx ) ;
            if( NULL != privateData )
            {
               for( JSStackFrame *fp = js_GetTopStackFrame( cx );
                    fp; fp = fp->prev() )
               {
                  jsbytecode *pc = JS_GetFramePC( cx, fp ) ;
                  if( pc )
                  {
                     JSScript *script = fp->script() ;
                     const CHAR* scriptFileName = JS_GetScriptFilename( cx, script) ;
                     privateData->SetErrInfo( scriptFileName ? scriptFileName: "(nofile)",
                                              JS_PCToLineNumber( cx,
                                                                 script,
                                                                 pc ) ) ;
                     break;
                  }
               }
            }
            else
            {
               PD_LOG( PDWARNING, "Failed to get private data" ) ;
            }
         }
         JS_SetPendingException( cx , INT_TO_JSVAL( rc ) ) ;
      }
   }

   UINT32 _sptInvoker::_getOpCode( JSContext *cx )
   {
      UINT32 opcode = 0 ;
      if ( cx && cx->regs )
      {
         opcode = (UINT32)(*cx->regs->pc ) ;
      }
      return opcode ;
   }

}

