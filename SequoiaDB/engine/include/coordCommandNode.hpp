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

   Source File Name = coordCommandNode.hpp

   Descriptive Name = Runtime Coord Commands for Node Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/25/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_NODE_HPP__
#define COORD_COMMAND_NODE_HPP__

#include "coordCommand2Phase.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordNodeCMD2Phase define
   */
   class _coordNodeCMD2Phase : public _coordCMD2Phase
   {
      public:
         _coordNodeCMD2Phase() ;
         virtual ~_coordNodeCMD2Phase() ;

      protected :
         virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          const vector<BSONObj> &cataObjs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) ;

         virtual INT32 _doAudit ( coordCMDArguments *pArgs, INT32 rc ) ;

      private:
         virtual AUDIT_OBJ_TYPE     _getAuditObjectType() const ;
         virtual string _getAuditObjectName( coordCMDArguments *pArgs ) const ;
         virtual string _getAuditDesp( coordCMDArguments *pArgs ) const ;

   } ;
   typedef _coordNodeCMD2Phase coordNodeCMD2Phase ;

   /*
      _coordNodeCMD3Phase define
   */
   class _coordNodeCMD3Phase : public _coordNodeCMD2Phase
   {
      public:
         _coordNodeCMD3Phase() ;
         virtual ~_coordNodeCMD3Phase() ;

      protected :
         virtual INT32 _doOnCataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord::sharePtr *ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &pGroupLst,
                                          vector<BSONObj> &cataObjs ) ;

   } ;
   typedef _coordNodeCMD3Phase coordNodeCMD3Phase ;

   /*
      _coordCMDOperateOnNode define
   */
   class _coordCMDOperateOnNode : public _coordCommandBase
   {
      public:
         _coordCMDOperateOnNode() ;
         virtual ~_coordCMDOperateOnNode() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual SINT32 getOpType() const = 0 ;
   } ;
   typedef _coordCMDOperateOnNode coordCMDOperateOnNode ;

   /*
      _coordCMDStartupNode define
   */
   class _coordCMDStartupNode : public _coordCMDOperateOnNode
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDStartupNode() ;
         virtual ~_coordCMDStartupNode() ;

         virtual SINT32 getOpType() const ;
   } ;
   typedef _coordCMDStartupNode coordCMDStartupNode ;

   /*
      _coordCMDShutdownNode define
   */
   class _coordCMDShutdownNode : public _coordCMDOperateOnNode
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDShutdownNode() ;
         virtual ~_coordCMDShutdownNode() ;

         virtual SINT32 getOpType() const ;
   } ;
   typedef _coordCMDShutdownNode coordCMDShutdownNode ;

   /*
      _coordCMDOpOnGroup define
   */
   class _coordCMDOpOnGroup
   {
      public:
         _coordCMDOpOnGroup() ;
         virtual ~_coordCMDOpOnGroup() ;

      protected :
         INT32 _opOnOneNode ( const vector<INT32> &opList,
                              string hostName,
                              string svcName,
                              vector<BSONObj> &dataObjs ) ;

         INT32 _opOnNodes ( const vector<INT32> &opList,
                            const BSONObj &boGroupInfo,
                            vector<BSONObj> &dataObjs ) ;

         INT32 _opOnCataNodes ( const vector<INT32> &opList,
                                clsGroupItem *pItem,
                                vector<BSONObj> &dataObjs ) ;

         INT32 _opOnVecNodes( const vector<INT32> &opList,
                              const CoordVecNodeInfo &vecNodes,
                              vector<BSONObj> &dataObjs ) ;

         void _logErrorForNodes ( const CHAR *pCmdName,
                                  const string &targetName,
                                  const vector<BSONObj> &dataObjs ) ;

      private:
         virtual BOOLEAN _flagIsStartNodes() const ;
   } ;
   typedef _coordCMDOpOnGroup coordCMDOpOnGroup ;

   /*
      _coordCMDConfigNode define
   */
   class _coordCMDConfigNode
   {
      public:
         _coordCMDConfigNode() ;
         virtual ~_coordCMDConfigNode() ;

         INT32 getNodeInfo( const CHAR *pQuery,
                            BSONObj &nodeInfo ) ;

         INT32 getNodeConf( coordResource *pResource,
                            const CHAR *pQuery,
                            BSONObj &nodeConf ) ;

      protected:
         virtual INT32  _onParseConfig( const BSONElement &ele,
                                        const CHAR *pFieldName,
                                        BOOLEAN &ignored ) ;

         virtual INT32  _onPostBuildConfig( coordResource *pResource,
                                            BSONObjBuilder &builder,
                                            const CHAR *roleStr,
                                            BOOLEAN hasCataAddr ) ;

         virtual BOOLEAN _ignoreGroupNameWhenNodeInfo() const ;

   } ;
   typedef _coordCMDConfigNode coordCMDConfigNode ;

   /*
      _coordCMDUpdateNode define
   */
   class _coordCMDUpdateNode : public _coordCommandBase,
                               public _coordCMDConfigNode
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDUpdateNode() ;
         virtual ~_coordCMDUpdateNode() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDUpdateNode coordCMDUpdateNode ;

   /*
      _coordCMDCreateCataGroup define
   */
   class _coordCMDCreateCataGroup : public _coordCommandBase,
                                    public _coordCMDConfigNode
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCreateCataGroup() ;
         virtual ~_coordCMDCreateCataGroup() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      protected:
         virtual INT32  _onParseConfig( const BSONElement &ele,
                                        const CHAR *pFieldName,
                                        BOOLEAN &ignored ) ;

         virtual INT32  _onPostBuildConfig( coordResource *pResource,
                                            BSONObjBuilder &builder,
                                            const CHAR *roleStr,
                                            BOOLEAN hasCataAddr ) ;

         virtual BOOLEAN _ignoreGroupNameWhenNodeInfo() const ;

      private:

         const CHAR           *_pHostName ;
         const CHAR           *_pSvcName ;
         string               _cataSvcName ;

   } ;
   typedef _coordCMDCreateCataGroup coordCMDCreateCataGroup ;

   /*
      _coordCMDCreateGroup define
   */
   class _coordCMDCreateGroup : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDCreateGroup() ;
         virtual ~_coordCMDCreateGroup() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDCreateGroup coordCMDCreateGroup ;

   /*
      _coordCMDActiveGroup define
   */
   class _coordCMDActiveGroup : public _coordNodeCMD2Phase,
                                public _coordCMDOpOnGroup
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDActiveGroup() ;
         virtual ~_coordCMDActiveGroup() ;

      protected :

         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual INT32 _doOnCataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        CoordGroupList *pGroupLst,
                                        vector<BSONObj> *pReplyObjs ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;

         virtual BOOLEAN _flagIsStartNodes() const ;

      private:
         CoordVecNodeInfo        _vecNodes ;

   } ;
   typedef _coordCMDActiveGroup coordCMDActiveGroup ;

   /*
      _coordCMDShutdownGroup define
   */
   class _coordCMDShutdownGroup : public _coordNodeCMD2Phase,
                                  public _coordCMDOpOnGroup
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDShutdownGroup() ;
         virtual ~_coordCMDShutdownGroup() ;

      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;

         virtual INT32 _doCommit ( MsgHeader *pMsg,
                                   pmdEDUCB * cb,
                                   rtnContextCoord::sharePtr *ppContext,
                                   coordCMDArguments *pArgs ) ;

   } ;
   typedef _coordCMDShutdownGroup coordCMDShutdownGroup ;

   /*
      _coordCMDRemoveGroup define
   */
   class _coordCMDRemoveGroup : public _coordNodeCMD3Phase,
                                public _coordCMDOpOnGroup
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDRemoveGroup() ;
         virtual ~_coordCMDRemoveGroup() ;

      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;

         virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord::sharePtr *ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &groupLst,
                                          const vector<BSONObj> &cataObjs ) ;

         virtual INT32 _doCommit ( MsgHeader *pMsg,
                                   pmdEDUCB * cb,
                                   rtnContextCoord::sharePtr *ppContext,
                                   coordCMDArguments *pArgs ) ;

      private:
         BOOLEAN           _enforce ;

   } ;
   typedef _coordCMDRemoveGroup coordCMDRemoveGroup ;

   /*
      _coordCMDCreateNode define
   */
   class _coordCMDCreateNode : public _coordNodeCMD3Phase,
                               public _coordCMDConfigNode,
                               public _coordCMDEventHandler
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCreateNode() ;
         virtual ~_coordCMDCreateNode() ;

      public:
         virtual INT32 parseCatP2Return( coordCMDArguments *pArgs,
                                         const std::vector< bson::BSONObj > &cataObjs );

      protected :

         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;

         virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord::sharePtr *ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &groupLst,
                                          const vector<BSONObj> &cataObjs ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

         virtual INT32 _doOutput( rtnContextBuf *buf ) ;

         virtual INT32 _regEventHandlers()
         {
            return _regEventHandler( this );
         }

      private:
         virtual AUDIT_OBJ_TYPE     _getAuditObjectType() const ;
         virtual string _getAuditObjectName( coordCMDArguments *pArgs ) const ;
         virtual string _getAuditDesp( coordCMDArguments *pArgs ) const ;

      protected:
         const CHAR        *_pHostName ;
         BOOLEAN           _onlyAttach ;
         BOOLEAN           _keepData ;
         const CHAR        *_pSvcName ;
         BSONObj           _nodeInfo ;
   } ;
   typedef _coordCMDCreateNode coordCMDCreateNode ;

   /*
      _coordCMDRemoveNode define
   */
   class _coordCMDRemoveNode : public _coordNodeCMD3Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDRemoveNode() ;
         virtual ~_coordCMDRemoveNode() ;

      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord::sharePtr *ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;

         virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord::sharePtr *ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &groupLst,
                                          const vector<BSONObj> &cataObjs ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

      private:
         virtual AUDIT_OBJ_TYPE     _getAuditObjectType() const ;
         virtual string _getAuditObjectName( coordCMDArguments *pArgs ) const ;
         virtual string _getAuditDesp( coordCMDArguments *pArgs ) const ;

      protected:
         const CHAR        *_pHostName ;
         const CHAR        *_pSvcName ;
         BOOLEAN           _onlyDetach ;
         BOOLEAN           _keepData ;
         BOOLEAN           _enforce ;

   } ;
   typedef _coordCMDRemoveNode coordCMDRemoveNode ;

   /*
      _coordCMDAlterNode define
   */
   class _coordCMDAlterNode : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDAlterNode() ;
         virtual ~_coordCMDAlterNode() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDAlterNode coordCMDAlterNode ;

   /*
      _coordCMDReelectBase define
   */
   class _coordCMDReelectBase : public _coordCommandBase
   {
   public:
      _coordCMDReelectBase() ;
      virtual ~_coordCMDReelectBase() ;

   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      INT32 _parseNodeInfo( const BSONObj &optionsObj,
                            MsgRouteID &nodeID,
                            pmdEDUCB *cb ) ;

      virtual INT32 _parseExtraArgs( const BSONElement &e ) ;

      virtual INT32 _checkExtraArgs( pmdEDUCB *cb ) ;

      virtual INT32 _getNotifyNode( MsgRouteID& nodeID,
                                    const UINT16 tmpNodeID,
                                    const CHAR* hostName,
                                    const CHAR* svcName ) = 0 ;

      virtual INT32 _buildNotifyMsg( CHAR **ppBuffer,
                                     INT32 *bufferSize,
                                     const MsgRouteID &nodeID,
                                     pmdEDUCB *cb ) = 0 ;

      INT32 _buildReelectMsg( CHAR **ppBuffer,
                              INT32 *bufferSize,
                              const BSONObj &optionsObj,
                              const MsgRouteID &nodeID,
                              const CHAR* cmdName,
                              pmdEDUCB *cb ) ;

      void _notifyReelect2DestNode( MsgHeader *pMsg,
                                    const MsgRouteID &nodeID,
                                    pmdEDUCB *cb ) ;

      virtual INT32 _reelect( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              rtnContextBuf *buf ) = 0 ;

   protected:
      CoordGroupInfoPtr _groupInfoPtr ;
      BOOLEAN           _updatedGrpInfo ;
      const CHAR*       _groupName ;
   } ;
   typedef _coordCMDReelectBase coordCMDReelect ;

   /*
      _coordCMDReelectGroup define
   */
   class _coordCMDReelectGroup : public _coordCMDReelectBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDReelectGroup() ;
      virtual ~_coordCMDReelectGroup() ;

   protected:
      virtual INT32 _getNotifyNode( MsgRouteID& nodeID,
                                    const UINT16 tmpNodeID,
                                    const CHAR* hostName,
                                    const CHAR* svcName ) ;

      virtual INT32 _buildNotifyMsg( CHAR **ppBuffer,
                                     INT32 *bufferSize,
                                     const MsgRouteID &nodeID,
                                     pmdEDUCB *cb ) ;

      virtual INT32 _reelect( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDReelectGroup coordCMDReelectGroup ;

   /*
      _coordCMDReelectLocation define
   */
   class _coordCMDReelectLocation : public _coordCMDReelectBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDReelectLocation() ;
      virtual ~_coordCMDReelectLocation() ;

   protected:
      virtual INT32 _parseExtraArgs( const BSONElement &e ) ;

      virtual INT32 _checkExtraArgs( pmdEDUCB *cb ) ;

      virtual INT32 _getNotifyNode( MsgRouteID& nodeID,
                                    const UINT16 tmpNodeID,
                                    const CHAR* hostName,
                                    const CHAR* svcName ) ;

      virtual INT32 _buildNotifyMsg( CHAR **ppBuffer,
                                     INT32 *bufferSize,
                                     const MsgRouteID &nodeID,
                                     pmdEDUCB *cb ) ;

      virtual INT32 _reelect( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              rtnContextBuf *buf ) ;

   private:
      const CHAR* _location ;
      UINT32      _locationID ;
   } ;
   typedef _coordCMDReelectLocation coordCMDReelectLocation ;

   /*
      _coordCMDAlterRG define
   */
   class _coordCMDAlterRG : public _coordNodeCMD2Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDAlterRG() ;
         virtual ~_coordCMDAlterRG() ;

      protected:
         virtual INT32 _parseMsg( MsgHeader *pMsg,
                                  coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         coordCMDArguments *pArgs,
                                         CHAR **ppMsgBuf,
                                         INT32 *pBufSize ) ;

         virtual INT32 _doOnDataGroup( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       rtnContextCoord::sharePtr *ppContext,
                                       coordCMDArguments *pArgs,
                                       const CoordGroupList &groupLst,
                                       const vector<BSONObj> &cataObjs,
                                       CoordGroupList &sucGroupLst ) ;

         virtual INT32 _doComplete( MsgHeader *pMsg,
                                    pmdEDUCB * cb,
                                    coordCMDArguments *pArgs ) ;

         virtual INT32 _executeByLocation( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           UINT32 locationID ) ;

         virtual INT32 _executeByNode( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       UINT16 nodeID ) ;

         virtual INT32 _reelectGroup( pmdEDUCB *cb ) ;

      private:
         INT32 _checkCriticalMode( pmdEDUCB *cb,
                                   const UINT16 &nodeID,
                                   const UINT32 &locationID,
                                   const UINT32 &waitTime ) ;

         INT32 _startCriticalModeOnData( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         const vector<BSONObj> &cataObjs ) ;

      private:
         UINT32              _groupID ;
         const CHAR*         _pActionName ;
         CoordGroupInfoPtr   _groupInfoPtr ;
   } ;
   typedef _coordCMDAlterRG coordCMDAlterRG ;

}

#endif // COORD_COMMAND_NODE_HPP__
