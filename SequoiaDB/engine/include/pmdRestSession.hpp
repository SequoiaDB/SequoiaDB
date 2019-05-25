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

   Source File Name = pmdRestSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_REST_SESSION_HPP_
#define PMD_REST_SESSION_HPP_

#include "pmdSessionBase.hpp"
#include "restDefine.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "pmdProcessorBase.hpp"
#include "pmdEDU.hpp"
#include <string>

using namespace bson ;

namespace engine
{

   #define  OM_REST_HEAD_SDBUSER         "SdbUser"
   #define  OM_REST_HEAD_SDBPASSWD       "SdbPasswd"
   #define  OM_REST_HEAD_SDBHOSTNAME     "SdbHostName"
   #define  OM_REST_HEAD_SDBSERVICENAME  "SdbServiceName"

   class _dpsLogWrapper ;
   class _SDB_RTNCB ;
   class restAdaptor ;

   /*
      global define
   */
   #define SESSION_USER_NAME_LEN       ( 63 )

   /*
      _restSessionInfo define
   */
   struct _restSessionInfo : public SDBObject
   {
      struct _sessionAttr
      {
         UINT64            _sessionID ;      // host ip + seq
         UINT64            _loginTime ;
         CHAR              _userName[SESSION_USER_NAME_LEN+1] ;
      } _attr ;

      UINT64               _activeTime ;
      INT64                _timeoutCounter ; // ms
      BOOLEAN              _authOK ;
      ossAtomic32          _inNum ;
      ossSpinXLatch        _inLatch ;
      BOOLEAN              _isLock ;

      std::string          _id ;

      _pmdEDUCB::CATCH_MAP _catchMap ;

      _restSessionInfo()
      :_inNum( 0 )
      {
         _attr._sessionID     = 0 ;
         _attr._loginTime     = (UINT64)time(NULL) ;
         ossMemset( _attr._userName, 0, sizeof( _attr._userName ) ) ;
         _activeTime          = _attr._loginTime ;
         _timeoutCounter      = 0 ;
         _authOK              = FALSE ;
         _isLock              = FALSE ;
      }
      INT32 getAttrSize()
      {
         return (INT32)sizeof( _attr ) ;
      }
      BOOLEAN isValid() const
      {
         return _id.size() == 0 ? FALSE : TRUE ;
      }
      BOOLEAN isIn()
      {
         return !_inNum.compare( 0 ) ;
      }
      void invalidate()
      {
         _id     = "" ;
         _authOK = FALSE ;
      }
      void active()
      {
         _activeTime       = (UINT64)time( NULL ) ;
         _timeoutCounter   = 0 ;
      }
      BOOLEAN isTimeout( INT64 timeout ) const
      {
         if ( timeout > 0 && _timeoutCounter > timeout )
         {
            return TRUE ;
         }
         return FALSE ;
      }
      void onTimer( UINT32 interval )
      {
         _timeoutCounter += interval ;
      }
      void lock()
      {
         _inLatch.get() ;
         _isLock = TRUE ;
      }
      void unlock()
      {
         _isLock = FALSE ;
         _inLatch.release() ;
      }
      BOOLEAN isLock() const
      {
         return _isLock ;
      }

      void releaseMem() ;
      void pushMemToMap( _pmdEDUCB::CATCH_MAP &catchMap ) ;
      void makeMemFromMap( _pmdEDUCB::CATCH_MAP &catchMap ) ;

   } ;
   typedef _restSessionInfo restSessionInfo ;


   class RestToMSGTransfer ;

   /*
      _pmdRestSession define
   */
   class _pmdRestSession : public _pmdSession
   {
      public:
         _pmdRestSession( SOCKET fd ) ;
         virtual ~_pmdRestSession () ;

         virtual INT32     getServiceType() const ;
         virtual SDB_SESSION_TYPE sessionType() const ;

         virtual INT32     run() ;


      public:
         httpConnection*   getRestConn() { return &_restConn ; }
         CHAR*             getFixBuff() ;
         INT32             getFixBuffSize () const ;

         BOOLEAN           isAuthOK() ;
         string            getLoginUserName() ;
         const CHAR*       getSessionID() ;

         void              doLogout () ;
         INT32             doLogin ( const string &username,
                                     UINT32 localIP ) ;

         INT32             _dealWithLoginReq( INT32 result ) ;

      protected:
         virtual void      _onAttach () ;
         virtual void      _onDetach () ;

         void              restoreSession() ;
         void              saveSession() ;

      protected:

