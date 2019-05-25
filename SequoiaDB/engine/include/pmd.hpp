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

   Source File Name = pmd.hpp

   Descriptive Name = Process MoDel Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure kernel control block,
   which is the most critical data structure in the engine process.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_HPP__
#define PMD_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "pmdEDUMgr.hpp"
#include "pd.hpp"
#include "pmdOptionsMgr.hpp"
#include "msg.h"
#include "msgDef.hpp"
#include "pmdEnv.hpp"
#include "sdbInterface.hpp"
#include "pmdMemPool.hpp"
#include "pmdSyncMgr.hpp"
#include "monCB.hpp"

namespace engine
{

   /*
      Sync clock interval define
   */
   #define PMD_SYNC_CLOCK_INTERVAL              ( 10 ) /// ms

   #define PMD_DB_STATUS()       pmdGetKRCB()->getDBStatus()
   #define PMD_IS_DB_NORMAL()    ( SDB_DB_NORMAL == PMD_DB_STATUS() )
   #define PMD_IS_DB_DOWN()      ( SDB_DB_SHUTDOWN == PMD_DB_STATUS() )
   #define PMD_IS_DB_UP()        ( !PMD_IS_DB_DOWN() )
   #define PMD_IS_DB_AVAILABLE() pmdGetKRCB()->isAvailable(NULL)

   #define PMD_SET_DB_STATUS(x)  pmdGetKRCB()->setDBStatus(x)

   #define PMD_SHUTDOWN_DB(code)  \
      do { \
         pmdGetKRCB()->setDBStatus( SDB_DB_SHUTDOWN ) ; \
         pmdGetKRCB()->setExitCode( code ) ; \
      } while ( 0 )

   #define PMD_RESTART_DB(code)   \
      do { \
         pmdGetKRCB()->setDBStatus( SDB_DB_SHUTDOWN ) ; \
         pmdGetKRCB()->setExitCode( code ) ; \
         pmdGetKRCB()->setRestart( TRUE ) ; \
      } while ( 0 )

   /*
      Register db to krcb
   */
   #define PMD_REGISTER_CB(pCB)  pmdGetKRCB()->registerCB(pCB, (void*)pCB)

   class _dpsLogWrapper ;
   class _clsMgr ;
   class dpsTransCB ;
   class sdbCatalogueCB ;
   class _bpsCB ;
   class _CoordCB ;
   class _SDB_RTNCB ;
   class _SDB_DMSCB ;
   class _sqlCB ;
   class _authCB ;
   class aggrBuilder ;
   class _spdFMPMgr ;

