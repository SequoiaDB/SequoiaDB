/******************************************************************************


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

   Source File Name = pd.hpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef NETMSGHANDLER_HPP_
#define NETMSGHANDLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"

namespace engine
{
   /*
      _netMsgHandler define
   */
   class _netMsgHandler : public SDBObject
   {
      public:
        _netMsgHandler(){}
        virtual ~_netMsgHandler(){}
      public:
        virtual INT32   handleMsg( const NET_HANDLE &handle,
                                   const _MsgHeader *header,
                                   const CHAR *msg ) = 0 ;

        virtual void    handleClose( const NET_HANDLE &handle,
                                     _MsgRouteID id )
        {
        }

        virtual void    handleConnect( const NET_HANDLE &handle,
                                       _MsgRouteID id,
                                       BOOLEAN isPositive )
        {
        }

        virtual void    onStop() {}

   } ;

   typedef _netMsgHandler INetMsgHandler ;

}

#endif // NETMSGHANDLER_HPP_

