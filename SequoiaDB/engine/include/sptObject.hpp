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

   Source File Name = sptObject.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_OBJECT_HPP
#define SPT_OBJECT_HPP

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include <string>
#include "sptObjDesc.hpp"
#include <boost/shared_ptr.hpp>

namespace engine
{
   #define SPT_CVT_FLAGS_FROM_BOOL     0x1
   #define SPT_CVT_FLAGS_FROM_INT      0x2
   #define SPT_CVT_FLAGS_FROM_DOUBLE   0x4
   #define SPT_CVT_FLAGS_FROM_STRING   0x8
   #define SPT_CVT_FLAGS_FROM_OBJECT   0x10

   #define SPT_CVT_FLAGS_FROM_NUMBER \
      ( SPT_CVT_FLAGS_FROM_INT | SPT_CVT_FLAGS_FROM_DOUBLE )

   enum SPT_JS_TYPE
   {
      SPT_JS_TYPE_BOOLEAN = 0 ,
      SPT_JS_TYPE_INT,
      SPT_JS_TYPE_DOUBLE,
      SPT_JS_TYPE_STRING,
      SPT_JS_TYPE_OBJECT
   } ;

   class sptObject ;
   typedef boost::shared_ptr< sptObject >       sptObjectPtr ;

   /*
      sptObject define
   */
   class sptObject: public SDBObject
   {
   public:
      virtual ~sptObject()
      {
      }

      virtual INT32 getObjectField( const std::string &fieldName,
                                    sptObjectPtr &objPtr ) const = 0 ;

      virtual INT32 getBoolField( const std::string &fieldName, BOOLEAN &rval,
                                  INT32 mask = SPT_CVT_FLAGS_FROM_BOOL )
                                  const = 0 ;

      virtual INT32 getIntField( const std::string &fieldName, INT32 &rval,
                                 INT32 mask = SPT_CVT_FLAGS_FROM_INT )
                                 const = 0 ;

      virtual INT32 getDoubleField( const std::string &fieldName, FLOAT64 &rval,
                                    INT32 mask = SPT_CVT_FLAGS_FROM_DOUBLE )
                                    const = 0 ;

      virtual INT32 getStringField( const std::string &fieldName, string &rval,
                                    INT32 mask = SPT_CVT_FLAGS_FROM_STRING )
                                    const = 0 ;

      virtual INT32 getUserObj( const sptObjDesc &objDesc,
                                const void** value ) const = 0 ;

      virtual INT32 toBSON( bson::BSONObj &rval, BOOLEAN strict = TRUE ) const = 0 ;

      virtual INT32 toString( std::string &rval ) const = 0 ;

      virtual INT32 getFieldType( const std::string &fieldName,
                                  SPT_JS_TYPE &type ) const = 0 ;

      virtual BOOLEAN isFieldExist( const std::string &fieldName ) const = 0 ;

      virtual INT32 getFieldNumber( UINT32 &number ) const = 0 ;

      virtual INT32 getDesc( const sptObjDesc **pDesc ) const = 0 ;
   } ;

}

#endif //SPT_OBJECT_HPP

