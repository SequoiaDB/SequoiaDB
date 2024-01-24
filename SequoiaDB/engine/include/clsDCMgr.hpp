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

   Source File Name = clsDCMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/02/2015  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DC_CLS_MGR_HPP__
#define DC_CLS_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "pmdOptionsMgr.hpp"
#include "netRouteAgent.hpp"
#include "ossLatch.hpp"
#include "clsCatalogAgent.hpp"
#include "utilRecycleBinConf.hpp"
#include "sdbInterface.hpp"
#include <map>
#include <string>
#include <vector>

using namespace bson ;
using namespace std ;

namespace engine
{
   // the dc mgr update some info from catalog timeout
   #define DC_UPDATE_TIMEOUT     (10*OSS_ONE_SEC)

   /*
      _clsDCBaseInfo define
   */
   class _clsDCBaseInfo : public SDBObject
   {
      public:
         _clsDCBaseInfo() ;
         ~_clsDCBaseInfo() ;

         INT32          lock_r( INT32 millisec = -1 ) ;
         INT32          release_r() ;
         INT32          lock_w( INT32 millisec = -1 ) ;
         INT32          release_w() ;

         INT32          updateFromBSON( BSONObj &obj,
                                        BOOLEAN checkGroups = FALSE ) ;

         void           setImageAddress( const string &addr ) ;
         void           setImageClusterName( const string &name ) ;
         void           setImageBusinessName( const string &name ) ;
         void           enableImage( BOOLEAN enable = TRUE ) ;

         INT32          addGroup( const string &source,
                                  const string &image,
                                  BOOLEAN *pAdded = NULL ) ;
         INT32          addGroups( const BSONObj &groups,
                                   map< string, string > *pAddMapGrps = NULL ) ;

         INT32          delGroup( const string &source, string &image ) ;
         INT32          delGroups( const BSONObj &groups,
                                   map< string, string > *pDelMapGrps = NULL ) ;

         void           setClusterName( const string &name ) ;
         void           setBusinessName( const string &name ) ;
         void           setAddress( const string &addr ) ;

         void           setAcitvated( BOOLEAN activated ) ;
         void           setReadonly( BOOLEAN readonly ) ;

      public:
         BOOLEAN        isActivated() const { return _activated ; }
         BOOLEAN        isReadonly() const { return _readonly ; }

         BOOLEAN        hasCSUniqueHWM() const { return _hasCsUniqueHWM ; }
         utilCSUniqueID getCSUniqueHWM() const { return _csUniqueHWM ; }
         UINT32         getCATVersion() const { return _catVersion ; }

         const CHAR*    getClusterName() const ;
         const CHAR*    getBusinessName() const ;
         const CHAR*    getAddress() const ;

         BOOLEAN        hasImage() const { return _hasImage ; }
         const CHAR*    getImageClusterName() const ;
         const CHAR*    getImageBusinessName() const ;
         const CHAR*    getImageAddress() const ;
         BOOLEAN        imageIsEnabled() const { return _imageIsEnabled ; }

         map<string, string>* getImageGroups() { return &_imageGroups ; }
         map<string, string>* getRImageGroups() { return &_imageRGroups ; }
         string         source2dest( const string &sourceGroup ) ;
         string         dest2source( const string &destGroup ) ;

         BSONObj        getOrgObj() const { return _orgObj ; }

         utilRecycleBinConf getRecycleBinConf() ;
         void setRecycleBinConf( const utilRecycleBinConf &conf ) ;

      protected:
         void           _reset() ;
         INT32          _addGroup( const BSONObj &obj, BOOLEAN check,
                                   BOOLEAN *pAdded = NULL,
                                   const CHAR **ppSource = NULL,
                                   const CHAR **ppImage = NULL ) ;
         INT32          _delGroup( const BSONObj &obj,
                                   string *pSourceName = NULL,
                                   string *pImageName = NULL ) ;

      private:
         BSONObj        _orgObj ;

         string         _clusterName ;
         string         _businessName ;
         string         _address ;
         BOOLEAN        _activated ;
         BOOLEAN        _readonly ;
         BOOLEAN        _hasCsUniqueHWM ;
         utilCSUniqueID _csUniqueHWM ;
         UINT32         _catVersion ;

