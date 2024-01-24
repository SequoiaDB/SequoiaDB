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

   Source File Name = coordCommands.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMANDS_HPP__
#define COORD_COMMANDS_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDSetSessionAttr define
    */
   class _coordCMDSetSessionAttr : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSetSessionAttr() ;
         virtual ~_coordCMDSetSessionAttr() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDSetSessionAttr coordCMDSetSessionAttr ;

   /*
      _coordCMDGetSessionAttr define
    */
   class _coordCMDGetSessionAttr : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public :
         _coordCMDGetSessionAttr () ;
         virtual ~_coordCMDGetSessionAttr () ;

         virtual INT32 execute ( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;
   } ;

   typedef _coordCMDGetSessionAttr coordCMDGetSessionAttr ;

}

#endif // COORD_COMMANDS_HPP__
