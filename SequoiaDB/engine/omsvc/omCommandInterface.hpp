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

   Source File Name = omCommandInterface.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_COMMANDINTERFACE_HPP_
#define OM_COMMANDINTERFACE_HPP_

#include "rtnCB.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "netDef.hpp"
#include "pmdRemoteSession.hpp"
#include "restAdaptor.hpp"
#include "pmdRestSession.hpp"
#include "omCommandTool.hpp"
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   class omCommandInterafce : public SDBObject
   {
      public:
         omCommandInterafce() ;
         virtual ~omCommandInterafce() ;

      public:
         virtual INT32     doCommand() = 0 ;
   } ;

   struct simpleDiskInfo: public SDBObject
   {
      string diskName ;
      string mountPath ;
      UINT64 totalSize ;
      UINT64 freeSize ;
   } ;

   struct simpleHostDisk : public SDBObject 
   {
      string hostName ;
      string user ;
      string passwd ;
      string agentPort ;
      list<simpleDiskInfo> diskInfo ;
   } ;

   struct simpleNetInfo : public SDBObject
   {
      string netName ;
      string ip ;
   } ;

   struct fullHostInfo : public SDBObject 
   {
      string hostName ;
      string user ;
      string passwd ;
      string agentPort ;
      list<simpleDiskInfo> diskInfo ;
      list<simpleNetInfo> netInfo ;
   } ;

   class omRestCommandBase : public omCommandInterafce
   {
      public:

         omRestCommandBase( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;

         virtual ~omRestCommandBase() ;

      public:
         virtual INT32     init( pmdEDUCB * cb ) ;
         virtual bool      isFetchAgentResponse( UINT64 requestID ) ;
         virtual INT32     doAgentResponse ( MsgHeader* pAgentResponse ) ;

      protected:
         INT32             _queryTable( const string &tableName, 
                                        const BSONObj &selector, 
                                        const BSONObj &matcher,
                                        const BSONObj &order, 
                                        const BSONObj &hint, SINT32 flag,
                                        SINT64 numSkip, SINT64 numReturn, 
                                        list<BSONObj> &records ) ;

         INT32             _getBusinessInfo( const string &business, 
                                             BSONObj &businessInfo ) ;
         INT32             _getBusinessInfoOfCluster( const string &clusterName,
                                                      BSONObj &clusterBusinessInfo ) ;
         INT32             _deleteHost( const string &hostName ) ;
         INT32             _getClusterInfo( const string &clusterName, 
                                            BSONObj &clusterInfo ) ;
         INT32             _getHostInfo( string hostName, 
                                         BSONObj &hostInfo ) ;
         INT32             _fetchHostDiskInfo( const string &clusterName, 
                                          list<string> &hostNameList, 
                                          list<simpleHostDisk> &hostInfoList ) ;
         INT32             _checkHostBasicContent( BSONObj &oneHost ) ;
         INT32             _getAllReplay( pmdRemoteSession *remoteSession, 
                                          VEC_SUB_SESSIONPTR *subSessionVec ) ;
         INT32             _receiveFromAgent( pmdRemoteSession *remoteSession,
                                              SINT32 &flag, BSONObj &result ) ;

         INT32             _getBusinessAuth( const string &businessName, 
                                             string &authUser, 
                                             string &authPasswd ) ;
         BOOLEAN           _isHostExistInTask( const string &hostName ) ;

         BOOLEAN           _isBusinessExistInTask( 
                                             const string &businessName ) ;

         INT32             _getBusinessType( const string &businessName ,
                                             string &businessType,
                                             string &deployMode ) ;

         BOOLEAN           _isClusterExist( const string &clusterName ) ;
         BOOLEAN           _isBusinessExist( const string &clusterName, 
                                             const string &businessName ) ;

         BOOLEAN           _isHostExistInCluster( const string &hostName,
                                                  const string &clusterName ) ;
         void              _sendOKRes2Web() ;
         void              _setOPResult( INT32 rc, const CHAR* detail ) ;
         void              _sendErrorRes2Web( INT32 rc, const CHAR* detail ) ;
         void              _sendErrorRes2Web( INT32 rc, const string &detail ) ;
         INT32             _parseIPsField( const CHAR *input,
                                           set< string > &IPs ) ;
         
      protected:
         SDB_RTNCB         *_pRTNCB ;
         SDB_DMSCB         *_pDMDCB ;
         pmdKRCB           *_pKRCB ;
         SDB_DMSCB         *_pDMSCB ;

         pmdEDUCB          *_cb ;

         string            _errorDetail ;
         restAdaptor*      _restAdaptor ;
         pmdRestSession*   _restSession ;

         omErrorTool       _errorMsg ;
   } ;

   class omAgentReqBase : public omCommandInterafce
   {
      public:
         omAgentReqBase( BSONObj &request ) ;
         virtual ~omAgentReqBase() ;

      public:
         void             getResponse( BSONObj &response ) ;

      protected:
         BSONObj          _request ;
         BSONObj          _response ;
   } ;

   const CHAR *omGetEDUInfoSafe( _pmdEDUCB *cb, EDU_INFO_TYPE type ) ;
   const CHAR *omGetMyEDUInfoSafe( EDU_INFO_TYPE type ) ;
}

#endif /* OM_COMMANDINTERFACE_HPP_ */


