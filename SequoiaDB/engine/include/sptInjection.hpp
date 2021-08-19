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

   Source File Name = sptInjection.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_INJECTION_HPP_
#define SPT_INJECTION_HPP_

#include "sptCommon.hpp"

namespace engine
{
   #define _SPT_MAKE_JOIN( A, B )      A##B
   #define _SPT_MAKE_JOIN2( A, B )     _SPT_MAKE_JOIN( A, B )
   #define _SPT_UNIQUE_NAME( A )       _SPT_MAKE_JOIN2( A, __LINE__ )

   #define _JS_MEMBER_FUNC_DEFINE_( className, funcName, resetError )\
           static JSBool __##funcName( JSContext *cx , uintN argc , jsval *vp )\
           {\
              JSBool ret = JS_TRUE ; \
              INT32 rc = SDB_OK ; \
              if ( resetError ) \
              { \
                 sdbClearErrorInfo() ; \
              } \
              else \
              { \
                 sdbSetNeedClearErrorInfo( FALSE ) ; \
              } \
              typedef INT32 (className::*FUNC)(const _sptArguments &,\
                                              _sptReturnVal &,\
                                               bson::BSONObj &);\
              rc = sptInvoker::callMemberFunc<className, FUNC>\
                               (cx, argc, vp, &className::funcName ) ;\
              if ( SDB_OK != rc )\
              {\
                 ret = JS_FALSE ;\
                 goto error ; \
              }\
           done:\
              return ret ;\
           error:\
              goto done ;\
           }

   #define JS_MEMBER_FUNC_DEFINE( className, funcName ) \
      _JS_MEMBER_FUNC_DEFINE_( className, funcName, TRUE )

   #define JS_MEMBER_FUNC_DEFINE_NORESET( className, funcName ) \
      _JS_MEMBER_FUNC_DEFINE_( className, funcName, FALSE )

   #define _JS_CONSTRUCT_FUNC_DEFINE_( className, funcName, resetError )\
           static JSBool __##funcName( JSContext *cx , uintN argc , jsval *vp )\
           {\
              JSBool ret = JS_TRUE ; \
              INT32 rc = SDB_OK ; \
              if ( resetError ) \
              { \
                 sdbClearErrorInfo() ; \
              } \
              else \
              { \
                 sdbSetNeedClearErrorInfo( FALSE ) ; \
              } \
              typedef INT32 (className::*FUNC)(const _sptArguments &,\
                                              _sptReturnVal &,\
                                               bson::BSONObj &);\
              rc = sptInvoker::callConstructFunc<className, FUNC>\
                               (cx, argc, vp, &className::funcName ) ;\
              if ( SDB_OK != rc )\
              {\
                 ret = JS_FALSE ;\
                 goto error ; \
              }\
           done:\
              return ret ;\
           error:\
              goto done ;\
           }

   #define JS_CONSTRUCT_FUNC_DEFINE( className, funcName ) \
      _JS_CONSTRUCT_FUNC_DEFINE_( className, funcName, TRUE )

   #define JS_CONSTRUCT_FUNC_DEFINE_NORESET( className, funcName ) \
      _JS_CONSTRUCT_FUNC_DEFINE_( className, funcName, FALSE )

   #define JS_DESTRUCT_FUNC_DEFINE( className, funcName ) \
            static void __##funcName( JSContext *cx ,  JSObject *obj )\
            {\
              typedef INT32 (className::*FUNC)() ;\
              sptInvoker::callDestructFunc<className, FUNC>\
                               (cx, obj, &className::funcName ) ;\
            }

