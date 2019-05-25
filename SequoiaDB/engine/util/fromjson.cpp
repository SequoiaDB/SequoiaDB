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

   Source File Name = fromjson.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossUtil.hpp"
#include "fromjson.hpp"
#include "json2rawbson.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
namespace bson
{
   INT32 fromjson ( const string &str, BSONObj &out, BOOLEAN isBatch )
   {
      return fromjson ( str.c_str(), out, isBatch ) ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB_FROMJSON, "fromjson" )
   INT32 fromjson ( const CHAR *pStr, BSONObj &out, BOOLEAN isBatch )
   {
      INT32 rc         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_FROMJSON );
      CHAR *p          = NULL ;

      if ( !pStr )
      {
         SDB_ASSERT ( FALSE, "empty str from json str" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( *pStr == '\0' )
      {
         BSONObj empty ;
         out = empty ;
         goto done ;
      }
      p = json2rawbson ( pStr, isBatch ) ;
      if ( p )
      {
         BSONObj::Holder *h = (BSONObj::Holder*)p ;
         try
         {
            BSONObj ret ( h ) ;
            out = ret ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                     e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_FROMJSON, rc );
      return rc ;
   error :
      goto done ;
   }
}

