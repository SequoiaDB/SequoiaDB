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

   Source File Name = rtnSQLSum.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSQLSUM_HPP_
#define RTNSQLSUM_HPP_

#include "rtnSQLFunc.hpp"
using namespace bson ;

namespace engine
{
   class _rtnSQLSum : public _rtnSQLFunc
   {
   public:
      _rtnSQLSum( const CHAR *pName ) ;
      virtual ~_rtnSQLSum() ;

   public:
      virtual INT32 result( BSONObjBuilder &builder ) ;
      virtual BOOLEAN isStat() const { return TRUE ; }

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param ) ;

   private:
      FLOAT64 _sum ;
      bsonDecimal _decSum ;
      BOOLEAN _effective ;
   } ;

   typedef class _rtnSQLSum rtnSQLSum ;
}

#endif