         INT32             _fetchOneContext( SINT64 &contextID,
                                             rtnContextBuf &contextBuff ) ;
         virtual INT32     _processMsg( HTTP_PARSE_COMMON command,
                                        const CHAR *pFilePath ) ;
         INT32             _processBusinessMsg( restAdaptor *pAdaptor ) ;
         INT32             _translateMSG( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;
         INT32             _checkAuth( restAdaptor *pAdaptor ) ;
      protected:
         httpConnection    _restConn ;
         CHAR*             _pFixBuff ;

         restSessionInfo*  _pSessionInfo ;

         string            _wwwRootPath ;

         _SDB_RTNCB        *_pRTNCB ;

         RestToMSGTransfer *_pRestTransfer ;

   } ;
   typedef _pmdRestSession pmdRestSession ;


   #define REST_CMD_NAME_QUERY         "query"
   #define REST_CMD_NAME_INSERT        "insert"
   #define REST_CMD_NAME_UPDATE        "update"
   #define REST_CMD_NAME_DELETE        "delete"
   #define REST_CMD_NAME_QUERY_UPDATE  "queryandupdate"
   #define REST_CMD_NAME_QUERY_REMOVE  "queryandremove"
   #define REST_CMD_NAME_UPSERT        "upsert"
   #define REST_CMD_NAME_LIST          "list"
   #define REST_CMD_NAME_SNAPSHOT      "snapshot"
   #define REST_CMD_NAME_EXEC          "exec"
   #define REST_CMD_NAME_LISTINDEXES   "list indexes"
   #define REST_CMD_NAME_TRUNCATE_COLLECTION "truncate collection"
   #define REST_CMD_NAME_ATTACH_COLLECTION "attach collection"
   #define REST_CMD_NAME_DETACH_COLLECTION "detach collection"
   #define REST_CMD_NAME_START_GROUP   "start group"
   #define REST_CMD_NAME_STOP_GROUP    "stop group"
   #define REST_CMD_NAME_START_NODE    "start node"
   #define REST_CMD_NAME_STOP_NODE     "stop node"

   class RestToMSGTransfer ;
   typedef INT32 ( RestToMSGTransfer::*restTransFunc )( restAdaptor *pAdaptor,
                                                        MsgHeader **msg ) ;

   class RestToMSGTransfer : public SDBObject
   {
      public:
         RestToMSGTransfer( pmdRestSession *session ) ;
         ~RestToMSGTransfer() ;

      public:
         INT32       trans( restAdaptor *pAdaptor, MsgHeader **msg ) ;
         INT32       init() ;

