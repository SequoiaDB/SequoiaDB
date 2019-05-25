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

   Source File Name = rtnSQLLast.hpp

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
#ifndef RTNSQLLAST_HPP__
#define RTNSQLLAST_HPP__

#include "rtnSQLFunc.hpp"

namespace engine
{

   class rtnSQLLast : public _rtnSQLFunc
   {
   public:
      rtnSQLLast( const CHAR *pName );

      virtual ~rtnSQLLast();

      virtual INT32 result( bson::BSONObjBuilder &builder );

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param );

   private:
      bson::BSONObj              _obj ;
      bson::BSONElement          _ele ;
   };
}

#endif
