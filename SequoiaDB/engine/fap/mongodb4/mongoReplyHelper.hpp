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

   Source File Name = mongReplyHelper.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_REPLY_HELPER_HPP_
#define _SDB_MONGO_REPLY_HELPER_HPP_

#include "msgBuffer.hpp"
#include "sdbInterface.hpp"
#include "rtnContextBuff.hpp"
#include "mongodef.hpp"

namespace fap
{
   enum FAP_WIRE_VERSION {
       FAP_MIN_WIRE_VERSION = 0,
       FAP_MAX_WIRE_VERSION = 8, // MongoDB(4.2+)
   };

   namespace mongo
   {
      void buildIsMasterReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetLastErrorReplyMsg( const bson::BSONObj &err,
                                      engine::rtnContextBuf &buff ) ;

      void buildNotSupportReplyMsg( engine::rtnContextBuf &buff,
                                    const char *cmdName ) ;

      void buildPingReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetMoreMsg( msgBuffer &out ) ;

      void buildWhatsmyuriReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetLogReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildBuildinfoReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildAuthStep3ReplyMsg( engine::rtnContextBuf &buff ) ;

   }
}
#endif
