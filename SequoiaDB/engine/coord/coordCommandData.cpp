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

   Source File Name = coordCommandData.cpp

   Descriptive Name = Coord Commands for Data Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordCommandData.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "dms.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "rtnContextDump.hpp"
#include "catCommon.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordSequenceAgent.hpp"
#include "clsResourceContainer.hpp"
#include "coordDSChecker.hpp"
#include "coordCacheAssist.hpp"
#include "coordCommandWithLocation.hpp"
#include "coordCommandRecycleBin.hpp"
#include "coordUtil.hpp"

using namespace bson;

namespace engine
{

   /*
      _coordDataCMD2Phase implement
   */
   _coordDataCMD2Phase::_coordDataCMD2Phase()
   {
   }

   _coordDataCMD2Phase::~_coordDataCMD2Phase()
   {
   }

   INT32 _coordDataCMD2Phase::_generateDataMsg ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 const vector<BSONObj> &cataObjs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      pMsg->opCode = MSG_BS_QUERY_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   INT32 _coordDataCMD2Phase::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         coordCMDArguments *pArgs,
                                                         CHAR **ppMsgBuf,
                                                         INT32 *pBufSize )
   {
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   void _coordDataCMD2Phase::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                      INT32 bufSize,
                                                      pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA2PHASE_DOONCATA, "_coordDataCMD2Phase::_doOnCataGroup" )
   INT32 _coordDataCMD2Phase::_doOnCataGroup( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnContextCoord::sharePtr *ppContext,
                                              coordCMDArguments *pArgs,
                                              CoordGroupList *pGroupLst,
                                              vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DATA2PHASE_DOONCATA ) ;

      rtnContextCoord::sharePtr pContext ;
      coordCataSel cataSel ;

      if ( _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         rc = cataSel.bind( _pResource, pArgs->_targetName.c_str(),
                            cb, TRUE, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                    "failed on command[%s], rc: %d",
                    pArgs->_targetName.c_str(), getName(), rc ) ;
            goto error ;
         }
         _cataPtr = cataSel.getCataPtr() ;
      }

   retry:
      if ( _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         MsgOpQuery *pOpMsg = (MsgOpQuery *)pMsg ;
         pOpMsg->version = cataSel.getCataPtr()->getVersion() ;
      }

      rc = _coordCMD2Phase::_doOnCataGroup( pMsg, cb, &pContext, pArgs,
                                            pGroupLst, pReplyObjs ) ;
      if ( rc && _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         if ( checkRetryForCLOpr( rc, NULL, cataSel, pMsg,
                                  cb, rc, NULL, TRUE ) )
         {
            _groupSession.getGroupCtrl()->incRetry() ;
            goto retry ;
         }
      }
      if ( rc )
      {
         PD_LOG( PDERROR, "Do on catalog failed on command[%s, target:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         goto error ;
      }

   done :
      if ( pContext )
      {
         *ppContext = pContext ;
      }
      PD_TRACE_EXITRC ( COORD_DATA2PHASE_DOONCATA, rc ) ;
      return rc ;
   error :
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext.release() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA2PHASE_DOONDATA, "_coordDataCMD2Phase::_doOnDataGroup" )
   INT32 _coordDataCMD2Phase::_doOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord::sharePtr *ppContext,
                                               coordCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs,
                                               CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DATA2PHASE_DOONDATA ) ;

      // For DropCL/DropCS, this will guarantee data updates are
      // started before catalog updates
      if ( _flagDoOnCollection() )
      {
         rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                           _flagUpdateBeforeData(),
                           _flagUseGrpLstInCoord() ? NULL : &groupLst,
                           &(pArgs->_ignoreRCList), &sucGroupLst,
                           ppContext, pArgs->_pBuf ) ;
      }
      else
      {
         rc = executeOnDataGroup( pMsg, cb, groupLst, TRUE,
                                  &(pArgs->_ignoreRCList), &sucGroupLst,
                                  ppContext, pArgs->_pBuf ) ;
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Do on data group failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DATA2PHASE_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA2PHASE_DOAUDIT "_coordDataCMD2Phase::_doAudit" )
   INT32 _coordDataCMD2Phase::_doAudit ( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY ( COORD_DATA2PHASE_DOAUDIT ) ;

      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(),
                           _flagDoOnCollection() ? AUDIT_OBJ_CL : AUDIT_OBJ_CS,
                           pArgs->_targetName.c_str(), rc, "Option: %s",
                           pArgs->_boQuery.toString().c_str() ) ;
      }

      PD_TRACE_EXIT ( COORD_DATA2PHASE_DOAUDIT ) ;
      return SDB_OK ;
   }

   INT32 _coordDataCMD2Phase::_doOutput( rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      if( _flagDoOnCollection () && getCataPtr() && NULL != buf )
      {
         buf->setStartFrom( getCataPtr()->getVersion() ) ;
      }

      return rc ;
   }

   /*
      _coordDataCMD3Phase implement
   */
   _coordDataCMD3Phase::_coordDataCMD3Phase()
   {
   }

   _coordDataCMD3Phase::~_coordDataCMD3Phase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA3PHASE_DOONCATA2, "_coordDataCMD3Phase::_doOnCataGroupP2" )
   INT32 _coordDataCMD3Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord::sharePtr *ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &pGroupLst,
                                                 vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_DATA3PHASE_DOONCATA2 ) ;

      rtnContextBuf buffObj ;

      rc = _processContext( cb, ppContext, 1, buffObj ) ;

      try
      {
         while ( !buffObj.eof() )
         {
            BSONObj reply ;
            rc = buffObj.nextObj( reply ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get obj from obj buf, rc: %d",
                         rc ) ;
            cataObjs.push_back( reply.getOwned() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get reply object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_DATA3PHASE_DOONCATA2, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATA3PHASE_DOONDATA2, "_coordDataCMD3Phase::_doOnDataGroupP2" )
   INT32 _coordDataCMD3Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord::sharePtr *ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_DATA3PHASE_DOONDATA2 ) ;

      rtnContextBuf buffObj ;

      rc = _processContext( cb, ppContext, 1, buffObj ) ;

      PD_TRACE_EXITRC ( COORD_DATA3PHASE_DOONDATA2, rc ) ;

      return rc ;
   }

   /*
      _coordAlterCMDArgument implement
    */
   _coordAlterCMDArguments::_coordAlterCMDArguments ()
   : _coordCMDArguments(),
     _task( NULL )
   {
   }

   _coordAlterCMDArguments::~_coordAlterCMDArguments ()
   {
   }

   void _coordAlterCMDArguments::clear ()
   {
      _boQuery = BSONObj() ;
      _targetName.clear() ;
      _ignoreRCList.clear() ;
      _pBuf = NULL ;
      _task = NULL ;
   }

   INT32 _coordAlterCMDArguments::addPostTask( UINT64 postTask )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _postTasks.push_back( postTask ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add post task, occur exception: %s",
                 e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _coordAlterCMDArguments::addPostTaskObj( const BSONObj &taskObj )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _postTasksObj.push_back( taskObj ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add post task info, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordDataCMDAlter implement
    */
   _coordDataCMDAlter::_coordDataCMDAlter ()
   : _coordDataCMD3Phase()
   {
   }

   _coordDataCMDAlter::~_coordDataCMDAlter()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER_EXECUTE, "_coordDataCMDAlter::execute" )
   INT32 _coordDataCMDAlter::execute ( MsgHeader * pMsg,
                                       pmdEDUCB * cb,
                                       INT64 & contextID,
                                       rtnContextBuf * buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER_EXECUTE ) ;

      coordCMDArguments arguments ;
      arguments._pBuf = buf ;

      // Extract message
      rc = _extractMsg ( pMsg, &arguments ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract message for command[%s], "
                   "rc: %d", getName(), rc ) ;

      // Parse the alter tasks
      rc = _alterJob.initialize( NULL, _getObjectType(), arguments._boQuery ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize alter job, "
                   "rc: %d", rc ) ;

      arguments._targetName = _alterJob.getObjectName() ;

      if ( !_alterJob.isEmpty() )
      {
         const CHAR * objectName = _alterJob.getObjectName() ;
         const rtnAlterOptions * options = _alterJob.getOptions() ;
         const RTN_ALTER_TASK_LIST & taskList = _alterJob.getAlterTasks() ;

         for ( RTN_ALTER_TASK_LIST::const_iterator taskIter = taskList.begin() ;
               taskIter != taskList.end() ;
               ++ taskIter )
         {
            MsgHeader * pTaskMsg = NULL ;
            CHAR * pTaskMsgBuf = NULL ;
            INT32 taskMsgSize = 0 ;
            BSONObj empty ;

            const rtnAlterTask * task = ( *taskIter ) ;

            rc = task->toCMDMessage( &pTaskMsgBuf, &taskMsgSize, objectName,
                                     empty, NULL, 0, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build alter command, "
                         "rc: %d", rc ) ;

            pTaskMsg = (MsgHeader *)pTaskMsgBuf ;
            _arguments.setTaskRunner( task ) ;

            rc = _coordDataCMD3Phase::execute( pTaskMsg, cb, contextID, buf ) ;
            msgReleaseBuffer( pTaskMsgBuf, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to execute command, rc: %d", rc ) ;
               if ( options->isIgnoreException() )
               {
                   rc = SDB_OK ;
               }
               else
               {
                  break ;
               }
            }

            _arguments.clear() ;
         }

         if ( SDB_OK != _alterJob.getParseRC() )
         {
            // Report the parse error
            rc = _alterJob.getParseRC() ;
            goto error ;
         }
      }

   done :
      // No context returned
      contextID = -1 ;

      PD_TRACE_EXITRC( COORD_DATAALTER_EXECUTE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCMD_PARSEMSG, "_coordDataCMDAlter::_parseMsg" )
   INT32 _coordDataCMDAlter::_parseMsg ( MsgHeader *pMsg,
                                         coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCMD_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, FIELD_NAME_NAME,
                                      pArgs->_targetName ) ;
         PD_CHECK( SDB_OK == rc, SDB_INVALIDARG, error, PDERROR,
                   "Get failed[%s] failed on command[%s], rc: %d",
                   FIELD_NAME_NAME, getName(), rc ) ;

         PD_CHECK( !pArgs->_targetName.empty(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse command [%s]: name is empty", getName() ) ;
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( COORD_ALTERCMD_PARSEMSG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   coordCMDArguments * _coordDataCMDAlter::_getArguments ()
   {
      return ( coordCMDArguments * )( &_arguments ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER_DOONCATA, "_coordDataCMDAlter::_doOnCataGroup" )
   INT32 _coordDataCMDAlter::_doOnCataGroup ( MsgHeader * pMsg,
                                              pmdEDUCB * cb,
                                              rtnContextCoord::sharePtr * ppContext,
                                              coordCMDArguments * pArgs,
                                              CoordGroupList * pGroupLst,
                                              vector<BSONObj> * pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER_DOONCATA ) ;

      rc = _coordDataCMD3Phase::_doOnCataGroup( pMsg, cb, ppContext, pArgs,
                                                pGroupLst, pReplyObjs ) ;

      if ( NULL != pReplyObjs && !pReplyObjs->empty() )
      {
         rc = _extractPostTasks( (*pReplyObjs)[0] ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to extract post tasks, rc: %d",
                      rc ) ;
         rc = _getPostTasksObj( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get post tasks obj, rc: %d",
                      rc ) ;
      }

   done :
      PD_TRACE_EXITRC( COORD_DATAALTER_DOONCATA, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER_DOONCATA2, "_coordDataCMDAlter::_doOnCataGroupP2" )
   INT32 _coordDataCMDAlter::_doOnCataGroupP2 ( MsgHeader * pMsg,
                                                pmdEDUCB * cb,
                                                rtnContextCoord::sharePtr *ppContext,
                                                coordCMDArguments * pArgs,
                                                const CoordGroupList & groupLst,
                                                vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER_DOONCATA2 ) ;

      const rtnAlterTask * task = _arguments.getTaskRunner() ;

      if ( NULL != task && task->testFlags( RTN_ALTER_TASK_FLAG_3PHASE ) )
      {
         rtnContextBuf replyBuff ;

         rc = _processContext( cb, ppContext, 1, replyBuff ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process context, rc: %d", rc ) ;

         try
         {
            while ( !replyBuff.eof() )
            {
               BSONObj reply ;
               rc = replyBuff.nextObj( reply ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get obj from obj buf, rc: %d",
                            rc ) ;
               rc = _extractPostTasks( reply ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to extract post tasks, rc: %d" ) ;
               rc = _getPostTasksObj( cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get post tasks obj, rc: %d" ) ;

               cataObjs.push_back( reply.getOwned() ) ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get reply object, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }
      else
      {
         rc = _coordDataCMD2Phase::_doOnCataGroupP2( pMsg, cb, ppContext,
                                                     pArgs, groupLst, cataObjs ) ;
      }

   done :
      PD_TRACE_EXITRC( COORD_DATAALTER_DOONCATA2, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER_DOONDATA2, "_coordDataCMDAlter::_doOnDataGroupP2" )
   INT32 _coordDataCMDAlter::_doOnDataGroupP2 ( MsgHeader * pMsg,
                                                pmdEDUCB * cb,
                                                rtnContextCoord::sharePtr *ppContext,
                                                coordCMDArguments * pArgs,
                                                const CoordGroupList & groupLst,
                                                const vector<BSONObj> & cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER_DOONDATA2 ) ;

      const rtnAlterTask * task = _arguments.getTaskRunner() ;

      if ( NULL != task && task->testFlags( RTN_ALTER_TASK_FLAG_3PHASE ) )
      {
         rc = _coordDataCMD3Phase::_doOnDataGroupP2( pMsg, cb, ppContext,
                                                     pArgs, groupLst,
                                                     cataObjs ) ;
      }
      else
      {
         rc = _coordDataCMD2Phase::_doOnDataGroupP2( pMsg, cb, ppContext,
                                                     pArgs, groupLst,
                                                     cataObjs ) ;
      }

      PD_TRACE_EXITRC( COORD_DATAALTER_DOONDATA2, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER_ROLLBACKDATA, "_coordDataCMDAlter::_rollbackOnDataGroup" )
   INT32 _coordDataCMDAlter::_rollbackOnDataGroup ( MsgHeader * pMsg,
                                                    pmdEDUCB * cb,
                                                    coordCMDArguments * pArgs,
                                                    const CoordGroupList & groupLst )
   {
      INT32 rc = SDB_OK, tmprc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER_ROLLBACKDATA ) ;

      rc = _coordDataCMD3Phase::_rollbackOnDataGroup( pMsg, cb, pArgs, groupLst ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to do commit, rc: %d", rc ) ;
      }

      tmprc = _cancelPostTasks( _arguments._targetName.c_str(),
                                _arguments.getPostTasks(), cb ) ;
      if ( SDB_OK != tmprc )
      {
         PD_LOG( PDWARNING, "Failed to cancel post tasks, rc: %d", tmprc ) ;
      }

      PD_TRACE_EXITRC( COORD_DATAALTER_ROLLBACKDATA, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER__DOCOMMIT, "_coordDataCMDAlter::_doCommit" )
   INT32 _coordDataCMDAlter::_doCommit ( MsgHeader * pMsg,
                                         pmdEDUCB * cb,
                                         rtnContextCoord::sharePtr *ppContext,
                                         coordCMDArguments * pArgs )
   {
      INT32 rc = SDB_OK ;
      CLS_TASK_TYPE type = CLS_TASK_UNKNOWN ;

      PD_TRACE_ENTRY( COORD_DATAALTER__DOCOMMIT ) ;

      rc = _coordDataCMD3Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do commit, rc: %d", rc ) ;

      if ( !_arguments.getPostTasks().empty() )
      {
         // Execute post tasks
         rc = _executePostTasks( _arguments._targetName.c_str(),
                                 _arguments.getPostTasks(), cb, &type ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to execute post tasks, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( COORD_DATAALTER__DOCOMMIT, rc ) ;
      return rc ;

   error :
      _cancelPostTasks( _arguments._targetName.c_str(),
                        _arguments.getPostTasks(), cb ) ;
      goto done ;
   }

   INT32 _coordDataCMDAlter::_generateCataMsg ( MsgHeader * pMsg,
                                                pmdEDUCB * cb,
                                                coordCMDArguments * pArgs,
                                                CHAR ** ppMsgBuf,
                                                INT32 * pBufSize )
   {
      pMsg->opCode = _getCatalogMessageType() ;
      *ppMsgBuf = ( CHAR * )pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTER_GENDATAMSG, "_coordDataCMDAlter::_generateDataMsg" )
   INT32 _coordDataCMDAlter::_generateDataMsg( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               coordCMDArguments *pArgs,
                                               const vector<BSONObj> &cataObjs,
                                               CHAR **ppMsgBuf,
                                               INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTER_GENDATAMSG ) ;

      CHAR *pBuf = NULL ;
      INT32 bufSize = 0 ;
      BOOLEAN hasInfo = FALSE ;
      BSONObj indexInfo, newMatcher ;
      BSONObjBuilder builder ;

      if ( 0 == cataObjs.size() )
      {
         rc = _coordDataCMD3Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                     cataObjs, ppMsgBuf,
                                                     pBufSize ) ;
         goto done ;
      }

      rc = _extractIndexInfo( cataObjs[0], indexInfo, hasInfo ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to extract index info from catalog reply, rc: %d",
                   rc ) ;

      if ( !hasInfo )
      {
         rc = _coordDataCMD3Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                     cataObjs, ppMsgBuf,
                                                     pBufSize ) ;
         goto done ;
      }

     /* build new matcher
      *
      * { AlterType: "collection", Name: "foo.bar", Options: {},
      *   Alter: { Name: "create id index", Args: {} } }
      * =>
      * { AlterType: "collection", Name: "foo.bar", Options: {},
      *   Alter: { Name: "create id index", Args: {} },
      *   AlterInfo: { Index:[ { Collection: "cs.cl",IndexDef:xxx } ] } }
      */
      builder.appendElements( pArgs->_boQuery ) ;
      builder.append( FIELD_NAME_ALTER_INFO, indexInfo ) ;
      newMatcher = builder.obj() ;

      rc = msgBuildQueryMsg( &pBuf, &bufSize,
                             CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION,
                             0, 0, 0, -1,
                             &newMatcher, NULL, NULL, NULL,
                             cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Build data message failed on command[%s], rc: %d",
                   getName(), rc ) ;

      *ppMsgBuf = (CHAR*)pBuf ;
      *pBufSize = bufSize ;

   done :
      PD_TRACE_EXITRC( COORD_ALTER_GENDATAMSG, rc ) ;
      return rc ;
   error :
      if ( pBuf )
      {
         msgReleaseBuffer( pBuf, cb ) ;
         pBuf = NULL ;
         *ppMsgBuf = NULL ;
         *pBufSize = 0 ;
      }
      goto done ;
   }

   INT32 _coordDataCMDAlter::_extractIndexInfo( const BSONObj& reply,
                                                BSONObj& indexInfo,
                                                BOOLEAN& hasInfo )
   {
      INT32 rc = SDB_OK ;
      hasInfo = FALSE ;

      try
      {
         // get index unique id from catalog reply
         BSONElement ele = reply.getField( FIELD_NAME_INDEX ) ;
         if ( ele.eoo() )
         {
            hasInfo = FALSE ;
            goto done ;
         }

         PD_CHECK( Array == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid field[%s] type[%d] in obj[%s]",
                   FIELD_NAME_INDEX, ele.type(),
                   reply.toString().c_str() ) ;

         // if it is empty array
         if ( ele.Obj().isEmpty() )
         {
            hasInfo = FALSE ;
            goto done ;
         }

         indexInfo = ele.wrap() ;
         hasInfo = TRUE ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER__EXTPOSTTASKS, "_coordDataCMDAlter::_extractPostTask" )
   INT32 _coordDataCMDAlter::_extractPostTasks ( const BSONObj & reply )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER__EXTPOSTTASKS ) ;

      if ( reply.hasField( CAT_TASKID_NAME ) )
      {
         BSONElement element = reply.getField( CAT_TASKID_NAME ) ;

         if ( Array == element.type() )
         {
            BSONObjIterator iterTask( element.embeddedObject() ) ;
            while ( iterTask.more() )
            {
               BSONElement beTask = iterTask.next() ;
               PD_CHECK( beTask.isNumber(), SDB_SYS, error, PDERROR,
                         "Failed to post task" ) ;
               rc = _arguments.addPostTask( (UINT64)beTask.numberLong() ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add task, rc: %d", rc ) ;
            }
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get task from reply" ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( COORD_DATAALTER__EXTPOSTTASKS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER__GETPOSTTASKS, "_coordDataCMDAlter::_getPostTasks" )
   INT32 _coordDataCMDAlter::_getPostTasksObj( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      CHAR * msgBuff = NULL ;
      INT32 msgSize = 0 ;
      MsgHeader * msgHeader = NULL ;

      vector<BSONObj> reply ;
      BSONObj taskDesc ;

      PD_TRACE_ENTRY( COORD_DATAALTER__GETPOSTTASKS ) ;

      if( _arguments.getPostTasks().size() <= 0 )
      {
         goto done;
      }

      rc = _buildPostTasks( _arguments.getPostTasks(), taskDesc ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build post task condition, rc: %d", rc ) ;

      rc = msgBuildQueryMsg( &msgBuff, &msgSize, CAT_TASK_INFO_COLLECTION,
                             0, 0, 0, -1, &taskDesc, NULL, NULL, NULL,
                             cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build query message, rc: %d", rc ) ;


      msgHeader = (MsgHeader *)msgBuff ;
      msgHeader->opCode = MSG_CAT_QUERY_TASK_REQ ;

      /// get task info from catalog.
      rc = executeOnCataGroup( msgHeader, cb, NULL, &reply ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get task info from catalog, rc: %d",
                   rc ) ;

      for ( vector<BSONObj>::const_iterator iterTask = reply.begin() ;
            iterTask != reply.end() ;
            iterTask ++ )
      {
         rc = _checkPostTask( cb, *iterTask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check task info, rc: %d", rc ) ;

         rc = _arguments.addPostTaskObj( *iterTask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add task info, rc: %d",
                      rc ) ;
      }

   done :
      if(NULL != msgBuff)
      {
         msgReleaseBuffer( msgBuff, cb ) ;
         msgBuff = NULL;
      }
      PD_TRACE_EXITRC( COORD_DATAALTER__GETPOSTTASKS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER__CHKPOSTTASK, "_coordDataCMDAlter::_checkPostTask" )
   INT32 _coordDataCMDAlter::_checkPostTask( pmdEDUCB *cb,
                                             const BSONObj &taskObj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER__CHKPOSTTASK ) ;

      CLS_TASK_TYPE taskType = CLS_TASK_UNKNOWN ;

      try
      {
         BSONElement ele ;
         PD_CHECK( taskObj.hasField( FIELD_NAME_TASKTYPE ),
                   SDB_SYS, error, PDERROR,
                   "Failed to get task field[%s] from obj[%s]",
                   FIELD_NAME_TASKTYPE, taskObj.toString().c_str() );
         ele = taskObj.getField( FIELD_NAME_TASKTYPE ) ;
         PD_CHECK( NumberInt == ele.type(), SDB_SYS, error, PDERROR,
                   "Failed to parse task info [%s]: task type is not a integer",
                   taskObj.toString().c_str() ) ;
         taskType = (CLS_TASK_TYPE)ele.Int() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse task info, occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// in transaction, can't do split
      if ( taskType == CLS_TASK_SPLIT &&
           cb->isTransaction() )
      {
         rc = SDB_OPERATION_INCOMPATIBLE ;
         PD_LOG_MSG( PDERROR, "Can't do split in transaction" ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_DATAALTER__CHKPOSTTASK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DATAALTER__BLDPOSTTASKS, "_coordDataCMDAlter::_buildPostTasks" )
   INT32 _coordDataCMDAlter::_buildPostTasks ( const ossPoolList< UINT64 > & postTasks,
                                               BSONObj & taskDesc )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DATAALTER__BLDPOSTTASKS ) ;

      BSONObjBuilder builder ;
      BSONObjBuilder taskBuilder( builder.subobjStart( FIELD_NAME_TASKID ) ) ;
      BSONArrayBuilder arrBuilder( taskBuilder.subarrayStart( "$in" ) ) ;
      for ( ossPoolList<UINT64>::const_iterator iter = postTasks.begin() ;
            iter != postTasks.end() ;
            iter ++ )
      {
         arrBuilder.append( (INT64)( *iter ) ) ;
      }
      arrBuilder.done() ;
      taskBuilder.done() ;
      taskDesc = builder.obj() ;

      PD_TRACE_EXITRC( COORD_DATAALTER__BLDPOSTTASKS, rc ) ;

      return rc ;
   }

   INT32 _coordDataCMDAlter::_executePostTasks ( const CHAR * name,
                                                 const ossPoolList< UINT64 > & postTasks,
                                                 pmdEDUCB * cb,
                                                 CLS_TASK_TYPE *type )
   {
      return SDB_OK ;
   }

   INT32 _coordDataCMDAlter::_cancelPostTasks ( const CHAR * name,
                                                const ossPoolList< UINT64 > & postTasks,
                                                pmdEDUCB * cb )
   {
      return SDB_OK ;
   }

   /*
      _coordCMDTestCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTestCollectionSpace,
                                      CMD_NAME_TEST_COLLECTIONSPACE,
                                      TRUE ) ;
   _coordCMDTestCollectionSpace::_coordCMDTestCollectionSpace()
   {
   }

   _coordCMDTestCollectionSpace::~_coordCMDTestCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_TESTCS_EXE, "_coordCMDTestCollectionSpace::execute" )
   INT32 _coordCMDTestCollectionSpace::execute( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                INT64 &contextID,
                                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMD_TESTCS_EXE ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator = NULL ;
      rtnContextBuf buffObj ;
      const CHAR *pQuery = NULL ;
      const CHAR *pCSName = NULL ;

      contextID = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract query message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj objQuery( pQuery ) ;
         BSONElement e = objQuery.getField( FIELD_NAME_NAME ) ;
         pCSName = e.valuestrsafe() ;
         if ( 0 == ossStrcmp( pCSName, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS ) )
         {
            goto done ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = pFactory->create( CMD_NAME_LIST_COLLECTIONSPACES, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                 CMD_NAME_LIST_COLLECTIONSPACES, rc ) ;
         goto error ;
      }
      rc = pOperator->init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator failed[%s], rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }
      rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Execute operator[%s] failed, rc: %d",
                  pOperator->getName(), rc ) ;
         goto error ;
      }

      /// get more
      rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
      if ( rc )
      {
         contextID = -1 ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_DMS_CS_NOTEXIST ;
            PD_LOG ( PDWARNING, "Collection space[%s] doesn't exist",
                     pCSName ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Getmore failed, rc: %d", rc ) ;
         }
      }

   done:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC ( COORD_CMD_TESTCS_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDTestCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTestCollection,
                                      CMD_NAME_TEST_COLLECTION,
                                      TRUE ) ;
   _coordCMDTestCollection::_coordCMDTestCollection()
   {
   }

   _coordCMDTestCollection::~_coordCMDTestCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_TESTCL_EXE, "_coordCMDTestCollection::execute" )
   INT32 _coordCMDTestCollection::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      rtnContextCoord::sharePtr pContext ;
      PD_TRACE_ENTRY ( COORD_CMD_TESTCL_EXE ) ;

      contextID  = -1 ;

      // In early versions, test collection is done by a list command. Now we
      // first try with test command. If it failed with error of unknow nessage
      // (maybe the catalogue is old version), then try in the old way.
      rc = executeOnCataGroup( pMsg, cb, TRUE, NULL, &pContext, NULL ) ;
      if ( rc )
      {
         if ( SDB_INVALIDARG == rc )
         {
            rc = _testByListCmd( pMsg, cb, contextID, buf ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }
      else if( pContext )
      {
         try
         {
            rtnContextBuf buffObj ;
            rc = pContext->getMore( -1, buffObj, cb ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_DMS_NOTEXIST ;
                  PD_LOG ( PDINFO, "Test collection doesn't exist" ) ;
               }
               else
               {
                  PD_LOG ( PDERROR, "Test collection get more failed, rc: %d", rc ) ;
               }
            }
            else
            {
               BSONObj obj = BSONObj( buffObj.data() ) ;
               BSONElement ele = obj.getField( FIELD_NAME_VERSION ) ;
               if( NumberInt == ele.type() || NumberLong == ele.type() )
               {
                  buf->setStartFrom( ele.numberInt() ) ;
               }
            }
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e )  ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }
      else
      {
         SDB_ASSERT( pContext, "context should not be NULL" ) ;
      }

   done:
      if ( pContext )
      {
         INT64 delContextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( delContextID, cb ) ;
         pContext.release() ;
      }
      PD_TRACE_EXITRC ( COORD_CMD_TESTCL_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_TESTCL__TESTBYLISTCMD, "_coordCMDTestCollection::_testByListCmd" )
   INT32 _coordCMDTestCollection::_testByListCmd( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  INT64 &contextID,
                                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMD_TESTCL__TESTBYLISTCMD ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator = NULL ;
      rtnContextBuf buffObj ;
      BSONObj obj ;
      BSONElement ele ;
      const CHAR *pQuery = NULL ;
      const CHAR *pCLName = NULL ;

      contextID = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract query message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj objQuery( pQuery ) ;
         BSONElement e = objQuery.getField( FIELD_NAME_NAME ) ;
         pCLName = e.valuestrsafe() ;
         if ( 0 == ossStrcmp( pCLName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
         {
            goto done ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = pFactory->create( CMD_NAME_LIST_COLLECTIONS, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                 CMD_NAME_LIST_COLLECTIONS, rc ) ;
         goto error ;
      }
      rc = pOperator->init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator failed[%s], rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }
      rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Execute operator[%s] failed, rc: %d",
                  pOperator->getName(), rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
      if ( rc )
      {
         contextID = -1 ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_DMS_NOTEXIST ;
            PD_LOG ( PDWARNING, "Collection[%s] doesn't exist", pCLName ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Getmore failed, rc: %d", rc ) ;
         }
      }
      else
      {
         obj = BSONObj( buffObj.data() ) ;
         ele = obj.getField( FIELD_NAME_VERSION ) ;
         if( NumberInt == ele.type() || NumberLong == ele.type() )
         {
            buf->setStartFrom( ele.numberInt() ) ;
         }
      }

   done:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC ( COORD_CMD_TESTCL__TESTBYLISTCMD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdWaitTask implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdWaitTask,
                                      CMD_NAME_WAITTASK,
                                      TRUE ) ;
   _coordCmdWaitTask::_coordCmdWaitTask()
   {
   }

   _coordCmdWaitTask::~_coordCmdWaitTask()
   {
   }

   INT32 _coordCmdWaitTask::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 firstResultCode = SDB_OK ;
      SET_RC ignoreRC ;
      rtnContextCoord::sharePtr pContext ;
      rtnContextBuf buffObj ;
      BOOLEAN allTaskFinish = TRUE ;
      INT32 queryTimes      = 0 ;
      pmdKRCB *pKRCB        = pmdGetKRCB() ;
      clsTask *pTask        = NULL ;
      contextID             = -1 ;
      pMsg->opCode          = MSG_CAT_QUERY_TASK_REQ ;
      pMsg->TID             = cb->getTID() ;

      ignoreRC.insert( SDB_DMS_EOC ) ;
      ignoreRC.insert( SDB_CAT_TASK_NOTFOUND ) ;

      // query catalog until all tasks finished
      while ( TRUE )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = executeOnCataGroup( pMsg, cb, TRUE, &ignoreRC, &pContext, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query task on catalog failed, rc: %d", rc ) ;
            goto error ;
         }

         // loop every task
         while ( pContext )
         {
            BSONObj taskObj ;

            rc = pContext->getMore( 1, buffObj, cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get more, rc: %d", rc ) ;

            try
            {
               taskObj = BSONObj( buffObj.data() ) ;
            }
            catch( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
            }

            rc = clsNewTask( taskObj, pTask ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to new task, rc: %d", rc ) ;

            if ( CLS_TASK_STATUS_FINISH != pTask->status() )
            {
               allTaskFinish = FALSE ;
               clsReleaseTask( pTask ) ;
               continue ;
            }

            if( SDB_OK == firstResultCode && SDB_OK != pTask->resultCode() )
            {
               firstResultCode = pTask->resultCode() ;
               // build error object
               try
               {
                  BSONObjBuilder errBuilder ;
                  pTask->toErrInfo( errBuilder ) ;
                  BSONObj errObj = errBuilder.done() ;
                  if ( !errObj.isEmpty() )
                  {
                     PD_LOG( PDERROR, "Task[%llu] failed: %s",
                             pTask->taskID(), errObj.toString().c_str() ) ;
                     if ( buf )
                     {
                        *buf = rtnContextBuf( errObj ) ;
                        INT32 rc1 = buf->getOwned() ;
                        if ( rc1 )
                        {
                           PD_LOG( PDERROR, "Failed to build buffer, rc: %d",
                                   rc1 ) ;
                        }
                     }
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               }
            }

            clsReleaseTask( pTask ) ;
         }

         pKRCB->getRTNCB()->contextDelete( pContext->contextID(), cb ) ;
         pContext.release() ;

         if ( allTaskFinish )
         {
            break ;
         }
         allTaskFinish = TRUE ;

         queryTimes++ ;
         if ( 1 == queryTimes )
         {
            ossSleep( 10 ) ;
         }
         else if ( 2 == queryTimes )
         {
            ossSleep( 100 ) ;
         }
         else
         {
            ossSleep( OSS_ONE_SEC ) ;
         }
      }

   done:
      if ( SDB_OK == rc && SDB_OK != firstResultCode )
      {
         rc = firstResultCode ;
      }
      if ( pTask )
      {
         clsReleaseTask( pTask ) ;
      }
      if ( pContext )
      {
         pKRCB->getRTNCB()->contextDelete( pContext->contextID(),  cb ) ;
         pContext.release() ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdCancelTask implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdCancelTask,
                                      CMD_NAME_CANCEL_TASK,
                                      TRUE ) ;
   _coordCmdCancelTask::_coordCmdCancelTask()
   {
   }

   _coordCmdCancelTask::~_coordCmdCancelTask()
   {
   }

   INT32 _coordCmdCancelTask::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator      = NULL ;
      BOOLEAN async                 = FALSE ;
      UINT64 taskID                 = CLS_INVALID_TASKID ;
      contextID                     = -1 ;
      INT32 rcTmp                   = SDB_OK ;
      CHAR *pWaitMsg                = NULL ;
      INT32 waitMsgSize             = 0 ;
      CoordGroupList groupLst ;
      BSONObj matcher ;

      // extract message
      const CHAR *pQueryBuf = NULL ;
      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQueryBuf,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      try
      {
         matcher = BSONObj( pQueryBuf ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = rtnGetBooleanElement( matcher, FIELD_NAME_ASYNC, async ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ASYNC, rc ) ;

      rc = rtnGetNumberLongElement( matcher, FIELD_NAME_TASKID,
                                    (INT64&)taskID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_TASKID, rc ) ;

      // cancel catalog's task
      pMsg->opCode = MSG_CAT_TASK_CANCEL_REQ ;

      rc = executeOnCataGroup( pMsg, cb, &groupLst, NULL, TRUE, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Excute on catalog failed, rc: %d", rc ) ;

      // notify to data node
      pMsg->opCode = MSG_BS_QUERY_REQ ;

      rcTmp = executeOnDataGroup( pMsg, cb, groupLst, TRUE, NULL, NULL, NULL,
                                  buf ) ;
      if ( rcTmp )
      {
         PD_LOG( PDWARNING, "Failed to notify to data node, rc: %d", rcTmp ) ;
      }

      // if sync, wait task to finish
      if ( !async )
      {
         // build wait task message
         BSONObj matcher1 ;
         try
         {
            matcher1 = BSON( FIELD_NAME_TASKID << (INT64)taskID ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
         rc = msgBuildQueryMsg( &pWaitMsg, &waitMsgSize,
                                CMD_ADMIN_PREFIX CMD_NAME_WAITTASK,
                                0, 0, 0, -1,
                                &matcher1, NULL, NULL, NULL, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build wait task message failed, rc: %d",
                      rc ) ;

         // create coordOperator
         rc = pFactory->create( CMD_NAME_WAITTASK, pOperator ) ;
         PD_RC_CHECK( rc, PDERROR, "Create operator by name[%s] failed, rc: %d",
                      CMD_NAME_WAITTASK, rc ) ;

         rc = pOperator->init( _pResource, cb, getTimeout() ) ;
         PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                      pOperator->getName(), rc ) ;

         // execute wait task
         rc = pOperator->execute( (MsgHeader*)pWaitMsg, cb, contextID, buf ) ;
         if ( SDB_TASK_HAS_CANCELED == rc )
         {
            // the task was cancelled, so result code is -243, just ignore
            rc = SDB_OK ;
         }
         if ( rc )
         {
            PD_LOG( PDWARNING, "Excute wait task[%llu], rc: %d", taskID, rc ) ;
         }
      }

   done:
      if ( pWaitMsg )
      {
         msgReleaseBuffer( pWaitMsg, cb ) ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDTruncate implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDTruncate,
                                      CMD_NAME_TRUNCATE,
                                      FALSE ) ;
   _coordCMDTruncate::_coordCMDTruncate()
   {
   }

   _coordCMDTruncate::~_coordCMDTruncate()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_TRUNCATECL_REGEVENTHANDLERS, "_coordCMDTruncate::_regEventHandlers" )
   INT32 _coordCMDTruncate::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_TRUNCATECL_REGEVENTHANDLERS ) ;

      rc = _regEventHandler( &_globIdxHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register global index handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, rc: %d",
                   rc ) ;

      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_TRUNCATECL_REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_TRUNCATECL_PARSEMSG, "_coordCMDTruncate::_parseMsg" )
   INT32 _coordCMDTruncate::_parseMsg( MsgHeader *pMsg,
                                       coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_TRUNCATECL_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( dmsCheckFullCLName( pArgs->_targetName.c_str() ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Collection name is invalid[%s], rc: %d",
                    pArgs->_targetName.c_str(), rc ) ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse truncate message, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_TRUNCATECL_PARSEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _coordCMDTruncate::_generateCataMsg( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              CHAR **ppMsgBuf,
                                              INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_TRUNCATE_REQ ;
      *ppMsgBuf = (CHAR *)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_TRUNCATE__GENDATAMSG, "_coordCMDTruncate::_generateDataMsg" )
   INT32 _coordCMDTruncate::_generateDataMsg( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              const vector<BSONObj> &cataObjs,
                                              CHAR **ppMsgBuf,
                                              INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_TRUNCATE__GENDATAMSG ) ;

      BSONObj cataReplyObj ;

      rc = _BASE::_generateDataMsg( pMsg, cb, pArgs, cataObjs, ppMsgBuf,
                                    pBufSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate truncate CL message, "
                   "rc: %d", rc ) ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      cataReplyObj = cataObjs[ 0 ] ;

      try
      {
         CoordCataInfoPtr cataPtr ;
         BSONObj objCata ;
         BSONElement beCollection = cataReplyObj.getField( CAT_COLLECTION ) ;
         if ( Object == beCollection.type() )
         {
            objCata = beCollection.embeddedObject() ;
            // The catalog info of collection maybe too old
            // The reply from Catalog implies that info need to be updated
            PD_LOG( PDDEBUG, "Updating catalog info of collection [%s]",
                    pArgs->_targetName.c_str() ) ;
            rc = coordInitCataPtrFromObj( objCata, cataPtr ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Init catalog info from obj[%s] failed, "
                       "collection:%s, rc: %d", pArgs->_targetName.c_str(),
                       objCata.toString().c_str(), rc ) ;
               goto error ;
            }
            // update with latest catalog info
            _pResource->addCataInfo( cataPtr ) ;
            _cataPtr = cataPtr ;
            ((MsgOpQuery*)(*ppMsgBuf))->version = cataPtr->getVersion() ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Occur exception when parse catalog "
                  "object info: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_TRUNCATE__GENDATAMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_TRUNCATE__DODATAGRP, "_coordCMDTruncate::_doOnDataGroup" )
   INT32 _coordCMDTruncate::_doOnDataGroup( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            rtnContextCoord::sharePtr *ppContext,
                                            coordCMDArguments *pArgs,
                                            const CoordGroupList &groupLst,
                                            const vector<BSONObj> &cataObjs,
                                            CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_TRUNCATE__DODATAGRP ) ;

      // do on data for P1 to lock collection on data nodes
      rc = _BASE::_doOnDataGroup( pMsg, cb, ppContext, pArgs, groupLst,
                                  cataObjs, sucGroupLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do on data group, rc: %d", rc ) ;

      // now collection on data nodes are locked,
      // remove cache of related sequences
      if ( getCataPtr().get() )
      {
         rc = coordInvalidateSequenceCache( getCataPtr(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to invalidate sequence cache of "
                      "cl[%s], rc: %d", pArgs->_targetName.c_str(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_TRUNCATE__DODATAGRP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDCreateCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateCollectionSpace,
                                      CMD_NAME_CREATE_COLLECTIONSPACE,
                                      FALSE ) ;
   _coordCMDCreateCollectionSpace::_coordCMDCreateCollectionSpace()
   {
   }

   _coordCMDCreateCollectionSpace::~_coordCMDCreateCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CREATECS_EXE, "_coordCMDCreateCollectionSpace::execute" )
   INT32 _coordCMDCreateCollectionSpace::execute( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  INT64 &contextID,
                                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATECS_EXE ) ;

      const CHAR *pQuery = NULL ;

      // fill default-reply
      contextID = -1 ;
      MsgOpQuery *pCreateReq = (MsgOpQuery *)pMsg;

      try
      {
         BSONObj boQuery ;
         BSONElement ele ;
         BSONElement dsEle ;
         BSONElement mappingEle ;
         const CHAR *pCSName = NULL ;

         _printDebug ( (const CHAR*)pMsg, getName() ) ;

         rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL,
                               NULL, NULL, &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to extract message, rc: %d", rc ) ;

         boQuery = BSONObj( pQuery ) ;
         ele = boQuery.getField( FIELD_NAME_NAME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "Get field[%s] failed from obj[%s] in "
                    "command[%s]", FIELD_NAME_NAME, boQuery.toString().c_str(),
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pCSName = ele.valuestr() ;

         // Check if 'DataSource' and 'Mapping' options are specified. If yes,
         // need to check if the target collectionspaces exists on data source
         // or not.
         dsEle = boQuery.getField( FIELD_NAME_DATASOURCE ) ;
         mappingEle = boQuery.getField( FIELD_NAME_MAPPING ) ;
         if ( dsEle.eoo() )
         {
            if ( !mappingEle.eoo() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Mapping could not be used with out data "
                           "source[%d]", rc ) ;
               goto error ;
            }
         }
         else
         {
            BOOLEAN exist = FALSE ;
            const CHAR *dsName = dsEle.valuestr() ;
            const CHAR *mappingCSName = mappingEle.eoo() ?
                  pCSName : mappingEle.valuestr() ;
            UINT8 expectArgNum = mappingEle.eoo() ? 1 : 2 ;

            // The client may add the default page size field in the option,
            // event when the user does not set it explicitly in the command.
            // The default page size set by the client is 0, which means using
            // the default value of the server.
            // The query may be in the format:
            // { Name: <csName>, DataSource: "ds1", PageSize: 0 }
            BSONElement pageSizeEle = boQuery.getField( FIELD_NAME_PAGE_SIZE ) ;
            if ( !pageSizeEle.eoo() && ( 0 == pageSizeEle.numberInt() ) )
            {
               ++expectArgNum ;
            }
            if ( boQuery.hasField( FIELD_NAME_NAME ) )
            {
               ++expectArgNum ;
            }
            if ( boQuery.nFields() > expectArgNum )
            {
               rc = SDB_OPERATION_INCOMPATIBLE ;
               PD_LOG_MSG( PDERROR, "Other options are not allowed to use "
                           "together with data source[%d]", rc ) ;
               goto error ;
            }

            rc = _testCSOnDataSource( dsName, mappingCSName, exist, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Test collection space[%s] on data "
                         "source[%s] failed[%d]",
                         mappingCSName, dsName, rc  ) ;
            if ( !exist )
            {
               rc = SDB_DMS_CS_NOTEXIST ;
               PD_LOG_MSG( PDERROR, "Mapping collection space[%s] does not "
                           "exist on data source[%s]", mappingCSName, dsName ) ;
               goto error ;
            }
         }

         pCreateReq->header.opCode = MSG_CAT_CREATE_COLLECTION_SPACE_REQ ;
         // execute create collection on catalog
         rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_CS,
                           pCSName, rc, "Option:%s",
                           boQuery.toString().c_str() ) ;
         /// CHECK ERRORS
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to create collection space[%s], rc: %d",
                     pCSName, rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_CREATECS_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDCreateCollectionSpace::_testCSOnDataSource( const CHAR *dsName,
                                                              const CHAR *csName,
                                                              BOOLEAN &exist,
                                                              pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      try
      {
         utilAddrPair address ;
         BSONObj dummyObj ;
         BSONObj query = BSON( FIELD_NAME_NAME << csName ) ;
         CoordDataSourcePtr dsPtr ;
         coordDSCSChecker csChecker ;

         rc = _pResource->getDSManager()->getOrUpdateDataSource( dsName,
                                                                 dsPtr,
                                                                 cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get information of data source[%s] "
                      "failed[%d]", dsName, rc ) ;

         rc = csChecker.check( dsPtr, csName, cb, exist ) ;
         PD_RC_CHECK( rc, PDERROR, "Check existence of collection space[%s] on "
                      "data source[%s] failed[%d]", csName, dsName, rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDDropCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropCollectionSpace,
                                      CMD_NAME_DROP_COLLECTIONSPACE,
                                      FALSE ) ;
   _coordCMDDropCollectionSpace::_coordCMDDropCollectionSpace()
   {
   }

   _coordCMDDropCollectionSpace::~_coordCMDDropCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCS_REGEVENTHANDLERS, "_coordCMDDropCollectionSpace::_regEventHandlers" )
   INT32 _coordCMDDropCollectionSpace::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DROPCS_REGEVENTHANDLERS ) ;

      rc = _regEventHandler( &_globIdxHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register global index handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, rc: %d",
                   rc ) ;

      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_DROPCS_REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCS_PARSEMSG, "_coordCMDDropCollectionSpace::_parseMsg" )
   INT32 _coordCMDDropCollectionSpace::_parseMsg ( MsgHeader *pMsg,
                                                   coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCS_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery,
                                      CAT_COLLECTION_SPACE_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_COLLECTION_SPACE_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( dmsCheckCSName( pArgs->_targetName.c_str() ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Collectionspace name is invalid[%s], rc: %d",
                    pArgs->_targetName.c_str(), rc ) ;
            goto error ;
         }

         // Add ignore return codes
         pArgs->_ignoreRCList.insert( SDB_DMS_CS_NOTEXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPCS_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDDropCollectionSpace::_generateCataMsg ( MsgHeader *pMsg,
                                                          pmdEDUCB *cb,
                                                          coordCMDArguments *pArgs,
                                                          CHAR **ppMsgBuf,
                                                          INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_DROP_SPACE_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCS_DOCOMPLETE, "_coordCMDDropCollectionSpace::_doComplete" )
   INT32 _coordCMDDropCollectionSpace::_doComplete ( MsgHeader *pMsg,
                                                     pmdEDUCB * cb,
                                                     coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCS_DOCOMPLETE ) ;
      vector< string > subCLSet ;
      _pResource->removeCataInfoByCS( pArgs->_targetName.c_str(), &subCLSet ) ;

      /// clear relate sub collection's catalog info
      vector< string >::iterator it = subCLSet.begin() ;
      while( it != subCLSet.end() )
      {
         _pResource->removeCataInfo( (*it).c_str() ) ;
         ++it ;
      }

      // If any objects related with this cs are using data source. If yes, need
      // to invalidate cache by cs name on all coordinators.
      if ( _needNotifyInvalidateCache( pArgs ) )
      {
         coordCacheInvalidator cacheInvalidator( _pResource ) ;
         rc = cacheInvalidator.notify( COORD_CACHE_CATALOGUE,
                                       pArgs->_targetName.c_str(), cb ) ;
         PD_LOG( PDDEBUG, "Notify other coordinators to invalidate catalogue "
                 "cache of collection space[%s]", pArgs->_targetName.c_str() ) ;
         if ( rc )
         {
            // For invalidate cache failure, report warning in the log, but
            // don't interrupt the current operation.
            PD_LOG( PDWARNING, "Notify all coordinators to invalidate cache for "
                    "dropping collection space[%s] failed[%d]",
                    pArgs->_targetName.c_str(), rc ) ;
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXIT ( COORD_DROPCS_DOCOMPLETE ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCS__NEEDNOTIFYINVALIDCACHE, "_coordCMDDropCollectionSpace::_needNotifyInvalidateCache" )
   BOOLEAN _coordCMDDropCollectionSpace::_needNotifyInvalidateCache( coordCMDArguments *pArgs )
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY( COORD_DROPCS__NEEDNOTIFYINVALIDCACHE ) ;
      // Check the group list returned by catalogue. If any one is a data source
      // group, need to broadcast the invalidation message.
      CoordGroupList::const_iterator itr = pArgs->_groupList.begin() ;
      while ( itr != pArgs->_groupList.end() )
      {
         if ( SDB_IS_DSID( itr->first ) )
         {
            result = TRUE ;
            break ;
         }
         ++itr ;
      }

      PD_TRACE_EXIT( COORD_DROPCS__NEEDNOTIFYINVALIDCACHE ) ;
      return result ;
   }

   /*
      _coordCMDRenameCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRenameCollectionSpace,
                                      CMD_NAME_RENAME_COLLECTIONSPACE,
                                      FALSE ) ;
   _coordCMDRenameCollectionSpace::_coordCMDRenameCollectionSpace()
   {
   }

   _coordCMDRenameCollectionSpace::~_coordCMDRenameCollectionSpace()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RENAMECS_PARSEMSG, "_coordCMDRenameCollectionSpace::_parseMsg" )
   INT32 _coordCMDRenameCollectionSpace::_parseMsg ( MsgHeader *pMsg,
                                                     coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_RENAMECS_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery,
                                      FIELD_NAME_OLDNAME,
                                      pArgs->_targetName ) ;
         PD_CHECK( SDB_OK == rc, SDB_INVALIDARG, error, PDERROR,
                   "Get field[%s] failed on command[%s], rc: %d",
                   FIELD_NAME_OLDNAME, getName(), rc );

         if ( dmsCheckCSName( pArgs->_targetName.c_str() ) )
         {
            PD_LOG( PDERROR, "Collection name is invalid[%s]",
                    pArgs->_targetName.c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

      // add retry error code
      pArgs->_retryRCList.insert( SDB_LOCK_FAILED ) ;

   done :
      PD_TRACE_EXITRC ( COORD_RENAMECS_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDRenameCollectionSpace::_generateCataMsg ( MsgHeader *pMsg,
                                                            pmdEDUCB *cb,
                                                            coordCMDArguments *pArgs,
                                                            CHAR **ppMsgBuf,
                                                            INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_RENAME_CS_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RENAMECS_DOCOMPLETE, "_coordCMDRenameCollectionSpace::_doComplete" )
   INT32 _coordCMDRenameCollectionSpace::_doComplete ( MsgHeader *pMsg,
                                                       pmdEDUCB * cb,
                                                       coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_RENAMECS_DOCOMPLETE ) ;

      vector< string > subCLSet ;
      _pResource->removeCataInfoByCS( pArgs->_targetName.c_str(), &subCLSet ) ;

      /// clear relate sub collection's catalog info
      vector< string >::iterator it = subCLSet.begin() ;
      while( it != subCLSet.end() )
      {
         _pResource->removeCataInfo( (*it).c_str() ) ;
         ++it ;
      }

      // Check the group list returned by catalogue. If any one is a data
      // source group, need to broadcast the invalidation message.
      if ( _needNotifyInvalidateCache( pArgs ) )
      {
         coordCacheInvalidator cacheInvalidator( _pResource ) ;
         rc = cacheInvalidator.notify( COORD_CACHE_CATALOGUE,
                                       pArgs->_targetName.c_str(), cb ) ;
         PD_LOG( PDDEBUG, "Notify other coordinators to invalidate catalogue "
                 "cache of collection space[%s]", pArgs->_targetName.c_str() ) ;
         if ( rc )
         {
            // For invalidate cache failure, report warning in the log, but
            // don't interrupt the current operation.
            PD_LOG( PDWARNING, "Notify all coordinators to invalidate "
                    "catalogue cache for renaming collection space[%s] "
                    "failed[%d]", pArgs->_targetName.c_str(), rc ) ;
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXIT ( COORD_RENAMECS_DOCOMPLETE ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RENAMECS__NEEDNOTIFYINVALIDCACHE, "_coordCMDRenameCollectionSpace::_needNotifyInvalidateCache" )
   BOOLEAN _coordCMDRenameCollectionSpace::_needNotifyInvalidateCache( coordCMDArguments *pArgs )
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY( COORD_RENAMECS__NEEDNOTIFYINVALIDCACHE ) ;
      // Check the group list returned by catalogue. If any one is a data source
      // group, need to broadcast the invalidation message.
      CoordGroupList::const_iterator itr = pArgs->_groupList.begin() ;
      while ( itr != pArgs->_groupList.end() )
      {
         if ( SDB_IS_DSID( itr->first ) )
         {
            result = TRUE ;
            break ;
         }
         ++itr ;
      }

      PD_TRACE_EXIT( COORD_RENAMECS__NEEDNOTIFYINVALIDCACHE ) ;
      return result ;
   }

   /*
      _coordCMDDropCollectionSpace implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterCollectionSpace,
                                      CMD_NAME_ALTER_COLLECTION_SPACE,
                                      FALSE ) ;
   _coordCMDAlterCollectionSpace::_coordCMDAlterCollectionSpace ()
   : _coordDataCMDAlter()
   {
   }

   _coordCMDAlterCollectionSpace::~_coordCMDAlterCollectionSpace ()
   {
   }

   /*
      _coordCMDCreateCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateCollection,
                                      CMD_NAME_CREATE_COLLECTION,
                                      FALSE ) ;
   _coordCMDCreateCollection::_coordCMDCreateCollection()
   {
   }

   _coordCMDCreateCollection::~_coordCMDCreateCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATECL_PARSEMSG, "_coordCMDCreateCollection::_parseMsg" )
   INT32 _coordCMDCreateCollection::_parseMsg ( MsgHeader *pMsg,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CREATECL_PARSEMSG ) ;

      try
      {
         BOOLEAN isMainCL = FALSE ;
         BOOLEAN isCapped = FALSE ;
         BOOLEAN isCompressed = FALSE ;
         const CHAR *dataSourceName = NULL ;
         const CHAR *mappingName = NULL ;
         UINT8 dsArgNum = 0 ;

         rc = rtnGetStringElement( pArgs->_boQuery, FIELD_NAME_DATASOURCE,
                                   &dataSourceName ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_DATASOURCE, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetStringElement( pArgs->_boQuery, FIELD_NAME_MAPPING,
                                   &mappingName ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            if ( dataSourceName )
            {
               // Field 'DataSource' is given, but not field 'Mapping'. In this
               // case, mapping to the collection with the same name of the
               // original collection.
               mappingName = pArgs->_targetName.c_str() ;
            }
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_MAPPING, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( !dataSourceName )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Mapping could not be used without data "
                                 "source[%d]", rc ) ;
            goto error ;
         }

         if ( dataSourceName )
         {
            ++dsArgNum ;
            if ( mappingName )
            {
               ++dsArgNum ;
            }

            if ( pArgs->_boQuery.hasField( FIELD_NAME_NAME ) )
            {
               ++dsArgNum ;
            }

            if ( pArgs->_boQuery.nFields() > dsArgNum )
            {
               rc = SDB_OPERATION_INCOMPATIBLE ;
               PD_LOG_MSG( PDERROR, "Other options are not allowed to use "
                                    "together with data source[%d]", rc ) ;
               goto error ;
            }
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_CAPPED,
                                    isCapped ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isCapped = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_CAPPED, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ISMAINCL,
                                    isMainCL ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isMainCL = FALSE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_ISMAINCL, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_COMPRESSED,
                                    isCompressed ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isCompressed = TRUE ;
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    FIELD_NAME_COMPRESSED, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( isMainCL )
         {
            // Check sharding keys
            BSONObj boShardingKey ;
            // Check sharding type
            string shardingType ;

            rc = rtnGetObjElement( pArgs->_boQuery, FIELD_NAME_SHARDINGKEY,
                                   boShardingKey ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                       FIELD_NAME_SHARDINGKEY, getName(), rc ) ;
               rc = SDB_FIELD_NOT_EXIST == rc ?
                    SDB_NO_SHARDINGKEY : SDB_INVALIDARG ;
               goto error ;
            }

            rc = rtnGetSTDStringElement( pArgs->_boQuery, FIELD_NAME_SHARDTYPE,
                                         shardingType ) ;
            if ( SDB_OK == rc )
            {
               if ( 0 != shardingType.compare( FIELD_NAME_SHARDTYPE_RANGE ) )
               {
                  PD_LOG( PDERROR, "Sharding type must be range in "
                          "main colllection" ) ;
                  rc = SDB_INVALID_MAIN_CL_TYPE ;
                  goto error ;
               }
            }
            else if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                       FIELD_NAME_SHARDTYPE_HASH, getName(), rc ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         // Validate collection on data source, if any.
         if ( dataSourceName )
         {
            BOOLEAN exist = FALSE ;
            // If there is no dot in the mapping name, it's a short collection
            // name. We should get the full name to test on data source.
            CHAR fullMappingName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
            SDB_ASSERT( mappingName, "Mapping should not be null" ) ;
            if ( NULL == ossStrstr( mappingName, "." ) )
            {
               string csName =
                     pArgs->_targetName.substr( 0,
                                                pArgs->_targetName.find( "." ) ) ;
               ossSnprintf( fullMappingName, DMS_COLLECTION_FULL_NAME_SZ,
                            "%s.%s", csName.c_str(), mappingName ) ;
               mappingName = fullMappingName ;
            }

            rc = _testCLOnDataSource( dataSourceName, mappingName, exist,
                                      pmdGetThreadEDUCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Test collection[%s] on data source[%s] "
                         "failed[%d]", mappingName, dataSourceName, rc ) ;
            if ( !exist )
            {
               rc = SDB_DMS_NOTEXIST ;
               PD_LOG_MSG( PDERROR, "Mapping collection[%s] does not exist on "
                           "data source[%s]", mappingName, dataSourceName ) ;
               goto error ;
            }
         }

         // Add ignored error return code
         pArgs->_ignoreRCList.insert( SDB_DMS_EXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATECL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDCreateCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       coordCMDArguments *pArgs,
                                                       CHAR **ppMsgBuf,
                                                       INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_CREATE_COLLECTION_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CRTCL_GENDATAMSG, "_coordCMDCreateCollection::_generateDataMsg" )
   INT32 _coordCMDCreateCollection::_generateDataMsg( MsgHeader *pMsg,
                                                      pmdEDUCB *cb,
                                                      coordCMDArguments *pArgs,
                                                      const vector<BSONObj> &cataObjs,
                                                      CHAR **ppMsgBuf,
                                                      INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CRTCL_GENDATAMSG ) ;

      BSONObjBuilder builder ;
      CHAR *pBuf = NULL ;
      INT32 bufSize = 0 ;

      PD_CHECK( cataObjs.size() > 0, SDB_SYS, error, PDERROR,
                "Catalog do not return index info" ) ;

      try
      {
         BSONObj newHint ;

         BSONElement ele = cataObjs[0].getField( FIELD_NAME_INDEX ) ;
         if ( ele.eoo() )
         {
            // old version of catalog doesn't has this field, just ignore error
            rc = _coordDataCMD2Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                        cataObjs, ppMsgBuf,
                                                        pBufSize ) ;
            goto done ;
         }
         PD_CHECK( Array == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid field[%s] type[%d] in obj[%s]",
                   FIELD_NAME_INDEX, ele.type(),
                   cataObjs[0].toString().c_str() ) ;

         newHint = ele.wrap() ;
         rc = msgBuildQueryMsg( &pBuf, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION,
                                0, 0, 0, -1,
                                &pArgs->_boQuery, NULL, NULL, &newHint,
                                cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Build data message failed on command[%s], rc: %d",
                      getName(), rc ) ;

         *ppMsgBuf = (CHAR*)pBuf ;
         *pBufSize = bufSize ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Occur exception when parse catalog "
                  "object info: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( COORD_CRTCL_GENDATAMSG, rc ) ;
      return rc ;
   error :
      if ( pBuf )
      {
         msgReleaseBuffer( pBuf, cb ) ;
         pBuf = NULL ;
         *ppMsgBuf = NULL ;
         *pBufSize = 0 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATECL_GENROLLBACKMSG, "_coordCMDCreateCollection::_generateRollbackDataMsg" )
   INT32 _coordCMDCreateCollection::_generateRollbackDataMsg( MsgHeader *pMsg,
                                                              pmdEDUCB *cb,
                                                              coordCMDArguments *pArgs,
                                                              CHAR **ppMsgBuf,
                                                              INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATECL_GENROLLBACKMSG ) ;

      rc = msgBuildDropCLMsg( ppMsgBuf, pBufSize, pArgs->_targetName.c_str(),
                              TRUE, FALSE, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message failed on command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATECL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDCreateCollection::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                            INT32 bufSize,
                                                            pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATECL_ROLLBACKONDATA, "_coordCMDCreateCollection::_rollbackOnDataGroup" )
   INT32 _coordCMDCreateCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                           pmdEDUCB *cb,
                                                           coordCMDArguments *pArgs,
                                                           const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATECL_ROLLBACKONDATA ) ;

      SET_RC ignoreRC ;
      rtnContextCoord::sharePtr pCtxForData ;

      ignoreRC.insert( SDB_DMS_NOTEXIST ) ;
      ignoreRC.insert( SDB_DMS_CS_NOTEXIST ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &ignoreRC, NULL,
                        &pCtxForData, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback-phase1 command[%s, target:%s] on "
                 "data group failed, rc: %d", getName(),
                 pArgs->_targetName.c_str(), rc ) ;
         goto error ;
      }

      if ( pCtxForData )
      {
         rtnContextBuf buffObj ;

         rc = _processContext( cb, &pCtxForData, -1, buffObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rollback-phase2 command[%s, targe:%s] on "
                    "data group failed, rc: %d", getName(),
                    pArgs->_targetName.c_str(), rc ) ;
            goto error ;
         }
      }

      // Clear Coord catalog info
      _pResource->removeCataInfo( pArgs->_targetName.c_str() ) ;

   done :
      if ( pCtxForData )
      {
         pmdKRCB *pKrcb = pmdGetKRCB();
         _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
         pRtncb->contextDelete ( pCtxForData->contextID(), cb ) ;
      }
      PD_TRACE_EXITRC ( COORD_CREATECL_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDCreateCollection::_testCLOnDataSource( const CHAR *dsName,
                                                         const CHAR *clName,
                                                         BOOLEAN &exist,
                                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      try
      {
         CoordDataSourcePtr dsPtr ;
         coordDSCLChecker clChecker ;
         utilAddrPair address ;
         BSONObj dummyObj ;
         BSONObj query = BSON( FIELD_NAME_NAME << clName ) ;

         rc = _pResource->getDSManager()->getOrUpdateDataSource( dsName,
                                                                 dsPtr,
                                                                 cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get information of data source[%s] "
                      "failed[%d]", dsName, rc ) ;

         rc = clChecker.check( dsPtr, clName, cb, exist ) ;
         PD_RC_CHECK( rc, PDERROR, "Check existence of collection[%s] on data "
                      "source[%s] failed[%d]", clName, dsName, rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDDropCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropCollection,
                                      CMD_NAME_DROP_COLLECTION,
                                      FALSE ) ;
   _coordCMDDropCollection::_coordCMDDropCollection()
   {
   }

   _coordCMDDropCollection::~_coordCMDDropCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL_REGEVENTHANDLERS, "_coordCMDDropCollection::_regEventHandlers" )
   INT32 _coordCMDDropCollection::_regEventHandlers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_DROPCL_REGEVENTHANDLERS ) ;

      rc = _regEventHandler( &_globIdxHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register global index handler, "
                   "rc: %d", rc ) ;

      rc = _regEventHandler( &_recycleHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register recycle handler, rc: %d",
                   rc ) ;

      rc = _regEventHandler( &_taskHandler ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register task handler, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( COORD_DROPCL_REGEVENTHANDLERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL_PARSEMSG, "_coordCMDDropCollection::_parseMsg" )
   INT32 _coordCMDDropCollection::_parseMsg ( MsgHeader *pMsg,
                                              coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCL_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( dmsCheckFullCLName( pArgs->_targetName.c_str() ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Collection name is invalid[%s], rc: %d",
                    pArgs->_targetName.c_str(), rc ) ;
            goto error ;
         }

         pArgs->_ignoreRCList.insert( SDB_DMS_NOTEXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDDropCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_DROP_COLLECTION_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL_GENDATAMSG, "_coordCMDDropCollection::_generateDataMsg" )
   INT32 _coordCMDDropCollection::_generateDataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     const vector<BSONObj> &cataObjs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCL_GENDATAMSG ) ;

      BSONObj cataReplyObj ;

      rc = _coordDataCMD3Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                  cataObjs, ppMsgBuf,
                                                  pBufSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate drop CL message, "
                   "rc: %d", rc ) ;

      if ( cataObjs.empty() )
      {
         goto done ;
      }

      cataReplyObj = cataObjs[ 0 ] ;

      try
      {
         CoordCataInfoPtr cataPtr ;
         BSONObj objCata ;
         BSONElement beCollection = cataReplyObj.getField( CAT_COLLECTION ) ;
         if ( Object == beCollection.type() )
         {
            objCata = beCollection.embeddedObject() ;
            // The catalog info of collection maybe too old
            // The reply from Catalog implies that info need to be updated
            PD_LOG( PDDEBUG, "Updating catalog info of collection [%s]",
                    pArgs->_targetName.c_str() ) ;
            rc = coordInitCataPtrFromObj( objCata, cataPtr ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Init catalog info from obj[%s] failed, "
                       "collection:%s, rc: %d", pArgs->_targetName.c_str(),
                       objCata.toString().c_str(), rc ) ;
               goto error ;
            }
            // update with latest catalog info
            _pResource->addCataInfo( cataPtr ) ;
            _cataPtr = cataPtr ;
            ((MsgOpQuery*)(*ppMsgBuf))->version = cataPtr->getVersion() ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Occur exception when parse catalog "
                  "object info: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( COORD_DROPCL_GENDATAMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL__DOCOMPLETE, "_coordCMDDropCollection::_doComplete" )
   INT32 _coordCMDDropCollection::_doComplete ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPCL__DOCOMPLETE ) ;
      ossPoolVector<string> collections ;
      CoordCataInfoPtr cataPtr = getCataPtr() ;
      if ( cataPtr.get() )
      {
         rc = coordInvalidateSequenceCache( getCataPtr(), cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to invalidate sequence cache, rc: %d", rc ) ;
      }

      if ( _needNotifyInvalidateCache( pArgs ) )
      {
         BOOLEAN notified = FALSE ;
         coordCacheInvalidator cacheInvalidator( _pResource ) ;
         try
         {
            if ( cataPtr->isMainCL() )
            {
               rc = cataPtr->getSubCLList( collections ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Get sub collection list for [%s] "
                          "failed[%d]", cataPtr->getName(), rc ) ;
                  rc = SDB_OK ;
               }
               collections.push_back( pArgs->_targetName ) ;
               rc = cacheInvalidator.notify( COORD_CACHE_CATALOGUE,
                                             collections, cb ) ;
            }
            else
            {
               rc = cacheInvalidator.notify( COORD_CACHE_CATALOGUE,
                                             pArgs->_targetName.c_str(),
                                             cb ) ;
            }
            PD_LOG( PDDEBUG, "Notify other coordinators to invalidate "
                    "catalogue cache when dropping collection[%s]",
                    pArgs->_targetName.c_str() ) ;
            if ( rc )
            {
               // For invalidation error, report warning in the log, but
               // don't interrupt the current operation.
               PD_LOG( PDWARNING, "Notify all coordinators to invalidate cache "
                       "when dropping collection[%s] failed[%d]",
                       pArgs->_targetName.c_str(), rc ) ;
               rc = SDB_OK ;
            }
            else
            {
               notified = TRUE ;
            }
         }
         catch ( std::exception &e )
         {
            rc= ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
            // In case of exception, we don't know which one to be clean. Try
            // to notify coordinators to clean all catalogue information.
            if ( !notified )
            {
               cacheInvalidator.notify( COORD_CACHE_CATALOGUE, NULL, cb ) ;
            }
            rc = SDB_OK ;
         }
      }

      if ( collections.size() > 0 )
      {
         for ( ossPoolVector<string>::const_iterator citr = collections.begin();
               citr != collections.end(); ++citr )
         {
            _pResource->removeCataInfoWithMain( citr->c_str() ) ;
         }
      }
      else
      {
         _pResource->removeCataInfoWithMain( pArgs->_targetName.c_str() ) ;
      }

   done:
      PD_TRACE_EXIT ( COORD_DROPCL__DOCOMPLETE ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPCL__NEEDNOTIFYINVALIDCACHE, "_coordCMDDropCollection::_needNotifyInvalidateCache" )
   BOOLEAN _coordCMDDropCollection::_needNotifyInvalidateCache( coordCMDArguments *pArgs )
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY( COORD_DROPCL__NEEDNOTIFYINVALIDCACHE ) ;
      // Check the group list returned by catalogue. If any one is a data source
      // group, need to broadcast the invalidation message.
      CoordGroupList::const_iterator itr = pArgs->_groupList.begin() ;
      while ( itr != pArgs->_groupList.end() )
      {
         if ( SDB_IS_DSID( itr->first ) )
         {
            result = TRUE ;
            break ;
         }
         ++itr ;
      }

      PD_TRACE_EXIT( COORD_DROPCL__NEEDNOTIFYINVALIDCACHE ) ;
      return result ;
   }

   /*
      _coordCMDRenameCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRenameCollection,
                                      CMD_NAME_RENAME_COLLECTION,
                                      FALSE ) ;
   _coordCMDRenameCollection::_coordCMDRenameCollection()
   {
   }

   _coordCMDRenameCollection::~_coordCMDRenameCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RENAMECL_PARSEMSG, "_coordCMDRenameCollection::_parseMsg" )
   INT32 _coordCMDRenameCollection::_parseMsg ( MsgHeader *pMsg,
                                              coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_RENAMECL_PARSEMSG ) ;

      try
      {
         string csName, clShortName ;
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTIONSPACE,
                                      csName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTIONSPACE, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_OLDNAME,
                                      clShortName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_OLDNAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         pArgs->_targetName = csName ;
         pArgs->_targetName += "." ;
         pArgs->_targetName += clShortName ;

         if ( dmsCheckFullCLName( pArgs->_targetName.c_str() ) )
         {
            PD_LOG( PDERROR, "Collection name is invalid[%s]",
                    pArgs->_targetName.c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // add retry error code
      pArgs->_retryRCList.insert( SDB_LOCK_FAILED ) ;

   done :
      PD_TRACE_EXITRC ( COORD_RENAMECL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDRenameCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_RENAME_CL_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RENAMECL_DOCOMPLETE, "_coordCMDRenameCollection::_doComplete" )
   INT32 _coordCMDRenameCollection::_doComplete ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_RENAMECL_DOCOMPLETE ) ;
      ossPoolVector<string> collections ;
      CoordCataInfoPtr cataPtr = getCataPtr() ;

      if ( _needNotifyInvalidateCache( pArgs ) )
      {
         coordCacheInvalidator cacheInvalidator( _pResource ) ;
         rc = cacheInvalidator.notify( COORD_CACHE_CATALOGUE,
                                       pArgs->_targetName.c_str(), cb ) ;
         PD_LOG( PDDEBUG, "Notify other coordinators to invalidate "
                 "catalogue cache when renaming collection[%s]",
                 pArgs->_targetName.c_str() ) ;
         if ( rc )
         {
            // For invalidation error, report warning in the log, but
            // don't interrupt the current operation.
            PD_LOG( PDWARNING, "Notify all coordinators to invalidate cache "
                    "when renaming collection[%s] failed[%d]",
                    pArgs->_targetName.c_str(), rc ) ;
            rc = SDB_OK ;
         }
      }

      _pResource->removeCataInfoWithMain( pArgs->_targetName.c_str() ) ;

      PD_TRACE_EXITRC ( COORD_RENAMECL_DOCOMPLETE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_RENAMECL__NEEDNOTIFYINVALIDCACHE, "_coordCMDRenameCollection::_needNotifyInvalidateCache" )
   BOOLEAN _coordCMDRenameCollection::_needNotifyInvalidateCache( coordCMDArguments *pArgs )
   {
      BOOLEAN result = FALSE ;
      PD_TRACE_ENTRY( COORD_RENAMECL__NEEDNOTIFYINVALIDCACHE ) ;
      // Check the group list returned by catalogue. If any one is a data source
      // group, need to broadcast the invalidation message.
      CoordGroupList::const_iterator itr = pArgs->_groupList.begin() ;
      while ( itr != pArgs->_groupList.end() )
      {
         if ( SDB_IS_DSID( itr->first ) )
         {
            result = TRUE ;
            break ;
         }
         ++itr ;
      }

      PD_TRACE_EXIT( COORD_RENAMECL__NEEDNOTIFYINVALIDCACHE ) ;
      return result ;
   }

   /*
      _coordCMDAlterCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterCollection,
                                      CMD_NAME_ALTER_COLLECTION,
                                      FALSE ) ;
   _coordCMDAlterCollection::_coordCMDAlterCollection ()
   : _coordDataCMDAlter()
   {
   }

   _coordCMDAlterCollection::~_coordCMDAlterCollection ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL_PARSEMSG, "_coordCMDAlterCollection::_parseMsg" )
   INT32 _coordCMDAlterCollection::_parseMsg ( MsgHeader *pMsg,
                                               coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_ALTERCL_PARSEMSG ) ;

      try
      {
         BOOLEAN isOldAlterCMD = FALSE ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get failed[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( pArgs->_boQuery.getField( FIELD_NAME_VERSION ).eoo() )
         {
            isOldAlterCMD = TRUE ;
         }

         PD_LOG ( PDDEBUG, "Alter collection command is %s",
                  isOldAlterCMD ? "old" : "new" ) ;

         if ( isOldAlterCMD )
         {
            /// we only want to update data's catalog version.
            pArgs->_ignoreRCList.insert( SDB_MAIN_CL_OP_ERR ) ;
            pArgs->_ignoreRCList.insert( SDB_CLS_COORD_NODE_CAT_VER_OLD ) ;
         }
         else
         {
            pArgs->_ignoreRCList.insert( SDB_IXM_REDEF ) ;
            pArgs->_ignoreRCList.insert( SDB_IXM_NOTEXIST ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_ALTERCL_PARSEMSG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL_DOCOMPLETE, "_coordCMDAlterCollection::_doComplete" )
   INT32 _coordCMDAlterCollection::_doComplete ( MsgHeader *pMsg,
                                                 pmdEDUCB * cb,
                                                 coordCMDArguments * pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL_DOCOMPLETE ) ;

      const CHAR * collection = pArgs->_targetName.c_str() ;

      rc = _coordDataCMDAlter::_doComplete( pMsg, cb, pArgs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to do complete on collection [%s], "
                   "rc: %d", collection, rc ) ;

      _pResource->removeCataInfo( collection ) ;

   done :
      PD_TRACE_EXITRC( COORD_ALTERCL_DOCOMPLETE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL_DOROLLBACK, "_coordCMDAlterCollection::_doRollback" )
   INT32 _coordCMDAlterCollection::_doRollback ( MsgHeader * pMsg,
                                                 pmdEDUCB * cb,
                                                 rtnContextCoord::sharePtr * ppCoordCtxForCata,
                                                 coordCMDArguments * pArguments,
                                                 CoordGroupList & sucGroupLst,
                                                 INT32 failedRC )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL_DOROLLBACK ) ;

      const CHAR * collection = pArguments->_targetName.c_str() ;

      rc = _coordDataCMDAlter::_doRollback( pMsg, cb, ppCoordCtxForCata,
                                            pArguments, sucGroupLst,
                                            failedRC ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to do rollback on collection [%s], "
                   "rc: %d", collection, rc ) ;

   done :
      // if error happened, invalidate sequence caches if needed
      if ( SDB_DMS_NOTEXIST != failedRC &&
           SDB_DMS_CS_NOTEXIST != failedRC &&
           SDB_SEQUENCE_NOT_EXIST != failedRC )
      {
         const CHAR * collection = _arguments._targetName.c_str() ;
         const rtnAlterTask * taskRunner = _arguments.getTaskRunner() ;
         if ( NULL != taskRunner &&
              taskRunner->testArgumentMask( UTIL_CL_AUTOINCREMENT_FIELD ) )
         {
            INT32 tmpRC = _invalidateSequences( collection, taskRunner, cb ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to invalidate sequences for "
                       "collection [%s], rc: %d", collection, tmpRC ) ;
            }
         }
      }

      _pResource->removeCataInfo( collection ) ;

      PD_TRACE_EXITRC( COORD_ALTERCL_DOROLLBACK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL__EXECPOSTTASKS, "_coordCMDAlterCollection::_executePostTasks" )
   INT32 _coordCMDAlterCollection::_executePostTasks ( const CHAR * name,
                                                       const ossPoolList< UINT64 > & postTasks,
                                                       pmdEDUCB * cb,
                                                       CLS_TASK_TYPE *type )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL__EXECPOSTTASKS ) ;

      BOOLEAN needWait = FALSE ;
      UINT64 taskId = 0 ;
      CHAR * msgBuff = NULL ;
      INT32 msgSize = 0 ;
      MsgHeader * msgHeader = NULL ;
      CLS_TASK_TYPE taskType = CLS_TASK_UNKNOWN ;
      BSONObj taskDesc ;
      BSONObjBuilder builder ;
      BSONObjBuilder taskBuilder( builder.subobjStart( FIELD_NAME_TASKID ) ) ;
      BSONArrayBuilder arrBuilder( taskBuilder.subarrayStart( "$in" ) ) ;

      const ossPoolVector<BSONObj> &taskObj = _arguments.getPostTasksObj() ;

      /// notify all groups to start task.
      for ( ossPoolVector<BSONObj>::const_iterator iterTask = taskObj.begin() ;
            iterTask != taskObj.end() ;
            iterTask ++ )
      {
         BSONElement ele ;
         utilSequenceID seqID = UTIL_SEQUENCEID_NULL ;
         BSONObj delTask ;
         BSONElement group ;
         CoordGroupList groupList ;

         PD_CHECK( iterTask->hasField( FIELD_NAME_TASKTYPE ),
                   SDB_SYS, error, PDERROR,
                   "Faield to get task field[%s]", FIELD_NAME_TASKTYPE);
         ele = iterTask->getField( FIELD_NAME_TASKTYPE ) ;
         PD_CHECK( NumberInt == ele.type(), SDB_SYS, error, PDERROR,
                   "Failed to parse task info [%s]: task type is not a integer",
                   iterTask->toString().c_str() ) ;
         taskType = (CLS_TASK_TYPE)ele.Int() ;
         switch( taskType )
         {
            case CLS_TASK_SPLIT :
            {
               group = iterTask->getField( FIELD_NAME_TARGETID ) ;
               PD_CHECK( NumberInt == group.type(), SDB_SYS, error, PDERROR,
                         "Failed to parse task info [%s]: target id is not a "
                         "integer", iterTask->toString().c_str() ) ;

               groupList[ group.Int() ] = group.Int() ;

               rc = msgBuildQueryMsg( &msgBuff, &msgSize,
                                      CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                      0, 0, 0, -1,
                                      &(*iterTask), NULL, NULL, NULL, cb ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to build split message, rc: %d", rc ) ;

               msgHeader = (MsgHeader *)msgBuff ;
               rc = executeOnCL( msgHeader, cb, name, FALSE, &groupList,
                                 NULL, NULL ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to notify group [%d] to split "
                            "collection [%s], rc: %d", group.Int(), name, rc ) ;
               ele = iterTask->getField( FIELD_NAME_TASKID ) ;
               PD_CHECK( ele.isNumber(), SDB_SYS, error, PDERROR,
                         "Failed to parse task info [%s]: task id is not a "
                         "integer", iterTask->toString().c_str() ) ;
               taskId = ele.Long() ;
               arrBuilder.append( (INT64)( taskId ) ) ;
               needWait = TRUE ;
               break ;
            }
            case CLS_TASK_SEQUENCE :
            {
               const CHAR * seqName = NULL ;
               ele = iterTask->getField( FIELD_NAME_AUTOINC_SEQ ) ;
               PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                         "Failed to parse task[%s]: type of task sequence name "
                         "is not a string", iterTask->toString().c_str() ) ;
               seqName = ele.valuestr() ;
               PD_CHECK( iterTask->hasField( FIELD_NAME_AUTOINC_SEQ_ID ),
                         SDB_SYS, error, PDERROR,
                         "Failed to get field[%s] on task[%s]",
                         iterTask->toString().c_str() ) ;
               ele = iterTask->getField( FIELD_NAME_AUTOINC_SEQ_ID ) ;
               PD_CHECK( ele.isNumber(), SDB_SYS, error, PDERROR,
                         "Failed to parse task[%s]: type of sequence id is not "
                         "a oid", iterTask->toString().c_str() ) ;
               seqID = ele.Long() ;
               rc = coordSequenceInvalidateCache( seqName, seqID, cb );
               break ;
            }
            default :
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Unkown post task type[%s]",
                       iterTask->toString().c_str() ) ;
               break ;
            }
         }
      }
      arrBuilder.done() ;
      taskBuilder.done() ;
      taskDesc = builder.obj() ;

      if( needWait )
      {
         rc = _waitPostTasks( taskDesc, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to wait post tasks, rc: %d", rc ) ;
      }

      *type = taskType ;

   done :
      if ( NULL != msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }

      PD_TRACE_EXITRC( COORD_ALTERCL__EXECPOSTTASKS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL__CANCELPOSTTASKS, "_coordCMDAlterCollection::_cancelPostTasks" )
   INT32 _coordCMDAlterCollection::_cancelPostTasks ( const CHAR * name,
                                                      const ossPoolList< UINT64 > & postTasks,
                                                      pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL__CANCELPOSTTASKS ) ;

      for ( ossPoolList< UINT64 >::const_iterator iterTask = postTasks.begin() ;
            iterTask != postTasks.end() ;
            iterTask ++ )
      {
         _cancelPostTask( (*iterTask), cb ) ;
      }

      PD_TRACE_EXITRC( COORD_ALTERCL__CANCELPOSTTASKS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL__WAITPOSTTASKS, "_coordCMDAlterCollection::_waitPostTasks" )
   INT32 _coordCMDAlterCollection::_waitPostTasks ( const bson::BSONObj & taskDesc,
                                                    pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL__WAITPOSTTASKS ) ;

      CHAR * msgBuff = NULL ;
      INT32 msgSize = 0 ;
      coordCommandFactory * factory = NULL ;
      coordOperator * coordOperator = NULL ;

      INT64 contextID = -1 ;
      rtnContextBuf buf ;

      rc = msgBuildQueryMsg( &msgBuff, &msgSize,
                             CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                             0, 0, 0, -1,
                             &taskDesc, NULL, NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build query task, rc: %d", rc ) ;

      factory = coordGetFactory() ;
      rc = factory->create( CMD_NAME_WAITTASK, coordOperator ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to create operator [%s], rc: %d",
                   CMD_NAME_WAITTASK, rc ) ;

      rc = coordOperator->init( _pResource, cb, getTimeout() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init operator [%s], rc: %d",
                   CMD_NAME_WAITTASK, rc ) ;

      rc = coordOperator->execute( (MsgHeader *)msgBuff, cb, contextID, &buf ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to execute operator [%s], rc: %d",
                   CMD_NAME_WAITTASK, rc ) ;

   done :
      if ( NULL != msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      if ( NULL != coordOperator )
      {
         factory->release( coordOperator ) ;
      }
      PD_TRACE_EXITRC( COORD_ALTERCL__WAITPOSTTASKS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL__CANCELPOSTTASK, "_coordCMDAlterCollection::_cancelPostTask" )
   INT32 _coordCMDAlterCollection::_cancelPostTask ( UINT64 taskID,
                                                     pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL__CANCELPOSTTASK ) ;

      CHAR * msgBuff = NULL ;
      INT32 msgSize = 0 ;
      MsgHeader * rollbackMsg = NULL ;
      rtnContextBuf buff ;

      try
      {
         BSONObj taskDesc = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
         rc = msgBuildQueryMsg( &msgBuff, &msgSize, "",
                                0, 0, 0, -1,
                                &taskDesc, NULL, NULL, NULL, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build message, rc: %d", rc ) ;
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rollbackMsg = (MsgHeader *)msgBuff ;
      rollbackMsg->opCode = MSG_CAT_TASK_CANCEL_REQ ;

      rc = executeOnCataGroup( rollbackMsg, cb, TRUE, NULL, NULL, &buff ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to cancel on catalog, rc: %d", rc ) ;

   done :
      if ( msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_ALTERCL__CANCELPOSTTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL__INVALIDATESEQ_TASK, "_coordCMDAlterCollection::_invalidateSequences" )
   INT32 _coordCMDAlterCollection::_invalidateSequences ( const CHAR * collection,
                                                          const rtnAlterTask * task,
                                                          pmdEDUCB * cb  )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL__INVALIDATESEQ_TASK ) ;

      SDB_ASSERT( NULL != task &&
                  task->testArgumentMask( UTIL_CL_AUTOINCREMENT_FIELD ),
                  "task runner is invalid" ) ;

      switch ( task->getActionType() )
      {
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            const _rtnCLSetAttributeTask * alterTask =
                  dynamic_cast< const _rtnCLSetAttributeTask * >( task ) ;
            SDB_ASSERT( NULL != task, "task is invalid" ) ;
            const autoIncFieldsList & autoIncList =
                                    alterTask->getAutoincFieldArgument() ;
            rc = _invalidateSequences( collection, autoIncList, cb ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         {
            const _rtnCLCreateAutoincFieldTask * createTask =
                  dynamic_cast< const _rtnCLCreateAutoincFieldTask * >( task ) ;
            SDB_ASSERT( NULL != task, "task is invalid" ) ;
            const autoIncFieldsList & autoIncList =
                                 createTask->getAutoincrementArgument() ;
            rc = _invalidateSequences( collection, autoIncList, cb ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            const rtnCLDropAutoincFieldTask * dropTask =
                  dynamic_cast< const _rtnCLDropAutoincFieldTask * >( task ) ;
            SDB_ASSERT( NULL != task, "task is invalid" ) ;
            const autoIncFieldsList & autoIncList =
                                 dropTask->getAutoincrementArgument() ;
            rc = _invalidateSequences( collection, autoIncList, cb ) ;
            break ;
         }
         default :
            break ;
      }

      PD_RC_CHECK( rc, PDWARNING, "Failed to invalid sequences for "
                   "collection [%s], rc: %d", collection, rc ) ;

   done :
      PD_TRACE_EXITRC( COORD_ALTERCL__INVALIDATESEQ_TASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERCL__INVALIDATESEQ_LIST, "_coordCMDAlterCollection::_invalidateSequences" )
   INT32 _coordCMDAlterCollection::_invalidateSequences (
                                       const CHAR * collection,
                                       const autoIncFieldsList & autoIncList,
                                       pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( COORD_ALTERCL__INVALIDATESEQ_LIST ) ;

      CoordCataInfoPtr cataPtr = getCataPtr() ;
      if ( NULL == cataPtr.get() )
      {
         // catalog info is empty, try to fetch
         // no need to get the latest, we might with problems of catalog
         _pResource->getCataInfo( collection, cataPtr ) ;
      }

      for ( autoIncFieldsList::const_iterator iter = autoIncList.begin() ;
            iter != autoIncList.end() ;
            ++ iter )
      {
         INT32 tmpRC = SDB_OK ;
         const CHAR * fieldName = NULL ;
         const CHAR * sequenceName = NULL ;
         utilSequenceID sequenceID = UTIL_SEQUENCEID_NULL ;

         rtnCLAutoincFieldArgument * argument = (*iter) ;
         SDB_ASSERT( NULL != argument, "argument is invalid" ) ;

         fieldName = argument->getFieldName() ;

         if ( NULL != cataPtr.get() )
         {
            const clsAutoIncSet & autoIncSet = cataPtr->getAutoIncSet() ;
            const clsAutoIncItem * autoIncItem = autoIncSet.find( fieldName ) ;
            if ( NULL != autoIncItem )
            {
               sequenceName = autoIncItem->sequenceName() ;
               sequenceID = autoIncItem->sequenceID() ;
            }
         }

         if ( NULL != sequenceName )
         {
            tmpRC = coordSequenceInvalidateCache( sequenceName, sequenceID,
                                                  cb ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to call invalidate sequence "
                       "cache [%s] of collection [%s], rc: %d",
                       sequenceName, collection, tmpRC ) ;
               if ( SDB_OK == rc )
               {
                  rc = tmpRC ;
               }
            }
            else
            {
               PD_LOG( PDDEBUG, "Call invalidate sequence cache [%s] of "
                       "collection [%s] done", sequenceName, collection ) ;
            }
         }
         else if ( NULL != fieldName )
         {
            tmpRC = coordSequenceInvalidateCache( collection, fieldName, cb ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to call invalidate sequence cache "
                       "for field [%s] of collection [%s], rc: %d",
                       fieldName, collection, tmpRC ) ;
               if ( SDB_OK == rc )
               {
                  rc = tmpRC ;
               }
            }
            else
            {
               PD_LOG( PDDEBUG, "Call invalidate sequence cache for field [%s] "
                       "of collection [%s] done", fieldName, collection ) ;
            }
         }
         else
         {
            PD_LOG( PDWARNING, "Failed to get sequence name for field [%s] of "
                    "collection [%s]", fieldName, collection ) ;
            continue ;
         }
      }

      PD_TRACE_EXITRC( COORD_ALTERCL__INVALIDATESEQ_LIST, rc ) ;

      return rc ;
   }

   /*
      _coordCMDLinkCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDLinkCollection,
                                      CMD_NAME_LINK_CL,
                                      FALSE ) ;
   _coordCMDLinkCollection::_coordCMDLinkCollection()
   {
   }

   _coordCMDLinkCollection::~_coordCMDLinkCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LINKCL_PARSEMSG, "_coordCMDLinkCollection::_parseMsg" )
   INT32 _coordCMDLinkCollection::_parseMsg ( MsgHeader *pMsg,
                                              coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_LINKCL_PARSEMSG ) ;

      try
      {
         BSONObj lowBound, upBound ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_SUBCL_NAME,
                                      _subCLName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_SUBCL_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( _subCLName.empty() )
         {
            PD_LOG( PDERROR, "Sub collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetObjElement( pArgs->_boQuery, CAT_LOWBOUND_NAME,
                                lowBound ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_LOWBOUND_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetObjElement( pArgs->_boQuery, CAT_UPBOUND_NAME,
                                upBound ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_UPBOUND_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_LINKCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }


   INT32 _coordCMDLinkCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     coordCMDArguments *pArgs,
                                                     CHAR **ppMsgBuf,
                                                     INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_LINK_CL_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LINKCL_GENROLLBACKMSG, "_coordCMDLinkCollection::_generateRollbackDataMsg" )
   INT32 _coordCMDLinkCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                             pmdEDUCB *cb,
                                                             coordCMDArguments *pArgs,
                                                             CHAR **ppMsgBuf,
                                                             INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_LINKCL_GENROLLBACKMSG ) ;

      rc = msgBuildUnlinkCLMsg( ppMsgBuf, pBufSize,
                                pArgs->_targetName.c_str(),
                                _subCLName.c_str(), 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message failed on command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_LINKCL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDLinkCollection::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                          INT32 bufSize,
                                                          pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LINKCL_ROLLBACKONDATA, "_coordCMDLinkCollection::_rollbackOnDataGroup" )
   INT32 _coordCMDLinkCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         coordCMDArguments *pArgs,
                                                         const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_LINKCL_ROLLBACKONDATA ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &(pArgs->_ignoreRCList), NULL,
                        NULL, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback command[%s, target:%s, sub:%s] on data "
                 "group failed, rc: %d", getName(),
                 pArgs->_targetName.c_str(), _subCLName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_LINKCL_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDUnlinkCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDUnlinkCollection,
                                      CMD_NAME_UNLINK_CL,
                                      FALSE ) ;
   _coordCMDUnlinkCollection::_coordCMDUnlinkCollection()
   {
   }

   _coordCMDUnlinkCollection::~_coordCMDUnlinkCollection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL_PARSEMSG, "_coordCMDUnlinkCollection::_parseMsg" )
   INT32 _coordCMDUnlinkCollection::_parseMsg ( MsgHeader *pMsg,
                                                coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UNLINKCL_PARSEMSG ) ;

      try
      {
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_SUBCL_NAME,
                                      _subCLName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_SUBCL_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( _subCLName.empty() )
         {
            PD_LOG( PDERROR, "Sub collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_COLLECTION_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pArgs->_targetName.empty() )
         {
            PD_LOG( PDERROR, "Collection name is empty in command[%s]",
                    getName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_UNLINKCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDUnlinkCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       coordCMDArguments *pArgs,
                                                       CHAR **ppMsgBuf,
                                                       INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_UNLINK_CL_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL_GENROLLBACKMSG, "_coordCMDUnlinkCollection::_generateRollbackDataMsg" )
   INT32 _coordCMDUnlinkCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                               pmdEDUCB *cb,
                                                               coordCMDArguments *pArgs,
                                                               CHAR **ppMsgBuf,
                                                               INT32 *pBufSize  )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UNLINKCL_GENROLLBACKMSG ) ;

      // The Data Group doesn't care about lowBound and upBound
      rc = msgBuildLinkCLMsg( ppMsgBuf, pBufSize,
                              pArgs->_targetName.c_str(),
                              _subCLName.c_str(),
                              NULL, NULL, 0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build rollback message on "
                 "command[%s, target:%s, sub:%s] failed, rc: %d",
                 getName(), pArgs->_targetName.c_str(),
                 _subCLName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_UNLINKCL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDUnlinkCollection::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                            INT32 bufSize,
                                                            pmdEDUCB *cb )
   {
      if ( pMsgBuf )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL_ROLLBACKONDATA, "_coordCMDUnlinkCollection::_rollbackOnDataGroup" )
   INT32 _coordCMDUnlinkCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                           pmdEDUCB *cb,
                                                           coordCMDArguments *pArgs,
                                                           const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UNLINKCL_ROLLBACKONDATA ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &(pArgs->_ignoreRCList), NULL,
                        NULL, pArgs->_pBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rollback command[%s, target:%s, sub:%s] on data "
                 "group failed, rc: %d", getName(),
                 pArgs->_targetName.c_str(),
                 _subCLName.c_str(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_UNLINKCL_ROLLBACKONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL__DOCOMPLETE, "_coordCMDUnlinkCollection::_doComplete" )
   INT32 _coordCMDUnlinkCollection::_doComplete ( MsgHeader *pMsg,
                                                  pmdEDUCB * cb,
                                                  coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_UNLINKCL__DOCOMPLETE ) ;
      ossPoolVector<string> collections ;

      // When unlinking a collection which is using data source, need to notify
      // other coordinators to invalidate the caches of the main and sub
      // collection. Otherwise, user may still access the sub collection through
      // the main collection on other coordinators, as the cache of the main
      // collection is old.
      if ( _needNotifyInvalidateCache( pArgs ) )
      {
         coordCacheInvalidator cacheInvalidator( _pResource ) ;
         try
         {
            collections.push_back( pArgs->_targetName ) ;
            collections.push_back( _subCLName ) ;
            rc = cacheInvalidator.notify( COORD_CACHE_CATALOGUE,
                                          collections, cb ) ;
            if ( rc )
            {
               // For invalidation error, report warning in the log, but
               // don't interrupt the current operation.
               PD_LOG( PDWARNING, "Notify all coordinators to invalidate cache "
                       "when unlinking sub collection[%s] failed[%d]",
                       _subCLName.c_str(), rc ) ;
               rc = SDB_OK ;
            }
         }
         catch ( std::exception &e )
         {
            rc= ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_UNLINKCL__DOCOMPLETE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_UNLINKCL__NEEDNOTIFYINVALIDCACHE, "_coordCMDUnlinkCollection::_needNotifyInvalidateCache" )
   BOOLEAN _coordCMDUnlinkCollection::_needNotifyInvalidateCache( coordCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY( COORD_UNLINKCL__NEEDNOTIFYINVALIDCACHE ) ;
      BOOLEAN result = FALSE ;
      // Check the group list returned by catalogue. If any one is a data source
      // group, need to broadcast the invalidation message.
      CoordGroupList::const_iterator itr = pArgs->_groupList.begin() ;
      while ( itr != pArgs->_groupList.end() )
      {
         if ( SDB_IS_DSID( itr->first ) )
         {
            result = TRUE ;
            break ;
         }
         ++itr ;
      }

      PD_TRACE_EXIT( COORD_UNLINKCL__NEEDNOTIFYINVALIDCACHE ) ;
      return result ;
   }

   /*
      _coordCMDSplit implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSplit,
                                      CMD_NAME_SPLIT,
                                      FALSE ) ;
   _coordCMDSplit::_coordCMDSplit()
   {
      _async = FALSE ;
      _percent = 0.0 ;
   }

   _coordCMDSplit::~_coordCMDSplit()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETCOUNT, "_coordCMDSplit::_getCLCount" )
   INT32 _coordCMDSplit::_getCLCount( const CHAR *clFullName,
                                      const CoordGroupList &groupList,
                                      pmdEDUCB *cb,
                                      UINT64 &count,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_SPLIT_GETCOUNT ) ;

      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      rtnContextCoord::sharePtr pContext ;
      BSONObj collectionObj ;
      BSONObj dummy ;
      rtnContextBuf buffObj ;

      CHAR *pMsg = NULL ;
      INT32 bufSize = 0 ;

      count = 0 ;

      collectionObj = BSON( FIELD_NAME_COLLECTION << clFullName ) ;
      rtnQueryOptions queryOption( dummy, dummy, dummy, collectionObj,
                                   CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                                   0, 1, FLG_QUERY_WITH_RETURNDATA ) ;

      rc = queryOption.toQueryMsg( &pMsg, bufSize, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build get count message failed in command[%s], "
                 "rc: %d", getName(), rc ) ;
         goto error ;
      }

      rc = queryOnCL( (MsgHeader*)pMsg, cb, clFullName, &pContext,
                      FALSE, &groupList, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get count from source node failed, collection:%s, "
                 "rc: %d", clFullName, rc ) ;
         goto error ;
      }

      rc = pContext->getMore( -1, buffObj, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get more from context[%lld] failed, rc: %d",
                 pContext->contextID(), rc ) ;
         goto error ;
      }
      else
      {
         // get count data
         BSONObj countObj ( buffObj.data() ) ;
         BSONElement beTotal = countObj.getField( FIELD_NAME_TOTAL ) ;
         if ( !beTotal.isNumber() )
         {
            PD_LOG( PDERROR, "Parse get count result[%s] failed, "
                    "the field[%s] is not number",
                    countObj.toString().c_str(), FIELD_NAME_TOTAL ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         count = beTotal.numberLong() ;
      }

   done :
      if ( pContext )
      {
         SINT64 contextID = pContext->contextID() ;
         pRtncb->contextDelete ( contextID, cb ) ;
      }
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC ( COORD_SPLIT_GETCOUNT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitPrepare( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pQuery = NULL ;
      CoordGroupList srcGrpLst ;

      // first round we perform prepare, so catalog node is able to do sanity
      // check for collection name and nodes
      MsgOpQuery *pSplitReq            = (MsgOpQuery *)pMsg ;
      pSplitReq->header.opCode         = MSG_CAT_SPLIT_PREPARE_REQ ;

      rc = executeOnCataGroup ( pMsg, cb, &srcGrpLst, NULL, TRUE,
                                NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Split prepare on catalog failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = msgExtractQuery ( (const CHAR*)pMsg, NULL, NULL,
                             NULL, NULL, &pQuery,
                             NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract split message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj obj( pQuery ) ;
         rc = _splitParamCheck( obj, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _makeSplitRange( cb, srcGrpLst, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitParamCheck( const BSONObj &obj,
                                           pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      BSONElement ele ;

      ele = obj.getField ( CAT_COLLECTION_NAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Collection name is invalid in object[%s]",
                 obj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _clName = ele.valuestr() ;

      _eleSplitQuery = obj.getField ( CAT_SPLITQUERY_NAME ) ;

      /// source group
      ele = obj.getField ( CAT_SOURCE_NAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                 CAT_SOURCE_NAME, obj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = catGroupNameValidate ( ele.valuestr() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Source group name[%s] is invalid, rc: %d",
                 ele.valuestr(), rc ) ;
         goto error ;
      }
      _srcGroup = ele.valuestr() ;

      /// target group
      ele = obj.getField ( CAT_TARGET_NAME ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                 CAT_TARGET_NAME, obj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = catGroupNameValidate ( ele.valuestr() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Target group name[%s] is invalid, rc: %d",
                 ele.valuestr(), rc ) ;
         goto error ;
      }
      _dstGroup = ele.valuestr() ;

      /// async
      ele = obj.getField ( FIELD_NAME_ASYNC ) ;
      if ( Bool == ele.type() )
      {
         _async = ele.Bool() ? TRUE : FALSE ;
      }
      else if ( !ele.eoo() )
      {
         PD_LOG( PDERROR, "Field[%s] type[%d] error", FIELD_NAME_ASYNC,
                 ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// percent
      ele = obj.getField( CAT_SPLITPERCENT_NAME ) ;
      _percent = ele.numberDouble() ;

      // make sure we have either split value or split query
      if ( !_eleSplitQuery.eoo() )
      {
         if ( Object != _eleSplitQuery.type() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                    CAT_SPLITQUERY_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _eleSplitEndQuery = obj.getField ( CAT_SPLITENDQUERY_NAME ) ;
         if ( !_eleSplitEndQuery.eoo() )
         {
            if ( Object != _eleSplitQuery.type() )
            {
               PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                       CAT_SPLITENDQUERY_NAME, obj.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      else
      {
         if ( _percent <= 0.0 || _percent > 100.0 )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in object[%s]",
                    CAT_SPLITPERCENT_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      rc = _cataSel.bind( _pResource, _clName.c_str(), cb, TRUE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", _clName.c_str(), rc ) ;
         goto error ;
      }

      _cataSel.getCataPtr()->getShardingKey( _shardkingKey ) ;
      if ( !_cataSel.getCataPtr()->isSharded() ||
           _shardkingKey.isEmpty() )
      {
         PD_LOG( PDERROR, "Collection[%s] is not shared", _clName.c_str() ) ;
         rc = SDB_COLLECTION_NOTSHARD ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_makeSplitRange( pmdEDUCB *cb,
                                          const CoordGroupList &srcGrpLst,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      clsCatalogSet *pSet = _cataSel.getCataPtr()->getCatalogSet() ;

      /// hash sharding
      if ( pSet->isHashSharding() )
      {
         if ( !_eleSplitQuery.eoo() )
         {
            BSONObj tmpStart = _eleSplitQuery.embeddedObject() ;
            BSONObjBuilder tmpStartBuilder ;
            tmpStartBuilder.appendElementsWithoutName( tmpStart ) ;
            _lowBound = tmpStartBuilder.obj() ;

            if ( !_eleSplitEndQuery.eoo() )
            {
               BSONObj tmpEnd = _eleSplitEndQuery.embeddedObject() ;
               BSONObjBuilder tmpEndBuilder ;
               tmpEndBuilder.appendElementsWithoutName( tmpEnd ) ;
               _upBound = tmpEndBuilder.obj() ;
            }
         }
      }
      else
      {
         if ( _eleSplitQuery.eoo() )
         {
            rc = _getBoundByPercent( _clName.c_str(), _percent,
                                     _cataSel.getCataPtr(), srcGrpLst,
                                     cb, _lowBound, _upBound,
                                     buf ) ;
         }
         else
         {
            rc = _getBoundByCondition( _clName.c_str(),
                                       _eleSplitQuery.embeddedObject(),
                                       _eleSplitEndQuery.eoo() ?
                                       BSONObj() :
                                       _eleSplitEndQuery.embeddedObject(),
                                       srcGrpLst, cb,
                                       _lowBound, _upBound,
                                       buf ) ;
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Make bound failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitReady( pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf,
                                      UINT64 &taskID,
                                      BSONObj &taskInfoObj )
   {
      INT32 rc = SDB_OK ;
      CHAR *pReadyMsg = NULL ;
      INT32 readyMsgSize = 0 ;
      MsgOpQuery *pSplitReadyMsg = NULL ;
      vector< BSONObj > vecObjs ;
      CoordGroupList dstGrpLst ;

      try
      {
         BSONObj boSend ;
         // construct the record that we are going to send to catalog
         boSend = BSON ( CAT_COLLECTION_NAME << _clName<<
                         CAT_SOURCE_NAME << _srcGroup <<
                         CAT_TARGET_NAME << _dstGroup <<
                         CAT_SPLITPERCENT_NAME << _percent <<
                         CAT_SPLITVALUE_NAME << _lowBound <<
                         CAT_SPLITENDVALUE_NAME << _upBound ) ;
         taskInfoObj = boSend ;

         rc = msgBuildQueryMsg( &pReadyMsg, &readyMsgSize,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                0, 0, 0, -1,
                                &boSend, NULL, NULL, NULL,
                                cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build split ready message failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pSplitReadyMsg = (MsgOpQuery *)pReadyMsg ;
      pSplitReadyMsg->header.opCode = MSG_CAT_SPLIT_READY_REQ ;
      pSplitReadyMsg->version = _cataSel.getCataPtr()->getVersion() ;

      rc = executeOnCataGroup ( (MsgHeader*)pReadyMsg, cb, &dstGrpLst,
                                &vecObjs, TRUE, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute split ready on catalog failed, rc: %d",
                 rc ) ;
         goto error ;
      }

      // Get task ID
      if ( vecObjs.empty() )
      {
         PD_LOG( PDERROR, "Failed to get task id from result msg" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         BSONObj resultObj = vecObjs[0] ;
         BSONElement ele = resultObj.getField( CAT_TASKID_NAME ) ;
         if ( !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Get taskid from split result object[%s] "
                    "failed", resultObj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         taskID = ele.numberLong() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc =  SDB_SYS ;
         goto error ;
      }

      /// notify to data
      pSplitReadyMsg->header.opCode = MSG_BS_QUERY_REQ ;
      pSplitReadyMsg->version = _cataSel.getCataPtr()->getVersion() ;

      rc = executeOnCL( (MsgHeader *)pSplitReadyMsg, cb, _clName.c_str(),
                        FALSE, &dstGrpLst, NULL, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Execute split on data node failed, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      if ( pReadyMsg )
      {
         msgReleaseBuffer( pReadyMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitRollback( UINT64 taskID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      MsgOpQuery *pRollbackMsg = NULL ;
      rtnContextBuf buff ;

      try
      {
         BSONObj boSend = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
         rc = msgBuildQueryMsg( &pMsg, &msgSize,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                0, 0, 0, -1,
                                &boSend, NULL, NULL, NULL,
                                cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build split notify message failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pRollbackMsg = ( MsgOpQuery* )pMsg ;
      pRollbackMsg->header.opCode = MSG_CAT_TASK_CANCEL_REQ ;

      rc = executeOnCataGroup ( (MsgHeader*)pRollbackMsg, cb, TRUE,
                                NULL, NULL, &buff ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Execute split cancel on catalog failed, "
                 "rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDSplit::_splitPost( UINT64 taskID,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      coordCommandFactory *pFactory = NULL ;
      coordOperator *pOperator = NULL ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      // if sync, need to wait task finished
      if ( !_async )
      {
         try
         {
            BSONObj boSend = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
            rc = msgBuildQueryMsg( &pMsg, &msgSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                   0, 0, 0, -1,
                                   &boSend, NULL, NULL, NULL,
                                   cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Build split notify message failed, rc: %d",
                       rc ) ;
               goto error ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         pFactory = coordGetFactory() ;
         rc = pFactory->create( CMD_NAME_WAITTASK, pOperator ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Create operator[%s] failed, rc: %d",
                    CMD_NAME_WAITTASK, rc ) ;
            goto error ;
         }
         rc = pOperator->init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    pOperator->getName(), rc ) ;
            goto error ;
         }
         rc = pOperator->execute( (MsgHeader *)pMsg, cb,
                                  contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Wait task[%lld] failed, rc: %d",
                    taskID, rc ) ;
            goto error ;
         }
      }
      else // return taskid to client
      {
         rtnContextDump::sharePtr pContext ;
         rc = rtnCB->contextNew( RTN_CONTEXT_DUMP,
                                 pContext,
                                 contextID, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = pContext->open( BSONObj(), BSONObj(), 1, 0 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
            goto error ;
         }
         pContext->append( BSON( CAT_TASKID_NAME << (long long)taskID ) ) ;
      }

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_EXE, "_coordCMDSplit::execute" )
   INT32 _coordCMDSplit::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_SPLIT_EXE ) ;
      coordSessionPropSite *pSiteProp  = NULL ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      BSONObj taskInfoObj ;
      rtnInstanceOption instanceOption ;
      BOOLEAN replacedInstanceOption = FALSE ;

      contextID = -1 ;

      /// in transaction, can't do split
      if ( cb->isTransaction() )
      {
         rc = SDB_OPERATION_INCOMPATIBLE ;
         PD_LOG_MSG( PDERROR, "Can't do split in transaction" ) ;
         goto error ;
      }

      if ( cb->getRemoteSite() )
      {
         pmdRemoteSessionSite *pSite = NULL ;
         pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
         pSiteProp = (coordSessionPropSite*)pSite->getUserData() ;
      }

      if ( NULL != pSiteProp && !pSiteProp->isMasterRequired() )
      {
         instanceOption = pSiteProp->getInstanceOption() ;
         pSiteProp->setMasterRequired() ;
         replacedInstanceOption = TRUE ;
      }

      /// prepare
      rc = _splitPrepare( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

      /// ready
      rc = _splitReady( cb, contextID, buf, taskID, taskInfoObj ) ;
      if ( rc )
      {
         if ( CLS_INVALID_TASKID != taskID )
         {
            /// rollback
            _splitRollback( taskID, cb ) ;
         }
         goto error ;
      }

      /// wait
      rc = _splitPost( taskID, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      /// restore
      if ( NULL != pSiteProp && replacedInstanceOption )
      {
         pSiteProp->setInstanceOption( instanceOption ) ;
      }
      if ( !_clName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_CL,
                           _clName.c_str(), rc, "Option:%s, TaskID:%llu",
                           taskInfoObj.toString().c_str(), taskID ) ;
      }
      PD_TRACE_EXITRC ( COORD_SPLIT_EXE, rc ) ;
      return rc ;
   error :
      if ( SDB_RTN_INVALID_HINT == rc )
      {
         rc = SDB_COORD_SPLIT_NO_SHDIDX ;
      }
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETBOUNDRECORDONDATA, "_coordCMDSplit::_getBoundRecordOnData" )
   INT32 _coordCMDSplit::_getBoundRecordOnData( const CHAR *cl,
                                                const BSONObj &condition,
                                                const BSONObj &hint,
                                                const BSONObj &sort,
                                                INT32 flag,
                                                INT64 skip,
                                                const CoordGroupList &groupList,
                                                pmdEDUCB *cb,
                                                const BSONObj &shardingKey,
                                                BSONObj &record,
                                                rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( COORD_SPLIT_GETBOUNDRECORDONDATA ) ;
      INT32 rc = SDB_OK ;
      rtnContextCoord::sharePtr pContext ;
      BSONObj obj ;

      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;

      // check condition has invalid fileds
      if ( !condition.okForStorage() )
      {
         PD_LOG( PDERROR, "Condition[%s] has invalid field name",
                 condition.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( condition.isEmpty() )
      {
         BSONObj empty ;
         rtnContextBuf buffObj ;
         rtnQueryOptions queryOption( condition, empty, sort, hint, cl,
                                      skip, 1, flag ) ;
         rc = queryOption.toQueryMsg( &pMsg, msgSize, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build query message failed, rc: %d", rc ) ;
            goto error ;
         }
         rc = queryOnCL( (MsgHeader*)pMsg, cb, NULL, &pContext,
                         FALSE, &groupList, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query on data group failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = pContext->getMore( -1, buffObj, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         else
         {
            obj = BSONObj( buffObj.data() ) ;
         }
      }
      else
      {
         obj = condition ;
      }

      // product split key
      {
         PD_LOG( PDINFO, "Split found record: %s", obj.toString().c_str() ) ;

         // we need to compare with boShardingKey and extract the partition key
         ixmIndexKeyGen keyGen ( shardingKey ) ;
         BSONObjSet keys ;
         BSONObjSet::iterator keyIter ;
         rc = keyGen.getKeys ( obj, keys, NULL, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Gen key failed, rc: %d" OSS_NEWLINE 
                    "record: %s" OSS_NEWLINE "keyDef: %s", rc,
                    obj.toString().c_str(),
                    shardingKey.toString().c_str() ) ;
            goto error ;
         }
         if ( 1 != keys.size() )
         {
            PD_LOG( PDERROR, "There must be a single key generate for "
                    "sharding" OSS_NEWLINE "record: %s" OSS_NEWLINE "keyDef: %s",
                    obj.toString().c_str(),
                    shardingKey.toString().c_str() ) ;
            rc = SDB_INVALID_SHARDINGKEY ;
            goto error ;
         }

         keyIter = keys.begin() ;
         record = (*keyIter).getOwned() ;

         // validate key does not contains Undefined
         /*
         {
            BSONObjIterator iter ( record ) ;
            while ( iter.more () )
            {
               BSONElement e = iter.next () ;
               PD_CHECK ( e.type() != Undefined, SDB_CLS_BAD_SPLIT_KEY,
                          error, PDERROR, "The split record does not contains "
                          "a valid key\nRecord: %s\nShardingKey: %s\n"
                          "SplitKey: %s", obj.toString().c_str(),
                          shardingKey.toString().c_str(),
                          record.toString().c_str() ) ;
            }
         }
         */

        PD_LOG ( PDINFO, "Split found key: %s", record.toString().c_str() ) ;
     }

   done:
      if ( pContext )
      {
         SINT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
      }
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_SPLIT_GETBOUNDRECORDONDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETBOUNDBYCOND, "_coordCMDSplit::_getBoundByCondition" )
   INT32 _coordCMDSplit::_getBoundByCondition( const CHAR *cl,
                                               const BSONObj &begin,
                                               const BSONObj &end,
                                               const CoordGroupList &groupList,
                                               pmdEDUCB *cb,
                                               BSONObj &lowBound,
                                               BSONObj &upBound,
                                               rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( COORD_SPLIT_GETBOUNDBYCOND ) ;
      INT32 rc = SDB_OK ;

      rc = _getBoundRecordOnData( cl, begin, BSONObj(), BSONObj(),
                                  FLG_QUERY_WITH_RETURNDATA, 0,
                                  groupList, cb, _shardkingKey,
                                  lowBound, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get lowbound failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( !end.isEmpty() )
      {
         rc = _getBoundRecordOnData( cl, end, BSONObj(),BSONObj(),
                                     FLG_QUERY_WITH_RETURNDATA, 0,
                                     groupList, cb, _shardkingKey,
                                     upBound, buf ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Get upbound failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_SPLIT_GETBOUNDBYCOND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SPLIT_GETBOUNDBYPERCENT, "_coordCMDSplit::_getBoundByPercent" )
   INT32 _coordCMDSplit::_getBoundByPercent( const CHAR *cl,
                                             FLOAT64 percent,
                                             const CoordCataInfoPtr &cataInfo,
                                             const CoordGroupList &groupList,
                                             pmdEDUCB *cb,
                                             BSONObj &lowBound,
                                             BSONObj &upBound,
                                             rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( COORD_SPLIT_GETBOUNDBYPERCENT ) ;
      INT32 rc = SDB_OK ;

      // if split percent is 100.0%, get the group low bound
      if ( 100.0 - percent < OSS_EPSILON )
      {
         rc = cataInfo->getGroupLowBound( groupList.begin()->second,
                                          lowBound ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get group[%d]'s low bound failed, rc: %d",
                    groupList.begin()->second, rc ) ;
            goto error ;
         }
      }
      else
      {
         UINT64 totalCount    = 0 ;
         INT64 skipCount      = 0 ;
         INT32 flag = FLG_QUERY_FORCE_IDX_BY_SORT | FLG_QUERY_WITH_RETURNDATA ;
         BSONObj hint = BSON( "" << "" ) ;

         while ( TRUE )
         {
            rc = _getCLCount( cl, groupList, cb, totalCount, buf ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get count from collection[%s] failed, rc: %d",
                       cl, rc ) ;
               goto error ;
            }

            if ( 0 == totalCount )
            {
               rc = SDB_DMS_EMPTY_COLLECTION ;
               PD_LOG( PDDEBUG, "Collection[%s] is empty", cl ) ;
               break ;
            }

            skipCount = (INT64)(totalCount * ( ( 100 - percent ) / 100 ) ) ;

            /// sort by shardingKey that if $shard index does not exist
            /// can still match index.
            rc = _getBoundRecordOnData( cl, BSONObj(), hint, _shardkingKey,
                                        flag, skipCount, groupList,
                                        cb, _shardkingKey, lowBound, buf ) ;
            if ( SDB_DMS_EOC == rc )
            {
               continue ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get bound from data, rc: %d",
                       rc ) ;
               goto error ;
            }
            else
            {
               break ;
            }
         }
      }

      /// upbound always be empty.
      upBound = BSONObj() ;
   done:
      PD_TRACE_EXITRC( COORD_SPLIT_GETBOUNDBYPERCENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDIndexHelper::_buildErrInfo( rtnContextBuf *buf,
                                              BSONArrayBuilder &arrayBD )
   {
      INT32 rc = SDB_OK ;
      BSONObj bufObj, errNodeArr ;

      /*{ "errno": -129,
          "description": "Full sync is in progress",
          "detail": "",
          "ErrNodes": [ { "NodeName": "ubuntu1604:20000",
                          "GroupName": "db1",
                          "Flag": -129,
                          "ErrInfo": { "errno": -129,
                                       "description": "Fullsync is in progress",
                                       "detail": "xxx" } },
                        ... ] }
        ===>
        [ { "GroupName": "db1",
            "ResultCode": -129,
            "Detail": "xxx" },
          ... ]
      */

      try
      {
         if ( buf && buf->data() )
         {
            bufObj = BSONObj( buf->data() ) ;
         }
         if ( bufObj.isEmpty() )
         {
            goto done ;
         }

         rc = rtnGetArrayElement( bufObj, FIELD_NAME_ERROR_NODES, errNodeArr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s], rc: %d",
                      FIELD_NAME_ERROR_NODES, bufObj.toString().c_str(), rc ) ;

         BSONObjIterator iter( errNodeArr ) ;
         while ( iter.more() )
         {
            INT32 resultCode = SDB_OK ;
            const CHAR* groupName = NULL ;
            const CHAR* detail = NULL ;
            BSONObj infoObj ;
            BSONObjBuilder objBD( arrayBD.subobjStart() ) ; ;

            BSONElement ele = iter.next() ;
            PD_CHECK( ele.type() == Object, SDB_SYS, error,
                      PDERROR, "Invalid element type[%d]", ele.type() ) ;

            BSONObj errNodeObj = ele.embeddedObject() ;

            rc = rtnGetStringElement( errNodeObj, FIELD_NAME_GROUPNAME,
                                      &groupName ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_GROUPNAME, errNodeObj.toString().c_str(), rc ) ;

            rc = rtnGetIntElement( errNodeObj, FIELD_NAME_RCFLAG, resultCode ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_RCFLAG, errNodeObj.toString().c_str(), rc ) ;

            rc = rtnGetObjElement( errNodeObj, FIELD_NAME_ERROR_INFO, infoObj ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_ERROR_INFO, errNodeObj.toString().c_str(), rc ) ;

            rc = rtnGetStringElement( infoObj, OP_ERR_DETAIL, &detail ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         OP_ERR_DETAIL, infoObj.toString().c_str(), rc ) ;

            objBD.append( FIELD_NAME_GROUPNAME, groupName ) ;
            objBD.append( FIELD_NAME_RESULTCODE, resultCode ) ;
            objBD.append( FIELD_NAME_DETAIL, detail ) ;
            objBD.done() ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDIDXHELP_EXECST, "_coordCMDIndexHelper::_executeConsistent" )
   INT32 _coordCMDIndexHelper::_executeConsistent( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   INT64 &contextID,
                                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDIDXHELP_EXECST ) ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      INT32 retryCnt = 0 ;
      INT32 tmpRc = SDB_OK;

      rc = _createTaskInCata( pMsg, cb, buf, taskID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create task on catalog" ) ;
         goto error ;
      }

   retry:
      rc = _notifyDataToDoTask( taskID, pMsg, cb, buf ) ;
      if ( SDB_CLS_FULL_SYNC == rc && retryCnt < 15 )
      {
         // data primary is in full sync, we can wait for the data group to
         // select a new primary
         PD_LOG( PDWARNING, "Failed to notify data node, rc: %d, "
                 "just retry", rc ) ;
         buf->release() ;
         retryCnt++ ;
         ossSleep( OSS_ONE_SEC ) ;
         goto retry ;
      }
      else if ( rc )
      {
         PD_LOG_MSG( PDERROR,
                     "Failed to notify data node to do task[%llu], rc: %d",
                     taskID, rc ) ;

         tmpRc = _cancelTask( taskID, rc, buf, cb ) ;
         if ( tmpRc )
         {
            PD_LOG( PDWARNING, "Failed to cancel task[%llu], rc: %d",
                    taskID, tmpRc ) ;
         }
         else if ( !_isAsync() )
         {
            tmpRc = _waitTask( taskID, cb, contextID, buf ) ;
            if ( tmpRc )
            {
               PD_LOG( PDWARNING, "Failed to wait task[%llu], rc: %d",
                       taskID, tmpRc ) ;
            }
         }

         goto error ;
      }

      if ( _isAsync() )
      {
         rc = _returnTaskID( taskID, cb, contextID ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR,
                        "Failed to return task id[%llu] to client, rc: %d",
                        taskID, rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _waitTask( taskID, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to wait task[%llu], rc: %d", taskID, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORDIDXHELP_EXECST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDIDXHELP_CRTTASKCAT, "_coordCMDIndexHelper::_createTaskInCata" )
   INT32 _coordCMDIndexHelper::_createTaskInCata( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  rtnContextBuf *buf,
                                                  UINT64 &taskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDIDXHELP_CRTTASKCAT ) ;
      INT32 orgCode = pMsg->opCode ;
      vector< BSONObj > vecObjs ;
      BSONObj collectionObj ;
      CoordCataInfoPtr cataPtr ;

      // build catalog message
      if ( 0 == ossStrcmp( getName(), CMD_NAME_CREATE_INDEX ) )
      {
         pMsg->opCode = MSG_CAT_CREATE_IDX_REQ ;
      }
      else if ( 0 == ossStrcmp( getName(), CMD_NAME_DROP_INDEX ) )
      {
         pMsg->opCode = MSG_CAT_DROP_IDX_REQ ;
      }

      // send message to catalog
      rc = executeOnCataGroup( pMsg, cb, NULL, &vecObjs, TRUE, NULL, buf ) ;
      PD_RC_CHECK( rc,
                   PDERROR, "Execute %s on catalog failed, rc: %d",
                   getName(), rc ) ;

      // get task ID
      PD_CHECK( !vecObjs.empty(), SDB_SYS, error, PDERROR,
                "Failed to get task id from empty result message" ) ;

      rc = rtnGetNumberLongElement( vecObjs[0], FIELD_NAME_TASKID,
                                    (INT64&)taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get task id from object[%s], rc: %d",
                   vecObjs[0].toString().c_str(), rc ) ;

   done:
      pMsg->opCode = orgCode ;
      PD_TRACE_EXITRC( COORDIDXHELP_CRTTASKCAT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDIDXHELP_NTFDATATODO, "_coordCMDIndexHelper::_notifyDataToDoTask" )
   INT32 _coordCMDIndexHelper::_notifyDataToDoTask( UINT64 taskID,
                                                    MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDIDXHELP_NTFDATATODO ) ;
      CHAR *pBuf = NULL ;
      INT32 bufSize = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pHint = NULL ;
      const CHAR *pCmdName = NULL ;
      BSONObj boQuery, boHint ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, &pCmdName, NULL, NULL,
                            &pQuery, NULL, NULL, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to extract message, rc: %d",
                   rc ) ;

      // build new message, add taskID
      try
      {
         boQuery = BSONObj( pQuery ) ;
         BSONObjBuilder builder ;
         builder.appendElements( BSONObj( pHint ) ) ;
         builder.append( FIELD_NAME_TASKID, (INT64)taskID ) ;
         boHint = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = msgBuildQueryMsg( &pBuf, &bufSize, pCmdName,
                             0, 0, 0, -1,
                             &boQuery, NULL, NULL, &boHint,
                             cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build data message, rc: %d",
                   rc ) ;

      // createIndex, dropIndex or copyIndex don't need to check replSize
      ((MsgOpQuery*)pBuf)->w = 1 ;

      // notify to data
      rc = executeOnCL( (MsgHeader*)pBuf, cb, _collectionName(),
                        FALSE, NULL, NULL, NULL, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to create index on data node, rc: %d", rc );

   done :
      if ( pBuf )
      {
         msgReleaseBuffer( pBuf, cb ) ;
         pBuf = NULL ;
      }
      PD_TRACE_EXITRC( COORDIDXHELP_NTFDATATODO, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDIDXHELP_CANCELTASK, "_coordCMDIndexHelper::_cancelTask" )
   INT32 _coordCMDIndexHelper::_cancelTask( UINT64 taskID, INT32 resultCode,
                                            rtnContextBuf *buf, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDIDXHELP_CANCELTASK ) ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      rtnContextBuf buff ;
      BSONObj boSend ;
      BSONObjBuilder builder ;

      try
      {
         BSONArrayBuilder arrayBD( builder.subarrayStart( FIELD_NAME_ERROR_INFO ) ) ;
         rc = _buildErrInfo( buf, arrayBD ) ;
         if ( rc )
         {
            goto error ;
         }
         arrayBD.done() ;

         builder.append( FIELD_NAME_TASKID, (INT64)taskID ) ;
         builder.append( FIELD_NAME_RESULTCODE, resultCode ) ;
         boSend = builder.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = msgBuildQueryMsg( &pMsg, &msgSize,
                             CMD_ADMIN_PREFIX CMD_NAME_CANCEL_TASK,
                             0, 0, 0, -1,
                             &boSend, NULL, NULL, NULL,
                             cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Build cancel task message failed, rc: %d",
                   rc ) ;

      ((MsgHeader*)pMsg)->opCode = MSG_CAT_TASK_CANCEL_REQ ;

      rc = executeOnCataGroup ( (MsgHeader*)pMsg, cb, TRUE,
                                NULL, NULL, &buff ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Execute task[%llu] cancel on catalog failed, rc: %d",
                   taskID, rc ) ;

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      PD_TRACE_EXITRC( COORDIDXHELP_CANCELTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDIDXHELP_WAITTASK, "_coordCMDIndexHelper::_waitTask" )
   INT32 _coordCMDIndexHelper::_waitTask( UINT64 taskID, pmdEDUCB *cb,
                                          INT64 &contextID, rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDIDXHELP_WAITTASK ) ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      coordOperator *pOperator = NULL ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      BSONObj boSend ;

      // Data source collection will return -1 taskID.
      if ( CLS_INVALID_TASKID == taskID )
      {
         goto done ;
      }

      try
      {
         boSend = BSON( FIELD_NAME_TASKID << (long long)taskID ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = msgBuildQueryMsg( &pMsg, &msgSize,
                             CMD_ADMIN_PREFIX CMD_NAME_WAITTASK,
                             0, 0, 0, -1,
                             &boSend, NULL, NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Build wait task message failed, rc: %d",
                   rc ) ;

      rc = pFactory->create( CMD_NAME_WAITTASK, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Create operator[%s] failed, rc: %d",
                 CMD_NAME_WAITTASK, rc ) ;
         // ignored the error
         rc = SDB_OK ;
         goto done ;
      }

      rc = pOperator->init( _pResource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Init operator[%s] failed, rc: %d",
                   pOperator->getName(), rc ) ;

      rc = pOperator->execute( (MsgHeader *)pMsg, cb, contextID, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait task[%lld] failed, rc: %d",
                   taskID, rc ) ;

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC( COORDIDXHELP_WAITTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDIDXHELP_RTNTASKID, "_coordCMDIndexHelper::_returnTaskID" )
   INT32 _coordCMDIndexHelper::_returnTaskID( UINT64 taskID, pmdEDUCB *cb,
                                              INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDIDXHELP_RTNTASKID ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextDump::sharePtr pContext ;
      BSONObj obj ;

      // coord return taskid to client, client will do "getMore"
      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP, pContext, contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Create context failed, rc: %d", rc ) ;

      rc = pContext->open( BSONObj(), BSONObj(), 1, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

      try
      {
         obj = BSON( FIELD_NAME_TASKID << (long long)taskID ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = pContext->append( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append obj, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( COORDIDXHELP_RTNTASKID, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   BOOLEAN _coordCMDIndexHelper::_hasSpecialError( ROUTE_RC_MAP faileds,
                                                   INT32 specialErr )
   {
      if ( faileds.size() > 0 )
      {
         for ( ROUTE_RC_MAP::iterator iter = faileds.begin() ;
               iter != faileds.end() ; ++iter )
         {
            if ( specialErr == iter->second._rc )
            {
               return TRUE ;
            }
         }
      }
      return FALSE ;
   }

   /*
      _coordCMDCreateIndex implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateIndex,
                                      CMD_NAME_CREATE_INDEX,
                                      FALSE ) ;
   _coordCMDCreateIndex::_coordCMDCreateIndex()
   : _pCollection( NULL ),
     _async( FALSE ),
     _isStandaloneIdx( FALSE )
   {
   }

   _coordCMDCreateIndex::~_coordCMDCreateIndex()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCRTIDX_EXE, "_coordCMDCreateIndex::execute" )
   INT32 _coordCMDCreateIndex::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDCRTIDX_EXE ) ;
      INT32 retryCnt = 0 ;

      rc = _parseMsg( pMsg ) ;
      if ( rc )
      {
         goto error ;
      }

   retry:
      if ( _isStandaloneIdx )
      {
         rc = _executeStandalone( pMsg, cb, contextID, buf ) ;
         if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc && retryCnt < 2 )
         {
            // the collection on data node may be splited, just retry
            PD_LOG( PDWARNING, "Failed to execute on data node, rc: %d, "
                    "just retry", rc ) ;
            buf->release() ;
            retryCnt++ ;
            goto retry ;
         }
      }
      else
      {
         rc = _executeConsistent( pMsg, cb, contextID, buf ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_CREATE_INDEX, AUDIT_OBJ_CL,
                        _pCollection, rc, "IndexDef: %s, Async: %s",
                        _boIndex.toString().c_str(),
                        _async ? "true" : "false" ) ;
   done:
      PD_TRACE_EXITRC( COORDCRTIDX_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCRTIDX_EXESTAND, "_coordCMDCreateIndex::_executeStandalone" )
   INT32 _coordCMDCreateIndex::_executeStandalone( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   INT64 &contextID,
                                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDCRTIDX_EXESTAND ) ;
      INT32 version = 0 ;
      CoordGroupList allgroups ;
      SET_ROUTEID sendNodes ;
      ROUTE_RC_MAP faileds ;
      SET_ROUTEID sucNodes ;
      CoordGroupList clGroups ;
      BOOLEAN allIsDS = TRUE ;

      // get nodes list from message
      rc = _pResource->updateGroupList( allgroups, cb, NULL,
                                        TRUE, TRUE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to update all group list, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      rc = coordGetGroupNodes( _pResource, cb, _boQuery, NODE_SEL_ALL,
                               allgroups, sendNodes, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get group nodes failed, rc: %d", rc ) ;
         goto error ;
      }
      if ( sendNodes.empty() )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "The specified node doesn't exist" ) ;
         goto error ;
      }

      // get collection's group
      rc = _getCLInfo( _pCollection, cb, clGroups, version ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get collection's group failed, rc: %d", rc ) ;
         goto error ;
      }

      // set the version for verify in data-groups
      ((MsgOpQuery*)pMsg)->version = version ;

      // check it is data source collection or not
      for ( CoordGroupList::iterator it = clGroups.begin() ;
            it != clGroups.end() ; it++ )
      {
         if ( !SDB_IS_DSID( it->first ) )
         {
            allIsDS = FALSE ;
            break ;
         }
      }

      if ( allIsDS )
      {
         // It is data source collection, or main-collection which all
         // sub-collections are data source collection. Use dummy routeid,
         // which will processed in _coordDSMsgConvertor::filter().
         sendNodes.clear() ;
         for ( CoordGroupList::iterator it = clGroups.begin() ;
               it != clGroups.end() ; it++ )
         {
            MsgRouteID route ;
            route.columns.groupID = it->first ;
            sendNodes.insert( route.value ) ;
         }

      }
      else
      {
         // clear nodes which don't belongs to this collection
         SET_UINT64::iterator it = sendNodes.begin() ;
         while ( it != sendNodes.end() )
         {
            MsgRouteID route ;
            route.value = *it ;
            if ( clGroups.find( route.columns.groupID ) == clGroups.end() )
            {
               sendNodes.erase( it++ ) ;
            }
            else
            {
               it++ ;
            }
         }
         if ( sendNodes.empty() )
         {
            rc = SDB_CL_NOT_EXIST_ON_NODE ;
            PD_LOG_MSG( PDERROR,
                        "Collection doesn't exist on the specified node" ) ;
            goto error ;
         }
      }

      // create index on the data nodes
      rc = executeOnNodes( pMsg, cb, sendNodes, faileds, &sucNodes ) ;
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to execute on data nodes, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || faileds.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &faileds,
                                                    sucNodes.size() ) ) ;
      }
      if ( _hasSpecialError( faileds, SDB_CLS_COORD_NODE_CAT_VER_OLD ) )
      {
         rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
      }
      PD_TRACE_EXITRC( COORDCRTIDX_EXESTAND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCRTIDX_CHKPARA, "_coordCMDCreateIndex::_checkLocationPara" )
   BOOLEAN _coordCMDCreateIndex::_checkLocationPara( BSONElement ele,
                                                     INT32 expectType )
   {
      BOOLEAN isOk = FALSE ;
      PD_TRACE_ENTRY( COORDCRTIDX_CHKPARA ) ;

      if ( expectType == ele.type() )
      {
         isOk = TRUE ;
      }
      else if ( Array == ele.type() )
      {
         BSONObjIterator it( ele.Obj() ) ;
         while ( it.more() )
         {
            BSONElement e = it.next() ;
            if ( expectType != e.type() )
            {
               goto error ;
            }
         }
         isOk = TRUE ;
      }

   done:
      PD_TRACE_EXIT( COORDCRTIDX_CHKPARA ) ;
      return isOk ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCRTIDX_PARSEMSG, "_coordCMDCreateIndex::_parseMsg" )
   INT32 _coordCMDCreateIndex::_parseMsg( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDCRTIDX_PARSEMSG ) ;
      BOOLEAN foundOutSort = FALSE ;
      BOOLEAN hasNode = FALSE ;
      BOOLEAN notNull = FALSE ;
      BOOLEAN notArray = FALSE ;
      BOOLEAN isUnique = FALSE ;
      BOOLEAN isGlobal = FALSE ;
      const CHAR* pQuery = NULL ;
      const CHAR* pHint = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract message, rc: %d", rc ) ;

      try
      {
         _boQuery = BSONObj( pQuery ) ;
         BSONObj newBoIndex ;
         BSONObj boHint( pHint ) ;
         BSONObjIterator iq( _boQuery ) ;
         BSONObjIterator ih( boHint ) ;
         BSONObjIterator ii ;

         // get Collection, Index
         while ( iq.more() )
         {
            BSONElement e = iq.next();
            if ( ossStrcmp( e.fieldName(), FIELD_NAME_COLLECTION ) == 0 )
            {
               if ( e.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_COLLECTION, _boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _pCollection = e.valuestr() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_INDEX ) == 0 )
            {
               if ( !e.isABSONObj() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_INDEX, _boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _boIndex = e.Obj() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_NODE_NAME ) == 0 )
            {
               if ( !_checkLocationPara( e, String ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR,
                              "%s should be String or Array of strings",
                              FIELD_NAME_NODE_NAME ) ;
                  goto error ;
               }
               hasNode = TRUE ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_NODEID ) == 0 )
            {
               if ( !_checkLocationPara( e, NumberInt ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR,
                              "%s should be NumberInt or Array of integers",
                              FIELD_NAME_NODEID ) ;
                  goto error ;
               }
               hasNode = TRUE ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_INSTANCEID ) == 0 )
            {
               if ( !_checkLocationPara( e, NumberInt ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR,
                              "%s should be NumberInt or Array of integers",
                              FIELD_NAME_INSTANCEID ) ;
                  goto error ;
               }
               hasNode = TRUE ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_ASYNC ) == 0 )
            {
               if ( e.type() != Bool )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_ASYNC, _boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _async = e.boolean();
            }
            else if ( ossStrcmp( e.fieldName(),
                                 IXM_FIELD_NAME_SORT_BUFFER_SIZE ) == 0 )
            {
               foundOutSort = TRUE ;
               if ( !e.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "%s should be Number",
                              IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
                  goto error ;
               }
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Unrecognized field[%s] when creating index",
                       e.fieldName() ) ;
               goto error ;
            }
         }

         // get name, key, unique, enforced, NotNull from Index
         newBoIndex = _boIndex ;
         rc = rtnCheckAndConvertIndexDef( newBoIndex ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to convert index definition: %s",
                      newBoIndex.toString().c_str() ) ;

         ii = BSONObjIterator( newBoIndex ) ;
         while ( ii.more() )
         {
            BSONElement e = ii.next();
            if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_UNIQUE ) == 0 )
            {
               isUnique = e.trueValue() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_NOTNULL ) == 0 )
            {
               notNull = e.trueValue() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_NOTARRAY ) == 0 )
            {
               notArray = e.trueValue() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_GLOBAL ) == 0 )
            {
               isGlobal = e.trueValue() ;
            }
            else if ( ossStrcmp( e.fieldName(), IXM_FIELD_NAME_STANDALONE ) == 0 )
            {
               _isStandaloneIdx = e.boolean();
            }
         }

         // get sort buffer size from hint
         if ( !foundOutSort )
         {
            while ( ih.more() )
            {
               BSONElement e = ih.next() ;
               if ( ossStrcmp( e.fieldName(),
                               IXM_FIELD_NAME_SORT_BUFFER_SIZE ) == 0 )
               {
                  if ( !e.isNumber() )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "%s should be number",
                                 IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
                     goto error ;
                  }
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Unrecognized field: %s", e.fieldName() ) ;
                  goto error ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      if ( _isStandaloneIdx )
      {
         if ( !hasNode )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR,
                        "One of %s/%s/%s must be specified when %s is true",
                        FIELD_NAME_NODE_NAME, FIELD_NAME_NODEID,
                        FIELD_NAME_INSTANCEID, IXM_FIELD_NAME_STANDALONE ) ;
            goto error ;
         }
         if ( _async )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR,
                        "Standalone index doesn't support async creation" ) ;
            goto error ;
         }
         if ( isUnique || notNull || notArray || isGlobal ||
              ixmIsTextIndex( _boIndex ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR,
                        "Standalone index doesn't support "
                        "Unique/NotNull/NotArray/Global/Text index" ) ;
            goto error ;
         }
      }
      else
      {
         if ( hasNode )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "%s/%s/%s take effect only when %s is true",
                        FIELD_NAME_NODE_NAME, FIELD_NAME_NODEID,
                        FIELD_NAME_INSTANCEID, IXM_FIELD_NAME_STANDALONE ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORDCRTIDX_PARSEMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCRTIDX_GETCLINFO, "_coordCMDCreateIndex::_getCLInfo" )
   INT32 _coordCMDCreateIndex::_getCLInfo( const CHAR *collection,
                                           pmdEDUCB *cb,
                                           CoordGroupList &grpLst,
                                           INT32 &version )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDCRTIDX_GETCLINFO ) ;
      coordCataSel cataSel ;
      CoordGroupList exceptLst ;

      rc = cataSel.bind( _pResource, collection, cb, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Update collection[%s]'s catalog info failed, rc: %d",
                   collection, rc ) ;


      rc = cataSel.getGroupLst( cb, exceptLst, grpLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s]'s group list failed, "
                   "rc: %d", collection, rc ) ;

      version = cataSel.getCataPtr()->getVersion() ;

   done :
      PD_TRACE_EXITRC( COORDCRTIDX_GETCLINFO, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDDropIndex implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropIndex,
                                      CMD_NAME_DROP_INDEX,
                                      FALSE ) ;
   _coordCMDDropIndex::_coordCMDDropIndex()
   : _pCollection( NULL ),
     _async( FALSE )
   {
   }

   _coordCMDDropIndex::~_coordCMDDropIndex()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDDROPIDX_GETIDXINFO, "_coordCMDDropIndex::_getIndexInfoFromObj" )
   INT32 _coordCMDDropIndex::_getIndexInfoFromObj( const BSONObj &obj,
                                                   BOOLEAN &isOldVersionIdx,
                                                   BOOLEAN &isStandaloneIdx,
                                                   const CHAR *&nodeName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDDROPIDX_GETIDXINFO ) ;
      BSONObj def ;
      isOldVersionIdx = FALSE ;
      isStandaloneIdx = FALSE ;

      /* obj format:
      *   { NodeName: "sdbserver1:11820", ... }
      * or
      *   { ErrNodes: [ { NodeName: ..., GroupName: ..., Flag: -79, ... ] }
      */
      rc = rtnGetObjElement( obj, IXM_FIELD_NAME_INDEX_DEF, def ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         // print error log
         BSONObj errArr ;
         INT32 rc1 = rtnGetArrayElement( obj, FIELD_NAME_ERROR_NODES, errArr ) ;
         if ( SDB_OK == rc1 )
         {
            rc = SDB_OK ;
            // get error node name and return code
            BSONObjIterator itr( errArr ) ;
            while( itr.more() )
            {
               BSONElement e = itr.next() ;
               if ( Object == e.type() )
               {
                  BSONObj errNodeObj ;
                  const CHAR* errNodeName = "" ;
                  INT32 flag = SDB_OK ;
                  try
                  {
                     errNodeObj = e.embeddedObject() ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                  }
                  rtnGetStringElement( errNodeObj, FIELD_NAME_NODE_NAME,
                                       &errNodeName ) ;
                  rtnGetIntElement( errNodeObj, FIELD_NAME_RCFLAG, flag ) ;
                  PD_LOG( PDWARNING,
                          "Failed to snapshot index on node[%s], rc: %d",
                          errNodeName, flag ) ;
               }
            }
         }
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   IXM_FIELD_NAME_INDEX_DEF, obj.toString().c_str(), rc ) ;

      rc = rtnGetBooleanElement( def, IXM_FIELD_NAME_STANDALONE,
                                 isStandaloneIdx ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         isOldVersionIdx = TRUE ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   IXM_FIELD_NAME_STANDALONE, def.toString().c_str(), rc ) ;

      rc = rtnGetStringElement( obj, FIELD_NAME_NODE_NAME, &nodeName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   FIELD_NAME_NODE_NAME, obj.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( COORDDROPIDX_GETIDXINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDDROPIDX_SNAPIDX, "_coordCMDDropIndex::_snapshotIndex" )
   INT32 _coordCMDDropIndex::_snapshotIndex( pmdEDUCB *cb,
                                             BOOLEAN &hasConsistentIdx,
                                             BOOLEAN &hasStandaloneIdx,
                                             BOOLEAN &hasOldVersionIdx,
                                             ossPoolVector<ossPoolString> &standIdxNodeList )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDDROPIDX_SNAPIDX ) ;
      CHAR *pMsg = NULL ;
      INT32 msgSize = 0 ;
      coordOperator *pOperator = NULL ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      INT64 contextID = -1 ;
      rtnContextBuf buf ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      const CHAR *indexName = _index.firstElement().valuestrsafe() ;

      hasStandaloneIdx = FALSE ;
      hasConsistentIdx = FALSE ;
      hasOldVersionIdx = FALSE ; // before v3.6&v5.0.3, index has no unique id

      try
      {
         // build message
         BSONObj match = BSON( FIELD_NAME_RAWDATA << true <<
                               IXM_FIELD_NAME_INDEX_DEF "."
                               IXM_FIELD_NAME_NAME << indexName ) ;
         BSONObj hint = BSON( FIELD_NAME_COLLECTION << _pCollection ) ;
         rc = msgBuildQueryMsg( &pMsg, &msgSize,
                                CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_INDEXES,
                                0, 0, 0, -1,
                                &match, NULL, NULL, &hint, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Build message failed, rc: %d",
                      rc ) ;

         // query( snapshot index )
         rc = pFactory->create( CMD_NAME_SNAPSHOT_INDEXES, pOperator ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Create operator[%s] failed, rc: %d",
                      CMD_NAME_SNAPSHOT_INDEXES, rc ) ;

         rc = pOperator->init( _pResource, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Init operator[%s] failed, rc: %d",
                      pOperator->getName(), rc ) ;

         rc = pOperator->execute( (MsgHeader *)pMsg, cb,
                                  contextID, &buf ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Execute operator[%s] failed, rc: %d",
                      pOperator->getName(), rc ) ;

         if ( -1 == contextID )
         {
            goto done ;
         }

         // get more
         while ( TRUE )
         {
            rc = rtnGetMore( contextID, -1, buf, cb, rtnCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

            while ( !buf.eof() )
            {
               const CHAR* nodeName = NULL ;
               BOOLEAN isOldVerIdx = FALSE ;
               BOOLEAN isStandIdx = FALSE ;
               BSONObj obj ;

               rc = buf.nextObj( obj ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to get obj from obj buf, rc: %d", rc ) ;

               rc = _getIndexInfoFromObj( obj, isOldVerIdx, isStandIdx,
                                          nodeName ) ;
               if ( SDB_OK == rc )
               {
                  if ( isOldVerIdx )
                  {
                     hasOldVersionIdx = TRUE ;
                  }
                  else if ( isStandIdx )
                  {
                     hasStandaloneIdx = TRUE ;
                     standIdxNodeList.push_back( nodeName ) ;
                  }
                  else
                  {
                     hasConsistentIdx = TRUE ;
                  }
               }
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      if ( SDB_OK == rc )
      {
         PD_LOG( PDDEBUG, "Index[%s:%s] has standalone index: %d, "
                 "has old version index: %d",
                 _pCollection, indexName, hasStandaloneIdx, hasOldVersionIdx ) ;
      }
      if ( contextID != -1 )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, cb ) ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC( COORDDROPIDX_SNAPIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDDROPIDX_EXE, "_coordCMDDropIndex::execute" )
   INT32 _coordCMDDropIndex::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDDROPIDX_EXE ) ;
      BOOLEAN hasStandaloneIdx = FALSE ;
      BOOLEAN hasConsistentIdx = FALSE ;
      BOOLEAN hasOldVersionIdx = FALSE ;
      ossPoolVector<ossPoolString> nodeList ;
      INT32 retryCnt = 0 ;
      CHAR *pBuf = NULL ;
      INT32 bufSize = 0 ;

      rc = _parseMsg( pMsg ) ;
      if ( rc )
      {
         goto error ;
      }

   retry:
      // If there is a mix of standalone index and consistent index, we need to
      // drop index twice.
      rc = _snapshotIndex( cb, hasConsistentIdx, hasStandaloneIdx,
                           hasOldVersionIdx, nodeList ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( hasStandaloneIdx && _async )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR,
                     "Standalone index doesn't support async drop" ) ;
         goto error ;
      }

      if ( hasStandaloneIdx )
      {
         rc = _executeStandalone( pMsg, nodeList, cb, contextID, buf ) ;
         if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc && retryCnt < 2 )
         {
            // the collection on data node may be splited, just retry
            PD_LOG( PDWARNING, "Failed to execute on data node, rc: %d, "
                    "just retry", rc ) ;
            buf->release() ;
            nodeList.clear() ;
            retryCnt++ ;
            goto retry ;
         }
         if ( rc )
         {
            goto error ;
         }
      }

      // If hasConsistentIdx=false, maybe catalog has index info. We need to
      // execute _executeConsistent() anyway.

      if ( hasOldVersionIdx )
      {
         // old version index doesn't has meta data on catalog, so we should
         // use 'enforce' mode to ignore catalog's error -47
         try
         {
            BSONObjBuilder builder ;
            builder.appendElements( _boQuery ) ;
            builder.append( FIELD_NAME_ENFORCED1, true ) ;
            BSONObj newQuery = builder.done() ;
            rc = msgBuildQueryMsg( &pBuf, &bufSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX,
                                   0, 0, 0, -1,
                                   &newQuery, NULL, NULL, NULL,
                                   cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build message, rc: %d",
                         rc ) ;
            pMsg = (MsgHeader*)pBuf ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      rc = _executeConsistent( pMsg, cb, contextID, buf ) ;
      if ( hasStandaloneIdx )
      {
         if ( SDB_IXM_NOTEXIST == rc ||
              SDB_DMS_CS_NOTEXIST == rc ||
              SDB_DMS_NOTEXIST == rc )
         {
            // has already drop standalone index
            rc = SDB_OK ;
         }
      }
      if ( rc )
      {
         goto error ;
      }

      PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_DROP_INDEX, AUDIT_OBJ_CL,
                        _pCollection, rc, "IndexName: %s, Async: %s",
                        _index.toString().c_str(), _async ? "true" : "false" ) ;
   done:
      if ( pBuf )
      {
         msgReleaseBuffer( pBuf, cb ) ;
      }
      PD_TRACE_EXITRC( COORDDROPIDX_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDDROPIDX_EXESTAND, "_coordCMDDropIndex::_executeStandalone" )
   INT32 _coordCMDDropIndex::_executeStandalone( MsgHeader *pMsg,
                                                 ossPoolVector<ossPoolString> &nodeList,
                                                 pmdEDUCB *cb,
                                                 INT64 &contextID,
                                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDDROPIDX_EXESTAND ) ;
      coordCataSel cataSel ;
      CoordGroupList allgroups ;
      SET_ROUTEID sendNodes ;
      ROUTE_RC_MAP faileds ;
      SET_ROUTEID sucNodes ;
      CoordGroupList clGroups ;
      BSONObj filter ;
      BSONObjBuilder builder ;
      BSONObj hint ;
      CHAR *pBuf = NULL ;
      INT32 bufSize = 0 ;

      // get nodes list from message
      rc = _pResource->updateGroupList( allgroups, cb, NULL,
                                        TRUE, TRUE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to update all group list, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      try
      {
         // eg: { NodeName: [ "sdbserver1:11820", ... ] }
         BSONArrayBuilder sub( builder.subarrayStart( FIELD_NAME_NODE_NAME ) ) ;
         for ( ossPoolVector<ossPoolString>::iterator it = nodeList.begin() ;
               it != nodeList.end() ; it++ )
         {
            sub.append( it->c_str() ) ;
         }
         sub.done() ;
         filter = builder.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = coordGetGroupNodes( _pResource, cb, filter, NODE_SEL_ALL,
                               allgroups, sendNodes, NULL, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get group nodes failed, rc: %d", rc ) ;
         goto error ;
      }

      // build new message, add Standalone to hint
      try
      {
         hint = BSON( FIELD_NAME_STANDALONE << true ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = msgBuildQueryMsg( &pBuf, &bufSize,
                             CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX,
                             0, 0, 0, -1,
                             &_boQuery, NULL, NULL, &hint,
                             cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build message, rc: %d",
                   rc ) ;

      // get collection version, and set the version for verify in data-groups
      rc = cataSel.bind( _pResource, _pCollection, cb, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Update collection[%s]'s catalog info failed, rc: %d",
                   _pCollection, rc ) ;

      ((MsgOpQuery*)pBuf)->version = cataSel.getCataPtr()->getVersion() ;

      // drop index on the data nodes
      rc = executeOnNodes( (MsgHeader*)pBuf, cb, sendNodes,
                           faileds, &sucNodes ) ;
      if ( rc )
      {
         if ( SDB_IXM_DROP_STANDALONE_ONLY == rc )
         {
            // The index was dropped and created as a consistent index.
            rc = SDB_IXM_NOTEXIST ;
         }
         PD_LOG( PDERROR,
                 "Failed to execute on data nodes, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || faileds.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &faileds,
                                                    sucNodes.size() ) ) ;
      }
      if ( _hasSpecialError( faileds, SDB_CLS_COORD_NODE_CAT_VER_OLD ) )
      {
         rc = SDB_CLS_COORD_NODE_CAT_VER_OLD ;
      }
      if ( pBuf )
      {
         msgReleaseBuffer( pBuf, cb ) ;
      }
      PD_TRACE_EXITRC( COORDDROPIDX_EXESTAND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDDROPIDX_PARSEMSG, "_coordCMDDropIndex::_parseMsg" )
   INT32 _coordCMDDropIndex::_parseMsg ( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDDROPIDX_PARSEMSG ) ;
      const CHAR *pQuery = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract message, rc: %d", rc ) ;

      try
      {
         _boQuery = BSONObj( pQuery ) ;
         BSONObjIterator iq( _boQuery ) ;

         // get Collection, IndexName
         while ( iq.more() )
         {
            BSONElement e = iq.next();
            if ( ossStrcmp( e.fieldName(), FIELD_NAME_COLLECTION ) == 0 )
            {
               if ( e.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_COLLECTION, _boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _pCollection = e.valuestr() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_INDEX ) == 0 )
            {
               if ( e.type() != Object )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_INDEX, _boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _index = e.Obj() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_ASYNC ) == 0 )
            {
               if ( e.type() != Bool )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_ASYNC, _boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _async = e.boolean();
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Unrecognized field[%s] when dropping index",
                       e.fieldName() ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( COORDDROPIDX_PARSEMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDCopyIndex implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCopyIndex,
                                      CMD_NAME_COPY_INDEX,
                                      FALSE ) ;
   _coordCMDCopyIndex::_coordCMDCopyIndex()
   : _pCollection( NULL ),
     _pSubCL( NULL ),
     _pIndexName( NULL ),
     _async( FALSE )
   {
   }

   _coordCMDCopyIndex::~_coordCMDCopyIndex()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCOPYIDX_EXE, "_coordCMDCopyIndex::execute" )
   INT32 _coordCMDCopyIndex::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDCOPYIDX_EXE ) ;

      rc = _parseMsg( pMsg ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _executeConsistent( pMsg, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;
      }

      PD_AUDIT_COMMAND( AUDIT_DDL, CMD_NAME_COPY_INDEX, AUDIT_OBJ_CL,
                        _pCollection, rc,
                        "SubCL: %s, IndexName: %s, Async: %s",
                        _pSubCL ? _pSubCL : "",
                        _pIndexName ? _pIndexName : "",
                        _async ? "true" : "false" ) ;
   done:
      PD_TRACE_EXITRC( COORDCOPYIDX_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDCOPYIDX_PARSEMSG, "_coordCMDCopyIndex::_parseMsg" )
   INT32 _coordCMDCopyIndex::_parseMsg ( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDCOPYIDX_PARSEMSG ) ;
      const CHAR *pQuery = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to extract message, rc: %d",
                   rc ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;
         BSONObjIterator it( boQuery ) ;

         while ( it.more() )
         {
            BSONElement e = it.next();
            if ( ossStrcmp( e.fieldName(), FIELD_NAME_NAME ) == 0 )
            {
               if ( e.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_NAME, boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _pCollection = e.valuestr() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_SUBCLNAME ) == 0 )
            {
               if ( e.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_SUBCLNAME, boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _pSubCL = e.valuestr() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_INDEXNAME ) == 0 )
            {
               if ( e.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_INDEXNAME, boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _pIndexName = e.valuestr() ;
            }
            else if ( ossStrcmp( e.fieldName(), FIELD_NAME_ASYNC ) == 0 )
            {
               if ( e.type() != Bool )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field[%s] invalid in obj[%s]",
                          FIELD_NAME_ASYNC, boQuery.toString().c_str() ) ;
                  goto error ;
               }
               _async = e.boolean();
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Unrecognized field[%s] when copying index",
                       e.fieldName() ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( COORDCOPYIDX_PARSEMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDPop implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDPop,
                                      CMD_NAME_POP,
                                      FALSE ) ;
   _coordCMDPop::_coordCMDPop()
   {
   }

   _coordCMDPop::~_coordCMDPop()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_POP_EXE, "_coordCMDPop::execute" )
   INT32 _coordCMDPop::execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_POP_EXE ) ;
      const CHAR *option = NULL;
      BSONObj boQuery ;
      const CHAR *fullName = NULL ;

      rc = msgExtractQuery( ( const CHAR * )pMsg, NULL, NULL,
                            NULL, NULL, &option, NULL,
                            NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      try
      {
         boQuery = BSONObj( option );
         BSONElement e = boQuery.getField( FIELD_NAME_COLLECTION );
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "invalid pop msg:%s",
                    boQuery.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         fullName = e.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error;
      }

      rc = executeOnCL( pMsg, cb, fullName, FALSE, NULL, NULL,
                        NULL, NULL, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate cl:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( COORD_POP_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
