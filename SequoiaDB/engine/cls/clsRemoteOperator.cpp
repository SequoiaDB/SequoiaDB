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

   Source File Name = clsRemoteOperator.cpp

   Descriptive Name = Remote Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of clsication component. This file contains structure for
   clsication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          3/10/2020   LYB     Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsRemoteOperator.hpp"
#include "pmdProcessor.hpp"
#include "ossUtil.hpp"
#include "dpsUtil.hpp"
#include "pdTrace.hpp"

using namespace bson ;

namespace engine
{
   _clsRemoteOperator::_clsRemoteOperator()
   : _processor( NULL ),
     _session( NULL ),
     _cb( NULL ),
     _sucCount( 0 ),
     _failureCount( 0 ),
     _ctrl( NULL )
   {
   }

   _clsRemoteOperator::~_clsRemoteOperator()
   {
      _clear() ;
   }

   INT32 _clsRemoteOperator::init( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_CHECK( NULL != cb, SDB_INVALIDARG, error, PDERROR,
                "cb can't be null" ) ;
      PD_CHECK( NULL == _cb, SDB_SYS, error, PDERROR, "Can't be init twice" ) ;

      _cb = cb ;
      _ctrl = _cb->getRemoteOpCtrl() ;
      _session = dynamic_cast<pmdSessionBase*>( _cb->getSession() ) ;
      PD_CHECK( NULL != _session , SDB_SYS, error, PDERROR,
                "Failed to dynamic_cast pmdSessionBase, rc: %d", rc ) ;

      _processor = SDB_OSS_NEW _pmdCoordProcessor() ;
      PD_CHECK( NULL != _processor , SDB_OOM, error, PDERROR,
                "Failed to malloc rocesser, rc: %d", rc ) ;

      _session->attachProcessor( _processor ) ;

      _sucCount = 0 ;
      _failureCount = 0 ;
   done:
      return rc ;
   error:
      _clear() ;
      goto done ;
   }

   void _clsRemoteOperator::_clear()
   {
      if ( NULL != _session )
      {
         _session->detachProcessor() ;
      }

      SAFE_OSS_DELETE( _processor ) ;

      _session = NULL ;
      _cb = NULL ;
      _ctrl = NULL ;
      _sucCount = 0 ;
      _failureCount = 0 ;
   }

   INT32 _clsRemoteOperator::transBegin()
   {
      PD_LOG( PDDEBUG, "_clsRemoteOperator::transBegin" ) ;
      _increaseCount( SDB_OK ) ;
      return SDB_OK ;
   }

   INT32 _clsRemoteOperator::transCommit()
   {
      PD_LOG( PDDEBUG, "_clsRemoteOperator::transCommit" ) ;
      _increaseCount( SDB_OK ) ;
      return SDB_OK ;
   }

   INT32 _clsRemoteOperator::transRollback()
   {
      PD_LOG( PDDEBUG, "_clsRemoteOperator::transRollback" ) ;
      _increaseCount( SDB_OK ) ;
      return SDB_OK ;
   }

   void _clsRemoteOperator::_generateErrorInfo( INT32 rc,
                                                rtnContextBuf &contextBuff,
                                                BSONObjBuilder &retBuilder )
   {
      SDB_ASSERT( SDB_OK != rc, "RC must be error" ) ;

      const CHAR *pResData = contextBuff.data() ;
      INT32 resLen = contextBuff.size() ;
      INT32 resNum = contextBuff.recordNum() ;

      if ( 0 == resLen )
      {
         utilBuildErrorBson( retBuilder, rc,
                             _cb->getInfo( EDU_INFO_ERROR ) ) ;
         _errorInfo = retBuilder.obj() ;
      }
      else
      {
         SDB_ASSERT( 1 == resNum, "Record number must be 1" ) ;
         BSONObj errObj( pResData ) ;
         retBuilder.appendElements( errObj ) ;
         _errorInfo = retBuilder.obj() ;
      }
   }

   void _clsRemoteOperator::_getResponse( INT32 rc, rtnContextBuf &contextBuff,
                                          BSONObjBuilder &retBuilder,
                                          const CHAR **ppResData,
                                          INT32 *pResLen,
                                          INT32 *pResNum )
   {
      const CHAR *pResData = contextBuff.data() ;
      INT32 resLen = contextBuff.size() ;
      INT32 resNum = contextBuff.recordNum() ;

      if ( SDB_OK != rc )
      {
         _generateErrorInfo( rc, contextBuff, retBuilder ) ;
         pResData = _errorInfo.objdata() ;
         resLen = (INT32)_errorInfo.objsize() ;
         resNum = 1 ;
      }
      /// succeed and has result info
      else if ( !retBuilder.isEmpty() && 0 == resLen )
      {
         _errorInfo = retBuilder.obj() ;

         pResData = _errorInfo.objdata() ;
         resLen = (INT32)_errorInfo.objsize() ;
         resNum = 1 ;
      }

      if ( NULL != ppResData )
      {
         *ppResData = pResData ;
      }

      if ( NULL != pResLen )
      {
         *pResLen = resLen ;
      }

      if ( NULL != pResNum )
      {
         *pResNum = resNum ;
      }
   }