         string         _imageClusterName ;
         string         _imageBusinessName ;
         string         _imageAddress ;
         BOOLEAN        _hasImage ;
         BOOLEAN        _imageIsEnabled ;

         map< string, string >  _imageGroups ;  // source 2 dest
         map< string, string >  _imageRGroups ; // dest 2 source

         utilRecycleBinConf _recycleBinConf ;

         ossRWMutex     _rwMutex ;

   } ;
   typedef _clsDCBaseInfo clsDCBaseInfo ;

   /*
      Node select strategy define
   */
   #define SEL_NODE_PRIMARY         0x0001
   #define SEL_NODE_SLAVE           0x0002
   #define SEL_NODE_ALL             0xFFFF

   /*
      Node send strategy define
   */
   enum SEND_STRATEGY
   {
      SEND_NODE_ONE,
      SEND_NODE_ALL
   } ;

   /*
      _clsDCMgr define
   */
   class _clsDCMgr :  public SDBObject
   {
      public:
         _clsDCMgr() ;
         ~_clsDCMgr() ;

         INT32          initialize() ;

         INT32          setImageCatAddr( const string &catAddr ) ;
         string         getImageCatAddr() ;

         vector< pmdAddrPair > getImageCatVec() ;

         catAgent*      getImageCataAgent () ;
         nodeMgrAgent*  getImageNodeMgrAgent () ;
         clsDCBaseInfo* getDCBaseInfo() { return &_baseInfo ; }
         clsDCBaseInfo* getImageDCBaseInfo( pmdEDUCB *cb,
                                            BOOLEAN update = FALSE ) ;

         /*
            Update data center base info from obj
         */
         INT32 updateDCBaseInfo( BSONObj &obj ) ;
         INT32 updateDCBaseInfo( MsgOpReply* pRes ) ;
         /*
            Update data center image's catalog group info
         */
         INT32 updateImageCataGroup( pmdEDUCB *cb,
                                     INT64 millsec = DC_UPDATE_TIMEOUT ) ;
         /*
            Update image group info by group id(catalog,data)
         */
         INT32 updateImageGroup( UINT32 groupID, pmdEDUCB *cb,
                                 INT64 millsec = DC_UPDATE_TIMEOUT ) ;
         /*
            Update image group info by group name(catalog,data)
         */
         INT32 updateImageGroup( const CHAR *groupName, pmdEDUCB *cb,
                                 INT64 millsec = DC_UPDATE_TIMEOUT ) ;
         /*
            Update image all groups
         */
         INT32 updateImageAllGroups( pmdEDUCB *cb,
                                     INT64 millsec = DC_UPDATE_TIMEOUT ) ;
         /*
            Update iamge all catalog
         */
         INT32 updateImageAllCatalog( pmdEDUCB *cb,
                                      INT64 millsec = DC_UPDATE_TIMEOUT ) ;

         /*
            Update image dc base info
         */
         INT32 updateImageDCBaseInfo( pmdEDUCB *cb,
                                      INT64 millsec = DC_UPDATE_TIMEOUT ) ;

         INT32 getAndLockImageCataSet( const CHAR *name,
                                       pmdEDUCB *cb,
                                       clsCatalogSet **ppSet,
                                       BOOLEAN noWithUpdate = TRUE,
                                       INT64 waitMillSec = DC_UPDATE_TIMEOUT,
                                       BOOLEAN *pUpdated = NULL ) ;
         INT32 unlockImageCataSet( clsCatalogSet *catSet ) ;

         INT32 getAndLockImageGroupItem( UINT32 id,
                                         pmdEDUCB *cb,
                                         clsGroupItem **ppItem,
                                         BOOLEAN noWithUpdate = TRUE,
                                         INT64 waitMillSec = DC_UPDATE_TIMEOUT,
                                         BOOLEAN *pUpdated = NULL ) ;
         INT32 unlockImageGroupItem( clsGroupItem *item ) ;

      public:

