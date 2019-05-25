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

   Source File Name = sptConvertor2.cpp

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

#include "ossUtil.hpp"
#include "sptConvertor2.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "sptConvertorHelper.hpp"

using namespace bson ;

INT32 sptConvertor2::toBson( JSObject *obj , bson::BSONObj &bsobj )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT( NULL != _cx, "can not be NULL" ) ;

   CHAR *pData = NULL ;

   rc = JSObj2BsonRaw( _cx, obj, &pData ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      bsobj = BSONObj( pData ).getOwned() ;
   }
   catch( std::exception & )
   {
      rc = SDB_SYS ;
      goto error ;
   }

done:
   if ( pData )
   {
      free( pData ) ;
      pData = NULL ;
   }
   return rc ;
error:
   goto done ;
}

INT32 sptConvertor2::toString( JSContext *cx,
                               const jsval &val,
                               std::string &str )
{
   SDB_ASSERT( NULL != cx, "impossible" ) ;
   return JSVal2String( cx, val, str ) ;
}

