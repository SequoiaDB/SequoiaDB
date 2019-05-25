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

   Source File Name = coordGroupHandle.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordGroupHandle.hpp"

namespace engine
{

   /*
      _coordGroupHandler implement
   */
   _coordGroupHandler::_coordGroupHandler()
   {
   }

   _coordGroupHandler::~_coordGroupHandler()
   {
   }

   void _coordGroupHandler::prepareForSend( pmdSubSession *pSub,
                                            _coordGroupSel *pSel,
                                            _coordGroupSessionCtrl *pCtrl )
   {
      if ( !pSel->isPrimary() && pSel->isPreferedPrimary() )
      {
         MsgHeader *pMsg = pSub->getReqMsg() ;
         BOOLEAN isResend = pCtrl->getRetryTimes() > 0 ? TRUE : FALSE ;

         if ( MSG_BS_QUERY_REQ == pMsg->opCode )
         {
            MsgOpQuery *pQuery = ( MsgOpQuery* )pMsg ;
            if ( FALSE == isResend )
            {
               pQuery->flags |= FLG_QUERY_PRIMARY ;
            }
            else
            {
               pQuery->flags &= ~FLG_QUERY_PRIMARY ;
            }
         }
         else if ( pMsg->opCode > ( SINT32 )MSG_LOB_BEGIN &&
                   pMsg->opCode < ( SINT32 )MSG_LOB_END )
         {
            MsgOpLob *pLobMsg = ( MsgOpLob* )pMsg ;
            if ( FALSE == isResend )
            {
               pLobMsg->flags |= FLG_LOBREAD_PRIMARY ;
            }
            else
            {
               pLobMsg->flags &= ~FLG_LOBREAD_PRIMARY ;
            }
         }
      }
   }

}

