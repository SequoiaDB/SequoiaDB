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

   Source File Name = sdbInterface.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_INTERFACE_HPP__
#define SDB_INTERFACE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "msg.h"
#include "msgDef.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace engine
{

   // forward declaration
   typedef class _authAccessControlList authAccessControlList;
   typedef class _authActionSet authActionSet;
   typedef class _authResource authResource;
   class IOperationContext ;

   /*
      ENUM define
   */
   enum SDB_CB_TYPE
   {
      SDB_CB_DPS              = 0,
      SDB_CB_DMS,
      SDB_CB_BPS,
      SDB_CB_RTN,
      SDB_CB_TRANS,           // must after SDB_CB_DPS
      SDB_CB_CATALOGUE,
      SDB_CB_CLS,             // must after SDB_CB_CATALOGUE
      SDB_CB_COORD,
      SDB_CB_SQL,
      SDB_CB_AUTH,
      SDB_CB_AGGR,
      SDB_CB_FMP,
      SDB_CB_OMSVC,
      SDB_CB_OMAGT,

      SDB_CB_PMDCTRL,
      SDB_CB_OMPROXY,
      SDB_CB_SEADAPTER,
      // THE MAX CB TYPE
      SDB_CB_MAX
   } ;

   /*
      SDB_INTERFACE_TYPE DEFINE
   */
   enum SDB_INTERFACE_TYPE
   {
      SDB_IF_CB      = 1,
      SDB_IF_SESSION,
      SDB_IF_EVT_HANDLER,
      SDB_IF_EVT_HOLDER,

      SDB_IF_CTXMGR,
      SDB_IF_CLS,

      // THD MAX IF TYPE
      SDB_IF_MAX
   } ;

   /*
      SDB_SESSION_TYPE define
   */
   enum SDB_SESSION_TYPE
   {
      SDB_SESSION_LOCAL       = 0,
      SDB_SESSION_REST,
      SDB_SESSION_REPL_DST,
      SDB_SESSION_REPL_SRC,
      SDB_SESSION_SHARD,
      SDB_SESSION_FS_SRC,
      SDB_SESSION_FS_DST,
      SDB_SESSION_SPLIT_SRC,
      SDB_SESSION_SPLIT_DST,
      SDB_SESSION_OMAGENT,
      SDB_SESSION_PROTOCOL,
      SDB_SESSION_SE_INDEX,
      SDB_SESSION_SE_AGENT,
      SDB_SESSION_DUMMY,
      // Reserved
      SDB_SESSION_MAX
   } ;

   /*
      SDB_CLIENT_TYPE define
   */
   enum SDB_CLIENT_TYPE
   {
      SDB_CLIENT_EXTERN    = 1,  // external client,ex: local service
      SDB_CLIENT_INNER,          // inner client, ex: shard service

      // Reserved
      SDB_CLIENT_MAX
   } ;

   /*
      SDB_DB_STATUS define
   */
   enum SDB_DB_STATUS
   {
      SDB_DB_NORMAL           = 0,
      SDB_DB_SHUTDOWN,
      SDB_DB_REBUILDING,
      SDB_DB_FULLSYNC,
      SDB_DB_OFFLINE_BK,

      SDB_DB_STATUS_MAX
   } ;

   /*
      Define SDB_DB_MODE value
   */
   #define SDB_DB_MODE_READONLY        0x00000001
   #define SDB_DB_MODE_DEACTIVATED     0x00000002

   /*
      _ISDBRoot define
   */
   class _ISDBRoot
   {
      public:
         _ISDBRoot () {}
         virtual ~_ISDBRoot () {}

         virtual void* queryInterface( SDB_INTERFACE_TYPE type )
         {
            return NULL ;
         }
   } ;
   typedef _ISDBRoot ISDBRoot ;

   /*
      EVENT MASK DEFINE
   */
   #define EVENT_MASK_ON_REGISTERED          0x00000001
   #define EVENT_MASK_ON_PRIMARYCHG          0x00000002

   #define EVENT_MASK_ON_ALL                 0xFFFFFFFF

   enum SDB_EVENT_OCCUR_TYPE
   {
      SDB_EVT_OCCUR_BEFORE   = 1,
      SDB_EVT_OCCUR_AFTER
   } ;

   /*
      _IEventHander define
   */
   class _IEventHander
   {
      public :
         _IEventHander () {}
         virtual ~_IEventHander () {}

         virtual void   onRegistered( const MsgRouteID &nodeID )
         {}
         virtual void   onPrimaryChange( BOOLEAN primary,
                                         SDB_EVENT_OCCUR_TYPE occurType )
         {}

   } ;
   typedef _IEventHander IEventHander ;

   /*
      _IEventHolder define
   */
   class _IEventHolder
   {
      public:
         _IEventHolder () {}
         virtual ~_IEventHolder () {}

         virtual INT32  regEventHandler( IEventHander *pHandler,
                                         UINT32 mask = EVENT_MASK_ON_ALL ) = 0 ;
         virtual void   unregEventHandler( IEventHander *pHandler ) = 0 ;
   } ;
   typedef _IEventHolder IEventHolder ;

   /*
      _IConfigHandle define
   */
   class _IConfigHandle
   {
      public:
         _IConfigHandle () {}
         virtual ~_IConfigHandle () {}

         virtual void   onConfigChange ( UINT32 changeID ) = 0 ;
         virtual INT32  onConfigInit () = 0 ;
         virtual void   onConfigSave () = 0 ;
   } ;
   typedef _IConfigHandle IConfigHandle ;

   /*
      _IParam define
   */
   class _IParam
   {
      public:
         _IParam() {}
         virtual ~_IParam() {}

      public:
         virtual  BOOLEAN hasField( const CHAR *pFieldName ) = 0 ;
         virtual  INT32   getFieldInt( const CHAR *pFieldName,
                                       INT32 &value,
                                       INT32 *pDefault = NULL ) = 0 ;
         virtual  INT32   getFieldStr( const CHAR *pFieldName,
                                       CHAR *pValue, UINT32 len,
                                       const CHAR *pDefault = NULL ) = 0 ;
         virtual  INT32   getFieldStr( const CHAR *pFieldName,
                                       std::string &strValue,
                                       const CHAR *pDefault = NULL ) = 0 ;

   } ;
   typedef _IParam IParam ;

   /*
      _IClient define
   */
   class _IClient : public SDBObject
   {
      public:
         _IClient() {}
         virtual ~_IClient() {}

      public:
         virtual SDB_CLIENT_TYPE clientType() const = 0 ;
         virtual const CHAR*     clientName() const = 0 ;

         virtual INT32        authenticate( MsgHeader *pMsg,
                                            const CHAR **authBuf = NULL
                                          ) = 0 ;
         virtual INT32        authenticate( const CHAR *username,
                                            const CHAR *password ) = 0 ;
         virtual void         logout() = 0 ;
         virtual INT32        disconnect() = 0 ;

         virtual BOOLEAN      isAuthed() const = 0 ;
         virtual BOOLEAN      isConnected() const = 0 ;
         virtual BOOLEAN      isClosed() const = 0 ;

         virtual UINT16       getLocalPort() const = 0 ;
         virtual const CHAR*  getLocalIPAddr() const = 0 ;
         virtual UINT16       getPeerPort() const = 0 ;
         virtual const CHAR*  getPeerIPAddr() const = 0 ;
         virtual const CHAR*  getUsername() const = 0 ;
         virtual const CHAR*  getPassword() const = 0 ;

         virtual const CHAR*  getFromIPAddr() const = 0 ;
         virtual UINT16       getFromPort() const = 0 ;

         virtual const MsgHeader *getInMsg() const = 0 ;
         virtual void         registerInMsg( const MsgHeader *msg ) = 0 ;
         virtual void         unregisterInMsg() = 0 ;
         virtual void         setClientVersion( SDB_PROTOCOL_VERSION version ) = 0 ;
         virtual SDB_PROTOCOL_VERSION getClientVersion() const = 0 ;

         virtual BOOLEAN      privCheckEnabled() const = 0 ;
         virtual UINT32       getRoleID() const = 0 ;
   } ;
   typedef _IClient IClient ;

   /*
      _IOperator define
   */
   class _IOperator : public SDBObject
   {
   public:
      _IOperator() {}
      virtual ~_IOperator() {}

   public:
      virtual const MsgHeader* getMsg() const = 0 ;
      virtual const MsgGlobalID& getGlobalID() const = 0 ;
      virtual void  updateGlobalID( const MsgGlobalID &globalID ) = 0 ;
   } ;
   typedef _IOperator IOperator ;

   /*
      _ISession define
   */
   class _ISession : public SDBObject
   {
      public:
         _ISession() {}
         virtual ~_ISession() {}

      public:
         virtual UINT64             identifyID() = 0 ;
         virtual MsgRouteID         identifyNID() = 0 ;
         virtual UINT32             identifyTID() = 0 ;
         virtual UINT64             identifyEDUID() = 0 ;

         virtual const CHAR*        sessionName() const = 0 ;
         virtual SDB_SESSION_TYPE   sessionType() const = 0 ;
         virtual BOOLEAN            isBusinessSession() const = 0 ;
         virtual INT32              getServiceType() const = 0 ;
         virtual IClient*           getClient() = 0 ;
         virtual IOperator*         getOperator() = 0 ;

         virtual void*              getSchedItemPtr() = 0 ;
         virtual void               setSchedItemVer( INT32 ver ) = 0 ;

         virtual INT32 checkPrivilegesForCmd( const CHAR *cmdName,
                                              const CHAR *pQuery,
                                              const CHAR *pSelector,
                                              const CHAR *pOrderby,
                                              const CHAR *pHint ) = 0;

         virtual INT32 checkPrivilegesForActionsOnExact( const CHAR *pCollectionName,
                                                         const authActionSet &actions ) = 0;

         virtual INT32 checkPrivilegesForActionsOnCluster( const authActionSet &actions ) = 0;

         virtual INT32 checkPrivilegesForActionsOnResource(
            const boost::shared_ptr< authResource > &,
            const authActionSet &actions ) = 0;

         virtual BOOLEAN privilegeCheckEnabled() = 0;

         virtual INT32 getACL( boost::shared_ptr<const authAccessControlList> &acl ) = 0;

         virtual IOperationContext *getOperationContext() = 0 ;

      protected:
         virtual void               _onAttach () {}
         virtual void               _onDetach () {}

   } ;
   typedef _ISession ISession ;

   /*
      SDB_LOCK_TYPE define
   */
   enum SDB_LOCK_TYPE
   {
      SDB_LOCK_DMS      = 0,

      SDB_LOCK_MAX
   } ;

   /*
      EDU INFO TYPE DEFINE
   */
   enum EDU_INFO_TYPE
   {
      EDU_INFO_ERROR                   = 1,     // Error
      EDU_INFO_DOING                            // Current doing
   } ;

   /*
      sdbLockItem define
   */
   struct sdbLockItem
   {
      UINT32   _lockMode ;
      UINT32   _lockCount ;
      UINT32   _lockHWCount ;

      sdbLockItem() { reset() ; }
      void     reset() { _lockMode = 0 ; _lockCount = 0 ; _lockHWCount = 0 ; }
      void     setMode( UINT32 mode ) { _lockMode = mode ; }
      UINT32   getMode() const { return _lockMode ; }
      UINT32   incCount() { ++_lockHWCount ; return ++_lockCount ; }
      UINT32   decCount() { return --_lockCount ; }
      UINT32   lockCount() const { return _lockCount ; }
      UINT32   lockHWCount() const { return _lockHWCount ; }
   } ;

   /*
      _IRemoteSite define
   */
   class _IRemoteSite : public SDBObject
   {
      public:
         _IRemoteSite() {}
         virtual ~_IRemoteSite() {}

      public:
         virtual  UINT64   getUserData() const = 0 ;

   } ;
   typedef _IRemoteSite IRemoteSite ;

   /*
      _IExecutor define
   */
   class _IExecutor : public SDBObject
   {
      public:
         _IExecutor() {}
         virtual ~_IExecutor() {}

      public:

         /*
            Base Function
         */
         virtual EDUID     getID() const = 0 ;
         virtual UINT32    getTID() const = 0 ;

         /*
            Session Related
         */
         virtual ISession* getSession() = 0 ;
         virtual IRemoteSite* getRemoteSite() = 0 ;
         virtual IOperationContext *getOperationContext() = 0 ;

         /*
            Status and Control
         */
         virtual BOOLEAN   isInterrupted ( BOOLEAN onlyFlag = FALSE ) = 0 ;
         virtual BOOLEAN   isDisconnected () = 0 ;
         virtual BOOLEAN   isForced () = 0 ;

         virtual BOOLEAN   isWritingDB() const = 0 ;
         virtual UINT64    getWritingID() const = 0 ;
         virtual void      writingDB( BOOLEAN writing, const CHAR* name ) = 0 ;

         virtual UINT32    getProcessedNum() const = 0 ;
         virtual void      incEventCount( UINT32 step = 1 ) = 0 ;

         virtual UINT32    getQueSize() = 0 ;

         /*
            Resource Info
         */
         virtual sdbLockItem* getLockItem( SDB_LOCK_TYPE lockType ) = 0 ;
         virtual INT32        appendInfo( EDU_INFO_TYPE type, const CHAR * format, ...) = 0 ;
         virtual INT32        printInfo ( EDU_INFO_TYPE type, const CHAR *format, ... ) = 0 ;
         virtual const CHAR*  getInfo ( EDU_INFO_TYPE type ) = 0 ;
         virtual void         resetInfo ( EDU_INFO_TYPE type ) = 0 ;

         /*
            Buffer Manager
         */
         virtual INT32     allocBuff( UINT32 len,
                                      CHAR **ppBuff,
                                      UINT32 *pRealSize = NULL ) = 0 ;

         virtual INT32     reallocBuff( UINT32 len,
                                        CHAR **ppBuff,
                                        UINT32 *pRealSize = NULL ) = 0 ;

         virtual void      releaseBuff( CHAR *pBuff ) = 0 ;

         virtual void*     getAlignedBuff( UINT32 size,
                                           UINT32 *pRealSize = NULL,
                                           UINT32 alignment =
                                           OSS_FILE_DIRECT_IO_ALIGNMENT ) = 0 ;

         virtual void      releaseAlignedBuff() = 0 ;

         virtual CHAR*     getBuffer( UINT32 len ) = 0 ;

         virtual void      releaseBuffer() = 0 ;

         /*
            Operation Related
         */
         /// for read
         virtual UINT64    getBeginLsn () const = 0 ;
         virtual UINT64    getEndLsn() const = 0 ;
         virtual UINT32    getLsnCount () const = 0 ;
         virtual BOOLEAN   isDoRollback () const = 0 ;

         virtual UINT64    getTransID () const = 0 ;
         virtual UINT64    getCurTransLsn () const = 0 ;
         /// for write
         virtual void      resetLsn() = 0 ;
         virtual void      insertLsn( UINT64 lsn,
                                      BOOLEAN isRollback = FALSE ) = 0 ;

         virtual void      setTransID( UINT64 transID ) = 0 ;
         virtual void      setCurTransLsn( UINT64 lsn ) = 0 ;

         /*
            Context Related
         */
         virtual BOOLEAN   contextInsert( INT64 contextID ) = 0 ;
         virtual void      contextDelete( INT64 contextID ) = 0 ;
         virtual INT64     contextPeek() = 0 ;
         virtual BOOLEAN   contextFind( INT64 contextID ) = 0 ;
         virtual UINT32    contextNum() = 0 ;

         /*
            Log config
          */
         virtual BOOLEAN   isLogTimeOn() const = 0 ;
         virtual UINT32    getLogWriteMod() const = 0 ;
   } ;
   typedef _IExecutor IExecutor ;

   // interface to get IExecutor of current thread
   IExecutor *sdbGetThreadExecutor() ;

   /*
      _IIOService define
   */
   class _IIOService
   {
      public:
         _IIOService() {}
         virtual ~_IIOService() {}

      public:
         virtual INT32     run() = 0 ;
         virtual void      stop() = 0 ;
         virtual void      resetMon() = 0 ;
   } ;
   typedef _IIOService IIOService ;

   /*
      _IExecutorMgr define
   */
   class _IExecutorMgr : public SDBObject
   {
      public:
         _IExecutorMgr() {}
         virtual ~_IExecutorMgr() {}

      public:
         virtual INT32     startEDU( INT32 type,
                                     void *args,
                                     EDUID *pEDUID = NULL,
                                     const CHAR *pInitName = "" ) = 0 ;

         virtual void      addIOService( IIOService *pIOService ) = 0 ;
         virtual void      delIOSerivce( IIOService *pIOService ) = 0 ;

   } ;
   typedef _IExecutorMgr IExecutorMgr ;

   /*
      _IContext define
   */
   class _IContext
   {
      public:
         _IContext() {}
         virtual ~_IContext() {}

      public:
         virtual INT32 pause() = 0 ;
         virtual INT32 resume() = 0 ;

   } ;
   typedef _IContext IContext ;

   /*
      _IContextMgr define
   */
   class _IContextMgr : public SDBObject
   {
      public:
         _IContextMgr() {}
         virtual ~_IContextMgr() {}

      public:
         virtual void contextDelete( INT64 contextID, IExecutor *pExe ) = 0 ;
   } ;
   typedef _IContextMgr IContextMgr ;

   /*
      _ICluster define
   */
   class _ICluster
   {
      public:
         _ICluster() {}
         virtual ~_ICluster() {}

      public:
         virtual UINT64    completeLsn( BOOLEAN doFast = TRUE,
                                        UINT32 *pVer = NULL ) = 0 ;
         virtual UINT32    lsnQueSize() = 0 ;
         virtual BOOLEAN   primaryLsn( UINT64 &lsn, UINT32 *pVer = NULL ) = 0 ;
   } ;
   typedef _ICluster ICluster ;

   /*
      _IControlBlock define
   */
   class _IControlBlock : public SDBObject, public _ISDBRoot
   {
      public:
         _IControlBlock () {}
         virtual ~_IControlBlock () {}

         virtual SDB_CB_TYPE cbType() const = 0 ;
         virtual const CHAR* cbName() const = 0 ;

         virtual INT32  init () = 0 ;
         virtual INT32  active () = 0 ;
         virtual INT32  deactive () = 0 ;
         virtual INT32  fini () = 0 ;
         virtual void   onConfigChange() {}
         virtual void   onConfigSave() {}

   } ;
   typedef _IControlBlock IControlBlock ;

   /*
      _IResource define
   */
   class _IResource
   {
      public:
         _IResource() {}
         virtual ~_IResource() {}

      public:
         virtual IParam*            getParam() = 0 ;
         virtual IControlBlock*     getCBByType( SDB_CB_TYPE type ) = 0 ;
         virtual BOOLEAN            isCBValue( SDB_CB_TYPE type ) const = 0 ;
         virtual void*              getOrgPointByType( SDB_CB_TYPE type ) = 0 ;
         virtual IExecutorMgr*      getExecutorMgr() = 0 ;
         virtual IContextMgr*       getContextMgr() = 0 ;
         virtual ICluster*          getCluster() = 0 ;

         virtual SDB_DB_STATUS      getDBStatus() const = 0 ;
         virtual const CHAR*        getDBStatusDesp() const = 0 ;
         virtual BOOLEAN            isShutdown() const = 0 ;
         virtual BOOLEAN            isNormal() const = 0 ;
         virtual BOOLEAN            isAvailable( INT32 *pCode = NULL ) const = 0 ;
         virtual BOOLEAN            isActive() const = 0 ;
         virtual INT32              getShutdownCode() const = 0 ;

         virtual UINT32             getDBMode() const = 0 ;
         virtual std::string        getDBModeDesp() const = 0 ;
         virtual BOOLEAN            isDBReadonly() const = 0 ;
         virtual BOOLEAN            isDBDeactivated() const = 0 ;

         virtual BOOLEAN            isInFlowControl() const = 0 ;

         virtual SDB_ROLE           getDBRole() const = 0 ;
         virtual const CHAR*        getDBRoleDesp() const = 0 ;

         virtual BOOLEAN            isPrimary() const = 0 ;
         virtual UINT16             getLocalPort() const = 0 ;
         virtual const CHAR*        getSvcname() const = 0 ;
         virtual const CHAR*        getDBPath() const = 0 ;
         virtual const CHAR*        getHostName() const = 0 ;
         virtual const CHAR*        getGroupName () const = 0 ;
         virtual UINT32             getNodeID() const = 0 ;
         virtual UINT32             getGroupID() const = 0 ;

         virtual UINT64             getStartTime() const = 0 ;
         virtual UINT64             getDBTick() const = 0 ;

         virtual void               getVersion( INT32 &ver,
                                                INT32 &subVer,
                                                INT32 &fixVer,
                                                INT32 &release,
                                                const CHAR **build,
                                                const CHAR **gitVer = NULL )
                                                const = 0 ;

         virtual BOOLEAN            isRestore() const = 0 ;

   } ;
   typedef _IResource IResource ;

}

#endif // SDB_INTERFACE_HPP__

