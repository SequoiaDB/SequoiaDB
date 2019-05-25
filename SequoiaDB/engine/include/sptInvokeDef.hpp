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

   Source File Name = sptInvokeDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_INVOKEDEF_HPP_
#define SPT_INVOKEDEF_HPP_

#include "jsapi.h"

namespace engine
{
   namespace JS_INVOKER
   {
      typedef JSBool (*MEMBER_FUNC)(JSContext *cx , uintN argc , jsval *vp) ;

      typedef void (*DESTRUCT_FUNC)(JSContext *cx , JSObject *obj) ;

      typedef JSBool (*RESLOVE_FUNC)( JSContext *cx , JSObject *obj , jsid id ,
                                      uintN flags , JSObject ** objp) ;
   }
}

#endif // SPT_INVOKEDEF_HPP_

