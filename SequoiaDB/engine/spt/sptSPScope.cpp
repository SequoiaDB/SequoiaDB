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

   Source File Name = sptSPScope.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptSPScope.hpp"
#include "sptObjDesc.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "sptSPDef.hpp"
#include "sptBsonobj.hpp"
#include "sptBsonobjArray.hpp"
#include "sptGlobalFunc.hpp"
#include "sptConvertor2.hpp"
#include "sptConvertorHelper.hpp"
#include "sptCommon.hpp"
#include "spt.hpp"
#include "sptFuncDef.hpp"
#include "sptHelp.hpp"
#include "sptInvoker.hpp"
#include "../spt/js_in_cpp.hpp"

using namespace std ;

namespace engine
{
   #define JS_ERROBJ_FILENAME    "fileName"
   #define JS_ERROBJ_LINENO      "lineNumber"
   /*
      case 1: when no argument, we display the functions of class/instance
      case 2: when getting argument in format of "Oma"/"Oma.createCoord"
              /"createCoord" or something like "create" for fuzzy searching,
              we display the manpage of the specifed function
   */
   static JSBool __instance_help( JSContext *cx , uintN argc , jsval *vp )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      SDB_ASSERT( NULL != cx && NULL != vp, "can not be NULL" ) ;

      jsval jsVal = JSVAL_VOID ;
      JSObject *constructor = NULL ;
      JSString *jsStr = NULL ;
      CHAR *pStr = NULL ;
      _sptSPArguments arg( cx, argc, vp ) ;
      string jsClassName ;

      JS_SET_RVAL( cx, vp, JSVAL_VOID ) ;
      constructor = JS_GetConstructor( cx, JS_THIS_OBJECT ( cx , vp ) ) ;
      if ( !constructor )
      {
         rc = SDB_SYS ;
         ss << "Failed to get constructor" ;
         goto error ;
      }
      if ( !JS_GetProperty( cx, constructor, "name", &jsVal ) ||
           !JSVAL_IS_STRING( jsVal ) )
      {
         rc = SDB_SYS ;
         ss << "Failed to get the js class name" ;
         goto error ;
      }
      jsStr = JS_ValueToString( cx, jsVal ) ;
      if ( !jsStr )
      {
         rc = SDB_SYS ;
         ss << "Failed to convert js class name" ;
         goto error ;
      }
      pStr = JS_EncodeString( cx, jsStr ) ;
      if ( !pStr )
      {
         rc = SDB_SYS ;
         ss << "Failed to encode js class name" ;
         goto error ;
      }
      jsClassName.assign( pStr ) ;
      JS_free( cx, pStr ) ;

