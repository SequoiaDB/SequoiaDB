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

   Source File Name = utilESUtil.hpp

   Descriptive Name = Elasticsearch utility.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/03/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ESUTIL_HPP__
#define UTIL_ESUTIL_HPP__

#include "core.hpp"
#include "../bson/bson.hpp"
#include <string>
#include <vector>

using namespace bson ;

namespace seadapter
{
   // Elasticsearch field datatypes.
   // Note: Any change here should also change getTypeStr.
   enum ES_DATA_TYPE
   {
      ES_TEXT,
      ES_KEYWORD,
      ES_MULTI_FIELDS,
      ES_DATE,
      ES_LONG,
      ES_DOUBLE,
      ES_BOOLEAN,
      ES_IP,

      ES_OBJECT,
      ES_NESTED,

      ES_GEO_POINT,
      ES_GEO_SHAPE,
      ES_COMPLETION
   } ;

   class _utilESMapProp
   {
      public:
         _utilESMapProp( const CHAR *name, ES_DATA_TYPE type ) ;
         ~_utilESMapProp() ;

         const std::string& getName() const { return _name ; }
         const ES_DATA_TYPE getType() const { return _type ; }

         // Reserved, maybe need to set more parameters in future.
         // INT32 setParams( const BSONObj &parameters ) ;

      private:
         std::string    _name ;
         ES_DATA_TYPE   _type ;
   } ;
   typedef _utilESMapProp utilESMapProp ;

   class _utilESMapping
   {
      public:
         _utilESMapping( const CHAR *index, const CHAR *type ) ;
         ~_utilESMapping() ;

         INT32 addProperty( const CHAR *name, ES_DATA_TYPE type,
                            BSONObj *parameters = NULL ) ;

         INT32 toObj( BSONObj &mapObj ) const ;

      private:
         std::string _index ;
         std::string _type ;
         vector<_utilESMapProp> _properties ;
   } ;
   typedef _utilESMapping utilESMapping ;

   void encodeID( const BSONElement &idEle, string &id ) ;
   INT32 decodeID( const CHAR *id, CHAR *raw, UINT32 &len, BSONType &type ) ;
}

#endif /* UTIL_ESUTIL_HPP__ */

