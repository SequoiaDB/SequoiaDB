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

   Source File Name = sptSPVal.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/27/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptSPVal.hpp"

namespace engine
{

   /*
      _sptSPVal implement
   */
   _sptSPVal::_sptSPVal( JSContext *pContext, const jsval &val )
   {
      reset( pContext, val ) ;
   }

   _sptSPVal::~_sptSPVal()
   {
   }

   void _sptSPVal::reset( JSContext * pContext,
                          const jsval &val )
   {
      _pContext = pContext ;
      _value = val ;
   }

   const jsval* _sptSPVal::valuePtr() const
   {
      return &_value ;
   }

   BOOLEAN _sptSPVal::isNull() const
   {
      return JSVAL_IS_NULL( _value ) ;
   }

   BOOLEAN _sptSPVal::isVoid() const
   {
      return JSVAL_IS_VOID( _value ) ;
   }

   BOOLEAN _sptSPVal::isInt() const
   {
      return JSVAL_IS_INT( _value ) ;
   }

   BOOLEAN _sptSPVal::isDouble() const
   {
      return JSVAL_IS_DOUBLE( _value ) ;
   }

   BOOLEAN _sptSPVal::isNumber() const
   {
      return JSVAL_IS_NUMBER( _value ) ;
   }

   BOOLEAN _sptSPVal::isString() const
   {
      return JSVAL_IS_STRING( _value ) ;
   }

   BOOLEAN _sptSPVal::isBoolean() const
   {
      return JSVAL_IS_BOOLEAN( _value ) ;
   }

   BOOLEAN _sptSPVal::isFunctionObj() const
   {
      if ( isObject() )
      {
         JSObject *jsObj = JSVAL_TO_OBJECT( _value ) ;

         if ( jsObj && JS_ObjectIsFunction( _pContext, jsObj ) )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _sptSPVal::isArrayObj() const
   {
      if ( isObject() )
      {
         JSObject *jsObj = JSVAL_TO_OBJECT( _value ) ;

         if ( jsObj && JS_IsArrayObject( _pContext, jsObj ) )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _sptSPVal::isObject() const
   {
      return JSVAL_IS_OBJECT( _value ) ;
   }

   BOOLEAN _sptSPVal::isSPTObject( BOOLEAN *pIsSpecial,
                                   string *pClassName,
                                   const sptObjDesc **ppDesc ) const
   {
      const sptObjDesc *desc = NULL ;
      BOOLEAN isSpecialObj = FALSE ;
      sptObjFactory *pFactory = sptGetObjFactory() ;

      if ( isObject() )
      {
         JSObject *jsObj = JSVAL_TO_OBJECT( _value ) ;

         if ( jsObj && SDB_OK == pFactory->getObjDesc( _pContext, jsObj,
                                                       isSpecialObj,
                                                       &desc ) &&
              desc )
         {
            if ( isSpecialObj && !pIsSpecial )
            {
               return FALSE ;
            }

            if ( pIsSpecial )
            {
               *pIsSpecial = isSpecialObj ;
            }
            if ( pClassName )
            {
               *pClassName = desc->getJSClassName() ;
            }
            if ( ppDesc )
            {
               *ppDesc = desc ;
            }
            return TRUE ;
         }
      }

      return FALSE ;
   }

   INT32 _sptSPVal::toInt( INT32 &value ) const
   {
      if ( !JS_ValueToInt32( _pContext, _value, &value ) )
      {
         return SDB_SYS ;
      }
      return SDB_OK ;
   }

   INT32 _sptSPVal::toDouble( FLOAT64 &value ) const
   {
      jsdouble jsd = 0 ;
      if ( !JS_ValueToNumber( _pContext, _value, &jsd ))
      {
         return SDB_SYS ;
      }
      value = jsd ;
      return SDB_OK ;
   }

   INT32 _sptSPVal::toString( string &value ) const
   {
      JSString *jsStr = NULL ;
      CHAR *pStr = NULL ;

      jsStr = JS_ValueToString ( _pContext , _value ) ;
      if ( !jsStr )
      {
         return SDB_SYS ;
      }

      pStr = JS_EncodeString ( _pContext , jsStr ) ;
      if ( pStr )
      {
         value.assign( pStr ) ;
         /// free
         JS_free( _pContext, pStr ) ;
      }
      else
      {
         return SDB_SYS ;
      }

      return SDB_OK ;
   }

   INT32 _sptSPVal::toBoolean( BOOLEAN &value ) const
   {
      JSBool bp = JS_FALSE ;
      if ( !JS_ValueToBoolean( _pContext, _value, &bp ) )
      {
         return SDB_SYS ;
      }
      value = bp ;
      return SDB_OK ;
   }

}

