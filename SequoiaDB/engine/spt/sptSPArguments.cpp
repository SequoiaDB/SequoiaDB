/******************************************************************************


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

   Source File Name = sptSPArguments.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptSPArguments.hpp"
#include "sptSPDef.hpp"
#include "pd.hpp"
#include "sptConvertor.hpp"
#include "sptObjDesc.hpp"
#include "sptSPVal.hpp"
#include "sptSPObject.hpp"

using namespace bson ;

namespace engine
{
   _sptSPArguments::_sptSPArguments( JSContext *context, uintN argc,
                                     jsval *vp, JSObject *pObj )
   :_context(context),
    _argc(argc),
    _vp(vp),
    _pObject( NULL )
   {
      SDB_ASSERT( NULL != _context && NULL != _vp, "can not be NULL" ) ;
      if ( pObj )
      {
         _pObject = SDB_OSS_NEW sptSPObject( _context, pObj ) ;
         SDB_ASSERT( _pObject, "Alloc out-of-memory" ) ;
      }
   }

   _sptSPArguments::~_sptSPArguments()
   {
      if ( _pObject )
      {
         SDB_OSS_DEL _pObject ;
         _pObject = NULL ;
      }
      _context = NULL ;
      _vp = NULL ;
   }

   INT32 _sptSPArguments::getString( UINT32 pos,
                                     std::string &value,
                                     BOOLEAN strict ) const
   {
      INT32 rc = SDB_OK ;
      sptSPVal spVal ;
      jsval *val = NULL ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         rc = SDB_SYS ;
         goto error ;
      }

      spVal.reset( _context, *val ) ;

      /// strict for String
      if ( strict )
      {
         if ( !spVal.isString() )
         {
            _errMsg = "Paramter is not string" ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      rc = spVal.toString( value ) ;
      if ( rc )
      {
         _errMsg = "Failed to convert a jsval to string" ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   jsval *_sptSPArguments::_getValAtPos( UINT32 pos ) const
   {
      return JS_ARGV( _context, _vp ) + pos ;
   }

   INT32 _sptSPArguments::getBsonobj( UINT32 pos,
                                      bson::BSONObj &value,
                                      BOOLEAN strict,
                                      BOOLEAN allowNull ) const
   {
      INT32 rc = SDB_OK ;
      jsval *val = NULL ;
      sptSPVal spVal ;
      sptConvertor convertor( _context, strict ) ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         rc = SDB_SYS ;
         goto error ;
      }

      spVal.reset( _context, *val ) ;

      if ( spVal.isNull() )
      {
         if ( !allowNull )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            goto done ;
         }
      }
      else if ( !spVal.isObject() )
      {
         _errMsg = "Parameter is not a object" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         rc = convertor.toBson( &spVal, value ) ;
         if ( SDB_OK != rc )
         {
            _errMsg = convertor.getErrMsg() ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPArguments::getArray( UINT32 pos, vector< bson::BSONObj > &value,
                                    BOOLEAN strict ) const
   {
      INT32 rc = SDB_OK ;
      JSObject *jsObj = NULL ;
      jsval *val = NULL ;
      sptConvertor convertor( _context, strict ) ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JSVAL_IS_OBJECT( *val ) )
      {
         _errMsg = "Parameter is not Object" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      jsObj = JSVAL_TO_OBJECT( *val ) ;
      if ( NULL == jsObj )
      {
         _errMsg = "Failed to convert jsval to Object" ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = convertor.toObjArray( jsObj, value ) ;
      if ( SDB_OK != rc )
      {
         _errMsg = convertor.getErrMsg() ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPArguments::getArray( UINT32 pos, vector< string > &value,
                                    BOOLEAN strict ) const
   {
      INT32 rc = SDB_OK ;
      JSObject *jsObj = NULL ;
      jsval *val = NULL ;
      sptConvertor convertor( _context, strict ) ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JSVAL_IS_OBJECT( *val ) )
      {
         _errMsg = "Parameter is not Object" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      jsObj = JSVAL_TO_OBJECT( *val ) ;
      if ( NULL == jsObj )
      {
         _errMsg = "Failed to convert jsval to object" ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = convertor.toStrArray( jsObj, value ) ;
      if ( SDB_OK != rc )
      {
         _errMsg = convertor.getErrMsg() ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptSPArguments::getUserObj( UINT32 pos, const _sptObjDesc &objDesc,
                                      const void** value ) const
   {
      INT32 rc = SDB_OK ;
      JSObject *jsObj = NULL ;
      jsval *val = NULL ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JSVAL_IS_OBJECT( *val ) )
      {
         _errMsg = "Parameter is not Object" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      jsObj = JSVAL_TO_OBJECT( *val ) ;
      if ( NULL == jsObj )
      {
         _errMsg = "Failed to convert jsval to object" ;
         rc = SDB_SYS ;
         goto error ;
      }

      if( string( objDesc.getJSClassName() ) !=
          sptGetObjFactory()->getClassName( _context, jsObj ) )
      {
         rc = SDB_INVALIDARG ;
         _errMsg = "Object is not the instance of " ;
         _errMsg += objDesc.getJSClassName() ;
         goto error ;
      }

      *value = JS_GetPrivate( _context, jsObj ) ;
      if( *value == NULL )
      {
         rc = SDB_SYS ;
         _errMsg = "Faild to convert jsobj to user Object" ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   sptPrivateData* _sptSPArguments::getPrivateData( ) const
   {
      return ( sptPrivateData* )JS_GetContextPrivate( _context ) ;
   }

   BOOLEAN _sptSPArguments::isString( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_STRING( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isNull( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_NULL( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isVoid( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_VOID( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isInt( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_INT( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isDouble( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_DOUBLE( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isNumber( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_NUMBER( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isObject( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_OBJECT( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isBoolean( UINT32 pos ) const
   {
      jsval *val = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_BOOLEAN( *val ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isUserObj( UINT32 pos,
                                       const _sptObjDesc &objDesc ) const
   {
      jsval *val = NULL ;
      JSObject *jsObj = NULL ;

      if ( _argc > pos &&
           NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_OBJECT( *val ) &&
           NULL != ( jsObj = JSVAL_TO_OBJECT( *val ) ) &&
           string( objDesc.getJSClassName() ) ==
           sptGetObjFactory()->getClassName( _context, jsObj ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _sptSPArguments::isArray( UINT32 pos ) const
   {
      jsval *val = NULL ;
      JSObject *jsObj = NULL ;
      if ( _argc > pos && NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_OBJECT( *val ) &&
           NULL != ( jsObj = JSVAL_TO_OBJECT( *val ) ) &&
           JS_IsArrayObject( _context, jsObj ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   string _sptSPArguments::getUserObjClassName( UINT32 pos ) const
   {
      jsval *val = NULL ;
      JSObject *jsObj = NULL ;

      if( _argc > pos &&
           NULL != ( val = _getValAtPos( pos ) ) &&
           JSVAL_IS_OBJECT( *val ) &&
           NULL != ( jsObj = JSVAL_TO_OBJECT( *val ) ) )
      {
         return sptGetObjFactory()->getClassName( _context, jsObj ) ;
      }
      return "" ;
   }

   string _sptSPArguments::getErrMsg() const
   {
      return _errMsg ;
   }

   BOOLEAN _sptSPArguments::hasErrMsg() const
   {
      return _errMsg.empty() ? FALSE : TRUE ;
   }

   #define NATIVE_VALUE_EQ( pData, type, value ) \
      do \
      { \
         switch( type ) \
         { \
            case SPT_NATIVE_CHAR : \
               *(CHAR*)pData = ( CHAR )( value ) ; \
               break ; \
            case SPT_NATIVE_INT16 : \
               *(INT16*)pData = ( INT16 )( value ) ; \
               break ; \
            case SPT_NATIVE_INT32 : \
               *(INT32*)pData = ( INT32 )( value ) ; \
               break ; \
            case SPT_NATIVE_INT64 : \
               *(INT64*)pData = ( INT64 )( value ) ; \
               break ; \
            case SPT_NATIVE_FLOAT32 : \
               *(FLOAT32*)pData = ( FLOAT32 )( value ) ; \
               break ; \
            case SPT_NATIVE_FLOAT64 : \
               *(FLOAT64*)pData = ( FLOAT64 )( value ) ; \
               break ; \
            default : \
               _errMsg = "unexpect type of the parameter" ; \
               rc = SDB_INVALIDARG ; \
               goto error ; \
         } \
      } while ( 0 )


   INT32 _sptSPArguments::getNative( UINT32 pos, void *value,
                                     SPT_NATIVE_TYPE type ) const
   {
      INT32 rc = SDB_OK ;
      jsval *val = NULL ;

      _errMsg.clear() ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         _errMsg = "Failed to get val at pos" ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( JSVAL_IS_INT( *val ) )
      {
         NATIVE_VALUE_EQ( value, type, JSVAL_TO_INT( *val ) ) ;
      }
      else if ( JSVAL_IS_BOOLEAN( *val ) )
      {
         NATIVE_VALUE_EQ( value, type, JSVAL_TO_BOOLEAN( *val ) ) ;
      }
      else if ( JSVAL_IS_DOUBLE( *val ) )
      {
         NATIVE_VALUE_EQ( value, type, JSVAL_TO_DOUBLE( *val ) ) ;
      }
      else
      {
         _errMsg = "Parameter is not a native value" ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

