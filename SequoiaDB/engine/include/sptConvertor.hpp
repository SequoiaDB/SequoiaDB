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

   Source File Name = sptConvertor.hpp

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
#include "sptObjDesc.hpp"
#include "sptSPVal.hpp"

namespace engine
{
   /*
      sptConvertor define
   */
   class sptConvertor
   {
   public:
      sptConvertor( JSContext *cx, BOOLEAN strict = TRUE )
      :_cx( cx ),_hasSetErrMsg( FALSE ), _strict( strict )
      {
      }

      ~sptConvertor()
      {
         _cx = NULL ;
      }

   public:
      static INT32 toString( JSContext *cx,
                             const jsval &val,
                             std::string &str ) ;

   public:
      INT32 toBson( JSObject *obj,
                    bson::BSONObj &bsobj,
                    BOOLEAN *pIsArray = NULL ) ;

      INT32 toBson( const sptSPVal *pVal,
                    bson::BSONObj &obj,
                    BOOLEAN *pIsArray = NULL ) ;

      INT32 appendToBson( const std::string &key,
                          const sptSPVal &val,
                          bson::BSONObjBuilder &builder ) ;

      INT32 toObjArray( JSObject *obj, vector< bson::BSONObj > &bsArray ) ;
      INT32 toStrArray( JSObject *obj, vector< string > &bsArray ) ;

      const string& getErrMsg() const ;

   private:
      INT32 _traverse( JSObject *obj,
                       bson::BSONObjBuilder &builder ) ;

      INT32 _appendToBson( const std::string &name,
                           const sptSPVal &val,
                           bson::BSONObjBuilder &builder ) ;

      void _setErrMsg( const string &msg, BOOLEAN isReplace ) ;

   private:
      JSContext   *_cx ;
      BOOLEAN     _hasSetErrMsg ;
      string      _errMsg ;
      BOOLEAN     _strict ;
   } ;
}
#endif // SPTCONVERTOR2_HPP_

