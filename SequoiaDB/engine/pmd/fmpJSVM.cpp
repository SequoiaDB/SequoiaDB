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

   Source File Name = fmpJSVM.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "fmpJSVM.hpp"
#include "spt.hpp"
#include "ossMem.hpp"
#include "sptContainer.hpp"
#include "sptSPScope.hpp"
#include "sptConvertor.hpp"
#include "sptObjDesc.hpp"
#include "sptSPObject.hpp"
#include "client.hpp"
#include "pd.hpp"

using namespace bson ;
using namespace engine ;

_fmpJSVM::_fmpJSVM()
:_engine( NULL ),
 _scope( NULL ),
 _cursor(NULL)
{
   _engine = SDB_OSS_NEW _sptContainer() ;
   _scope = _engine->newScope() ;
   if ( NULL == _scope )
   {
      PD_LOG( PDERROR, "failed to new scope" ) ;
      return ;
   }
   _setOK( TRUE ) ;
}

_fmpJSVM::~_fmpJSVM()
{
   SAFE_OSS_DELETE( _scope ) ;
   SAFE_OSS_DELETE( _engine ) ;
   /// cursor was get from JSObject, do not free it.
   _cursor = NULL ;
}

INT32 _fmpJSVM::initGlobalDB( const CHAR *pHostName,
                              const CHAR *pSvcname,
                              const CHAR *pUser,
                              const CHAR *pPasswd,
                              BSONObj &res )
{
   INT32 rc = SDB_OK ;
   StringBuilder ss ;
   BSONObjBuilder builder ;

   try
   {
      ss << "var db = new Sdb( "
         << "'" << pHostName << "', "
         << "'" << pSvcname << "', "
         << "'" << pUser << "', "
         << "'" << pPasswd << "' ) ;" ;

      builder.appendCode( FMP_FUNC_VALUE, ss.poolStr() ) ;
      builder.append( FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS ) ;

      rc = eval( builder.obj(), res ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   catch( std::exception &e )
   {
      rc = SDB_OOM ;
      res = BSON( FMP_ERR_MSG << "e.what()" <<
                  FMP_RES_CODE << rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpJSVM::_getValType( const sptSPVal *pVal ) const
{
   if ( pVal->isNull() )
   {
      return FMP_RES_TYPE_NULL ;
   }
   else if ( pVal->isVoid() )
   {
      return FMP_RES_TYPE_VOID ;
   }
   else if ( pVal->isNumber() )
   {
      return FMP_RES_TYPE_NUMBER ;
   }
   else if ( pVal->isString() )
   {
      return FMP_RES_TYPE_STR ;
   }
   else if ( pVal->isBoolean() )
   {
      return FMP_RES_TYPE_BOOL ;
   }
   else if( pVal->isSPTObject() )
   {
      return FMP_RES_TYPE_SPECIALOBJ ;
   }
   else if ( pVal->isObject() )
   {
      return FMP_RES_TYPE_OBJ ;
   }

   return FMP_RES_TYPE_VOID ;
}

INT32 _fmpJSVM::eval( const BSONObj &func, BSONObj &res )
{
   INT32 rc = SDB_OK ;
   BSONElement ele = func.getField( FMP_FUNC_VALUE ) ;
   BSONElement type ;
   const _sptResultVal *pRval = NULL ;

   if ( ele.eoo() || Code != ele.type() )
   {
      PD_LOG( PDERROR, "invalid func type: %d", ele.type() ) ;
      rc = SDB_INVALIDARG ;
      res = BSON( FMP_ERR_MSG << "type of element must be Code" <<
                  FMP_RES_CODE << rc ) ;
      goto error ;
   }

   type = func.getField( FMP_FUNC_TYPE ) ;
   if ( type.eoo() || NumberInt != type.type() ||
        FMP_FUNC_TYPE_JS != type.Int() )
   {
      rc = SDB_INVALIDARG ;
      res = BSON( FMP_ERR_MSG << "type of func must be JS" <<
                  FMP_RES_CODE << rc ) ;
      goto error ;
   }

   rc = _transCode2Str( ele, _cmd ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to trans code to str:%d", rc ) ;
      res = BSON( FMP_ERR_MSG << "trans code to str failed" <<
                  FMP_RES_CODE << rc ) ;
      goto error ;
   }

   rc = _scope->eval( _cmd.c_str(), _cmd.length(),
                      NULL, 1, SPT_EVAL_FLAG_NONE,
                      &pRval ) ;
   if ( SDB_OK != rc )
   {
      const CHAR *pLastErr = _scope->getLastErrMsg() ;
      INT32 lastErrno = _scope->getLastError() ;

      if ( !*pLastErr && pRval->hasError() )
      {
         pLastErr = pRval->getErrrInfo() ;
      }
      res = BSON( FMP_ERR_MSG << pLastErr <<
                  FMP_RES_CODE << lastErrno ) ;
      goto error ;
   }

   {
      BSONObjBuilder builder ;
      const sptSPVal *pVal = pRval->getVal() ;
      JSContext *pContext = ((sptSPScope*)_scope)->getContext() ;
      const sptObjDesc *desc = NULL ;
      sptConvertor convertor( pContext ) ;

      if ( !pVal->isSPTObject( NULL, NULL, &desc ) )
      {
         builder.append( FMP_RES_TYPE, _getValType( pVal ) ) ;
         rc = convertor.appendToBson( FMP_RES_VALUE, *pVal, builder ) ;
         if ( rc )
         {
            res = BSON( FMP_ERR_MSG << convertor.getErrMsg() <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }
         res = builder.obj() ;
      }
      else
      {
         string errMsg ;
         JSObject *obj = NULL ;
         BSONObj val ;

         obj = JSVAL_TO_OBJECT( *(pVal->valuePtr()) ) ;
         if ( !obj )
         {
            rc = SDB_SYS ;
            res = BSON( FMP_ERR_MSG << "Failed to convert jsval to object" <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }

         rc = desc->fmpToBSON( sptSPObject( pContext, obj ), val, errMsg ) ;
         if( SDB_OK != rc )
         {
            res = BSON( FMP_ERR_MSG << errMsg <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }

         rc = desc->fmpToCursor( sptSPObject( pContext, obj ), &_cursor,
                                 errMsg ) ;
         if( SDB_OK != rc )
         {
            res = BSON( FMP_ERR_MSG << errMsg <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }
         if( NULL != _cursor )
         {
            res = BSON( FMP_RES_TYPE << FMP_RES_TYPE_RECORDSET ) ;
         }
         else
         {
            res = BSON( FMP_RES_TYPE << FMP_RES_TYPE_SPECIALOBJ <<
                        FMP_RES_VALUE << val <<
                        FMP_RES_CLASSNAME << desc->getJSClassName() ) ;
         }
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpJSVM::fetch( BSONObj &res )
{
   INT32 rc = SDB_OK ;
   BSONObj record ;
   rc = _cursor->next( record ) ;
   if ( SDB_DMS_EOC == rc )
   {
      res = BSON( FMP_RES_CODE << rc ) ;
      goto error ;
   }
   else if ( SDB_OK == rc )
   {
      res = BSON( FMP_RES_TYPE << FMP_RES_TYPE_OBJ <<
                  FMP_RES_VALUE << record ) ;
   }
   else
   {
      res = BSON( FMP_ERR_MSG << "failed to getnext" <<
                  FMP_RES_CODE << rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpJSVM::_transCode2Str( const BSONElement &ele,
                                std::string &str )
{
   INT32 rc = SDB_OK ;
   str = ele.code() ;
   return rc ;
}