   #define _JS_RESOLVE_FUNC_DEFINE_( className, funcName, resetError )\
           static JSBool __##funcName(JSContext *cx , JSObject *obj , jsid id ,\
                            uintN flags , JSObject ** objp)\
            {\
              JSBool ret = JS_TRUE ; \
              INT32 rc = SDB_OK ; \
              if ( resetError ) \
              { \
                 sdbClearErrorInfo() ; \
              } \
              else \
              { \
                 sdbSetNeedClearErrorInfo( FALSE ) ; \
              } \
              typedef INT32 (className::*FUNC)( const _sptArguments &args, \
                                                UINT32 opcode, \
                                                BOOLEAN &processed, \
                                                string &callFunc, \
                                                BOOLEAN &setIDProp, \
                                                _sptReturnVal &rval, \
                                                bson::BSONObj &detail ) ;\
              rc = sptInvoker::callResolveFunc<className, FUNC>\
                               (cx, obj, id, flags, objp, &className::funcName ) ;\
              if ( SDB_OK != rc )\
              {\
                 ret = JS_FALSE ;\
                 goto error ; \
              }\
           done:\
              return ret ;\
           error:\
              goto done ;\
           }

   #define JS_RESOLVE_FUNC_DEFINE( className, funcName ) \
      _JS_RESOLVE_FUNC_DEFINE_( className, funcName, TRUE )

   #define JS_RESOLVE_FUNC_DEFINE_NORESET( className, funcName ) \
      _JS_RESOLVE_FUNC_DEFINE_( className, funcName, FALSE )

   #define _JS_STATIC_FUNC_DEFINE_( className, funcName, resetError )\
           static JSBool __##funcName(JSContext *cx, uintN argc , jsval *vp ) \
           {\
              JSBool ret = JS_TRUE ; \
              INT32 rc = SDB_OK ; \
              if ( resetError ) \
              { \
                 sdbClearErrorInfo() ; \
              } \
              else \
              { \
                 sdbSetNeedClearErrorInfo( FALSE ) ; \
              } \
              typedef INT32 (*FUNC)( const _sptArguments &,\
                                     _sptReturnVal &,\
                                     bson::BSONObj &);\
              rc = sptInvoker::callStaticFunc<FUNC>\
                         (cx, argc, vp, &className::funcName ) ;\
              if ( SDB_OK != rc )\
              {\
                 ret = JS_FALSE ;\
                 goto error ; \
              }\
           done:\
              return ret ;\
           error:\
              goto done ;\
           }

   #define JS_STATIC_FUNC_DEFINE( className, funcName ) \
      _JS_STATIC_FUNC_DEFINE_( className, funcName, TRUE )

   #define JS_STATIC_FUNC_DEFINE_NORESET( className, funcName ) \
      _JS_STATIC_FUNC_DEFINE_( className, funcName, FALSE )

   #define _JS_GLOBAL_FUNC_DEFINE_( className, funcName, resetError )\
           static JSBool __##funcName(JSContext *cx, uintN argc , jsval *vp ) \
           {\
              JSBool ret = JS_TRUE ; \
              INT32 rc = SDB_OK ; \
              if ( resetError ) \
              { \
                 sdbClearErrorInfo() ; \
              } \
              else \
              { \
                 sdbSetNeedClearErrorInfo( FALSE ) ; \
              } \
              typedef INT32 (*FUNC)( const _sptArguments &,\
                                     _sptReturnVal &,\
                                     bson::BSONObj &);\
              rc = sptInvoker::callStaticFunc<FUNC>\
                         (cx, argc, vp, &className::funcName ) ;\
              if ( SDB_OK != rc )\
              {\
                 ret = JS_FALSE ;\
                 goto error ; \
              }\
           done:\
              return ret ;\
           error:\
              goto done ;\
           }

   #define JS_GLOBAL_FUNC_DEFINE( className, funcName ) \
      _JS_GLOBAL_FUNC_DEFINE_( className, funcName, TRUE )

   #define JS_GLOBAL_FUNC_DEFINE_NORESET( className, funcName ) \
      _JS_GLOBAL_FUNC_DEFINE_( className, funcName, FALSE )

   #define JS_DECLARE_CLASS( className )\
           public: \
           static className *crtInstance(){ return SDB_OSS_NEW className();} \
           static void releaseInstance( void *instance ) \
           { \
              className *p = ( className* )instance ; \
              SAFE_OSS_DELETE(p); \
           } \
           class __objDesc : public _sptObjDesc\
           { \
           public: \
              __objDesc() ; \
              virtual ~__objDesc(){}\
           }; \
           static __objDesc __desc ; \
           const _sptObjDesc* getDesc() \
           { \
              return &className::__desc ; \
           } \
           private:

   #define _JS_BEGIN_MAPPING_(className, jsClassName, hide ) \
           className::__objDesc className::__desc ; \
           sptObjAssist _SPT_UNIQUE_NAME( _tmp##className )( &className::__desc ) ; \
           className::__objDesc::__objDesc() \
           {\
              setClassName(jsClassName) ; \
              setHide( hide ) ;

   #define JS_BEGIN_MAPPING(className, jsClassName) \
           _JS_BEGIN_MAPPING_(className,jsClassName,FALSE)

   #define JS_BEGIN_MAPPING_WITHHIDE(className, jsClassName) \
           _JS_BEGIN_MAPPING_(className,jsClassName,TRUE)

   #define JS_BEGIN_MAPPING_WITHPARENT(className, jsClassName, parentClassName) \
           className::__objDesc className::__desc ; \
           sptObjAssist _SPT_UNIQUE_NAME( _tmp##className )( &className::__desc ) ; \
           className::__objDesc::__objDesc() \
           {\
              setClassName(jsClassName) ; \
              setParent( &parentClassName::__desc ) ;

   #define JS_ADD_MEMBER_FUNC( jsFuncName, funcName ) \
           _funcMap.addMemberFunc( jsFuncName, __##funcName ) ;

   #define JS_ADD_MEMBER_FUNC_WITHATTR( jsFuncName, funcName, attr ) \
           _funcMap.addMemberFunc( jsFuncName, __##funcName, attr ) ;

   #define JS_ADD_CONSTRUCT_FUNC( funcName ) \
           _funcMap.setConstructor( __##funcName ) ;

   #define JS_ADD_RESOLVE_FUNC( funcName ) \
           _funcMap.setResolver( __##funcName ) ;

   #define JS_ADD_DESTRUCT_FUNC( funcName ) \
           _funcMap.setDestructor( __##funcName ) ;

   #define JS_ADD_STATIC_FUNC( jsFuncName, funcName ) \
           _funcMap.addStaticFunc( jsFuncName, __##funcName ) ;

   #define JS_ADD_STATIC_FUNC_WITHATTR( jsFuncName, funcName, attr ) \
           _funcMap.addStaticFunc( jsFuncName, __##funcName, attr ) ;

   #define JS_ADD_GLOBAL_FUNC( jsFuncName, funcName ) \
           JS_ADD_STATIC_FUNC( jsFuncName, funcName )

   #define JS_ADD_GLOBAL_FUNC_WITHATTR( jsFuncName, funcName, attr ) \
           JS_ADD_STATIC_FUNC_WITHATTR( jsFuncName, funcName, attr )

   #define JS_SET_CVT_TO_BOOL_FUNC( funcName ) \
           setCVTToBoolFunc( funcName ) ;

   #define JS_SET_CVT_TO_INT_FUNC( funcName ) \
           setCVTToIntFunc( funcName ) ;

   #define JS_SET_CVT_TO_DOUBLE_FUNC( funcName ) \
           setCVTToDoubleFunc( funcName ) ;

   #define JS_SET_CVT_TO_STRING_FUNC( funcName ) \
           setCVTToStringFunc( funcName ) ;

   #define JS_SET_CVT_TO_BSON_FUNC( funcName ) \
           setCVTToBSONFunc( funcName ) ;

   #define JS_SET_SPECIAL_FIELD_NAME( fieldName ) \
           setSpecialFieldName( fieldName ) ;

   #define JS_SET_JSOBJ_TO_BSON_FUNC( funcName ) \
           setFMPToBSONFunc( funcName ) ;

   #define JS_SET_JSOBJ_TO_CURSOR_FUNC( funcName ) \
           setFMPToCursorFunc( funcName ) ;

   #define JS_SET_BSON_TO_JSOBJ_FUNC( funcName ) \
           setBSONToJSObjFunc( funcName ) ;

   #define JS_MAPPING_END() }


}

#endif

