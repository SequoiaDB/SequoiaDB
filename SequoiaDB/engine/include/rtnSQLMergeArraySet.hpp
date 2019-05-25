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

   Source File Name = rtnSQLMergeArraySet.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/18/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNSQLMERGEARRAYSET_HPP__
#define RTNSQLMERGEARRAYSET_HPP__

#include "rtnSQLFunc.hpp"
#include "../bson/bsonobj.h"
#include <set>
#include <string>

namespace engine
{

   class rtnSQLMergeArraySet : public _rtnSQLFunc
   {
   typedef std::set< bson::BSONElement >     FIELD_SET;
   typedef std::vector< bson::BSONObj >      OBJ_VEC;
   public:
      rtnSQLMergeArraySet( const CHAR *pName );

      virtual ~rtnSQLMergeArraySet();

      virtual INT32 result( bson::BSONObjBuilder &builder );

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param );

   private:
      bson::BSONArrayBuilder     *_pArrBuilder;
      FIELD_SET                  _fieldSet;
      OBJ_VEC                    _objVec;
   };
}

#endif


