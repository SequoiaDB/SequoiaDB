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

   Source File Name = coordUtil.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_UTIL_HPP__
#define COORD_UTIL_HPP__

#include "coordDef.hpp"
#include "coordCommon.hpp"
#include "coordResource.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   void  coordBuildFailedNodeReply( coordResource *pResource,
                                    ROUTE_RC_MAP &failedNodes,
                                    BSONObjBuilder &builder ) ;

   BSONObj coordBuildErrorObj( coordResource *pResource,
                               INT32 &flag,
                               _pmdEDUCB *cb,
                               ROUTE_RC_MAP *pFailedNodes ) ;

   INT32 coordGetGroupsFromObj( const BSONObj &obj,
                                CoordGroupList &groupLst ) ;

   INT32 coordParseGroupList( coordResource *pResource,
                              _pmdEDUCB *cb,
                              const BSONObj &obj,
                              CoordGroupList &groupList,
                              BSONObj *pNewObj = NULL,
                              BOOLEAN strictCheck = FALSE ) ;

   INT32 coordParseGroupList( coordResource *pResource,
                              _pmdEDUCB *cb,
                              MsgOpQuery *pMsg,
                              FILTER_BSON_ID filterObjID,
                              CoordGroupList &groupList,
                              BOOLEAN strictCheck = FALSE ) ;

   INT32 coordGroupList2GroupPtr( coordResource *pResource,
                                  _pmdEDUCB *cb,
                                  CoordGroupList &groupList,
                                  GROUP_VEC & groupPtrs ) ;

   INT32 coordGroupList2GroupPtr( coordResource *pResource,
                                  _pmdEDUCB *cb,
                                  CoordGroupList &groupList,
                                  CoordGroupMap &groupMap,
                                  BOOLEAN reNew ) ;

   void  coordGroupPtr2GroupList( GROUP_VEC &groupPtrs,
                                  CoordGroupList &groupList ) ;

   INT32 coordGetGroupNodes( coordResource *pResource,
                             _pmdEDUCB *cb,
                             const BSONObj &filterObj,
                             NODE_SEL_STY emptyFilterSel,
                             CoordGroupList &groupList,
                             SET_ROUTEID &nodes,
                             BSONObj *pNewObj = NULL,
                             BOOLEAN strictCheck = FALSE ) ;

}

#endif // COORD_UTIL_HPP__

