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

   Source File Name = coordCommandBackup.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandBackup.hpp"
#include "coordUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordBackupBase implement
   */
   _coordBackupBase::_coordBackupBase()
   {
   }

   _coordBackupBase::~_coordBackupBase()
   {
   }

   INT32 _coordBackupBase::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordCtrlParam ctrlParam ;
      rtnContextCoord::sharePtr pContext ;
      ROUTE_RC_MAP failedNodes ;
      SET_ROUTEID sucNodes ;
      UINT32 mask = _getMask() ;

      contextID = -1 ;
      ctrlParam._isGlobal = TRUE ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
      ctrlParam._filterID = _getGroupMatherIndex() ;
      ctrlParam._emptyFilterSel = _nodeSelWhenNoFilter() ;

      rc = executeOnNodes( pMsg, cb, ctrlParam, mask, failedNodes,
                           _useContext() ? &pContext : NULL,
                           NULL, NULL, &sucNodes ) ;
      if ( rc )
      {
         if ( SDB_RTN_CMD_IN_LOCAL_MODE == rc )
         {
            rc = SDB_INVALIDARG ;
         }
         else
         {
            PD_LOG( PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }

   done:
      if ( ( rc || failedNodes.size() > 0 ) && -1 == contextID && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &failedNodes,
                                                    sucNodes.size() ) ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordRemoveBackup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordRemoveBackup,
                                      CMD_NAME_REMOVE_BACKUP,
                                      FALSE ) ;
   _coordRemoveBackup::_coordRemoveBackup()
   {
   }

   _coordRemoveBackup::~_coordRemoveBackup()
   {
   }

   FILTER_BSON_ID _coordRemoveBackup::_getGroupMatherIndex ()
   {
      return FILTER_ID_MATCHER ;
   }

   NODE_SEL_STY _coordRemoveBackup::_nodeSelWhenNoFilter ()
   {
      return NODE_SEL_ALL ;
   }

   BOOLEAN _coordRemoveBackup::_useContext ()
   {
      return FALSE ;
   }

   UINT32 _coordRemoveBackup::_getMask() const
   {
      return ~COORD_CTRL_MASK_NODE_SELECT ;
   }

   /*
      _coordBackupOffline implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordBackupOffline,
                                      CMD_NAME_BACKUP_OFFLINE,
                                      FALSE ) ;
   _coordBackupOffline::_coordBackupOffline()
   {
   }

   _coordBackupOffline::~_coordBackupOffline()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORD_BK_OFFLINE_EXEC, "_coordBackupOffline::execute" )
   INT32 _coordBackupOffline::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( SDB__COORD_BK_OFFLINE_EXEC ) ;

      INT32 rc = SDB_OK ;
      CoordGroupList groupLst ;

      rc = _parseGroupList( pMsg, groupLst, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse group list, rc: %d", rc ) ;

      rc = _executeOnGroups( pMsg, cb, groupLst, MSG_ROUTE_SHARD_SERVCIE,
                             FALSE, NULL, NULL ,NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do backup on groups, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__COORD_BK_OFFLINE_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORD_BK_OFFLINE__PARSE_GRP_LST, "_coordBackupOffline::_parseGroupList" )
   INT32 _coordBackupOffline::_parseGroupList( MsgHeader *pMsg,
                                               CoordGroupList &groupLst,
                                               pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__COORD_BK_OFFLINE__PARSE_GRP_LST ) ;

      INT32 rc = SDB_OK ;
      rtnQueryOptions queryOption ;
      BSONObj *pFilterObj = NULL ;
      BSONObj newFilterObj ;
      INT32 roleFilter[ SDB_ROLE_MAX ] = { 0 } ;

      rc = queryOption.fromQueryMsg( (CHAR*)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      pFilterObj = coordGetFilterByID( _getGroupMatherIndex(), queryOption ) ;

      rc = coordParseGroupList( _pResource, cb, *pFilterObj, groupLst,
                                &newFilterObj, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse group list, rc: %d", rc ) ;

      // use all group when nothing specified
      if ( pFilterObj->equal( newFilterObj ) )
      {
         rc = _pResource->updateGroupList( groupLst, cb, NULL, FALSE, FALSE,
                                           TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get all group list, rc: %d", rc ) ;
      }

      roleFilter[ SDB_ROLE_DATA ] = 1 ;
      roleFilter[ SDB_ROLE_CATALOG ] = 1 ;
      coordFilterGroupsByRole( groupLst, roleFilter ) ;

      // TODO: reject group argument when cpID was specified.

   done:
      PD_TRACE_EXITRC( SDB__COORD_BK_OFFLINE__PARSE_GRP_LST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   FILTER_BSON_ID _coordBackupOffline::_getGroupMatherIndex ()
   {
      return FILTER_ID_MATCHER ;
   }

   NODE_SEL_STY _coordBackupOffline::_nodeSelWhenNoFilter ()
   {
      return NODE_SEL_PRIMARY ;
   }

   BOOLEAN _coordBackupOffline::_useContext ()
   {
      return FALSE ;
   }

   UINT32 _coordBackupOffline::_getMask() const
   {
      return ~COORD_CTRL_MASK_NODE_SELECT ;
   }

   BOOLEAN _coordBackupOffline::_interruptWhenFailed() const
   {
      // TODO: There are some issues of interruption.
      // Wait SEQUOIADBMAINSTREAM-8068 to fix it.
      // return TRUE ;
      return FALSE ;
   }

}

