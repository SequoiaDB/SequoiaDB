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

   Source File Name = mongReplyHelper.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_REPLY_HELPER_HPP_
#define _SDB_MONGO_REPLY_HELPER_HPP_

#include "msgBuffer.hpp"
#include "sdbInterface.hpp"
#include "rtnContextBuff.hpp"

namespace fap
{
   namespace mongo
   {
      void buildIsMasterReplyMsg( engine::IResource *resource,
                             engine::rtnContextBuf &buff ) ;

      void buildGetNonceReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetLastErrorReplyMsg( const bson::BSONObj &err,
                                 engine::rtnContextBuf &buff ) ;

      void buildNotSupportReplyMsg( engine::rtnContextBuf &buff,
                                    const char *cmdName ) ;

      void buildPingReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetMoreMsg( msgBuffer &out ) ;
   }
}
#endif
