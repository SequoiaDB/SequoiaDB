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

   Source File Name = sptInvoker.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_INVOKER_HPP_
#define SPT_INVOKER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptInvokeDef.hpp"
#include "sptFuncMap.hpp"
#include "pd.hpp"
#include "sptSPDef.hpp"
#include "sptSPArguments.hpp"
#include "sptReturnVal.hpp"
#include "sptCommon.hpp"
#include "../bson/bson.hpp"


namespace engine
{
   using namespace JS_INVOKER;
   using namespace bson ;
   using namespace std ;

   /*
      _sptInvoker define
   */
   class _sptInvoker : public SDBObject
   {
   public:
      _sptInvoker(){}
      virtual ~_sptInvoker() {}

   public:

      template<typename T, typename Func>
      static INT32 callMemberFunc( JSContext *cx,
                                   uintN argc,
                                   jsval *vp,
                                   Func f )
      {
         INT32 rc = SDB_OK ;
         SDB_ASSERT( NULL != cx && NULL != vp, "can not be NULL" ) ;
         SDB_ASSERT( NULL != f, "can not be NULL" ) ;

         jsval jsRval = JSVAL_VOID ;
         _sptSPArguments arg( cx, argc, vp ) ;
         _sptReturnVal rval ;
         bson::BSONObj detail ;
         void *instance = JS_GetPrivate ( cx , JS_THIS_OBJECT ( cx , vp ) ) ;
         if ( NULL == instance )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "js object has no private data." ) ;
            goto error ;
         }

