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
                                             cb, contextID );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to execute aggregation operation(rc=%d)",
                  rc );

   done:
      return rc;
   error:
      goto done;
   }
}
