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

   Source File Name = pmd.cpp

   Descriptive Name = Process MoDel

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains kernel control block object.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include <string.h>
#include "core.hpp"
#include "pmd.hpp"
#include "ossVer.h"
#include "pdTrace.hpp"
#include "netFrame.hpp"

namespace engine
{

   /*
      _SDB_KRCB implement
   */
   _SDB_KRCB::_SDB_KRCB ()
   {
      ossMemset( _hostName, 0, sizeof( _hostName ) ) ;
      ossMemset( _groupName, 0, sizeof( _groupName ) ) ;

      _exitCode = SDB_OK ;
      _businessOK = TRUE ;
      _restart = FALSE ;
      _isRestore = FALSE ;

      _flowControl = FALSE ;

      _dbMode = 0 ;
      _mainEDU = NULL ;

      for ( INT32 i = 0 ; i < SDB_CB_MAX ; ++i )
      {
         _arrayCBs[ i ] = NULL ;
         _arrayCBs[ i ] = NULL ;
      }
      _init             = FALSE ;
      _isActive         = FALSE ;

      /* <-- internal status, can't be modified by config file --> */
      _dbStatus = SDB_DB_NORMAL ;
      /* <-- external status, can be changed by modifying config file --> */

      _role = SDB_ROLE_STANDALONE ;

      setGroupName ( "" );

      _monCfgCB.timestampON = TRUE ;
      _monDBCB.recordActivateTimestamp () ;

      _optioncb.setConfigHandler( this ) ;
   }

   _SDB_KRCB::~_SDB_KRCB ()
   {
      SDB_ASSERT( _vecEventHandler.size() == 0, "Has some handler not unreg" ) ;
   }

   IParam* _SDB_KRCB::getParam()
   {
      return &_optioncb ;
   }

   IControlBlock* _SDB_KRCB::getCBByType( SDB_CB_TYPE type )
   {
      if ( (INT32)type < 0 || (INT32)type >= SDB_CB_MAX )
      {
         return NULL ;
      }
      return _arrayCBs[ type ] ;
   }

   void* _SDB_KRCB::getOrgPointByType( SDB_CB_TYPE type )
   {
      if ( (INT32)type < 0 || (INT32)type >= SDB_CB_MAX )
      {
         return NULL ;
      }
      return _arrayOrgs[ type ] ;
   }

   BOOLEAN _SDB_KRCB::isCBValue( SDB_CB_TYPE type ) const
   {
      if ( (INT32)type < 0 || (INT32)type >= SDB_CB_MAX )
      {
         return FALSE ;
      }
      return _arrayCBs[ type ] ? TRUE : FALSE ;
   }

   IExecutorMgr* _SDB_KRCB::getExecutorMgr()
   {
      return &_eduMgr ;
   }

   IContextMgr* _SDB_KRCB::getContextMgr()
   {
      IControlBlock *rtnCB = getCBByType( SDB_CB_RTN ) ;
      if ( rtnCB )
      {
         return (IContextMgr*)( rtnCB->queryInterface( SDB_IF_CTXMGR ) ) ;
      }
      return NULL ;
   }

   SDB_DB_STATUS _SDB_KRCB::getDBStatus() const
   {
      return _dbStatus ;
   }

   const CHAR* _SDB_KRCB::getDBStatusDesp() const
   {
      return utilDBStatusStr( _dbStatus ) ;
   }

   BOOLEAN _SDB_KRCB::isNormal() const
   {
      return SDB_DB_NORMAL == _dbStatus ? TRUE : FALSE ;
   }

   BOOLEAN _SDB_KRCB::isAvailable( INT32 *pCode ) const
   {
      INT32 rc = SDB_OK ;
      switch ( _dbStatus )
      {
         case SDB_DB_FULLSYNC :
            rc = SDB_CLS_FULL_SYNC ;
            break ;
         case SDB_DB_REBUILDING :
            rc = SDB_RTN_IN_REBUILD ;
            break ;
         default :
            break ;
      }

      if ( pCode )
      {
         *pCode = rc ;
      }
      return SDB_OK == rc ? TRUE : FALSE ;
   }