      private:
         INT32       _convertCreateCS( restAdaptor *pAdaptor,
                                       MsgHeader **msg ) ;
         INT32       _convertCreateCL( restAdaptor *pAdaptor,
                                       MsgHeader **msg ) ;
         INT32       _convertDropCS( restAdaptor *pAdaptor, MsgHeader **msg ) ;
         INT32       _convertDropCL( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertQueryBasic( restAdaptor *pAdaptor,
                                         const CHAR** collectionName,
                                         BSONObj& match,
                                         BSONObj& selector,
                                         BSONObj& order,
                                         BSONObj& hint,
                                         INT32* flag,
                                         SINT64* skip,
                                         SINT64* returnRow ) ;
         INT32       _convertQuery( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertQueryUpdate( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;
         INT32       _convertQueryRemove( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;
         INT32       _convertQueryModify( restAdaptor *pAdaptor,
                                          MsgHeader **msg,
                                          BOOLEAN isUpdate ) ;

         INT32       _convertInsert( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertUpdateBase( restAdaptor *pAdaptor, MsgHeader **msg,
                                         BOOLEAN isUpsert = FALSE ) ;
         INT32       _convertUpdate( restAdaptor *pAdaptor, MsgHeader **msg ) ;
         INT32       _convertUpsert( restAdaptor * pAdaptor, MsgHeader **msg ) ;

         INT32       _convertDelete( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertSplit( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertCreateIndex( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;

         INT32       _convertDropIndex( restAdaptor *pAdaptor,
                                        MsgHeader **msg ) ;

         INT32       _convertTruncateCollection( restAdaptor *pAdaptor,
                                                 MsgHeader **msg ) ;
         INT32       _coverAttachCollection( restAdaptor *pAdaptor,
                                             MsgHeader **msg ) ;
         INT32       _coverDetachCollection( restAdaptor *pAdaptor,
                                             MsgHeader **msg ) ;

         INT32       _convertAlterCollection( restAdaptor *pAdaptor,
                                              MsgHeader **msg ) ;
         INT32       _convertGetCount( restAdaptor *pAdaptor,
                                       MsgHeader **msg ) ;

         INT32       _convertListContexts( restAdaptor *pAdaptor,
                                           MsgHeader **msg ) ;
         INT32       _convertListBase( restAdaptor *pAdaptor,
                                              BSONObj &match, BSONObj &selector,
                                              BSONObj &order ) ;
         INT32       _convertListContextsCurrent( restAdaptor *pAdaptor,
                                                  MsgHeader **msg ) ;
         INT32       _convertListSessions( restAdaptor *pAdaptor,
                                           MsgHeader **msg ) ;

         INT32       _convertListGroups( restAdaptor *pAdaptor,
                                         MsgHeader **msg ) ;
         INT32       _convertStartGroup( restAdaptor *pAdaptor,
                                         MsgHeader **msg ) ;
         INT32       _convertStopGroup( restAdaptor *pAdaptor,
                                        MsgHeader **msg ) ;

         INT32       _convertStartNode( restAdaptor *pAdaptor,
                                         MsgHeader **msg ) ;
         INT32       _convertStopNode( restAdaptor *pAdaptor,
                                        MsgHeader **msg ) ;

         INT32       _convertListSessionsCurrent( restAdaptor *pAdaptor,
                                                  MsgHeader **msg ) ;
         INT32       _convertListCollections( restAdaptor *pAdaptor,
                                              MsgHeader **msg ) ;
         INT32       _convertListCollectionSpaces( restAdaptor *pAdaptor,
                                                   MsgHeader **msg ) ;
         INT32       _convertListStorageUnits( restAdaptor *pAdaptor,
                                               MsgHeader **msg ) ;

         INT32       _convertCreateProcedure( restAdaptor *pAdaptor,
                                              MsgHeader **msg ) ;
         INT32       _convertRemoveProcedure( restAdaptor *pAdaptor,
                                              MsgHeader **msg ) ;
         INT32       _convertListProcedures( restAdaptor *pAdaptor,
                                             MsgHeader **msg ) ;

         INT32       _convertCreateDomain( restAdaptor *pAdaptor,
                                           MsgHeader **msg ) ;
         INT32       _convertDropDomain( restAdaptor *pAdaptor,
                                         MsgHeader **msg ) ;
         INT32       _convertAlterDomain( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;
         INT32       _convertListDomains( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;
         INT32       _convertListTasks( restAdaptor *pAdaptor,
                                        MsgHeader **msg ) ;
         INT32       _convertListCSInDomain( restAdaptor *pAdaptor,
                                             MsgHeader **msg ) ;
         INT32       _convertListCLInDomain( restAdaptor *pAdaptor,
                                             MsgHeader **msg ) ;

         INT32       _convertListLobs( restAdaptor *pAdaptor,
                                       MsgHeader **msg ) ;

         INT32       _convertListIndexes( restAdaptor *pAdaptor,
                                          MsgHeader **msg ) ;

         INT32       _convertSnapshotContext( restAdaptor *pAdaptor,
                                              MsgHeader **msg ) ;
         INT32       _convertSnapshotContextCurrent( restAdaptor *pAdaptor,
                                                     MsgHeader **msg ) ;
         INT32       _convertSnapshotSessions( restAdaptor *pAdaptor,
                                               MsgHeader **msg ) ;
         INT32       _convertSnapshotSessionsCurrent( restAdaptor *pAdaptor,
                                                      MsgHeader **msg ) ;
         INT32       _convertSnapshotCollections( restAdaptor *pAdaptor,
                                                  MsgHeader **msg ) ;
         INT32       _convertSnapshotCollectionSpaces( restAdaptor *pAdaptor,
                                                       MsgHeader **msg ) ;
         INT32       _convertSnapshotDatabase( restAdaptor *pAdaptor,
                                               MsgHeader **msg ) ;
         INT32       _convertSnapshotSystem( restAdaptor *pAdaptor,
                                             MsgHeader **msg ) ;
         INT32       _convertSnapshotCata( restAdaptor *pAdaptor,
                                           MsgHeader **msg ) ;
         INT32       _convertSnapshotAccessPlans ( restAdaptor * pAdaptor,
                                                   MsgHeader ** msg ) ;
         INT32       _convertSnapshotHealth ( restAdaptor * pAdaptor,
                                              MsgHeader ** msg ) ;

         INT32       _buildExecMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                    const CHAR *pSql, UINT64 reqID ) ;
         INT32       _convertExec( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertForceSession( restAdaptor *pAdaptor,
                                           MsgHeader **msg ) ;

         INT32       _convertLogin( restAdaptor *pAdaptor, MsgHeader **msg ) ;

         INT32       _convertAnalyze ( restAdaptor * pAdaptor, MsgHeader ** msg ) ;

      private:
         pmdRestSession    *_restSession ;
         std::map< string, restTransFunc > _mapTransFunc ;
         typedef std::map< string, restTransFunc >::value_type _value_type ;
         typedef std::map< string, restTransFunc >::iterator _iterator ;
   } ;

   void _sendOpError2Web ( INT32 rc, restAdaptor *pAdptor,
                           pmdRestSession *pRestSession, pmdEDUCB* pEduCB ) ;

}

#endif //PMD_REST_SESSION_HPP_

