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

   Source File Name = omSdbConnector.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/28/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_SDB_CONNECTOR_HPP_
#define OM_SDB_CONNECTOR_HPP_

#include "oss.hpp"
#include "msg.h"
#include "msgDef.h"
#include "ossSocket.hpp"
#include "../bson/bson.h"
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   /*
      _omSdbConnector define
   */
   class _omSdbConnector : public SDBObject
   {
      public:
         _omSdbConnector() ;
         ~_omSdbConnector() ;
         INT32    init( const string &hostName, UINT32 port, 
                        const string &user, const string &passwd,
                        INT32 preferedInstance ) ;

         INT32    sendMessage( const MsgHeader *msg ) ;
         INT32    recvMessage( MsgHeader **msg ) ;

         INT32    close() ;

      private:
         INT32    _connect( const CHAR *host, UINT32 port ) ;
         INT32    _sendRequest( const CHAR *request, INT32 reqSize ) ;
         INT32    _recvResponse( CHAR *response, INT32 resSize ) ;
         INT32    _requestSysInfo() ;
         INT32    _authority( const string &user, const string &passwd ) ;
         INT32    _negotiation( const string &user, const string &passwd ) ;
         INT32    _setAttr( INT32 preferedInstance ) ;

      private:
         string      _hostName ;
         UINT32      _port ;
         _ossSocket  *_pSocket ;
         BOOLEAN     _init ;
   } ;
   
   typedef _omSdbConnector omSdbConnector ;

}

#endif //OM_SDB_CONNECTOR_HPP_

