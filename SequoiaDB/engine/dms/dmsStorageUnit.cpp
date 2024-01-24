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

   Source File Name = dmsStorageUnit.cpp

   Descriptive Name = Data Management Service Storage Unit

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data insert/update/delete. This file does NOT include index logic.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageUnit.hpp"
#include "dmsScanner.hpp"
#include "mthModifier.hpp"
#include "mthMatchRuntime.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageLob.hpp"
#include "pmdStartup.hpp"
#include "dmsStorageDataFactory.hpp"
#include "dmsTransContext.hpp"
#include "dmsOprHandler.hpp"
#include "utilMath.hpp"

namespace engine
{

   static OSS_THREAD_LOCAL BOOLEAN s_isInCallback = FALSE ;

   class _dmsCallbackShield
   {
   public:
      _dmsCallbackShield()
      {
         if ( !s_isInCallback )
         {
            s_isInCallback = TRUE ;
            _isRecursive = FALSE ;
         }
         else
         {
            _isRecursive = TRUE ;
         }
      }

      ~_dmsCallbackShield()
      {
         if ( !_isRecursive )
         {
            s_isInCallback = FALSE ;
         }
      }

      BOOLEAN isRecursive()
      {
         return _isRecursive ;
      }

   protected:
      BOOLEAN _isRecursive ;
   } ;

   typedef class _dmsCallbackShield dmsCallbackShield ;

   /*
      _dmsEventHolder implement
    */
   _dmsEventHolder::_dmsEventHolder ( dmsStorageUnit *su )
   : _handlers( NULL )
   {
      SDB_ASSERT( su, "Storage Unit is no valid" ) ;
      _su = su ;
   }