   BOOLEAN _SDB_KRCB::isShutdown() const
   {
      return SDB_DB_SHUTDOWN == _dbStatus ? TRUE : FALSE ;
   }

   INT32 _SDB_KRCB::getShutdownCode() const
   {
      return _exitCode ;
   }

   UINT32 _SDB_KRCB::getDBMode() const
   {
      return _dbMode ;
   }

   std::string _SDB_KRCB::getDBModeDesp() const
   {
      return utilDBModeStr( _dbMode ) ;
   }

   BOOLEAN _SDB_KRCB::isDBReadonly() const
   {
      return ( SDB_DB_MODE_READONLY & _dbMode ) ? TRUE : FALSE ;
   }

   BOOLEAN _SDB_KRCB::isDBDeactivated() const
   {
      return ( SDB_DB_MODE_DEACTIVATED & _dbMode ) ? TRUE : FALSE ;
   }

   BOOLEAN _SDB_KRCB::isInFlowControl() const
   {
      return _flowControl ;
   }

   SDB_ROLE _SDB_KRCB::getDBRole() const
   {
      return _role ;
   }

   const CHAR* _SDB_KRCB::getDBRoleDesp() const
   {
      return utilDBRoleStr( _role ) ;
   }

   UINT16 _SDB_KRCB::getLocalPort() const
   {
      return _optioncb.getServicePort() ;
   }

   const CHAR* _SDB_KRCB::getSvcname() const
   {
      return _optioncb.getServiceAddr() ;
   }

   const CHAR* _SDB_KRCB::getDBPath() const
   {
      return _optioncb.getDbPath() ;
   }

   UINT32 _SDB_KRCB::getNodeID() const
   {
      return (UINT32)pmdGetNodeID().columns.nodeID ;
   }

   UINT32 _SDB_KRCB::getGroupID() const
   {
      return (UINT32)pmdGetNodeID().columns.groupID ;
   }

   BOOLEAN _SDB_KRCB::isPrimary() const
   {
      return pmdIsPrimary() ;
   }

   UINT64 _SDB_KRCB::getStartTime() const
   {
      return pmdGetStartTime() ;
   }

   UINT64 _SDB_KRCB::getDBTick() const
   {
      return pmdGetDBTick() ;
   }

   void _SDB_KRCB::getVersion( INT32 &ver, INT32 &subVer, INT32 &fixVer,
                               INT32 &release, const CHAR **build ) const
   {
      ossGetVersion( &ver, &subVer, &fixVer, &release, build ) ;
   }

   INT32 _SDB_KRCB::registerCB( IControlBlock *pCB, void *pOrg )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( pCB, "CB can't be NULL" ) ;
      SDB_ASSERT( FALSE == _init, "Registered cb must be done before "
                                  "KRCB initialization" ) ;

