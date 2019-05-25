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

namespace engine
{

   /*
      _dmsEventHolder implement
    */
   _dmsEventHolder::_dmsEventHolder ( dmsStorageUnit *su )
   {
      SDB_ASSERT( su, "Storage Unit is no valid" ) ;
      _su = su ;
      unregAllHandlers() ;
   }

   _dmsEventHolder::~_dmsEventHolder ()
   {
      unregAllHandlers() ;
   }

   void _dmsEventHolder::regHandler ( _IDmsEventHandler *pHandler )
   {
      if ( !pHandler )
      {
         return ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         if ( *iter == pHandler )
         {
            return ;
         }
      }

      _handlers.push_back( pHandler ) ;
   }

   void _dmsEventHolder::unregHandler ( _IDmsEventHandler *pHandler )
   {
      if ( pHandler )
      {
         return ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         if ( *iter == pHandler )
         {
            _handlers.erase( iter ) ;
            break ;
         }
      }
   }

   void _dmsEventHolder::unregAllHandlers ()
   {
      _handlers.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCRTCS, "_dmsEventHolder::onCreateCS" )
   INT32 _dmsEventHolder::onCreateCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCRTCS ) ;

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPCS, "_dmsEventHolder::onDropCS" )
   INT32 _dmsEventHolder::onDropCS ( UINT32 mask, pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPCS ) ;

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onDropCS( this, _pCacheHolder,
                                              cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPCS, rc ) ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONTRUNCCL, "_dmsEventHolder::onTruncateCL" )
   INT32 _dmsEventHolder::onTruncateCL ( UINT32 mask,
                                         const dmsEventCLItem &clItem,
                                         UINT32 newCLLID,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONTRUNCCL ) ;

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onTruncateCL( this, _pCacheHolder, clItem,
                                                  newCLLID, cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONTRUNCCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPCL, "_dmsEventHolder::onDropCL" )
   INT32 _dmsEventHolder::onDropCL ( UINT32 mask,
                                     const dmsEventCLItem &clItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPCL ) ;

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
            ++ iter )
      {
         _IDmsEventHandler *pHandler = (*iter) ;
         if ( pHandler && ( pHandler->getMask() & mask ) )
         {
            INT32 tmprc = pHandler->onDropCL( this, _pCacheHolder, clItem,
                                              cb, dpsCB ) ;
            if ( SDB_OK != tmprc )
            {
               rc = tmprc ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONDROPCL, rc ) ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONDROPIDX, "_dmsEventHolder::onDropIndex" )
   INT32 _dmsEventHolder::onDropIndex ( UINT32 mask,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONDROPIDX ) ;

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      if ( !cb || cb->getType() == EDU_TYPE_MAIN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLRSUCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCLRCLCACHES, "_dmsEventHolder::onClearCLCaches" )
   INT32 _dmsEventHolder::onClearCLCaches ( UINT32 mask,
                                            const dmsEventCLItem &clItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCLRCLCACHES ) ;

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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

      PD_TRACE_EXITRC( SDB__DMSEVTHLD_ONCLRCLCACHES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEVTHLD_ONCHGSUCACHES, "_dmsEventHolder::onChangeSUCacheConfigs" )
   INT32 _dmsEventHolder::onChangeSUCaches ( UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSEVTHLD_ONCHGSUCACHES ) ;

      for ( HANDLER_LIST::iterator iter = _handlers.begin() ;
            iter != _handlers.end() ;
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
         switch ( type )
         {
            case DMS_CACHE_TYPE_STAT :
            {
               if ( !isSysSU() )
               {
                  _pSUCaches[ type ] = SDB_OSS_NEW dmsStatCache( this ) ;
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
      INDEX_STAT_MAP::iterator iterIdx ;

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
            pCollectionStat->removeFieldStat( pIndexStat->getFirstField(),
                                              TRUE ) ;
            iterIdx = indexStats.erase( iterIdx ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSCACHEHOLDER_CHKIDXSTAT, "_dmsCacheHolder::_checkCollectionStat" )
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
   _dmsStorageUnit::_dmsStorageUnit ( const CHAR *pSUName,
                                      UINT32 sequence,
                                      utilCacheMgr *pMgr,
                                      INT32 pageSize,
                                      INT32 lobPageSize,
                                      DMS_STORAGE_TYPE type,
                                      IDmsExtDataHandler *extDataHandler )
   :_pDataSu( NULL ),
    _pIndexSu( NULL ),
    _pLobSu( NULL ),
    _pMgr( pMgr ),
    _pCacheUnit( NULL ),
    _eventHolder ( this ),
    _cacheHolder ( this )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU ) ;
      SDB_ASSERT ( pSUName, "name can't be null" ) ;

      pmdOptionsCB *options = pmdGetOptionCB() ;

      if ( 0 == pageSize )
      {
         pageSize = DMS_PAGE_SIZE_DFT ;
      }

      if ( 0 == lobPageSize )
      {
         lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      _storageInfo._pageSize = pageSize ;
      _storageInfo._lobdPageSize = lobPageSize ;
      ossStrncpy( _storageInfo._suName, pSUName, DMS_SU_NAME_SZ ) ;
      _storageInfo._suName[DMS_SU_NAME_SZ] = 0 ;
      _storageInfo._sequence = sequence ;
      _storageInfo._overflowRatio = options->getOverFlowRatio() ;
      _storageInfo._extentThreshold = options->getExtendThreshold() << 20 ;
      _storageInfo._enableSparse = options->sparseFile() ;
      _storageInfo._directIO = options->useDirectIOInLob() ;
      _storageInfo._cacheMergeSize = options->getCacheMergeSize() ;
      _storageInfo._pageAllocTimeout = options->getPageAllocTimeout() ;
      _storageInfo._dataIsOK = pmdGetStartup().isOK() ;
      _storageInfo._curLSNOnStart = pmdGetSyncMgr()->getCompleteLSN() ;
      _storageInfo._secretValue = ossPack32To64( (UINT32)time(NULL),
                                                 (UINT32)(ossRand()*239641) ) ;
      _storageInfo._type = type ;
      _storageInfo._extDataHandler = extDataHandler ;

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

      _eventHolder.unregAllHandlers() ;
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
                                BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_OPEN ) ;

      if ( !createNew )
      {
         rc = _getTypeFromFile( pDataPath, _storageInfo._type ) ;
         PD_RC_CHECK( rc, PDERROR, "Get type for storage unit[ %s ] from "
                      "file failed[ %d ]", _storageInfo._suName, rc ) ;
      }

      rc = _createStorageObjs() ;
      PD_RC_CHECK( rc, PDERROR, "Create storage objects for storage unit[ %s ] "
                   "failed[ %d ]", _storageInfo._suName, rc ) ;

      rc = _pDataSu->openStorage( pDataPath, pSyncMgr, createNew ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open storage data su failed, rc: %d", rc ) ;
         if ( createNew && SDB_FE != rc )
         {
            goto rmdata ;
         }
         goto error ;
      }

      rc = _pIndexSu->openStorage( pIndexPath, pSyncMgr, createNew ) ;
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
            goto rmdata ;
         }
         goto error ;
      }

      rc = _pLobSu->open( pLobPath, pLobMetaPath, pSyncMgr, createNew ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open storage lob, rc:%d", rc ) ;
         if ( createNew )
         {
            goto rmboth ;
         }
         goto error ;
      }

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
                    "rollback, rc: %d", _pDataSu->getSuFileName(), rc ) ;
         }
      }
      goto done ;
   rmboth:
      {
         INT32 rcTmp = _pIndexSu->removeStorage() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Failed to remove cs idnex file[%s] in "
                    "rollback, rc: %d", _pIndexSu->getSuFileName(), rc ) ;
         }
      }
      goto rmdata ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CLOSE, "_dmsStorageUnit::close" )
   void _dmsStorageUnit::close()
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_CLOSE ) ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

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

      ossStrncpy( _storageInfo._suName, pNewName, DMS_SU_NAME_SZ ) ;
      _storageInfo._suName[DMS_SU_NAME_SZ] = 0 ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_RENAMECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__RESETCOLLECTION, "_dmsStorageUnit::_resetCollection" )
   INT32 _dmsStorageUnit::_resetCollection( dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU__RESETCOLLECTION ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;

      rc = _pIndexSu->dropAllIndexes( context, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Drop all indexes failed, rc: %d", rc ) ;
      }

      rc = _pDataSu->_truncateCollection( context ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Truncate collection data failed, rc: %d", rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU__RESETCOLLECTION, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_LDEXTA, "_dmsStorageUnit::loadExtentA" )
   INT32 _dmsStorageUnit::loadExtentA ( dmsMBContext *mbContext,
                                        const CHAR *pBuffer,
                                        UINT16 numPages,
                                        const BOOLEAN toLoad,
                                        SINT32 *tAllocatedExtent )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_LDEXTA ) ;

      dmsExtRW extRW ;
      dmsExtent *sourceExt  = (dmsExtent*)pBuffer ;
      dmsExtent *extAddr = NULL ;
      SINT32 allocatedExtent = DMS_INVALID_EXTENT ;

      rc = _pDataSu->_allocateExtent( mbContext, numPages, FALSE, toLoad,
                                      &allocatedExtent ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Can't allocate extent for %d pages, rc = %d",
                  numPages, rc ) ;
         goto error ;
      }

      extRW = _pDataSu->extent2RW( allocatedExtent, mbContext->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.writePtr<dmsExtent>( 0, getPageSize() * numPages ) ;
      if ( !extAddr )
      {
         PD_LOG( PDERROR, "Get extent[%d] write address failed",
                 allocatedExtent ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy ( &((CHAR*)extAddr)[DMS_EXTENT_METADATA_SZ],
                  &pBuffer[DMS_EXTENT_METADATA_SZ],
                  _pDataSu->pageSize() * numPages  - DMS_EXTENT_METADATA_SZ ) ;

      extAddr->_recCount          = sourceExt->_recCount ;
      extAddr->_firstRecordOffset = sourceExt->_firstRecordOffset ;
      extAddr->_lastRecordOffset  = sourceExt->_lastRecordOffset ;
      extAddr->_freeSpace         = sourceExt->_freeSpace ;

      if ( tAllocatedExtent )
      {
         *tAllocatedExtent = allocatedExtent ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_LDEXTA, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_LDEXT, "_dmsStorageUnit::loadExtent" )
   INT32 _dmsStorageUnit::loadExtent ( dmsMBContext *mbContext,
                                       const CHAR *pBuffer,
                                       UINT16 numPages )
   {
      INT32 rc                 = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_LDEXT ) ;

      dmsExtRW extRW ;
      SINT32 allocatedExtent   = DMS_INVALID_EXTENT ;
      dmsExtent *extAddr       = NULL ;
      SDB_ASSERT ( pBuffer, "buffer can't be NULL" ) ;

      rc = loadExtentA ( mbContext, pBuffer, numPages, FALSE,
                         &allocatedExtent ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to loadExtentA, rc = %d",
                  numPages, rc ) ;
         goto error ;
      }

      extRW = _pDataSu->extent2RW( allocatedExtent, mbContext->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.writePtr<dmsExtent>() ;
      if ( !extAddr )
      {
         PD_LOG( PDERROR, "Get extent[%d] write address failed",
                 allocatedExtent ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _pDataSu->postLoadExt( mbContext, extAddr, allocatedExtent ) ;

      addExtentRecordCount( mbContext->mb(), extAddr->_recCount ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSU_LDEXT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_INSERTRECORD, "_dmsStorageUnit::insertRecord" )
   INT32 _dmsStorageUnit::insertRecord ( const CHAR *pName,
                                         BSONObj &record,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpscb,
                                         BOOLEAN mustOID,
                                         BOOLEAN canUnLock,
                                         dmsMBContext *context,
                                         INT64 position )
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

      rc = _pDataSu->insertRecord( context, record, cb, dpscb, mustOID,
                                   canUnLock, position ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_INSERTRECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_UPDATERECORDS, "_dmsStorageUnit::updateRecords" )
   INT32 _dmsStorageUnit::updateRecords ( const CHAR *pName,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpscb,
                                          mthMatchRuntime *matchRuntime,
                                          mthModifier &modifier,
                                          SINT64 &numRecords,
                                          SINT64 maxUpdate,
                                          dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_UPDATERECORDS ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      {
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr = 0 ;
         numRecords = 0 ;
         dmsTBScanner tbScanner( _pDataSu, context, matchRuntime,
                                 DMS_ACCESS_TYPE_UPDATE, maxUpdate ) ;
         while ( SDB_OK == ( rc = tbScanner.advance( recordID, generator,
                                                     cb ) ) )
         {
            generator.getDataPtr( recordDataPtr ) ;
            rc = _pDataSu->updateRecord( context, recordID, recordDataPtr, cb,
                                         dpscb, modifier ) ;
            PD_RC_CHECK( rc, PDERROR, "Update record failed, rc: %d", rc ) ;

            ++numRecords ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get next record, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_UPDATERECORDS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DELETERECORDS, "_dmsStorageUnit::deleteRecords" )
   INT32 _dmsStorageUnit::deleteRecords ( const CHAR *pName,
                                          pmdEDUCB * cb,
                                          SDB_DPSCB *dpscb,
                                          mthMatchRuntime *matchRuntime,
                                          SINT64 &numRecords,
                                          SINT64 maxDelete,
                                          dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_DELETERECORDS ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      {
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr = 0 ;
         numRecords = 0 ;
         dmsTBScanner tbScanner( _pDataSu, context, matchRuntime,
                                 DMS_ACCESS_TYPE_DELETE, maxDelete ) ;
         while ( SDB_OK == ( rc = tbScanner.advance( recordID, generator,
                                                     cb ) ) )
         {
            generator.getDataPtr( recordDataPtr ) ;
            rc = _pDataSu->deleteRecord( context, recordID, recordDataPtr,
                                         cb, dpscb ) ;
            PD_RC_CHECK( rc, PDERROR, "Delete record failed, rc: %d", rc ) ;

            ++numRecords ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get next record, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DELETERECORDS, rc ) ;
      return rc ;
   error :
      goto done ;
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
                                       INT32 sortBufferSize )
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
                                   isSys, sortBufferSize ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( const CHAR *pName, const CHAR *indexName,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context )
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

      rc = _pIndexSu->dropIndex( context, indexName, cb, dpscb, isSys ) ;
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
                                     BOOLEAN isSys, dmsMBContext *context )
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

      rc = _pIndexSu->dropIndex( context, indexOID, cb, dpscb, isSys ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_COUNTCOLLECTION, "_dmsStorageUnit::countCollection" )
   INT32 _dmsStorageUnit::countCollection ( const CHAR *pName,
                                            INT64 &recordNum,
                                            pmdEDUCB *cb,
                                            dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
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

      /*{
         dmsExtentItr itr( _pDataSu, context ) ;
         while ( SDB_OK == ( rc = itr.next( &pExtent, cb ) ) )
         {
            recordNum += pExtent->_recCount ;
         }
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
      }*/
      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock dms mb context[%s], rc: %d",
                      context->toString().c_str(), rc ) ;
      }
      recordNum = context->mbStat()->_totalRecords ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETSEGEXTENTS, "_dmsStorageUnit::getSegExtents" )
   INT32 _dmsStorageUnit::getSegExtents( const CHAR *pName,
                                         vector < dmsExtentID > &segExtents,
                                         dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      const dmsMBEx *mbEx          = NULL ;
      dmsExtentID firstID          = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETSEGEXTENTS ) ;
      segExtents.clear() ;

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
            segExtents.push_back( firstID ) ;
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
                                       MON_IDX_LIST &resultIndexes )
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

      rc = _getIndexes( context->mb(), resultIndexes ) ;
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
                                       MON_IDX_LIST &resultIndexes )
   {
      INT32 rc                     = SDB_OK ;
      dmsMBContext * context       = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETINDEXES_NAME ) ;

      SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

      rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                   "rc: %d", pName, rc ) ;

      rc = getIndexes( context, resultIndexes ) ;
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
   void _dmsStorageUnit::dumpInfo ( MON_CL_SIM_VEC &clList,
                                    BOOLEAN sys,
                                    BOOLEAN dumpIdx )
   {
      PD_TRACE_ENTRY( SDB__DMSSU_DUMPINFO_CLSIMVEC ) ;

      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         monCLSimple collection ;

         if ( !sys && dmsIsSysCLName( it->first ) )
         {
            ++it ;
            continue ;
         }

         if ( SDB_OK == _dumpCLInfo( collection, it->second ) )
         {
            clList.push_back ( collection ) ;
         }

         ++it ;
      }

      _pDataSu->_metadataLatch.release_shared() ;

      if ( dumpIdx )
      {
         MON_CL_SIM_VEC::iterator iter = clList.begin() ;
         while ( iter != clList.end() )
         {
            if ( SDB_OK == getIndexes( iter->_clname,
                                       iter->_idxList ) )
            {
               ++ iter ;
            }
            else
            {
               iter = clList.erase( iter ) ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_CLSIMVEC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CLSIMLIST, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo( MON_CL_SIM_LIST &clList,
                                   BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CLSIMLIST ) ;

      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         monCLSimple collection ;

         if ( !sys && dmsIsSysCLName( it->first ) )
         {
            ++it ;
            continue ;
         }

         if ( SDB_OK == _dumpCLInfo( collection, it->second ) )
         {
            clList.insert ( collection ) ;
         }

         ++it ;
      }

      _pDataSu->_metadataLatch.release_shared() ;

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_CLSIMLIST ) ;
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
         getIndexes( context, collection._idxList ) ;
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
   void _dmsStorageUnit::dumpInfo ( MON_CL_LIST &clList,
                                    BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CLLIST ) ;
      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         monCollection collection ;

         if ( !sys && dmsIsSysCLName( it->first ) )
         {
            ++it ;
            continue ;
         }

         if ( SDB_OK == _dumpCLInfo( collection, it->second ) )
         {
            clList.insert ( collection ) ;
         }

         ++it ;
      }

      _pDataSu->_metadataLatch.release_shared() ;

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_CLLIST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_SU, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( monStorageUnit &storageUnit )
   {
      const dmsStorageUnitHeader *dataHeader = _pDataSu->getHeader() ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_SU ) ;

      storageUnit.setName( CSName() ) ;

      storageUnit._pageSize = getPageSize() ;
      storageUnit._lobPageSize = getLobPageSize() ;
      storageUnit._sequence = CSSequence() ;
      storageUnit._numCollections = dataHeader->_numMB ;
      storageUnit._collectionHWM = dataHeader->_MBHWM ;
      storageUnit._size = totalSize() ;
      storageUnit._CSID = CSID() ;
      storageUnit._logicalCSID = LogicalCSID() ;

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_SU ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CSSIM, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( monCSSimple &collectionSpace,
                                    BOOLEAN sys,
                                    BOOLEAN dumpCL,
                                    BOOLEAN dumpIdx )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CSSIM ) ;

      collectionSpace.setName( CSName() ) ;
      collectionSpace._suID = CSID() ;
      collectionSpace._logicalID = LogicalCSID() ;

      if ( dumpCL )
      {
         dumpInfo ( collectionSpace._clList, sys, dumpIdx ) ;
      }

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_CSSIM ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO_CS, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( monCollectionSpace &collectionSpace,
                                    BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO_CS ) ;

      dmsStorageUnitStat statInfo ;

      getStatInfo( statInfo ) ;
      INT64 totalDataFreeSize    = totalFreeSize( DMS_SU_DATA ) +
                                   statInfo._totalDataFreeSpace ;
      INT64 totalIndexFreeSize   = totalFreeSize( DMS_SU_INDEX ) +
                                   statInfo._totalIndexFreeSpace ;
      INT64 totalLobFreeSize     = totalFreeSize( DMS_SU_LOB ) ;

      ossMemset( collectionSpace._name, 0, sizeof(collectionSpace._name) ) ;
      ossStrncpy( collectionSpace._name, CSName(), DMS_COLLECTION_SPACE_NAME_SZ );
      collectionSpace._pageSize = getPageSize() ;
      collectionSpace._lobPageSize = getLobPageSize() ;
      collectionSpace._totalSize = totalSize() ;
      collectionSpace._clNum    = statInfo._clNum ;
      collectionSpace._totalRecordNum = statInfo._totalCount ;
      collectionSpace._freeSize = totalDataFreeSize + totalIndexFreeSize +
                                  totalLobFreeSize ;
      collectionSpace._totalDataSize = totalSize( DMS_SU_DATA ) ;
      collectionSpace._freeDataSize  = totalDataFreeSize ;
      collectionSpace._totalIndexSize = totalSize( DMS_SU_INDEX ) ;
      collectionSpace._freeIndexSize = totalIndexFreeSize ;
      collectionSpace._totalLobSize = totalSize( DMS_SU_LOB ) ;
      collectionSpace._freeLobSize = totalLobFreeSize ;

      collectionSpace._dataCommitLsn = getCurrentDataLSN() ;
      collectionSpace._idxCommitLsn = getCurrentIdxLSN() ;
      collectionSpace._lobCommitLsn = getCurrentLobLSN() ;
      getValidFlag( collectionSpace._dataIsValid,
                    collectionSpace._idxIsValid,
                    collectionSpace._lobIsValid ) ;

      collectionSpace._dirtyPage = cacheUnit()->dirtyPages() ;
      collectionSpace._type = type() ;

      dumpInfo ( collectionSpace._collections, sys, FALSE ) ;

      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO_CS ) ;
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
         totalSize += ( (INT64)( _pLobSu->getHeader()->_storageUnitSize ) <<
                        _pLobSu->pageSizeSquareRoot() ) ;
         totalSize += _pLobSu->getLobData()->getFileSz() ;
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

      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         mbStat = &_pDataSu->_mbStatInfo[it->second] ;

         ++statInfo._clNum ;
         statInfo._totalCount += mbStat->_totalRecords ;
         statInfo._totalDataPages += mbStat->_totalDataPages ;
         statInfo._totalIndexPages += mbStat->_totalIndexPages ;
         statInfo._totalLobPages += mbStat->_totalLobPages ;
         statInfo._totalDataFreeSpace += mbStat->_totalDataFreeSpace ;
         statInfo._totalIndexFreeSpace += mbStat->_totalIndexFreeSpace ;

         ++it ;
      }

      _pDataSu->_metadataLatch.release_shared() ;
      PD_TRACE_EXIT ( SDB__DMSSU_GETSTATINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__DUMPCLINFO_CL, "_dmsStorageUnit::_dumpCLInfo" )
   INT32 _dmsStorageUnit::_dumpCLInfo ( monCollection &collection, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSU__DUMPCLINFO_CL ) ;

      const dmsMB *mb = NULL ;
      const dmsMBStatInfo *mbStat = NULL ;

      PD_CHECK( mbID < DMS_MME_SLOTS, SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u]", mbID ) ;

      mb = _pDataSu->getMBInfo( mbID ) ;
      mbStat = _pDataSu->getMBStatInfo( mbID ) ;

      PD_CHECK( DMS_IS_MB_INUSE ( mb->_flag ), SDB_INVALIDARG, error, PDERROR,
                "Invalid mbID [%u], metablock is not in-used", mbID ) ;

      collection.setName( CSName(), mb->_collectionName ) ;

      {
         detailedInfo &info = collection.addDetails( CSSequence(),
                                                     mb->_numIndexes,
                                                     mb->_blockID,
                                                     mb->_flag,
                                                     mb->_logicalID,
                                                     mbStat->_totalRecords,
                                                     mbStat->_totalDataPages,
                                                     mbStat->_totalIndexPages,
                                                     mbStat->_totalLobPages,
                                                     mbStat->_totalDataFreeSpace,
                                                     mbStat->_totalIndexFreeSpace ) ;

         info._attribute = mb->_attributes ;
         info._dictCreated = mb->_dictExtentID != DMS_INVALID_EXTENT ? 1 : 0 ;
         info._compressType = mb->_compressorType ;
         info._dictVersion = mb->_dictVersion ;

         info._totalLobs = mbStat->_totalLobs ;

         info._pageSize = getPageSize() ;
         info._lobPageSize = getLobPageSize() ;
         info._currCompressRatio = mbStat->_lastCompressRatio ;

         info._dataCommitLSN = mb->_commitLSN ;
         info._idxCommitLSN = mb->_idxCommitLSN ;
         info._lobCommitLSN = mb->_lobCommitLSN ;
         info._dataIsValid = mbStat->_commitFlag.peek() ? TRUE : FALSE ;
         info._idxIsValid = mbStat->_idxCommitFlag.peek() ? TRUE : FALSE ;
         info._lobIsValid = mbStat->_lobCommitFlag.peek() ? TRUE : FALSE ;


         if ( !_pLobSu->isOpened() )
         {
            info._lobCommitLSN = 0 ;
            info._lobIsValid = TRUE ;
         }
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

   done :
      PD_TRACE_EXITRC( SDB__DMSSU__DUMPCLINFO_CLSIMPLE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__GETINDEXES, "_dmsStorageUnit::_getIndexes" )
   INT32 _dmsStorageUnit::_getIndexes ( const dmsMB *mb,
                                        MON_IDX_LIST &resultIndexes )
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

         monIndex indexItem ;
         ixmIndexCB indexCB ( mb->_indexExtent[indexID], _pIndexSu, NULL ) ;

         indexItem._indexFlag = indexCB.getFlag () ;
         indexItem._scanExtLID = indexCB.scanExtLID () ;
         indexItem._indexLID = indexCB.getLogicalID () ;
         indexItem._version = indexCB.version () ;
         indexItem._indexDef = indexCB.getDef().copy () ;
         resultIndexes.push_back ( indexItem ) ;
      }

      PD_TRACE_EXITRC( SDB__DMSSU__GETINDEXES, rc ) ;

      return rc ;
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
         if ( 0 == ossStrcmp( indexCB.getName(), pIndexName ) )
         {
            resultIndex._indexFlag = indexCB.getFlag () ;
            resultIndex._scanExtLID = indexCB.scanExtLID () ;
            resultIndex._indexLID = indexCB.getLogicalID () ;
            resultIndex._version = indexCB.version () ;
            resultIndex._indexDef = indexCB.getDef().copy () ;

            rc = SDB_OK ;
            break ;
         }
      }

      PD_TRACE_EXITRC ( SDB__DMSSU__GETINDEX, rc ) ;

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
                    _pLobSu->getSuFileName(), rc ) ;
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
                    _pLobSu->getSuFileName(), rc ) ;
            rc = rc ? rc : rcTmp ;
         }
      }

      if ( NULL != _pDataSu )
      {
         _pDataSu->lock() ;
         rc = _pDataSu->sync( TRUE, sync, cb ) ;
         _pDataSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rc ) ;
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

      lobFlag = ( NULL == _pLobSu || !_pLobSu->isOpened() ) ?
                TRUE : ( _pLobSu->getCommitFlag() ? TRUE : FALSE ) ;
   }

   _IDmsEventHolder *_dmsStorageUnit::getEventHolder ()
   {
      return &_eventHolder ;
   }

   void _dmsStorageUnit::regEventHandler ( _IDmsEventHandler *pHandler )
   {
      _eventHolder.regHandler( pHandler ) ;
   }

   void _dmsStorageUnit::unregEventHandler ( _IDmsEventHandler *pHandler )
   {
      _eventHolder.unregHandler( pHandler ) ;
   }

   void _dmsStorageUnit::unregEventHandlers ()
   {
      _eventHolder.unregAllHandlers() ;
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

      _pDataSu = getDMSStorageDataFactory()->createProduct( _storageInfo._type,
                                                            dataFileName,
                                                            &_storageInfo,
                                                            &_eventHolder ) ;
      if ( !_pDataSu )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create storage data structure of type[ %d ] with "
                 "data file[ %s ] failed[ %d ]",
                 _storageInfo._type, dataFileName, rc ) ;
         goto error ;
      }

      _pIndexSu = SDB_OSS_NEW dmsStorageIndex( idxFileName, &_storageInfo,
                                               _pDataSu ) ;
      if ( !_pIndexSu )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create index structure with "
                 "index file[ %s ] failed[ %d ]",
                 idxFileName, rc ) ;
         goto error ;
      }

      _pCacheUnit = SDB_OSS_NEW utilCacheUnit( _pMgr ) ;
      if ( !_pCacheUnit )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Create cache unit failed[ %d ]", rc ) ;
         goto error ;
      }

      ossMemset( dataFileName, 0, sizeof( dataFileName ) ) ;
      ossMemset( idxFileName, 0 , sizeof( idxFileName ) ) ;
      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_LOB_META_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, _storageInfo._sequence,
                   DMS_LOB_DATA_SU_EXT_NAME ) ;

      _pLobSu = SDB_OSS_NEW dmsStorageLob( dataFileName, idxFileName,
                                           &_storageInfo, _pDataSu,
                                           _pCacheUnit ) ;
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
      if ( _pDataSu )
      {
         SDB_OSS_DEL _pDataSu ;
         _pDataSu = NULL ;
      }
      if ( _pIndexSu )
      {
         SDB_OSS_DEL _pIndexSu ;
         _pIndexSu = NULL ;
      }
      if ( _pCacheUnit )
      {
         SDB_OSS_DEL _pCacheUnit ;
         _pCacheUnit = NULL ;
      }
      if ( _pLobSu )
      {
         SDB_OSS_DEL _pLobSu ;
         _pLobSu = NULL ;
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
      if ( fileSize < DMS_HEADER_EYECATCHER_LEN )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File size[ %lld ] is too small. Maybe the file is "
                 "corrupted", fileSize ) ;
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
      PD_TRACE_EXITRC( SDB__DMSSU__GETTYPEFROMFILE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}  // namespace engine