   _dmsEventHolder::~_dmsEventHolder ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTCS, "_dmsEventHolder::onCreateCS" )
   INT32 _dmsEventHolder::onCreateCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTCS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onCreateCS( this, _pCacheHolder, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCRTCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONLOADCS, "_dmsEventHolder::onLoadCS" )
   INT32 _dmsEventHolder::onLoadCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONLOADCS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onLoadCS( this, _pCacheHolder, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONLOADCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONUNLOADCS, "_dmsEventHolder::onUnloadCS" )
   INT32 _dmsEventHolder::onUnloadCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONUNLOADCS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onUnloadCS( this, _pCacheHolder, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONUNLOADCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONRENAMECS, "_dmsEventHolder::onRenameCS" )
   INT32 _dmsEventHolder::onRenameCS ( UINT32 mask, const CHAR *pOldCSName,
                                       const CHAR *pNewCSName, pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONRENAMECS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onRenameCS( this, _pCacheHolder, pOldCSName,
                                                pNewCSName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONRENAMECS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCHECKDROPCS, "_dmsEventHolder::onCheckDropCS" )
   INT32 _dmsEventHolder::onCheckDropCS( UINT32 mask,
                                         const dmsEventSUItem &suItem,
                                         dmsDropCSOptions *options,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCHECKDROPCS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            rc = pHandler->onCheckDropCS( this, _pCacheHolder, suItem,
                                          options, cb, dpsCB ) ;
            PD_RC_CHECK( rc,  PDERROR, "Failed to call check drop "
                         "collection space event in handle [%s],rc: %d",
                         pHandler->getName(), rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCHECKDROPCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPCS, "_dmsEventHolder::onDropCS" )
   INT32 _dmsEventHolder::onDropCS ( UINT32 mask,
                                     SDB_EVENT_OCCUR_TYPE type,
                                     const dmsEventSUItem &suItem,
                                     dmsDropCSOptions *options,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPCS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            rc = pHandler->onDropCS( type, this, _pCacheHolder, suItem,
                                     options, cb, dpsCB ) ;
            PD_RC_CHECK( rc,  PDERROR, "Failed to call [%s] drop collection "
                         "space event in handle [%s],rc: %d",
                         SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                         pHandler->getName(), rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLEANDROPCS, "_dmsEventHolder::onCleanDropCS" )
   INT32 _dmsEventHolder::onCleanDropCS( UINT32 mask,
                                         const dmsEventSUItem &suItem,
                                         dmsDropCSOptions *options,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLEANDROPCS ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmpRC = pHandler->onCleanDropCS( this, _pCacheHolder, suItem,
                                                    options, cb, dpsCB ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to call clean drop collection space"
                       "event in handle [%s],rc: %d",
                       pHandler->getName(), tmpRC ) ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLEANDROPCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTCL, "_dmsEventHolder::onCreateCL" )
   INT32 _dmsEventHolder::onCreateCL ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onCreateCL( this, _pCacheHolder, clItem,
                                                cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCRTCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONRENAMECL, "_dmsEventHolder::onRenameCL" )
   INT32 _dmsEventHolder::onRenameCL ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       const CHAR *pNewCLName,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONRENAMECL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onRenameCL( this, _pCacheHolder, clItem,
                                                pNewCLName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONRENAMECL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCHECKTRUNCCL, "_dmsEventHolder::onCheckTruncCL" )
   INT32 _dmsEventHolder::onCheckTruncCL( UINT32 mask,
                                          const dmsEventCLItem &clItem,
                                          dmsTruncCLOptions *options,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCHECKTRUNCCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            rc = pHandler->onCheckTruncCL( this, _pCacheHolder, clItem,
                                           options, cb, dpsCB ) ;
            PD_RC_CHECK( rc,  PDERROR, "Failed to call check truncate "
                         "collection event in handle [%s],rc: %d",
                         pHandler->getName(), rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCHECKTRUNCCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONTRUNCCL, "_dmsEventHolder::onTruncateCL" )
   INT32 _dmsEventHolder::onTruncateCL ( UINT32 mask,
                                         SDB_EVENT_OCCUR_TYPE type,
                                         const dmsEventCLItem &clItem,
                                         dmsTruncCLOptions *options,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONTRUNCCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            rc = pHandler->onTruncateCL( type, this, _pCacheHolder, clItem,
                                         options, cb, dpsCB ) ;
            PD_RC_CHECK( rc,  PDERROR, "Failed to call [%s] truncate "
                         "collection event in handle [%s], rc: %d",
                         SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                         pHandler->getName(), rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONTRUNCCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLEANTRUNCCL, "_dmsEventHolder::onCleanTruncCL" )
   INT32 _dmsEventHolder::onCleanTruncCL( UINT32 mask,
                                          const dmsEventCLItem &clItem,
                                          dmsTruncCLOptions *options,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLEANTRUNCCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmpRC = pHandler->onCleanTruncCL( this, _pCacheHolder, clItem,
                                                    options, cb, dpsCB ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to call clean truncate collection "
                       "event in handle [%s],rc: %d",
                       pHandler->getName(), tmpRC ) ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLEANTRUNCCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCHECKDROPCL, "_dmsEventHolder::onCheckDropCL" )
   INT32 _dmsEventHolder::onCheckDropCL( UINT32 mask,
                                         const dmsEventCLItem &clItem,
                                         dmsDropCLOptions *options,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCHECKDROPCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            rc = pHandler->onCheckDropCL( this, _pCacheHolder, clItem,
                                          options, cb, dpsCB ) ;
            PD_RC_CHECK( rc,  PDERROR, "Failed to call check drop collection "
                         "event in handle [%s],rc: %d",
                         pHandler->getName(), rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCHECKDROPCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPCL, "_dmsEventHolder::onDropCL" )
   INT32 _dmsEventHolder::onDropCL ( UINT32 mask,
                                     SDB_EVENT_OCCUR_TYPE type,
                                     const dmsEventCLItem &clItem,
                                     dmsDropCLOptions *options,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            rc = pHandler->onDropCL( type, this, _pCacheHolder, clItem,
                                     options, cb, dpsCB ) ;
            PD_RC_CHECK( rc,  PDERROR, "Failed to call [%s] drop collection "
                       "event in handle [%s],rc: %d",
                       SDB_EVT_OCCUR_BEFORE == type ? "before" : "after",
                       pHandler->getName(), rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLEANDROPCL, "_dmsEventHolder::onCleanDropCL" )
   INT32 _dmsEventHolder::onCleanDropCL( UINT32 mask,
                                         const dmsEventCLItem &clItem,
                                         dmsDropCLOptions *options,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLEANDROPCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmpRC = pHandler->onCleanDropCL( this, _pCacheHolder, clItem,
                                                   options, cb, dpsCB ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to call clean drop collection "
                       "event in handle [%s],rc: %d",
                       pHandler->getName(), tmpRC ) ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLEANDROPCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTIDX, "_dmsEventHolder::onCreateIndex" )
   INT32 _dmsEventHolder::onCreateIndex ( UINT32 mask,
                                          const dmsEventCLItem &clItem,
                                          const dmsEventIdxItem &idxItem,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTIDX ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onCreateIndex( this, _pCacheHolder, clItem,
                                                   idxItem, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCRTIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONREBUILDIDX, "_dmsEventHolder::onRebuildIndex" )
   INT32 _dmsEventHolder::onRebuildIndex ( UINT32 mask,
                                           const dmsEventCLItem &clItem,
                                           const dmsEventIdxItem &idxItem,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONREBUILDIDX ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onRebuildIndex( this, _pCacheHolder,
                                                    clItem, idxItem, cb,
                                                    dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONREBUILDIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPIDX, "_dmsEventHolder::onDropIndex" )
   INT32 _dmsEventHolder::onDropIndex ( UINT32 mask,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPIDX ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onDropIndex( this, _pCacheHolder, clItem,
                                                 idxItem, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONLINKCL, "_dmsEventHolder::onLinkCL" )
   INT32 _dmsEventHolder::onLinkCL ( UINT32 mask,
                                     const dmsEventCLItem &clItem,
                                     const CHAR *pMainCLName,
                                     _pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONLINKCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onLinkCL( this, _pCacheHolder, clItem,
                                              pMainCLName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONLINKCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONUNLINKCL, "_dmsEventHolder::onUnlinkCL" )
   INT32 _dmsEventHolder::onUnlinkCL ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       const CHAR *pMainCLName,
                                       _pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONUNLINKCL ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }

      // Event could not be handled in main thread
      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onUnlinkCL( this, _pCacheHolder, clItem,
                                                pMainCLName, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONUNLINKCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLRSUCACHES, "_dmsEventHolder::onClearSUCaches" )
   INT32 _dmsEventHolder::onClearSUCaches ( UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLRSUCACHES ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onClearSUCaches( this, _pCacheHolder ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLRSUCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLRCLCACHES, "_dmsEventHolder::onClearCLCaches" )
   INT32 _dmsEventHolder::onClearCLCaches ( UINT32 mask,
                                            const dmsEventCLItem &clItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLRCLCACHES ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onClearCLCaches( this, _pCacheHolder,
                                                     clItem ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLRCLCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCHGSUCACHES, "_dmsEventHolder::onChangeSUCaches" )
   INT32 _dmsEventHolder::onChangeSUCaches ( UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCHGSUCACHES ) ;

      // avoid recursively calling
      dmsCallbackShield shield ;
      if ( shield.isRecursive() )
      {
         goto done ;
      }
      else if ( NULL == _handlers )
      {
         goto done ;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers->begin() ;
            iter != _handlers->end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onChangeSUCaches( this, _pCacheHolder ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCHGSUCACHES, rc ) ;

      return rc ;
   }

   const CHAR *_dmsEventHolder::getCSName () const
   {
      return _su->CSName() ;
   }

   UINT32 _dmsEventHolder::getSUID () const
   {
      return _su->CSID() ;
   }

   UINT32 _dmsEventHolder::getSULID () const
   {
      return _su->LogicalCSID() ;
   }

   /*
      _dmsCacheHolder implement
    */
   _dmsCacheHolder::_dmsCacheHolder ( dmsStorageUnit *su )
   : IDmsSUCacheHolder()
   {
      SDB_ASSERT( su, "Storage Unit is not valid" ) ;

      _su = su ;
      ossMemset( _pSUCaches, 0, sizeof( _pSUCaches ) ) ;
   }

   _dmsCacheHolder::~_dmsCacheHolder ()
   {
      deleteAllSUCaches() ;
   }

   const CHAR *_dmsCacheHolder::getCSName () const
   {
      return _su->CSName() ;
   }

   UINT32 _dmsCacheHolder::getSUID () const
   {
      return _su->CSID() ;
   }

   UINT32 _dmsCacheHolder::getSULID () const
   {
      return _su->LogicalCSID() ;
   }

   BOOLEAN _dmsCacheHolder::isSysSU () const
   {
      return dmsIsSysCSName( getCSName() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKUNIT, "_dmsCacheHolder::checkCacheUnit" )
   BOOLEAN _dmsCacheHolder::checkCacheUnit ( utilSUCacheUnit *pCacheUnit )
   {
      BOOLEAN exists = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CHKUNIT ) ;

      switch ( pCacheUnit->getUnitType() )
      {
         case UTIL_SU_CACHE_UNIT_CLSTAT :
         {
            if ( SDB_OK != _checkCollectionStat( (dmsCollectionStat *)pCacheUnit ) )
            {
               PD_LOG( PDWARNING, "Failed to check collection statistics" ) ;
               goto error ;
            }
            exists = TRUE ;
            break ;
         }
         case UTIL_SU_CACHE_UNIT_IXSTAT :
         {
            if ( SDB_OK != _checkIndexStat( (dmsIndexStat *)pCacheUnit , NULL ) )
            {
               PD_LOG( PDWARNING, "Failed to check index statistics" ) ;
               goto error ;
            }
            exists = TRUE ;
            break ;
         }
         case UTIL_SU_CACHE_UNIT_CLPLAN :
            exists = TRUE ;
            break ;
         default :
            break ;
      }

   done :
      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_CHKUNIT ) ;
      return exists ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CRTCACHE, "_dmsCacheHolder::createSUCache" )
   BOOLEAN _dmsCacheHolder::createSUCache ( UINT8 type )
   {
      BOOLEAN created = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CRTCACHE ) ;

      if ( type < DMS_CACHE_TYPE_NUM &&
           NULL == _pSUCaches[ type ] )
      {
         BOOLEAN needCreate = TRUE ;
         switch ( type )
         {
            case DMS_CACHE_TYPE_STAT :
            {
               if ( !isSysSU() )
               {
                  _pSUCaches[ type ] = SDB_OSS_NEW dmsStatCache( this ) ;
               }
               else
               {
                  needCreate = FALSE ;

               }
               break ;
            }
            case DMS_CACHE_TYPE_PLAN :
            {
               _pSUCaches[ type ] = SDB_OSS_NEW dmsCachedPlanMgr( this ) ;
               break ;
            }
            default :
            {
               SDB_ASSERT( FALSE, "Invalid switch branch" ) ;
               break ;
            }
         }
         if ( _pSUCaches[ type ] )
         {
            created = TRUE ;
         }
         else if ( needCreate )
         {
            PD_LOG( PDWARNING, "Failed to create cache unit [%u] for CS %s",
                    type, getCSName() ) ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_CRTCACHE ) ;

      return created ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_DELCACHE, "_dmsCacheHolder::deleteSUCache" )
   BOOLEAN _dmsCacheHolder::deleteSUCache ( UINT8 type )
   {
      BOOLEAN deleted = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_DELCACHE ) ;

      if ( type < DMS_CACHE_TYPE_NUM && NULL != _pSUCaches[ type ] )
      {
         if ( _pSUCaches[ type ] != NULL )
         {
            _pSUCaches[ type ]->clearCacheUnits() ;
            SDB_OSS_DEL _pSUCaches[ type ] ;
            _pSUCaches[ type ] = NULL ;
            deleted = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_DELCACHE ) ;

      return deleted ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_DELALLCACHES, "_dmsCacheHolder::deleteAllSUCaches" )
   void _dmsCacheHolder::deleteAllSUCaches ()
   {
      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_DELALLCACHES ) ;

      for ( UINT8 type = 0 ; type < DMS_CACHE_TYPE_NUM ; type ++ )
      {
         if ( _pSUCaches[ type ] != NULL )
         {
            _pSUCaches[ type ]->clearCacheUnits() ;
            SDB_OSS_DEL _pSUCaches[ type ] ;
            _pSUCaches[ type ] = NULL ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSCACHEHOLDER_DELALLCACHES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKCLSTAT, "_dmsCacheHolder::_checkCollectionStat" )
   INT32 _dmsCacheHolder::_checkCollectionStat( dmsCollectionStat *pCollectionStat )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CHKCLSTAT ) ;

      SDB_ASSERT( pCollectionStat, "pCollectionStat is invalid" ) ;

      dmsMBContext *mbContext = NULL ;
      const CHAR *pCSName = pCollectionStat->getCSName() ;
      const CHAR *pCLName = pCollectionStat->getCLName() ;
      INDEX_STAT_MAP &indexStats = pCollectionStat->getIndexStats() ;
      INDEX_STAT_ITERATOR iterIdx ;

      BOOLEAN needCheck =
            ( pCollectionStat->getMBID() != UTIL_SU_INVALID_UNITID ) ;

      if ( needCheck )
      {
         PD_CHECK( _su->LogicalCSID() == pCollectionStat->getSULogicalID(),
                   SDB_DMS_CS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection space [%s] for statistics", pCSName ) ;
      }
      else
      {
         pCollectionStat->setSULogicalID( _su->LogicalCSID() ) ;
      }

      rc = _su->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get collection [%s], rc: %d",
                   pCLName, rc ) ;

      if ( needCheck )
      {
         PD_CHECK( mbContext->mbID() == pCollectionStat->getMBID() &&
                   mbContext->clLID() == pCollectionStat->getCLLogicalID(),
                   SDB_DMS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection [%s.%s] for statistics", pCSName, pCLName ) ;
      }
      else
      {
         pCollectionStat->setMBID( mbContext->mbID() ) ;
         pCollectionStat->setCLLogicalID( mbContext->clLID() ) ;
      }

      iterIdx = indexStats.begin() ;
      while ( iterIdx != indexStats.end() )
      {
         dmsIndexStat *pIndexStat = iterIdx->second ;
         if ( SDB_OK != _checkIndexStat( pIndexStat, mbContext ) )
         {
            // Remove field statistics reference
            pCollectionStat->removeFieldStat( pIndexStat->getFirstField(),
                                              TRUE ) ;
            // Remove index statistics reference
            iterIdx = indexStats.erase( iterIdx ) ;
            // Delete the index statistics
            SAFE_OSS_DELETE( pIndexStat ) ;
         }
         else
         {
            ++ iterIdx ;
         }
      }

   done :
      if ( mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSCACHEHOLDER_CHKCLSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKIDXSTAT, "_dmsCacheHolder::_checkIndexStat" )
   INT32 _dmsCacheHolder::_checkIndexStat ( dmsIndexStat *pIndexStat,
                                            dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSCACHEHOLDER_CHKIDXSTAT ) ;

      BOOLEAN needAllocate = !mbContext ;
      dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;
      const CHAR *pCSName = pIndexStat->getCSName() ;
      const CHAR *pCLName = pIndexStat->getCLName() ;
      const CHAR *pIndexName = pIndexStat->getIndexName() ;

      BOOLEAN needCheck = ( pIndexStat->getMBID() != UTIL_SU_INVALID_UNITID ) ;

      if ( needCheck )
      {
         PD_CHECK( _su->LogicalCSID() == pIndexStat->getSULogicalID(),
                   SDB_DMS_CS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection space [%s] for statistics", pCSName ) ;
      }
      else
      {
         pIndexStat->setSULogicalID( _su->LogicalCSID() ) ;
      }

      if ( !mbContext )
      {
         rc = _su->data()->getMBContext( &mbContext, pCLName, SHARED ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get collection [%s], rc: %d",
                      pCLName, rc ) ;
      }

      if ( needCheck )
      {
         PD_CHECK( mbContext->mbID() == pIndexStat->getMBID() &&
                   mbContext->clLID() == pIndexStat->getCLLogicalID(),
                   SDB_DMS_NOTEXIST, error, PDWARNING, "Failed to get "
                   "collection [%s.%s] for statistics", pCSName, pCLName ) ;
      }
      else
      {
         pIndexStat->setMBID( mbContext->mbID() ) ;
         pIndexStat->setCLLogicalID( mbContext->clLID() ) ;
      }

      rc = _su->index()->getIndexCBExtent( mbContext, pIndexName, indexCBExtent ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index [%s], rc: %d",
                   pIndexName, rc ) ;

      {
         ixmIndexCB indexCB ( indexCBExtent, _su->index(), NULL ) ;

         PD_CHECK( indexCB.isInitialized(),
                   SDB_DMS_INIT_INDEX, error, PDWARNING,
                   "Index [%s] is invalid", pIndexName ) ;
         PD_CHECK( indexCB.getFlag() == IXM_INDEX_FLAG_NORMAL,
                   SDB_IXM_UNEXPECTED_STATUS, error, PDDEBUG,
                   "Index [%s] is not normal status",pIndexName ) ;

         if ( needCheck )
         {
            PD_CHECK( pIndexStat->getIndexLogicalID() == indexCB.getLogicalID(),
                      SDB_IXM_NOTEXIST, error, PDWARNING,
                      "Logical ID of index [%s] are not matched", pIndexName ) ;
         }
         else
         {
            pIndexStat->setIndexLogicalID( indexCB.getLogicalID() ) ;
         }

         PD_CHECK( 0 == pIndexStat->getKeyPattern().woCompare(
                               indexCB.keyPattern(), BSONObj(), TRUE ),
                   SDB_IXM_NOTEXIST, error, PDWARNING,
                   "Keys of index [%s] are not matched", pIndexName ) ;
      }

   done :
      if ( needAllocate && mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSCACHEHOLDER_CHKIDXSTAT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU, "_dmsStorageUnit::_dmsStorageUnit" )
   _dmsStorageUnit::_dmsStorageUnit ( IStorageService *storageService,
                                      const CHAR *pSUName,
                                      UINT32 csUniqueID,
                                      UINT32 sequence,
                                      utilCacheMgr *pMgr,
                                      INT32 pageSize,
                                      INT32 lobPageSize,
                                      DMS_STORAGE_TYPE type,
                                      IDmsExtDataHandler *extDataHandler )
   :_dmsSUDescriptor( pSUName,
                      csUniqueID,
                      sequence,
                      pageSize,
                      lobPageSize,
                      type,
                      pmdGetOptionCB()->getOverFlowRatio(),
                      pmdGetOptionCB()->getExtendThreshold() << 20,
                      pmdGetOptionCB()->sparseFile(),
                      pmdGetOptionCB()->useDirectIOInLob(),
                      pmdGetOptionCB()->getCacheMergeSize(),
                      pmdGetOptionCB()->getPageAllocTimeout(),
                      pmdGetStartup().isOK(),
                      pmdGetSyncMgr()->getCompleteLSN(),
                      extDataHandler ),
    _storageService( storageService ),
    _pDataSu( NULL ),
    _pIndexSu( NULL ),
    _pLobSu( NULL ),
    _pMgr( pMgr ),
    _pCacheUnit( NULL ),
    _eventHolder ( this ),
    _cacheHolder ( this )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU ) ;

      // Create caches
      _cacheHolder.createSUCache( DMS_CACHE_TYPE_STAT ) ;
      _cacheHolder.createSUCache( DMS_CACHE_TYPE_PLAN ) ;
      _eventHolder.setCacheHolder( &_cacheHolder ) ;

      PD_TRACE_EXIT ( SDB__DMSSU ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DESC, "_dmsStorageUnit::~_dmsStorageUnit" )
   _dmsStorageUnit::~_dmsStorageUnit()
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DESC ) ;
      close() ;

      _cacheHolder.deleteAllSUCaches() ;

      if ( _pIndexSu )
      {
         SDB_OSS_DEL _pIndexSu ;
         _pIndexSu = NULL ;
      }
      if ( _pLobSu )
      {
         SDB_OSS_DEL _pLobSu ;
         _pLobSu = NULL ;
      }
      if ( _pCacheUnit )
      {
         SDB_OSS_DEL _pCacheUnit ;
         _pCacheUnit = NULL ;
      }
      // _pDataSu must be delete at the last
      if ( _pDataSu )
      {
         SDB_OSS_DEL _pDataSu ;
         _pDataSu = NULL ;
      }
      PD_TRACE_EXIT ( SDB__DMSSU_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_OPEN, "_dmsStorageUnit::open" )
   INT32 _dmsStorageUnit::open( const CHAR *pDataPath,
                                const CHAR *pIndexPath,
                                const CHAR *pLobPath,
                                const CHAR *pLobMetaPath,
                                IDataSyncManager *pSyncMgr,
                                IDataStatManager *pStatMgr,
                                BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_OPEN ) ;

      // If openning existing storage unit, get the type from the header.
      if ( !createNew )
      {
         rc = _getTypeFromFile( pDataPath, _storageInfo._type ) ;
         PD_RC_CHECK( rc, PDERROR, "Get type for storage unit[ %s ] from "
                      "file failed[ %d ]", _storageInfo._suName, rc ) ;
      }

      rc = _createStorageObjs() ;
      PD_RC_CHECK( rc, PDERROR, "Create storage objects for storage unit[ %s ] "
                   "failed[ %d ]", _storageInfo._suName, rc ) ;

      // open data
      rc = _pDataSu->openStorage( pDataPath, pSyncMgr, pStatMgr, createNew ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open storage data su failed, rc: %d", rc ) ;
         if ( createNew && SDB_FE != rc )
         {
            goto rmdata ;
         }
         goto error ;
      }

      // open index
      rc = _pIndexSu->openStorage( pIndexPath, pSyncMgr, pStatMgr, createNew ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open storage index su failed, rc: %d", rc ) ;
         if ( createNew )
         {
            if ( SDB_FE != rc )
            {
               goto rmboth ;
            }
            goto rmdata ;
         }
         else if ( SDB_FNE == rc &&
                   _pDataSu->isCrashed() &&
                   0 == _pDataSu->getCollectionNum() )
         {
            /// create data file then crashed, so clean the data file
            goto rmdata ;
         }
         goto error ;
      }

      // open lob
      rc = _pLobSu->open( pLobPath, pLobMetaPath, pSyncMgr, pStatMgr, createNew ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open storage lob, rc:%d", rc ) ;
         if ( createNew )
         {
            goto rmboth ;
         }
         goto error ;
      }

      _storageInfo._createTime = _pDataSu->getCreateTime() ;
      _storageInfo._updateTime = _pDataSu->getUpdateTime() ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_OPEN, rc ) ;
      return rc ;
   error:
      close() ;
      goto done ;
   rmdata:
      {
         INT32 rcTmp = _pDataSu->removeStorage() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Failed to remove cs data file[%s] in "
                    "rollback, rc: %d", _pDataSu->getSuFileName(), rcTmp ) ;
         }
      }
      goto done ;
   rmboth:
      {
         INT32 rcTmp = _pIndexSu->removeStorage() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Failed to remove cs index file[%s] in "
                    "rollback, rc: %d", _pIndexSu->getSuFileName(), rcTmp ) ;
         }
      }
      goto rmdata ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CLOSE, "_dmsStorageUnit::close" )
   void _dmsStorageUnit::close()
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_CLOSE ) ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      /// The order is:
      /// cacheUnit -> lob -> index -> data( must be in last )
      if ( _pCacheUnit )
      {
         _pCacheUnit->fini( cb ) ;
      }
      if ( _pLobSu )
      {
         _pLobSu->closeStorage() ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->closeStorage() ;
      }
      if ( _pDataSu )
      {
         _pDataSu->closeStorage() ;
      }
      PD_TRACE_EXIT ( SDB__DMSSU_CLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_REMOVE, "_dmsStorageUnit::remove" )
   INT32 _dmsStorageUnit::remove ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_REMOVE ) ;

      /// The order is:
      /// cacheUnit -> lob -> index -> data( must be in last )

      if ( _pCacheUnit )
      {
         _pCacheUnit->dropDirty() ;
      }

      if ( _pLobSu )
      {
         _pLobSu->removeStorageFiles() ;
      }

      if ( _pIndexSu )
      {
         /// first clear all page map
         _pIndexSu->getPageMapUnit()->clear() ;

         rc = _pIndexSu->removeStorage() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove collection space[%s] "
                      "index file, rc: %d", CSName(), rc ) ;
      }

      if ( _pDataSu )
      {
         rc = _pDataSu->removeStorage() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove collection space[%s] "
                      "data file, rc: %d", CSName(), rc ) ;
      }

      PD_LOG( PDEVENT, "Remove collection space[%s] files succeed", CSName() ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_REMOVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_RENAMECS, "_dmsStorageUnit::renameCS" )
   INT32 _dmsStorageUnit::renameCS( const CHAR *pNewName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_RENAMECS ) ;

      CHAR dataFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;
      CHAR idxFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu || !_pCacheUnit )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory failed" ) ;
         goto error ;
      }

      /// data and index
      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_DATA_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_INDEX_SU_EXT_NAME ) ;

      rc = _pDataSu->renameStorage( pNewName, dataFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename storage data failed, rc: %d", rc ) ;
         goto error ;
      }
      rc = _pIndexSu->renameStorage( pNewName, idxFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename storage index failed, rc: %d", rc ) ;
         goto error ;
      }

      /// lobm and lobd
      ossMemset( dataFileName, 0, sizeof( dataFileName ) ) ;
      ossMemset( idxFileName, 0 , sizeof( idxFileName ) ) ;
      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_LOB_META_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_LOB_DATA_SU_EXT_NAME ) ;

      rc = _pLobSu->rename( pNewName, dataFileName, idxFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename storage lob failed, rc: %d", rc ) ;
         goto error ;
      }

      /// update storage info
      ossStrncpy( _storageInfo._suName, pNewName, DMS_SU_NAME_SZ ) ;
      _storageInfo._suName[DMS_SU_NAME_SZ] = 0 ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_RENAMECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETLOBPAGESIZE, "_dmsStorageUnit::setLobPageSize" )
   INT32 _dmsStorageUnit::setLobPageSize ( UINT32 lobPageSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETLOBPAGESIZE ) ;

      PD_CHECK( DMS_DO_NOT_CREATE_LOB != _storageInfo._lobdPageSize,
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "LOB is not enabled" ) ;

      PD_CHECK( DMS_PAGE_SIZE4K == lobPageSize ||
                DMS_PAGE_SIZE8K == lobPageSize ||
                DMS_PAGE_SIZE16K == lobPageSize ||
                DMS_PAGE_SIZE32K == lobPageSize ||
                DMS_PAGE_SIZE64K == lobPageSize ||
                DMS_PAGE_SIZE128K == lobPageSize ||
                DMS_PAGE_SIZE256K == lobPageSize ||
                DMS_PAGE_SIZE512K == lobPageSize,
                SDB_INVALIDARG, error, PDERROR,
                "LOB page size should be 4K/8K/16K/32K/64K/128K/256K/512K" ) ;

      PD_CHECK( !_pLobSu->isOpened(), SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "LOB storage is already opened, could not change" ) ;

      rc = _pDataSu->setLobPageSize( lobPageSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set LOB page size in data storage "
                   "unit, rc: %d", rc ) ;

      rc = _pIndexSu->setLobPageSize( lobPageSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set LOB page size in index storage "
                   "unit, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__DMSSU_SETLOBPAGESIZE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__RESETCOLLECTION, "_dmsStorageUnit::_resetCollection" )
   INT32 _dmsStorageUnit::_resetCollection( dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU__RESETCOLLECTION ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;

      // drop all indexes
      rc = _pIndexSu->dropAllIndexes( context, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Drop all indexes failed, rc: %d", rc ) ;
         // don't go to error, continue
      }

      rc = _pDataSu->_truncateCollection( context ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Truncate collection data failed, rc: %d", rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU__RESETCOLLECTION, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_INSERTRECORD, "_dmsStorageUnit::insertRecord" )
   INT32 _dmsStorageUnit::insertRecord ( const CHAR *pName,
                                         const BSONObj &record,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpscb,
                                         BOOLEAN mustOID,
                                         BOOLEAN canUnLock,
                                         dmsMBContext *context,
                                         INT64 position,
                                         utilInsertResult *insertResult )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_INSERTRECORD ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      if ( NULL != cb )
      {
         cb->registerMonCRUDCB( &( context->mbStat()->_crudCB ) ) ;
      }

      rc = _pDataSu->insertRecord( context, record, cb, dpscb, mustOID,
                                   canUnLock, position, insertResult ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( NULL != cb )
      {
         cb->unregisterMonCRUDCB() ;
      }
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_INSERTRECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_RECYCS, "_dmsStorageUnit::recycleCollectionSpace" )
   INT32 _dmsStorageUnit::recycleCollectionSpace( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_RECYCS ) ;

      for ( UINT16 mbID = 0 ; mbID < DMS_MME_SLOTS ; ++mbID )
      {
         if ( DMS_IS_MB_INUSE ( _pDataSu->_dmsMME->_mbList[ mbID ]._flag ) )
         {
            INT32 tmpRC = SDB_OK ;

            dmsMBContext *mbContext = NULL ;

            tmpRC = _pDataSu->getMBContext( &mbContext, mbID,
                                            DMS_INVALID_CLID, DMS_INVALID_CLID ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to get metablock context for "
                       "collection on slot [%u], rc: %d", mbID, tmpRC ) ;
               continue ;
            }

            tmpRC = mbContext->mbTryLock( EXCLUSIVE ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to lock collection on slot [%u], "
                       "rc: %d", mbID, tmpRC ) ;
               _pDataSu->releaseMBContext( mbContext ) ;
               continue ;
            }

            // drop all indexes with external data
            // ( text index and global index )
            tmpRC = _pDataSu->_dropIndexesWithTypes( mbContext, cb,
                                                     ( IXM_EXTENT_TYPE_TEXT |
                                                       IXM_EXTENT_TYPE_GLOBAL ) ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to drop indexes with external data "
                       "from collection [%s], rc: %d",
                       mbContext->mb()->_collectionName, tmpRC ) ;
            }

            _pDataSu->releaseMBContext( mbContext ) ;

         }
      }

      PD_TRACE_EXITRC( SDB__DMSSU_RECYCS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_REBUILDINDEXES, "_dmsStorageUnit::rebuildIndexes" )
   INT32 _dmsStorageUnit::rebuildIndexes( const CHAR *pName,
                                          pmdEDUCB * cb,
                                          dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_REBUILDINDEXES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->rebuildIndexes( context, cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_REBUILDINDEXES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CREATEINDEX, "_dmsStorageUnit::createIndex" )
   INT32 _dmsStorageUnit::createIndex( const CHAR *pName, const BSONObj &index,
                                       pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                       BOOLEAN isSys, dmsMBContext * context,
                                       INT32 sortBufferSize,
                                       utilWriteResult *pResult,
                                       dmsIdxTaskStatus *pIdxStatus,
                                       BOOLEAN forceTransCallback,
                                       BOOLEAN addUIDIfNotExist )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_CREATEINDEX ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->createIndex( context, index, cb, dpscb,
                                   isSys, sortBufferSize,
                                   pResult, pIdxStatus,
                                   forceTransCallback, addUIDIfNotExist ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_CREATEINDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CREATEINDEX1, "_dmsStorageUnit::createIndex" )
   INT32 _dmsStorageUnit::createIndex( utilCLUniqueID clUniqID,
                                       const BSONObj &index,
                                       pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                       BOOLEAN isSys, dmsMBContext * context,
                                       INT32 sortBufferSize,
                                       utilWriteResult *pResult,
                                       dmsIdxTaskStatus *pIdxStatus,
                                       BOOLEAN forceTransCallback,
                                       BOOLEAN addUIDIfNotExist )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN getContext = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_CREATEINDEX1 ) ;

      if ( NULL == context )
      {
         rc = _pDataSu->getMBContextByID( &context, clUniqID, -1 ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get collection[%llu] mb context failed, rc: %d",
                      clUniqID, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->createIndex( context, index, cb, dpscb,
                                   isSys, sortBufferSize,
                                   pResult, pIdxStatus,
                                   forceTransCallback, addUIDIfNotExist ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_CREATEINDEX1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( const CHAR *pName, const CHAR *indexName,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context,
                                     dmsIdxTaskStatus *pIdxStatus,
                                     BOOLEAN onlyStandalone )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DROPINDEX ) ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->dropIndex( context, indexName, cb, dpscb, isSys,
                                 pIdxStatus, onlyStandalone ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DROPINDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX1, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( const CHAR *pName, OID &indexOID,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context,
                                     dmsIdxTaskStatus *pIdxStatus,
                                     BOOLEAN onlyStandalone )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DROPINDEX1 ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->dropIndex( context, indexOID, cb, dpscb, isSys,
                                 pIdxStatus, onlyStandalone ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DROPINDEX1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX2, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( utilCLUniqueID clUniqID,
                                     const CHAR *indexName,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context,
                                     dmsIdxTaskStatus *pIdxStatus,
                                     BOOLEAN onlyStandalone )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN getContext = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DROPINDEX2 ) ;

      if ( NULL == context )
      {
         rc = _pDataSu->getMBContextByID( &context, clUniqID, -1 ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Get collection[%llu] mb context failed, rc: %d",
                      clUniqID, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->dropIndex( context, indexName, cb, dpscb, isSys,
                                 pIdxStatus, onlyStandalone ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DROPINDEX2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX3, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( utilCLUniqueID clUniqID, OID &indexOID,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context,
                                     dmsIdxTaskStatus *pIdxStatus,
                                     BOOLEAN onlyStandalone )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN getContext = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DROPINDEX3 ) ;

      if ( NULL == context )
      {
         rc = _pDataSu->getMBContextByID( &context, clUniqID, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%llu] mb context failed, "
                      "rc: %d", clUniqID, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->dropIndex( context, indexOID, cb, dpscb, isSys,
                                 pIdxStatus, onlyStandalone ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DROPINDEX3, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_COUNTCOLLECTION, "_dmsStorageUnit::countCollection" )
   INT32 _dmsStorageUnit::countCollection ( const CHAR *pName,
                                            INT64 &recordNum,
                                            pmdEDUCB *cb,
                                            dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      //dmsExtent *pExtent           = NULL ;
      recordNum                    = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSU_COUNTCOLLECTION ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock dms mb context[%s], rc: %d",
                      context->toString().c_str(), rc ) ;
      }
      if ( ( cb->isTransRC() || cb->isTransRS() ) &&
           cb->getTransExecutor()->isTransRCCount() )
      {
         // NOTE: actually for RC only
         // for RS should consider locking the whole table or need MVCC,
         // this may cause unmatched results between count() and find() with
         // deleting from other transactions ( phantom read ? )

         // no need collection IS lock to protect CL against being dropped
         // since we already have mblatch S
         if ( !cb->getTransExecutor()->getMBTotalRecords(
                                                context->mb()->_clUniqueID,
                                                (UINT64 &)recordNum ) )
         {
            recordNum =
                  (INT64)context->mbStat()->_rcTotalRecords.fetch() ;
         }
      }
      else
      {
         recordNum = context->mbStat()->_totalRecords.fetch() ;
      }

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_COUNTCOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONFLAG, "_dmsStorageUnit::getCollectionFlag" )
   INT32 _dmsStorageUnit::getCollectionFlag( const CHAR *pName, UINT16 &flag,
                                             dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETCOLLECTIONFLAG ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      flag = context->mb()->_flag ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETCOLLECTIONFLAG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CHANGECOLLECTIONFLAG, "_dmsStorageUnit::changeCollectionFlag" )
   INT32 _dmsStorageUnit::changeCollectionFlag( const CHAR *pName, UINT16 flag,
                                                dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_CHANGECOLLECTIONFLAG ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      context->mb()->_flag = flag ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_CHANGECOLLECTIONFLAG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONATTRIBUTES, "_dmsStorageUnit::getCollectionAttributes" )
   INT32 _dmsStorageUnit::getCollectionAttributes( const CHAR *pName,
                                                   UINT32 &attributes,
                                                   dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETCOLLECTIONATTRIBUTES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      attributes = context->mb()->_attributes ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETCOLLECTIONATTRIBUTES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_UPDATECOLLECTIONATTRIBUTES, "_dmsStorageUnit::updateCollectionAttributes" )
   INT32 _dmsStorageUnit::updateCollectionAttributes( const CHAR *pName,
                                                      UINT32 newAttributes,
                                                      dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_UPDATECOLLECTIONATTRIBUTES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      context->mb()->_attributes = newAttributes ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_UPDATECOLLECTIONATTRIBUTES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONCOMPTYPE, "_dmsStorageUnit::getCollectionCompType" )
   INT32 _dmsStorageUnit::getCollectionCompType( const CHAR *pName,
                                                 UTIL_COMPRESSOR_TYPE &compType,
                                                 dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSU_GETCOLLECTIONCOMPTYPE ) ;
      BOOLEAN getContext = FALSE ;

      if ( !context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      compType = (UTIL_COMPRESSOR_TYPE)context->mb()->_compressorType ;
   done:
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_GETCOLLECTIONCOMPTYPE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCOLLECTIONCOMPTYPE, "_dmsStorageUnit::setCollectionCompType" )
   INT32 _dmsStorageUnit::setCollectionCompType ( const CHAR * pName,
                                                  UTIL_COMPRESSOR_TYPE compType,
                                                  dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCOLLECTIONCOMPTYPE ) ;

      BOOLEAN getContext = FALSE ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s] mb context, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                      pName, rc ) ;
      }

      compType = (UTIL_COMPRESSOR_TYPE)context->mb()->_compressorType ;

   done:
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_SETCOLLECTIONCOMPTYPE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCLATTR, "_dmsStorageUnit::setCollectionAttribute" )
   INT32 _dmsStorageUnit::setCollectionAttribute ( const CHAR * pName,
                                                   UINT32 attributeMask,
                                                   BOOLEAN attributeValue,
                                                   dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCLATTR ) ;

      BOOLEAN getContext = FALSE ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s] mb context, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                      pName, rc ) ;
      }

      if ( attributeValue )
      {
         OSS_BIT_SET( context->mb()->_attributes, attributeMask ) ;
      }
      else
      {
         OSS_BIT_CLEAR( context->mb()->_attributes, attributeMask ) ;
      }

      // on metadata updated
      _pDataSu->_onMBUpdated( context->mbID() ) ;

      // Flush MME
      _pDataSu->flushMME( _pDataSu->isSyncDeep() ) ;

   done:
      if ( getContext && NULL != context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_SETCLATTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCLSTRICTDATAMODE, "_dmsStorageUnit::setCollectionStrictDataMode" )
   INT32 _dmsStorageUnit::setCollectionStrictDataMode ( const CHAR * pName,
                                                        BOOLEAN strictDataMode,
                                                        dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCLSTRICTDATAMODE ) ;

      rc = setCollectionAttribute( pName, DMS_MB_ATTR_STRICTDATAMODE,
                                   strictDataMode, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set collection attribute, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSU_SETCLSTRICTDATAMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCLNOIDIDX, "_dmsStorageUnit::setCollectionNoIDIndex" )
   INT32 _dmsStorageUnit::setCollectionNoIDIndex ( const CHAR * pName,
                                                   BOOLEAN noIDIndex,
                                                   dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCLNOIDIDX ) ;

      rc = setCollectionAttribute( pName, DMS_MB_ATTR_NOIDINDEX,
                                   noIDIndex, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set collection attribute, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSU_SETCLNOIDIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCLNOTRANS, "_dmsStorageUnit::setCollectionNoTrans" )
   INT32 _dmsStorageUnit::setCollectionNoTrans ( const CHAR * pName,
                                                 BOOLEAN noTrans,
                                                 dmsMBContext * context,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCLNOTRANS ) ;

      BOOLEAN hasCLLocked = FALSE ;
      dpsTransCB *transCB = sdbGetTransCB() ;

      SDB_ASSERT( NULL != context, "context should be valid" ) ;

      // NOTE: if the context is no-trans now, it can ignore transaction lock
      if ( data()->isTransLockRequired( context ) &&
           NULL != cb &&
           cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         // need to get S lock of collection to avoid transactions have
         // inserted/updated/deleted records on the same collection,
         // including this session itself if it is in transaction
         rc = transCB->transLockTrySAgainstWrite( cb,
                                                  LogicalCSID(),
                                                  context->mbID(),
                                                  NULL,
                                                  &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to lock the collection, rc: %d" OSS_NEWLINE
                      "Conflict( representative ):" OSS_NEWLINE
                      "   EDUID:  %llu" OSS_NEWLINE
                      "   TID:    %u" OSS_NEWLINE
                      "   LockId: %s" OSS_NEWLINE
                      "   Mode:   %s" OSS_NEWLINE,
                      rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;

         hasCLLocked = TRUE ;
      }

      rc = setCollectionAttribute( pName, DMS_MB_ATTR_NOTRANS,
                                   noTrans, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set collection attribute, rc: %d",
                   rc ) ;

   done:
      if ( hasCLLocked )
      {
         transCB->transLockRelease( cb, _pDataSu->logicalID(),
                                    context->mbID(), NULL, NULL ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_SETCLNOTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CANSETCLCOMPRESS, "_dmsStorageUnit::canSetCollectionCompressor" )
   INT32 _dmsStorageUnit::canSetCollectionCompressor ( dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_CANSETCLCOMPRESS ) ;

      SDB_ASSERT( NULL != context, "context is invalid" ) ;

      dmsMB * mb = context->mb() ;
      SDB_ASSERT( NULL != mb, "metablock is invalid" ) ;

      dmsMBStatInfo * mbStat = context->mbStat() ;
      SDB_ASSERT( NULL != mbStat, "stat block is invalid" ) ;

      if ( !_storageService->isAlterCompressorSupported() )
      {
         PD_LOG( PDERROR, "Failed to change compressor, "
                 "storage service [%s] is not supported",
                 dmsGetStorageEngineName( _storageService->getEngineType() ) ) ;
         rc = SDB_ENGINE_NOT_SUPPORT ;
         goto error ;
      }

      if ( !OSS_BIT_TEST( mb->_compressFlags, UTIL_COMPRESS_ALTERABLE_FLAG ) )
      {
         // Old version collection
         if ( mbStat->_totalRecords.fetch() > 0 )
         {
            if ( mb->_compressorType == UTIL_COMPRESSOR_LZW )
            {
               // Should have no dictionary
               PD_CHECK( mb->_dictExtentID == DMS_INVALID_EXTENT,
                         SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                         "Failed to change compressor: should have no "
                         "compressed records and compress dictionary in old "
                         "version collection" ) ;
            }
            else
            {
               // OK for uncompressed and snappy compressed collections
               // Note: Since the length is less than 16MB, the high byte of
               // snappy compress is always 0
               PD_CHECK( mb->_compressorType == UTIL_COMPRESSOR_INVALID ||
                         mb->_compressorType == UTIL_COMPRESSOR_SNAPPY,
                         SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                         "Failed to change compressor: invalid compressor" ) ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSSU_CANSETCLCOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCLCOMPRESS, "_dmsStorageUnit::setCollectionCompress" )
   INT32 _dmsStorageUnit::setCollectionCompressor ( const CHAR * pName,
                                                    UTIL_COMPRESSOR_TYPE compressorType,
                                                    dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCLCOMPRESS ) ;

      BOOLEAN getContext = FALSE ;
      dmsMB * mb = NULL ;
      dmsMBStatInfo * mbStat = NULL ;

      PD_CHECK( UTIL_COMPRESSOR_INVALID == compressorType ||
                UTIL_COMPRESSOR_SNAPPY == compressorType ||
                UTIL_COMPRESSOR_LZW == compressorType,
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to set compressor, unsupported compressor type" ) ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection [%s] mb context, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection [%s], rc: %d",
                      pName, rc ) ;
      }

      mb = context->mb() ;
      SDB_ASSERT( NULL != mb, "metablock is invalid" ) ;

      mbStat = context->mbStat() ;
      SDB_ASSERT( NULL != mbStat, "stat block is invalid" ) ;

      rc = canSetCollectionCompressor( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check collection for setting compress, "
                   "rc: %d", rc ) ;

      if ( !OSS_BIT_TEST( mb->_compressFlags, UTIL_COMPRESS_ALTERABLE_FLAG ) )
      {
         // Now we could add alterable flag
         OSS_BIT_SET( mb->_compressFlags, UTIL_COMPRESS_ALTERABLE_FLAG ) ;
      }

      mb->_compressorType = compressorType ;
      if ( UTIL_COMPRESSOR_INVALID == compressorType )
      {
         OSS_BIT_CLEAR( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) ;
      }
      else
      {
         OSS_BIT_SET( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) ;
      }

      // on metadata updated
      _pDataSu->_onMBUpdated( context->mbID() ) ;

      // Flush MME
      _pDataSu->flushMME( _pDataSu->isSyncDeep() ) ;

   done :
      if ( getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_SETCLCOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONEXTOPTIONS, "_dmsStorageUnit::getCollectionExtOptions" )
   INT32 _dmsStorageUnit::getCollectionExtOptions( const CHAR *pName,
                                                   BSONObj &extOptions,
                                                   dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSU_GETCOLLECTIONEXTOPTIONS ) ;
      BOOLEAN getContext = FALSE ;

      if ( !context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      rc = _pDataSu->dumpExtOptions( context, extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Dump collection extend options failed: %d",
                   rc ) ;

   done:
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_GETCOLLECTIONEXTOPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_SETCOLLECTIONEXTOPTIONS, "_dmsStorageUnit::setCollectionExtOptions" )
   INT32 _dmsStorageUnit::setCollectionExtOptions ( const CHAR * pName,
                                                    const BSONObj & extOptions,
                                                    dmsMBContext * context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_SETCOLLECTIONEXTOPTIONS ) ;

      BOOLEAN getContext = FALSE ;

      if ( !context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      rc = _pDataSu->setExtOptions( context, extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set collection extend options, "
                   "rc: %d", rc ) ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_SETCOLLECTIONEXTOPTIONS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

      // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONINFO, "_dmsStorageUnit::getCollectionInfo" )
   INT32 _dmsStorageUnit::getCollectionInfo( const CHAR *pName,
                                             UINT16 &mbID,
                                             UINT32 &clLID,
                                             utilCLUniqueID &clUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_GETCOLLECTIONINFO ) ;

      SDB_ASSERT( pName, "collection name is invalid" ) ;

      rc = _pDataSu->getMBInfo( pName, mbID, clLID, clUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get info of collection [%s], "
                   "rc: %d", pName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSU_GETCOLLECTIONINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETSEGEXTENTS, "_dmsStorageUnit::getSegExtents" )
   INT32 _dmsStorageUnit::getSegExtents( const CHAR *pName,
                                         ossPoolVector< dmsExtentID > &segExtents,
                                         dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      const dmsMBEx *mbEx          = NULL ;
      dmsExtentID firstID          = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETSEGEXTENTS ) ;
      segExtents.clear() ;

      PD_CHECK( _pDataSu->isBlockScanSupport(), SDB_SYS, error, PDERROR,
                "Collection space [%s] does not support block scan",
                CSName() ) ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      if ( DMS_INVALID_EXTENT == context->mb()->_mbExExtentID )
      {
         PD_LOG( PDERROR, "Invalid meta extent id: %d, collection name: %s",
                 context->mb()->_mbExExtentID,
                 context->mb()->_collectionName ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      extRW = _pDataSu->extent2RW( context->mb()->_mbExExtentID,
                                   context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      mbEx = extRW.readPtr<dmsMBEx>() ;
      if ( mbEx )
      {
         mbEx = extRW.readPtr<dmsMBEx>( 0, (UINT32)mbEx->_header._blockSize <<
                                           _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( !mbEx )
      {
         PD_LOG( PDERROR, "Get extent[%d] read address failed",
                 context->mb()->_mbExExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < mbEx->_header._segNum ; ++i )
      {
         mbEx->getFirstExtentID( i, firstID ) ;
         if ( DMS_INVALID_EXTENT != firstID )
         {
            try
            {
               segExtents.push_back( firstID ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Failed to add segmentExtent: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETSEGEXTENTS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETINDEXES_CTX, "_dmsStorageUnit::getIndexes" )
   INT32 _dmsStorageUnit::getIndexes ( dmsMBContext * context,
                                       MON_IDX_LIST &resultIndexes,
                                       BOOLEAN excludeStandalone )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN lockContext          = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETINDEXES_CTX ) ;

      SDB_ASSERT( context, "context can't be NULL" ) ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         lockContext = TRUE ;
      }

      rc = _getIndexes( context->mb(), resultIndexes, excludeStandalone ) ;
      PD_RC_CHECK( rc, PDERROR, "dump indexes failed, rc: %d", rc ) ;

   done :
      if ( lockContext )
      {
         context->mbUnlock() ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETINDEXES_CTX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETINDEXES_NAME, "_dmsStorageUnit::getIndexes" )
   INT32 _dmsStorageUnit::getIndexes ( const CHAR *pName,
                                       MON_IDX_LIST &resultIndexes,
                                       BOOLEAN excludeStandalone )
   {
      INT32 rc                     = SDB_OK ;
      dmsMBContext * context       = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETINDEXES_NAME ) ;

      SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

      rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                   "rc: %d", pName, rc ) ;

      rc = getIndexes( context, resultIndexes, excludeStandalone ) ;
      PD_RC_CHECK( rc, PDERROR, "dump indexes failed, rc: %d", rc ) ;

   done :
      if ( context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETINDEXES_NAME, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETINDEX, "_dmsStorageUnit::getIndex" )
   INT32 _dmsStorageUnit::getIndex ( dmsMBContext *context,
                                     const CHAR *pIndexName,
                                     _monIndex &resultIndex )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN lockContext          = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETINDEX ) ;

      SDB_ASSERT( context, "context can't be NULL" ) ;
      SDB_ASSERT( pIndexName, "Index name can't be NULL" ) ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         lockContext = TRUE ;
      }

      rc = _getIndex( context->mb(), pIndexName, resultIndex ) ;

   done :
      if ( lockContext )
      {
         context->mbUnlock() ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETINDEX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CLSIMVEC, "_dmsStorageUnit::dumpInfo" )
   INT32 _dmsStorageUnit::dumpInfo ( MON_CL_SIM_VEC &clList,
                                     BOOLEAN sys,
                                     BOOLEAN dumpIdx )
   {
      PD_TRACE_ENTRY( SDB__DMSSU_DUMPINFO_CLSIMVEC ) ;
      INT32 rc = SDB_OK ;
      dmsStorageData::COLNAME_MAP_IT it ;

      /// Guard
      {
         ossScopedLock lock( &_pDataSu->_metadataLatch, SHARED ) ;

         it = _pDataSu->_collectionNameMap.begin() ;
         while ( it != _pDataSu->_collectionNameMap.end() )
         {
            monCLSimple collection ;

            if ( !sys && dmsIsSysCLName( it->first ) )
            {
               ++it ;
               continue ;
            }

            rc = _dumpCLInfo( collection, it->second ) ;
            if ( SDB_OK == rc )
            {
               try
               {
                  clList.push_back ( collection ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Add collection information occur "
                          "exception: %s", e.what() ) ;
                  rc = SDB_OOM ;
               }
            }

            if ( SDB_OOM == rc )
            {
               goto error ;
            }
            else
            {
               /// ignore error
               rc = SDB_OK ;
            }

            ++it ;
         }
      }

      if ( dumpIdx )
      {
         // Dump indexes for each collection
         MON_CL_SIM_VEC::iterator iter = clList.begin() ;
         while ( iter != clList.end() )
         {
            rc = getIndexes( iter->_clname, iter->_idxList ) ;
            if ( SDB_OOM == rc )
            {
               goto error ;
            }
            else if ( SDB_OK == rc )
            {
               ++ iter ;
            }
            else
            {
               // Dump index with error, erase this collection from list
               // The collection may be dropped
               iter = clList.erase( iter ) ;
               rc = SDB_OK ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_DUMPINFO_CLSIMVEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CLSIMLIST, "_dmsStorageUnit::dumpInfo" )
   INT32 _dmsStorageUnit::dumpInfo( MON_CL_SIM_LIST &clList,
                                    BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CLSIMLIST ) ;
      INT32 rc = SDB_OK ;

      dmsStorageData::COLNAME_MAP_IT it ;
      /// Guard
      {
         ossScopedLock lock( &_pDataSu->_metadataLatch, SHARED) ;

         it = _pDataSu->_collectionNameMap.begin() ;
         while ( it != _pDataSu->_collectionNameMap.end() )
         {
            monCLSimple collection ;

            if ( !sys && dmsIsSysCLName( it->first ) )
            {
               ++it ;
               continue ;
            }

            rc = _dumpCLInfo( collection, it->second ) ;
            if ( SDB_OK == rc )
            {
               try
               {
                  clList.insert ( collection ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Add collection information occur "
                          "exception: %s", e.what() ) ;
                  rc = SDB_OOM ;
               }
            }

            if ( SDB_OOM == rc )
            {
               goto error ;
            }
            else if ( rc )
            {
               rc = SDB_OK ;  /// ignore other errors
            }

            ++it ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_DUMPINFO_CLSIMLIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CLSIMPLE_CTX, "_dmsStorageUnit::dumpInfo" )
   INT32 _dmsStorageUnit::dumpInfo ( monCLSimple &collection,
                                     dmsMBContext *context,
                                     BOOLEAN dumpIdx )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN lockContext = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CLSIMPLE_CTX ) ;

      SDB_ASSERT( context, "context can't be NULL" ) ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         lockContext = TRUE ;
      }

      collection.setName( CSName(), context->mb()->_collectionName ) ;
      collection._blockID = context->mbID() ;
      collection._logicalID = context->clLID() ;

      if ( dumpIdx )
      {
         rc = getIndexes( context, collection._idxList ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done :
      if ( lockContext )
      {
         context->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB__DMSSU_DUMPINFO_CLSIMPLE_CTX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CLLIST, "_dmsStorageUnit::dumpInfo" )
   INT32 _dmsStorageUnit::dumpInfo ( MON_CL_LIST &clList,
                                     BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CLLIST ) ;
      INT32 rc = SDB_OK ;

      dmsStorageData::COLNAME_MAP_IT it ;
      /// Guard
      {
         ossScopedLock lock( &_pDataSu->_metadataLatch, SHARED ) ;
         it = _pDataSu->_collectionNameMap.begin() ;
         while ( it != _pDataSu->_collectionNameMap.end() )
         {
            monCollection collection ;

            if ( !sys && dmsIsSysCLName( it->first ) )
            {
               ++it ;
               continue ;
            }

            rc = _dumpCLInfo( collection, it->second ) ;
            if ( SDB_OK == rc )
            {
               try
               {
                  clList.insert ( collection ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Add collection information occur "
                          "exception: %s", e.what() ) ;
                  rc = SDB_OOM ;
               }
            }

            if ( SDB_OOM == rc )
            {
               goto error ;
            }
            else if ( rc )
            {
               rc = SDB_OK ;  /// ignore other errors
            }

            ++it ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_DUMPINFO_CLLIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_SU, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( monStorageUnit &storageUnit )
   {
      const dmsStorageUnitHeader *dataHeader = _pDataSu->getHeader() ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_SU ) ;

      storageUnit.setName( CSName() ) ;
      storageUnit._csUniqueID = CSUniqueID() ;
      storageUnit._pageSize = getPageSize() ;
      storageUnit._lobPageSize = getLobPageSize() ;
      storageUnit._sequence = CSSequence() ;
      storageUnit._numCollections = dataHeader->_numMB ;
      storageUnit._collectionHWM = dataHeader->_MBHWM ;
      storageUnit._size = totalSize() ;
      storageUnit._CSID = CSID() ;
      storageUnit._logicalCSID = LogicalCSID() ;
      storageUnit._createTime = getCreateTime() ;
      storageUnit._updateTime = getUpdateTime() ;

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_SU ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CSSIM, "_dmsStorageUnit::dumpInfo" )
   INT32 _dmsStorageUnit::dumpInfo ( monCSSimple &collectionSpace,
                                     BOOLEAN sys,
                                     BOOLEAN dumpCL,
                                     BOOLEAN dumpIdx )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CSSIM ) ;
      INT32 rc = SDB_OK ;

      collectionSpace.setName( CSName() ) ;
      collectionSpace._suID = CSID() ;
      collectionSpace._logicalID = LogicalCSID() ;
      collectionSpace._csUniqueID = CSUniqueID() ;

      if ( dumpCL )
      {
         rc = dumpInfo ( collectionSpace._clList, sys, dumpIdx ) ;
      }

      PD_TRACE_EXITRC ( SDB__DMSSU_DUMPINFO_CSSIM, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CS, "_dmsStorageUnit::dumpInfo" )
   INT32 _dmsStorageUnit::dumpInfo ( monCollectionSpace &collectionSpace,
                                     BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CS ) ;
      INT32 rc = SDB_OK ;
      dmsStorageUnitStat statInfo ;

      // get stat info
      getStatInfo( statInfo ) ;
      INT64 totalDataFreeSize    = totalFreeSize( DMS_SU_DATA ) +
                                   statInfo._totalDataFreeSpace ;
      INT64 totalIndexFreeSize   = totalFreeSize( DMS_SU_INDEX ) +
                                   statInfo._totalIndexFreeSpace ;
      INT64 totalLobFreeSpace    = totalFreeSize( DMS_SU_LOB ) ;

      ossMemset( collectionSpace._name, 0, sizeof(collectionSpace._name) ) ;
      ossStrncpy( collectionSpace._name, CSName(), DMS_COLLECTION_SPACE_NAME_SZ );
      collectionSpace._csUniqueID = CSUniqueID() ;
      collectionSpace._suID = CSID() ;
      collectionSpace._csLID = LogicalCSID() ;
      collectionSpace._pageSize = getPageSize() ;
      collectionSpace._lobPageSize = getLobPageSize() ;
      collectionSpace._totalSize = totalSize() ;
      collectionSpace._clNum    = statInfo._clNum ;
      collectionSpace._totalRecordNum = statInfo._totalCount ;
      collectionSpace._freeSize = totalDataFreeSize + totalIndexFreeSize +
                                  totalLobFreeSpace ;
      collectionSpace._totalDataSize = totalSize( DMS_SU_DATA ) ;
      collectionSpace._freeDataSize  = totalDataFreeSize ;
      collectionSpace._totalIndexSize = totalSize( DMS_SU_INDEX ) ;
      collectionSpace._freeIndexSize = totalIndexFreeSize ;
      collectionSpace._recycleDataSize = statInfo._recycleDataSize ;
      collectionSpace._recycleIndexSize = statInfo._recycleIndexSize ;
      collectionSpace._recycleLobSize = statInfo._recycleLobSize ;

      collectionSpace._lobCapacity = totalSize( DMS_SU_LOB ) ;
      collectionSpace._lobMetaCapacity = totalSize( DMS_SU_LOB_META ) ;
      collectionSpace._freeLobSpace = totalLobFreeSpace ;
      collectionSpace._totalLobPages = statInfo._totalLobPages ;
      collectionSpace._totalLobs = statInfo._totalLobs ;
      collectionSpace._totalLobSize = statInfo._totalLobSize ;
      collectionSpace._totalValidLobSize = statInfo._totalValidLobSize ;

      collectionSpace._totalLobGet = statInfo._totalLobGet ;
      collectionSpace._totalLobPut = statInfo._totalLobPut ;
      collectionSpace._totalLobDelete = statInfo._totalLobDelete ;
      collectionSpace._totalLobList = statInfo._totalLobList ;
      collectionSpace._totalLobReadSize = statInfo._totalLobReadSize ;
      collectionSpace._totalLobWriteSize = statInfo._totalLobWriteSize ;
      collectionSpace._totalLobRead = statInfo._totalLobRead ;
      collectionSpace._totalLobWrite = statInfo._totalLobWrite ;
      collectionSpace._totalLobTruncate = statInfo._totalLobTruncate ;
      collectionSpace._totalLobAddressing = statInfo._totalLobAddressing ;

      /// sync info
      collectionSpace._dataCommitLsn = getCurrentDataLSN() ;
      collectionSpace._idxCommitLsn = getCurrentIdxLSN() ;
      collectionSpace._lobCommitLsn = getCurrentLobLSN() ;
      getValidFlag( collectionSpace._dataIsValid,
                    collectionSpace._idxIsValid,
                    collectionSpace._lobIsValid ) ;

      /// cache info
      collectionSpace._dirtyPage = cacheUnit()->dirtyPages() ;
      collectionSpace._type = type() ;

      collectionSpace._createTime = getCreateTime() ;
      collectionSpace._updateTime = getUpdateTime() ;

      rc = dumpInfo ( collectionSpace._collections, sys, FALSE ) ;

      PD_TRACE_EXITRC ( SDB__DMSSU_DUMPINFO_CS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMP_CL_INFO, "_dmsStorageUnit::dumpCLInfo" )
   INT32 _dmsStorageUnit::dumpCLInfo ( const CHAR *collectionName,
                                       monCollection &info )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMP_CL_INFO ) ;
      INT32 rc = SDB_OK ;
      dmsStorageData::COLNAME_MAP_IT it ;
      ossScopedLock lock( &_pDataSu->_metadataLatch, SHARED ) ;
      it = _pDataSu->_collectionNameMap.find( collectionName ) ;
      if ( _pDataSu->_collectionNameMap.end() == it )
      {
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }
      rc = _dumpCLInfo( info, it->second ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to dump collection info, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_DUMP_CL_INFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALSIZE, "_dmsStorageUnit::totalSize" )
   INT64 _dmsStorageUnit::totalSize( UINT32 type ) const
   {
      INT64 totalSize = 0 ;
      const dmsStorageUnitHeader *dataHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALSIZE ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         dataHeader = _pDataSu->getHeader() ;
         totalSize += ( (INT64)( dataHeader->_storageUnitSize ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( type & DMS_SU_INDEX )
      {
         dataHeader = _pIndexSu->getHeader() ;
         totalSize += ( (INT64)( dataHeader->_storageUnitSize ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalSize +=
            ( _pLobSu->getLobData()->getFileSz() - DMS_HEADER_SZ ) ;
      }
      if ( ( type & DMS_SU_LOB_META ) && _pLobSu->isOpened() )
      {
         totalSize += ( (INT64)( _pLobSu->getHeader()->_storageUnitSize ) <<
                        _pLobSu->pageSizeSquareRoot() ) ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALSIZE,
                  PD_PACK_LONG ( totalSize ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALSIZE ) ;
      return totalSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALDATAPAGES, "_dmsStorageUnit::totalDataPages" )
   INT64 _dmsStorageUnit::totalDataPages( UINT32 type ) const
   {
      INT64 totalDataPages = 0 ;
      const dmsStorageUnitHeader *dataHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALDATAPAGES ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         dataHeader = _pDataSu->getHeader() ;
         totalDataPages += dataHeader->_pageNum ;
      }
      if ( type & DMS_SU_INDEX )
      {
         dataHeader = _pIndexSu->getHeader() ;
         totalDataPages += dataHeader->_pageNum ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalDataPages += _pLobSu->getHeader()->_pageNum ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALDATAPAGES,
                  PD_PACK_LONG ( totalDataPages ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALDATAPAGES ) ;
      return totalDataPages ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALDATASIZE, "_dmsStorageUnit::totalDataSize" )
   INT64 _dmsStorageUnit::totalDataSize( UINT32 type ) const
   {
      INT64 totalSize = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALDATASIZE ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         totalSize += ( totalDataPages( DMS_SU_DATA ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( type & DMS_SU_INDEX )
      {
         totalSize += ( totalDataPages( DMS_SU_INDEX ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalSize += _pLobSu->getLobData()->getDataSz() ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALDATASIZE,
                  PD_PACK_LONG ( totalSize ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALDATASIZE ) ;
      return totalSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALFREEPAGES, "_dmsStorageUnit::totalFreePages" )
   INT64 _dmsStorageUnit::totalFreePages ( UINT32 type ) const
   {
      INT64 freePages = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALFREEPAGES ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         freePages += (INT64)_pDataSu->freePageNum() ;
      }
      if ( type & DMS_SU_INDEX )
      {
         freePages += (INT64)_pIndexSu->freePageNum() ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         freePages += (INT64)_pLobSu->freePageNum() ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALFREEPAGES,
                  PD_PACK_INT ( freePages ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALFREEPAGES ) ;
      return freePages ;
   }

   INT64 _dmsStorageUnit::totalFreeSize( UINT32 type ) const
   {
      INT64 totalFreeSize = 0 ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         totalFreeSize += ( totalFreePages( DMS_SU_DATA ) <<
                            _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( type & DMS_SU_INDEX )
      {
         totalFreeSize += ( totalFreePages( DMS_SU_INDEX ) <<
                            _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalFreeSize += ( totalFreePages( DMS_SU_LOB ) *
                            _pDataSu->getLobdPageSize() ) ;
      }

   done:
      return totalFreeSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETSTATINFO, "_dmsStorageUnit::getStatInfo" )
   void _dmsStorageUnit::getStatInfo( dmsStorageUnitStat & statInfo )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_GETSTATINFO ) ;
      ossMemset( &statInfo, 0, sizeof( dmsStorageUnitStat ) ) ;

      dmsMBStatInfo *mbStat = NULL ;

      /// Guard
      {
         ossScopedLock lock( &_pDataSu->_metadataLatch, SHARED ) ;

         dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
         while ( it != _pDataSu->_collectionNameMap.end() )
         {
            mbStat = &_pDataSu->_mbStatInfo[it->second] ;

            if ( dmsIsSysRecycleName( it->first ) )
            {
               statInfo._recycleDataSize += ( mbStat->_totalDataPages <<
                                              _pDataSu->pageSizeSquareRoot() ) ;
               statInfo._recycleIndexSize += ( mbStat->_totalIndexPages <<
                                              _pDataSu->pageSizeSquareRoot() ) ;
               statInfo._recycleLobSize += ( mbStat->_totalLobPages.fetch() *
                                              _pDataSu->getLobdPageSize() ) ;
            }
            else
            {
               ++statInfo._clNum ;
               statInfo._totalDataFreeSpace += mbStat->_totalDataFreeSpace ;
               statInfo._totalIndexFreeSpace += mbStat->_totalIndexFreeSpace ;
               statInfo._totalCount += mbStat->_totalRecords.fetch() ;
               statInfo._totalDataPages += mbStat->_totalDataPages ;
               statInfo._totalIndexPages += mbStat->_totalIndexPages ;
               statInfo._totalLobPages += mbStat->_totalLobPages.fetch() ;

               statInfo._totalLobs += mbStat->_totalLobs.fetch() ;
               statInfo._totalValidLobSize += mbStat->_totalValidLobSize.fetch() ;
               statInfo._totalLobSize += mbStat->_totalLobSize.fetch() ;

               statInfo._totalLobGet += mbStat->_crudCB._totalLobGet ;
               statInfo._totalLobPut += mbStat->_crudCB._totalLobPut ;
               statInfo._totalLobDelete += mbStat->_crudCB._totalLobDelete ;
               statInfo._totalLobList += mbStat->_crudCB._totalLobList ;
               statInfo._totalLobReadSize += mbStat->_crudCB._totalLobReadSize ;
               statInfo._totalLobWriteSize += mbStat->_crudCB._totalLobWriteSize ;
               statInfo._totalLobRead += mbStat->_crudCB._totalLobRead ;
               statInfo._totalLobWrite += mbStat->_crudCB._totalLobWrite ;
               statInfo._totalLobTruncate += mbStat->_crudCB._totalLobTruncate ;
               statInfo._totalLobAddressing += mbStat->_crudCB._totalLobAddressing ;
            }

            ++it ;
         }
      }
      PD_TRACE_EXIT ( SDB__DMSSU_GETSTATINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__DUMPCLINFO_CL, "_dmsStorageUnit::_dumpCLInfo" )
   INT32 _dmsStorageUnit::_dumpCLInfo ( monCollection &collection, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;
      INT64 lobCapacity = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSU__DUMPCLINFO_CL ) ;

      const dmsMB *mb = NULL ;
      dmsMBStatInfo *mbStat = NULL ;

      PD_CHECK( mbID < DMS_MME_SLOTS, SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u]", mbID ) ;

      mb = _pDataSu->getMBInfo( mbID ) ;
      mbStat = _pDataSu->getMBStatInfo( mbID ) ;
      lobCapacity = totalSize( DMS_SU_LOB ) ;

      PD_CHECK( DMS_IS_MB_INUSE ( mb->_flag ), SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u], metablock is not in-used", mbID ) ;

      collection.setName( CSName(), mb->_collectionName ) ;
      collection._clUniqueID = mb->_clUniqueID ;

      try
      {
         detailedInfo &info = collection.addDetails( CSSequence(),
                                                     mb->_numIndexes,
                                                     mb->_blockID,
                                                     mb->_flag,
                                                     mb->_logicalID,
                                                     mbStat->_totalRecords.fetch(),
                                                     mbStat->_totalDataPages,
                                                     mbStat->_totalIndexPages,
                                                     mbStat->_totalLobPages.fetch(),
                                                     mbStat->_totalDataFreeSpace,
                                                     mbStat->_totalIndexFreeSpace ) ;

         info._attribute = mb->_attributes ;
         info._dictCreated = mb->_dictExtentID != DMS_INVALID_EXTENT ? 1 : 0 ;
         info._compressType = mb->_compressorType ;
         info._dictVersion = mb->_dictVersion ;

         info._totalLobs = mbStat->_totalLobs.fetch() ;
         info._totalUsedLobSpace = (INT64)mbStat->_totalLobPages.fetch() * getLobPageSize() ;
         info._usedLobSpaceRatio = utilPercentage( info._totalUsedLobSpace, lobCapacity ) ;
         info._totalLobSize = mbStat->_totalLobSize.fetch() ;
         info._totalValidLobSize = mbStat->_totalValidLobSize.fetch() ;
         /// Because lob page 0 is unevenly distributed on data nodes, the
         /// _totalValidLobSize may be larger than the _totalUsedLobSpace,
         /// so use _totalLobSize / _totalUsedLobSpace in data nodes.
         info._lobUsageRate = utilPercentage( info._totalLobSize, info._totalUsedLobSpace ) ;
         if ( 0 < info._totalLobs )
         {
            info._avgLobSize = info._totalValidLobSize / info._totalLobs ;
         }
         else
         {
            info._avgLobSize = 0 ;
         }

         info._pageSize = getPageSize() ;
         info._lobPageSize = getLobPageSize() ;
         info._currCompressRatio = mbStat->_lastCompressRatio ;

         /// sync info
         info._dataCommitLSN = mb->_commitLSN ;
         info._idxCommitLSN = mb->_idxCommitLSN ;
         info._lobCommitLSN = mb->_lobCommitLSN ;
         info._dataIsValid = mbStat->_commitFlag.peek() ? TRUE : FALSE ;
         info._idxIsValid = mbStat->_idxCommitFlag.peek() ? TRUE : FALSE ;
         info._lobIsValid = mbStat->_lobCommitFlag.peek() ? TRUE : FALSE ;

         info._createTime = mbStat->_createTime ;
         info._updateTime = mbStat->_updateTime ;

         info._crudCB.setFromOnce( mbStat->_crudCB ) ;

         if ( !_pLobSu->isOpened() )
         {
            info._lobCommitLSN = 0 ;
            info._lobIsValid = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Build collection information occur exception: %s",
                 e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSSU__DUMPCLINFO_CL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__DUMPCLINFO_CLSIMPLE, "_dmsStorageUnit::_dumpCLInfo" )
   INT32 _dmsStorageUnit::_dumpCLInfo ( monCLSimple &collection,
                                        UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSU__DUMPCLINFO_CLSIMPLE ) ;

      const dmsMB *mb = NULL ;

      PD_CHECK( mbID < DMS_MME_SLOTS, SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u]", mbID ) ;

      mb = _pDataSu->getMBInfo( mbID ) ;

      PD_CHECK( DMS_IS_MB_INUSE( mb->_flag ), SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u], metablock is not in-used", mbID ) ;

      collection.setName( CSName(), mb->_collectionName ) ;
      collection._blockID = mb->_blockID ;
      collection._logicalID = mb->_logicalID ;
      collection._clUniqueID = mb->_clUniqueID ;

   done :
      PD_TRACE_EXITRC( SDB__DMSSU__DUMPCLINFO_CLSIMPLE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__GETINDEXES, "_dmsStorageUnit::_getIndexes" )
   INT32 _dmsStorageUnit::_getIndexes ( const dmsMB *mb,
                                        MON_IDX_LIST &resultIndexes,
                                        BOOLEAN excludeStandalone )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSU__GETINDEXES ) ;
      UINT32 indexID = 0 ;
      SDB_ASSERT( mb, "mb is invalid" ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == mb->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( mb->_indexExtent[indexID], _pIndexSu, NULL ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG( PDERROR, "Failed to initialize index[%u], indexID" ) ;
            continue ;
         }
         if ( excludeStandalone && indexCB.standalone() )
         {
            // No need to process it
            continue ;
         }

         try
         {
            monIndex indexItem ;
            indexItem._indexFlag = indexCB.getFlag () ;
            indexItem._scanRID = indexCB.getScanRID() ;
            indexItem._indexLID = indexCB.getLogicalID () ;
            indexItem._version = indexCB.version () ;
            // copy the index def to it's owned buffer
            indexItem._indexDef = indexCB.getDef().copy () ;
            if ( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT,
                                      indexCB.getIndexType() )
                 && IXM_INDEX_FLAG_NORMAL == indexCB.getFlag() )
            {
               SDB_ASSERT( indexCB.getExtDataName(),
                           "External data name is NULL") ;
               ossStrncpy( indexItem._extDataName, indexCB.getExtDataName(),
                           DMS_MAX_EXT_NAME_SIZE + 1 ) ;
            }

            // add
            resultIndexes.push_back ( indexItem ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Build index information occur exception: %s",
                    e.what() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSU__GETINDEXES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__GETINDEX, "_dmsStorageUnit::_getIndex" )
   INT32 _dmsStorageUnit::_getIndex ( const dmsMB *mb,
                                      const CHAR *pIndexName,
                                      monIndex &resultIndex )
   {
      INT32 rc = SDB_IXM_NOTEXIST ;
      UINT32 indexID = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSU__GETINDEX ) ;

      SDB_ASSERT( mb, "mb can't be NULL" ) ;
      SDB_ASSERT( pIndexName, "Index name can't be NULL" ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == mb->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( mb->_indexExtent[indexID], _pIndexSu, NULL ) ;
         if ( indexCB.isInitialized() &&
              0 == ossStrcmp( indexCB.getName(), pIndexName ) )
         {
            resultIndex._indexFlag = indexCB.getFlag () ;
            resultIndex._scanRID = indexCB.getScanRID() ;
            resultIndex._indexLID = indexCB.getLogicalID () ;
            resultIndex._version = indexCB.version () ;
            // copy the index def to it's owned buffer
            try
            {
               resultIndex._indexDef = indexCB.getDef().copy () ;
               rc = SDB_OK ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Copy index define occur exception: %s",
                       e.what() ) ;
               rc = SDB_OOM ;
            }
            break ;
         }
      }

      PD_TRACE_EXITRC ( SDB__DMSSU__GETINDEX, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPRECYCLEINFO, "_dmsStorageUnit::dumpRecycleInfo" )
   INT32 _dmsStorageUnit::dumpRecycleInfo( monRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU_DUMPRECYCLEINFO ) ;

      UINT16 mbID = DMS_INVALID_MBID ;

      if ( UTIL_RECYCLE_CL == item.getType() )
      {
         {
            ossScopedLock lock( &( _pDataSu->_metadataLatch ), SHARED ) ;
            dmsStorageData::COLNAME_MAP_IT it =
                  _pDataSu->_collectionNameMap.find( item.getRecycleName() ) ;
            PD_CHECK( it != _pDataSu->_collectionNameMap.end(),
                      SDB_RECYCLE_ITEM_NOTEXIST, error, PDWARNING,
                      "Failed to find recycle item [%s]",
                      item.getRecycleName() ) ;
            mbID = it->second ;
         }

         rc = _dumpRecycleInfo( mbID, item ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to dump recycle item [%s], "
                      "rc: %d", item.getRecycleName(), rc ) ;
      }
      else if ( UTIL_RECYCLE_CS == item.getType() )
      {
         rc = _dumpRecycleInfo( item ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to dump recycle item, rc: %d",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSU_DUMPRECYCLEINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__DUMPRECYCLEINFO_CL, "_dmsStorageUnit::_dumpRecycleInfo" )
   INT32 _dmsStorageUnit::_dumpRecycleInfo( UINT16 mbID,
                                            monRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU__DUMPRECYCLEINFO_CL ) ;

      dmsMBStatInfo *mbStat = NULL ;

      PD_CHECK( mbID < DMS_MME_SLOTS, SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u]", mbID ) ;

      mbStat = _pDataSu->getMBStatInfo( mbID ) ;
      SDB_ASSERT( NULL != mbStat, "mb stat info is invalid" ) ;

      item._pageSize = getPageSize() ;
      item._lobPageSize = getLobPageSize() ;

      item._totalRecords = mbStat->_totalRecords.fetch() ;
      item._totalLobs = mbStat->_totalLobs.fetch() ;

      item._totalDataSize = mbStat->_totalDataPages <<
                                        _pDataSu->pageSizeSquareRoot() ;
      item._totalIndexSize = mbStat->_totalIndexPages <<
                                        _pIndexSu->pageSizeSquareRoot() ;
      item._totalLobSize = mbStat->_totalLobPages.fetch() *
                                        _pLobSu->getLobdPageSize() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSU__DUMPRECYCLEINFO_CL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__DUMPRECYCLEINFO_CS, "_dmsStorageUnit::_dumpRecycleInfo" )
   INT32 _dmsStorageUnit::_dumpRecycleInfo( monRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSU__DUMPRECYCLEINFO_CS ) ;

      dmsStorageUnitStat statInfo ;

      // get stat info
      getStatInfo( statInfo ) ;

      item._pageSize = getPageSize() ;
      item._lobPageSize = getLobPageSize() ;
      item._totalRecords = statInfo._totalCount ;
      item._totalLobs = statInfo._totalLobs ;
      item._totalDataSize = totalSize( DMS_SU_DATA ) ;
      item._totalIndexSize = totalSize( DMS_SU_INDEX ) ;
      item._totalLobSize = totalSize( DMS_SU_LOB ) ;

      PD_TRACE_EXITRC( SDB__DMSSU__DUMPRECYCLEINFO_CS, rc ) ;

      return rc ;
   }

   void _dmsStorageUnit::setSyncConfig( UINT32 syncInterval,
                                        UINT32 syncRecordNum,
                                        UINT32 syncDirtyRatio )
   {
      if ( _pLobSu )
      {
         _pLobSu->setSyncConfig( syncInterval,
                                 syncRecordNum,
                                 syncDirtyRatio ) ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->setSyncConfig( syncInterval,
                                   syncRecordNum,
                                   syncDirtyRatio ) ;
      }
      if ( _pDataSu )
      {
         _pDataSu->setSyncConfig( syncInterval,
                                  syncRecordNum,
                                  syncDirtyRatio ) ;
      }
   }

   void _dmsStorageUnit::setSyncDeep( BOOLEAN syncDeep )
   {
      if ( _pLobSu )
      {
         _pLobSu->setSyncDeep( syncDeep ) ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->setSyncDeep( syncDeep ) ;
      }
      if ( _pDataSu )
      {
         _pDataSu->setSyncDeep( syncDeep ) ;
      }
   }

   void _dmsStorageUnit::enableSync( BOOLEAN enable )
   {
      if ( _pLobSu )
      {
         _pLobSu->enableSync( enable ) ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->enableSync( enable ) ;
      }
      if ( _pDataSu )
      {
         _pDataSu->enableSync( enable ) ;
      }
   }

   void _dmsStorageUnit::restoreForCrash()
   {
      if ( _pLobSu )
      {
         _pLobSu->restoreForCrash() ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->restoreForCrash() ;
      }
      if ( _pDataSu )
      {
         _pDataSu->restoreForCrash() ;
      }
   }

   INT32 _dmsStorageUnit::sync( BOOLEAN sync,
                                IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      if ( NULL != _pLobSu && _pLobSu->isOpened() )
      {
         _pLobSu->lock() ;
         rcTmp = _pLobSu->sync( TRUE, sync, cb ) ;
         _pLobSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rcTmp ) ;
            /// not go to error
            rc = rc ? rc : rcTmp ;
         }
      }

      if ( NULL != _pIndexSu )
      {
         _pIndexSu->lock() ;
         rcTmp = _pIndexSu->sync( TRUE, sync, cb ) ;
         _pIndexSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rcTmp ) ;
            /// not go to error
            rc = rc ? rc : rcTmp ;
         }
      }

      /// data file must be the last
      if ( NULL != _pDataSu )
      {
         _pDataSu->lock() ;
         rc = _pDataSu->sync( TRUE, sync, cb ) ;
         _pDataSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rcTmp ) ;
            /// not go to error
            rc = rc ? rc : rcTmp ;
         }
      }

      return rc ;
   }

   UINT64 _dmsStorageUnit::getCurrentDataLSN() const
   {
      return NULL == _pDataSu ?
             -1 : _pDataSu->getCommitLSN() ;
   }

   UINT64 _dmsStorageUnit::getCurrentIdxLSN() const
   {
      return NULL == _pIndexSu ?
             -1 : _pIndexSu->getCommitLSN() ;
   }

   UINT64 _dmsStorageUnit::getCurrentLobLSN() const
   {
      return NULL == _pLobSu ?
             -1 : _pLobSu->getCommitLSN() ;
   }

   void _dmsStorageUnit::getValidFlag( BOOLEAN &dataFlag,
                                       BOOLEAN &idxFlag,
                                       BOOLEAN &lobFlag ) const
   {
      dataFlag =  NULL == _pDataSu ?
                  TRUE : ( _pDataSu->getCommitFlag() ? TRUE : FALSE ) ;
      idxFlag = NULL == _pIndexSu ?
                TRUE : ( _pIndexSu->getCommitFlag() ? TRUE : FALSE ) ;

      /// _pLobSu may be NULL, set it as 1
      lobFlag = ( NULL == _pLobSu || !_pLobSu->isOpened() ) ?
                TRUE : ( _pLobSu->getCommitFlag() ? TRUE : FALSE ) ;
   }

   _IDmsEventHolder *_dmsStorageUnit::getEventHolder ()
   {
      return &_eventHolder ;
   }

   void _dmsStorageUnit::setEventHandlers ( DMS_HANDLER_LIST *handlers )
   {
      _eventHolder.setHandlers( handlers ) ;
   }

   void _dmsStorageUnit::unsetEventHandlers ()
   {
      _eventHolder.unsetHandlers() ;
   }

   dmsSUCache *_dmsStorageUnit::getSUCache ( UINT32 type )
   {
      return _cacheHolder.getSUCache( type ) ;
   }

   dmsStatCache *_dmsStorageUnit::getStatCache ()
   {
      return (dmsStatCache *)getSUCache( DMS_CACHE_TYPE_STAT ) ;
   }

   dmsCachedPlanMgr *_dmsStorageUnit::getCachedPlanMgr ()
   {
      return (dmsCachedPlanMgr *)getSUCache( DMS_CACHE_TYPE_PLAN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CLEARMBCRUDCB, "_dmsStorageUnit::clearMBCRUDCB" )
   void _dmsStorageUnit::clearMBCRUDCB ()
   {
      PD_TRACE_ENTRY( SDB__DMSSU_CLEARMBCRUDCB ) ;

      if ( NULL == _pDataSu )
      {
         PD_LOG( PDINFO, "storage data unit for [%s] is empty", CSName() ) ;
         return ;
      }

      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         if ( DMS_IS_MB_INUSE ( _pDataSu->_dmsMME->_mbList[i]._flag ) )
         {
            _pDataSu->_mbStatInfo[ i ]._crudCB.resetOnce() ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSSU_CLEARMBCRUDCB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__CREATESTORAGEOBJS, "_dmsStorageUnit::_createStorageObjs" )
   INT32 _dmsStorageUnit::_createStorageObjs()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSU__CREATESTORAGEOBJS ) ;
      CHAR dataFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;
      CHAR idxFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;

      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_DATA_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_INDEX_SU_EXT_NAME ) ;

      _pDataSu = getDMSStorageDataFactory()->createProduct( _storageService,
                                                            this,
                                                            _storageInfo._type,
                                                            dataFileName,
                                                            &_eventHolder ) ;
      if ( !_pDataSu )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create storage data structure of type[ %d ] with "
                 "data file[ %s ] failed[ %d ]",
                 _storageInfo._type, dataFileName, rc ) ;
         goto error ;
      }

      _pIndexSu = SDB_OSS_NEW dmsStorageIndex( _storageService, this,
                                               idxFileName, _pDataSu ) ;
      if ( !_pIndexSu )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create index structure with "
                 "index file[ %s ] failed[ %d ]",
                 idxFileName, rc ) ;
         goto error ;
      }

      /// alloc cache unit
      _pCacheUnit = SDB_OSS_NEW utilCacheUnit( _pMgr ) ;
      if ( !_pCacheUnit )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create cache unit failed[ %d ]", rc ) ;
         goto error ;
      }

      /// reuse buf for lob
      ossMemset( dataFileName, 0, sizeof( dataFileName ) ) ;
      ossMemset( idxFileName, 0 , sizeof( idxFileName ) ) ;
      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_LOB_META_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_LOB_DATA_SU_EXT_NAME ) ;

      _pLobSu = SDB_OSS_NEW dmsStorageLob( _storageService, this,
                                           dataFileName, idxFileName,
                                           _pDataSu, _pCacheUnit ) ;
      if ( !_pLobSu )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create lob structure with file[ %s, %s ] "
                 "failed[ %d ]", dataFileName, idxFileName, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSU__CREATESTORAGEOBJS, rc ) ;
      return rc ;
   error:
      if ( _pLobSu )
      {
         SDB_OSS_DEL _pLobSu ;
         _pLobSu = NULL ;
      }
      if ( _pCacheUnit )
      {
         SDB_OSS_DEL _pCacheUnit ;
         _pCacheUnit = NULL ;
      }
      if ( _pIndexSu )
      {
         SDB_OSS_DEL _pIndexSu ;
         _pIndexSu = NULL ;
      }
      if ( _pDataSu )
      {
         SDB_OSS_DEL _pDataSu ;
         _pDataSu = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__GETTYPEFROMFILE, "_dmsStorageUnit::_getTypeFromFile" )
   INT32 _dmsStorageUnit::_getTypeFromFile( const CHAR *dataPath,
                                            DMS_STORAGE_TYPE &type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSU__GETTYPEFROMFILE ) ;
      OSSFILE file ;
      INT64 fileSize = 0 ;
      INT64 readSize = 0 ;
      BOOLEAN isFileInvalid = FALSE ;
      CHAR fileName[ DMS_SU_FILENAME_SZ + 1 ] = { 0 } ;
      CHAR fullFilePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR eyeCatcher[DMS_HEADER_EYECATCHER_LEN + 1 ] = { 0 } ;

      SDB_ASSERT( dataPath, "Data path should not be NULL" ) ;

      ossSnprintf( fileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_DATA_SU_EXT_NAME ) ;

      rc = utilBuildFullPath( dataPath, fileName,
                              OSS_MAX_PATHSIZE, fullFilePath ) ;
      PD_RC_CHECK( rc, PDERROR, "Build full path for path[ %s ] and file[ %s ] "
                   "failed[ %d ]", dataPath, _storageInfo._suName, rc ) ;

      rc = ossOpen( fullFilePath, OSS_READONLY, OSS_DEFAULTFILE, file ) ;
      PD_RC_CHECK( rc, PDERROR, "Open file[ %s ] failed[ %d ]",
                   fullFilePath, rc ) ;
      rc = ossGetFileSize( &file, &fileSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Get file[ %s ] size failed[ %d ]",
                   fullFilePath, rc ) ;
      // Only read the eyecathcer
      if ( fileSize < DMS_HEADER_EYECATCHER_LEN )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[ %s ] size[ %lld ] is too small. Maybe the file is "
                 "corrupted", fileName, fileSize ) ;
         if ( 0 == fileSize )
         {
            isFileInvalid = TRUE ;
         }
         goto error ;
      }

      rc = ossReadN( &file, DMS_HEADER_EYECATCHER_LEN, eyeCatcher, readSize ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Read eyecatcher data from file[ %s ] failed[ %d ]",
                   fullFilePath, rc ) ;

      if ( 0 == ossStrcmp( DMS_DATASU_EYECATCHER, eyeCatcher ) )
      {
         type = DMS_STORAGE_NORMAL ;
      }
      else if ( 0 == ossStrcmp( DMS_DATACAPSU_EYECATCHER, eyeCatcher ) )
      {
         type = DMS_STORAGE_CAPPED ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Eye catcher[ %s ] in file is invalid",
                      eyeCatcher ) ;
      }
   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      if ( isFileInvalid )
      {
         INT32 tmpRC = SDB_OK ;
         tmpRC = dmsRenameInvalidFile( fullFilePath ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDERROR, "Fail to rename invalid file, rc:%d", tmpRC ) ;
         }
      }
      PD_TRACE_EXITRC( SDB__DMSSU__GETTYPEFROMFILE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}  // namespace engine

