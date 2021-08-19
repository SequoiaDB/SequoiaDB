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

   Source File Name = fapMongoCursor.hpp

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
#ifndef _SDB_MONGO_CURSOR_HPP_
#define _SDB_MONGO_CURSOR_HPP_

#include "ossUtil.hpp"
#include "utilConcurrentMap.hpp"
#include "mongodef.hpp"

using namespace std ;

namespace fap
{

struct mongoCursorInfo
{
   INT64 cursorID ;
   UINT64 EDUID ;
   BOOLEAN needAuth ; // TODO
} ;

class _mongoCursorMgr : public SDBObject
{
   typedef engine::utilConcurrentMap<INT64, mongoCursorInfo, 32> CURSOR_MAP ;

public:
   _mongoCursorMgr() {}
   virtual ~_mongoCursorMgr() {}

   BOOLEAN find( INT64 cursorID, mongoCursorInfo& info ) ;
   INT32 insert( const mongoCursorInfo& info ) ;
   void remove( INT64 cursorID ) ;

private:
   CURSOR_MAP _cursorMap ;
} ;
typedef _mongoCursorMgr mongoCursorMgr ;

_mongoCursorMgr *getMongoCursorMgr() ;

}
#endif