      if ( arg.argc() == 0 )
      {
         rc = sptHelp::getInstance().displayMethod( jsClassName, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else if ( arg.argc() >= 1 )
      {
         string fuzzyFuncName ;
         rc = arg.getString( 0, fuzzyFuncName ) ;
         if ( rc )
         {
            ss << "The 1st param must be string"  ;
            goto error ;
         }
         rc = sptHelp::getInstance().displayManual( fuzzyFuncName,
                                                    jsClassName, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      if ( !ss.str().empty() )
      {
         std::cout << ss.str().c_str() << std::endl ;
      }
      goto done ;
   }

   static JSBool __static_help( JSContext *cx , uintN argc , jsval *vp )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      SDB_ASSERT( NULL != cx && NULL != vp, "can not be NULL" ) ;

      jsval jsVal = JSVAL_VOID ;
      JSString *jsStr = NULL ;
      CHAR *pStr = NULL ;
      _sptSPArguments arg( cx, argc, vp ) ;
      string jsClassName ;

      JS_SET_RVAL( cx, vp, JSVAL_VOID ) ;

      if ( !JS_GetProperty( cx, JS_THIS_OBJECT ( cx , vp ), "name", &jsVal ) ||
           !JSVAL_IS_STRING( jsVal ) )
      {
         rc = SDB_SYS ;
         ss << "Failed to get the js class name" ;
         goto error ;
      }
      jsStr = JS_ValueToString( cx, jsVal ) ;
      if ( !jsStr )
      {
         rc = SDB_SYS ;
         ss << "Failed to convert string" ;
         goto error ;
      }
      pStr = JS_EncodeString( cx, jsStr ) ;
      if ( !pStr )
      {
         rc = SDB_SYS ;
         ss << "Failed to encode string" ;
         goto error ;
      }
      jsClassName.assign( pStr ) ;
      JS_free( cx, pStr ) ;

      if ( arg.argc() == 0 )
      {
         rc = sptHelp::getInstance().displayMethod( jsClassName, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else if ( arg.argc() >= 1 )
      {
         string fuzzyFuncName ;
         rc = arg.getString( 0, fuzzyFuncName ) ;
         if ( rc )
         {
            ss << "The 1st param must be string"  ;
            goto error ;
         }
         rc = sptHelp::getInstance().displayManual( fuzzyFuncName,
                                                    jsClassName, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      if ( !ss.str().empty() )
      {
         std::cout << ss.str().c_str() << std::endl ;
      }
      goto done ;
   }

   /*
      Local function define
   */
   static JSClass global_class = {
   "Global",                     // class name
   JSCLASS_GLOBAL_FLAGS,         // flags
   JS_PropertyStub,              // addProperty
   JS_PropertyStub,              // delProperty
   JS_PropertyStub,              // getProperty
   JS_StrictPropertyStub,        // setProperty
   JS_EnumerateStub,             // enumerate
   JS_ResolveStub,               // resolve
   JS_ConvertStub,               // convert
   JS_FinalizeStub,              // finalize
   JSCLASS_NO_OPTIONAL_MEMBERS   // optional members
   } ;

   #define SPT_RVAL_KEY          ""
   const UINT32 RUNTIME_SIZE = 64 * 1024 * 1024 ;

   /*
      _sptSPResultVal implement
   */
   _sptSPResultVal::_sptSPResultVal()
   :_value( JSVAL_VOID )
   {
      _ctx = NULL ;
   }

   _sptSPResultVal::~_sptSPResultVal()
   {
   }

   const void* _sptSPResultVal::rawPtr() const
   {
      return (void*)&_value ;
   }

   bson::BSONObj _sptSPResultVal::toBSON() const
   {
      bson::BSONObj obj ;
      _rval2obj( _ctx, _value, obj ) ;
      return obj ;
   }

   void _sptSPResultVal::reset( JSContext *ctx )
   {
      _ctx = ctx ;
      _errStr.clear() ;
      _value = JSVAL_VOID ;
   }

   INT32 _sptSPResultVal::_rval2obj( JSContext *cx,
                                     const jsval &jsrval,
                                     bson::BSONObj &rval ) const
   {
      INT32 rc = SDB_OK ;
      bson::BSONObjBuilder builder ;

      if ( JSVAL_IS_VOID( jsrval ) )
      {
      }
      else if ( JSVAL_IS_STRING( jsrval ) )
      {
         std::string v ;
         rc = sptConvertor2::toString( cx, jsrval, v ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         builder.append( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_INT( jsrval ) )
      {
         int32 v = 0 ;
         if ( !JS_ValueToInt32( cx, jsrval, &v ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.append( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_DOUBLE( jsrval ) )
      {
         jsdouble v ;
         if ( !JS_ValueToNumber( cx, jsrval, &v ))
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.appendNumber( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_BOOLEAN( jsrval ) )
      {
         JSBool v ;
         if ( !JS_ValueToBoolean( cx, jsrval, &v ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.appendBool( SPT_RVAL_KEY, v ) ;
      }
      else if ( JSVAL_IS_OBJECT( jsrval ) )
      {
         JSObject *obj = JSVAL_TO_OBJECT( jsrval ) ;
         if ( JSObjIsBsonobj( cx, obj ) )
         {
            CHAR *rawData = NULL ;
            rc = getBsonRawFromBsonClass( cx, obj, &rawData ) ;
            if ( rc )
            {
               goto error ;
            }
            else if ( !rawData )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            builder.append( SPT_RVAL_KEY, bson::BSONObj( rawData ) ) ;
         }
         else if ( _sptBsonobj::__desc.isInstanceOf( cx, obj ) )
         {
            _sptBsonobj *p = (_sptBsonobj*)JS_GetPrivate( cx, obj ) ;
            if ( NULL == p )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            builder.append( SPT_RVAL_KEY, p->getBson() ) ;
         }
         else if ( _sptBsonobjArray::__desc.isInstanceOf( cx, obj ) )
         {
            _sptBsonobjArray *p = (_sptBsonobjArray*)JS_GetPrivate( cx, obj ) ;
            const vector<bson::BSONObj> vecObjs = p->getBsonArray() ;
            bson::BSONArrayBuilder sub( builder.subarrayStart( SPT_RVAL_KEY ) ) ;
            for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
            {
               sub.append( vecObjs[ i ] ) ;
            }
            sub.done() ;
         }
         else if ( !JSObjIsSdbObj( cx, JSVAL_TO_OBJECT( jsrval ) ) )
         {
            sptConvertor2 c( cx ) ;
            bson::BSONObj v ;
            rc = c.toBson( JSVAL_TO_OBJECT( jsrval ), v ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            builder.append( SPT_RVAL_KEY, v ) ;
         }
      }
      else
      {
         ossPrintf( "the type[%d] is not supported yet"OSS_NEWLINE,
                    JS_TypeOfValue( cx, jsrval ) ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rval = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _sptSPScope define
   */
   _sptSPScope::_sptSPScope()
   :_runtime( NULL ),
    _context( NULL )
   {
   }

   _sptSPScope::~_sptSPScope()
   {
      shutdown() ;
   }

   INT32 _sptSPScope::start( UINT32 loadMask )
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _runtime )
      {
         ossPrintf( "scope has already been started up"OSS_NEWLINE) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _runtime = JS_NewRuntime( RUNTIME_SIZE );
      if ( NULL == _runtime )
      {
         ossPrintf( "failed to init js runtime"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _context = JS_NewContext( _runtime, RUNTIME_SIZE / 8 );
      if ( NULL == _context )
      {
         ossPrintf( "failed to init js context"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      JS_SetOptions( _context, JSOPTION_VAROBJFIX | JSOPTION_WERROR );
      JS_SetVersion( _context, JSVERSION_LATEST );
      JS_SetErrorReporter( _context, sdbReportError ) ;

      _global = JS_NewCompartmentAndGlobalObject( _context, &global_class,
                                                  NULL ) ;
      if ( NULL == _global )
      {
         ossPrintf( "failed to init js global object"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JS_InitStandardClasses( _context, _global ) )
      {
         ossPrintf( "failed to init standard class"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _loadObj( loadMask ) ;
      if ( rc )
      {
         ossPrintf( "Failed to load object: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }
      _loadMask = loadMask ;

      {
         sptPrivateData *privateData = SDB_OSS_NEW sptPrivateData( this ) ;
         if( NULL == privateData )
         {
            ossPrintf( "Failed to new sptPrivateData obj" ) ;
            goto error ;
         }
         JS_SetContextPrivate( _context, privateData ) ;
      }
   done:
      return rc ;
   error:
      shutdown() ;
      goto done ;
   }

   INT32 _sptSPScope::_loadObj( UINT32 loadMask )
   {
      INT32 rc = SDB_OK ;

      if ( loadMask & SPT_OBJ_MASK_STANDARD )
      {
         if ( !InitDbClasses( _context, _global ) )
         {
            ossPrintf( "Failed to init dbclass"OSS_NEWLINE ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( loadMask & SPT_OBJ_MASK_USR )
      {
         SPT_VEC_OBJDESC vecObjs ;
         sptGetObjFactory()->getObjDescs( vecObjs ) ;
         for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
         {
            sptObjDesc *desc = (sptObjDesc*)vecObjs[ i ] ;

            rc = loadUsrDefObj( desc ) ;
            if ( rc )
            {
               ossPrintf( "Load object[%s] failed, rc: %d"OSS_NEWLINE,
                          desc->getJSClassName(), rc ) ;
               goto error ;
            }
         }
      }

      if ( loadMask & SPT_OBJ_MASK_INNER_JS )
      {
         rc = evalInitScripts( this ) ;
         if ( rc )
         {
            ossPrintf ( "Failed to init spt scope, rc = %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptSPScope::shutdown()
   {
      if ( NULL != _context )
      {
         sptPrivateData* p = (sptPrivateData*)JS_GetContextPrivate( _context ) ;
         if ( NULL != p )
         {
            SDB_OSS_DEL p ;
         }

         JS_SetContextPrivate( _context, NULL ) ;

         JS_EndRequest(_context) ;
         JS_DestroyContext( _context ) ;
         _context = NULL ;
      }

      if ( NULL != _runtime )
      {
         JS_DestroyRuntime( _runtime ) ;
         _runtime = NULL ;
         JS_ShutDown() ;
      }

      _global = NULL ;
   }

   INT32 _sptSPScope::_loadUsrDefObj( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      if ( !desc->isIgnoredName() )
      {
         rc = _loadUsrClass( desc ) ;
      }
      else
      {
         rc = _loadGlobal( desc ) ;
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

   INT32 _sptSPScope::_loadGlobal( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      const _sptFuncMap &fMap = desc->getFuncMap() ;
      const sptFuncMap::NORMAL_FUNCS &funcs = fMap.getStaticFuncs() ;
      JSFunctionSpec *specs = new JSFunctionSpec[funcs.size() + 1] ;
      if ( NULL == specs )
      {
         ossPrintf( "failed to allocate mem."OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      {
      UINT32 i = 0 ;
      sptFuncMap::NORMAL_FUNCS::const_iterator itr = funcs.begin() ;
      for ( ; i < funcs.size() ; i++, itr++ )
      {
         specs[i].name = itr->first.c_str() ;
         specs[i].call = itr->second._pFunc ;
         specs[i].nargs = 0 ;
         specs[i].flags = itr->second._attr ;
      }
      specs[i].name = NULL ;
      specs[i].call = NULL ;
      specs[i].nargs = 0 ;
      specs[i].flags = 0 ;

      if ( !JS_DefineFunctions( _context, _global, specs ) )
      {
         ossPrintf( "failed to define global functions"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      }
   done:
      delete []specs ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPScope::_loadUsrClass( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      JSObject *prototype = NULL ;
      const sptObjDesc *parentDesc = NULL ;
      JSObject *parent_proto = NULL ;
      const CHAR *objName = desc->getJSClassName() ;
      const _sptFuncMap &fMap = desc->getFuncMap() ;
      JS_INVOKER::MEMBER_FUNC construct = fMap.getConstructor() ;
      JS_INVOKER::DESTRUCT_FUNC destruct = fMap.getDestructor() ;
      JS_INVOKER::RESLOVE_FUNC resolve = fMap.getResolver() ;

      uint32 flags = NULL == resolve ?
                     JSCLASS_HAS_PRIVATE :
                     JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE ;

      JSResolveOp resolveOp = NULL == resolve ?
                              JS_ResolveStub : (JSResolveOp)resolve ;

      JSClass cDef = { ( CHAR * )objName,
                    flags,
                    JS_PropertyStub,
                    JS_PropertyStub,
                    JS_PropertyStub,
                    JS_StrictPropertyStub,
                    JS_EnumerateStub,
                    resolveOp,
                    JS_ConvertStub,
                    destruct,
                    JSCLASS_NO_OPTIONAL_MEMBERS } ;

      desc->setClassDef( cDef ) ;

      const sptFuncMap::NORMAL_FUNCS &memberFuncs = fMap.getMemberFuncs() ;
      const sptFuncMap::NORMAL_FUNCS &staticFuncs = fMap.getStaticFuncs() ;

      JSFunctionSpec *fSpecs = NULL ;
      JSFunctionSpec *sfSpecs = NULL ;

      if ( !desc->isIgnoredParent() )
      {
         parentDesc = desc->getParent() ;
         if ( !parentDesc )
         {
            ossPrintf( "Get object[%s]'s parent object failed"OSS_NEWLINE,
                       desc->getJSClassName() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         parent_proto = (JSObject*)parentDesc->getPrototypeDef() ;
      }

      fSpecs = new JSFunctionSpec[memberFuncs.size() + 1 + 1] ;
      if ( NULL == fSpecs )
      {
         ossPrintf( "failed to allocate mem."OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      sfSpecs = new JSFunctionSpec[staticFuncs.size() + 1 + 1] ;
      if ( NULL == sfSpecs )
      {
         ossPrintf( "failed to allocate mem."OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      {
         UINT32 i = 0 ;
         sptFuncMap::NORMAL_FUNCS::const_iterator itr = memberFuncs.begin() ;
         for ( ; i < memberFuncs.size() ; i++, itr++ )
         {
            fSpecs[i].name = itr->first.c_str() ;
            fSpecs[i].call = itr->second._pFunc ;
            fSpecs[i].nargs = 0 ;
            fSpecs[i].flags = itr->second._attr ;
         }
         if ( memberFuncs.end() != memberFuncs.find( "help" ) )
         {
            fSpecs[i].name = NULL ;
            fSpecs[i].call = NULL ;
            fSpecs[i].nargs = 0 ;
            fSpecs[i].flags = 0 ;
         }
         else
         {
            fSpecs[i].name = "help" ;
            fSpecs[i].call = __instance_help ;
            fSpecs[i].nargs = 0 ;
            fSpecs[i].flags = SPT_FUNC_DEFAULT ;
         }
         i++ ;
         fSpecs[i].name = NULL ;
         fSpecs[i].call = NULL ;
         fSpecs[i].nargs = 0 ;
         fSpecs[i].flags = 0 ;

         i = 0 ;
         itr = staticFuncs.begin() ;
         for ( ; i < staticFuncs.size() ; i++, itr++ )
         {
            sfSpecs[i].name = itr->first.c_str() ;
            sfSpecs[i].call = itr->second._pFunc ;
            sfSpecs[i].nargs = 0 ;
            sfSpecs[i].flags = itr->second._attr ;
         }
         if ( staticFuncs.end() != staticFuncs.find( "help" ) )
         {
            sfSpecs[i].name = NULL ;
            sfSpecs[i].call = NULL ;
            sfSpecs[i].nargs = 0 ;
            sfSpecs[i].flags = 0 ;
         }
         else
         {
            sfSpecs[i].name = "help" ;
            sfSpecs[i].call = __static_help ;
            sfSpecs[i].nargs = 0 ;
            sfSpecs[i].flags = SPT_FUNC_DEFAULT ;
         }
         i++ ;
         sfSpecs[i].name = NULL ;
         sfSpecs[i].call = NULL ;
         sfSpecs[i].nargs = 0 ;
         sfSpecs[i].flags = 0 ;

         prototype = JS_InitClass( _context, /// context
                                   _global,  /// object
                                   parent_proto,  /// parent_proto
                                   (JSClass*)desc->getClassDef(), /// class
                                   construct, /// constructor
                                   0, /// nargs
                                   0, /// ps
                                   fSpecs, /// fs
                                   0, /// static_ps
                                   sfSpecs /// static_fs
                                   ) ;

         if ( !prototype )
         {
            ossPrintf( "failed to call js_initclass"OSS_NEWLINE ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         desc->setClassPrototype( prototype ) ;
      }

   done:
      delete []fSpecs ;
      delete []sfSpecs ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPScope::eval( const CHAR *code, UINT32 len,
                            const CHAR *filename,
                            UINT32 lineno,
                            INT32 flag,
                            const sptResultVal **ppRval )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( _context && _global, "this scope has not been initilized" ) ;
      SDB_ASSERT( NULL != code || 0 < len, "code can not be empty" ) ;
      jsval exception = JSVAL_VOID ;
      CHAR *print = NULL ;

      _rval.reset( _context ) ;
      jsval *pRval = ( jsval* )_rval.rawPtr() ;

      sdbSetPrintError( ( flag & SPT_EVAL_FLAG_PRINT ) ? TRUE : FALSE ) ;
      sdbSetIgnoreErrorPrefix( ( flag & SPT_EVAL_FLAG_IGNORE_ERR_PREFIX ) ?
                               TRUE : FALSE ) ;
      sdbSetNeedClearErrorInfo( TRUE ) ;

      if ( !JS_EvaluateScript( _context, _global, code,
                               len, filename, lineno,
                               pRval ) )
      {
         rc = sdbGetErrno() ? sdbGetErrno() : SDB_SPT_EVAL_FAIL ;
         goto error ;
      }

      if ( flag & SPT_EVAL_FLAG_PRINT )
      {
         if ( !JSVAL_IS_VOID ( *pRval ) )
         {
            print = convertJsvalToString ( _context , *pRval ) ;
            if ( !print )
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }

         if ( NULL != print && print[0] != '\0' )
         {
            ossPrintf( "%s"OSS_NEWLINE, print ) ;
         }
      }

      if ( sdbIsNeedClearErrorInfo() &&
           !JS_IsExceptionPending( _context ) )
      {
         sdbClearErrorInfo() ;
      }

   done:
      if ( ppRval )
      {
         *ppRval = &_rval ;
      }
      SAFE_JS_FREE ( _context , print ) ;
      return rc ;
   error:
      if ( JS_IsExceptionPending( _context ) &&
           JS_GetPendingException ( _context , &exception ) )
      {
         CHAR *strException = NULL ;
         JSString *jsstr = JS_ValueToString( _context, exception ) ;
         if ( NULL != jsstr )
         {
            strException = JS_EncodeString ( _context, jsstr ) ;
         }

         if ( NULL != strException )
         {
            std::stringstream errMsg ;
            std::stringstream errPrefix ;
            sptPrivateData *privateData  = NULL ;

            privateData = ( sptPrivateData* ) JS_GetContextPrivate( _context ) ;
            /*
             * Branch 1: userdef function throw errno
             *    true == privateData->isSetErrInfo() means exception occurs in
             *    userdef function
             */
            if( NULL != privateData && privateData->isSetErrInfo() )
            {
               {
                  errPrefix << privateData->getErrFileName().c_str() << ":"
                            << privateData->getErrLineno() << " " ;
               }
               if( flag & SPT_EVAL_FLAG_IGNORE_ERR_PREFIX && !sdbIsErrMsgEmpty() )
               {
                  errMsg << sdbGetErrMsg() ;
               }
               else
               {
                  errMsg << "uncaught exception: "
                         << strException << endl
                         << sdbGetErrMsg() ;
               }
            }
            /*
             * Branch 2: throw obj
             *    unable to determine if the obj type is Error
             *
             * TODO: find a way to determine if the obj type is Error
             */
            else if( JSVAL_IS_OBJECT( exception ) )
            {
               CHAR *errfileName = NULL ;
               UINT32 errLineno = 0 ;
               JSObject *errObj = NULL ;
               jsval fileName ;
               jsval lineNumber ;

               errObj = JSVAL_TO_OBJECT( exception ) ;
               if( NULL != errObj )
               {
                  if( JS_GetProperty( _context, errObj,
                                      JS_ERROBJ_FILENAME, &fileName )
                      && JSVAL_IS_STRING( fileName ) )
                  {
                     JSString *jsStr = JSVAL_TO_STRING( fileName ) ;
                     if( NULL != jsStr )
                     {
                        errfileName = JS_EncodeString ( _context , jsStr ) ;
                     }
                  }
                  if( JS_GetProperty( _context, errObj,
                                      JS_ERROBJ_LINENO, &lineNumber )
                      && JSVAL_IS_INT( lineNumber ) )
                  {
                     errLineno = (UINT32) JSVAL_TO_INT( lineNumber ) ;
                  }
               }
               errPrefix << ( errfileName ? string( errfileName ): "(nofile)" ) << ":"
                         << errLineno << " " ;
               errMsg << strException ;
            }
            /*
             *Branch 3: Throw other type, such as string
             */
            else
            {
               errPrefix << ( filename ? string( filename ): "(nofile)" )
                         << ":"
                         << lineno << " " ;
               errMsg << "uncaught exception: "
                      << strException ;
            }

            std::string errInfo ;
            if( flag & SPT_EVAL_FLAG_IGNORE_ERR_PREFIX )
            {
               errInfo = errMsg.str() ;
            }
            else
            {
               errInfo = errPrefix.str() + errMsg.str() ;
            }
            _rval.setError( errInfo ) ;
            SAFE_JS_FREE( _context, strException ) ;
         }
         JS_ReportPendingException( _context ) ;
      }
      goto done ;
   }

   BOOLEAN _sptSPScope::isInstanceOf( const void *pObj,
                                      const string &objName )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->isInstanceOf( _context, pJSObj, objName ) ;
   }

   string _sptSPScope::getObjClassName( const void *pObj )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->getClassName( _context, pJSObj ) ;
   }

   void _sptSPScope::getGlobalFunNames( set< string > &setFunc,
                                        BOOLEAN showHide )
   {
      sptGetObjFactory()->getObjStaticFunNames( _context, _global,
                                                setFunc, showHide ) ;
   }

   void _sptSPScope::getObjStaticFunNames( const string &objName,
                                           set< string > &setFunc,
                                           BOOLEAN showHide )
   {
      sptGetObjFactory()->getClassStaticFuncNames( _context, objName,
                                                   setFunc, showHide ) ;
   }

   void _sptSPScope::getObjFunNames( const void *pObj,
                                     set< string > &setFunc,
                                     BOOLEAN showHide )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->getObjFuncNames( _context, pJSObj,
                                                  setFunc, showHide ) ;
   }

   void _sptSPScope::getObjPropNames( const void *pObj,
                                      set< string > &setProp )
   {
      JSObject *pJSObj = ( JSObject* )pObj ;
      return sptGetObjFactory()->getObjPropNames( _context, pJSObj, setProp ) ;
   }
}

