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

   Source File Name = sptConvertor2.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Script component. This file contains structures for javascript
   engine wrapper

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/13/2013  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPTCONVERTOR2_HPP_
#define SPTCONVERTOR2_HPP_

#include "core.hpp"
#include "jsapi.h"
#include "../bson/bson.hpp"
#include <string>

class sptConvertor2
{
public:
   sptConvertor2( JSContext *cx )
   :_cx( cx )
   {
   }

   ~sptConvertor2()
   {
      _cx = NULL ;
   }

public:
   INT32 toBson( JSObject *obj , bson::BSONObj &bsobj ) ;

   static INT32 toString( JSContext *cx,
                          const jsval &val,
                          std::string &str ) ;

private:
   JSContext *_cx ;
} ;

#endif // SPTCONVERTOR2_HPP_

