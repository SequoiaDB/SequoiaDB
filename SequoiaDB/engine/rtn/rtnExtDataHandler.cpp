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

   Source File Name = rtnExtDataHandler.cpp

   Descriptive Name = External data process handler for rtn.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "rtnExtDataHandler.hpp"
#include "../bson/lib/md5.hpp"

namespace engine
{
   _rtnExtDataHandler::_rtnExtDataHandler( rtnExtDataProcessorMgr *edpMgr )
   : _enabled( TRUE ),
     _refCount( 0 ),
     _edpMgr( edpMgr )
   {
   }

   _rtnExtDataHandler::~_rtnExtDataHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ENABLE, "_rtnExtDataHandler::enable" )
   void _rtnExtDataHandler::enable()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ENABLE ) ;
      if ( !_enabled )
      {
         _enabled = TRUE ;
         PD_LOG( PDEVENT, "External data handler enabled" ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAHANDLER_ENABLE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_DISABLE, "_rtnExtDataHandler::disable" )
   INT32 _rtnExtDataHandler::disable( INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_DISABLE ) ;
      BOOLEAN currentStat = _enabled ;

      // Disable and wait for all current users to finish.
      _enabled = FALSE ;

      while ( _refCount.fetch() > 0 )
      {
         ossSleep( OSS_ONE_SEC ) ;
         if ( timeout >= OSS_ONE_SEC )
         {
            timeout -= OSS_ONE_SEC ;
         }
         else
         {
            rc = SDB_TIMEOUT ;
            PD_LOG( PDERROR, "Failed to disable external handler "
                             "in %d milliseconds", timeout ) ;
            // If not all users are gone in specified time, restore the status.
            _enabled = currentStat ;
            goto error ;
         }
      }

      PD_LOG( PDEVENT, "External data handler disabled" ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_DISABLE, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_PREPARE, "_rtnExtDataHandler::prepare" )
   INT32 _rtnExtDataHandler::prepare( DMS_EXTOPR_TYPE type,
                                      const CHAR *csName,
                                      const CHAR *clName,
                                      const CHAR *idxName,
                                      const BSONObj *object,
                                      const BSONObj *objNew,
                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_PREPARE ) ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;
      rtnExtContextBase *context = NULL ;

      if ( !( DMS_EXTOPR_TYPE_INSERT == type ||
              DMS_EXTOPR_TYPE_DELETE == type ||
              DMS_EXTOPR_TYPE_UPDATE == type ) )
      {
         goto done ;
      }

      // During rebuilding or full sync, the original collection and the capped
      // collection are processed seperately.
      if ( SDB_DB_REBUILDING == dbStatus || SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      rc = _getContext( type, context, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare external operation context "
                                "failed[ %d ]", rc ) ;

      // If the context is not in normal state, just ignore.
      if ( EXT_CTX_STAT_NORMAL == context->getStat() )
      {
         ((rtnExtDataOprCtx *)context)->setOrigRecord( *object ) ;
         if ( DMS_EXTOPR_TYPE_UPDATE == type )
         {
            ((rtnExtDataOprCtx *)context)->setNewRecord( *objNew ) ;
         }

         rc = _prepare( (rtnExtDataOprCtx *)context, csName, clName, idxName ) ;
         PD_RC_CHECK( rc, PDERROR,
                     "External data process checking failed[ %d ]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX, "_rtnExtDataHandler::onOpenTextIdx" )
   INT32 _rtnExtDataHandler::onOpenTextIdx( const CHAR *csName,
                                            const CHAR *clName,
                                            ixmIndexCB &indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX ) ;
      rtnExtDataProcessor *processor = NULL ;

      // For compatibility with version 3.0.
      // From version 3.0.1, external data name(apped cs name) is stored in
      // indexCB. Version 3.0 didn't do that. And the external name generation
      // are different. So during upgrade, for text indices created with sdb
      // version 3.0. generate the name with the old rule and append it to the
      // indexCB.
      if ( !_hasExtName( indexCB ) )
      {
         rc = _extendIndexDef( csName, clName, indexCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Extend index definition failed[ %d ]",
                      rc ) ;
      }

      rc = _edpMgr->createProcessor( csName, clName, indexCB.getName(),
                                     indexCB.getExtDataName(),
                                     indexCB.keyPattern(), processor, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Create external processor failed[ %d ]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONOPENTEXTIDX, rc ) ;
      return rc ;
   error:
      if ( processor )
      {
         _edpMgr->destroyProcessor( processor ) ;
         processor = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDELCS, "_rtnExtDataHandler::onDelCS" )
   INT32 _rtnExtDataHandler::onDelCS( const CHAR *csName, pmdEDUCB *cb,
                                      BOOLEAN removeFiles, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDELCS ) ;
      rtnExtDropCSCtx *context = NULL ;

      context = (rtnExtDropCSCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DROPCS,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      rc = context->open( _edpMgr, csName, cb, removeFiles, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Open external delete cs context failed[ %d ]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELCS, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDELCL, "_rtnExtDataHandler::onDelCL" )
   INT32 _rtnExtDataHandler::onDelCL( const CHAR *csName, const CHAR *clName,
                                      pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDELCL ) ;
      rtnExtDropCLCtx *context = NULL ;

      context = (rtnExtDropCLCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DROPCL,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      rc = context->open( _edpMgr, csName, clName, cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Open external delete cs context failed[ %d ]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELCL, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONCRTTEXTIDX, "_rtnExtDataHandler::onCrtTextIdx" )
   INT32 _rtnExtDataHandler::onCrtTextIdx( dmsMBContext *context,
                                           const CHAR *csName,
                                           ixmIndexCB &indexCB,
                                           pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONCRTTEXTIDX ) ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      rtnExtCrtIdxCtx *crtContext = NULL ;
      CHAR extName[ DMS_MAX_EXT_NAME_SIZE + 1 ] = { 0 };

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller should hold mb exclusive lock" ) ;
         goto error ;
      }

      if ( _edpMgr->number() >= RTN_EXT_PROCESSOR_MAX_NUM )
      {
         rc = SDB_OSS_UP_TO_LIMIT ;
         PD_LOG( PDERROR, "Max number of text indices[%d] has been created",
                 RTN_EXT_PROCESSOR_MAX_NUM ) ;
         goto error ;
      }

      clUniqueID = context->mb()->_clUniqueID ;
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Can not create text index on collection whose "
                          "unique id is invalid" ) ;
         goto error ;
      }

      rc = _getExtDataName( clUniqueID, indexCB.getName(), extName,
                            DMS_MAX_EXT_NAME_SIZE + 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external data name failed[%d]", rc ) ;

      try
      {
         // Extend the index definition in indexCB, adding the ExtDataName.
         BSONObj indexExtObj = BSON( FIELD_NAME_EXT_DATA_NAME << extName );
         indexCB.extendDef( indexExtObj.firstElement() );
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception ocurred: %s", e.what() ) ;
         goto error ;
      }

      rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_CRTIDX, cb,
                                      (rtnExtContextBase **)&crtContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Create new context failed[%d]", rc ) ;

      rc = crtContext->open( _edpMgr, context, csName, indexCB, cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context for creating index failed[%d]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONCRTTEXTIDX, rc ) ;
      return rc ;
   error:
      if ( crtContext )
      {
         _contextMgr.delContext( crtContext->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX, "_rtnExtDataHandler::onDropTextIdx" )
   INT32 _rtnExtDataHandler::onDropTextIdx( const CHAR *extName,
                                            pmdEDUCB *cb,
                                            SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX ) ;
      rtnExtDropIdxCtx *context = NULL ;

      context = (rtnExtDropIdxCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_DROPIDX,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      // If the current context is not DROPIDX, the current operation may be
      // dropping cs or cl. Nothing should be done in that cases.
      if ( DMS_EXTOPR_TYPE_DROPIDX == context->getType() )
      {
         rc = context->open( _edpMgr, extName, cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Open external drop context failed[ %d ]",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDROPTEXTIDX, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX, "_rtnExtDataHandler::onRebuildTextIdx" )
   INT32 _rtnExtDataHandler::onRebuildTextIdx( const CHAR *csName,
                                               const CHAR *clName,
                                               const CHAR *idxName,
                                               const CHAR *extName,
                                               const BSONObj &keyDef,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX ) ;
      rtnExtRebuildIdxCtx *context = NULL ;

      SDB_ASSERT( cb, "cb should not be NULL" ) ;

      context = (rtnExtRebuildIdxCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_REBUILDIDX,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
      }

      rc = context->open( _edpMgr, csName, clName, idxName,
                          extName, keyDef, cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Open external rebuild context failed[ %d ]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONREBUILDTEXTIDX, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONINSERT, "_rtnExtDataHandler::onInsert" )
   INT32 _rtnExtDataHandler::onInsert( const CHAR *extName,
                                       const BSONObj &object,
                                       pmdEDUCB* cb,
                                       SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONINSERT ) ;
      rtnExtContextBase *context = NULL ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;

      context = _contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         if ( SDB_DB_FULLSYNC == dbStatus || SDB_DB_REBUILDING == dbStatus )
         {
            goto done ;
         }
         SDB_ASSERT( FALSE, "context not found" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Context for delete not found" ) ;
         goto error ;
      }

      if ( EXT_CTX_STAT_ABORTING == context->getStat() )
      {
         goto done ;
      }

      SDB_ASSERT( DMS_EXTOPR_TYPE_INSERT == context->getType(),
                  "Type not match") ;

      rc = static_cast<rtnExtDataOprCtx*>(context)->open( _edpMgr, extName,
                                                          cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Do external operation for insert failed[%d]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONINSERT, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         context->setStat( EXT_CTX_STAT_ABORTING ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONDELETE, "_rtnExtDataHandler::onDelete" )
   INT32 _rtnExtDataHandler::onDelete( const CHAR *extName,
                                       const BSONObj &object,
                                       pmdEDUCB* cb,
                                       SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONDELETE ) ;
      rtnExtContextBase *context = NULL ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;

      context = _contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         if ( SDB_DB_FULLSYNC == dbStatus || SDB_DB_REBUILDING == dbStatus )
         {
            goto done ;
         }
         SDB_ASSERT( FALSE, "context not found" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Context for delete not found" ) ;
         goto error ;
      }

      if ( EXT_CTX_STAT_ABORTING == context->getStat() )
      {
         goto done ;
      }

      SDB_ASSERT( DMS_EXTOPR_TYPE_DELETE == context->getType(),
                  "Type not match") ;

      rc = static_cast<rtnExtDataOprCtx*>(context)->open( _edpMgr, extName,
                                                          cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Do external operation for delete failed[%d]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONDELETE, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         context->setStat( EXT_CTX_STAT_ABORTING ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONUPDATE, "_rtnExtDataHandler::onUpdate" )
   INT32 _rtnExtDataHandler::onUpdate( const CHAR *extName,
                                       const BSONObj &orignalObj,
                                       const BSONObj &newObj,
                                       pmdEDUCB* cb,
                                       BOOLEAN isRollback,
                                       SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONUPDATE ) ;
      rtnExtContextBase *context = NULL ;
      SDB_DB_STATUS dbStatus = pmdGetKRCB()->getDBStatus() ;

      context = _contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         // During full sync or rebuilding, the capped collection will be
         // processed seperately from the original collection. So nothing will
         // be done in callback functions.
         // When rolling back, the original update may have failed. The context
         // would have been freed in that case. So we also do nothing.
         if ( SDB_DB_FULLSYNC == dbStatus || SDB_DB_REBUILDING == dbStatus
              || isRollback )
         {
            goto done ;
         }
         SDB_ASSERT( FALSE, "context not found" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Context for delete not found" ) ;
         goto error ;
      }

      if ( isRollback )
      {
         context->setStat( EXT_CTX_STAT_ABORTING ) ;
      }

      if ( EXT_CTX_STAT_ABORTING == context->getStat() )
      {
         goto done ;
      }

      SDB_ASSERT( DMS_EXTOPR_TYPE_UPDATE == context->getType(),
                  "Type not match") ;

      rc = static_cast<rtnExtDataOprCtx*>(context)->open( _edpMgr, extName,
                                                          cb, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Do external operation for update failed[%d]",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONUPDATE, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         context->setStat( EXT_CTX_STAT_ABORTING ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL, "_rtnExtDataHandler::onTruncateCL" )
   INT32 _rtnExtDataHandler::onTruncateCL( const CHAR *csName,
                                           const CHAR *clName,
                                           pmdEDUCB *cb,
                                           BOOLEAN needChangeCLID,
                                           SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL ) ;
      rtnExtTruncateCtx *context = NULL ;

      context = (rtnExtTruncateCtx *)_contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( DMS_EXTOPR_TYPE_TRUNCATE,
                                         cb, (rtnExtContextBase**)&context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
         rc = _hold() ;
         PD_RC_CHECK( rc, PDERROR, "Take hold of external handler failed[ %d ]",
                      rc ) ;
      }

      rc = context->open( _edpMgr, csName, clName, cb, needChangeCLID, dpscb ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context for truncate failed[ %d ]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONTRUNCATECL, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
         _release() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONRENAMECS, "_rtnExtDataHandler::onRenameCS" )
   INT32 _rtnExtDataHandler::onRenameCS( const CHAR *oldCSName,
                                         const CHAR *newCSName,
                                         pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONRENAMECS ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      SDB_ASSERT( oldCSName, "Old CS name is null" ) ;
      SDB_ASSERT( newCSName, "New CS name is null" ) ;

      if ( 0 == ossStrcmp( oldCSName, newCSName ) )
      {
         goto done ;
      }

      rc = _edpMgr->renameCS( oldCSName, newCSName ) ;
      PD_RC_CHECK( rc, PDERROR, "External data processor manager rename cs "
                                "failed[ %d ]", rc ) ;

      rtnCB->incTextIdxVersion() ;
   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONRENAMECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ONRENAMECL, "_rtnExtDataHandler::onRenameCL" )
   INT32 _rtnExtDataHandler::onRenameCL( const CHAR *csName,
                                         const CHAR *oldCLName,
                                         const CHAR *newCLName,
                                         pmdEDUCB *cb, SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ONRENAMECL ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      SDB_ASSERT( csName, "CS name is null" ) ;
      SDB_ASSERT( oldCLName, "Old CL name is null" ) ;
      SDB_ASSERT( newCLName, "New CL name is null" ) ;

      if ( 0 == ossStrcmp( oldCLName, newCLName ) )
      {
         goto done ;
      }

      rc = _edpMgr->renameCL( csName, oldCLName, newCLName ) ;
      PD_RC_CHECK( rc, PDERROR, "External data processor manager rename cl "
                                "failed[ %d ]", rc ) ;
      rtnCB->incTextIdxVersion() ;
   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ONRENAMECL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_DONE, "_rtnExtDataHandler::done" )
   INT32 _rtnExtDataHandler::done( DMS_EXTOPR_TYPE type, pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_DONE ) ;
      rtnExtContextBase *context = _contextMgr.findContext( cb->getTID() ) ;
      BOOLEAN ownContext = FALSE ;
      if ( !context )
      {
         goto done ;
      }

      ownContext = ( context->getType() == type ) ;
      if ( !ownContext )
      {
         goto done ;
      }

      // In some cases, done will be called in aborting process. For example,
      // when deleting records, even deleting index fails, the records will be
      // deleted anyway. So this done will be called, as the error code of ixm
      // will be ignored.
      if ( EXT_CTX_STAT_ABORTING == context->getStat() )
      {
         rc = context->abort( cb, dpscb ) ;
      }
      else
      {
         rc = context->done( cb, dpscb ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Final step of current operation failed[ %d ]",
                   rc ) ;

   done:
      if ( context && ownContext )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
         if ( _holdingType( type ) )
         {
            SDB_ASSERT( _refCount.fetch() > 0, "External handler reference count "
                                               "is not greater than 0" ) ;
            _release() ;
         }
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ABORTOPERATION, "_rtnExtDataHandler::abortOperation" )
   INT32 _rtnExtDataHandler::abortOperation( DMS_EXTOPR_TYPE type,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ABORTOPERATION ) ;
      rtnExtContextBase *context = _contextMgr.findContext( cb->getTID() ) ;
      BOOLEAN ownContext = FALSE ;
      if ( !context )
      {
         goto done ;
      }

      ownContext = ( context->getType() == type ) ;
      if ( !ownContext )
      {
         goto done ;
      }

      rc = context->abort( cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Final step of current operation failed[ %d ]",
                   rc) ;

   done:
      if ( context && ownContext )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
         if ( _holdingType( type ) )
         {
            SDB_ASSERT( _refCount.fetch() > 0, "External handler reference count "
                                               "is not greater than 0" ) ;
            _release() ;
         }
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ABORTOPERATION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_ACQUIRELOCK, "_rtnExtDataHandler::acquireLock" )
   INT32 _rtnExtDataHandler::acquireLock( const CHAR *extName, INT32 lockType,
                                          LOCK_HANDLE &handle )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_ACQUIRELOCK ) ;
      rtnExtDataProcessor *processor = NULL ;

      SDB_ASSERT( extName, "External name is NULL" ) ;

      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         goto done ;
      }

      rc = _edpMgr->getProcessorByExtName( extName, lockType, processor ) ;
      PD_RC_CHECK( rc, PDERROR, "Get external processor for[%s] failed[%d]",
                   extName, rc ) ;
      if ( processor )
      {
         handle = processor ;
         ossScopedLock lock( &_latch ) ;
         _lockInfo[ processor ] = lockType ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get processor for[%s] failed", extName ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER_ACQUIRELOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER_RELEASELOCK, "_rtnExtDataHandler::releaseLock" )
   void _rtnExtDataHandler::releaseLock( LOCK_HANDLE handle )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER_RELEASELOCK ) ;
      rtnExtDataProcessor *processor = (rtnExtDataProcessor *)handle ;
      if ( processor )
      {
         ossScopedLock lock( &_latch ) ;
         LOCK_INFO_MAP_ITR itr = _lockInfo.find( processor ) ;
         if ( itr != _lockInfo.end() )
         {
            vector<rtnExtDataProcessor *> processors ;
            processors.push_back( itr->first ) ;
            _edpMgr->unlockProcessors( processors, itr->second ) ;
            _lockInfo.erase( itr ) ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAHANDLER_RELEASELOCK ) ;
   }

   BOOLEAN _rtnExtDataHandler::_hasExtName( const ixmIndexCB &indexCB )
   {
      return ( ossStrlen( indexCB.getExtDataName() ) > 0 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER__EXTENDINDEXDEF, "_rtnExtDataHandler::_extendIndexDef" )
   INT32 _rtnExtDataHandler::_extendIndexDef( const CHAR *csName,
                                              const CHAR *clName,
                                              ixmIndexCB &indexCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER__EXTENDINDEXDEF ) ;
      CHAR extName[ DMS_MAX_EXT_NAME_SIZE + 1 ] = { 0 };

      rc = _getExtDataNameV1( csName, clName, indexCB.getName(), extName,
                              DMS_MAX_EXT_NAME_SIZE + 1 )  ;
      PD_RC_CHECK( rc, PDERROR, "Get external data name failed[%d]", rc ) ;

      try
      {
         BSONObj extendObj = BSON( FIELD_NAME_EXT_DATA_NAME << extName ) ;
         indexCB.extendDef( extendObj.firstElement() ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER__EXTENDINDEXDEF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER__PREPARE, "_rtnExtDataHandler::_prepare" )
   INT32 _rtnExtDataHandler::_prepare( rtnExtDataOprCtx *context,
                                       const CHAR *csName,
                                       const CHAR *clName,
                                       const CHAR *idxName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER__PREPARE ) ;

      std::vector<rtnExtDataProcessor *> processors ;

      if ( idxName )
      {
         rtnExtDataProcessor *processor = NULL ;
         rc = _edpMgr->getProcessorByIdx( csName, clName, idxName, SHARED,
                                          processor ) ;
         PD_RC_CHECK( rc, PDERROR, "Get external processor failed[ %d ]", rc ) ;
         processors.push_back( processor ) ;
      }
      else if ( clName )
      {
         rc = _edpMgr->getProcessorsByCL( csName, clName, SHARED, processors ) ;
         PD_RC_CHECK( rc, PDERROR, "Get external processors failed[ %d ]",
                      rc ) ;
         SDB_ASSERT( processors.size() <= 1, "Processor number is wrong" ) ;
         if ( 0 == processors.size() )
         {
            goto done ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Collection is empty for check" ) ;
         goto error ;
      }

      for ( vector<rtnExtDataProcessor *>::iterator itr = processors.begin();
            itr != processors.end(); ++itr )
      {
         rc = (*itr)->prepare( context->getType(), (rtnExtOprData *)context ) ;
         PD_RC_CHECK( rc, PDERROR, "Processor check failed[ %d ]", rc ) ;
      }

   done:
      if ( processors.size() > 0 )
      {
         _edpMgr->unlockProcessors( processors, SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER__PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER__PREPARECTX, "_rtnExtDataHandler::_getContext" )
   INT32 _rtnExtDataHandler::_getContext( DMS_EXTOPR_TYPE type,
                                          rtnExtContextBase *&ctx,
                                          pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER__PREPARECTX ) ;
      rtnExtContextBase *context = NULL ;
      BOOLEAN newContext = FALSE ;
      BOOLEAN hasHold = FALSE ;

      context = _contextMgr.findContext( cb->getTID() ) ;
      if ( !context )
      {
         rc = _contextMgr.createContext( type, cb, &context ) ;
         PD_RC_CHECK( rc, PDERROR, "Create new context failed[ %d ]", rc ) ;
         newContext = TRUE ;
         rc = _hold() ;
         PD_RC_CHECK( rc, PDERROR, "Take hold of external handler failed[ %d ]",
                      rc ) ;
         hasHold = TRUE ;
      }
      else if ( type != context->getType() &&
                ( EXT_CTX_STAT_ABORTING != context->getStat() ) )
      {
         context->setStat( EXT_CTX_STAT_ABORTING ) ;
      }

      ctx = context ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER__PREPARECTX, rc ) ;
      return rc ;
   error:
      if ( newContext )
      {
         _contextMgr.delContext( context->getID(), cb ) ;
         if ( hasHold )
         {
            SDB_ASSERT( _refCount.fetch() > 0, "External handler reference count is "
                           "not greater than 0" ) ;
            _release() ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER__GETEXTDATANAME, "_rtnExtDataHandler::_getExtDataName" )
   INT32 _rtnExtDataHandler::_getExtDataName( utilCLUniqueID clUniqID,
                                              const CHAR *idxName,
                                              CHAR *extName,
                                              UINT32 buffSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER__GETEXTDATANAME ) ;
      INT32 length = 0 ;

      SDB_ASSERT( idxName, "Index name is NULL") ;
      SDB_ASSERT( extName, "Buffer is empty" ) ;
      SDB_ASSERT( buffSize >= DMS_MAX_EXT_NAME_SIZE + 1, "buffer too small" ) ;

      length = ossSnprintf( extName, DMS_MAX_EXT_NAME_SIZE + 1,
                            SYS_PREFIX"_%llu_", clUniqID ) ;
      if ( length + ossStrlen( idxName ) > DMS_MAX_EXT_NAME_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Index name size[%u] too long for external data",
                 ossStrlen( idxName ) ) ;
         goto error ;
      }

      ossSnprintf( extName + length, DMS_MAX_EXT_NAME_SIZE + 1 - length,
                   idxName ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAHANDLER__GETEXTDATANAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAHANDLER__GETEXTDATANAMEV1, "_rtnExtDataHandler::_getExtDataNameV1" )
   INT32 _rtnExtDataHandler::_getExtDataNameV1( const CHAR *csName,
                                                const CHAR *clName,
                                                const CHAR *idxName,
                                                CHAR *extName,
                                                UINT32 buffSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAHANDLER__GETEXTDATANAMEV1 ) ;
      UINT32 hashVal = 0 ;
      string md5Val ;

      SDB_ASSERT( csName, "cs name is NULL") ;
      SDB_ASSERT( clName, "cl name is NULL") ;
      SDB_ASSERT( idxName, "Index name is NULL") ;
      SDB_ASSERT( extName, "Buffer is empty" ) ;
      SDB_ASSERT( buffSize >= DMS_MAX_EXT_NAME_SIZE + 1, "buffer too small" ) ;

      INT32 nameTotalLen = ossStrlen( csName ) + ossStrlen( clName )
                            + ossStrlen( idxName ) ;
      CHAR *names = (CHAR *)SDB_OSS_MALLOC( nameTotalLen + 1 ) ;
      if ( !names )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory failed. Size request: %d",
                 nameTotalLen + 1 ) ;
         goto error ;
      }

      ossSnprintf( names, nameTotalLen + 1, "%s%s%s",
                   csName, clName, idxName ) ;

      hashVal = ossHash( names ) ;
      md5Val = md5::md5simpledigest( (void *)names, nameTotalLen ) ;

      ossSnprintf( extName, buffSize, SYS_PREFIX"_%u%s", hashVal,
                   md5Val.substr(0, 4).c_str() ) ;
   done:
      SAFE_OSS_FREE( names ) ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAHANDLER__GETEXTDATANAMEV1 ) ;
      return rc ;
   error:
      goto done ;
   }

   rtnExtDataHandler* rtnGetExtDataHandler()
   {
      static rtnExtDataHandler s_edh( rtnGetExtDataProcessorMgr() ) ;
      return &s_edh ;
   }
}

