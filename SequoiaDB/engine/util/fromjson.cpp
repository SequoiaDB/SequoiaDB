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

      // defensive code
      if ( !pStr )
      {
         // we should never hit here
         SDB_ASSERT ( FALSE, "empty str from json str" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( *pStr == '\0' )
      {
         // special case handling for empty string, we will return empty bson
         BSONObj empty ;
         out = empty ;
         // we may not need to getOwned () because BSON is using smart pointer
         // out.getOwned () ;
         goto done ;
      }
      p = json2rawbson ( pStr, isBatch ) ;
      if ( p )
      {
         BSONObj::Holder *ph = (BSONObj::Holder*)p ;
         try
         {
            bson_intrusive_ptr<BSONObj::Holder,HeapAllocator> h( ph ) ;
            BSONObj ret ( h ) ;
            out = ret.getOwned() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                     e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // we may not need to getOwned () because BSON is using smart pointer
         // out.getOwned () ;
      }
      else
      {
         // if json2rawbson returns NULL, that means we cannot parse json
         rc = SDB_INVALIDARG ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_FROMJSON, rc );
      return rc ;
   error :
      goto done ;
   }
}

