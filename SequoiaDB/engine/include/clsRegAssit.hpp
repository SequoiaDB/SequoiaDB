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

   Source File Name = clsRegAssit.hpp

   Descriptive Name = node register assistant

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          10/10/2017  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSREGASSIT_HPP_
#define CLSREGASSIT_HPP_

#include "../bson/bsonobj.h"
#include "msg.hpp"
#include "ossSocket.hpp"

using namespace bson ;

namespace engine
{
   class _clsRegAssit : public SDBObject
   {
      public :
         _clsRegAssit () ;
         ~_clsRegAssit () ;
         INT32 buildRequestObj ( BSONObj &request ) ;
         INT32 extractResponseMsg ( MsgHeader *pMsg ) ;
         UINT32 getGroupID () ;
         UINT16 getNodeID () ;
         const CHAR* getHostname () ;

      private :
         UINT32 _groupID ;
         UINT16 _nodeID ;
         CHAR _hostName[ OSS_MAX_HOSTNAME + 1 ];
   } ;
   typedef _clsRegAssit clsRegAssit ;
}

#endif

