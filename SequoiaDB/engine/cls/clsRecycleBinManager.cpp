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

#include "clsRecycleBinManager.hpp"
#include "rtnLocalTask.hpp"
#include "clsUniqueIDCheckJob.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   // query 1000 recycle bin items for each time
   #define CLS_QUERY_RECYCLE_ITEM_STEP ( 1000 )

   /*
      _clsBlockDropCSContext define
    */
   class _clsBlockDropCSContext : public IContext
   {
   public:
      _clsBlockDropCSContext( const CHAR *csName,
                              UINT32 suLogicalID,
                              pmdEDUCB *cb ) ;
      virtual ~_clsBlockDropCSContext() ;

      virtual INT32 pause() ;
      virtual INT32 resume() ;

   protected:
      const CHAR * _csName ;
      UINT32       _suLogicalID ;
      pmdEDUCB *   _cb ;
   } ;

   typedef class _clsBlockDropCSContext clsBlockDropCSContext ;

   _clsBlockDropCSContext::_clsBlockDropCSContext( const CHAR *csName,
                                                   UINT32 suLogicalID,
                                                   pmdEDUCB *cb )

   : _csName( csName ),
     _suLogicalID( suLogicalID ),
     _cb( cb )
   {
   }

   _clsBlockDropCSContext::~_clsBlockDropCSContext()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSBLOCKDROPCSCTX_PAUSE, "_clsBlockDropCSContext::pause" )
   INT32 _clsBlockDropCSContext::pause()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBLOCKDROPCSCTX_PAUSE ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;

      // delete contexts of self
      if ( NULL != _cb )
      {
         rtnDelContextForCollectionSpace( _csName, _suLogicalID, _cb ) ;
      }

      // notify others to delete contexts of the same collection space
      rtnCB->preDelContext( _csName, _suLogicalID ) ;

      // notify others to give up wait locks
      if ( NULL != _cb && _cb->getTransExecutor()->useTransLock() )
      {
         transCB->transLockKillWaiters( _suLogicalID,
                                        DMS_INVALID_MBID,
                                        NULL,
                                        SDB_DPS_TRANS_LOCK_INCOMPATIBLE ) ;
      }

      PD_TRACE_EXITRC( SDB__CLSBLOCKDROPCSCTX_PAUSE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSBLOCKDROPCSCTX_RESUME, "_clsBlockDropCSContext::resume" )
   INT32 _clsBlockDropCSContext::resume()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBLOCKDROPCSCTX_RESUME ) ;

      // do nothing

      PD_TRACE_EXITRC( SDB__CLSBLOCKDROPCSCTX_RESUME, rc ) ;

      return rc ;
   }

   /*
      _clsRecycleBinManager implement
    */
   _clsRecycleBinManager::_clsRecycleBinManager()
   : _freezingWindow( NULL ),
     _localTaskManager( NULL )
   {
   }

   _clsRecycleBinManager::~_clsRecycleBinManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_INIT, "_clsRecycleBinManager::init" )
   INT32 _clsRecycleBinManager::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_INIT ) ;

      rc = _BASE::init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init based recycle bin manager, "
                   "rc: %d", rc ) ;

      if ( NULL != pmdGetKRCB()->getClsCB() &&
           NULL != pmdGetKRCB()->getClsCB()->getShardCB() )
      {
         _freezingWindow =
               pmdGetKRCB()->getClsCB()->getShardCB()->getFreezingWindow() ;
      }
      if ( NULL != pmdGetKRCB()->getRTNCB() &&
           NULL != pmdGetKRCB()->getRTNCB()->getLTMgr() )
      {
         _localTaskManager = pmdGetKRCB()->getRTNCB()->getLTMgr() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_FINI, "_clsRecycleBinManager::fini" )
   void _clsRecycleBinManager::fini()
   {
      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_FINI ) ;

      _BASE::fini() ;
      _freezingWindow = NULL ;
      _localTaskManager = NULL ;

      PD_TRACE_EXIT( SDB__CLSRECYBINMGR_FINI ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_DROPITEMCHK, "_clsRecycleBinManager::dropItemWithCheck" )
   INT32 _clsRecycleBinManager::dropItemWithCheck( const utilRecycleItem &item,
                                                   pmdEDUCB *cb,
                                                   BOOLEAN checkCatalog,
                                                   BOOLEAN &isDropped )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_DROPITEMCHK ) ;

      const CHAR *originName = item.getOriginName() ;
      const CHAR *recycleName = item.getRecycleName() ;

      isDropped = FALSE ;

      PD_CHECK( pmdIsPrimary(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to check primary status" ) ;
      PD_CHECK( !cb->isInterrupted(), SDB_APP_INTERRUPT, error, PDERROR,
                "Failed to drop item, session is interrupted" ) ;

      if ( checkCatalog )
      {
         shardCB *pShdMgr = sdbGetShardCB() ;
         utilRecycleItem remoteItem ;
         rc = pShdMgr->rGetRecycleItem( cb, item.getRecycleID(), remoteItem ) ;
         if ( SDB_OK == rc )
         {
            // recycle item still exists, can not job
            goto done ;
         }
         else if ( SDB_RECYCLE_ITEM_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s] "
                      "from catalog, rc: %d", recycleName, rc ) ;
      }

      rc = _dropItemImpl( item, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                   "[origin %s, recycle %s], rc: %d", originName,
                   recycleName, rc ) ;

      isDropped = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_DROPITEMCHK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPALLITEMSTYPE, "_clsRecycleBinManager::_dropAllItemInType" )
   INT32 _clsRecycleBinManager::_dropAllItemInType( UTIL_RECYCLE_TYPE type,
                                                    pmdEDUCB *cb,
                                                    BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPALLITEMSTYPE ) ;

      BSONObj matcher, orderBy ;
      CHAR lastRecycleName[ UTIL_RECYCLE_NAME_SZ + 1 ] = { '\0' } ;

      try
      {
         orderBy = BSON( FIELD_NAME_RECYCLE_NAME << 1 ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build order-by BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      while ( TRUE )
      {
         UTIL_RECY_ITEM_LIST itemList ;

         try
         {
            matcher = BSON( FIELD_NAME_TYPE <<
                                  utilGetRecycleTypeName( type ) <<
                            FIELD_NAME_RECYCLE_NAME <<
                                  BSON( "$gt" << lastRecycleName ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build query BSON, occur exception %s",
                             e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         // query for a batch of items (1000 for each time)
         rc = getItems( matcher, orderBy, _hintName,
                        CLS_QUERY_RECYCLE_ITEM_STEP, cb, itemList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items after [%s], "
                      "rc: %d", lastRecycleName, rc ) ;

         if ( itemList.empty() )
         {
            break ;
         }

         for ( UTIL_RECY_ITEM_LIST::iterator iter = itemList.begin() ;
               iter != itemList.end() ;
               ++ iter )
         {
            BOOLEAN isDropped = FALSE ;
            rc = dropItemWithCheck( ( *iter ), cb, checkCatalog, isDropped ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [%s], "
                         "rc: %d", iter->getRecycleName(), rc ) ;
         }

         ossStrncpy( lastRecycleName, itemList.back().getRecycleName(),
                     UTIL_RECYCLE_NAME_SZ ) ;
         lastRecycleName[ UTIL_RECYCLE_NAME_SZ ] = '\0' ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPALLITEMSTYPE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_DROPALLITEMS, "_clsRecycleBinManager::dropAllItems" )
   INT32 _clsRecycleBinManager::dropAllItems( pmdEDUCB *cb,
                                              BOOLEAN checkCatalog )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_DROPALLITEMS ) ;

      rc = _dropAllItemInType( UTIL_RECYCLE_CS, cb, checkCatalog ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collection space items, "
                   "rc: %d", rc ) ;

      rc = _dropAllItemInType( UTIL_RECYCLE_CL, cb, checkCatalog ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collection items, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_DROPALLITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__REGBLOCKCL, "_clsRecycleBinManager::_regBlockCL" )
   INT32 _clsRecycleBinManager::_regBlockCL( const CHAR *originName,
                                             const CHAR *recycleName,
                                             const dmsEventCLItem &clItem,
                                             UINT64 &opID,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__REGBLOCKCL ) ;

      UINT64 blockID = 0 ;

      if ( NULL != _freezingWindow )
      {
         rc = _freezingWindow->registerCL( originName, blockID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register origin name [%s], "
                      "rc: %d", originName, rc ) ;

         PD_LOG( PDDEBUG, "Start to block collection [%s] block ID [%llu]",
                 originName, blockID ) ;

         rc = _freezingWindow->registerCL( recycleName, blockID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register recycle name [%s], "
                      "rc: %d", recycleName, rc ) ;

         PD_LOG( PDDEBUG, "Start to block collection [%s] block ID [%llu]",
                 recycleName, blockID ) ;

         {
            const CHAR *mainCLName = cb->getCurMainCLName() ;
            clsFreezingCLChecker checker( _freezingWindow, blockID,
                                          originName, mainCLName ) ;
            // already lock Z, so no need to wait transaction
            rc = checker.enableEDUCheck( cb, EDU_BLOCK_FREEZING_WND ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to enable EDU check for "
                         "collection [%s], rc: %d", originName, rc ) ;

            rc = checker.enableCtxCheck( cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to enable context check for "
                         "collection [%s], rc: %d", originName, rc ) ;

            // pass the meta data block context to checker,
            // pause the meta data block to allow blocking operations
            // contexts to finish
            cb->setBlock( EDU_BLOCK_RENAMECHK,
                          "Waiting for writing operations check in "
                          "recycle" ) ;
            rc = checker.loopCheck( cb,
                                    CLS_BLOCKWRITE_TIMES,
                                    clItem._mbContext ) ;
            cb->unsetBlock() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check freezing window for "
                         "collection [%s], rc: %d", originName, rc ) ;
         }
      }

      opID = blockID ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__REGBLOCKCL, rc ) ;
      return rc ;

   error:
      if ( 0 != blockID )
      {
         _unregBlockCL( originName, recycleName, blockID ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__UNREGBLOCKCL, "_clsRecycleBinManager::_unregBlockCL" )
   void _clsRecycleBinManager::_unregBlockCL( const CHAR *originName,
                                              const CHAR *recycleName,
                                              UINT64 opID )
   {
      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__UNREGBLOCKCL ) ;

      if ( NULL != _freezingWindow )
      {
         _freezingWindow->unregisterCL( originName, opID ) ;
         PD_LOG( PDDEBUG, "End to block collection [%s] block ID [%llu]",
                 originName, opID ) ;
         _freezingWindow->unregisterCL( recycleName, opID ) ;
         PD_LOG( PDDEBUG, "End to block collection [%s] block ID [%llu]",
                 recycleName, opID ) ;
      }

      PD_TRACE_EXIT( SDB__CLSRECYBINMGR__UNREGBLOCKCL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__REGBLOCKCS, "_clsRecycleBinManager::_regBlockCS" )
   INT32 _clsRecycleBinManager::_regBlockCS( const CHAR *originName,
                                             const CHAR *recycleName,
                                             const dmsEventSUItem &suItem,
                                             UINT64 &opID,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__REGBLOCKCS ) ;

      UINT64 blockID = 0 ;

      if ( NULL != _freezingWindow )
      {
         // already lock Z, so no need to wait
         rc = _freezingWindow->registerCS( originName, blockID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register origin name [%s], "
                      "rc: %d", originName, rc ) ;

         PD_LOG( PDDEBUG, "Start to block collection space [%s] block ID [%llu]",
                 originName, blockID ) ;

         rc = _freezingWindow->registerCS( recycleName, blockID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register recycle name [%s], "
                      "rc: %d", recycleName, rc ) ;

         PD_LOG( PDDEBUG, "Start to block collection space [%s] block ID [%llu]",
                 recycleName, blockID ) ;

         {
            clsFreezingCSChecker checker( _freezingWindow, blockID,
                                          originName ) ;

            clsBlockDropCSContext context( originName,
                                           suItem._suLID,
                                           cb ) ;

            // already lock Z, so no need to wait transaction
            rc = checker.enableEDUCheck( cb, EDU_BLOCK_FREEZING_WND ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to enable EDU check for "
                         "collection space [%s], rc: %d", originName, rc ) ;

            rc = checker.enableCtxCheck( cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to enable context check for "
                         "collection space [%s], rc: %d", originName, rc ) ;

            cb->setBlock( EDU_BLOCK_RENAMECHK,
                          "Waiting for writing operations check in recycle" ) ;
            rc = checker.loopCheck( cb, CLS_BLOCKWRITE_TIMES, &context ) ;
            cb->unsetBlock() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check freezing window for "
                         "collection space [%s], rc: %d", originName, rc ) ;
         }
      }

      opID = blockID ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__REGBLOCKCS, rc ) ;
      return rc ;

   error:
      if ( 0 != blockID )
      {
         _unregBlockCS( originName, recycleName, blockID ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__UNREGBLOCKCS, "_clsRecycleBinManager::_unregBlockCS" )
   void _clsRecycleBinManager::_unregBlockCS( const CHAR *originName,
                                              const CHAR *recycleName,
                                              UINT64 opID )
   {
      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__UNREGBLOCKCS ) ;

      if ( NULL != _freezingWindow )
      {
         _freezingWindow->unregisterCS( originName, opID ) ;
         PD_LOG( PDDEBUG, "End to block collection space [%s] block ID [%llu]",
                 originName, opID ) ;
         _freezingWindow->unregisterCS( recycleName, opID ) ;
         PD_LOG( PDDEBUG, "End to block collection space [%s] block ID [%llu]",
                 recycleName, opID ) ;
      }

      PD_TRACE_EXIT( SDB__CLSRECYBINMGR__UNREGBLOCKCS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__CRTRECYCLTASK, "_clsRecycleBinManager::_createRecycleCLTask" )
   INT32 _clsRecycleBinManager::_createRecycleCLTask( const CHAR *originName,
                                                      const CHAR *recycleName,
                                                      const utilRecycleItem &item,
                                                      pmdEDUCB *cb,
                                                      UINT64 &taskID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__CRTRECYCLTASK ) ;

      if ( NULL != _localTaskManager )
      {
         rtnLocalTaskPtr taskPtr ;
         rtnLTRecycleCL *pRenameTask = NULL ;

         rc = rtnGetLTFactory()->create( RTN_LOCAL_TASK_RECYCLECL, taskPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate recycle "
                      "collection task, rc: %d", rc ) ;

         pRenameTask = dynamic_cast< rtnLTRecycleCL * >( taskPtr.get() ) ;
         SDB_ASSERT( NULL != pRenameTask, "local task is invalid" ) ;

         pRenameTask->setInfo( originName, recycleName, item ) ;

         rc = _localTaskManager->addTask( taskPtr, cb, _dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add task [%s], rc: %d",
                      taskPtr->toPrintString().c_str(), rc ) ;

         taskID = taskPtr->getTaskID() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__CRTRECYCLTASK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__CRTRECYCSTASK, "_clsRecycleBinManager::_createRecycleCSTask" )
   INT32 _clsRecycleBinManager::_createRecycleCSTask( const CHAR *originName,
                                                      const CHAR *recycleName,
                                                      const utilRecycleItem &item,
                                                      pmdEDUCB *cb,
                                                      UINT64 &taskID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__CRTRECYCSTASK ) ;

      if ( NULL != _localTaskManager )
      {
         rtnLocalTaskPtr taskPtr ;
         rtnLTRecycleCS *pRenameTask = NULL ;

         rc = rtnGetLTFactory()->create( RTN_LOCAL_TASK_RECYCLECS, taskPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate recycle "
                      "collection task, rc: %d", rc ) ;

         pRenameTask = dynamic_cast< rtnLTRecycleCS * >( taskPtr.get() ) ;
         SDB_ASSERT( NULL != pRenameTask, "local task is invalid" ) ;

         pRenameTask->setInfo( originName, recycleName, item ) ;

         rc = _localTaskManager->addTask( taskPtr, cb, _dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add task [%s], rc: %d",
                      taskPtr->toPrintString().c_str(), rc ) ;

         taskID = taskPtr->getTaskID() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__CRTRECYCSTASK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__CLEANLOCALTASK, "_clsRecycleBinManager::_cleanLocalTask" )
   void _clsRecycleBinManager::_cleanLocalTask( UINT64 taskID,
                                                UINT64 &opID,
                                                pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__CLEANLOCALTASK ) ;

      if ( NULL != _localTaskManager )
      {
         rtnLocalTaskPtr taskPtr ;

         taskPtr = _localTaskManager->getTask( taskID ) ;
         if ( NULL != taskPtr.get() && taskPtr->isTaskValid() )
         {
            /// context is killed by interrupted
            if ( cb->isInterrupted() )
            {
               if ( SDB_OK == clsStartRenameCheckJob( taskPtr, opID ) )
               {
                  opID = 0 ;
               }
               else
               {
                  _localTaskManager->removeTask( taskPtr, cb, _dpsCB ) ;
               }
            }
            else
            {
               _localTaskManager->removeTask( taskPtr, cb, _dpsCB ) ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__CLSRECYBINMGR__CLEANLOCALTASK ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONCHECKTRUNCCL, "_clsRecycleBinManager::onCheckTruncCL" )
   INT32 _clsRecycleBinManager::onCheckTruncCL( IDmsEventHolder *pEventHolder,
                                                IDmsSUCacheHolder *pCacheHolder,
                                                const dmsEventCLItem &clItem,
                                                dmsTruncCLOptions *options,
                                                pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONCHECKTRUNCCL ) ;

      if ( NULL != dpsCB &&
           NULL != options &&
           options->_recycleItem.isValid() )
      {
         UINT64 opID = 0 ;
         UINT64 taskID = 0 ;

         const utilRecycleItem &item = options->_recycleItem ;
         const CHAR *origFullName = item.getOriginName() ;
         CHAR recyFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

         ossSnprintf( recyFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                      pEventHolder->getCSName(), item.getRecycleName() ) ;

         rc = _regBlockCL( origFullName, recyFullName, clItem, opID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register block ID, rc: %d",
                      rc ) ;
         options->_blockOpID = opID ;

         rc = _createRecycleCLTask( origFullName, recyFullName, item, cb,
                                    taskID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create local task to "
                      "recycle truncate collection, rc: %d", rc ) ;

         options->_localTaskID = taskID ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONCHECKTRUNCCL, rc ) ;
      return rc ;

   error:
      if ( NULL != options &&
           0 != options->_blockOpID )
      {
         const utilRecycleItem &item = options->_recycleItem ;
         SDB_ASSERT( options->_recycleItem.isValid(), "should be valid" ) ;
         const CHAR *origFullName = item.getOriginName() ;
         CHAR recyFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         ossSnprintf( recyFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                      pEventHolder->getCSName(), item.getRecycleName() ) ;
         _unregBlockCL( origFullName, recyFullName, options->_blockOpID ) ;
         options->_blockOpID = 0 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__RECYTRUNCCL, "_clsRecycleBinManager::_recycleTruncCL" )
   INT32 _clsRecycleBinManager::_recycleTruncCL( const CHAR *clShortName,
                                                 dmsStorageUnit *su,
                                                 dmsMBContext *mbContext,
                                                 const utilRecycleItem &item,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__RECYTRUNCCL ) ;

      SDB_ASSERT( NULL != clShortName, "collection short name is invalid" ) ;

      const CHAR *newName = item.getRecycleName() ;
      utilCLUniqueID origUniqueID = (utilCLUniqueID)( item.getOriginID() ) ;
      utilCLUniqueID newUniqueID = UTIL_UNIQUEID_NULL ;
      UINT32 newStartLID = DMS_INVALID_CLID ;

      PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                "Failed to recycle collection [%s], "
                "storage unit is invalid",
                item.getOriginName() ) ;
      PD_CHECK( NULL != mbContext, SDB_SYS, error, PDERROR,
                "Failed to recycle collection [%s], "
                "meta block context is invalid",
                item.getOriginName() ) ;
      PD_CHECK( mbContext->isMBLock( EXCLUSIVE ), SDB_SYS, error, PDERROR,
                "Failed to recycle collection [%s], "
                "meta block context is not locked in exclusive",
                item.getOriginName() ) ;

      // keep collection space unique ID
      newUniqueID = utilBuildCLUniqueID( su->CSUniqueID(),
                                         UTIL_CLINNERID_LOCAL ) ;

      // change the start LID in meta data lock, so other mb context won't
      // get the origin name with new start LID
      rc = su->data()->renameCollection( clShortName, newName, cb, NULL, TRUE,
                                         newUniqueID, &newStartLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection from "
                   "[%s] to [%s], rc: %d", clShortName, newName, rc ) ;

      // clear truncate flag, so other mbContext can detect collection truncated
      DMS_MB_STATINFO_SET_TRUNCATED( mbContext->mbStat()->_flag ) ;

      rc = su->data()->copyCollection( mbContext, clShortName, origUniqueID,
                                       cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to copy collection [%s], "
                   "rc: %d", clShortName, rc ) ;

      PD_LOG( PDEVENT, "Recycle truncate collection [%s] to [%s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__RECYTRUNCCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONTRUNCCL, "_clsRecycleBinManager::onTruncateCL" )
   INT32 _clsRecycleBinManager::onTruncateCL( SDB_EVENT_OCCUR_TYPE type,
                                              IDmsEventHolder *pEventHolder,
                                              IDmsSUCacheHolder *pCacheHolder,
                                              const dmsEventCLItem &clItem,
                                              dmsTruncCLOptions *options,
                                              pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONTRUNCCL ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         if ( NULL != options &&
              options->_recycleItem.isValid() )
         {
            const utilRecycleItem &item = options->_recycleItem ;
            dmsStorageUnit *su = NULL ;
            dmsMBContext *mbContext = clItem._mbContext ;

            dmsEventHolder *holder =
                                 dynamic_cast<dmsEventHolder *>( pEventHolder ) ;
            PD_CHECK( NULL != holder, SDB_SYS, error, PDERROR,
                      "Failed to recycle collection [%s], failed to get "
                      "dms event holder from event holder",
                      item.getOriginName() ) ;

            su = holder->getSU() ;
            PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                      "Failed to recycle collection [%s], failed to get "
                      "storage unit from event holder",
                      item.getOriginName() ) ;

            if ( options->_needSaveItem && NULL != dpsCB )
            {
               rc = saveItem( item, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to save recycle item "
                            "[origin %s, recycle %s], rc: %d",
                            item.getOriginName(),
                            item.getRecycleName(), rc ) ;
            }

            rc = _recycleTruncCL( clItem._pCLName, su, mbContext, item, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle truncate collection "
                         "[origin %s, recycle %s], rc: %d",
                         item.getOriginName(),
                         item.getRecycleName(), rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONTRUNCCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONCLEANTRUNCCL, "_clsRecycleBinManager::onCleanTruncCL" )
   INT32 _clsRecycleBinManager::onCleanTruncCL( IDmsEventHolder *pEventHolder,
                                                IDmsSUCacheHolder *pCacheHolder,
                                                const dmsEventCLItem &clItem,
                                                dmsTruncCLOptions *options,
                                                pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONCLEANTRUNCCL ) ;

      if ( NULL != options &&
           options->_recycleItem.isValid() )
      {
         UINT64 blockID = options->_blockOpID ;
         UINT64 taskID = options->_localTaskID ;

         const utilRecycleItem &item = options->_recycleItem ;
         const CHAR *origFullName = item.getOriginName() ;
         CHAR recyFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

         ossSnprintf( recyFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                      pEventHolder->getCSName(), item.getRecycleName() ) ;

         if ( 0 != taskID )
         {
            _cleanLocalTask( taskID, blockID, cb ) ;
         }
         if ( 0 != blockID )
         {
            _unregBlockCL( origFullName, recyFullName, blockID ) ;
         }
         options->_localTaskID = 0 ;
         options->_blockOpID = 0 ;
      }

      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONCLEANTRUNCCL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONCHECKDROPCL, "_clsRecycleBinManager::onCheckDropCL" )
   INT32 _clsRecycleBinManager::onCheckDropCL( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               dmsDropCLOptions *options,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONCHECKDROPCL ) ;

      if ( NULL != dpsCB &&
           NULL != options &&
           options->_recycleItem.isValid() )
      {
         UINT64 opID = 0 ;
         UINT64 taskID = 0 ;

         const utilRecycleItem &item = options->_recycleItem ;
         const CHAR *origFullName = item.getOriginName() ;
         CHAR recyFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

         ossSnprintf( recyFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                      pEventHolder->getCSName(), item.getRecycleName() ) ;

         rc = _regBlockCL( origFullName, recyFullName, clItem, opID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register block ID, rc: %d",
                      rc ) ;
         options->_blockOpID = opID ;

         rc = _createRecycleCLTask( origFullName, recyFullName, item, cb,
                                    taskID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create local task to "
                      "recycle drop collection, rc: %d", rc ) ;

         options->_localTaskID = taskID ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONCHECKDROPCL, rc ) ;
      return rc ;

   error:
      if ( NULL != options &&
           0 != options->_blockOpID )
      {
         const utilRecycleItem &item = options->_recycleItem ;
         SDB_ASSERT( options->_recycleItem.isValid(), "should be valid" ) ;
         const CHAR *origFullName = item.getOriginName() ;
         CHAR recyFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         ossSnprintf( recyFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                      pEventHolder->getCSName(), item.getRecycleName() ) ;
         _unregBlockCL( origFullName, recyFullName, options->_blockOpID ) ;
         options->_blockOpID = 0 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__RECYDROPCL, "_clsRecycleBinManager::_recycleDropCL" )
   INT32 _clsRecycleBinManager::_recycleDropCL( const CHAR *clShortName,
                                                dmsStorageUnit *su,
                                                dmsMBContext *mbContext,
                                                const utilRecycleItem &item,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__RECYDROPCL ) ;

      SDB_ASSERT( NULL != clShortName, "collection short name is invalid" ) ;

      const CHAR *newName = item.getRecycleName() ;
      utilCLUniqueID newUniqueID = UTIL_UNIQUEID_NULL ;
      UINT32 newStartLID = DMS_INVALID_CLID ;

      PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                "Failed to recycle collection [%s], "
                "storage unit is invalid",
                item.getOriginName() ) ;
      PD_CHECK( NULL != mbContext, SDB_SYS, error, PDERROR,
                "Failed to recycle collection [%s], "
                "meta block context is invalid",
                item.getOriginName() ) ;
      PD_CHECK( mbContext->isMBLock( EXCLUSIVE ), SDB_SYS, error, PDERROR,
                "Failed to recycle collection [%s], "
                "meta block context is not locked in exclusive",
                item.getOriginName() ) ;

      // keep collection space unique ID
      newUniqueID = utilBuildCLUniqueID( su->CSUniqueID(),
                                         UTIL_CLINNERID_LOCAL ) ;

      // change the start LID in meta data lock, so other mb context won't
      // get the origin name with new start LID
      rc = su->data()->renameCollection( clShortName, newName, cb, NULL, TRUE,
                                         newUniqueID, &newStartLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection from "
                   "[%s] to [%s], rc: %d", clShortName, newName, rc ) ;

      rc = su->data()->recycleCollection( mbContext, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection [%s], rc: %d",
                   newName, rc ) ;

      // clear truncate flag, so other mbContext can detect collection dropped
      DMS_MB_STATINFO_CLEAR_TRUNCATED( mbContext->mbStat()->_flag ) ;

      PD_LOG( PDEVENT, "Recycle drop collection [%s] to [%s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__RECYDROPCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONDROPCL, "_clsRecycleBinManager::onDropCL" )
   INT32 _clsRecycleBinManager::onDropCL( SDB_EVENT_OCCUR_TYPE type,
                                          IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder,
                                          const dmsEventCLItem &clItem,
                                          dmsDropCLOptions *options,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONDROPCL ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         if ( NULL != options &&
              options->_recycleItem.isValid() )
         {
            const utilRecycleItem &item = options->_recycleItem ;
            dmsStorageUnit *su = NULL ;
            dmsMBContext *mbContext = clItem._mbContext ;

            dmsEventHolder *holder =
                              dynamic_cast<dmsEventHolder *>( pEventHolder ) ;
            PD_CHECK( NULL != holder, SDB_SYS, error, PDERROR,
                      "Failed to recycle collection [%s], failed to get "
                      "dms event holder from event holder",
                      item.getOriginName() ) ;

            su = holder->getSU() ;
            PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                      "Failed to recycle collection [%s], failed to get "
                      "storage unit from event holder",
                      item.getOriginName() ) ;

            if ( options->_needSaveItem && NULL != dpsCB )
            {
               rc = saveItem( item, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to save recycle item "
                            "[origin %s, recycle %s], rc: %d",
                            item.getOriginName(),
                            item.getRecycleName(), rc ) ;
            }

            rc = _recycleDropCL( clItem._pCLName, su, mbContext, item, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle drop collection "
                         "[origin %s, recycle %s], rc: %d",
                         item.getOriginName(),
                         item.getRecycleName(), rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONDROPCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONCLEANDROPCL, "_clsRecycleBinManager::onCleanDropCL" )
   INT32 _clsRecycleBinManager::onCleanDropCL( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               dmsDropCLOptions *options,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONCLEANDROPCL ) ;

      if ( NULL != options &&
           options->_recycleItem.isValid() )
      {
         UINT64 blockID = options->_blockOpID ;
         UINT64 taskID = options->_localTaskID ;

         const utilRecycleItem &item = options->_recycleItem ;
         const CHAR *origFullName = item.getOriginName() ;
         CHAR recyFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

         ossSnprintf( recyFullName, DMS_COLLECTION_FULL_NAME_SZ, "%s.%s",
                      pEventHolder->getCSName(), item.getRecycleName() ) ;

         if ( 0 != taskID )
         {
            _cleanLocalTask( taskID, blockID, cb ) ;
         }
         if ( 0 != blockID )
         {
            _unregBlockCL( origFullName, recyFullName, blockID ) ;
         }
         options->_localTaskID = 0 ;
         options->_blockOpID = 0 ;
      }

      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONCLEANDROPCL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONCHECKDROPCS, "_clsRecycleBinManager::onCheckDropCS" )
   INT32 _clsRecycleBinManager::onCheckDropCS( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventSUItem &suItem,
                                               dmsDropCSOptions *options,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONCHECKDROPCS ) ;

      if ( NULL != dpsCB &&
           NULL != options &&
           options->_recycleItem.isValid() )
      {
         UINT64 opID = 0 ;
         UINT64 taskID = 0 ;

         const utilRecycleItem &item = options->_recycleItem ;
         const CHAR *originName = item.getOriginName() ;
         const CHAR *recycleName = item.getRecycleName() ;

         rc = _regBlockCS( originName, recycleName, suItem, opID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register block ID, rc: %d",
                      rc ) ;
         options->_blockOpID = opID ;

         /// log to .SEQUOIADB_RENAME_INFO
         utilRenameLog aLog( originName, recycleName ) ;

         rc = options->_logger.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init rename logger, "
                      "rc: %d", rc );

         rc = options->_logger.log( aLog ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to log rename info to file, "
                      "rc: %d", rc ) ;

         rc = _createRecycleCSTask( originName, recycleName, item, cb, taskID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create local task to "
                      "recycle drop collection collection, rc: %d", rc ) ;

         options->_localTaskID = taskID ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONCHECKDROPCS, rc ) ;
      return rc ;

   error:
      if ( NULL != options )
      {
         INT32 tmpRC = options->_logger.clear() ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDERROR, "Failed to clear rename info, rc: %d", tmpRC ) ;
         }
         if ( 0 != options->_blockOpID )
         {
            const utilRecycleItem &item = options->_recycleItem ;
            SDB_ASSERT( options->_recycleItem.isValid(), "should be valid" ) ;
            const CHAR *originName = item.getOriginName() ;
            const CHAR *recycleName = item.getRecycleName() ;
            _unregBlockCS( originName, recycleName, options->_blockOpID ) ;
            options->_blockOpID = 0 ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__RECYDROPCS, "_clsRecycleBinManager::_recycleDropCS" )
   INT32 _clsRecycleBinManager::_recycleDropCS( dmsStorageUnit *su,
                                                const CHAR *csName,
                                                const utilRecycleItem &item,
                                                pmdEDUCB *cb,
                                                BOOLEAN needLogRename )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__RECYDROPCS ) ;

      const CHAR *newName = item.getRecycleName() ;
      utilRenameLogger logger ;

      // the storage unit is now in deleting list (temp list), need
      // to be restored to do the rename
      rc = _dmsCB->restoreCollectionSpace( csName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to restore collection space "
                   "[%s] from deleting, rc: %d", csName, rc ) ;

      /// log to .SEQUOIADB_RENAME_INFO
      if ( needLogRename )
      {
         utilRenameLog aLog( csName, newName ) ;

         rc = logger.init() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init rename logger, "
                      "rc: %d", rc );

         rc = logger.log( aLog ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to log rename info to file, "
                      "rc: %d", rc ) ;
      }

      rc = _dmsCB->renameCollectionSpaceP2( csName, newName, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection space from "
                   "[%s] to [%s], rc: %d", csName, newName, rc ) ;

      if ( needLogRename )
      {
         rc = logger.clear() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to clear rename info, rc: %d", rc ) ;
      }

      rc = su->recycleCollectionSpace( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to recycle collection space [%s], "
                   "rc: %d", item.getRecycleName(), rc ) ;

      PD_LOG( PDEVENT, "Recycle drop collection space [%s] to [%s]",
              item.getOriginName(), item.getRecycleName() ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__RECYDROPCS, rc ) ;
      return rc ;

   error:
      {
         INT32 tmpRC = _dmsCB->moveCSToDeleting( csName ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDERROR, "Failed to remove collection space [%s], rc: %d",
                    csName, tmpRC ) ;
         }
         if ( needLogRename )
         {
            tmpRC = logger.clear() ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDERROR, "Failed to clear rename info, rc: %d", tmpRC ) ;
            }
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONDROPCS, "_clsRecycleBinManager::onDropCS" )
   INT32 _clsRecycleBinManager::onDropCS( SDB_EVENT_OCCUR_TYPE type,
                                          IDmsEventHolder *pEventHolder,
                                          IDmsSUCacheHolder *pCacheHolder,
                                          const dmsEventSUItem &suItem,
                                          dmsDropCSOptions *options,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONDROPCS ) ;

      if ( SDB_EVT_OCCUR_BEFORE == type )
      {
         if ( NULL != options &&
              options->_recycleItem.isValid() )
         {
            const utilRecycleItem &item = options->_recycleItem ;
            dmsStorageUnit *su = NULL ;
            dmsEventHolder *holder =
                              dynamic_cast<dmsEventHolder *>( pEventHolder ) ;
            PD_CHECK( NULL != holder, SDB_SYS, error, PDERROR,
                      "Failed to recycle collection [%s], failed to get "
                      "dms event holder from event holder",
                      item.getOriginName() ) ;

            su = holder->getSU() ;
            PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                      "Failed to recycle collection [%s], failed to get "
                      "storage unit from event holder",
                      item.getOriginName() ) ;

            if ( options->_needSaveItem && NULL != dpsCB )
            {
               rc = saveItem( item, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to save recycle item "
                            "[origin %s, recycle %s], rc: %d",
                            item.getOriginName(),
                            item.getRecycleName(), rc ) ;
            }

            rc = _recycleDropCS( su, suItem._pCSName, item, cb,
                                 !options->_logger.isOpened() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recycle drop collection "
                         "space [origin %s, recycle %s], rc: %d",
                         item.getOriginName(),
                         item.getRecycleName(), rc ) ;
         }
      }
      else
      {
         if ( ( NULL == options ) ||
              ( !( options->_recycleItem.isValid() ) ) )
         {
            // drop collection space without recycle bin,
            // drop all recycle items in collection space
            if ( UTIL_UNIQUEID_NULL != suItem._csUniqueID && NULL != dpsCB )
            {
               deleteItemsInCS( suItem._csUniqueID, cb ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONDROPCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_ONCLEANDROPCS, "_clsRecycleBinManager::onCleanDropCS" )
   INT32 _clsRecycleBinManager::onCleanDropCS( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventSUItem &suItem,
                                               dmsDropCSOptions *options,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_ONCLEANDROPCS ) ;

      if ( NULL != options &&
           options->_recycleItem.isValid() )
      {
         UINT64 blockID = options->_blockOpID ;
         UINT64 taskID = options->_localTaskID ;
         INT32 tmpRC = SDB_OK ;

         const utilRecycleItem &item = options->_recycleItem ;
         const CHAR *originName = item.getOriginName() ;
         const CHAR *recycleName = item.getRecycleName() ;

         if ( 0 != taskID )
         {
            _cleanLocalTask( taskID, blockID, cb ) ;
         }
         if ( 0 != blockID )
         {
            _unregBlockCS( originName, recycleName, blockID ) ;
         }
         options->_localTaskID = 0 ;
         options->_blockOpID = 0 ;

         /// remove .SEQUOIADB_RENAME_INFO
         tmpRC = options->_logger.clear() ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to clear rename info, rc: %d", tmpRC ) ;
         }
      }

      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_ONCLEANDROPCS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPITEM_DONE, "_clsRecycleBinManager::_dropItem" )
   INT32 _clsRecycleBinManager::_dropItem( const utilRecycleItem &item,
                                           pmdEDUCB *cb,
                                           INT16 w,
                                           BOOLEAN ignoreNotExists,
                                           BOOLEAN &isDropped )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPITEM_DONE ) ;

      rc = dropItemWithCheck( item, cb, TRUE, isDropped ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                   "recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPITEM_DONE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__CREATEBGJOB, "_clsRecycleBinManager::_createBGJob" )
   INT32 _clsRecycleBinManager::_createBGJob( utilLightJob **pJob )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__CREATEBGJOB ) ;

      rtnDropRecycleBinBGJob *job = NULL ;

      SDB_ASSERT( NULL != pJob, "job pointer is invalid" ) ;

      job = SDB_OSS_NEW clsDropRecycleBinBGJob( this ) ;
      PD_CHECK( NULL != job, SDB_OOM, error, PDERROR, "Failed to allocate BG "
                "job, rc: %d", rc ) ;

      *pJob = (utilLightJob *)job ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__CREATEBGJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPITEMIMPL, "_clsRecycleBinManager::_dropItemImpl" )
   INT32 _clsRecycleBinManager::_dropItemImpl( const utilRecycleItem &item,
                                               pmdEDUCB *cb,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPITEMIMPL ) ;

      if ( UTIL_RECYCLE_CS == item.getType() )
      {
         rc = _dropCSItem( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s] for collection space, rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == item.getType() &&
                item.isMainCL() )
      {
         rc = _dropMainCLItem( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s] for main collection, rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }
      else if ( UTIL_RECYCLE_CL == item.getType() )
      {
         rc = _dropCLItem( item, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item "
                      "[origin %s, recycle %s] for collection, rc: %d",
                      item.getOriginName(), item.getRecycleName(), rc ) ;
      }
      else
      {
         SDB_ASSERT( FALSE, "invalid recycle type" ) ;
         PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to drop item, "
                   "invalid recycle type [%d]", item.getType() ) ;
      }

      PD_LOG( PDEVENT, "Drop recycle item [origin %s, recycle %s], "
              "type [%s]", item.getOriginName(), item.getRecycleName(),
              utilGetRecycleTypeName( item.getType() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPITEMIMPL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPCSITEM, "_clsRecycleBinManager::_dropCSItem" )
   INT32 _clsRecycleBinManager::_dropCSItem( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPCSITEM ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == item.getType(),
                  "should be recycle item for collection space" ) ;

      const CHAR *recycleName = item.getRecycleName() ;
      const CHAR *originName = item.getOriginName() ;

      rc = rtnDropCollectionSpaceCommand( recycleName, cb, _dmsCB, _dpsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // ignore not exist error
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to drop collection space for "
                   "recycle item [%s], rc: %d", recycleName, rc ) ;

      // remove all recycle items inside this collection space
      // including all recycled collections and the collection space itself
      rc = _deleteItemsInCS( (utilCSUniqueID)( item.getOriginID() ),
                             TRUE, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle items in "
                   "recycled collection space item [origin %s, recycle %s], "
                   "rc: %d", originName, recycleName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPCSITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPMAINCLITEM, "_clsRecycleBinManager::_dropMainCLItem" )
   INT32 _clsRecycleBinManager::_dropMainCLItem( const utilRecycleItem &item,
                                                 pmdEDUCB *cb,
                                                 INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPMAINCLITEM ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType() && item.isMainCL(),
                  "should be recycle item for main collection" ) ;

      UTIL_RECY_ITEM_LIST itemList ;

      // get sub-items
      rc = getSubItems( item, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get sub-collection recycle "
                   "items for main-collection item [origin %s, recycle %s], "
                   "rc: %d", item.getOriginName(), item.getRecycleName(),
                   rc ) ;

      // drop each sub-items
      for ( UTIL_RECY_ITEM_LIST_IT iter = itemList.begin() ;
            iter != itemList.end() ;
            ++ iter )
      {
         utilRecycleItem &subItem = *iter ;
         rc = _dropCLItem( subItem, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop recycle item [origin %s, "
                      "recycle %s], rc: %d", subItem.getOriginName(),
                      subItem.getRecycleName(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPMAINCLITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DROPCLITEM, "_clsRecycleBinManager::_dropCLItem" )
   INT32 _clsRecycleBinManager::_dropCLItem( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DROPCLITEM ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType(),
                  "should be recycle item for collection" ) ;

      BOOLEAN writable = FALSE ;
      const CHAR *recycleName = item.getRecycleName() ;
      const CHAR *originName = item.getOriginName() ;
      utilCSUniqueID csUniqueID =
            utilGetCSUniqueID( (utilCLUniqueID)( item.getOriginID() ) ) ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      BOOLEAN itemDeleted = FALSE ;

      rc = _dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = _dmsCB->idToSUAndLock( csUniqueID, suID, &su ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space, rc: %d",
                   rc ) ;

      if ( NULL != su )
      {
         INT32 tmpRC = SDB_OK ;

         CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]  = { 0 } ;

         ossStrncpy( szSpace, su->CSName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;
         szSpace[ DMS_COLLECTION_SPACE_NAME_SZ ] = '\0' ;

         rc = su->data()->dropCollection( recycleName, cb, _dpsCB ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            // ignore not exist error
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop collection [%s], rc: %d",
                      recycleName, rc ) ;

         _dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_SUID ;
         su = NULL ;

         // try drop empty collection space
         tmpRC = _dmsCB->dropEmptyCollectionSpace( szSpace, cb, _dpsCB ) ;
         if ( SDB_OK == tmpRC &&
              0 == ossStrncmp( szSpace,
                               UTIL_RECYCLE_PREFIX,
                               UTIL_RECYCLE_PREFIX_SZ ) )
         {
            // if dropped by me, safe to remove recycle items inside this
            // collection space
            tmpRC = _deleteItemsInCS( (utilCSUniqueID)( item.getOriginID() ),
                                      TRUE, cb, w ) ;
            if ( SDB_OK == tmpRC )
            {
               itemDeleted = TRUE ;
            }
            else
            {
               // don't go to error, will try to delete the collection item
               PD_LOG( PDWARNING, "Failed to delete recycle item "
                       "[origin %s, recycle %s], rc: %d", originName,
                       recycleName, tmpRC ) ;
            }
         }
      }

      if ( !itemDeleted )
      {
         rc = _deleteItem( item, cb, w, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle item "
                      "[origin %s, recycle %s], rc: %d", originName,
                      recycleName, rc ) ;
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         _dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         _dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DROPCLITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR__DELITEMSINCS, "_clsRecycleBinManager::_deleteItemsInCS" )
   INT32 _clsRecycleBinManager::_deleteItemsInCS( utilCSUniqueID csUniqueID,
                                                  BOOLEAN includeSelf,
                                                  pmdEDUCB *cb,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR__DELITEMSINCS ) ;

      BSONObj matcher ;
      UINT64 deletedCount = 0 ;

      if ( includeSelf )
      {
         try
         {
            BSONObjBuilder builder ;
            BSONArrayBuilder orBuilder( builder.subarrayStart( "$or" ) ) ;

            BSONObjBuilder csBuilder( orBuilder.subobjStart() ) ;
            csBuilder.append( FIELD_NAME_ORIGIN_ID, (INT32)csUniqueID ) ;
            csBuilder.doneFast() ;

            BSONObjBuilder clBuilder( orBuilder.subobjStart() ) ;
            rc = utilGetCSBounds( FIELD_NAME_ORIGIN_ID, csUniqueID, clBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled "
                         "collections with collection space unique ID [%u], "
                         "rc: %d", csUniqueID, rc ) ;
            clBuilder.doneFast() ;

            orBuilder.doneFast() ;
            matcher = builder.obj() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }
      else
      {
         rc = utilGetRecyCLsInCSBounds( FIELD_NAME_ORIGIN_ID,
                                        csUniqueID,
                                        matcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get bounds of recycled "
                      "collections with collection space unique ID [%u], "
                      "rc: %d", csUniqueID, rc ) ;
      }

      rc = _deleteItems( matcher, cb, w, deletedCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to delete recycle items in collection "
                   "space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR__DELITEMSINCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_GETSUBITEMS, "_clsRecycleBinManager::getSubItems" )
   INT32 _clsRecycleBinManager::getSubItems( const utilRecycleItem &item,
                                             pmdEDUCB *cb,
                                             UTIL_RECY_ITEM_LIST &itemList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_GETSUBITEMS ) ;

      BSONObj matcher, dummy ;
      utilRecycleID recycleID = item.getRecycleID() ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_ID << (INT64)( recycleID ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _BASE::_getItems( matcher, dummy, _hintRecyID, -1, cb, itemList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle items with recycle "
                   "ID [%llu], rc: %d", recycleID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_GETSUBITEMS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_SAVEITEM, "_clsRecycleBinManager::saveItem" )
   INT32 _clsRecycleBinManager::saveItem( const utilRecycleItem &item,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_SAVEITEM ) ;

      BSONObj itemObject ;

      rc = item.toBSON( itemObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle item "
                   "[origin %s, recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

      rc = _saveItemObject( itemObject, cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save object for recycle item "
                   "[origin %s, recycle %s], rc: %d", item.getOriginName(),
                   item.getRecycleName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_SAVEITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINMGR_GETITEM, "_clsRecycleBinManager::getItem" )
   INT32 _clsRecycleBinManager::getItem( const CHAR *recycleName,
                                         pmdEDUCB *cb,
                                         utilRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINMGR_GETITEM ) ;

      SDB_ASSERT( NULL != recycleName, "recycle name is invalid" ) ;

      BSONObj matcher, dummy ;
      INT64 contextID = -1 ;
      rtnContextBuf buffObj ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_NAME << recycleName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _getItems( matcher, dummy, dummy, 1, cb, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query recycle item [%s], "
                   "rc: %d", recycleName, rc ) ;

      rc = rtnGetMore( contextID, 1, buffObj, cb, _rtnCB ) ;
      if ( SDB_DMS_EOC == rc )
      {
         PD_LOG( PDINFO, "Failed to get recycle item [%s], it does not exist",
                 recycleName ) ;
         rc = SDB_RECYCLE_ITEM_NOTEXIST ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item [%s], rc: %d",
                   recycleName, rc ) ;

      try
      {
         BSONObj object( buffObj.data() ) ;

         rc = item.fromBSON( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item from BSON, "
                      "rc: %d", rc ) ;

      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get recycle item, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         _rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSRECYBINMGR_GETITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _clsDropRecycleBinBGJob implement
    */
   _clsDropRecycleBinBGJob::_clsDropRecycleBinBGJob(
                                          rtnRecycleBinManager *recycleBinMgr )
   : _rtnDropRecycleBinBGJob( recycleBinMgr )
   {
      SDB_ASSERT( NULL != _recycleBinMgr,
                  "recycle bin manager is invalid" ) ;
   }

   _clsDropRecycleBinBGJob::~_clsDropRecycleBinBGJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYBINBGJOB_DOIT, "_clsDropRecycleBinBGJob::doit" )
   INT32 _clsDropRecycleBinBGJob::doit( IExecutor *pExe,
                                        UTIL_LJOB_DO_RESULT &result,
                                        UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSRECYBINBGJOB_DOIT ) ;

      if ( PMD_IS_DB_DOWN() )
      {
         PD_LOG( PDDEBUG, "DB is down, stop to drop expired items" ) ;
         result = UTIL_LJOB_DO_FINISH ;
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      else if ( !_recycleBinMgr->isConfValid() )
      {
         shardCB *shardCB = sdbGetShardCB() ;

         // update DC from remote
         rc = shardCB->updateDCBaseInfo() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to update DC info from CATALOG, "
                      "rc: %d", rc ) ;
            result = UTIL_LJOB_DO_CONT ;
            sleepTime = RTN_RECYCLE_RETRY_INTERVAL ;
            goto error ;
         }

         _recycleBinMgr->setConf(
               shardCB->getDCMgr()->getDCBaseInfo()->getRecycleBinConf() ) ;
      }

      rc = _rtnDropRecycleBinBGJob::doit( pExe, result, sleepTime ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSRECYBINBGJOB_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