   /*
    * Kernel Control Block
    * Database Kernel Variables
    */
   class _SDB_KRCB : public SDBObject, public _IConfigHandle,
                     public IResource, _IEventHolder
   {
      typedef std::pair< IEventHander*, UINT32 >   EVENT_HANDLER_INFO ;
      typedef std::vector< EVENT_HANDLER_INFO >    VEC_EVENTHANDLER ;
   public:
      _SDB_KRCB () ;
      virtual ~_SDB_KRCB () ;

      INT32 init () ;
      void  destroy () ;

      void    setIsRestore( BOOLEAN isRestore ) { _isRestore = isRestore ; }
      BOOLEAN isRestore() const { return _isRestore ; }

   public:
      virtual IParam*            getParam() ;
      virtual IControlBlock*     getCBByType( SDB_CB_TYPE type ) ;
      virtual void*              getOrgPointByType( SDB_CB_TYPE type ) ;
      virtual BOOLEAN            isCBValue( SDB_CB_TYPE type ) const ;
      virtual IExecutorMgr*      getExecutorMgr() ;
      virtual IContextMgr*       getContextMgr() ;

      virtual SDB_DB_STATUS      getDBStatus() const ;
      virtual const CHAR*        getDBStatusDesp() const ;
      virtual BOOLEAN            isShutdown() const ;
      virtual BOOLEAN            isNormal() const ;
      virtual BOOLEAN            isAvailable( INT32 *pCode = NULL ) const ;
      virtual BOOLEAN            isActive() const { return _isActive ; }
      virtual INT32              getShutdownCode() const ;

      virtual UINT32             getDBMode() const ;
      virtual std::string        getDBModeDesp() const ;
      virtual BOOLEAN            isDBReadonly() const ;
      virtual BOOLEAN            isDBDeactivated() const ;

      virtual BOOLEAN            isInFlowControl() const ;

      virtual SDB_ROLE           getDBRole() const ;
      virtual const CHAR*        getDBRoleDesp() const ;

      virtual UINT16             getLocalPort() const ;
      virtual const CHAR*        getSvcname() const ;
      virtual const CHAR*        getDBPath() const ;
      virtual const CHAR*        getHostName() const { return _hostName ; }
      virtual const CHAR*        getGroupName () const { return _groupName ; }
      virtual UINT32             getNodeID() const ;
      virtual UINT32             getGroupID() const ;
      virtual BOOLEAN            isPrimary() const ;

      virtual UINT64             getStartTime() const ;
      virtual UINT64             getDBTick() const ;

      virtual void               getVersion( INT32 &ver,
                                             INT32 &subVer,
                                             INT32 &fixVer,
                                             INT32 &release,
                                             const CHAR **build ) const ;

      INT32             registerCB( IControlBlock *pCB, void *pOrg ) ;

      /*
         _IConfigHandle Interface
      */
      virtual void      onConfigChange ( UINT32 changeID ) ;
      virtual INT32     onConfigInit () ;
      virtual void      onConfigSave () ;

      /*
         _IEventHolder Interface
      */
      virtual INT32  regEventHandler( IEventHander *pHandler,
                                      UINT32 mask = EVENT_MASK_ON_ALL ) ;
      virtual void   unregEventHandler( IEventHander *pHandler ) ;

      void           callRegisterEventHandler( const MsgRouteID &nodeID ) ;
      void           callPrimaryChangeHandler( BOOLEAN primary,
                                               SDB_EVENT_OCCUR_TYPE type ) ;

   private:
      IControlBlock                 *_arrayCBs[ SDB_CB_MAX ] ;
      void                          *_arrayOrgs[ SDB_CB_MAX ] ;
      BOOLEAN                       _init ;
      BOOLEAN                       _isActive ;

      VEC_EVENTHANDLER              _vecEventHandler ;
      ossSpinSLatch                 _handlerLatch ;

   private :
      CHAR           _groupName[ OSS_MAX_GROUPNAME_SIZE + 1 ] ;
      CHAR           _hostName[ OSS_MAX_HOSTNAME + 1 ] ;

      SDB_ROLE       _role ;
      SDB_DB_STATUS  _dbStatus ;
      UINT32         _dbMode ;

      BOOLEAN        _flowControl ;

      BOOLEAN        _businessOK ;
      INT32          _exitCode ;
      BOOLEAN        _restart ;

      BOOLEAN        _isRestore ;

      _pmdEDUMgr     _eduMgr ;
      pmdBuffPool    _buffPool ;
      pmdSyncMgr     _syncMgr ;

      _pmdOptionsMgr _optioncb ;
      ossTick        _curTime ;

      pmdEDUCB*      _mainEDU ;

      monConfigCB    _monCfgCB ;
      monDBCB        _monDBCB ;

   public :
      pmdEDUMgr* getEDUMgr ()
      {
         return &_eduMgr ;
      }
      pmdBuffPool* getBuffPool()
      {
         return &_buffPool ;
      }
      pmdSyncMgr *getSyncMgr()
      {
         return &_syncMgr ;
      }
      CHAR *getGroupName ( CHAR *pBuffer, UINT32 size ) const
      {
         if ( !pBuffer || 0 == size )
            return NULL;
         ossStrncpy ( pBuffer, _groupName, size ) ;
         pBuffer[ size - 1 ] = 0 ;
         return pBuffer ;
      }
      OSS_INLINE _SDB_DMSCB *getDMSCB ()
      {
         return ( _SDB_DMSCB* )getOrgPointByType( SDB_CB_DMS ) ;
      }
      OSS_INLINE _SDB_RTNCB *getRTNCB ()
      {
         return ( _SDB_RTNCB* )getOrgPointByType( SDB_CB_RTN ) ;
      }
      OSS_INLINE monConfigCB * getMonCB()
      {
         return & _monCfgCB ;
      }
      OSS_INLINE monDBCB * getMonDBCB ()
      {
         return &_monDBCB ;
      }
      void setMonCB( monConfigCB & monCB )
      {
         _monCfgCB = monCB ;
      }
      void setMonDBCB ( monDBCB & cb )
      {
         _monDBCB = cb ;
      }
      void setMonTimestampSwitch( BOOLEAN flag )
      {
          _monCfgCB.timestampON = flag ;
      }
      OSS_INLINE _clsMgr *getClsCB ()
      {
         return ( _clsMgr* )getOrgPointByType( SDB_CB_CLS ) ;
      }
      OSS_INLINE _dpsLogWrapper* getDPSCB ()
      {
         return ( _dpsLogWrapper* )getOrgPointByType( SDB_CB_DPS ) ;
      }
      OSS_INLINE _bpsCB *getBPSCB ()
      {
         return ( _bpsCB* )getOrgPointByType( SDB_CB_BPS ) ;
      }
      OSS_INLINE _pmdOptionsMgr *getOptionCB()
      {
         return &_optioncb;
      }
      OSS_INLINE sdbCatalogueCB *getCATLOGUECB()
      {
         return ( sdbCatalogueCB* )getOrgPointByType( SDB_CB_CATALOGUE ) ;
      }
      OSS_INLINE _CoordCB *getCoordCB()
      {
         return ( _CoordCB* )getOrgPointByType( SDB_CB_COORD ) ;
      }
      OSS_INLINE _sqlCB *getSqlCB()
      {
         return ( _sqlCB* )getOrgPointByType( SDB_CB_SQL ) ;
      }
      OSS_INLINE _authCB *getAuthCB()
      {
         return ( _authCB* )getOrgPointByType( SDB_CB_AUTH ) ;
      }
      OSS_INLINE dpsTransCB *getTransCB()
      {
         return ( dpsTransCB* )getOrgPointByType( SDB_CB_TRANS ) ;
      }
      OSS_INLINE aggrBuilder *getAggrCB()
      {
         return ( aggrBuilder* )getOrgPointByType( SDB_CB_AGGR ) ;
      }
      OSS_INLINE _spdFMPMgr *getFMPCB()
      {
         return ( _spdFMPMgr* )getOrgPointByType( SDB_CB_FMP ) ;
      }

      BOOLEAN isBusinessOK() const
      {
         return _businessOK ;
      }
      BOOLEAN needRestart() const
      {
         return _restart ;
      }
      void setExitCode( INT32 exitCode )
      {
         if ( exitCode && _exitCode )
         {
            return ;
         }
         _exitCode = exitCode ;
      }
      void setRestart( BOOLEAN restart )
      {
         _restart = restart ;
      }
      void setBusinessOK( BOOLEAN businessOK )
      {
         _businessOK = businessOK ;
      }
      void setDBStatus ( SDB_DB_STATUS status )
      {
         if ( SDB_DB_SHUTDOWN != _dbStatus )
         {
            _dbStatus = status ;
         }
      }
      void setDBReadonly( BOOLEAN readonly )
      {
         if ( readonly )
         {
            _dbMode |= SDB_DB_MODE_READONLY ;
         }
         else
         {
            _dbMode &= ~SDB_DB_MODE_READONLY ;
         }
      }
      void setDBDeactivated( BOOLEAN deactivated )
      {
         if ( deactivated )
         {
            _dbMode |= SDB_DB_MODE_DEACTIVATED ;
         }
         else
         {
            _dbMode &= ~SDB_DB_MODE_DEACTIVATED ;
         }
      }
      void setFlowControl( BOOLEAN flowControl )
      {
         _flowControl = flowControl ;
      }
      void setGroupName ( const CHAR *groupName )
      {
         ossMemset( _groupName, 0, sizeof(_groupName) ) ;
         ossStrncpy ( _groupName, groupName, sizeof(_groupName)-1 );
      }
      void setHostName ( const CHAR* hostName )
      {
         ossMemset( _hostName, 0, sizeof(_hostName) ) ;
         ossStrncpy ( _hostName, hostName, sizeof(_hostName) - 1 );
      }

      ossTick  getCurTime() ;
      void     syncCurTime() ;

   } ;
   typedef _SDB_KRCB pmdKRCB ;

   /*
    * Get global kernel control block
    * This variable is unique per process
    */
   extern pmdKRCB pmd_krcb ;
   OSS_INLINE pmdKRCB* pmdGetKRCB()
   {
      return &pmd_krcb ;
   }
   OSS_INLINE pmdOptionsCB* pmdGetOptionCB()
   {
      return pmdGetKRCB()->getOptionCB() ;
   }
   OSS_INLINE pmdBuffPool* pmdGetBuffPool()
   {
      return pmdGetKRCB()->getBuffPool() ;
   }
   OSS_INLINE pmdSyncMgr *pmdGetSyncMgr()
   {
      return pmdGetKRCB()->getSyncMgr() ;
   }

}

#endif //PMD_HPP__

