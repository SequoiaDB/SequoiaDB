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

   Source File Name = catEventHandler.hpp

   Descriptive Name = Interfaces for Catalog event handlers

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/13/2016  hgm Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_EVENT_HANDLER_HPP_
#define CAT_EVENT_HANDLER_HPP_

#include "msg.hpp"
#include "netMsgHandler.hpp"

namespace engine
{

   /*
      _catEventHandler define
   */
   class _catEventHandler
   {
   public :
      _catEventHandler () {}

      virtual ~_catEventHandler () {}

      virtual const CHAR *getHandlerName () = 0 ;

      virtual INT32 onBeginCommand ( MsgHeader *pReqMsg ) = 0 ;

      virtual INT32 onEndCommand ( MsgHeader *pReqMsg, INT32 result ) = 0 ;

      virtual INT32 onSendReply ( MsgOpReply *pReply, INT32 result ) = 0 ;
   } ;

}

#endif // CAT_EVENT_HANDLER_HPP_
