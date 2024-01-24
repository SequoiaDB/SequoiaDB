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

   void _coordGroupHandler::prepareForSend( UINT32 groupID,
                                            pmdSubSession *pSub,
                                            _coordGroupSel *pSel,
                                            _coordGroupSessionCtrl *pCtrl )
   {
      SDB_ASSERT( pSub, "sub session can't be NULL" ) ;
      SDB_ASSERT( pSel, "group sel can't be NULL" ) ;
      SDB_ASSERT( pCtrl, "group session ctrl can't be NULL" ) ;

      MsgHeader *pMsg = pSub->getReqMsg() ;
      BOOLEAN isResend = pCtrl->getRetryTimes() > 0 ? TRUE : FALSE ;
      SINT32 *pFlags = NULL ;
      SINT32 primaryFlag = 0 ;
      SINT32 secondaryFlag = 0 ;
      CoordGroupInfoPtr groupPtr ;

      if ( MSG_BS_QUERY_REQ == pMsg->opCode )
      {
         MsgOpQuery *pQuery = ( MsgOpQuery* )pMsg ;
         pFlags = &(pQuery->flags) ;
         primaryFlag = FLG_QUERY_PRIMARY ;
         secondaryFlag = FLG_QUERY_SECONDARY ;
      }
      else if ( pMsg->opCode > ( SINT32 )MSG_LOB_BEGIN &&
                pMsg->opCode < ( SINT32 )MSG_LOB_END )
      {
         MsgOpLob *pLobMsg = ( MsgOpLob* )pMsg ;
         pFlags = &(pLobMsg->flags) ;
         primaryFlag = FLG_LOBREAD_PRIMARY ;
         secondaryFlag = FLG_LOBREAD_SECONDARY ;
      }
      else
      {
         goto done ;
      }

      OSS_BIT_CLEAR( *pFlags, primaryFlag ) ;
      OSS_BIT_CLEAR( *pFlags, secondaryFlag ) ;

      if ( !pSel->isPrimary() )
      {
         // NOTE: we need to set query flags for below cases
         // - primary is preferred( including required )
         // - secondary is required
         // - secondary is preferred, and not select from the last node,
         //   and not the only one node in the group.
         // NOTE: for last case, only set flag for node which is not
         //       selected from the last node, since we need to keep use
         //       the last node during preferred period after a write operator.
         if ( isResend && !pSel->isRequiredPrimary() &&
              !pSel->isRequiredSecondary() )
         {
            // make sure to clear both primary or secondary flag when resend
            OSS_BIT_CLEAR( *pFlags, primaryFlag ) ;
            OSS_BIT_CLEAR( *pFlags, secondaryFlag ) ;
         }
         else if ( pSel->isPreferredPrimary() )
         {
            // if primary is required or preferred, set primary query flag
            OSS_BIT_SET( *pFlags, primaryFlag ) ;
         }
         else if ( pSel->isRequiredSecondary() ||
                   ( pSel->isPreferredSecondary() &&
                     !pSel->existLastNode( groupID ) &&
                     pSel->getGroupPtrFromMap( groupID, groupPtr ) &&
                     groupPtr->nodeCount() > 1 ) )
         {
            // if secondary is required or preferred, set secondary query flag
            OSS_BIT_SET( *pFlags, secondaryFlag ) ;
         }
      }

   done:
      return ;
   }

}