      if ( (INT32)( pCB->cbType () ) < 0 ||
           (INT32)( pCB->cbType () ) >= SDB_CB_MAX )
      {
         SDB_ASSERT ( FALSE, "CB registration should not be out of range" ) ;
         PD_LOG ( PDSEVERE, "Control Block type is not valid: %d",
                  pCB->cbType () ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      _arrayCBs[ pCB->cbType () ] = pCB ;
      _arrayOrgs[ pCB->cbType () ] = pOrg ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _SDB_KRCB::init ()
   {
      INT32 rc = SDB_OK ;
      INT32 index = 0 ;
      IControlBlock *pCB = NULL ;

      rc = _eduMgr.init( this ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init EduMgr failed, rc: %d", rc ) ;
         goto error ;
      }

      _mainEDU = SDB_OSS_NEW pmdEDUCB( &_eduMgr, EDU_TYPE_MAIN ) ;
      if ( !_mainEDU )
      {
         PD_LOG( PDERROR, "Malloc main educb failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _mainEDU->setName( "Main" ) ;

#if defined (_LINUX )
      _mainEDU->setThreadID ( ossPThreadSelf() ) ;
#endif
      _mainEDU->setTID ( ossGetCurrentThreadID() ) ;
      if ( NULL == pmdGetThreadEDUCB() )
      {
         pmdDeclareEDUCB( _mainEDU ) ;
      }

      rc = ossGetHostName( _hostName, OSS_MAX_HOSTNAME ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get host name, rc: %d", rc ) ;

      if ( 0 == _netFrame::getLocalAddress() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get local address, rc: %d", rc ) ;
         goto error ;
      }

      _init = TRUE ;

      _buffPool.setMaxCacheJob( _optioncb.getMaxCacheJob() ) ;
      _buffPool.enablePerfStat( _optioncb.isEnabledPerfStat() ) ;
      rc = _buffPool.init( _optioncb.getMaxCacheSize() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init cache buffer failed, rc: %d", rc ) ;
         goto error ;
      }

      _syncMgr.init( _optioncb.getMaxSyncJob(),
                     _optioncb.isSyncDeep() ) ;

      for ( index = 0 ; index < SDB_CB_MAX ; ++index )
      {
         pCB = _arrayCBs[ index ] ;
         if ( !pCB )
         {
            continue ;
         }
         if ( SDB_OK != ( rc = pCB->init() ) )
         {
            PD_LOG( PDERROR, "Init cb[Type: %d, Name: %s] failed, rc: %d",
                    pCB->cbType(), pCB->cbName(), rc ) ;
            goto error ;
         }
      }

      for ( index = 0 ; index < SDB_CB_MAX ; ++index )
      {
         pCB = _arrayCBs[ index ] ;
         if ( !pCB )
         {
            continue ;
         }
         if ( SDB_OK != ( rc = pCB->active() ) )
         {
            PD_LOG( PDERROR, "Active cb[Type: %d, Name: %s] failed, rc: %d",
                    pCB->cbType(), pCB->cbName(), rc ) ;
            goto error ;
         }
      }

      _isActive = TRUE ;

      _curTime.sample() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _SDB_KRCB::destroy ()
   {
      INT32 rc = SDB_OK ;
      INT32 index = 0 ;
      IControlBlock *pCB = NULL ;
      BOOLEAN normalStop = TRUE ;

      if ( !_init )
      {
         return ;
      }

      _isActive = FALSE ;

      for ( index = SDB_CB_MAX ; index > 0 ; --index )
      {
         pCB = _arrayCBs[ index - 1 ] ;
         if ( !pCB )
         {
            continue ;
         }
         if ( SDB_OK != ( rc = pCB->deactive() ) )
         {
            PD_LOG( PDERROR, "Deactive cb[Type: %d, Name: %s] failed, rc: %d",
                    pCB->cbType(), pCB->cbName(), rc ) ;
         }
      }

      normalStop = _eduMgr.reset() ;

      _syncMgr.syncAndGetLastLSN() ;

      for ( index = SDB_CB_MAX ; index > 0 ; --index )
      {
         pCB = _arrayCBs[ index - 1 ] ;
         if ( !pCB )
         {
            continue ;
         }
         if ( SDB_OK != ( rc = pCB->fini() ) )
         {
            PD_LOG( PDERROR, "Fini cb[Type: %d, Name: %s] failed, rc: %d",
                    pCB->cbType(), pCB->cbName(), rc ) ;
         }
      }

      _buffPool.fini() ;
      _syncMgr.fini() ;

      pmdUndeclareEDUCB() ;

      if ( _mainEDU )
      {
         SDB_OSS_DEL _mainEDU ;
         _mainEDU = NULL ;
      }

      if ( !normalStop && _eduMgr.dumpAbnormalEDU() > 0 )
      {
         PD_LOG( PDSEVERE, "Stop all EDUs timeout, crashed." ) ;
         ossPanic() ;
      }
   }

   void _SDB_KRCB::onConfigChange ( UINT32 changeID )
   {
      INT32 index = 0 ;
      IControlBlock *pCB = NULL ;

      for ( index = 0 ; index < SDB_CB_MAX ; ++index )
      {
         pCB = _arrayCBs[ index ] ;
         if ( !pCB )
         {
            continue ;
         }
         pCB->onConfigChange() ;
      }
   }

   void _SDB_KRCB::onConfigSave()
   {
      INT32 index = 0 ;
      IControlBlock *pCB = NULL ;

      for ( index = 0 ; index < SDB_CB_MAX ; ++index )
      {
         pCB = _arrayCBs[ index ] ;
         if ( !pCB )
         {
            continue ;
         }
         pCB->onConfigSave() ;
      }
   }

   INT32 _SDB_KRCB::onConfigInit ()
   {
      _role = utilGetRoleEnum( _optioncb.krcbRole() ) ;
      pmdSetDBRole( _role ) ;

      if ( _optioncb.isTraceOn() && _optioncb.traceBuffSize() != 0 )
      {
         sdbGetPDTraceCB()->start ( (UINT64)_optioncb.traceBuffSize(),
                                    0xFFFFFFFF ) ;
      }

      ossEnableMemDebug( _optioncb.memDebugEnabled(),
                         _optioncb.memDebugSize() ) ;

      return _optioncb.makeAllDir() ;
   }

   INT32 _SDB_KRCB::regEventHandler( IEventHander *pHandler,
                                     UINT32 mask )
   {
      if ( !pHandler ) return SDB_INVALIDARG ;

      ossScopedLock lock ( &_handlerLatch, EXCLUSIVE ) ;
      for ( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
      {
         if ( _vecEventHandler[ i ].first == pHandler )
         {
            return SDB_SYS ;
         }
      }
      _vecEventHandler.push_back( EVENT_HANDLER_INFO( pHandler, mask ) ) ;
      return SDB_OK ;
   }

   void _SDB_KRCB::unregEventHandler( IEventHander *pHandler )
   {
      if ( !pHandler ) return ;

      ossScopedLock lock ( &_handlerLatch, EXCLUSIVE ) ;
      VEC_EVENTHANDLER::iterator it ;
      for ( it = _vecEventHandler.begin() ;
            it != _vecEventHandler.end() ;
            ++it )
      {
         if ( (*it).first == pHandler )
         {
            _vecEventHandler.erase( it ) ;
            break ;
         }
      }
   }

   void _SDB_KRCB::callRegisterEventHandler( const MsgRouteID &nodeID )
   {
      ossScopedLock lock ( &_handlerLatch, SHARED ) ;
      for ( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
      {
         if ( _vecEventHandler[ i ].second & EVENT_MASK_ON_REGISTERED )
         {
            ( _vecEventHandler[ i ].first )->onRegistered( nodeID ) ;
         }
      }
   }

   void _SDB_KRCB::callPrimaryChangeHandler( BOOLEAN primary,
                                             SDB_EVENT_OCCUR_TYPE type )
   {
      ossScopedLock lock ( &_handlerLatch, SHARED ) ;
      for ( UINT32 i = 0 ; i < _vecEventHandler.size() ; ++i )
      {
         if ( _vecEventHandler[ i ].second & EVENT_MASK_ON_PRIMARYCHG )
         {
            ( _vecEventHandler[ i ].first )->onPrimaryChange( primary, type ) ;
         }
      }
   }

   ossTick _SDB_KRCB::getCurTime()
   {
      return _curTime ;
   }

   void _SDB_KRCB::syncCurTime()
   {
      _curTime.sample() ;
   }

   /*
    * kernel control block
    */
   pmdKRCB pmd_krcb ;
}

