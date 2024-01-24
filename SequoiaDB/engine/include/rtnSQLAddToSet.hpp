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

   Source File Name = rtnSQLAddToSet.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/05/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNSQLADDTOSET_HPP__
#define RTNSQLADDTOSET_HPP__

#include "rtnSQLFunc.hpp"
#include "../bson/bsonobj.h"
#include "ossMemPool.hpp"
#include <string>

namespace engine
{

   class rtnSQLAddToSet : public _rtnSQLFunc
   {
   typedef ossPoolSet< bson::BSONElement >   FIELD_SET;
   typedef ossPoolVector< bson::BSONObj >    OBJ_VEC;
   public:
      rtnSQLAddToSet( const CHAR *pName );

      virtual ~rtnSQLAddToSet();

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