         INT32  syncSend2ImageNode( MsgHeader *msg,
                                    pmdEDUCB *cb,
                                    UINT32 groupID,
                                    vector< pmdEDUEvent > &vecRecv,
                                    vector< pmdAddrPair > &vecSendNode,
                                    INT32 selType = SEL_NODE_PRIMARY,
                                    SEND_STRATEGY sendSty = SEND_NODE_ONE,
                                    MSG_ROUTE_SERVICE_TYPE type =
                                    MSG_ROUTE_CAT_SERVICE,
                                    INT64 millisecond = DC_UPDATE_TIMEOUT,
                                    vector< pmdAddrPair > *pVecFailedNode = NULL ) ;

         INT32  syncSend2ImageNodes( MsgHeader *msg,
                                     pmdEDUCB *cb,
                                     vector< pmdEDUEvent > &vecRecv,
                                     vector< pmdAddrPair > &vecSendNode,
                                     INT32 selType = SEL_NODE_PRIMARY,
                                     SEND_STRATEGY sendSty = SEND_NODE_ONE,
                                     INT64 millisecond = DC_UPDATE_TIMEOUT,
                                     vector< pmdAddrPair > *pVecFailedNode = NULL ) ;

      protected:

         /*
            if ppRecvMsg is NULL, don't recv reply
         */
         INT32 _syncSend2PeerNode( pmdEDUCB *cb, MsgHeader *msg,
                                   pmdEDUEvent *pRecvEvent,
                                   const pmdAddrPair &node,
                                   INT64 millsec = DC_UPDATE_TIMEOUT ) ;

         /*
            Only for catalog group info is empty, and the msg is end to
            catalog service, not shard service
         */
         INT32 _syncSend2PeerCatGroup( pmdEDUCB *cb, MsgHeader *msg,
                                       pmdEDUEvent &recvEvent,
                                       INT64 millsec = DC_UPDATE_TIMEOUT ) ;

         /*
            Process cat group response, this result will update to
            _pNodeMgrAgent.
         */
         INT32 _processCatGrpRes( pmdEDUCB *cb, MsgHeader *pRes,
                                  UINT32 groupID ) ;
         /*
            Process group query response, this result will update to
            _pNodeMgrAgent
         */
         INT32 _processGrpQueryRes( pmdEDUCB *cb, MsgHeader *pRes ) ;
         /*
            Process catalog query response, this result will update to
            _pCatAgent
         */
         INT32 _processCatQueryRes( pmdEDUCB *cb, MsgHeader *pRes ) ;
         /*
            Process dc base info query response, this result will update
            to pBaseInfo
         */
         INT32 _processDCBaseInfoQueryRes( pmdEDUCB *cb, MsgHeader *pRes,
                                           clsDCBaseInfo *pBaseInfo ) ;

         /*
            Send CAT_GRP_REQ to image catalog, and process the reply.
            If the error occurs, the memthod will retry again.
         */
         INT32 _updateImageGroup( pmdEDUCB *cb, MsgHeader *msg,
                                  UINT32 groupID,
                                  INT64 millsec = DC_UPDATE_TIMEOUT ) ;
         /*
            Send Group Query to image catalog, and recv reply
         */
         INT32 _queryOnImageCatalog( pmdEDUCB *cb,
                                     pmdEDUEvent &recvEvent,
                                     UINT32 opCode,
                                     const CHAR *pCollectionName = NULL,
                                     const BSONObj &cond = BSONObj(),
                                     const BSONObj &sel = BSONObj(),
                                     const BSONObj &orderby = BSONObj(),
                                     const BSONObj &hint = BSONObj(),
                                     INT64 returnNum = -1,
                                     INT64 skipNum = 0,
                                     INT64 millsec = DC_UPDATE_TIMEOUT ) ;

      private:
         _clsCatalogAgent              *_pCatAgent ;
         _clsNodeMgrAgent              *_pNodeMgrAgent ;
         BOOLEAN                       _init ;

         clsDCBaseInfo                 _baseInfo ;       // this dc base info
         clsDCBaseInfo                 _imageBaseInfo ;  // image dc base info
         vector< pmdAddrPair >         _vecCatlog ;      // the image address
         ossSpinXLatch                 _peerCatLatch ;
         INT32                         _peerCatPrimary ;
         UINT64                        _requestID ;

   } ;

   typedef _clsDCMgr clsDCMgr ;
}

#endif //DC_CLS_MGR_HPP__

