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

   Source File Name = sptSPObject.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptObject.hpp"
#include "jsapi.h"

namespace engine
{
   /*
      sptSPObject define
   */
   class sptSPObject: public sptObject
   {
   public:
      sptSPObject( JSContext *cx, JSObject *obj ) ;

      virtual ~sptSPObject() ;

      INT32 getObjectField( const std::string &fieldName,
                            sptObjectPtr &objPtr ) const ;

      INT32 getBoolField( const std::string &fieldName, BOOLEAN &rval,
                          INT32 mask = SPT_CVT_FLAGS_FROM_BOOL ) const ;

      INT32 getIntField( const std::string &fieldName, INT32 &rval,
                         INT32 mask = SPT_CVT_FLAGS_FROM_INT ) const ;

      INT32 getDoubleField( const std::string &fieldName, FLOAT64 &rval,
                            INT32 mask = SPT_CVT_FLAGS_FROM_DOUBLE ) const ;

      INT32 getStringField( const std::string &fieldName, string &rval,
                            INT32 mask = SPT_CVT_FLAGS_FROM_STRING ) const ;

      INT32 getUserObj( const sptObjDesc &objDesc, const void** value ) const ;

      INT32 toBSON( bson::BSONObj &rval, BOOLEAN strict = TRUE ) const ;

      INT32 toString( std::string &rval ) const ;

      INT32 getFieldType( const std::string &fieldName,
                          SPT_JS_TYPE &type ) const ;

      BOOLEAN isFieldExist( const std::string &fieldName ) const ;

      INT32 getFieldNumber( UINT32 &number ) const ;

      INT32 getDesc( const sptObjDesc **pDesc ) const ;

   protected:
      INT32 _getTypeOfVal( jsval val, SPT_JS_TYPE &type ) const ;

      INT32 _getObjectDesc( JSObject* obj, BOOLEAN &isSpecialObj,
                            const sptObjDesc **pDesc ) const ;
   private:
      JSContext *_cx ;
      JSObject *_obj ;
   } ;
}