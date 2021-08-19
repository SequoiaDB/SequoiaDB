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

   Source File Name = catNodeManager.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/
#ifndef CATNODEMANAGER_HPP__
#define CATNODEMANAGER_HPP__

#include "pmd.hpp"
#include "netDef.hpp"
#include "rtnContextBuff.hpp"

using namespace bson ;

namespace engine
{
   class sdbCatalogueCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _SDB_DMSCB ;

   /*
      catNodeManager define
   */
   class catNodeManager : public SDBObject
   {
   public:
      catNodeManager() ;
      ~catNodeManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;

      INT32 processCmdCreateGrp( const CHAR *pQuery ) ;
      INT32 processCmdUpdateNode( const NET_HANDLE &handle,
                                  const CHAR *pQuery,
                                  const CHAR *pSelector ) ;

      INT32 processGrpReq( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processRegReq( const NET_HANDLE &handle, MsgHeader *pMsg ) ;
      INT32 processPrimaryChange( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 readCataConf();
      INT32 parseCatalogConf( CHAR *pData, const SINT64 sDataSize,
                              SINT64 &sParseBytes );
      INT32 parseLine( const CHAR *pLine, BSONObj &obj );
      INT32 generateGroupInfo( BSONObj &boConf,
                               BSONObj &boGroupInfo );
      INT32 saveGroupInfo ( BSONObj &boGroupInfo, INT16 w );
      INT32 parseIDInfo( BSONObj &obj );
      INT32 getNodeInfo( const BSONObj &boReq, BSONObj &boNodeInfo,
                         INT32 &role ) ;

      INT32 _checkAndUpdateNodeInfo( const BSONObj &reqObj,
                                     INT32 role,
                                     const BSONObj &nodeObj ) ;

      INT32 _loadGroupInfo() ;

   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

   // tool fuctions
   private:
      INT32 _createGrp( const CHAR *groupName ) ;
      INT32 _updateNodeToGrp ( BSONObj &boGroupInfo,
                               const BSONObj &boNodeInfoNew,
                               UINT16 nodeID,
                               BOOLEAN isLoalConn,
                               BOOLEAN setStatus,
                               BOOLEAN keepInstanceID ) ;
      INT32 _getRemovedGroupsObj( const BSONObj &srcGroupsObj,
                                  UINT16 removeNode,
                                  BSONObj &removedObj,
                                  BSONArrayBuilder &newObjBuilder ) ;
      INT16 _majoritySize() ;

      INT32 _getNodeInfoByConf( BSONObj &boConf, BSONObjBuilder &bobNodeInfo ) ;

   private:
      _SDB_DMSCB                 *_pDmsCB;
      _dpsLogWrapper             *_pDpsCB;
      _SDB_RTNCB                 *_pRtnCB;
      sdbCatalogueCB             *_pCatCB;
      pmdEDUCB                   *_pEduCB;

   } ;
}

#endif // CATNODEMANAGER_HPP__

