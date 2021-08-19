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

   Source File Name = sptSPVal.hpp

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

#ifndef SPT_SP_VAL_HPP__
#define SPT_SP_VAL_HPP__

#include "core.hpp"
#include "jsapi.h"
#include <string>
#include "sptObjDesc.hpp"

namespace engine
{

   /*
      _sptSPVal define
   */
   class _sptSPVal : public SDBObject
   {
      public:
         _sptSPVal( JSContext *pContext = NULL,
                    const jsval &val = JSVAL_VOID ) ;
         virtual ~_sptSPVal() ;

         const jsval*   valuePtr() const ;

         void           reset( JSContext *pContext = NULL,
                               const jsval &val = JSVAL_VOID ) ;

      public:

         BOOLEAN     isNull() const ;
         BOOLEAN     isVoid() const ;
         BOOLEAN     isInt() const ;
         BOOLEAN     isDouble() const ;
         BOOLEAN     isNumber() const ;
         BOOLEAN     isString() const ;
         BOOLEAN     isBoolean() const ;

         BOOLEAN     isObject() const ;
         BOOLEAN     isFunctionObj() const ;
         BOOLEAN     isArrayObj() const ;

         /*
            Special Object
         */
         BOOLEAN     isSPTObject( BOOLEAN *pIsSpecial = NULL,
                                  string *pClassName = NULL,
                                  const sptObjDesc **ppDesc = NULL ) const ;

         INT32       toInt( INT32 &value ) const ;
         INT32       toDouble( FLOAT64 &value ) const ;
         INT32       toString( string &value ) const ;
         INT32       toBoolean( BOOLEAN &value ) const ;

      private:
         JSContext      *_pContext ;
         jsval          _value ;

   } ;
   typedef _sptSPVal sptSPVal ;

}
#endif // SPT_SP_VAL_HPP__

