/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

using bson::BSONObj ;

namespace seadapter
{
   enum ES_DATA_TYPE
   {
      ES_TEXT,
      ES_KEYWORD,
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
}

#endif /* UTIL_ESUTIL_HPP__ */