         rc = (((T*)instance)->*f)( arg, rval, detail ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "member function returns err: %d, detail: %s",
                    rc, detail.isEmpty() ? "" : detail.toString().c_str() ) ;
            goto error ;
         }

         rc = _callbackDone( cx, JS_THIS_OBJECT( cx, vp ), rval, detail, &jsRval ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( JS_IsExceptionPending( cx ) )
         {
            rc = SDB_SPT_EVAL_FAIL ;
            goto error ;
         }
         JS_SET_RVAL( cx, vp, jsRval ) ;
      done:
         return rc ;
      error:
         _reportError( cx, rc, detail ) ;
         goto done ;         
      }

      template<typename Func>
      static INT32 callStaticFunc( JSContext *cx,
                                   uintN argc,
                                   jsval *vp,
                                   Func f )
      {
         INT32 rc = SDB_OK ;
         SDB_ASSERT( NULL != cx && NULL != vp, "can not be NULL" ) ;
         SDB_ASSERT( NULL != f, "can not be NULL" ) ;

         jsval jsRval = JSVAL_VOID ;
         _sptSPArguments arg( cx, argc, vp ) ;
         _sptReturnVal rval ;
         bson::BSONObj detail ;

         rc = (*f)( arg, rval, detail ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "member function returns err: %d, detail: %s",
                    rc, detail.isEmpty() ? "" : detail.toString().c_str() ) ;
            goto error ;
         }

         rc = _callbackDone( cx, NULL, rval, detail, &jsRval ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         JS_SET_RVAL( cx, vp, jsRval ) ;
      done:
         return rc ;
      error:
         _reportError( cx, rc, detail ) ;
         goto done ;
      }


      template<typename T, typename Func>
      static INT32 callConstructFunc( JSContext *cx,
                                      uintN argc,
                                      jsval *vp,
                                      Func f )
      {
         INT32 rc = SDB_OK ;
         SDB_ASSERT( NULL != cx && NULL != vp, "can not be NULL" ) ;
         SDB_ASSERT( NULL != f, "can not be NULL" ) ;

         JSObject *jsObj = NULL ;
         _sptSPArguments arg( cx, argc, vp ) ;
         _sptReturnVal rval ;
         bson::BSONObj detail ;
         T * instance = T::crtInstance() ;
         if ( NULL == instance )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         rc = (instance->*f)( arg, rval, detail ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "construct function returns err: %d, detail: %s",
                    rc, detail.isEmpty() ? "" : detail.toString().c_str() ) ;
            goto error ;
         }

         jsObj = JS_NewObject ( cx , (JSClass *)(T::__desc.getClassDef()), 0 , 0 ) ;
         if ( NULL == jsObj )
         {
            PD_LOG( PDERROR, "failed to allocate js object" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         JS_SET_RVAL ( cx , vp , OBJECT_TO_JSVAL ( jsObj ) ) ;
         if ( !JS_SetPrivate ( cx , jsObj , instance ) )
         {
            PD_LOG( PDERROR, "failed to set object to js object" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         /*
            Add the fixed property
         */
         rval.addSelfProperty( SPT_OBJ_CNAME_PROPNAME,
                               SPT_PROP_READONLY|
                               SPT_PROP_PERMANENT )->setValue(
                               T::__desc.getJSClassName() ) ;
         rval.addSelfProperty( SPT_OBJ_ID_PROPNAME,
                               SPT_PROP_READONLY|
                               SPT_PROP_PERMANENT )->setValue(
                               sdbGetGlobalID() ) ;

         if ( !rval.getSelfProperties().empty() )
         {
            rc = setProperty( cx, jsObj, rval.getSelfProperties() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         if ( JS_IsExceptionPending( cx ) )
         {
            rc = SDB_SPT_EVAL_FAIL ;
            goto error ;
         }
      done:
         return rc ;
      error:
         if ( NULL != jsObj )
         {
            JS_SetPrivate ( cx , jsObj , NULL ) ;
         }
         T::releaseInstance( instance ) ;

         _reportError( cx, rc, detail ) ;
         goto done ;
      }

      template<typename T, typename Func>
      static INT32 callDestructFunc( JSContext *cx,
                                     JSObject *obj,
                                     Func f )
      {
         void *instance = JS_GetPrivate ( cx , obj ) ;
         JS_SetPrivate ( cx , obj , NULL ) ;
         if ( NULL != instance )
         {
            T *t = ( T * )instance ;
            (t->*f)() ;
            T::releaseInstance( t ) ;
         }
         return SDB_OK ;
      }

      template<typename T, typename Func>
      static INT32 callResolveFunc( JSContext *cx,
                                    JSObject *obj,
                                    jsid id,
                                    uintN flags,
                                    JSObject ** objp,
                                    Func f )
      {
         INT32 rc = SDB_OK ;
         bson::BSONObj detail ;
         _sptReturnVal rval ;
         jsval jsRval = JSVAL_VOID ;
         jsval valID = JSVAL_VOID ;

         BOOLEAN processed = FALSE ;
         BOOLEAN setIDProp = FALSE ;
         string  callFunc ;

         void *instance = JS_GetPrivate( cx, obj ) ;
         if ( NULL == instance )
         {
            goto done ;
         }

         if ( !JS_IdToValue( cx, id, &valID ) )
         {
            PD_LOG( PDERROR, "failed to convert js id to value" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( flags & JSRESOLVE_ASSIGNING )
         {
            goto done ;
         }

         if ( JSVAL_IS_INT( valID ) || JSVAL_IS_STRING ( valID ) )
         {
            jsval vp[ 3 ] = { JSVAL_VOID, JSVAL_VOID, valID } ;
            _sptSPArguments args( cx, 1, &vp[0] ) ;

            JSObject *prototype = JS_GetPrototype( cx, obj ) ;
            if ( prototype )
            {
               JSClass *pClass = NULL ;
               JSResolveOp tmpResoveOp = NULL ;
               JSBool hasRet = FALSE ;
               JSBool found = FALSE ;

               pClass = JS_GET_CLASS( cx, obj ) ;
               SDB_ASSERT( pClass, "Class can't be empty" ) ;
               if ( pClass )
               {
                  tmpResoveOp = pClass->resolve ;
                  pClass->resolve = JS_ResolveStub ;
               }
               hasRet = JS_HasPropertyById( cx, prototype, id, &found ) ;
               if ( pClass )
               {
                  pClass->resolve = tmpResoveOp ;
               }

               if ( hasRet && found )
               {
                  goto done ;
               }
            }

            if ( JSVAL_IS_STRING ( valID ) )
            {
               string idstr ;
               args.getString( 0, idstr ) ;

               if ( T::__desc.getFuncMap().isMemberFunc( idstr.c_str() ) )
               {
                  goto done ;
               }
            }

            rc = (( ( T * )instance )->*f)( args,
                                            _getOpCode( cx ), /// SPT_JS_OP_CODE
                                            processed,
                                            callFunc,
                                            setIDProp,
                                            rval,
                                            detail ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "resolve func returns err:%d, detail:%s",
                       rc, detail.isEmpty() ? "" : detail.toString().c_str() ) ;
               goto error ;
            }
            else if ( !processed )
            {
               goto done ;
            }
            else if ( !callFunc.empty() )
            {
               if ( !JS_CallFunctionName ( cx, obj, callFunc.c_str(),
                                           1, &valID, &jsRval ) )
               {
                  rc = SDB_SYS ;
                  detail = BSON( SPT_ERR << "Call function failed" ) ;
                  goto error ;
               }
            }
            else
            {
               rc = _callbackDone( cx, obj, rval, detail, &jsRval ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }

            if ( setIDProp && !JS_SetPropertyById ( cx, obj, id, &jsRval ) )
            {
               rc = SDB_SYS ;
               detail = BSON( SPT_ERR << "Set property by id failed" ) ;
               goto error ;
            }
         }
         else
         {
            goto done ;
         }

         *objp = obj ;

      done:
         return rc ;
      error:
          _reportError( cx, rc, detail ) ;
         *objp = NULL ;
         goto done ;
      }

      static INT32 setProperty( JSContext *cx,
                                JSObject *obj,
                                const SPT_PROPERTIES &properties ) ;

      static INT32 setArrayElems( JSContext *cx,
                                  JSObject *obj,
                                  const SPT_PROPERTIES &properties ) ;

   private:
      static INT32 _getValFromProperty( JSContext *cx,
                                        const sptProperty &pro,
                                        jsval &val ) ;

      static INT32 _callbackDone( JSContext *cx, JSObject *obj,
                                  _sptReturnVal &rval,
                                  const bson::BSONObj &detail,
                                  jsval *rvp ) ;

      static void _reportError( JSContext *cx,
                                INT32 rc,
                                const bson::BSONObj &detail ) ;

      static UINT32 _getOpCode( JSContext *cx ) ;
 
   } ;
   typedef class _sptInvoker sptInvoker ;
}

#endif

