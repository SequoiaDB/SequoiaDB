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

   Source File Name = coordCommandBase.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_BASE_HPP__
#define COORD_COMMAND_BASE_HPP__

#include "coordOperator.hpp"
#include "coordContext.hpp"
#include "rtnQueryOptions.hpp"
#include "coordCommon.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCommandBase define
   */
   class _coordCommandBase : public _coordOperator
   {
      public:
         _coordCommandBase() ;
         virtual ~_coordCommandBase() ;

      public:
         INT32         executeOnCL( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    const CHAR *pCLName,
                                    BOOLEAN firstUpdateCata = FALSE,
                                    const CoordGroupList *pSpecGrpLst = NULL,
                                    SET_RC *pIgnoreRC = NULL,
                                    CoordGroupList *pSucGrpLst = NULL,
                                    rtnContextCoord **ppContext = NULL,
                                    rtnContextBuf *buf = NULL ) ;

         INT32         queryOnCL( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  const CHAR *pCLName,
                                  rtnContextCoord **ppContext,
                                  BOOLEAN onPrimary = FALSE,
                                  const CoordGroupList *pSpecGrpLst = NULL,
                                  rtnContextBuf *buf = NULL ) ;

         INT32         executeOnDataGroup ( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            const CoordGroupList &groupLst,
                                            BOOLEAN onPrimary = TRUE,
                                            SET_RC *pIgnoreRC = NULL,
                                            CoordGroupList *pSucGrpLst = NULL,
                                            rtnContextCoord **ppContext = NULL,
                                            rtnContextBuf *buf = NULL ) ;

         INT32         executeOnCataGroup ( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            BOOLEAN onPrimary = TRUE,
                                            SET_RC *pIgnoreRC = NULL,
                                            rtnContextCoord **ppContext = NULL,
                                            rtnContextBuf *buf = NULL ) ;

         INT32         executeOnCataGroup ( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            CoordGroupList *pGroupList,
                                            vector<BSONObj> *pReplyObjs = NULL,
                                            BOOLEAN onPrimary = TRUE,
                                            SET_RC *pIgnoreRC = NULL,
                                            rtnContextBuf *buf = NULL ) ;

         INT32         executeOnCataCL( MsgOpQuery *pMsg,
                                        pmdEDUCB *cb,
                                        const CHAR *pCLName,
                                        BOOLEAN onPrimary = TRUE,
                                        SET_RC *pIgnoreRC = NULL,
                                        rtnContextCoord **ppContext = NULL,
                                        rtnContextBuf *buf = NULL ) ;

         INT32         queryOnCatalog( MsgHeader *pMsg,
                                       INT32 requestType,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

         INT32         queryOnCatalog( const rtnQueryOptions &options,
                                       pmdEDUCB *cb,
                                       SINT64 &contextID,
                                       rtnContextBuf *buf ) ;

         INT32         queryOnCataAndPushToVec( const rtnQueryOptions &options,
                                                pmdEDUCB *cb,
                                                vector<BSONObj> &objs,
                                                rtnContextBuf *buf ) ;

         INT32         executeOnNodes( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       SET_ROUTEID &nodes,
                                       ROUTE_RC_MAP &faileds,
                                       SET_ROUTEID *pSucNodes = NULL,
                                       SET_RC *pIgnoreRC = NULL,
                                       rtnContextCoord *pContext = NULL ) ;

         INT32         executeOnNodes( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       coordCtrlParam &ctrlParam,
                                       UINT32 mask,
                                       ROUTE_RC_MAP &faileds,
                                       rtnContextCoord **ppContext = NULL,
                                       BOOLEAN openEmptyContext = FALSE,
                                       SET_RC *pIgnoreRC = NULL,
                                       SET_ROUTEID *pSucNodes = NULL ) ;

      protected:
         virtual void _printDebug ( const CHAR *pReceiveBuffer,
                                    const CHAR *pFuncName ) ;

         /* Enable preRead in Coord context (send GetMore in advanced) */
         virtual BOOLEAN _flagCoordCtxPreRead () { return TRUE ; }

      private:

         INT32 _processSucReply( ROUTE_REPLY_MAP &okReply,
                                 rtnContextCoord *pContext ) ;

         INT32 _processNodesReply( pmdRemoteSession *pSession,
                                   ROUTE_RC_MAP &faileds,
                                   rtnContextCoord *pContext = NULL,
                                   SET_RC *pIgnoreRC = NULL,
                                   SET_ROUTEID *pSucNodes = NULL ) ;

         INT32 _buildFailedNodeReply( ROUTE_RC_MAP &failedNodes,
                                      rtnContextCoord *pContext ) ;

         INT32 _executeOnGroups ( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  const CoordGroupList &groupLst,
                                  MSG_ROUTE_SERVICE_TYPE type,
                                  BOOLEAN onPrimary = TRUE,
                                  SET_RC *pIgnoreRC = NULL,
                                  CoordGroupList *pSucGrpLst = NULL,
                                  rtnContextCoord **ppContext = NULL,
                                  rtnContextBuf *buf = NULL ) ;

   } ;
   typedef _coordCommandBase coordCommandBase ;

}

#endif // COORD_COMMAND_BASE_HPP__

