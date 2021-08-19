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

   Source File Name = seAdptMsgHandler.hpp

   Descriptive Name = Search Engine Adapter Message Handler.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SEADPT_MSG_HANDLER_HPP__
#define SEADPT_MSG_HANDLER_HPP__

#include "core.hpp"
#include "pmdAsyncHandler.hpp"
#include "pmdAsyncSession.hpp"

using namespace engine ;

namespace seadapter
{
   class _indexMsgHandler : public _pmdAsyncMsgHandler
   {
      public:
         _indexMsgHandler( pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_indexMsgHandler() ;

         virtual void handleClose( const NET_HANDLE &handle,
                                   _MsgRouteID id ) ;
   } ;
   typedef _indexMsgHandler indexMsgHandler ;
}

#endif /* SEADPT_MSG_HANDLER_HPP__ */