   void _clsRemoteOperator::_increaseCount( INT32 rc )
   {
      if ( SDB_OK == rc )
      {
         ++_sucCount ;
         _ctrl->afterSucExecute() ;
      }
      else
      {
         ++_failureCount ;
      }
   }

   INT32 _clsRemoteOperator::_processMsg( MsgHeader* msg )
   {
      rtnContextBuf contextBuff ;
      INT64 contextID = -1 ;
      return _processMsg( msg, contextBuff, contextID ) ;
   }

   INT32 _clsRemoteOperator::_processMsg( MsgHeader* msg,
                                          rtnContextBuf& contextBuff,
                                          INT64& contextID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needReply = TRUE ;
      BOOLEAN needRollback = FALSE ;
      BSONObjBuilder builder( PMD_RETBUILDER_DFT_SIZE ) ;

      try
      {
         rc = _processor->processMsg( msg, contextBuff, contextID,
                                      needReply, needRollback, builder ) ;
      }
      catch( std::bad_alloc &e )
      {
         PD_LOG_MSG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

      if ( SDB_OK != rc )
      {
         _generateErrorInfo( rc, contextBuff, builder ) ;
         PD_LOG( PDERROR, "Failed to process message, opCode: %d, error: %s,"
                 " rc: %d", msg->opCode, _errorInfo.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::insert( const CHAR *clName,
                                     const BSONObj &insertor,
                                     INT32 flags,
                                     utilInsertResult *pResult )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufferSize = 0 ;
      _rtnRemoteSiteHandle handler ;

      if ( !_ctrl->beforeExecute() )
      {
         goto done ;
      }

      rc = msgBuildInsertMsg( &msg, &bufferSize, clName, 0, 0, &insertor,
                              _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build message, rc: %d", rc ) ;

      rc = _processMsg( (MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process insert message, rc: %d",
                   rc ) ;

   done:
      _increaseCount( rc ) ;
      msgReleaseBuffer( msg, _cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::remove( const CHAR *clName,
                                     const BSONObj &matcher,
                                     const BSONObj &hint, INT32 flags,
                                     utilDeleteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufferSize = 0 ;

      if ( !_ctrl->beforeExecute() )
      {
         goto done ;
      }

      rc = msgBuildDeleteMsg( &msg, &bufferSize, clName, flags, 0, &matcher,
                              &hint, _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build message, rc: %d", rc ) ;

      rc = _processMsg( (MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process remove message, rc: %d",
                   rc ) ;

   done:
      _increaseCount( rc ) ;
      msgReleaseBuffer( msg, _cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::update( const CHAR *clName,
                                     const BSONObj &matcher,
                                     const BSONObj &updator,
                                     const BSONObj &hint,
                                     INT32 flags,
                                     utilUpdateResult *pResult )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufferSize = 0 ;

      if ( !_ctrl->beforeExecute() )
      {
         goto done ;
      }

      rc = msgBuildUpdateMsg( &msg, &bufferSize, clName, flags, 0, &matcher,
                              &updator, &hint, _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build message, rc: %d", rc ) ;

      rc = _processMsg( (MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process update message, rc: %d",
                   rc ) ;

   done:
      _increaseCount( rc ) ;
      msgReleaseBuffer( msg, _cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::truncateCL( const CHAR *clName )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufferSize = 0 ;
      BSONObj query ;
      const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE ;

      try
      {
         BSONObjBuilder ob ;
         ob.append ( FIELD_NAME_COLLECTION, clName ) ;
         query = ob.obj () ;
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &msg, &bufferSize, pCommand, 0, 0, 0, -1,
                             &query, NULL, NULL, NULL, _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build command, command: %s, rc: %d",
                   pCommand, rc ) ;

      rc = _processMsg( (MsgHeader *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process truncate message, rc: %d",
                   rc ) ;

   done:
      msgReleaseBuffer( msg, _cb ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::snapshot( INT64 &contextID,
                                       const CHAR *pCommand,
                                       const BSONObj &matcher,
                                       const BSONObj &selector,
                                       const BSONObj &orderBy,
                                       const BSONObj &hint,
                                       INT64 numToSkip,
                                       INT64 numToReturn,
                                       INT32 flag )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufferSize = 0 ;
      rtnContextBuf contextBuff ;
      contextID = -1 ;

      rc = msgBuildQueryMsg( &msg, &bufferSize, pCommand,
                             flag, 0, numToSkip, numToReturn,
                             &matcher, &selector, &orderBy, &hint, _cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build snapshot message, command: %s, rc: %d",
                   pCommand, rc ) ;

      rc = _processMsg( (MsgHeader *)msg, contextBuff, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to process snapshot message, rc: %d",
                   rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::snapshotIndexes( INT64 &contextID,
                                              const CHAR *clName,
                                              const CHAR *indexName,
                                              BOOLEAN rawData )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher, hint ;

      try
      {
         matcher = BSON( IXM_FIELD_NAME_INDEX_DEF "." IXM_FIELD_NAME_NAME <<
                         indexName <<
                         FIELD_NAME_RAWDATA << rawData ) ;
         hint = BSON( FIELD_NAME_COLLECTION << clName ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = snapshot( contextID, CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_INDEXES,
                     matcher, BSONObj(), BSONObj(), hint ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::list( INT64 &contextID,
                                   const CHAR *pCommand,
                                   const BSONObj &matcher,
                                   const BSONObj &selector,
                                   const BSONObj &orderBy,
                                   const BSONObj &hint,
                                   INT64 numToSkip,
                                   INT64 numToReturn,
                                   INT32 flag )
   {
      INT32 rc = SDB_OK ;
      CHAR *msg = NULL ;
      INT32 bufferSize = 0 ;
      rtnContextBuf contextBuff ;
      contextID = -1 ;

      rc = msgBuildQueryMsg( &msg, &bufferSize, pCommand,
                             flag, 0, numToSkip, numToReturn,
                             &matcher, &selector, &orderBy, &hint, _cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build snapshot message, command: %s, rc: %d",
                   pCommand, rc ) ;

      rc = _processMsg( (MsgHeader *)msg, contextBuff, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to process snapshot message, rc: %d",
                   rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::listCSIndexes( INT64 &contextID,
                                            utilCSUniqueID csUniqID )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher, dummyObj ;

      rc = utilGetCSBounds( FIELD_NAME_CL_UNIQUEID, csUniqID, matcher ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build cs bound, rc: %d",
                   rc ) ;

      // CMD_NAME_LIST_INDEXES will filter Collection/CLUniqueID field, so
      // we use CMD_NAME_LIST_INDEXES_INTR instead.
      rc = list( contextID, CMD_ADMIN_PREFIX CMD_NAME_LIST_INDEXES_INTR,
                 matcher, dummyObj, dummyObj, dummyObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::stopCriticalMode( const UINT32 &groupID )
   {
      INT32 rc = SDB_OK ;

      CHAR              *msg = NULL ;
      INT32             bufferSize = 0 ;
      rtnContextBuf     contextBuff ;
      INT64             contextID = -1 ;
      const CHAR        *pCommand = CMD_ADMIN_PREFIX CMD_NAME_ALTER_GROUP ;
      BSONObj           queryObj, hintObj ;

      try
      {
         BSONObjBuilder queryBuilder, hintBuilder ;

         // Build Action and Options
         queryBuilder.append( FIELD_NAME_ACTION, SDB_ALTER_GROUP_STOP_CRITICAL_MODE ) ;
         queryBuilder.appendNull( FIELD_NAME_OPTIONS ) ;
         queryBuilder.doneFast() ;

         // Build GroupID
         hintBuilder.append( FIELD_NAME_GROUPID, groupID ) ;
         hintBuilder.doneFast() ;

         queryObj = queryBuilder.obj() ;
         hintObj = hintBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected error happened:%s", e.what() ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &msg, &bufferSize, pCommand, 0, 0, 0, -1,
                             &queryObj, NULL, NULL, &hintObj, _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build stop critical mode message, command: %s, rc: %d",
                   pCommand, rc ) ;

      rc = _processMsg( (MsgHeader *)msg, contextBuff, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process stop critical mode message, rc: %d", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRemoteOperator::stopMaintenanceMode( const UINT32 &groupID,
                                                  const CHAR *pNodeName )
   {
      INT32 rc = SDB_OK ;

      CHAR              *msg = NULL ;
      INT32             bufferSize = 0 ;
      rtnContextBuf     contextBuff ;
      INT64             contextID = -1 ;
      const CHAR        *pCommand = CMD_ADMIN_PREFIX CMD_NAME_ALTER_GROUP ;
      BSONObj           queryObj, hintObj ;

      try
      {
         BSONObjBuilder queryBuilder, hintBuilder ;

         // Build Action and Options
         queryBuilder.append( FIELD_NAME_ACTION, SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE ) ;
         queryBuilder.append( FIELD_NAME_OPTIONS, BSON( FIELD_NAME_NODE_NAME << pNodeName ) ) ;
         queryBuilder.doneFast() ;

         // Build GroupID
         hintBuilder.append( FIELD_NAME_GROUPID, groupID ) ;
         hintBuilder.doneFast() ;

         queryObj = queryBuilder.obj() ;
         hintObj = hintBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected error happened:%s", e.what() ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &msg, &bufferSize, pCommand, 0, 0, 0, -1,
                             &queryObj, NULL, NULL, &hintObj, _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build stop maintenance mode message, command: %s, rc: %d",
                   pCommand, rc ) ;

      rc = _processMsg( (MsgHeader *)msg, contextBuff, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process stop maintenance mode message, rc: %d", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }


   UINT64 _clsRemoteOperator::getSucCount()
   {
      return _sucCount ;
   }

   UINT64 _clsRemoteOperator::getFailureCount()
   {
      return _failureCount ;
   }

   _sdbRemoteOpCtrl* _clsRemoteOperator::getController()
   {
      return _ctrl ;
   }
}

