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

   Source File Name = fapMongoCursor.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          2020/04/30  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoCursor.hpp"

namespace fap
{
   BOOLEAN _mongoCursorMgr::find( INT64 cursorID, mongoCursorInfo& info )
   {
      BOOLEAN foundOut = FALSE ;

      if ( cursorID != MONGO_INVALID_CURSORID )
      {
         CURSOR_MAP::Bucket& bucket = _cursorMap.getBucket( cursorID ) ;
         BUCKET_SLOCK( bucket ) ;
         CURSOR_MAP::map_const_iterator itr = bucket.find( cursorID ) ;
         if ( itr != bucket.end() )
         {
            info = (*itr).second ;
            foundOut = TRUE ;
         }
      }

      return foundOut ;
   }

   INT32 _mongoCursorMgr::insert( const mongoCursorInfo& info )
   {
      INT32 rc = SDB_OK ;

      if ( info.cursorID != MONGO_INVALID_CURSORID )
      {
         CURSOR_MAP::Bucket& bucket = _cursorMap.getBucket( info.cursorID ) ;
         BUCKET_XLOCK( bucket ) ;

         try
         {
            bucket.insert( CURSOR_MAP::value_type( info.cursorID, info ) ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "An exception occurred when inserting cursor "
                    "info: %s, rc: %d", e.what(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mongoCursorMgr::remove( INT64 cursorID )
   {
      if ( cursorID != MONGO_INVALID_CURSORID )
      {
         CURSOR_MAP::Bucket& bucket = _cursorMap.getBucket( cursorID ) ;
         BUCKET_XLOCK( bucket ) ;
         bucket.erase( cursorID ) ;
      }
   }

   _mongoCursorMgr* getMongoCursorMgr()
   {
      static _mongoCursorMgr cursorMgr ;
      return &cursorMgr ;
   }

}
