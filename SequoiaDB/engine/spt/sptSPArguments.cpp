/******************************************************************************


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
#include "sptConvertor2.hpp"
#include "sptObjDesc.hpp"

using namespace bson ;

namespace engine
{
   _sptSPArguments::_sptSPArguments( JSContext *context, uintN argc, jsval *vp )
   :_context(context),
    _argc(argc),
    _vp(vp)
   {
      SDB_ASSERT( NULL != _context && NULL != _vp, "can not be NULL" ) ;
   }

   _sptSPArguments::~_sptSPArguments()
   {
      _context = NULL ;
      _vp = NULL ;
   }

   INT32 _sptSPArguments::getString( UINT32 pos,
                                     std::string &value,
                                     BOOLEAN strict ) const
   {
      INT32 rc = SDB_OK ;
      JSString *jsStr = NULL ;
      CHAR *str = NULL ;
      jsval *val = NULL ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         PD_LOG( PDERROR, "failed to get val at pos" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( strict )
      {
         if ( !JSVAL_IS_STRING( *val ) )
         {
            PD_LOG( PDERROR, "jsval is not a string." ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         jsStr = JSVAL_TO_STRING( *val ) ;
      }
      else
      {
         jsStr = JS_ValueToString( _context, *val ) ;
      }
      if ( NULL == jsStr )
      {
         PD_LOG( PDERROR, "failed to convert jsval to jsstr" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      str = JS_EncodeString ( _context , jsStr ) ;
      if ( NULL == str )
      {
         PD_LOG( PDERROR, "failed to convert a js str to a normal str" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      value.assign( str ) ;

   done:
      SAFE_JS_FREE( _context, str ) ;
      return rc ;
   error:
      goto done ;
   }


   jsval *_sptSPArguments::_getValAtPos( UINT32 pos ) const
   {
      return JS_ARGV( _context, _vp ) + pos ;
   }

   INT32 _sptSPArguments::getBsonobj( UINT32 pos,
                                      bson::BSONObj &value ) const
   {
      INT32 rc = SDB_OK ;
      JSObject *jsObj = NULL ;
      jsval *val = NULL ;
      sptConvertor2 convertor( _context ) ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         PD_LOG( PDERROR, "failed to get val at pos" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JSVAL_IS_OBJECT( *val ) )
      {
         PD_LOG( PDERROR, "jsval is not a object" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      jsObj = JSVAL_TO_OBJECT( *val ) ;
      if ( NULL == jsObj )
      {
         PD_LOG( PDERROR, "failed to convert jsval to object" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = convertor.toBson( jsObj, value ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to convert jsobj to bsonobj:%d", rc ) ;
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

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         PD_LOG( PDERROR, "failed to get val at pos" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !JSVAL_IS_OBJECT( *val ) )
      {
         PD_LOG( PDERROR, "jsval is not a object" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      jsObj = JSVAL_TO_OBJECT( *val ) ;
      if ( NULL == jsObj )
      {
         PD_LOG( PDERROR, "failed to convert jsval to object" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if( string( objDesc.getJSClassName() ) !=
            sptGetObjFactory()->getClassName( _context, jsObj ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "jsObj className must be: %s", objDesc.getJSClassName() ) ;
         goto error ;
      }

      *value = JS_GetPrivate( _context, jsObj ) ;
      if( *value == NULL )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "failed to convert jsobj to user obj:%d", rc ) ;
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
           JSVAL_TO_DOUBLE( *val ) )
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
               PD_LOG( PDERROR, "type[%d] is error", type ) ; \
               rc = SDB_INVALIDARG ; \
               goto error ; \
         } \
      } while ( 0 )


   INT32 _sptSPArguments::getNative( UINT32 pos, void *value,
                                     SPT_NATIVE_TYPE type ) const
   {
      INT32 rc = SDB_OK ;
      jsval *val = NULL ;

      if ( _argc <= pos )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      val = _getValAtPos( pos ) ;
      if ( NULL == val )
      {
         PD_LOG( PDERROR, "failed to get val at pos" ) ;
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
         PD_LOG( PDERROR, "jsval is not a native value" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

