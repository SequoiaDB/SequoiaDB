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

   #define OMREST_CLASS_PARAMETER pmdRestSession *pRestSession,\
                                  restAdaptor *pRestAdaptor,\
                                  restRequest *pRequest,\
                                  restResponse *pResponse,\
                                  const string &localAgentHost,\
                                  const string &localAgentService,\
                                  const string &rootPath

   #define OMREST_CLASS_INPUT_PARAMETER pRestSession,\
                                        pRestAdaptor,\
                                        pRequest,\
                                        pResponse,\
                                        localAgentHost,\
                                        localAgentService,\
                                        rootPath


   #define DECLARE_OMREST_CMD_AUTO_REGISTER()                           \
      public:                                                           \
         static omRestCommandBase* newThis( OMREST_CLASS_PARAMETER ) ;  \

   #define IMPLEMENT_OMREST_CMD_AUTO_REGISTER(theClass)                       \
      omRestCommandBase* theClass::newThis( OMREST_CLASS_PARAMETER )          \
      {                                                                       \
         return SDB_OSS_NEW theClass( OMREST_CLASS_INPUT_PARAMETER ) ;        \
      }                                                                       \
      _omRestCmdAssit theClass##Assit( theClass::newThis ) ;                  \


   class omRestCommandBase : public omCommandInterafce
   {
      public:
         omRestCommandBase( pmdRestSession *pRestSession,
                            restAdaptor *pRestAdaptor,
                            restRequest *pRequest,
                            restResponse *pResponse,
                            const string &localAgentHost,
                            const string &localAgentService,
                            const string &rootPath ) ;

         virtual ~omRestCommandBase() ;

      public:
         virtual INT32     init( pmdEDUCB * cb ) ;
         virtual bool      isFetchAgentResponse( UINT64 requestID ) ;
         virtual INT32     doAgentResponse ( MsgHeader* pAgentResponse ) ;

         virtual const CHAR * name () = 0 ;

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
         SDB_RTNCB        *_pRTNCB ;
         SDB_DMSCB        *_pDMDCB ;
         pmdKRCB          *_pKRCB ;
         SDB_DMSCB        *_pDMSCB ;

         pmdEDUCB         *_cb ;

         pmdRestSession   *_restSession ;
         restAdaptor      *_restAdaptor ;
         restRequest      *_request ;
         restResponse     *_response ;

         string            _errorDetail ;
         string            _localAgentHost ;
         string            _localAgentService ;
         string            _rootPath ;

         omErrorTool       _errorMsg ;
   } ;

   typedef omRestCommandBase* (*OMREST_NEW_FUNC)( OMREST_CLASS_PARAMETER ) ;

   /*
      _omRestCmdAssit
   */
   class _omRestCmdAssit : public SDBObject
   {
   public:
      _omRestCmdAssit( OMREST_NEW_FUNC ) ;
      virtual ~_omRestCmdAssit() ;
   } ;

   struct _classComp
   {
      bool operator()( const CHAR *lhs, const CHAR *rhs ) const
      {
         return ossStrcasecmp( lhs, rhs ) < 0 ;
      }
   } ;

   typedef map<const CHAR*, OMREST_NEW_FUNC, _classComp> MAP_OACMD ;
#if defined (_WINDOWS)
   typedef MAP_OACMD::iterator MAP_OACMD_IT ;
#else
   typedef map<const CHAR*, OMREST_NEW_FUNC>::iterator MAP_OACMD_IT ;
#endif // _WINDOWS

   /*
      _omRestCmdBuilder
   */
   class _omRestCmdBuilder : public SDBObject
   {
   friend class _omRestCmdAssit ;

   public:
      _omRestCmdBuilder () ;
      ~_omRestCmdBuilder () ;

   public:
      omRestCommandBase *create ( const CHAR *command,
                                  OMREST_CLASS_PARAMETER ) ;

      void release ( omRestCommandBase *&pCommand ) ;

      INT32 _register ( const CHAR *name, OMREST_NEW_FUNC pFunc ) ;

      OMREST_NEW_FUNC _find ( const CHAR * name ) ;

   private:
      MAP_OACMD _cmdMap ;
   } ;

   /*
      get om rest command builder
   */
   _omRestCmdBuilder* getOmRestCmdBuilder() ;

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


