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

   Source File Name = clsDCMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/02/2015  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsDCMgr.hpp"
#include "msgDef.h"
#include "msgMessage.hpp"
#include "msgCatalog.hpp"
#include "clsMgr.hpp"
#include "catDef.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define DC_UPDATE_FORCE_TIMEOUT           ( 60 * OSS_ONE_SEC )

   /*
      _clsDCBaseInfo implement
   */
   _clsDCBaseInfo::_clsDCBaseInfo()
   {
      _reset() ;
   }

   _clsDCBaseInfo::~_clsDCBaseInfo()
   {
   }

   void _clsDCBaseInfo::_reset()
   {
      _clusterName = "" ;
      _businessName = "" ;
      _address = "" ;
      _imageClusterName = "" ;
      _imageBusinessName = "" ;
      _imageAddress = "" ;
      _hasImage = FALSE ;
      _imageIsEnabled = FALSE ;
      _activated = TRUE ;
      _readonly = FALSE ;
      _hasCsUniqueHWM = FALSE ;
      _csUniqueHWM = 0 ;
      _catVersion = CATALOG_VERSION_CUR ;

      _orgObj = BSONObj() ;
      _imageGroups.clear() ;
      _imageRGroups.clear() ;

      _recycleBinConf.reset() ;
   }

   INT32 _clsDCBaseInfo::lock_r( INT32 millisec )
   {
      return _rwMutex.lock_r( millisec ) ;
   }

   INT32 _clsDCBaseInfo::release_r()
   {
      return _rwMutex.release_r() ;
   }

   INT32 _clsDCBaseInfo::lock_w( INT32 millisec )
   {
      return _rwMutex.lock_w( millisec ) ;
   }

   INT32 _clsDCBaseInfo::release_w()
   {
      return _rwMutex.release_w() ;
   }

   INT32 _clsDCBaseInfo::updateFromBSON( BSONObj &obj, BOOLEAN checkGroups )
   {
      INT32 rc = SDB_OK ;

      //reset
      _reset() ;

      // begin to update
      _orgObj = obj ;

      BSONObj subObj ;
      BSONElement subEle ;
      BSONElement e = obj.getField( FIELD_NAME_DATACENTER ) ;
      if ( Object == e.type() )
      {
         subObj = e.embeddedObject() ;
         subEle = subObj.getField( FIELD_NAME_CLUSTERNAME ) ;
         _clusterName = subEle.valuestrsafe() ;

         subEle = subObj.getField( FIELD_NAME_BUSINESSNAME ) ;
         _businessName = subEle.valuestrsafe() ;

         subEle = subObj.getField( FIELD_NAME_ADDRESS ) ;
         _address = subEle.valuestrsafe() ;
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_ACTIVATED ) ;
      if ( Bool == e.type() )
      {
         _activated = e.Bool() ? TRUE : FALSE ;
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_READONLY ) ;
      if ( Bool == e.type() )
      {
         _readonly = e.Bool() ? TRUE : FALSE ;
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      e = obj.getField( FIELD_NAME_CSUNIQUEHWM ) ;
      if ( e.eoo() )
      {
         _hasCsUniqueHWM = FALSE ;
      }
      else
      {
         if ( e.isNumber() )
         {
            _hasCsUniqueHWM = TRUE ;
            _csUniqueHWM = (utilCSUniqueID)e.numberInt() ;
         }
         else
         {
            goto error ;
         }
      }

      e = obj.getField( FIELD_NAME_CAT_VERSION ) ;
      if ( e.eoo() )
      {
         _catVersion = CATALOG_VERSION_V0 ;
      }
      else if ( e.isNumber() )
      {
         _catVersion = (UINT32)( e.numberInt() ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to parse field [%s]",
                 FIELD_NAME_CAT_VERSION ) ;
         goto error ;
      }

      e = obj.getField( FIELD_NAME_IMAGE ) ;
      if ( Object == e.type() )
      {
         subObj = e.embeddedObject() ;
         subEle = subObj.getField( FIELD_NAME_CLUSTERNAME ) ;
         _imageClusterName = subEle.valuestrsafe() ;

         subEle = subObj.getField( FIELD_NAME_BUSINESSNAME ) ;
         _imageBusinessName = subEle.valuestrsafe() ;

         subEle = subObj.getField( FIELD_NAME_ADDRESS ) ;
         _imageAddress = subEle.valuestrsafe() ;
         _hasImage = _imageAddress.empty() ? FALSE : TRUE ;

         subEle = subObj.getField( FIELD_NAME_ENABLE ) ;
         if ( Bool == subEle.type() )
         {
            _imageIsEnabled = subEle.boolean() ? TRUE : FALSE ;
         }
         else if ( !subEle.eoo() )
         {
            goto error ;
         }

         // image groups
         subEle = subObj.getField( FIELD_NAME_GROUPS ) ;
         if ( Array == subEle.type() )
         {
            BSONObj groupObj = subEle.embeddedObject() ;
            BSONObjIterator it( groupObj ) ;
            while ( it.more() )
            {
               BSONObj itemObj ;
               BSONElement itemEle = it.next() ;
               if ( Array != itemEle.type() )
               {
                  goto error ;
               }
               itemObj = itemEle.embeddedObject() ;
               if ( 2 != itemObj.nFields() )
               {
                  goto error ;
               }
               rc = _addGroup( itemObj, checkGroups ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
         else if ( !subEle.eoo() )
         {
            goto error ;
         }
      }
      else if ( !e.eoo() )
      {
         goto error ;
      }

      rc = _recycleBinConf.fromBSON( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update recycle info from BSON, "
                   "rc: %d", rc ) ;

   done:
      return rc ;
   error:
      PD_LOG( PDERROR, "Update base info failed, obj[%s] is invalid",
              obj.toString().c_str() ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      goto done ;
   }

   void _clsDCBaseInfo::setImageAddress( const string &addr )
   {
      _imageAddress = addr ;
      if ( _imageAddress.empty() )
      {
         _hasImage = FALSE ;
         _imageIsEnabled = FALSE ;
      }
      else
      {
         _hasImage = TRUE ;
      }
   }

   void _clsDCBaseInfo::setImageClusterName( const string &name )
   {
      _imageClusterName = name ;
   }

   void _clsDCBaseInfo::setImageBusinessName( const string &name )
   {
      _imageBusinessName = name ;
   }

   void _clsDCBaseInfo::enableImage( BOOLEAN enable )
   {
      _imageIsEnabled = enable ;
   }

   INT32 _clsDCBaseInfo::addGroups( const BSONObj &groups,
                                    map< string, string > *pAddMapGrps )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN added = FALSE ;
      const CHAR *pSource = NULL ;
      const CHAR *pImage = NULL ;
      BSONObjIterator it( groups ) ;
      while ( it.more() )
      {
         BSONObj itemObj ;
         BSONElement itemEle = it.next() ;
         if ( Array != itemEle.type() )
         {
            goto error ;
         }
         itemObj = itemEle.embeddedObject() ;
         if ( 2 != itemObj.nFields() )
         {
            goto error ;
         }
         rc = _addGroup( itemObj, TRUE, &added, &pSource, &pImage ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( pAddMapGrps )
         {
            (*pAddMapGrps)[ string( pSource ) ] = string( pImage ) ;
         }
      }

   done:
      return rc ;
   error:
      PD_LOG( PDERROR, "Add groups failed, obj[%s] is invalid",
              groups.toString().c_str() ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      goto done ;
   }

   INT32 _clsDCBaseInfo::addGroup( const string &source,
                                   const string &image,
                                   BOOLEAN *pAdded )
   {
      INT32 rc = SDB_OK ;
      map< string, string >::iterator it ;

      if ( source.empty() || image.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( pAdded )
      {
         *pAdded = FALSE ;
      }

      it = _imageGroups.find( source ) ;
      if ( it != _imageGroups.end() )
      {
         // already exist
         if ( it->second == image )
         {
            goto done ;
         }
         PD_LOG( PDERROR, "Source group[%s] conflict", source.c_str() ) ;
         rc = SDB_CAT_GRP_EXIST ;
         goto error ;
      }
      it = _imageRGroups.find( image ) ;
      if ( it != _imageRGroups.end() )
      {
         // already exist
         if ( it->second == source )
         {
            goto done ;
         }
         PD_LOG( PDERROR, "Image group[%s] conflict", image.c_str() ) ;
         rc = SDB_CAT_GRP_EXIST ;
         goto error ;
      }

      _imageGroups[ source ] = image ;
      _imageRGroups[ image ] = source ;
      if ( pAdded )
      {
         *pAdded = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCBaseInfo::delGroup( const string &source, string &image )
   {
      INT32 rc = SDB_OK ;
      map< string, string >::iterator it ;

      if ( source.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      it = _imageGroups.find( source ) ;
      if ( it == _imageGroups.end() )
      {
         PD_LOG( PDERROR, "Source group[%s] does not exist", source.c_str() ) ;
         rc = SDB_CLS_GRP_NOT_EXIST ;
         goto error ;
      }
      image = it->second ;

      _imageRGroups.erase( it->second ) ;
      _imageGroups.erase( it ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCBaseInfo::delGroups( const BSONObj &groups,
                                    map< string, string > *pDelMapGrps )
   {
      INT32 rc = SDB_OK ;
      string source ;
      string image ;

      BSONObjIterator it( groups ) ;
      while ( it.more() )
      {
         BSONObj itemObj ;
         BSONElement itemEle = it.next() ;
         if ( Array != itemEle.type() )
         {
            goto error ;
         }
         itemObj = itemEle.embeddedObject() ;
         if ( 2 != itemObj.nFields() )
         {
            goto error ;
         }
         rc = _delGroup( itemObj, &source, &image ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( pDelMapGrps )
         {
            (*pDelMapGrps)[ source ] = image ;
         }
      }

   done:
      return rc ;
   error:
      PD_LOG( PDERROR, "Del groups failed, obj[%s] is invalid",
              groups.toString().c_str() ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      goto done ;
   }

   void _clsDCBaseInfo::setClusterName( const string &name )
   {
      _clusterName = name ;
   }

   void _clsDCBaseInfo::setBusinessName( const string &name )
   {
      _businessName = name ;
   }

   void _clsDCBaseInfo::setAddress( const string &addr )
   {
      _address = addr ;
   }

   void _clsDCBaseInfo::setAcitvated( BOOLEAN activated )
   {
      _activated = activated ;
   }

   void _clsDCBaseInfo::setReadonly( BOOLEAN readonly )
   {
      _readonly = readonly ;
   }

   INT32 _clsDCBaseInfo::_addGroup( const BSONObj &obj, BOOLEAN check,
                                    BOOLEAN *pAdded,
                                    const CHAR **ppSource,
                                    const CHAR **ppImage )
   {
      INT32 rc = SDB_OK ;
      string sourceGroup ;
      string destGroup ;
      BSONElement s = obj.getField( "0" ) ;
      BSONElement d = obj.getField( "1" ) ;
      if ( String != s.type() || String != d.type() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( pAdded )
      {
         *pAdded = FALSE ;
      }
      sourceGroup = s.valuestr() ;
      destGroup = d.valuestr() ;
      if ( ppSource )
      {
         *ppSource = s.valuestr() ;
      }
      if ( ppImage )
      {
         *ppImage = d.valuestr() ;
      }

      if ( sourceGroup.empty() || destGroup.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( check )
      {
         map< string, string >::iterator it = _imageGroups.find( sourceGroup ) ;
         if ( it != _imageGroups.end() )
         {
            if ( it->second == destGroup )
            {
               goto done ;
            }
            PD_LOG( PDERROR, "Source group[%s] conflict",
                    sourceGroup.c_str() ) ;
            rc = SDB_CAT_GRP_EXIST ;
            goto error ;
         }
         it = _imageRGroups.find( destGroup ) ;
         if ( it != _imageRGroups.end() )
         {
            if ( it->second == sourceGroup )
            {
               goto done ;
            }
            PD_LOG( PDERROR, "Image group[%s] conflict",
                    destGroup.c_str() ) ;
            rc = SDB_CAT_GRP_EXIST ;
            goto error ;
         }
      }
      _imageGroups[ sourceGroup ] = destGroup ;
      _imageRGroups[ destGroup ] = sourceGroup ;
      if ( pAdded )
      {
         *pAdded = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCBaseInfo::_delGroup( const BSONObj &obj,
                                    string *pSourceName,
                                    string *pImageName )
   {
      INT32 rc = SDB_OK ;
      map< string, string >::iterator it ;
      string sourceGroup ;
      BSONElement s = obj.getField( "0" ) ;
      BSONElement d = obj.getField( "1" ) ;
      if ( String != s.type() || String != d.type() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      sourceGroup = s.valuestr() ;
      if ( sourceGroup.empty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      it = _imageGroups.find( sourceGroup ) ;
      if ( it == _imageGroups.end() )
      {
         PD_LOG( PDERROR, "Source group[%s] does not exist",
                 sourceGroup.c_str() ) ;
         rc = SDB_CLS_GRP_NOT_EXIST ;
         goto error ;
      }

      if ( pSourceName )
      {
         *pSourceName = sourceGroup ;
      }
      if ( pImageName )
      {
         *pImageName = it->second ;
      }

      _imageRGroups.erase( it->second ) ;
      _imageGroups.erase( it ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _clsDCBaseInfo::getClusterName() const
   {
      return _clusterName.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getBusinessName() const
   {
      return _businessName.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getAddress() const
   {
      return _address.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getImageClusterName() const
   {
      return _imageClusterName.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getImageBusinessName() const
   {
      return _imageBusinessName.c_str() ;
   }

   const CHAR* _clsDCBaseInfo::getImageAddress() const
   {
      return _imageAddress.c_str() ;
   }

   string _clsDCBaseInfo::source2dest( const string &sourceGroup )
   {
      map< string, string >::iterator it = _imageGroups.find( sourceGroup ) ;
      if ( it != _imageGroups.end() )
      {
         return it->second ;
      }
      return "" ;
   }

   string _clsDCBaseInfo::dest2source( const string &destGroup )
   {
      map< string, string >::iterator it = _imageRGroups.find( destGroup ) ;
      if ( it != _imageRGroups.end() )
      {
         return it->second ;
      }
      return "" ;
   }

   /*
      _clsDCMgr implement
   */
   _clsDCMgr::_clsDCMgr ()
   {
      _requestID = 0 ;
      _pCatAgent = NULL ;
      _pNodeMgrAgent = NULL ;
      _init = FALSE ;

      _peerCatPrimary = -1 ;
   }

   _clsDCMgr::~_clsDCMgr()
   {
      SAFE_DELETE ( _pCatAgent ) ;
      SAFE_DELETE ( _pNodeMgrAgent ) ;
   }

   INT32 _clsDCMgr::initialize()
   {
      INT32 rc = SDB_OK ;

      if ( _init )
      {
         // already init
         goto done ;
      }

      SAFE_NEW_GOTO_ERROR( _pCatAgent, _clsCatalogAgent ) ;
      SAFE_NEW_GOTO_ERROR( _pNodeMgrAgent, _clsNodeMgrAgent ) ;

      _init = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_syncSend2PeerNode( pmdEDUCB *cb, MsgHeader *msg,
                                        pmdEDUEvent *pRecvEvent,
                                        const pmdAddrPair &node,
                                        INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      UINT16 port = 0 ;

      SDB_ASSERT( cb && msg, "Handle can't be NULL" ) ;

      rc = ossGetPort( node._service, port ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid svcname: %s", node._service ) ;

      {
         ossSocket sock( node._host, port, millsec ) ;
         rc = sock.initSocket() ;
         PD_RC_CHECK( rc, PDERROR, "Init socket[%s:%d] failed, rc: %d",
                      node._host, port, rc ) ;

         rc = sock.connect( (INT32)millsec ) ;
         PD_RC_CHECK( rc, PDWARNING, "Connect to %s:%d failed, rc: %d",
                      node._host, port, rc ) ;

         sock.disableNagle() ;
         // set keep alive
         sock.setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                            OSS_SOCKET_KEEP_INTERVAL,
                            OSS_SOCKET_KEEP_CONTER ) ;

         if ( pRecvEvent )
         {
            rc = pmdSyncSendMsg( msg, *pRecvEvent, &sock, cb,
                                 (INT32)millsec, DC_UPDATE_FORCE_TIMEOUT ) ;
         }
         else
         {
            rc = pmdSend( (const CHAR*)msg, msg->messageLength, &sock, cb,
                          (INT32)millsec ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_syncSend2PeerCatGroup( pmdEDUCB *cb, MsgHeader *msg,
                                            pmdEDUEvent &recvEvent,
                                            INT64 millsec )
   {
      INT32 rc = SDB_CLS_NO_CATALOG_INFO ;
      BOOLEAN hasLock = TRUE ;

      _peerCatLatch.get() ;
      hasLock = TRUE ;

      // first send to primary node
      if ( _peerCatPrimary >= 0 &&
           ( UINT32 )_peerCatPrimary < _vecCatlog.size() )
      {
         rc = _syncSend2PeerNode( cb, msg, &recvEvent,
                                  _vecCatlog[ _peerCatPrimary ],
                                  millsec ) ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }
         _peerCatPrimary = -1 ;
      }
      // then send to any other node
      for ( UINT32 i = 0 ; i < _vecCatlog.size() ; ++i )
      {
         rc = _syncSend2PeerNode( cb, msg, &recvEvent, _vecCatlog[ i ],
                                  millsec ) ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }
      }

   done:
      if ( hasLock )
      {
         _peerCatLatch.release() ;
      }
      return rc ;
   }

   clsDCBaseInfo* _clsDCMgr::getImageDCBaseInfo( pmdEDUCB *cb, BOOLEAN update )
   {
      if ( update || _imageBaseInfo.getOrgObj().isEmpty() )
      {
         updateImageDCBaseInfo( cb, DC_UPDATE_TIMEOUT ) ;
      }
      return &_imageBaseInfo ;
   }

   INT32 _clsDCMgr::updateDCBaseInfo( BSONObj &obj )
   {
      BOOLEAN hasLock = FALSE ;
      INT32 rc = SDB_OK ;

      rc = _baseInfo.lock_w() ;
      PD_RC_CHECK( rc, PDERROR, "Lock dc base info write failed, rc: %d",
                   rc ) ;
      hasLock = TRUE ;

      rc = _baseInfo.updateFromBSON( obj, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }
      // if has image, parse image address
      if ( _baseInfo.hasImage() )
      {
         ossScopedLock lock( &_peerCatLatch ) ;
         pmdOptionsCB option ;
         rc = option.parseAddressLine( _baseInfo.getImageAddress(),
                                       _vecCatlog ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Parse image address[%s] failed, rc: %d",
                    _baseInfo.getImageAddress(), rc ) ;
            goto error ;
         }
      }

   done:
      if ( hasLock )
      {
         _baseInfo.release_w() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateDCBaseInfo( MsgOpReply* pRes )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;
      BSONObj objInfo ;

      rc = msgExtractReply( (CHAR *)pRes, &flag, &contextID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query reply msg failed, rc: %d",
                   rc ) ;

      rc = flag ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING, "Recieve failed dc base info query reply, "
                 "flag: %d", rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < objList.size() ; ++i )
      {
         objInfo = objList[ i ] ;
         break ;
      }
      rc = updateDCBaseInfo( objInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Update dc base info failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::setImageCatAddr( const string &catAddr )
   {
      INT32 rc = SDB_OK ;
      pmdOptionsCB option ;
      vector< pmdAddrPair > tmpVecCat ;

      rc = option.parseAddressLine( catAddr.c_str(), tmpVecCat ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Parse image address[%s] failed, rc: %d",
                 catAddr.c_str(), rc ) ;
         goto error ;
      }
      if ( 0 == tmpVecCat.size() )
      {
         PD_LOG( PDERROR, "Image catalog address[%s] is invalid",
                 catAddr.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _peerCatLatch.get() ;
      _vecCatlog = tmpVecCat ;
      _peerCatLatch.release() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   string _clsDCMgr::getImageCatAddr()
   {
      pmdOptionsCB option ;
      ossScopedLock lock( &_peerCatLatch ) ;
      return option.makeAddressLine( _vecCatlog ) ;
   }

   vector< pmdAddrPair > _clsDCMgr::getImageCatVec()
   {
      ossScopedLock lock( &_peerCatLatch ) ;
      return _vecCatlog ;
   }

   INT32 _clsDCMgr::updateImageCataGroup( pmdEDUCB *cb, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      MsgCatCatGroupReq req ;

      vector< pmdAddrPair > vecCatalog ;
      INT32 tmpPrimary = -1 ;
      clsGroupItem *pCatItem = NULL ;
      UINT32 pos = 0 ;
      MsgRouteID id ;
      string hostname ;
      string svcname ;

      req.id.value = MSG_INVALID_ROUTEID ;
      req.id.columns.groupID = CATALOG_GROUPID ;

      rc = _updateImageGroup( cb, &(req.header), CATALOG_GROUPID, millsec ) ;
      if ( rc )
      {
         goto error ;
      }

      _pNodeMgrAgent->lock_r() ;
      pCatItem = _pNodeMgrAgent->groupItem( CATALOG_GROUPID ) ;
      if ( !pCatItem )
      {
         PD_LOG( PDERROR, "Not found image catalog group" ) ;
         rc = SDB_CLS_GRP_NOT_EXIST ;
         _pNodeMgrAgent->release_r() ;
         goto error ;
      }

      // need to update _vecCatlog
      while ( TRUE )
      {
         if ( SDB_OK != pCatItem->getNodeInfo( pos++, id, hostname, svcname,
                                               MSG_ROUTE_CAT_SERVICE ) )
         {
            break ;
         }
         vecCatalog.push_back( pmdAddrPair( hostname, svcname ) ) ;
      }
      tmpPrimary = (INT32)pCatItem->getPrimaryPos() ;
      // release lock
      _pNodeMgrAgent->release_r() ;

      _peerCatLatch.get() ;
      _vecCatlog.clear() ;
      _vecCatlog = vecCatalog ;
      _peerCatPrimary = tmpPrimary ;
      _peerCatLatch.release() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_updateImageGroup( pmdEDUCB *cb, MsgHeader *msg,
                                       UINT32 groupID, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      pmdEDUEvent recvEvent ;
      BOOLEAN hasRetry = ( CATALOG_GROUPID == groupID ) ? TRUE : FALSE ;

   retry:
      msg->requestID = ++_requestID ;
      msg->TID = cb->getTID() ;

      rc = _syncSend2PeerCatGroup( cb, msg, recvEvent, millsec ) ;
      if ( rc )
      {
         if ( !hasRetry )
         {
            hasRetry = TRUE ;
            if ( SDB_OK == updateImageCataGroup( cb, millsec ) )
            {
               goto retry ;
            }
         }
         PD_LOG( PDERROR, "Send msg CAT_GRP_REQ to image catalog failed, "
                 "rc: %d", rc ) ;
         goto error ;
      }
      rc = _processCatGrpRes( cb, (MsgHeader*)recvEvent._Data, groupID ) ;
      if ( rc )
      {
         if ( SDB_CLS_NOT_PRIMARY == rc && !hasRetry )
         {
            hasRetry = TRUE ;
            if ( SDB_OK == updateImageCataGroup( cb, millsec ) )
            {
               pmdEduEventRelease( recvEvent, cb ) ;
               goto retry ;
            }
         }
         PD_LOG( PDERROR, "Process CAT_GRP_RES failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      pmdEduEventRelease( recvEvent, cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_processCatGrpRes( pmdEDUCB *cb, MsgHeader *pRes,
                                       UINT32 groupID )
   {
      INT32 rc = SDB_OK ;

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(pRes) )
      {
         INT32 flags = MSG_GET_INNER_REPLY_RC(pRes) ;
         // let's check if response shows unable to find group
         if ( SDB_CLS_GRP_NOT_EXIST == flags ||
              SDB_DMS_EOC == flags )
         {
            // in that case, let's clear local group cache information
            _pNodeMgrAgent->lock_w() ;
            _pNodeMgrAgent->clearGroup( groupID ) ;
            _pNodeMgrAgent->release_w() ;
         }
         else
         {
            rc = MSG_GET_INNER_REPLY_RC(pRes) ;
            PD_LOG( PDWARNING, "Update group[%d] failed[rc:%d]",
                    groupID, rc ) ;
            goto error ;
         }
      }
      else
      {
         _pNodeMgrAgent->lock_w() ;

         const CHAR* objdata = MSG_GET_INNER_REPLY_DATA(pRes) ;
         UINT32 length = pRes->messageLength -
                         MSG_GET_INNER_REPLY_HEADER_LEN(pRes) ;
         UINT32 tmpGroupID = 0 ;

         rc = _pNodeMgrAgent->updateGroupInfo( objdata, length, &tmpGroupID ) ;
         PD_LOG ( ( (SDB_OK == rc) ? PDEVENT : PDERROR ),
                  "Update group[groupID:%u, rc: %d]", groupID, rc ) ;

         _pNodeMgrAgent->release_w() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_processGrpQueryRes( pmdEDUCB *cb, MsgHeader *pRes )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;
      UINT32 groupID = 0 ;

      rc = msgExtractReply( (CHAR *)pRes, &flag, &contextID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query reply msg failed, rc: %d",
                   rc ) ;

      rc = flag ;
      if ( SDB_DMS_EOC == rc || SDB_CLS_GRP_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING, "Recieve failed group query reply, flag: %d",
                 rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < objList.size() ; ++i )
      {
         _pNodeMgrAgent->lock_w() ;
         rc = _pNodeMgrAgent->updateGroupInfo( objList[i].objdata(),
                                               objList[i].objsize(),
                                               &groupID ) ;
         _pNodeMgrAgent->release_w() ;

         if ( rc )
         {
            PD_LOG( PDERROR, "Update group[%s] failed, rc: %d",
                    objList[i].toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_processCatQueryRes( pmdEDUCB *cb, MsgHeader *pRes )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;

      rc = msgExtractReply( (CHAR *)pRes, &flag, &contextID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query reply msg failed, rc: %d",
                   rc ) ;

      rc = flag ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING, "Recieve failed catalog query reply, flag: %d",
                 rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < objList.size() ; ++i )
      {
         _pCatAgent->lock_w() ;
         rc = _pCatAgent->updateCatalog( 0, 0, objList[i].objdata(),
                                         objList[i].objsize(), NULL ) ;
         _pCatAgent->release_w() ;

         if ( rc )
         {
            PD_LOG( PDERROR, "Update catalog[%s] failed, rc: %d",
                    objList[i].toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_processDCBaseInfoQueryRes( pmdEDUCB *cb, MsgHeader *pRes,
                                                clsDCBaseInfo *pBaseInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      INT64 contextID = 0 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector< BSONObj > objList ;
      BSONObj objInfo ;

      rc = msgExtractReply( (CHAR *)pRes, &flag, &contextID, &startFrom,
                            &numReturned, objList ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query reply msg failed, rc: %d",
                   rc ) ;

      rc = flag ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING, "Recieve failed DC base info query reply from "
                 "image, flag: %d", rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < objList.size() ; ++i )
      {
         objInfo = objList[ i ] ;
         break ;
      }

      // update dc base info
      if ( SDB_OK == pBaseInfo->lock_w() )
      {
         rc = pBaseInfo->updateFromBSON( objInfo, FALSE ) ;
         pBaseInfo->release_w() ;
         PD_RC_CHECK( rc, PDERROR, "Update dc base info failed, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateImageGroup( UINT32 groupID, pmdEDUCB *cb,
                                      INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      MsgCatCatGroupReq req ;

      if ( CATALOG_GROUPID == groupID )
      {
         rc = updateImageCataGroup( cb ) ;
         goto done ;
      }

      req.id.value = MSG_INVALID_ROUTEID ;
      req.id.columns.groupID = groupID ;

      rc = _updateImageGroup( cb, &(req.header), groupID, millsec ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateImageGroup( const CHAR *groupName, pmdEDUCB *cb,
                                      INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      MsgCatCatGroupReq *pReq = NULL ;
      CHAR *pBuff = NULL ;
      UINT32 msgSize = 0 ;
      UINT32 groupID = 0 ;

      SDB_ASSERT( groupName, "Group name can't be NULL" ) ;

      if ( 0 == ossStrcmp( groupName, CATALOG_GROUPNAME ) )
      {
         rc = updateImageCataGroup( cb, millsec ) ;
         goto done ;
      }

      // try go get group id
      _pNodeMgrAgent->lock_r() ;
      _pNodeMgrAgent->groupName2ID( groupName, groupID ) ;
      _pNodeMgrAgent->release_r() ;

      msgSize = ossStrlen( groupName ) + 1 ;
      msgSize += sizeof( MsgCatGroupReq ) ;

      // alloc buff
      rc = cb->allocBuff( msgSize, &pBuff, NULL ) ;
      if ( rc )
      {
         goto error ;
      }
      pReq = ( MsgCatGroupReq * )pBuff ;

      pReq->id.value = 0 ;
      pReq->header.messageLength = msgSize ;
      pReq->header.opCode = MSG_CAT_GRP_REQ ;
      pReq->header.routeID.value = 0 ;
      ossMemcpy( pBuff + sizeof( MsgCatGroupReq ),
                 groupName, ossStrlen( groupName ) ) ;

      rc = _updateImageGroup( cb, &(pReq->header), groupID, millsec ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateImageAllGroups( pmdEDUCB *cb, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      pmdEDUEvent recvEvent ;
      BOOLEAN hasRetry = FALSE ;
      BSONObj hint ;
      BSONObj cond = BSON( FIELD_NAME_ROLE <<
                           BSON( "$ne" << SDB_ROLE_COORD ) ) ;

   retry:
      rc = _queryOnImageCatalog( cb, recvEvent, MSG_BS_QUERY_REQ,
                                 CAT_NODE_INFO_COLLECTION, cond,
                                 hint, hint, hint, -1, 0,
                                 DC_UPDATE_TIMEOUT ) ;
      if ( rc )
      {
         goto error ;
      }

      // clear all groups
      _pNodeMgrAgent->lock_w() ;
      _pNodeMgrAgent->clearAll() ;
      _pNodeMgrAgent->release_w() ;

      // process query result
      rc = _processGrpQueryRes( cb, (MsgHeader*)recvEvent._Data ) ;
      if ( rc )
      {
         if ( SDB_CLS_NOT_PRIMARY == rc && !hasRetry )
         {
            hasRetry = TRUE ;
            if ( SDB_OK == updateImageCataGroup( cb, millsec ) )
            {
               pmdEduEventRelease( recvEvent, cb ) ;
               goto retry ;
            }
         }
         goto error ;
      }

   done:
      pmdEduEventRelease( recvEvent, cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateImageAllCatalog( pmdEDUCB *cb, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      pmdEDUEvent recvEvent ;
      BOOLEAN hasRetry = FALSE ;
      BSONObj hint ;

   retry:
      rc = _queryOnImageCatalog( cb, recvEvent, MSG_BS_QUERY_REQ,
                                 CAT_COLLECTION_INFO_COLLECTION,
                                 hint, hint, hint, hint, -1, 0,
                                 DC_UPDATE_TIMEOUT ) ;
      if ( rc )
      {
         goto error ;
      }

      // clear all groups
      _pCatAgent->lock_r() ;
      _pCatAgent->clearAll() ;
      _pCatAgent->release_r() ;

      // process query result
      rc = _processCatQueryRes( cb, (MsgHeader*)recvEvent._Data ) ;
      if ( rc )
      {
         if ( SDB_CLS_NOT_PRIMARY == rc && !hasRetry )
         {
            hasRetry = TRUE ;
            if ( SDB_OK == updateImageCataGroup( cb, millsec ) )
            {
               pmdEduEventRelease( recvEvent, cb ) ;
               goto retry ;
            }
         }
         goto error ;
      }

   done:
      pmdEduEventRelease( recvEvent, cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::updateImageDCBaseInfo( pmdEDUCB *cb, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      pmdEDUEvent recvEvent ;
      BOOLEAN hasRetry = FALSE ;
      BSONObj hint ;
      BSONObj cond = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;

   retry:
      rc = _queryOnImageCatalog( cb, recvEvent, MSG_BS_QUERY_REQ,
                                 CAT_SYSDCBASE_COLLECTION_NAME,
                                 cond, hint, hint, hint, -1, 0,
                                 DC_UPDATE_TIMEOUT ) ;
      if ( rc )
      {
         goto error ;
      }

      // process query result
      rc = _processDCBaseInfoQueryRes( cb, (MsgHeader*)recvEvent._Data,
                                       &_imageBaseInfo ) ;
      if ( rc )
      {
         if ( SDB_CLS_NOT_PRIMARY == rc && !hasRetry )
         {
            hasRetry = TRUE ;
            if ( SDB_OK == updateImageCataGroup( cb, millsec ) )
            {
               pmdEduEventRelease( recvEvent, cb ) ;
               goto retry ;
            }
         }
         goto error ;
      }

   done:
      pmdEduEventRelease( recvEvent, cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::_queryOnImageCatalog( pmdEDUCB *cb, pmdEDUEvent &recvEvent,
                                          UINT32 opCode,
                                          const CHAR *pCollectionName,
                                          const BSONObj &cond,
                                          const BSONObj &sel,
                                          const BSONObj &orderby,
                                          const BSONObj &hint, INT64 returnNum,
                                          INT64 skipNum, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *pMsg = NULL ;

      BOOLEAN hasRetry = FALSE ;

      rc = msgBuildQueryMsg( &pBuff, &buffSize, pCollectionName,
                             FLG_QUERY_WITH_RETURNDATA, ++_requestID,
                             skipNum, returnNum, &cond, &sel, &orderby,
                             &hint ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query msg failed, rc: %d", rc ) ;

      pMsg = ( MsgHeader* )pBuff ;
      pMsg->opCode = opCode ;

   retry:
      // send msg
      rc = _syncSend2PeerCatGroup( cb, pMsg, recvEvent, millsec ) ;
      if ( rc )
      {
         if ( !hasRetry )
         {
            hasRetry = TRUE ;
            if ( SDB_OK == updateImageCataGroup( cb, millsec ) )
            {
               goto retry ;
            }
         }
         PD_LOG( PDWARNING, "Send group query msg to image catalog failed, "
                 "rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
         pBuff = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   catAgent* _clsDCMgr::getImageCataAgent ()
   {
      return _pCatAgent ;
   }

   nodeMgrAgent* _clsDCMgr::getImageNodeMgrAgent ()
   {
      return _pNodeMgrAgent ;
   }

   INT32 _clsDCMgr::getAndLockImageCataSet( const CHAR * name,
                                            pmdEDUCB *cb,
                                            clsCatalogSet **ppSet,
                                            BOOLEAN noWithUpdate,
                                            INT64 waitMillSec,
                                            BOOLEAN * pUpdated )
   {
      INT32 rc = SDB_OK ;
      // sanity check
      SDB_ASSERT ( ppSet && name,
                   "ppSet and name can't be NULL" ) ;

      while ( SDB_OK == rc )
      {
         _pCatAgent->lock_r() ;
         *ppSet = _pCatAgent->collectionSet( name ) ;
         // if we can't find the name and request to update catalog
         // we'll call syncUpdateCatalog and refind again
         if ( !(*ppSet) && noWithUpdate )
         {
            _pCatAgent->release_r() ;
            // request to update catalog
            rc = updateImageAllCatalog( cb, waitMillSec ) ;
            if ( rc )
            {
               // if we can't find the collection and not able to update
               // catalog, we'll return the error of synUpdateCatalog
               // call
               PD_LOG ( PDERROR, "Failed to sync update catalog, rc = %d",
                        rc ) ;
               goto error ;
            }
            if ( pUpdated )
            {
               *pUpdated = TRUE ;
            }
            // we don't want to update again
            noWithUpdate = FALSE ;
            continue ;
         }
         // if still not able to find it
         if ( !(*ppSet) )
         {
            _pCatAgent->release_r() ;
            rc = SDB_CLS_NO_CATALOG_INFO ;
         }
         break ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsDCMgr::unlockImageCataSet( clsCatalogSet * catSet )
   {
      if ( catSet )
      {
         _pCatAgent->release_r() ;
      }
      return SDB_OK ;
   }

   INT32 _clsDCMgr::getAndLockImageGroupItem( UINT32 id,
                                              pmdEDUCB *cb,
                                              clsGroupItem **ppItem,
                                              BOOLEAN noWithUpdate,
                                              INT64 waitMillSec,
                                              BOOLEAN * pUpdated )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( ppItem, "ppItem can't be NULL" ) ;

      while ( SDB_OK == rc )
      {
         _pNodeMgrAgent->lock_r() ;
         *ppItem = _pNodeMgrAgent->groupItem( id ) ;
         // can we find the group from local cache?
         if ( !(*ppItem) && noWithUpdate )
         {
            _pNodeMgrAgent->release_r() ;
            // if we can't find such group and is okay to update cache
            // we'll update cache from catalog
            rc = updateImageGroup( id, cb, waitMillSec ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to sync update group info, rc = %d",
                        rc ) ;
               goto error ;
            }
            if ( pUpdated )
            {
               *pUpdated = TRUE ;
            }
            // only update it once
            noWithUpdate = FALSE ;
            continue ;
         }
         // if we are not able to find one, let's return group not found
         if ( !(*ppItem) )
         {
            _pNodeMgrAgent->release_r() ;
            rc = SDB_CLS_NO_GROUP_INFO ;
         }
         break ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsDCMgr::unlockImageGroupItem( clsGroupItem * item )
   {
      if ( item )
      {
         _pNodeMgrAgent->release_r() ;
      }
      return SDB_OK ;
   }

   INT32 _clsDCMgr::syncSend2ImageNode( MsgHeader *msg,
                                        pmdEDUCB *cb,
                                        UINT32 groupID,
                                        vector< pmdEDUEvent > &vecRecv,
                                        vector<pmdAddrPair> &vecSendNode,
                                        INT32 selType,
                                        SEND_STRATEGY sendSty,
                                        MSG_ROUTE_SERVICE_TYPE type,
                                        INT64 millisecond,
                                        vector< pmdAddrPair > *pVecFailedNode )
   {
      INT32 rc = SDB_OK ;
      MsgRouteID nodeID ;
      BOOLEAN hasLock = FALSE ;
      clsGroupItem *groupItem = NULL ;
      vector< pmdAddrPair > nodes ;
      string hostname ;
      string svcname ;
      pmdEDUEvent event ;

      // 1. get the nodes
      _pNodeMgrAgent->lock_r() ;
      hasLock = TRUE ;
      groupItem = _pNodeMgrAgent->groupItem( groupID ) ;
      if ( NULL == groupItem )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
         goto error ;
      }
      else if ( 0 == groupItem->nodeCount() )
      {
         rc = SDB_CLS_EMPTY_GROUP ;
         goto error ;
      }

      if ( SDB_OK == groupItem->getNodeInfo( groupItem->getPrimaryPos(),
                                             nodeID, hostname,
                                             svcname, type ) )
      {
         nodes.push_back( pmdAddrPair( hostname, svcname ) ) ;
      }
      if ( selType & SEL_NODE_SLAVE )
      {
         UINT32 pos = 0 ;
         while( SDB_OK == groupItem->getNodeInfo( pos, nodeID, hostname,
                                                  svcname, type ) )
         {
            ++pos ;
            if ( pos == groupItem->getPrimaryPos() )
            {
               continue ;
            }
            nodes.push_back( pmdAddrPair( hostname, svcname ) ) ;
         }
      }
      _pNodeMgrAgent->release_r() ;
      hasLock = FALSE ;

      if ( 0 == nodes.size() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }

      // 2. send to nodes
      for ( UINT32 i = 0 ; i < nodes.size() ; ++i )
      {
         rc = _syncSend2PeerNode( cb, msg, &event, nodes[ i ], millisecond ) ;
         if ( SDB_OK == rc )
         {
            vecRecv.push_back( event ) ;
            vecSendNode.push_back( nodes[ i ] ) ;
            if ( SEND_NODE_ONE == sendSty )
            {
               break ;
            }
         }
         else if ( pVecFailedNode )
         {
            pVecFailedNode->push_back( nodes[ i ] ) ;
         }
      }

   done:
      if ( hasLock )
      {
         _pNodeMgrAgent->release_r() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDCMgr::syncSend2ImageNodes( MsgHeader *msg, pmdEDUCB *cb,
                                         vector< pmdEDUEvent > &vecRecv,
                                         vector< pmdAddrPair > &vecSendNode,
                                         INT32 selType, SEND_STRATEGY sendSty,
                                         INT64 millisecond,
                                         vector< pmdAddrPair > *pVecFailedNode )
   {
      INT32 rc = SDB_OK ;
      VEC_UINT32 groups ;

      rc = updateImageAllGroups( cb, millisecond ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update image all groups failed, rc: %d", rc ) ;
         goto error ;
      }

      // get all groups
      _pNodeMgrAgent->lock_r() ;
      _pNodeMgrAgent->getGroupsID( groups ) ;
      _pNodeMgrAgent->release_r() ;

      for ( UINT32 i = 0 ; i < groups.size() ; ++i )
      {
         rc = syncSend2ImageNode( msg, cb, groups[ i ], vecRecv, vecSendNode,
                                  selType, sendSty, MSG_ROUTE_SHARD_SERVCIE,
                                  millisecond, pVecFailedNode ) ;
         if ( SDB_CLS_EMPTY_GROUP == rc )
         {
            continue ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Send msg to image group[%d] failed, rc: %d",
                    groups[ i ], rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   utilRecycleBinConf _clsDCBaseInfo::getRecycleBinConf()
   {
      ossScopedRWLock _lock( &_rwMutex, SHARED ) ;
      return _recycleBinConf ;
   }

   void _clsDCBaseInfo::setRecycleBinConf( const utilRecycleBinConf &conf )
   {
      ossScopedRWLock _lock( &_rwMutex, EXCLUSIVE ) ;
      _recycleBinConf = conf ;
   }

}

