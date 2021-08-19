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

   Source File Name = rtnAggregate.cpp

   Descriptive Name = Runtime Aggregate

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   aggregation on data node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "rtn.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"

using namespace bson;

namespace engine
{
   INT32 rtnAggregate( const CHAR *pCollectionName, BSONObj &objs,
                       INT32 objNum, SINT32 flags, pmdEDUCB *cb,
                       SDB_DMSCB *dmsCB, SINT64 &contextID )
   {
      INT32 rc = SDB_OK;
      rc = pmdGetKRCB()->getAggrCB()->build( objs, objNum, pCollectionName,
                                             BSONObj(), cb, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to execute aggregation operation, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
