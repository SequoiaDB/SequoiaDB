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

   Source File Name = fapMongoCommand.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          2020/04/21  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoCommand.hpp"
#include "fapMongoMessageDef.hpp"
#include "msg.hpp"
#include "msgDef.hpp"
#include "pd.hpp"
#include "../bson/lib/md5.hpp"
#include "fapMongoDecimal.hpp"
#include "fapMongoTrace.hpp"
#include "pdTrace.hpp"

using namespace bson ;

namespace fap
{

#define FAP_MONGO_PAYLOADD_MAX_SIZE 128
#define FAP_MONGO_CLIENT_VERSION_STR_MAX_SIZE 128

static INT32 appendEmptyObj2Buf( engine::rtnContextBuf &bodyBuf,
                                 mongoMsgBuffer &msgBuf )
{
   INT32 rc = SDB_OK ;
   BSONObj empty ;
   BSONObj org( bodyBuf.data() ) ;
   msgBuf.zero() ;

   rc = msgBuf.write( org.objdata(), org.objsize() ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = msgBuf.write( empty.objdata(), empty.objsize() ) ;
   if ( rc )
   {
      goto error ;
   }

   bodyBuf = engine::rtnContextBuf( msgBuf.data(), msgBuf.size(), 2 ) ;

done:
   return rc ;
error:
   goto done ;
}

static INT32 escapeDot( string& collectionFullName )
{
   INT32 rc = SDB_OK ;
   BOOLEAN firstLoop = TRUE ;
   string::size_type pos = 0 ;

   while( TRUE )
   {
      pos = collectionFullName.find( '.', pos ) ;
      if ( string::npos == pos )
      {
         break ;
      }
      else if ( firstLoop )
      {
         pos++ ;
         firstLoop = FALSE ;
         continue ;
      }
      else
      {
         try
         {
            collectionFullName.replace( pos, 1, "%2E" ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "An exception occurred when replacing string: "
                    "%s, rc: %d", e.what(), rc ) ;
            goto error ;
         }
         pos += 3 ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

static void unescapeDot( string& collectionName )
{
   string::size_type pos = 0 ;

   while( TRUE )
   {
      pos = collectionName.find( "%2E", pos ) ;
      if ( string::npos == pos )
      {
         break ;
      }
      else
      {
         collectionName.replace( pos, 3, "." ) ;
         pos++ ;
      }
   }
}

static INT32 convertProjection( BSONObj &proj )
{
   INT32 rc = SDB_OK ;
   BSONObjBuilder newBuilder ;
   BOOLEAN hasId = FALSE ;
   BOOLEAN addInclude = FALSE ;
   BOOLEAN addExclude = FALSE ;
   BOOLEAN addIdExclude = FALSE ;
   BOOLEAN hasOperator = FALSE ;
   BSONObjIterator i( proj ) ;

   if ( proj.isEmpty() )
   {
      goto done ;
   }

   try
   {
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         if ( 0 == ossStrcmp( e.fieldName(), "_id" ) )
         {
            hasId = TRUE ;
         }
         if ( Object == e.type() && '$' == e.Obj().firstElementFieldName()[0] )
         {
            hasOperator = TRUE ;
            newBuilder.append( e ) ;
         }
         else
         {
            // { b: 1 } => { b: { $include: 1 } }
            newBuilder.append( e.fieldName(),
                               BSON( "$include" <<
                                     ( e.trueValue() ? 1 : 0 ) ) ) ;
            if ( e.trueValue() )
            {
               addInclude = TRUE ;
            }
            else
            {
               addExclude = TRUE ;
               if ( 0 == ossStrcmp( e.fieldName(), "_id" ) )
               {
                  addIdExclude = TRUE ;
               }
            }
         }
      }

      if ( !hasOperator && !hasId && !addExclude )
      {
         newBuilder.append( "_id", BSON( "$include" << 1 ) ) ;
      }

      proj = newBuilder.obj() ;

      if ( addIdExclude && addInclude )
      {
         proj = proj.filterFieldsUndotted( BSON( "_id" << 1 ), false ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when converting projection: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

static INT32 convertIndexObj( BSONObj& indexObj, string clFullName )
{
   INT32 rc = SDB_OK ;
   BSONObjBuilder builder ;
   BSONObj sdbIdxDef ;

   // listIndexes command may send getMore message, we should convert fullname:
   //     foo.$cmd.listIndexes.bar => foo.bar
   string::size_type pos = clFullName.find( "$cmd.listIndexes" ) ;
   if ( string::npos != pos )
   {
      clFullName.erase( pos, sizeof( "$cmd.listIndexes" ) ) ;
   }

   unescapeDot( clFullName ) ;

   try
   {
      sdbIdxDef = indexObj.getObjectField( "IndexDef" ) ;
      builder.append( "v", sdbIdxDef.getIntField( "v" ) ) ;
      if ( sdbIdxDef.getBoolField( "unique" ) &&
           sdbIdxDef.getBoolField( "enforced" ) )
      {
         builder.append( "unique", true ) ;
      }
      builder.append( "key", sdbIdxDef.getObjectField( "key" ) ) ;
      builder.append( "name", sdbIdxDef.getStringField( "name" ) ) ;
      builder.append( "ns", clFullName.c_str() ) ;

      indexObj = builder.obj() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building bson : %s, rc: %d",
              e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

static INT32 convertCollectionObj( BSONObj& collectionObj )
{
   INT32 rc = SDB_OK ;
   // { Name: "foo.bar" } => { name: "bar" }
   const CHAR* pClFullName = NULL ;
   string clShortName ;

   try
   {
      pClFullName = collectionObj.getStringField( "Name" ) ;
      clShortName = ossStrstr( pClFullName, "." ) + 1 ;

      unescapeDot( clShortName ) ;

      collectionObj = BSON( "name" << clShortName.c_str() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when converting collection obj:"
              " %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

template< typename T >
static INT32 convertMongoOperator2Sdb( const BSONObj &matchConditonObj,
                                       T &matchConditonBob )
{
   INT32  rc = SDB_OK ;

   try
   {
      BSONObjIterator itr( matchConditonObj ) ;
      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;
         const CHAR* pFieldName = NULL ;

         if ( 0 == ossStrcmp( ele.fieldName(), FAP_MONGO_OPERATOR_EQ ) )
         {
            // if the field name is "$eq", we should build the new field name
            // named "$et"
            pFieldName = FAP_MONGO_OPERATOR_ET ;
         }

         if ( Object == ele.type() )
         {
            BSONObjBuilder subBob( matchConditonBob.subobjStart(
                                   ( NULL == pFieldName ) ?
                                   ele.fieldName() : pFieldName ) ) ;
            rc = convertMongoOperator2Sdb( ele.Obj(), subBob ) ;
            if ( rc )
            {
               goto error ;
            }
            subBob.doneFast() ;
         }
         else if ( Array == ele.type() )
         {
            BSONArrayBuilder subBob( matchConditonBob.subarrayStart(
                                     ( NULL == pFieldName ) ?
                                     ele.fieldName() : pFieldName ) ) ;
            rc = convertMongoOperator2Sdb( ele.Obj(), subBob ) ;
            if ( rc )
            {
               goto error ;
            }
            subBob.doneFast() ;
         }
         else
         {
            if ( NULL == pFieldName )
            {
               matchConditonBob.append( ele ) ;
            }
            else
            {
               matchConditonBob.appendAs( ele, pFieldName ) ;
            }
         }
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when converting mongodb op: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

_mongoCmdFactory::_mongoCmdFactory ()
{
   _pCmdInfoRoot = NULL ;
}

_mongoCmdFactory::~_mongoCmdFactory ()
{
   if ( _pCmdInfoRoot )
   {
      _releaseCmdInfo ( _pCmdInfoRoot ) ;
      _pCmdInfoRoot = NULL ;
   }
}

void _mongoCmdFactory::_releaseCmdInfo ( _mongoCmdInfo *pCmdInfo )
{
   if ( pCmdInfo->pNext )
   {
      _releaseCmdInfo ( pCmdInfo->pNext ) ;
      pCmdInfo->pNext = NULL ;
   }
   if ( pCmdInfo->pSub )
   {
      _releaseCmdInfo ( pCmdInfo->pSub ) ;
      pCmdInfo->pSub = NULL ;
   }

   SDB_OSS_DEL pCmdInfo ;
}

INT32 _mongoCmdFactory::_register ( const CHAR * name,
                                    MONGO_CMD_NEW_FUNC pFunc )
{
   INT32 rc = SDB_OK ;

   if ( !_pCmdInfoRoot )
   {
      _pCmdInfoRoot = SDB_OSS_NEW _mongoCmdInfo ;
      _pCmdInfoRoot->cmdName = name ;
      _pCmdInfoRoot->createFunc = pFunc ;
      _pCmdInfoRoot->nameSize = ossStrlen ( name ) ;
      _pCmdInfoRoot->pNext = NULL ;
      _pCmdInfoRoot->pSub = NULL ;
   }
   else
   {
      rc = _insert ( _pCmdInfoRoot, name, pFunc ) ;
   }

   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Register command [%s] failed", name ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoCmdFactory::_insert( _mongoCmdInfo * pCmdInfo,
                                 const CHAR * name,
                                 MONGO_CMD_NEW_FUNC pFunc )
{
   INT32 rc = SDB_OK ;
   _mongoCmdInfo *pNewCmdInfo = NULL ;
   UINT32 sameNum = _near ( pCmdInfo->cmdName.c_str(), name ) ;

   if ( sameNum == 0 )
   {
      if ( !pCmdInfo->pSub )
      {
         pNewCmdInfo = SDB_OSS_NEW _mongoCmdInfo ;
         pNewCmdInfo->cmdName = name ;
         pNewCmdInfo->createFunc = pFunc ;
         pNewCmdInfo->nameSize = ossStrlen(name) ;
         pNewCmdInfo->pNext = NULL ;
         pNewCmdInfo->pSub = NULL ;

         pCmdInfo->pSub = pNewCmdInfo ;
      }
      else
      {
         rc = _insert ( pCmdInfo->pSub, name, pFunc ) ;
      }
   }
   else if ( sameNum == pCmdInfo->nameSize )
   {
      if ( sameNum == ossStrlen ( name ) )
      {
         if ( !pCmdInfo->createFunc )
         {
            pCmdInfo->createFunc = pFunc ;
         }
         else
         {
            rc = SDB_SYS ; //already exist
         }
      }
      else if ( !pCmdInfo->pNext )
      {
         pNewCmdInfo = SDB_OSS_NEW _mongoCmdInfo ;
         pNewCmdInfo->cmdName = &name[sameNum] ;
         pNewCmdInfo->createFunc = pFunc ;
         pNewCmdInfo->nameSize = ossStrlen(&name[sameNum]) ;
         pNewCmdInfo->pNext = NULL ;
         pNewCmdInfo->pSub = NULL ;

         pCmdInfo->pNext = pNewCmdInfo ;
      }
      else
      {
         rc = _insert ( pCmdInfo->pNext, &name[sameNum], pFunc ) ;
      }
   }
   else
   {
      //split next node first
      pNewCmdInfo = SDB_OSS_NEW _mongoCmdInfo ;
      pNewCmdInfo->cmdName = pCmdInfo->cmdName.substr( sameNum ) ;
      pNewCmdInfo->createFunc = pCmdInfo->createFunc ;
      pNewCmdInfo->nameSize = pCmdInfo->nameSize - sameNum ;
      pNewCmdInfo->pNext = pCmdInfo->pNext ;
      pNewCmdInfo->pSub = NULL ;

      pCmdInfo->pNext = pNewCmdInfo ;

      //change cur node
      pCmdInfo->cmdName = pCmdInfo->cmdName.substr ( 0, sameNum ) ;
      pCmdInfo->nameSize = sameNum ;

      if ( sameNum != ossStrlen ( name ) )
      {
         pCmdInfo->createFunc = NULL ;

         rc = _insert ( pNewCmdInfo, &name[sameNum], pFunc ) ;
      }
      else
      {
         pCmdInfo->createFunc = pFunc ;
      }
   }

   return rc ;
}

_mongoCommand *_mongoCmdFactory::create ( const CHAR *pCommandName )
{
   MONGO_CMD_NEW_FUNC pFunc = _find ( pCommandName ) ;
   if ( pFunc )
   {
      return (*pFunc)() ;
   }

   return NULL ;
}

void _mongoCmdFactory::release ( _mongoCommand *pCommand )
{
   if ( pCommand )
   {
      SDB_OSS_DEL pCommand ;
   }
}

MONGO_CMD_NEW_FUNC _mongoCmdFactory::_find ( const CHAR *pName )
{
   MONGO_CMD_NEW_FUNC pFunc = NULL ;
   _mongoCmdInfo *pCmdNode = _pCmdInfoRoot ;
   UINT32 index = 0 ;
   BOOLEAN findRoot = FALSE ;

   while ( pCmdNode )
   {
      findRoot = FALSE ;
      if ( pCmdNode->cmdName.at ( 0 ) != pName[index] )
      {
         pCmdNode = pCmdNode->pSub ;
      }
      else
      {
         findRoot = TRUE ;
      }

      if ( findRoot )
      {
         if ( _near ( pCmdNode->cmdName.c_str(), &pName[index] )
            != pCmdNode->nameSize )
         {
            break ;
         }
         index += pCmdNode->nameSize ;
         if ( pName[index] == 0 ) //find
         {
            pFunc = pCmdNode->createFunc ;
            break ;
         }

         pCmdNode = pCmdNode->pNext ;
      }
   }

   return pFunc ;
}

UINT32 _mongoCmdFactory::_near ( const CHAR *pStr1, const CHAR *pStr2 )
{
   UINT32 same = 0 ;
   while ( pStr1[same] && pStr2[same] && pStr1[same] == pStr2[same] )
   {
      ++same ;
   }
   return same ;
}

_mongoCmdFactory *getMongoCmdFactory()
{
   static _mongoCmdFactory factory ;
   return &factory ;
}

_mongoCmdAssit::_mongoCmdAssit ( MONGO_CMD_NEW_FUNC pFunc )
{
   if ( pFunc )
   {
      _mongoCommand *pCommand = (*pFunc)() ;
      if ( pCommand )
      {
         getMongoCmdFactory()->_register ( pCommand->name(), pFunc ) ;
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }
}

INT32 mongoBuildSdbMsg( _mongoCommand **ppCommand,
                        mongoSessionCtx &sessCtx,
                        mongoMsgBuffer &sdbMsg )
{
   SDB_ASSERT( ppCommand != NULL , "ppCommand can't be NULL!" ) ;

   INT32 rc = SDB_OK ;

   sdbMsg.zero() ;

   rc = (*ppCommand)->buildSdbRequest( sdbMsg, sessCtx ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to build sdb message for command[%s], rc: %d",
                (*ppCommand)->name(), rc ) ;

   PD_LOG( PDDEBUG, "Build sdb msg[ tid: %d, session: %s, "
           "command: %s, clFullName: %s, eduID: %llu ] done",
           ossGetCurrentThreadID(),
           sessCtx.sessionName,
           (*ppCommand)->name() ? (*ppCommand)->name() : "",
           (*ppCommand)->clFullName() ? (*ppCommand)->clFullName() : "",
           sessCtx.eduID ) ;

done:
   return rc ;
error:
   goto done ;
}

static INT32 _mongoGetAndInitCommand( const CHAR *pCommandName,
                                      _mongoMessage *pMsg,
                                      _mongoCommand **ppCommand,
                                      mongoSessionCtx &sessCtx )
{
   SDB_ASSERT( ppCommand != NULL , "ppCommand can't be NULL!" ) ;
   INT32 rc = SDB_OK ;

   *ppCommand = getMongoCmdFactory()->create( pCommandName ) ;
   if ( NULL == *ppCommand )
   {
      CHAR err[ 64 ] = { 0 } ;

      rc = SDB_OPTION_NOT_SUPPORT ;
      PD_LOG( PDERROR,
              "Unknown command name for mongo request: %s",
              pCommandName ) ;

      ossSnprintf( err, 63, "Command '%s' is not supported", pCommandName ) ;
      sessCtx.setError( rc, err ) ;
      goto error ;
   }

   rc = (*ppCommand)->init( pMsg, sessCtx ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to init command[%s], rc: %d",
                (*ppCommand)->name(), rc ) ;

done:
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_MONGOGETANDINITCOM, "mongoGetAndInitCommand" )
INT32 mongoGetAndInitCommand( const CHAR *pMsg,
                              _mongoCommand **ppCommand,
                              mongoSessionCtx &sessCtx )
{
   SDB_ASSERT( ppCommand != NULL , "ppCommand can't be NULL!" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_MONGOGETANDINITCOM ) ;
   INT32 rc = SDB_OK ;
   _mongoMessage mongoMsg ;

   rc = mongoMsg.init( pMsg ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to init message, rc: %d",
                rc ) ;

   if ( MONGO_OP_QUERY == mongoMsg.opCode() )
   {
      _mongoQueryRequest req ;
      const CHAR* ptr = NULL ;
      const CHAR* pCommandName = NULL ;

      rc = req.init( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init query message, rc: %d",
                   rc ) ;

      // real query: "foo.bar"
      // other operation: "foo.$cmd" { insert: "bar", ...}
      ptr = ossStrstr( req.fullCollectionName(),
                       FAP_MONGO_FIELD_NAME_CMD ) ;
      if ( ptr )
      {
         BSONObj obj( req.query() ) ;
         pCommandName = obj.firstElementFieldName() ;
      }
      else
      {
         pCommandName = MONGO_CMD_NAME_QUERY ;
      }

      rc = _mongoGetAndInitCommand( pCommandName, &req, ppCommand, sessCtx ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_OP_GET_MORE == mongoMsg.opCode() )
   {
      _mongoGetmoreRequest req ;
      rc = req.init( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init get more message, rc: %d",
                   rc ) ;

      rc = _mongoGetAndInitCommand( MONGO_CMD_NAME_GETMORE, &req,
                                    ppCommand, sessCtx ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_OP_KILL_CURSORS == mongoMsg.opCode() )
   {
      _mongoKillCursorsRequest req ;
      rc = req.init( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init kill cursors message, rc: %d",
                   rc ) ;

      rc = _mongoGetAndInitCommand( MONGO_CMD_NAME_KILL_CURSORS, &req,
                                    ppCommand, sessCtx ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_OP_COMMAND == mongoMsg.opCode() )
   {
      _mongoCommandRequest req ;
      rc = req.init( pMsg ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init command message, rc: %d",
                   rc ) ;

      rc = _mongoGetAndInitCommand( req.commandName(), &req,
                                    ppCommand, sessCtx ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Unknown message type: %d, rc: %d",
              mongoMsg.opCode(), rc ) ;
      goto error ;
   }

   PD_LOG( PDDEBUG, "Init mongo msg[ tid: %d, session: %s, "
           "command: %s, clFullName: %s, eduID: %llu ] done",
           ossGetCurrentThreadID(),
           sessCtx.sessionName,
           (*ppCommand)->name() ? (*ppCommand)->name() : "",
           (*ppCommand)->clFullName() ? (*ppCommand)->clFullName() : "",
           sessCtx.eduID ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_MONGOGETANDINITCOM, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 mongoReleaseCommand ( _mongoCommand **ppCommand )
{
   if ( ppCommand && *ppCommand )
   {
      getMongoCmdFactory()->release( *ppCommand ) ;
      *ppCommand = NULL ;
   }
   return SDB_OK ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GINIT, "_mongoGlobalCommand::init" )
INT32 _mongoGlobalCommand::init( const _mongoMessage *pMsg,
                                 mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_GINIT ) ;
   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      BSONObj obj = BSONObj( pReq->query() ) ;
      const CHAR* pCommandName = obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Unknown message type: %d, rc: %d", pMsg->type(), rc ) ;
      goto error ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GBUILDREPLYCOMMON, "_mongoGlobalCommand::_buildReplyCommon" )
INT32 _mongoGlobalCommand::_buildReplyCommon( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_GBUILDREPLYCOMMON ) ;
   INT32 rc = SDB_OK ;

   if ( _initMsgType == MONGO_COMMAND_MSG )
   {
      rc = appendEmptyObj2Buf( bodyBuf, _msgBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to append empty obj to buf, rc: %d", rc ) ;
         goto error ;
      }

      mongoCommandResponse res ;
      res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }
   else
   {
      mongoResponse res ;
      res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      res.nReturned = bodyBuf.recordNum() ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GBUILDREPLYCOMMON, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoDatabaseCommand::_queryMsgInit( const _mongoMessage *pMsg )
{
   INT32 rc = SDB_OK ;
   const CHAR* pCommandName = NULL ;
   const CHAR* pNameInReq = NULL ;
   const CHAR* ptr = NULL ;
   const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

   _obj = BSONObj( pReq->query() ) ;
   pCommandName = _obj.firstElementFieldName() ;

   SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
               "Invalid command name" ) ;

   pNameInReq = pReq->fullCollectionName() ;
   ptr = ossStrstr( pNameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), pNameInReq ) ;
   try
   {
      _csName.assign( pNameInReq, ptr - pNameInReq ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName: %s, rc: %d",
              e.what(), rc ) ;
      goto error ;
   }

   _initMsgType = MONGO_QUERY_MSG ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoDatabaseCommand::_commandMsgInit( const _mongoMessage *pMsg )
{
   INT32 rc = SDB_OK ;
   const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

   SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
               "Invalid command name" ) ;

   try
   {
      _csName = pReq->databaseName() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName: %s, rc: %d",
              e.what(), rc ) ;
      goto error ;
   }

   _initMsgType = MONGO_COMMAND_MSG ;

done:
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DBINIT, "_mongoDatabaseCommand::init" )
INT32 _mongoDatabaseCommand::init( const _mongoMessage *pMsg,
                                   mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_DBINIT ) ;
   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      rc = _queryMsgInit( pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init query msg, rc: %d", rc ) ;
         goto error ;
      }
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      rc = _commandMsgInit( pMsg ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init command msg, rc: %d", rc ) ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Unknown message type: %d, rc: %d", pMsg->type(), rc ) ;
      goto error ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DBINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DBBUILDREPLYCOMMON, "_mongoDatabaseCommand::_buildReplyCommon" )
INT32 _mongoDatabaseCommand::_buildReplyCommon( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_DBBUILDREPLYCOMMON ) ;
   INT32 rc = SDB_OK ;

   if ( _initMsgType == MONGO_COMMAND_MSG )
   {
      rc = appendEmptyObj2Buf( bodyBuf, _msgBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to append empty obj to buf, rc: %d", rc ) ;
         goto error ;
      }

      mongoCommandResponse res ;
      res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }
   else
   {
      mongoResponse res ;
      res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      res.nReturned = bodyBuf.recordNum() ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DBBUILDREPLYCOMMON, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DBBUILDFIRBATCH, "_mongoDatabaseCommand::_buildFirstBatch" )
INT32 _mongoDatabaseCommand::_buildFirstBatch( const MsgOpReply &sdbReply,
                                               engine::rtnContextBuf &bodyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { firstBatch: [ {xxx}, {xxx}... ], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   PD_TRACE_ENTRY( SDB_FAPMONGO_DBBUILDFIRBATCH ) ;
   INT32 rc = SDB_OK ;
   BSONObjBuilder resultBuilder ;
   BSONObjBuilder cursorBuilder ;

   if ( SDB_OK != sdbReply.flags && SDB_DMS_EOC != sdbReply.flags )
   {
      goto done ;
   }

   try
   {
      BSONArrayBuilder arr( cursorBuilder.subarrayStart( "firstBatch" ) ) ;
      if ( SDB_OK == sdbReply.flags )
      {
         INT32 offset = 0 ;
         while ( offset < bodyBuf.size() )
         {
            BSONObj obj( bodyBuf.data() + offset ) ;
            offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;

            if ( CMD_LIST_COLLECTION == type() )
            {
               rc = convertCollectionObj( obj ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to convert collection obj, rc: %d",
                          rc ) ;
                  goto error ;
               }
            }
            arr.append( obj ) ;
         }
      }
      arr.done() ;

      if ( CMD_LIST_COLLECTION == type() )
      {
         /* listCL
          *  request: "foo.$cmd" { listCollections: 1 }
          *  reply:   { ... ns: "foo.$cmd.listCollections" ... }
          */
         string ns = csName() ;
         ns += ".$cmd.listCollections" ;
         cursorBuilder.append( "ns", ns.c_str() ) ;
      }

      cursorBuilder.append( "id", SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ) ;
      resultBuilder.append( "cursor", cursorBuilder.obj() ) ;
      resultBuilder.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building first batch : "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DBBUILDFIRBATCH, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CLINIT, "_mongoCollectionCommand::init" )
INT32 _mongoCollectionCommand::init( const _mongoMessage *pMsg,
                                     mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_CLINIT ) ;
   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;
      rc = _init( pReq ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init query msg, rc: %d", rc ) ;
         goto error ;
      }
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;
      rc = _init( pReq ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init command msg, rc: %d", rc ) ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Unknown message type: %d, rc: %d", pMsg->type(), rc ) ;
      goto error ;
   }

   _requestID = pMsg->requestID() ;

   if ( needConvertDecimal() &&
        mongoIsSupportDecimal( ctx.clientInfo ) )
   {
      try
      {
         BSONObjBuilder sdbMsgObjBob( _obj.objsize() ) ;
         rc = mongoDecimal2SdbDecimal( _obj, sdbMsgObjBob ) ;
         if ( rc )
         {
            ctx.setError( rc, "Invalid decimal" ) ;
            PD_LOG( PDERROR, "Failed to convert mongo msg to sdb msg: %d", rc ) ;
            goto error ;
         }
         _obj = sdbMsgObjBob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when building new decimal "
                 "bson : %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CLINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO__CLINIT1, "_mongoCollectionCommand::_init" )
INT32 _mongoCollectionCommand::_init( const _mongoQueryRequest *pReq )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO__CLINIT1 ) ;
   INT32 rc = SDB_OK ;
   const CHAR* pClShortName = NULL ;
   const CHAR* pNameInReq = pReq->fullCollectionName() ;
   const CHAR* pCommandName = NULL ;
   const CHAR* ptr = NULL ;

   _obj = BSONObj( pReq->query() ) ;

   pCommandName = _obj.firstElementFieldName() ;
   SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
               "Invalid command name" ) ;

   rc = mongoGetStringElement( _obj, pCommandName, pClShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                pCommandName, rc ) ;

   ptr = ossStrstr( pNameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), pNameInReq ) ;\

   try
   {
      _csName.assign( pNameInReq, ptr - pNameInReq ) ;

      _clFullName = _csName ;
      _clFullName += "." ;
      _clFullName += pClShortName ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName and clName:"
              " %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = escapeDot( _clFullName ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO__CLINIT1, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO__CLINIT2, "_mongoCollectionCommand::_init" )
INT32 _mongoCollectionCommand::_init( const _mongoCommandRequest *pReq )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO__CLINIT2 ) ;
   INT32 rc = SDB_OK ;
   const CHAR* pClShortName = NULL ;

   _obj = BSONObj( pReq->metadata() ) ;

   SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
               "Invalid command name" ) ;

   rc = mongoGetStringElement( _obj, pReq->commandName(), pClShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                pReq->commandName(), rc ) ;

   try
   {
      _csName = pReq->databaseName() ;

      _clFullName = _csName ;
      _clFullName += "." ;
      _clFullName += pClShortName ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName and clName:"
              " %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = escapeDot( _clFullName ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO__CLINIT2, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CLBUILDREPLYCOMMON, "_mongoCollectionCommand::_buildReplyCommon" )
INT32 _mongoCollectionCommand::_buildReplyCommon( const MsgOpReply &sdbReply,
                                                  engine::rtnContextBuf &bodyBuf,
                                                  _mongoResponseBuffer &headerBuf,
                                                  BOOLEAN setCursorID )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   PD_TRACE_ENTRY( SDB_FAPMONGO_CLBUILDREPLYCOMMON ) ;
   INT32 rc = SDB_OK ;

   if ( _initMsgType == MONGO_COMMAND_MSG )
   {
      rc = appendEmptyObj2Buf( bodyBuf, _msgBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to append empty obj to buf, rc: %d", rc ) ;
         goto error ;
      }

      mongoCommandResponse res ;
      res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }
   else
   {
      mongoResponse res ;
      res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      if ( setCursorID && SDB_OK == sdbReply.flags )
      {
         res.cursorId = SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ;
      }
      res.nReturned = bodyBuf.recordNum() ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CLBUILDREPLYCOMMON, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CLBUILDFIRBATCH, "_mongoCollectionCommand::_buildFirstBatch" )
INT32 _mongoCollectionCommand::_buildFirstBatch( const MsgOpReply &sdbReply,
                                                 engine::rtnContextBuf &bodyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { firstBatch: [ {xxx}, {xxx}... ], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   PD_TRACE_ENTRY( SDB_FAPMONGO_CLBUILDFIRBATCH ) ;
   INT32 rc = SDB_OK ;
   BSONObjBuilder resultBuilder ;
   BSONObjBuilder cursorBuilder ;

   if ( SDB_OK != sdbReply.flags && SDB_DMS_EOC != sdbReply.flags &&
        SDB_DMS_CS_NOTEXIST != sdbReply.flags &&
        SDB_DMS_NOTEXIST != sdbReply.flags)
   {
      goto done ;
   }

   try
   {
      BSONArrayBuilder arr( cursorBuilder.subarrayStart( "firstBatch" ) ) ;
      if ( SDB_OK == sdbReply.flags )
      {
         INT32 offset = 0 ;
         BSONObjBuilder decimalConvertBob ;
         BOOLEAN hasDecimal = FALSE ;

         while ( offset < bodyBuf.size() )
         {
            BSONObj obj( bodyBuf.data() + offset ) ;
            offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;

            if ( CMD_LIST_INDEX == type() )
            {
               rc = convertIndexObj( obj, clFullName() ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to convert index obj, rc: %d", rc ) ;
                  goto error ;
               }
               arr.append( obj ) ;
            }
            else
            {
               rc = sdbDecimal2MongoDecimal( obj, decimalConvertBob, hasDecimal ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to convert sdb record to mongo record, "
                            "rc: %d", rc ) ;

               if ( hasDecimal )
               {
                  arr.append( decimalConvertBob.done() ) ;
                  decimalConvertBob.reset() ;
               }
               else
               {
                  arr.append( obj ) ;
               }
            }
         }
      }
      arr.done() ;

      if ( CMD_LIST_INDEX == type() )
      {
         // listIndexes reply:   { ... ns: "foo.$cmd.listIndexes.bar" ... }
         string ns = _clFullName ;
         string::size_type pos = ns.find( '.' ) ;
         ns.insert( pos, ".$cmd.listIndexes" ) ;
         cursorBuilder.append( "ns", ns.c_str() ) ;
      }
      else
      {
         // real query or aggregate reply:   { ... ns: "foo.bar" ... }
         cursorBuilder.append( "ns", clFullName() ) ;
      }

      cursorBuilder.append( "id", SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ) ;
      resultBuilder.append( "cursor", cursorBuilder.obj() ) ;
      resultBuilder.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building first batch: %s, "
              "rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CLBUILDFIRBATCH, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoInsertCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_INSERTBUILDSDBREQ, "_mongoInsertCommand::buildSdbRequest" )
INT32 _mongoInsertCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                            mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_INSERTBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc                = SDB_OK ;
   MsgOpInsert *pInsert    = NULL ;
   BSONObj docList ;

   rc = sdbMsg.reserve( sizeof( MsgOpInsert ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpInsert ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pInsert = ( MsgOpInsert *)sdbMsg.data() ;
   pInsert->header.opCode = MSG_BS_INSERT_REQ ;
   pInsert->header.TID = 0 ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.requestID = _requestID ;
   pInsert->version = 0 ;
   pInsert->w = 0 ;
   pInsert->padding = 0 ;
   pInsert->flags = FLG_INSERT_RETURNNUM ;
   pInsert->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pInsert->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   // get records to be insert
   rc = mongoGetArrayElement( _obj, FAP_MONGO_FIELD_NAME_DOCUMENTS, docList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_DOCUMENTS, rc ) ;

   try
   {
      BSONObjIterator it( docList ) ;
      while ( it.more() )
      {
         BSONElement ele = it.next() ;
         PD_CHECK( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Invalid object[%s] in mongo %s request",
                   docList.toString().c_str(), name() ) ;

         rc = sdbMsg.write( ele.Obj(), TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb insert request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_INSERTBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_INSERTBUILDMONGOREPLY, "_mongoInsertCommand::buildMongoReply" )
INT32 _mongoInsertCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                            engine::rtnContextBuf &bodyBuf,
                                            _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_INSERTBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         // reply: { n: 1, ok: 1 }
         BSONObj resObj( bodyBuf.data() ) ;
         BSONObjBuilder bob ;

         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bob.append( "n", resObj.getIntField( "InsertedNum" ) ) ;
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo insert reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_INSERTBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDeleteCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DELETEBUILDSDBREQ, "_mongoDeleteCommand::buildSdbRequest" )
INT32 _mongoDeleteCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                            mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DELETEBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc          = SDB_OK ;
   MsgOpDelete *pDel = NULL ;
   BSONObj objList, deleteObj, qObj ;
   BSONObjBuilder operatorBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpDelete ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpDelete ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pDel = ( MsgOpDelete *)sdbMsg.data() ;
   pDel->header.opCode = MSG_BS_DELETE_REQ ;
   pDel->header.TID = 0 ;
   pDel->header.routeID.value = 0 ;
   pDel->header.requestID = _requestID ;
   pDel->version = 0 ;
   pDel->w = 0 ;
   pDel->padding = 0 ;
   pDel->flags = FLG_DELETE_RETURNNUM ;
   pDel->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pDel->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   // { delete: "bar", deletes: [ { q: {xxx}, limit: 0 } ] }
   rc = mongoGetArrayElement( _obj, FAP_MONGO_FIELD_NAME_DELETES, objList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_DELETES, rc ) ;

   try
   {
      if ( !_hasBuildMsgVec )
      {
         BSONObjIterator itr( objList ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid deletes field. The type of element in "
                       "deletes must be Object, rc: %d", rc ) ;
               goto error ;
            }
            _msgVec.push_back( ele ) ;
         }
         _hasBuildMsgVec = TRUE ;
      }

      deleteObj = _msgVec[_msgIndex++].embeddedObject() ;
      qObj = deleteObj.getObjectField( "q" ) ;
      _hasProcessAllMsg = ( _msgIndex >= _msgVec.size() ) ;

      if ( 1 == deleteObj.getIntField( "limit" ) )
      {
         pDel->flags |= FLG_DELETE_ONE ;
      }

      rc = convertMongoOperator2Sdb( qObj, operatorBob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert mongo operator to sdb operator, rc: %d",
                   rc ) ;

      rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( BSONObj(), TRUE ) ; // hint
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb delete request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DELETEBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DELETEPARSESDBREPLY, "_mongoDeleteCommand::parseSdbReply" )
INT32 _mongoDeleteCommand::parseSdbReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DELETEPARSESDBREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         BSONObj resObj( bodyBuf.data() ) ;

         BSONObjIterator itr( resObj ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;

            if ( 0 == ossStrcmp( "DeletedNum", ele.fieldName() ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "the type of DeletedNum field must be "
                          "number, rc: %d", rc ) ;
                  goto error ;
               }

               _deletedNum += ele.numberLong() ;
            }
         }
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when parsing sdb delete reply "
              "msg: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DELETEPARSESDBREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DELETEBUILDMONGOREPLY, "_mongoDeleteCommand::buildMongoReply" )
INT32 _mongoDeleteCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                            engine::rtnContextBuf &bodyBuf,
                                            _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DELETEBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags || SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
           SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         // reply: { n: 1, ok: 1 }
         BSONObjBuilder bob ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;

         if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
              SDB_DMS_NOTEXIST == sdbReply.flags )
         {
            bob.append( "n", 0 ) ;
         }
         else
         {
            bob.append( "n", (INT32)_deletedNum ) ;
         }

         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo delete "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DELETEBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoUpdateCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_UPDATEBUILDSDBREQ, "_mongoUpdateCommand::buildSdbRequest" )
INT32 _mongoUpdateCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                            mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_UPDATEBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc = SDB_OK ;
   BOOLEAN updateMulti  = FALSE ;
   MsgOpUpdate *pUpdate = NULL ;
   BSONObj query, updator, hint, setOnObj, objList, updateObj ;
   BSONObjBuilder operatorBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpUpdate ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpUpdate ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pUpdate = ( MsgOpUpdate *)sdbMsg.data() ;
   pUpdate->header.opCode = MSG_BS_UPDATE_REQ ;
   pUpdate->header.TID = 0 ;
   pUpdate->header.routeID.value = 0 ;
   pUpdate->header.requestID = _requestID ;
   pUpdate->version = 0 ;
   pUpdate->w = 0 ;
   pUpdate->padding = 0 ;
   pUpdate->flags = FLG_UPDATE_RETURNNUM ;
   pUpdate->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pUpdate->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   // { update: "bar",
   //   updates: [ { q: {xxx}, u: {xxx}, upsert: false, multi: true } ... ] }
   rc = mongoGetArrayElement( _obj, FAP_MONGO_FIELD_NAME_UPDATES, objList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_UPDATES, rc ) ;

   try
   {
      if ( !_hasBuildMsgVec )
      {
         BSONObjIterator itr( objList ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid updates field. The type of element in "
                       "updates must be Object, rc: %d", rc ) ;
               goto error ;
            }
            _msgVec.push_back( ele ) ;
         }
         _hasBuildMsgVec = TRUE ;
      }

      updateObj   = _msgVec[_msgIndex++].embeddedObject() ;
      query       = updateObj.getObjectField( "q" ) ;
      updator     = updateObj.getObjectField( "u" ) ;
      updateMulti = updateObj.getBoolField( "multi" ) ;
      _isUpsert   = updateObj.getBoolField( "upsert" ) ;
      _hasProcessAllMsg = ( _msgIndex >= _msgVec.size() ) ;

      // set flag
      if ( FALSE == updateMulti )
      {
         pUpdate->flags |= FLG_UPDATE_ONE ;
      }
      if ( _isUpsert )
      {
         pUpdate->flags |= FLG_UPDATE_UPSERT ;
      }

      // if updator without operator, convert to $replace
      if( 0 == updator.nFields() ||
          updator.firstElementFieldName()[0] != '$' )
      {
         if ( updateMulti )
         {
            rc = SDB_OPTION_NOT_SUPPORT ;
            ctx.setError( 9, "Multi update only works with $ operators" ) ;
            goto error ;
         }
         updator = BSON( "$replace" << updator ) ;
      }

      // upsert operation requires _id to return
      if ( _isUpsert )
      {
         _idObj = BSONObj() ;
         // get the _id from the query condition or the update condition
         rc = _getId( query, updator, ctx, _idObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get _id from the query condition[%s] "
                    "or the update condition[%s], rc: %d",
                    query.toString().c_str(), updator.toString().c_str(), rc ) ;
            goto error ;
         }

         // filter $setOnInsert
         if ( updator.hasField( "$setOnInsert" ) )
         {
            setOnObj = updator.getObjectField( "$setOnInsert" ) ;
            updator = updator.filterFieldsUndotted( BSON( "$setOnInsert" << 1 ),
                                                    false ) ;
            if( 0 == updator.nFields() )
            {
               updator = BSON( "$set" << BSONObj() ) ;
            }
         }

         // add _id to $SetOnInsert if _id doesn't exist
         if ( _idObj.isEmpty() )
         {
            OID oid = OID::gen() ;
            _idObj = BSON( "_id" << oid ) ;

            BSONObjBuilder bob ;
            bob.append( "_id", oid ) ;
            bob.appendElements( setOnObj ) ;
            hint = BSON( FIELD_NAME_SET_ON_INSERT << bob.obj() ) ;
         }
         else
         {
            hint = BSON( FIELD_NAME_SET_ON_INSERT << setOnObj ) ;
         }
      }

      rc = convertMongoOperator2Sdb( query, operatorBob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert mongo operator to sdb operator, rc: %d",
                   rc ) ;

      rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( updator, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( hint, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb update request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_UPDATEBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_UPDATEPARSESDBREPLY, "_mongoUpdateCommand::parseSdbReply" )
INT32 _mongoUpdateCommand::parseSdbReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_UPDATEPARSESDBREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         BSONObj resObj( bodyBuf.data() ) ;

         BSONObjIterator itr( resObj ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;

            if ( 0 == ossStrcmp( ele.fieldName(), "InsertedNum" ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "the type of InsertedNum field must be "
                          "number, rc: %d", rc ) ;
                  goto error ;
               }

               if ( ele.numberLong() >= 1 )
               {
                  _idObjMap[_msgIndex-1] = _idObj ;
               }

               _insertedNum += ele.Long() ;
            }
            else  if ( 0 == ossStrcmp( ele.fieldName(), "UpdatedNum" ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "the type of UpdatedNum field must be "
                          "number, rc: %d", rc ) ;
                  goto error ;
               }

               _updatedNum += ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), "ModifiedNum" ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "the type of ModifiedNum field must be "
                          "number, rc: %d", rc ) ;
                  goto error ;
               }

               _modifiedNum += ele.numberLong() ;
            }
         }
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when parsing sdb update reply "
              "msg: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_UPDATEPARSESDBREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_UPDATEBUILDMONGOREPLY, "_mongoUpdateCommand::buildMongoReply" )
INT32 _mongoUpdateCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                            engine::rtnContextBuf &bodyBuf,
                                            _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_UPDATEBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         // update reply: { ok: 1, n: 1, nModified: 1 }
         // upsert reply: { ok: 1, n: 1, nModified: 0,
         //                 upserted: [ { index: 0, _id: xxx } ] }
         BSONObjBuilder bob ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;

         //n
         if ( _insertedNum > 0 )
         {
            bob.append( "n", (INT32)(_insertedNum + _updatedNum) ) ;
         }
         else
         {
            bob.append( "n", (INT32)_updatedNum ) ;
         }

         //nModified
         bob.append( "nModified", (INT32)_modifiedNum ) ;

         //upserted
         if ( _insertedNum > 0 )
         {
            BSONArrayBuilder sub( bob.subarrayStart( "upserted" ) ) ;

            ossPoolMap<INT32, BSONObj>::iterator iter ;
            iter = _idObjMap.begin() ;
            while( iter != _idObjMap.end() )
            {
               sub.append( BSON( "index" << iter->first <<
                                 "_id" << iter->second.getField( "_id" ) ) ) ;
               iter++ ;
            }

            sub.done() ;
         }
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
                SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         BSONObjBuilder bob ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bob.append( "n", 0 ) ;
         bob.append( "nModified", 0 ) ;
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo update reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_UPDATEBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoUpdateCommand::_getId( const BSONObj &queryObj,
                                   const BSONObj &updatorObj,
                                   mongoSessionCtx &ctx,
                                   BSONObj &idObj )
{
   INT32 rc = SDB_OK ;

   try
   {
      // find _id field from the update condition
      BSONObjIterator i( updatorObj ) ;
      while ( i.more() )
      {
         BSONElement ele = i.next() ;

         if ( Object == ele.type() )
         {
            BSONElement e = ele.Obj().getField( "_id" ) ;

            if ( EOO != e.type() )
            {
               idObj = BSON( "_id" << e ) ;
               break ;
            }
         }
      }

      if ( idObj.isEmpty() )
      {
         // There is no _id field in the update condition, so we need to find
         // _id field from the query condition
         rc = _getIdFromQuery( queryObj, ctx, idObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get _id from the query condition, "
                    "rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         // There is an _id field in the update condition
         // However, because the query condition and update condition can't
         // specify the _id field at the same time, we need to find whether
         // there is an _id field in the query condition
         BSONObj tmpIdObj = idObj ;
         idObj = BSONObj() ;

         rc = _getIdFromQuery( queryObj, ctx, idObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get _id from the query condition, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         if ( !idObj.isEmpty() && !( idObj == tmpIdObj ) )
         {
            rc = SDB_INVALIDARG ;
            ctx.setError( rc, "The query condition and update condition can't "
                          "specify the different _id field at the same time" ) ;
            goto error ;
         }
         else
         {
            idObj = tmpIdObj ;
         }
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting _id: %s, rc: %d",
              e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoUpdateCommand::_getIdFromQuery( const BSONObj &queryObj,
                                            mongoSessionCtx &ctx,
                                            BSONObj &idObj )
{
   INT32 rc = SDB_OK ;

   try
   {
      BSONObjIterator iQuery( queryObj ) ;
      while ( iQuery.more() )
      {
         BSONElement ele = iQuery.next() ;
         if ( 0 == ossStrcmp( "_id", ele.fieldName() ) )
         {
            if ( Array == ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of _id field can't be Array" ) ;
               goto error ;
            }

            if ( Object != ele.type() )
            {
               if ( !idObj.isEmpty() )
               {
                  rc = SDB_INVALIDARG ;
                  ctx.setError( rc, "Can't infer query fields to set. The _id "
                                "field is matched twice" ) ;
                  goto error ;
               }

               idObj = BSON( "_id" << ele ) ;
            }
         }
         // Among the relational operators, only $and can uniquely determine
         // the _id value. So we only need to consider the $and operator.
         // For other operators, we will automatically generate the _id value.
         else if ( 0 == ossStrcmp( "$and", ele.fieldName() ) )
         {
            if ( Array != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of $and field must be Array" ) ;
               goto error ;
            }

            BSONObjIterator i( ele.embeddedObject() ) ;
            while ( i.more() )
            {
               BSONElement e = i.next() ;

               rc = _getIdFromQuery( e.embeddedObject(), ctx, idObj ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to get _id from the query condition"
                          ", rc: %d", rc ) ;
                  goto error ;
               }
            }
         }
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting _id from query "
              "condition: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoQueryCommand)
_mongoQueryCommand::_mongoQueryCommand()
: _skip( 0 ),
  _nReturn( -1 ),
  _client( OTHER ),
  _requestID( 0 ),
  _isInitialized( FALSE )
{
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_QUERYINIT, "_mongoQueryCommand::init" )
INT32 _mongoQueryCommand::init( const _mongoMessage *pMsg,
                                mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_QUERYINIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;
   INT32 rc = SDB_OK ;
   const CHAR* ptr = NULL ;
   const CHAR* pNameInReq = NULL ;
   const _mongoQueryRequest* pReq = NULL ;

   PD_CHECK( MONGO_QUERY_MSG == pMsg->type(),
             SDB_INVALIDARG, error, PDERROR,
             "Unknown message type: %d",
             pMsg->type() ) ;

   pReq = (_mongoQueryRequest*)pMsg ;

   pNameInReq = pReq->fullCollectionName() ;

   ptr = ossStrstr( pNameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( NULL == ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), pNameInReq ) ;

   ptr = ossStrstr( pNameInReq, FAP_MONGO_DOT ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), pNameInReq ) ;

   try
   {
      _csName.assign( pNameInReq, ptr - pNameInReq ) ;
      _clFullName = pNameInReq ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName and clName: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = escapeDot( _clFullName ) ;
   if ( rc )
   {
      goto error ;
   }

   _skip = pReq->nToSkip() ;
   _nReturn = pReq->nToReturn() ;
   _query = BSONObj( pReq->query() ) ;
   if ( pReq->selector() )
   {
      _selector = BSONObj( pReq->selector() ) ;
   }

   _client = ctx.clientInfo.type ;

   _requestID = pMsg->requestID() ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO_QUERYINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_QUERYBUILDREQ, "_mongoQueryCommand::buildSdbRequest" )
INT32 _mongoQueryCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                           mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_QUERYBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc                = SDB_OK ;
   MsgOpQuery *pQuery      = NULL ;
   BSONObj cond, orderby, hint ;
   BSONObjBuilder operatorBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = FLG_QUERY_WITH_RETURNDATA ;
   pQuery->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery->numToSkip = _skip ;
   if ( _nReturn > 0 )
   {
      if ( 1000 == _nReturn && NODEJS_DRIVER == _client )
      {
         // The defalut value of batchSize for NODEJS-driver is 1000,
         // while other driver is 0.
         pQuery->numToReturn = -1 ;
      }
      else
      {
         pQuery->numToReturn = _nReturn ;
      }
   }
   else if ( 0 == _nReturn )
   {
      pQuery->numToReturn = -1 ;
   }
   else
   {
      pQuery->numToReturn = -_nReturn ;
   }

   rc = convertProjection( _selector ) ;
   if ( rc )
   {
      goto error ;
   }

   // eg: cl.find( { $query: {a:1}, $orderby: {b:1}, $hint: "aIdx" } )
   rc = _getQueryObj( _query, cond ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _getSortObj( _query, orderby ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _getHintObj( _query, hint ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = convertMongoOperator2Sdb( cond, operatorBob ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to convert mongo operator to sdb operator, rc: %d",
                rc ) ;

   try
   {
      rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb query request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( _selector, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( orderby, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( hint, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_QUERYBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_QUERYBUILDMONGOREPLY, "_mongoQueryCommand::buildMongoReply" )
INT32 _mongoQueryCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                           engine::rtnContextBuf &bodyBuf,
                                           _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_QUERYBUILDMONGOREPLY ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc = SDB_OK ;
   mongoResponse res ;

   // build body
   if ( sdbReply.numReturned > 1 )
   {
      INT32 offset = 0 ;
      _msgBuf.zero() ;
      while ( offset < bodyBuf.size() )
      {
         BSONObj obj( bodyBuf.data() + offset ) ;

         rc = _msgBuf.write( obj.objdata(), obj.objsize() ) ;
         if ( rc )
         {
            goto error ;
         }

         offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
      }
      bodyBuf = engine::rtnContextBuf( _msgBuf.data(),
                                       _msgBuf.size(),
                                       sdbReply.numReturned ) ;
   }

   if ( SDB_OK != sdbReply.flags )
   {
      if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
           SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf() ;
      }
      else if ( SDB_DMS_EOC != sdbReply.flags )
      {
         // reply: { $err: "xxx", code: 1234 }
         BSONObj resObj( bodyBuf.data() ) ;
         BSONObj newObj ;

         try
         {
            newObj = BSON( FAP_MONGO_DOLLAR FAP_MONGO_FIELD_NAME_ERR <<
                           resObj.getStringField(
                           FAP_MONGO_FIELD_NAME_ERRMSG ) <<
                           FAP_MONGO_FIELD_NAME_CODE <<
                           resObj.getIntField( FAP_MONGO_FIELD_NAME_CODE ) ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "An exception occurred when building query mongo"
                    " reply msg: %s, rc: %d", e.what(), rc ) ;
            goto error ;
         }

         bodyBuf = engine::rtnContextBuf( newObj ) ;
      }
   }

   res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
   res.header.responseTo = _requestID ;
   if ( sdbReply.flags != SDB_OK && sdbReply.flags != SDB_DMS_EOC )
   {
      res.reservedFlags = MONGO_REPLY_FLAG_QUERY_FAILURE ;
   }
   if ( SDB_OK == sdbReply.flags )
   {
      res.cursorId = SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ;
   }
   res.nReturned = bodyBuf.recordNum() ;
   headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_QUERYBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoQueryCommand::_getQueryObj( const BSONObj &obj, BSONObj &query )
{
   INT32 rc = SDB_OK ;

   try
   {
      if ( obj.hasField( "$query" ) )
      {
         query = obj.getObjectField( "$query" ) ;
      }
      else if ( obj.hasField( "query" ) )
      {
         query = obj.getObjectField( "query" ) ;
      }
      else
      {
         query = obj;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting query obj: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoQueryCommand::_getSortObj( const BSONObj &obj, BSONObj &sort )
{
   INT32 rc = SDB_OK ;

   try
   {
      if ( obj.hasField( "$orderby" ) )
      {
         sort = obj.getObjectField( "$orderby" ) ;
      }
      else if ( obj.hasField( "orderby" ) )
      {
         sort = obj.getObjectField( "orderby" ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting sort obj: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoQueryCommand::_getHintObj( const BSONObj &obj, BSONObj &hint )
{
   INT32 rc = SDB_OK ;

   try
   {
      if ( obj.hasField( "$hint" ) )
      {
         if ( String == obj.getField( "$hint" ).type() )
         {
            hint = BSON( "" << obj.getStringField( "$hint" ) ) ;
         }
         else if ( obj.getField( "$hint" ).isABSONObj() )
         {
            hint = obj.getObjectField( "$hint" ) ;
         }
      }
      else if ( obj.hasField( "hint" ) )
      {
         if ( String == obj.getField( "hint" ).type() )
         {
            hint = BSON( "" << obj.getStringField( "hint" ) ) ;
         }
         else if ( obj.getField( "hint" ).isABSONObj() )
         {
            hint = obj.getObjectField( "hint" ) ;
         }
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting hint obj: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoFindCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_FINDBUILDREQ, "_mongoFindCommand::buildSdbRequest" )
INT32 _mongoFindCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                          mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_FINDBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32      rc      = SDB_OK ;
   MsgOpQuery *pQuery = NULL ;
   BSONObj cond, orderby, hint, selector ;
   BSONObjBuilder operatorBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = FLG_QUERY_WITH_RETURNDATA ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      cond     = _obj.getObjectField( "filter" ) ;
      orderby  = _obj.getObjectField( "sort" ) ;
      selector = _obj.getObjectField( "projection" ) ;

      rc = convertProjection( selector ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( _obj.hasField( "hint" ) )
      {
         hint = BSON( "" << _obj.getStringField( "hint" ) ) ;
      }
      if ( _obj.hasField( "limit" ) )
      {
         pQuery->numToReturn = _obj.getIntField( "limit" ) ;
      }
      if ( _obj.hasField( "skip" ) )
      {
         pQuery->numToSkip = _obj.getIntField( "skip" ) ;
      }

      rc = convertMongoOperator2Sdb( cond, operatorBob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert mongo operator to sdb operator, rc: %d",
                   rc ) ;

      rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( selector, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( orderby, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( hint, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb find request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_FINDBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_FINDBUILDMONGOREPLY, "_mongoFindCommand::buildMongoReply" )
INT32 _mongoFindCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf,
                                          _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_FINDBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   if ( SDB_OK      == sdbReply.flags ||
        SDB_DMS_EOC == sdbReply.flags ||
        SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
        SDB_DMS_NOTEXIST == sdbReply.flags )
   {
      if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
           SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf() ;
      }

      rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build first batch, rc: %d", rc ) ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_FINDBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetmoreCommand)
_mongoGetmoreCommand::_mongoGetmoreCommand()
: _nToReturn( 0 ),
  _cursorID( 0 ),
  _requestID( 0 ),
  _type( GETMORE_OTHER ),
  _isInitialized( FALSE ),
  _initMsgType( MONGO_UNKNOWN_MSG )
{}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREINIT, "_mongoGetmoreCommand::init" )
INT32 _mongoGetmoreCommand::init( const _mongoMessage *pMsg,
                                  mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREINIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;
   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;
      rc = _init( pReq ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_GETMORE_MSG == pMsg->type() )
   {
      const _mongoGetmoreRequest* pReq = (_mongoGetmoreRequest*)pMsg ;
      rc = _init( pReq ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;
      rc = _init( pReq ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   if ( 0 == _nToReturn )
   {
      _nToReturn = -1 ;
   }
   else if( _nToReturn < 0 )
   {
      _nToReturn = -_nToReturn ;
   }

   if ( string::npos != _clFullName.find( "$cmd.listCollections" ) )
   {
      _type = GETMORE_LISTCOLLECTION ;
   }
   else if ( string::npos != _clFullName.find( "$cmd.listIndexes" ) )
   {
      _type = GETMORE_LISTINDEX ;
   }
   else
   {
      _type = GETMORE_OTHER ;
   }

   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREINIT1, "_mongoGetmoreCommand::_init" )
INT32 _mongoGetmoreCommand::_init( const _mongoGetmoreRequest *pReq )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREINIT1 ) ;
   INT32 rc = SDB_OK ;
   const CHAR* pNameInReq = NULL ;
   const CHAR* ptr = NULL ;

   try
   {
      pNameInReq = pReq->fullCollectionName() ;
      _clFullName = pNameInReq ;

      ptr = ossStrstr( pNameInReq, FAP_MONGO_DOT ) ;
      PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
                "Invalid collectionFullName for mongo %s request: %s",
                name(), pNameInReq ) ;

      _csName.assign( pNameInReq, ptr - pNameInReq ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when initing getMore request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   _cursorID = pReq->cursorID() ;
   _nToReturn = pReq->nToReturn() ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_GETMORE_MSG ;
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREINIT1, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREINIT2, "_mongoGetmoreCommand::_init" )
INT32 _mongoGetmoreCommand::_init( const _mongoQueryRequest *pReq )
{
   // mongo message:
   // fullCollectionName: "foo.$cmd"
   // { getMore: <cursorID>, collection: "bar", batchSize: 1000 }
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREINIT2 ) ;
   INT32 rc = SDB_OK ;
   const CHAR* pClShortName = NULL ;
   const CHAR* pNameInReq = NULL ;
   BSONObj obj( pReq->query() ) ;
   const CHAR* ptr = NULL ;
   const CHAR* pCommandName = NULL ;

   pCommandName = obj.firstElementFieldName() ;
   SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
               "Invalid command name" ) ;

   rc = mongoGetStringElement( obj, FAP_MONGO_FIELD_NAME_COLLECTION,
                               pClShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_COLLECTION, rc ) ;

   pNameInReq = pReq->fullCollectionName() ;
   ptr = ossStrstr( pNameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), pNameInReq ) ;

   try
   {
      _csName.assign( pNameInReq, ptr - pNameInReq ) ;
      _clFullName = _csName ;
      _clFullName += "." ;
      _clFullName += pClShortName ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName and clName"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = mongoGetNumberLongElement( obj, pCommandName, _cursorID ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                pCommandName, rc ) ;

   rc = mongoGetIntElement( obj, FAP_MONGO_FIELD_NAME_BATCHSIZE, _nToReturn ) ;
   if ( SDB_FIELD_NOT_EXIST == rc )
   {
      rc = SDB_OK ;
   }
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_BATCHSIZE, rc ) ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREINIT2, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREINIT3, "_mongoGetmoreCommand::_init" )
INT32 _mongoGetmoreCommand::_init( const _mongoCommandRequest *pReq )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREINIT3 ) ;
   INT32 rc = SDB_OK ;
   const CHAR* pClShortName = NULL ;
   BSONObj obj( pReq->metadata() ) ;

   SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
               "Invalid command name" ) ;

   rc = mongoGetStringElement( obj, FAP_MONGO_FIELD_NAME_COLLECTION,
                               pClShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_COLLECTION, rc ) ;

   try
   {
      _csName = pReq->databaseName() ;
      _clFullName = _csName ;
      _clFullName += "." ;
      _clFullName += pClShortName ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting csName and clName"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = mongoGetNumberLongElement( obj, pReq->commandName(), _cursorID ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                pReq->commandName(), rc ) ;

   rc = mongoGetIntElement( obj, FAP_MONGO_FIELD_NAME_BATCHSIZE, _nToReturn ) ;
   if ( SDB_FIELD_NOT_EXIST == rc )
   {
      rc = SDB_OK ;
   }
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_BATCHSIZE, rc ) ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREINIT3, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREBUILDREQ, "_mongoGetmoreCommand::buildSdbRequest" )
INT32 _mongoGetmoreCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                             mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREBUILDREQ ) ;
   MsgOpGetMore *pGetMore = NULL ;
   INT32 rc = SDB_OK ;

   rc = sdbMsg.reserve( sizeof( MsgOpGetMore ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpGetMore ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pGetMore = ( MsgOpGetMore * )sdbMsg.data() ;
   pGetMore->header.opCode = MSG_BS_GETMORE_REQ ;
   pGetMore->header.TID = 0 ;
   pGetMore->header.routeID.value = 0 ;
   pGetMore->header.requestID = _requestID ;
   pGetMore->contextID = MGCURSOID_TO_SDBCTXID( _cursorID ) ;
   pGetMore->numToReturn = _nToReturn ;
   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_BUILDGETMOREREPLY, "_mongoGetmoreCommand::_buildGetmoreReply" )
INT32 _mongoGetmoreCommand::_buildGetmoreReply( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_BUILDGETMOREREPLY ) ;
   INT32 rc = SDB_OK ;
   mongoResponse res ;

   if ( SDB_OK == sdbReply.flags )
   {
      INT32 offset = 0 ;
      _msgBuf.zero() ;
      while ( offset < bodyBuf.size() )
      {
         BSONObj obj( bodyBuf.data() + offset ) ;
         offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;

         if ( GETMORE_LISTINDEX == _type )
         {
            rc = convertIndexObj( obj, _clFullName ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to convert index obj, rc: %d", rc ) ;
               goto error ;
            }
         }
         else if ( GETMORE_LISTCOLLECTION == _type )
         {
            rc = convertCollectionObj( obj ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to convert collection obj, rc: %d",
                       rc ) ;
               goto error ;
            }
         }

         rc = _msgBuf.write( obj.objdata(), obj.objsize() ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      bodyBuf = engine::rtnContextBuf( _msgBuf.data(),
                                       _msgBuf.size(),
                                       bodyBuf.recordNum() ) ;
   }
   else if ( SDB_DMS_EOC == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf() ;
   }

   res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
   res.header.responseTo = _requestID ;
   if ( SDB_RTN_CONTEXT_NOTEXIST == sdbReply.flags )
   {
      res.reservedFlags = MONGO_REPLY_FLAG_CURSOR_NOT_FOUND ;
   }
   if ( SDB_OK == sdbReply.flags )
   {
      res.cursorId = SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ;
   }
   res.startingFrom = sdbReply.startFrom ;
   res.nReturned = bodyBuf.recordNum() ;
   headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_BUILDGETMOREREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_BUILDQUERYREPLY, "_mongoGetmoreCommand::_buildQueryReply" )
INT32 _mongoGetmoreCommand::_buildQueryReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_BUILDQUERYREPLY ) ;
   INT32 rc = SDB_OK ;
   mongoResponse res ;

   if ( SDB_OK == sdbReply.flags || SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildNextBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build next batch, rc: %d", rc ) ;
   }

   res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
   res.header.responseTo = _requestID ;
   if ( SDB_RTN_CONTEXT_NOTEXIST == sdbReply.flags )
   {
      res.reservedFlags = MONGO_REPLY_FLAG_CURSOR_NOT_FOUND ;
   }
   res.nReturned = bodyBuf.recordNum() ;
   headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_BUILDQUERYREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_BUILDCOMMANDREPLY, "_mongoGetmoreCommand::_buildCommandReply" )
INT32 _mongoGetmoreCommand::_buildCommandReply( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_BUILDCOMMANDREPLY ) ;
   INT32 rc = SDB_OK ;
   mongoCommandResponse res ;

   if ( SDB_OK == sdbReply.flags || SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildNextBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build next batch, rc: %d", rc ) ;
   }

   rc = appendEmptyObj2Buf( bodyBuf, _msgBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to append empty obj to buf, rc: %d", rc ) ;
      goto error ;
   }

   res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
   res.header.responseTo = _requestID ;
   headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_BUILDCOMMANDREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREBUILDMONGOREPLY, "_mongoGetmoreCommand::buildMongoReply" )
INT32 _mongoGetmoreCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREBUILDMONGOREPLY ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc = SDB_OK ;

   if( MONGO_GETMORE_MSG == _initMsgType )
   {
      rc = _buildGetmoreReply( sdbReply, bodyBuf, headerBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build getMore reply, rc: %d", rc ) ;
         goto error ;
      }
   }
   else if( MONGO_QUERY_MSG == _initMsgType )
   {
      rc = _buildQueryReply( sdbReply, bodyBuf, headerBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build query reply, rc: %d", rc ) ;
         goto error ;
      }
   }
   else if( MONGO_COMMAND_MSG == _initMsgType )
   {
      rc = _buildCommandReply( sdbReply, bodyBuf, headerBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build command reply, rc: %d", rc ) ;
         goto error ;
      }
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_GETMOREBUILDNEXTBATCH, "_mongoGetmoreCommand::_buildNextBatch" )
INT32 _mongoGetmoreCommand::_buildNextBatch( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { nextBatch: [{xxx}, {xxx}...], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   PD_TRACE_ENTRY( SDB_FAPMONGO_GETMOREBUILDNEXTBATCH ) ;
   INT32 rc = SDB_OK ;
   bson::BSONObjBuilder resultBuilder ;
   bson::BSONObjBuilder cursorBuilder ;

   if ( SDB_OK != sdbReply.flags && SDB_DMS_EOC != sdbReply.flags )
   {
      goto done ;
   }

   try
   {
      bson::BSONArrayBuilder arr( cursorBuilder.subarrayStart( "nextBatch" ) ) ;
      INT32 offset = 0 ;
      BSONObjBuilder decimalConvertBob ;
      BOOLEAN hasDecimal = FALSE ;

      if ( SDB_OK == sdbReply.flags )
      {
         while ( offset < bodyBuf.size() )
         {
            bson::BSONObj obj( bodyBuf.data() + offset ) ;
            offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
            if ( GETMORE_LISTINDEX == _type )
            {
               rc = convertIndexObj( obj, _clFullName ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to convert index obj, rc: %d", rc ) ;
                  goto error ;
               }
               arr.append( obj ) ;
            }
            else if ( GETMORE_LISTCOLLECTION == _type )
            {
               rc = convertCollectionObj( obj ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to convert collection obj, rc: %d",
                          rc ) ;
                  goto error ;
               }
               arr.append( obj ) ;
            }
            else
            {
               rc = sdbDecimal2MongoDecimal( obj, decimalConvertBob,
                                             hasDecimal ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to convert sdb record to mongo record, "
                            "rc: %d", rc ) ;

               if ( hasDecimal )
               {
                  arr.append( decimalConvertBob.done() ) ;
                  decimalConvertBob.reset() ;
               }
               else
               {
                  arr.append( obj ) ;
               }
            }
         }
      }
      arr.done() ;

      /* getMore message may come from three command:
       * 1. listIndexes
       *           request: { getMore: <>, collection: "$cmd.listIndexes.bar" }
       *           reply:   { ... ns: "foo.$cmd.listIndexes.bar" ... }
       * 2. listCL
       *           request: { getMore: <>, collection: "$cmd.listCollections" }
       *           reply:   { ... ns: "foo.$cmd.listCollections" ... }
       * 3. real query
       *           request: { getMore: <>, collection: bar" }
       *           reply:   { ... ns: "foo.bar" ... }
       */
      cursorBuilder.append( "ns", _clFullName.c_str() ) ;
      cursorBuilder.append( "id", SDBCTXID_TO_MGCURSOID( sdbReply.contextID ) ) ;
      resultBuilder.append( "cursor", cursorBuilder.obj() ) ;
      resultBuilder.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building next batch"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_GETMOREBUILDNEXTBATCH, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoKillCursorCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_KILLCURSORINIT, "_mongoKillCursorCommand::init" )
INT32 _mongoKillCursorCommand::init( const _mongoMessage *pMsg,
                                     mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_KILLCURSORINIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;
   INT32 rc = SDB_OK ;

   if ( MONGO_KILL_CURSORS_MSG == pMsg->type() )
   {
      rc = _killCursorMsgInit( pMsg ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      rc = _queryMsgInit( pMsg ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      rc = _commandMsgInit( pMsg ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_KILLCURSORINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoKillCursorCommand::_killCursorMsgInit( const _mongoMessage *pMsg )
{
   INT32 rc = SDB_OK ;

   try
   {
      const _mongoKillCursorsRequest* pReq = (_mongoKillCursorsRequest*)pMsg ;
      const INT64 *pCursorID = pReq->cursorIDs() ;

      for( INT32 i = 0 ; i < pReq->numCursors() ; i++ )
      {
         _killCursorList.push_back( *pCursorID ) ;
         pCursorID += sizeof( INT64 ) ;
      }

      _initMsgType = MONGO_KILL_CURSORS_MSG ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when initing killCursor msg: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoKillCursorCommand::_queryMsgInit( const _mongoMessage *pMsg )
{
   INT32 rc = SDB_OK ;

   try
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;
      BSONObj arr ;
      BSONObj obj( pReq->query() ) ;

      const CHAR* pCommandName = obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      rc = mongoGetArrayElement( obj, FAP_MONGO_FIELD_NAME_CURSORS, arr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s], rc: %d",
                   FAP_MONGO_FIELD_NAME_CURSORS, rc ) ;
      {
         BSONObjIterator it( arr ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next() ;
            PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid object[%s] in mongo %s request",
                      obj.toString().c_str(), name() ) ;
            _killCursorList.push_back( ele.numberLong() ) ;
         }
      }

      _initMsgType = MONGO_QUERY_MSG ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when initing query msg: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoKillCursorCommand::_commandMsgInit( const _mongoMessage *pMsg )
{
   INT32 rc = SDB_OK ;

   try
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;
      BSONObj obj, arr ;

      PD_CHECK( 0 == ossStrcmp( pReq->commandName(), name() ),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid command name for mongo %s request: %s",
                name(), pReq->commandName() ) ;

      obj = BSONObj( pReq->metadata() ) ;
      rc = mongoGetArrayElement( obj, FAP_MONGO_FIELD_NAME_CURSORS, arr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s], rc: %d",
                   FAP_MONGO_FIELD_NAME_CURSORS, rc ) ;
      {
         BSONObjIterator it( arr ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next() ;
            PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid object[%s] in mongo %s request",
                      obj.toString().c_str(), name() ) ;
            _killCursorList.push_back( ele.numberLong() ) ;
         }
      }

      _initMsgType = MONGO_COMMAND_MSG ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when initing command msg: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_KILLCURSORBUILDREQ, "_mongoKillCursorCommand::buildSdbRequest" )
INT32 _mongoKillCursorCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_KILLCURSORBUILDREQ ) ;
   INT32 rc                 = SDB_OK ;
   MsgOpKillContexts *pKill = NULL ;

   if ( _killCursorList.size() > 1 )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      ctx.setError( rc, "Killing two or more cursors is not supported " ) ;
      goto error ;
   }

   rc = sdbMsg.reserve( sizeof( MsgOpKillContexts ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpKillContexts ) - sizeof( SINT64 ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pKill = ( MsgOpKillContexts * )sdbMsg.data() ;
   pKill->header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
   pKill->header.TID = 0 ;
   pKill->header.routeID.value = 0 ;
   pKill->header.requestID = _requestID ;
   pKill->ZERO = 0 ;
   pKill->numContexts = _killCursorList.size() ;

   for( vector<INT64>::iterator it = _killCursorList.begin() ;
        it != _killCursorList.end() ; it++ )
   {
      INT64 contextID = MGCURSOID_TO_SDBCTXID( *it ) ;
      if ( contextID != SDB_INVALID_CONTEXTID )
      {
         rc = sdbMsg.write( (CHAR*)&contextID, sizeof( SINT64 ) ) ;
         if ( rc )
         {
            goto error ;
         }
      }
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_KILLCURSORBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_KILLCURSORBUILDMONGOREPLY, "_mongoKillCursorCommand::buildMongoReply" )
INT32 _mongoKillCursorCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_KILLCURSORBUILDMONGOREPLY ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( MONGO_KILL_CURSORS_MSG == _initMsgType )
      {
         // do not reply
         headerBuf.usedSize = 0 ;
      }
      else if ( MONGO_QUERY_MSG == _initMsgType )
      {
         if ( SDB_OK == sdbReply.flags )
         {
            bodyBuf = engine::rtnContextBuf(
                      BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
         }
         mongoResponse res ;
         res.header.msgLen = sizeof( mongoResponse ) + bodyBuf.size() ;
         res.header.responseTo = _requestID ;
         res.nReturned = bodyBuf.recordNum() ;
         headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
      }
      else if ( MONGO_COMMAND_MSG == _initMsgType )
      {
         if ( SDB_OK == sdbReply.flags )
         {
            bodyBuf = engine::rtnContextBuf(
                      BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
         }

         rc = appendEmptyObj2Buf( bodyBuf, _msgBuf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to append empty obj to buf, rc: %d", rc ) ;
            goto error ;
         }

         mongoCommandResponse res ;
         res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
         res.header.responseTo = _requestID ;
         headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo killCursor"
              " reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_KILLCURSORBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCountCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_COUNTBUILDSDBREQ, "_mongoCountCommand::buildSdbRequest" )
INT32 _mongoCountCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                           mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_COUNTBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *pQuery = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT ;
   BSONObj empty, hint, matcher ;
   BSONObjBuilder operatorBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto done ;
   }

   try
   {
      if ( _obj.hasField( "skip" ) )
      {
         pQuery->numToSkip = _obj.getIntField( "skip" ) ;
      }
      if ( _obj.hasField( "limit" ) )
      {
         pQuery->numToReturn = _obj.getIntField( "limit" ) ;
      }

      // mongo command: db.bar.count( { a: 1 }, { hint: "aIdx" } )
      // sdb msg: matcher: { a: 1 }
      //          hint:    { Collection: "bar", Hint: { "": "aIdx" } }
      matcher = _obj.getObjectField( "query" ) ;

      if ( _obj.hasField( "hint" ) )
      {
         hint = BSON( FIELD_NAME_COLLECTION <<
                      _clFullName.c_str() <<
                      FIELD_NAME_HINT <<
                      BSON( "" << _obj.getStringField( "hint" ) ) ) ;
      }
      else
      {
         hint = BSON( FIELD_NAME_COLLECTION << _clFullName.c_str() ) ;
      }

      rc = convertMongoOperator2Sdb( matcher, operatorBob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert mongo operator to sdb operator, rc: %d",
                   rc ) ;

      rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( hint, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb count request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_COUNTBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_COUNTBUILDMONGOREPLY, "_mongoCountCommand::buildMongoReply" )
INT32 _mongoCountCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                           engine::rtnContextBuf &bodyBuf,
                                           _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_COUNTBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         // reply: { n: 1, ok: 1 }
         BSONObj resObj( bodyBuf.data() ) ;
         BSONObjBuilder bob ;
         bob.append( "n", resObj.getIntField( "Total" ) ) ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
                SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         BSONObjBuilder bob ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bob.append( "n", 0 ) ;
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo count reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_COUNTBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoAggregateCommand)
INT32 _mongoAggregateCommand::_convertAggrSumIfExist( const BSONElement& ele,
                                                      BSONObjBuilder& builder )
{
   INT32 rc = SDB_OK ;

   try
   {
      if ( ele.isABSONObj() && ele.Obj().getIntField( "$sum" ) == 1 )
      {
         // { $group: { _id: ..., total: { $sum: 1 } } } =>
         // { $group: { _id: ..., total: { $count: "$_id" } } }
         builder.append( ele.fieldName(), BSON( "$count" << "$_id" ) ) ;
      }
      else
      {
         builder.append( ele ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when coverting mongo $sum to sdb"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoAggregateCommand::_convertAggrGroup( const BSONObj& groupObj,
                                                 vector<BSONObj>& newStageList )
{
   /* cl.aggregate( { $group: { _id: "$a" ) )
    * MongoDB return { _id: <value of a> }, but SequoiaDB return all fields.
    * So we should convert:
    * { $group: { _id: "$a", total: { $sum: "$b" } } } ==>
    * { $group: { _id: "$a", total: { $sum: "$b" }, tmp_id_field: "$a" } },
    * { $project: { _id: "$tmp_id_field", total: 1 } }
    */
   INT32 rc = SDB_OK ;

   if ( 0 != ossStrcmp( groupObj.firstElementFieldName(), "$group" ) )
   {
      goto done ;
   }

   try
   {
      BSONObj groupValue = groupObj.getObjectField( "$group" ) ;
      const CHAR* pIdValue = groupValue.getStringField( "_id" ) ;

      if ( '$' == pIdValue[0] )
      {
         BSONObjBuilder bobGroup, bobProj ;
         BSONObjIterator itr( groupValue ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            rc = _convertAggrSumIfExist( e, bobGroup ) ;
            if ( rc )
            {
               goto error ;
            }

            if ( 0 == ossStrcmp( e.fieldName(), "_id" ) )
            {
               bobProj.append( "_id", "$tmp_id_field" ) ;
            }
            else
            {
               bobProj.append( e.fieldName(), 1 ) ;
            }
         }
         bobGroup.append( "tmp_id_field", pIdValue ) ;

         newStageList.push_back( BSON( "$group" << bobGroup.obj() ) ) ;
         newStageList.push_back( BSON( "$project" << bobProj.obj() ) ) ;
      }
      else
      {
         // When Pyrhon driver execute count_documents command, it will enter
         // this branch.

         // In MongoDB, if the value of _id field is null or any other constant
         // value, it means no grouping.
         // In SequoiaDB, the value[0] of _id field must be '$',
         // otherwise an error -6 will be reported. If _id field isn't specified,
         // it means no grouping.

         // eg: groupValue = { "$group": { xxx, "_id": 1 } }.
         // So we should convert groupValue to { "$group": { xxx } }
         BSONObjBuilder bobGroup ;
         BSONObjIterator itr( groupValue ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 == ossStrcmp( "_id", e.fieldName() ) )
            {
               continue ;
            }
            rc = _convertAggrSumIfExist( e, bobGroup ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         newStageList.push_back( BSON( "$group" << bobGroup.obj() ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when coverting mongo $group to "
              "sdb: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoAggregateCommand::_convertAggrProject( BSONObj& projectObj,
                                                   BSONObj& errorObj )
{
   INT32 rc = SDB_OK ;

   if ( 0 != ossStrcmp( projectObj.firstElementFieldName(), "$project" ) )
   {
      goto done ;
   }

   try
   {
      /* MongoDB return _id field by default, but SequoiaDB doesn't return _id
       * field by default. So we should convert:
       * { $project: { a: 1 } }  ==>  { $project: { id: 1, a: 1 } }
       *
       * MongoDB support exclusion of fields, but SequoiaDB not support.
       * { $project: { a: 0, b: 0 } } is not supported.
       * { $project: { a: 1, _id: 0 } } is supported.
       */
      BSONObj projValue = projectObj.getObjectField( "$project" ) ;
      BOOLEAN foundId = FALSE ;
      BOOLEAN foundInclude = FALSE ;
      BSONObjIterator i( projValue );

      if ( projValue.isEmpty() )
      {
         goto done ;
      }

      while ( i.more() )
      {
         BSONElement e = i.next();
         if ( !foundId && 0 == ossStrcmp( e.fieldName(), "_id" ) )
         {
            foundId = TRUE ;
         }
         if ( !foundInclude && e.trueValue() )
         {
            foundInclude = TRUE ;
         }
      }

      if ( !foundInclude )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         BSONObjBuilder builder ;
         builder.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
         builder.append( FAP_MONGO_FIELD_NAME_ERRMSG,
                         "Exclusion fields is not supported" ) ;
         builder.append( FAP_MONGO_FIELD_NAME_CODE, rc ) ;
         errorObj = builder.obj() ;
         goto error ;
      }

      if ( !foundId )
      {
         BSONObjBuilder builder ;
         builder.append( "_id", 1 ) ;
         builder.appendElements( projValue ) ;
         projectObj = BSON( "$project" << builder.obj() ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when coverting mongo $project to "
              "sdb: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AGGRBUILDREQ, "_mongoAggregateCommand::buildSdbRequest" )
INT32 _mongoAggregateCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                               mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AGGRBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpAggregate *pAggre  = NULL ;

   rc = sdbMsg.reserve( sizeof( MsgOpAggregate ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpAggregate ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pAggre = ( MsgOpAggregate * )sdbMsg.data() ;
   pAggre->header.opCode = MSG_BS_AGGREGATE_REQ ;
   pAggre->header.TID = 0 ;
   pAggre->header.routeID.value = 0 ;
   pAggre->header.requestID = _requestID ;
   pAggre->version = 0 ;
   pAggre->w = 0 ;
   pAggre->padding = 0 ;
   pAggre->flags = 0 ;
   pAggre->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pAggre->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      /* eg: { pipeline: [ { $match: { b: 1 } },
                           { $group: { _id: "$a", b: { $sum: "$b" } } }
                         ] } */
      BSONObjIterator it( _obj.getObjectField( "pipeline" ) ) ;
      while ( it.more() )
      {
         BSONElement ele = it.next() ;
         if ( ele.type() != Object )
         {
            rc = sdbMsg.write( ele.rawdata(), ele.size(), TRUE ) ;
            if ( rc )
            {
               goto error ;
            }
            continue ;
         }

         BSONObj oneStage = ele.Obj() ;
         if ( 0 == ossStrcmp( oneStage.firstElementFieldName(), "$project" ) )
         {
            rc = _convertAggrProject( oneStage, ctx.errorObj ) ;
            if ( rc )
            {
               goto error ;
            }

            rc = sdbMsg.write( oneStage, TRUE ) ;
            if ( rc )
            {
               goto error;
            }
         }
         else if ( 0 == ossStrcmp( oneStage.firstElementFieldName(), "$group" ) )
         {
            std::vector<BSONObj> newStageList ;

            rc = _convertAggrGroup( oneStage, newStageList ) ;
            if ( rc )
            {
               goto error ;
            }

            for( std::vector<BSONObj>::iterator it = newStageList.begin() ;
                 it != newStageList.end() ; it++ )
            {
               rc = sdbMsg.write( *it, TRUE ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
         else if ( 0 == ossStrcmp( oneStage.firstElementFieldName(), "$match" ) )
         {
            BSONObjBuilder operatorBob ;

            rc = convertMongoOperator2Sdb( oneStage, operatorBob ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to convert mongo operator to sdb operator, ",
                         "rc: %d", rc ) ;

            rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else
         {
            rc = sdbMsg.write( oneStage, TRUE ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb aggr request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AGGRBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AGGRBUILDMONGOREPLY, "_mongoAggregateCommand::buildMongoReply" )
INT32 _mongoAggregateCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                               engine::rtnContextBuf &bodyBuf,
                                               _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AGGRBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( _obj.hasField( "cursor" ) )
      {
         if ( SDB_OK      == sdbReply.flags ||
              SDB_DMS_EOC == sdbReply.flags ||
              SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
              SDB_DMS_NOTEXIST == sdbReply.flags )
         {
            if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
                 SDB_DMS_NOTEXIST == sdbReply.flags )
            {
               bodyBuf = engine::rtnContextBuf() ;
            }

            rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build first batch, rc: %d", rc ) ;
         }

         rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         // reply: { result: [ {xxx}, {xxx}, ... ], ok: 1 }
         if ( SDB_OK == sdbReply.flags )
         {
            BSONObjBuilder bob ;
            BSONArrayBuilder arr( bob.subarrayStart( "result" ) ) ;
            INT32 offset = 0 ;
            while ( offset < bodyBuf.size() )
            {
               BSONObj obj( bodyBuf.data() + offset ) ;
               arr.append( obj ) ;
               offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
            }
            arr.done() ;
            bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;

            bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
         }
         else if ( SDB_DMS_EOC == sdbReply.flags ||
                   SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
                   SDB_DMS_NOTEXIST == sdbReply.flags )
         {
            bodyBuf = engine::rtnContextBuf( BSON( "result" << BSONArray() <<
                                                   FAP_MONGO_FIELD_NAME_OK <<
                                                   1 ) ) ;
         }

         rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
            goto error ;
         }
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo aggr reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AGGRBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDistinctCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DISTINCTBUILDREQ, "_mongoDistinctCommand::buildSdbRequest" )
INT32 _mongoDistinctCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                              mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DISTINCTBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   /* mongo request:
    * { distinct: "bar", key: "a", query: { b: 1 } }
    */
   INT32 rc = SDB_OK ;
   MsgOpAggregate *pAggre   = NULL ;
   BSONObj match, group1, group2 ;
   BSONObjBuilder builder, operatorBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpAggregate ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpAggregate ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pAggre = ( MsgOpAggregate * )sdbMsg.data() ;
   pAggre->header.opCode = MSG_BS_AGGREGATE_REQ ;
   pAggre->header.TID = 0 ;
   pAggre->header.routeID.value = 0 ;
   pAggre->header.requestID = _requestID ;
   pAggre->version = 0 ;
   pAggre->w = 0 ;
   pAggre->padding = 0 ;
   pAggre->flags = 0 ;
   pAggre->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pAggre->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      std::string distinctfield = "$" ;
      distinctfield += _obj.getStringField( "key" ) ;

      // distinct( "a", { b: 1 } ) =>
      // { $match: { b: 1 } },
      // { $group: { _id: "$a" } },
      // { $group: { _id: null, values: { $addtoset: "$a" } } }
      if ( _obj.hasField( "query" ) )
      {
         match = BSON( "$match" << _obj.getField( "query" ) ) ;
      }

      group1 = BSON( "$group" << BSON( "_id" << distinctfield ) ) ;

      builder.appendNull( "_id" ) ;
      builder.append( "values", BSON( "$addtoset" << distinctfield ) ) ;
      group2 = BSON( "$group" << builder.done() ) ;

      if ( !match.isEmpty() )
      {
         rc = convertMongoOperator2Sdb( match, operatorBob ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to convert mongo operator to sdb operator, rc: %d",
                      rc ) ;
         rc = sdbMsg.write( operatorBob.obj(), TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      rc = sdbMsg.write( group1, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( group2, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb distinct request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DISTINCTBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DISTINCTBUILDMONGOREPLY, "_mongoDistinctCommand::buildMongoReply" )
INT32 _mongoDistinctCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DISTINCTBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      // reply: { values: [ 1, 3, 4 ], ok: 1 }
      if ( SDB_OK == sdbReply.flags )
      {
         BSONObjBuilder bob ;
         bob.appendElements( BSONObj( bodyBuf.data() ) ) ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_EOC == sdbReply.flags ||
                SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
                SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( "values" << BSONArray() <<
                                                FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo distinct reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DISTINCTBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCreateCLCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRCLBUILDREQ, "_mongoCreateCLCommand::buildSdbRequest" )
INT32 _mongoCreateCLCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                              mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRCLBUILDREQ ) ;
   INT32 rc                = SDB_OK ;
   MsgOpQuery *pQuery      = NULL ;
   const CHAR *pCmdName    = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
   BSONObj cond, empty ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   try
   {
      cond = BSON( FIELD_NAME_NAME << _clFullName.c_str() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb createCL request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( cond, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRCLBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRCLBUILDMONGOREPLY, "_mongoCreateCLCommand::buildMongoReply" )
INT32 _mongoCreateCLCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRCLBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf(
            BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo createCL "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRCLBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropCLCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPCLBUILDREQ, "_mongoDropCLCommand::buildSdbRequest" )
INT32 _mongoDropCLCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                            mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPCLBUILDREQ ) ;
   INT32 rc = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ;
   BSONObj cond, empty ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   try
   {
      cond = BSON( FIELD_NAME_NAME << _clFullName.c_str() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb dropCL request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( cond, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPCLBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPCLBUILDMONGOREPLLY, "_mongoDropCLCommand::buildMongoReply" )
INT32 _mongoDropCLCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                            engine::rtnContextBuf &bodyBuf,
                                            _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPCLBUILDMONGOREPLLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( "ns" << _clFullName.c_str() <<
                                                FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
      else if ( SDB_DMS_NOTEXIST == sdbReply.flags ||
                SDB_DMS_CS_NOTEXIST == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( FAP_MONGO_FIELD_NAME_OK << 0 <<
                                                FAP_MONGO_FIELD_NAME_ERRMSG <<
                                                "ns not found" ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo createCL reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPCLBUILDMONGOREPLLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListIdxCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTIDXBUILDREQ, "_mongoListIdxCommand::buildSdbRequest" )
INT32 _mongoListIdxCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                             mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTIDXBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES ;
   BSONObj empty, cond ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      cond = BSON( FIELD_NAME_COLLECTION << _clFullName.c_str() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb listIdx request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   rc = sdbMsg.write( cond, TRUE ) ;
   if ( rc )
   {
      goto error;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTIDXBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTIDXBUILDMONGOREPLLY, "_mongoListIdxCommand::buildMongoReply" )
INT32 _mongoListIdxCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTIDXBUILDMONGOREPLLY ) ;
   INT32 rc = SDB_OK ;

   if ( SDB_OK      == sdbReply.flags ||
        SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build first batch, rc: %d", rc ) ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTIDXBUILDMONGOREPLLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCreateIdxCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRIDXBUILDREQ, "_mongoCreateIdxCommand::buildSdbRequest" )
INT32 _mongoCreateIdxCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                               mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRIDXBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpQuery *pQuery      = NULL ;
   const CHAR *pCmdName    = CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX ;
   BSONObj objList, obj, indexObj ;
   BSONObjBuilder bob ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   // { createIndexes: "bar", indexes: [ { key: {a:1}, name: "aIdx" } ] }
   rc = mongoGetArrayElement( _obj, FAP_MONGO_FIELD_NAME_INDEXES, objList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_INDEXES, rc ) ;

   try
   {
      if ( !_hasBuildMsgVec )
      {
         BSONObjIterator itr( objList ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid indexes field. The type of element in "
                       "indexes must be Object, rc: %d", rc ) ;
               goto error ;
            }
            _msgVec.push_back( ele ) ;
         }
         _hasBuildMsgVec = TRUE ;
      }

      obj = _msgVec[_msgIndex++].embeddedObject() ;
      _hasProcessAllMsg = ( _msgIndex >= _msgVec.size() ) ;

      // mongo unique index => sequoiadb unique enforce index
      // { key: {a:1}, name: "aIdx", unique: true } =>
      // { Index: { key: {a:1}, name: "aIdx", unique: true, enforced: true },
      //   Collection: "foo.bar" }
      indexObj = BSON( IXM_FIELD_NAME_KEY <<
                       obj.getObjectField( "key" ) <<
                       IXM_FIELD_NAME_NAME <<
                       obj.getStringField( "name") <<
                       IXM_FIELD_NAME_UNIQUE <<
                       obj.getBoolField( "unique" ) <<
                       IXM_FIELD_NAME_ENFORCED <<
                       obj.getBoolField( "unique" ) );
      bob.append( FIELD_NAME_INDEX, indexObj ) ;
      bob.append( FIELD_NAME_COLLECTION, _clFullName.c_str() ) ;

      rc = sdbMsg.write( bob.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( BSONObj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( BSONObj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( BSONObj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb createIdx "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRIDXBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRIDXBUILDMONGOREPLLY, "_mongoCreateIdxCommand::buildMongoReply" )
INT32 _mongoCreateIdxCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                               engine::rtnContextBuf &bodyBuf,
                                               _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRIDXBUILDMONGOREPLLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags ||
           SDB_IXM_REDEF == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf(
            BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo createIdx "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRIDXBUILDMONGOREPLLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDeleteIdxCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropIdxCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPIDXBUILDREQ, "_mongoDropIdxCommand::buildSdbRequest" )
INT32 _mongoDropIdxCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                             mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPIDXBUILDREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX ;
   BSONObj empty, cond ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      // mongo message:
      // { deleteIndexes: "bar", index: "aIdx" } or
      // { dropIndexes: "bar", index: "aIdx" }
      cond = BSON( FIELD_NAME_COLLECTION <<
                   _clFullName.c_str() <<
                   FIELD_NAME_INDEX <<
                   BSON( "" << _obj.getStringField( "index" ) ) ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb dropIdx request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( cond, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPIDXBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPIDXBUILDMONGOREPLLY, "_mongoDropIdxCommand::buildMongoReply" )
INT32 _mongoDropIdxCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPIDXBUILDMONGOREPLLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf(
            BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo dropIdx reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPIDXBUILDMONGOREPLLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropDatabaseCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPDBBUILDREQ, "_mongoDropDatabaseCommand::buildSdbRequest" )
INT32 _mongoDropDatabaseCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                  mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPDBBUILDREQ ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE ;
   BSONObj obj, empty ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      obj = BSON( FIELD_NAME_NAME << _csName.c_str() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb dropDB request"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( obj, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPDBBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPDBBUILDMONGOREPLLY, "_mongoDropDatabaseCommand::buildMongoReply" )
INT32 _mongoDropDatabaseCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                  engine::rtnContextBuf &bodyBuf,
                                                  _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPDBBUILDMONGOREPLLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( "dropped" << _csName.c_str() <<
                                              FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
      else if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf(
            BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo dropDB reply"
              ": %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPDBBUILDMONGOREPLLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCreateUserCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRUSERINIT, "_mongoCreateUserCommand::init" )
INT32 _mongoCreateUserCommand::init( const _mongoMessage *pMsg,
                                     mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRUSERINIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* pCommandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRUSERINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRUSERBUILDREQ, "_mongoCreateUserCommand::buildSdbRequest" )
INT32 _mongoCreateUserCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRUSERBUILDREQ ) ;
   INT32 rc = SDB_OK ;
   MsgAuthCrtUsr *pCtrUser = NULL ;
   BSONObj obj ;
   const CHAR *pUserName = NULL ;
   const CHAR *pTextPasswd = NULL ;
   BOOLEAN digestPasswd = TRUE ;
   string  md5PwdStr ;

   rc = sdbMsg.reserve( sizeof( MsgAuthCrtUsr ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgAuthCrtUsr ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pCtrUser = ( MsgAuthCrtUsr * )sdbMsg.data() ;
   pCtrUser->header.opCode = MSG_AUTH_CRTUSR_REQ ;
   pCtrUser->header.TID = 0 ;
   pCtrUser->header.routeID.value = 0 ;
   pCtrUser->header.requestID = _requestID ;

   try
   {
      BSONObjIterator itr( _obj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();

         if ( 0 == ossStrcmp( ele.fieldName(), name() ) )
         {
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "Username must be string" ) ;
               goto error ;
            }
            pUserName = ele.valuestr() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FAP_MONGO_FIELD_NAME_PWD ) )
         {
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "Passwd must be string" ) ;
               goto error ;
            }
            pTextPasswd = ele.valuestr() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FAP_MONGO_FIELD_NAME_DIGESTPWD ) )
         {
            if ( !ele.isNumber() && Bool != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "DigestPassword must be number or bool" ) ;
               goto error ;
            }
            digestPasswd = ele.trueValue() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                   FAP_MONGO_FIELD_NAME_MECHANISMS ) )
         {
            rc = _checkAuthMechanisms( ele, ctx ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }

      if ( NULL == pUserName || NULL == pTextPasswd )
      {
         rc = SDB_INVALIDARG ;
         ctx.setError( rc, "Username or passwd can't be empty" ) ;
         goto error ;
      }

      if ( FALSE == digestPasswd )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         ctx.setError( rc, "'digestPasswd' should be true, "
                       "use 'db.runCommand( { createUser: ... } )' instead" ) ;
         goto error ;
      }

      md5PwdStr = md5::md5simpledigest( pTextPasswd ) ;

      obj = BSON( SDB_AUTH_USER << pUserName <<
                  SDB_AUTH_PASSWD << md5PwdStr.c_str() <<
                  SDB_AUTH_TEXTPASSWD << pTextPasswd ) ;
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb createUser "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( obj, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRUSERBUILDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_CTRUSERBUILDMONGOREPLLY, "_mongoCreateUserCommand::buildMongoReply" )
INT32 _mongoCreateUserCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_CTRUSERBUILDMONGOREPLLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf(
            BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo createUser "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_CTRUSERBUILDMONGOREPLLY, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoCreateUserCommand::_checkAuthMechanisms( const BSONElement &mechanismsEle,
                                                     mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;
   const CHAR* pMechanismsStr = NULL ;
   CHAR  errMsg[64] = { 0 } ;

   try
   {
      if ( Array != mechanismsEle.type() )
      {
         rc = SDB_INVALIDARG ;
         ctx.setError( rc, "Mechanisms field must be an array" ) ;
         goto error ;
      }

      BSONObjIterator itr( mechanismsEle.embeddedObject() ) ;
      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;

         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            ctx.setError( rc, "Mechanisms field must be an array of strings" ) ;
            goto error ;
         }

         pMechanismsStr = ele.valuestr() ;

         if ( 0 == ossStrcmp( pMechanismsStr, FAP_MONGO_FIELD_VALUE_SCRAMSHA1 ) )
         {
            continue ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            ossSnprintf( errMsg, 64, "Unsupported auth mechanism[%s]",
                         pMechanismsStr ) ;
            ctx.setError( rc, errMsg ) ;
            goto error ;
         }
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when checking auth mechanisms: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropUserCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPUSERINIT, "_mongoDropUserCommand::init" )
INT32 _mongoDropUserCommand::init( const _mongoMessage *pMsg,
                                   mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPUSERINIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* pCommandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPUSERINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPUSERBUILDSDBREQ, "_mongoDropUserCommand::buildSdbRequest" )
INT32 _mongoDropUserCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                              mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPUSERBUILDSDBREQ ) ;
   INT32 rc = SDB_OK ;
   MsgAuthDelUsr *pAuth = NULL ;
   BSONObj obj ;
   const CHAR* pUserName = NULL ;

   rc = sdbMsg.reserve( sizeof( MsgAuthDelUsr ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgAuthDelUsr ) ) ;
   if ( rc )
   {
      goto error ;
   }

   pAuth = ( MsgAuthDelUsr * )sdbMsg.data() ;
   pAuth->header.opCode = MSG_AUTH_DELUSR_REQ ;
   pAuth->header.TID = 0 ;
   pAuth->header.routeID.value = 0 ;
   pAuth->header.requestID = _requestID ;

   try
   {
      // eg: { dropUser: "myuser" }
      pUserName = _obj.getStringField( name() ) ;
      if ( 0 != ossStrcmp( pUserName, ctx.userName.c_str() ) )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         ctx.setError( rc, "Only current user can be dropped" ) ;
         goto error ;
      }

      obj = BSON( SDB_AUTH_USER << pUserName <<
                  SDB_AUTH_NONCE << ctx.authInfo.nonce <<
                  SDB_AUTH_IDENTIFY << ctx.authInfo.identify <<
                  SDB_AUTH_PROOF << ctx.authInfo.clientProof <<
                  SDB_AUTH_TYPE << ctx.authInfo.type ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb dropUser "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( obj, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPUSERBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_DROPUSERBUILDMONGOREPLY, "_mongoDropUserCommand::buildMongoReply" )
INT32 _mongoDropUserCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_DROPUSERBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf(
            BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo dropUser "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_DROPUSERBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListUserCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTUSERINIT, "_mongoListUserCommand::init" )
INT32 _mongoListUserCommand::init( const _mongoMessage *pMsg,
                                   mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTUSERINIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* pCommandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTUSERINIT, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoListUserCommand::_getUsernames( vector<string> &usernames )
{
   INT32       rc = SDB_OK ;
   BSONElement usersInfoEle ;
   BSONType    type ;

   /*
      The format of _obj is as follows:

      1. if we execute getUsers(), its format is { usersInfo: 1 }

      2. if we execute getUser( "admin" ), its format is { usersInfo: "admin" }

      3. if we execute
         runCommand( { usersInfo: [ { user: "admin", db: dbName }, ... ] } ),
         its format is { usersInfo: [ { user: "admin", db: dbName }, ... ] }
   */

   try
   {
      usersInfoEle = _obj.getField( FAP_MONGO_FIELD_NAME_USERSINFO ) ;
      type = usersInfoEle.type() ;

      if ( String == type )
      {
         usernames.push_back( usersInfoEle.String() ) ;
      }
      else if ( NumberDouble == type || NumberInt == type )
      {
         goto done ;
      }
      else if ( Array == type )
      {
         vector<BSONElement> usernamesArr = usersInfoEle.Array() ;

         for ( UINT32 i = 0; i < usernamesArr.size(); i++ )
         {
            BSONObj usernameObj = usernamesArr[i].Obj() ;
            usernames.push_back(
               usernameObj.getStringField( FAP_MONGO_FIELD_NAME_USER ) ) ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when getting usernames: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTUSERBUILDSDBREQ, "_mongoListUserCommand::buildSdbRequest" )
INT32 _mongoListUserCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                              mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTUSERBUILDSDBREQ ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_USERS ;
   BSONObj empty ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTUSERBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTUSERBUILDMONGOREPLY, "_mongoListUserCommand::buildMongoReply" )
INT32 _mongoListUserCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTUSERBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         BSONObjBuilder bob ;
         BSONArrayBuilder arr(
            bob.subarrayStart( FAP_MONGO_FIELD_NAME_USERS ) ) ;
         INT32   offset = 0 ;
         BOOLEAN returnAllUsers = FALSE ;
         vector<string> usernames ;

         rc = _getUsernames( usernames ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get usernames, rc: %d", rc ) ;

         if ( 0 == usernames.size() )
         {
            returnAllUsers = TRUE ;
         }

         while ( offset < bodyBuf.size() )
         {
            // { User: "myuser", ... } => { user: "myuser" }
            BSONObj obj( bodyBuf.data() + offset ) ;
            const CHAR* pUser = obj.getStringField( FIELD_NAME_USER ) ;

            if ( returnAllUsers )
            {
               arr.append( BSON( FAP_MONGO_FIELD_NAME_USER << pUser ) ) ;
            }
            else
            {
               for ( UINT32 i = 0; i < usernames.size(); i++ )
               {
                  if ( 0 == ossStrcmp( usernames[i].c_str(), pUser ) )
                  {
                     arr.append( BSON( FAP_MONGO_FIELD_NAME_USER << pUser ) ) ;
                  }
               }
            }
            offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         }
         arr.done() ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;

         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_EOC == sdbReply.flags )
      {
         BSONObj obj = BSON( FAP_MONGO_FIELD_NAME_USERS << BSONArray() <<
                             FAP_MONGO_FIELD_NAME_OK << 1 ) ;
         bodyBuf = engine::rtnContextBuf( obj ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo listUser "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTUSERBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoSaslStartCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTH1INIT, "_mongoSaslStartCommand::init" )
INT32 _mongoSaslStartCommand::init( const _mongoMessage *pMsg,
                                    mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTH1INIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* pCommandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTH1INIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTH1BUILDSDBREQ, "_mongoSaslStartCommand::buildSdbRequest" )
INT32 _mongoSaslStartCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                               mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTH1BUILDSDBREQ ) ;
   INT32 rc = SDB_OK ;
   MsgAuthentication *pAuth = NULL ;
   const CHAR* pUserName = NULL ;
   const CHAR* pClientNonce = NULL ;
   CHAR* p = NULL ;
   CHAR* nextPtr = NULL ;
   const CHAR* pPayload = NULL ;
   INT32 payloadLen = 0 ;
   CHAR payloadCpy[ FAP_MONGO_PAYLOADD_MAX_SIZE ] = { 0 } ;
   BSONObj obj ;

   try
   {
      // get payload
      BSONElement ele = _obj.getField( FAP_MONGO_FIELD_NAME_PAYLOAD ) ;
      pPayload = ele.binData( payloadLen ) ;
      if ( payloadLen > FAP_MONGO_PAYLOADD_MAX_SIZE )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemcpy( payloadCpy, pPayload, payloadLen ) ;

     /* The two allowed payload forms are:
      * n,,n=encoded-username,r=client-nonce
      * n,a=authzid,n=encoded-username,r=client-nonce
      */
      p = ossStrtok( payloadCpy, FAP_MONGO_COMMA, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pUserName = p + 2 ;

      p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pClientNonce = p + 2 ;

      rc = sdbMsg.reserve( sizeof( MsgAuthentication ) ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.advance( sizeof( MsgAuthentication ) ) ;
      if ( rc )
      {
         goto error ;
      }

      pAuth = ( MsgAuthentication * ) sdbMsg.data() ;
      pAuth->header.opCode = MSG_AUTH_VERIFY1_REQ ;
      pAuth->header.TID = 0 ;
      pAuth->header.routeID.value = 0 ;
      pAuth->header.requestID = _requestID ;

      obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_1 <<
                  SDB_AUTH_USER << pUserName <<
                  SDB_AUTH_NONCE << pClientNonce <<
                  SDB_AUTH_TYPE << SDB_AUTH_TYPE_EXTEND_PWD ) ;

      rc = sdbMsg.write( obj, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;

      ctx.userName = pUserName ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb auth1 "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTH1BUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTH1BUILDMONGOREPLY, "_mongoSaslStartCommand::buildMongoReply" )
INT32 _mongoSaslStartCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                               engine::rtnContextBuf &bodyBuf,
                                               _mongoResponseBuffer &headerBuf )
{
  /* Generate server-first-message on the form:
   * r=client-nonce+server-nonce,s=user-salt,i=iteration-count
   */
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTH1BUILDMONGOREPLY ) ;
   stringstream ss ;
   string payload ;
   BSONObjBuilder bob ;
   BSONObj replyObj( bodyBuf.data() ) ;
   BOOLEAN rebuildBuf = FALSE ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( replyObj.isEmpty() )
      {
         bob.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
         bob.append( FAP_MONGO_FIELD_NAME_CODE, 18 ) ;
         bob.append( FAP_MONGO_FIELD_NAME_ERRMSG,
                     "Authentication has been disabled, "
                     "you don't need to execute 'db.auth()' " ) ;
         rebuildBuf = TRUE ;
      }
      else
      {
         if ( SDB_OK == sdbReply.flags )
         {
            const CHAR* pSalt = replyObj.getStringField( SDB_AUTH_SALT ) ;
            UINT32 iterationCount = replyObj.getIntField( SDB_AUTH_ITERATIONCOUNT ) ;
            const CHAR* pNonce = replyObj.getStringField( SDB_AUTH_NONCE ) ;

            ss << FAP_MONGO_SASL_MSG_RANDOM  FAP_MONGO_EQUAL << pNonce
               << FAP_MONGO_COMMA
               << FAP_MONGO_SASL_MSG_SALT    FAP_MONGO_EQUAL << pSalt
               << FAP_MONGO_COMMA
               << FAP_MONGO_SASL_MSG_ITERATE FAP_MONGO_EQUAL << iterationCount ;

            payload = ss.str() ;
            bob.appendBinData( FAP_MONGO_FIELD_NAME_PAYLOAD, payload.length(),
                               BinDataGeneral, payload.c_str() ) ;
            bob.append( FAP_MONGO_FIELD_NAME_DONE, false ) ;
            bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_CONVERID, 1 ) ;
            rebuildBuf = TRUE ;
         }
         else if ( SDB_AUTH_AUTHORITY_FORBIDDEN == sdbReply.flags )
         {
            bob.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_CODE, 18 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_ERRMSG, "Authentication failed." ) ;
            rebuildBuf = TRUE ;
         }
         else if ( SDB_AUTH_INCOMPATIBLE == sdbReply.flags )
         {
            bob.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_CODE, 18 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_ERRMSG,
                        getErrDesp( SDB_AUTH_INCOMPATIBLE ) ) ;
            rebuildBuf = TRUE ;
         }
      }

      if ( rebuildBuf )
      {
         // Only when rebuildBuf is true can set bodyBuf, otherwise the original
         // content will be overwritten with empty bson.
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo auth1 "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTH1BUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoSaslContinueCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTH2INIT, "_mongoSaslContinueCommand::init" )
INT32 _mongoSaslContinueCommand::init( const _mongoMessage *pMsg,
                                       mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTH2INIT ) ;
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* pCommandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( pCommandName, name() ),
                  "Invalid command name" ) ;

      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _isInitialized = TRUE ;
   _requestID = pMsg->requestID() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTH2INIT, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTH2BUILDSDBREQ, "_mongoSaslContinueCommand::buildSdbRequest" )
INT32 _mongoSaslContinueCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                  mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTH2BUILDSDBREQ ) ;
   INT32 rc = SDB_OK ;
   MsgAuthentication *pAuth = NULL ;
   const CHAR* pChannel = NULL ;
   const CHAR* pNonce = NULL ;
   const CHAR* pClientProof = NULL ;
   CHAR* p = NULL ;
   CHAR* nextPtr = NULL ;
   const CHAR* pPayload = NULL ;
   INT32 payloadLen = 0 ;
   CHAR payloadCpy[ FAP_MONGO_PAYLOADD_MAX_SIZE ] = { 0 } ;
   BSONObj obj ;

   try
   {
      // get payload
      BSONElement ele = _obj.getField( FAP_MONGO_FIELD_NAME_PAYLOAD ) ;
      pPayload = ele.binData( payloadLen ) ;
      if ( payloadLen > FAP_MONGO_PAYLOADD_MAX_SIZE )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( 0 == payloadLen || 'v' == pPayload[0] )
      {
         // step 3 format: { payload: "" } or { payload: "v=ServerSignature" }
         _step = MONGO_AUTH_STEP3 ;
         goto done ;
      }

      ossMemcpy( payloadCpy, pPayload, payloadLen ) ;

     /* The payload forms is:
      * c=channel-binding(base64),r=client-nonce+server-nonce,p=ClientProof
      */
      p = ossStrtok( payloadCpy, FAP_MONGO_COMMA, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pChannel = p + 2 ;

      p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pNonce = p + 2 ;

      p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
      if ( NULL == p )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pClientProof = p + 2 ;

      rc = sdbMsg.reserve( sizeof( MsgAuthentication ) ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.advance( sizeof( MsgAuthentication ) ) ;
      if ( rc )
      {
         goto error ;
      }

      pAuth = ( MsgAuthentication * ) sdbMsg.data() ;
      pAuth->header.opCode = MSG_AUTH_VERIFY1_REQ ;
      pAuth->header.TID = 0 ;
      pAuth->header.routeID.value = 0 ;
      pAuth->header.requestID = _requestID ;

      obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_2 <<
                  SDB_AUTH_USER << ctx.userName <<
                  SDB_AUTH_NONCE << pNonce <<
                  SDB_AUTH_IDENTIFY << pChannel <<
                  SDB_AUTH_PROOF << pClientProof <<
                  SDB_AUTH_TYPE << SDB_AUTH_TYPE_EXTEND_PWD ) ;

      rc = sdbMsg.write( obj, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;

      ctx.authInfo.nonce = pNonce ;
      ctx.authInfo.identify = pChannel ;
      ctx.authInfo.clientProof = pClientProof ;
      ctx.authInfo.type = SDB_AUTH_TYPE_EXTEND_PWD ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb auth2 "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTH2BUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_AUTH2BUILDMONGOREPLY, "_mongoSaslContinueCommand::buildMongoReply" )
INT32 _mongoSaslContinueCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                  engine::rtnContextBuf &bodyBuf,
                                                  _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_AUTH2BUILDMONGOREPLY ) ;
   BSONObjBuilder bob ;
   BOOLEAN rebuildBuf = FALSE ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( MONGO_AUTH_STEP2 == _step )
      {
        /* Generate successful authentication server-final-message on the form:
         * v=ServerSignature
         *
         * or failed authentication server-final-message on the form:
         * e=message
         */
         if ( SDB_OK == sdbReply.flags )
         {
            BSONObj replyObj( bodyBuf.data() ) ;
            const CHAR* pServerProof = replyObj.getStringField( SDB_AUTH_PROOF ) ;
            string payload = FAP_MONGO_SASL_MSG_VALUE FAP_MONGO_EQUAL ;
            payload += pServerProof ;

            bob.appendBinData( FAP_MONGO_FIELD_NAME_PAYLOAD, payload.length(),
                               BinDataGeneral, payload.c_str() ) ;
            bob.append( FAP_MONGO_FIELD_NAME_DONE, false ) ;
            bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_CONVERID, 1 ) ;
            rebuildBuf = TRUE ;
         }
         else if ( SDB_AUTH_AUTHORITY_FORBIDDEN == sdbReply.flags )
         {
            bob.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_CODE, 18 ) ;
            bob.append( FAP_MONGO_FIELD_NAME_ERRMSG, "Authentication failed." ) ;
            rebuildBuf = TRUE ;
         }
      }
      else if ( MONGO_AUTH_STEP3 == _step )
      {
         bob.appendBinData( FAP_MONGO_FIELD_NAME_PAYLOAD, 0, BinDataGeneral, "" ) ;
         bob.append( FAP_MONGO_FIELD_NAME_DONE, true ) ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bob.append( FAP_MONGO_FIELD_NAME_CONVERID, 1 ) ;
         rebuildBuf = TRUE ;
      }

      if ( rebuildBuf )
      {
         // Only when rebuildBuf is true can set bodyBuf, otherwise the original
         // content will be overwritten with empty bson.
         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo auth2 "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_AUTH2BUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListCollectionCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTCLBUILDSDBREQ, "_mongoListCollectionCommand::buildSdbRequest" )
INT32 _mongoListCollectionCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                    mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTCLBUILDSDBREQ ) ;
   INT32 rc                = SDB_OK ;
   MsgOpQuery *pQuery      = NULL ;
   const CHAR *pCmdName    = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS ;
   BSONObj empty ;
   BSONObjBuilder builder ;
   string lowBound ;
   string upBound ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   // filter cs[foo]'s collections
   // condition: { Name: { $gt: "foo.", $lt: "foo/" }, ... }
   try
   {
      lowBound = _csName ;
      lowBound += "." ;
      upBound = _csName ;
      upBound += "/" ;
      builder.appendElements( _obj.getObjectField( "filter" ) ) ;
      builder.append( "Name", BSON( "$gt" << lowBound << "$lt" << upBound ) ) ;

      rc = sdbMsg.write( builder.obj(), TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error;
      }

      rc = sdbMsg.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = sdbMsg.write( empty, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      sdbMsg.doneLen() ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb "
              "listCollections request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTCLBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTCLBUILDMONGOREPLY, "_mongoListCollectionCommand::buildMongoReply" )
INT32 _mongoListCollectionCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                    engine::rtnContextBuf &bodyBuf,
                                                    _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTCLBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   if ( SDB_OK      == sdbReply.flags ||
        SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build first batch, rc: %d", rc ) ;
         goto error ;
      }
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTCLBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListDatabaseCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTDBBUILDSDBREQ, "_mongoListDatabaseCommand::buildSdbRequest" )
INT32 _mongoListDatabaseCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                  mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTDBBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   const CHAR *pCmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES ;
   BSONObj empty ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pQuery = ( MsgOpQuery * )sdbMsg.data() ;
   pQuery->header.opCode = MSG_BS_QUERY_REQ ;
   pQuery->header.TID = 0 ;
   pQuery->header.routeID.value = 0 ;
   pQuery->header.requestID = _requestID ;
   pQuery->version = 0 ;
   pQuery->w = 0 ;
   pQuery->padding = 0 ;
   pQuery->flags = 0 ;
   pQuery->numToSkip = 0 ;
   pQuery->numToReturn = -1 ;
   pQuery->nameLength = ossStrlen( pCmdName ) ;

   rc = sdbMsg.write( pCmdName, pQuery->nameLength + 1, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( empty, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTDBBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_LISTDBBUILDMONGOREPLY, "_mongoListDatabaseCommand::buildMongoReply" )
INT32 _mongoListDatabaseCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                  engine::rtnContextBuf &bodyBuf,
                                                  _mongoResponseBuffer &headerBuf )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_LISTDBBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;

   try
   {
      if ( SDB_OK == sdbReply.flags )
      {
         BSONObjBuilder bob ;
         BSONArrayBuilder arr( bob.subarrayStart( "databases" ) ) ;
         INT32 offset = 0 ;
         while ( offset < bodyBuf.size() )
         {
            BSONObj obj( bodyBuf.data() + offset ) ;
            // { Name: "cs" } => { name: "cs" }
            arr.append( BSON( "name" << obj.getStringField( "Name" ) ) ) ;
            offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         }
         arr.done() ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;

         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_EOC == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( "databases" << BSONArray() <<
                                          FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo "
              "listDatabase reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_LISTDBBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetLogCommand)
INT32 _mongoGetLogCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                            engine::rtnContextBuf &bodyBuf,
                                            _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   try
   {
      bson::BSONObjBuilder bob ;
      bob.append( "totalLinesWritten", 0 ) ;
      bob.append( "log", BSONArray() ) ;
      bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo getLog "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoIsmasterCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoIsMasterCommand)
INT32 _mongoIsMasterCommand::init( const _mongoMessage *pMsg,
                                   mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;

   rc = _mongoGlobalCommand::init( pMsg, ctx ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init isMaster command, rc: %d", rc ) ;
      goto error ;
   }

   try
   {
      if ( MONGO_QUERY_MSG == pMsg->type() && !ctx.hasParsedClientInfo )
      {
         const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

         BSONObj obj = BSONObj( pReq->query() ) ;
         if ( obj.hasField( "client" ) )
         {
            BSONObj clientObj = obj.getObjectField( "client" ) ;
            BSONObj driverObj = clientObj.getObjectField( "driver" ) ;
            const CHAR* pDriverName   = driverObj.getStringField( "name" ) ;
            const CHAR* pDriverVerStr = driverObj.getStringField( "version" ) ;

            rc = _parseClientInfo( pDriverName, pDriverVerStr, ctx.clientInfo ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to parse client info, rc: %d", rc ) ;

            ctx.hasParsedClientInfo = TRUE ;
         }
      }
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when initing isMaster command: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoIsMasterCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   try
   {
      BSONObjBuilder bob ;
      bob.appendBool( "ismaster", TRUE ) ;
      bob.append( "maxBsonObjectSize", 16*1024*1024 ) ;
      bob.append( "maxMessageSizeBytes", SDB_MAX_MSG_LENGTH ) ;
      bob.append( "maxWriteBatchSize", 1000 ) ;
      bob.append( "maxWireVersion", 4 ) ; // correspions to mongodb3.2
      bob.append( "minWireVersion", 0 ) ;
      bob.append( "msg", "isdbgrid" ) ;
      bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo isMaster "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoIsMasterCommand::_parseClientInfo( const CHAR* pClientName,
                                               const CHAR* pClientVerStr,
                                               mongoClientInfo &clientInfo )
{
   INT32 rc          = SDB_OK ;
   INT32 i           = 0 ;
   CHAR *pCurStr     = NULL ;
   CHAR *pLastParsed = NULL ;
   CHAR clientVerStrCpy[FAP_MONGO_CLIENT_VERSION_STR_MAX_SIZE+1] = { 0 } ;

   if ( NULL == pClientName || NULL == pClientVerStr )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ossStrlen( pClientVerStr ) < 0 ||
        ossStrlen( pClientVerStr ) > FAP_MONGO_CLIENT_VERSION_STR_MAX_SIZE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossStrncpy( clientVerStrCpy, pClientVerStr, ossStrlen( pClientVerStr ) ) ;

   if ( 0 == ossStrcmp( pClientName, FAP_MONGO_FIELD_VALUE_NODEJS ) )
   {
      clientInfo.type = NODEJS_DRIVER ;
   }
   else if ( 0 == ossStrcmp( pClientName, FAP_MONGO_FIELD_VALUE_JAVA ) )
   {
      clientInfo.type = JAVA_DRIVER ;
   }
   else if ( 0 == ossStrcmp( pClientName,
                             FAP_MONGO_FIELD_VALUE_MONGOSHELL ) )
   {
      clientInfo.type = MONGO_SHELL ;
   }

   pCurStr = ossStrtok( clientVerStrCpy, ".", &pLastParsed ) ;
   while ( NULL != pCurStr && '\0' != pCurStr[0] )
   {
      if( 0 == i )
      {
         clientInfo.version = ossAtoi( pCurStr ) ;
      }
      else if( 1 == i )
      {
         clientInfo.subVersion = ossAtoi( pCurStr ) ;
      }
      else if( 2 == i )
      {
         clientInfo.fixVersion = ossAtoi( pCurStr ) ;
      }
      pCurStr = ossStrtok( pLastParsed, ".", &pLastParsed ) ;
      i++ ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoBuildinfoCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoBuildInfoCommand)
INT32 _mongoBuildInfoCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                               engine::rtnContextBuf &bodyBuf,
                                               _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   try
   {
      BSONObjBuilder bob ;
      bob.append( "version", "3.2.22" ) ;
      // versionArray is important, it affects the protocol of messages
      // sent by mongo client
      bson::BSONArrayBuilder sub( bob.subarrayStart( "versionArray" ) ) ;
      sub.append( 3 ) ;
      sub.append( 2 ) ;
      sub.append( 22 ) ;
      sub.append( 0 ) ;
      sub.done() ;
      bob.append( "maxBsonObjectSize", 16*1024*1024 ) ;
      bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo buildInfo "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetLastErrorCommand)
INT32 _mongoGetLastErrorCommand::init( const _mongoMessage *pMsg,
                                       mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;

   _errorInfoObj = ctx.lastErrorObj.getOwned() ;

   rc = _mongoGlobalCommand::init( pMsg, ctx ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init getLastError command, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetLastErrorCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                  engine::rtnContextBuf &bodyBuf,
                                                  _mongoResponseBuffer &headerBuf )
{
   INT32       rc = SDB_OK ;
   INT32       errCode = SDB_OK ;
   BSONElement codeEle ;
   bson::BSONObjBuilder bob ;

   try
   {
      codeEle = _errorInfoObj.getField( FAP_MONGO_FIELD_NAME_CODE ) ;
      if ( !codeEle.eoo() )
      {
         errCode = codeEle.numberInt() ;
      }

      if ( SDB_OK == errCode )
      {
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bob.appendNull( FAP_MONGO_FIELD_NAME_ERR ) ;
      }
      else
      {
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bob.append( FAP_MONGO_FIELD_NAME_CODE, errCode ) ;
         bob.append( FAP_MONGO_FIELD_NAME_ERR,
                 _errorInfoObj.getStringField( FAP_MONGO_FIELD_NAME_ERRMSG ) ) ;
      }

      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo getLastError "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoLogoutCommand)
INT32 _mongoLogoutCommand::init( const _mongoMessage *pMsg,
                                 mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;

   // errorObj indicates the error information of the current command. However,
   // executing the logout command is always successful. So we need to reset
   // errorObj to empty.
   ctx.errorObj = BSONObj() ;

   rc = _mongoGlobalCommand::init( pMsg, ctx ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init logout command, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoLogoutCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                            engine::rtnContextBuf &bodyBuf,
                                            _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   try
   {
      bodyBuf = engine::rtnContextBuf( BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo logout "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoDummyCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                           engine::rtnContextBuf &bodyBuf,
                                           _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   try
   {
      bodyBuf = engine::rtnContextBuf( BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
   }
   catch ( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo logout "
              "reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoWhatsMyUriCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetReplStatCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetCmdLineOptsCommand)

// If the command is findAndModify, findOneAndUpdate, findOneAndDelete or
// findOneAndReplace, we will enter this class
// The functions of these four commands are the same, the only difference is
// that the parameters of the commands are different
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoFindandmodifyCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoFindAndModifyCommand)
//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_FINDANDMODIFYBUILDSDBREQ, "_mongoFindAndModifyCommand::buildSdbRequest" )
INT32 _mongoFindAndModifyCommand::buildSdbRequest( mongoMsgBuffer &sdbMsg,
                                                   mongoSessionCtx &ctx )
{
   PD_TRACE_ENTRY( SDB_FAPMONGO_FINDANDMODIFYBUILDSDBREQ ) ;
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *pFindAndModify = NULL ;
   BSONObj sort, selector, hint ;
   BOOLEAN isUpdate = FALSE ;
   BOOLEAN isRemove = FALSE ;
   BSONObjBuilder operatorBob, hintBob ;

   rc = sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;
   if ( rc )
   {
      goto error ;
   }

   pFindAndModify = ( MsgOpQuery * )sdbMsg.data() ;
   pFindAndModify->header.opCode = MSG_BS_QUERY_REQ ;
   pFindAndModify->header.TID = 0 ;
   pFindAndModify->header.routeID.value = 0 ;
   pFindAndModify->header.requestID = _requestID ;
   pFindAndModify->version = 0 ;
   pFindAndModify->w = 0 ;
   pFindAndModify->padding = 0 ;
   pFindAndModify->flags = FLG_QUERY_WITH_RETURNDATA | FLG_QUERY_MODIFY ;
   pFindAndModify->numToSkip = 0 ;
   // findAndModify, findOneAndUpdate, findOneAndDelete and findOneAndReplace
   // will only modify and return a single document
   pFindAndModify->numToReturn = 1 ;
   pFindAndModify->nameLength = _clFullName.length() ;

   rc = sdbMsg.write( _clFullName.c_str(), pFindAndModify->nameLength + 1,
                      TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   try
   {
      /*
      eg: _obj =
      {
         "findandmodify": "clName",
         "query": <object>,
         "sort": <object>,
         "fields": <object>,
         "update": <object>,
         "remove": <bool>,
         "new": <bool>
      }
      */
      rc = convertMongoOperator2Sdb( _obj, operatorBob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert mongo operator to sdb operator, rc: %d",
                   rc ) ;
      _obj = operatorBob.obj() ;

      /*
      The format of hint we will build is as follows:
      {
         "$Modify":
         {
            "OP": "update",
            "Update": <object>,
            "ReturnNew": <bool>
         }
      }
      or
      {
         "$Modify":
         {
            "OP": "remove",
            "Remove": <bool>
         }
      }
      */

      BSONObjBuilder subHintBob( hintBob.subobjStart( FIELD_NAME_MODIFY ) ) ;
      BSONObjIterator itr( _obj ) ;
      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;

         if ( 0 == ossStrcmp( ele.fieldName(), FAP_MONGO_FIELD_NAME_QUERY  ) )
         {
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of query field must be object" ) ;
               goto error ;
            }
            _cond = ele.embeddedObject() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FAP_MONGO_FIELD_NAME_SORT  ) )
         {
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of sort field must be object" ) ;
               goto error ;
            }
            sort = ele.embeddedObject() ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_OP_VALUE_REMOVE  ) )
         {
            if ( Bool != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of remove field must be bool" ) ;
               goto error ;
            }
            isRemove = ele.Bool() ;
            if ( isRemove )
            {   
               subHintBob.append( FIELD_NAME_OP, FIELD_OP_VALUE_REMOVE ) ;
               subHintBob.appendBool( FIELD_NAME_OP_REMOVE, isRemove ) ;
            }
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_OP_VALUE_UPDATE  ) )
         {
            BOOLEAN hasOp = FALSE ;

            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of update field must be object" ) ;
               goto error ;
            }

            _updater = ele.embeddedObject() ;

            rc = _isCondHasOp( _updater, hasOp ) ;
            if ( rc )
            {
               goto error ;
            }
            if ( !hasOp )
            {
               // if the command is findOneAndReplace, the update field won't
               // has operator name
               // eg:
               // { a: 1, b: 1 } ==> { "$replace": { a: 1, b: 1 } }
               BSONObjBuilder newUpdateBob ;
               newUpdateBob.append( FAP_MONGO_OPERATOR_REPLACE, _updater ) ;
               _updater = newUpdateBob.obj() ;
               subHintBob.append( FIELD_NAME_OP_UPDATE, _updater ) ;
            }
            else
            {
               subHintBob.append( FIELD_NAME_OP_UPDATE, _updater ) ;
            }

            subHintBob.append( FIELD_NAME_OP, FIELD_OP_VALUE_UPDATE ) ;
            isUpdate = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FAP_MONGO_FIELD_NAME_NEW  ) )
         {
            if ( Bool != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of new field must be bool" ) ;
               goto error ;
            }
            _isReturnNew = ele.Bool() ;
            subHintBob.appendBool( FIELD_NAME_RETURNNEW, _isReturnNew ) ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FAP_MONGO_FIELD_NAME_FIELDS  ) )
         {
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of fields field must be object" ) ;
               goto error ;
            }
            selector = ele.embeddedObject() ;

            rc = convertProjection( selector ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else if ( 0 == ossStrcmp( ele.fieldName(),
                                   FAP_MONGO_FIELD_NAME_UPSERT  ) )
         {
            if ( Bool != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               ctx.setError( rc, "The type of upsert field must be bool" ) ;
               goto error ;
            }
            _isUpsert = ele.Bool() ;
         }

         /*
         SequoiaDB don't support these parameters:
         collation: <object>
         bypassDocumentValidation: <bool>
         arrayFilters: [ <filter1>... ]
         writeConcern: <object>
         */
      }

      subHintBob.doneFast() ;
      hint = hintBob.done() ;

      if ( isUpdate && isRemove )
      {
         rc = SDB_INVALIDARG ;
         ctx.setError( rc, "Cannot specify update and remove = true "
                       "at the same time" ) ;
         goto error ;
      }

      if ( !isUpdate && !isRemove )
      {
         rc = SDB_INVALIDARG ;
         ctx.setError( rc, "Either an update or remove = true must be "
                       "specified" ) ;
         goto error ;
      }

      if ( isRemove && _isReturnNew )
      {
         rc = SDB_INVALIDARG ;
         ctx.setError( rc, "Cannot specify new = true and remove = true "
                       "at the same time. 'remove' always returns the deleted "
                       "document" ) ;
         goto error ;
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building sdb findAndModify "
              "request: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

   rc = sdbMsg.write( _cond, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( selector, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( sort, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = sdbMsg.write( hint, TRUE ) ;
   if ( rc )
   {
      goto error ;
   }

   sdbMsg.doneLen() ;

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_FINDANDMODIFYBUILDSDBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_FAPMONGO_FINDANDMODIFYBUILDMONGOREPLY, "_mongoFindAndModifyCommand::buildMongoReply" )
INT32 _mongoFindAndModifyCommand::buildMongoReply( const MsgOpReply &sdbReply,
                                                   engine::rtnContextBuf &bodyBuf,
                                                   _mongoResponseBuffer &headerBuf )
{
   /*
   The format of reply msg is as follows:

   {
      lastErrorObject:
      {
         n: <INT32>,
         updatedExisting: <bool>
      },
      value: <object>,
      ok: <INT32>
   }
   */
   PD_TRACE_ENTRY( SDB_FAPMONGO_FINDANDMODIFYBUILDMONGOREPLY ) ;
   INT32 rc = SDB_OK ;
   BSONObjBuilder resultBuilder ;
   BOOLEAN hasRecordReturned = ( 0 < bodyBuf.size() ) ;
   BOOLEAN hasDecimal = FALSE ;

   try
   {
      if ( SDB_OK == sdbReply.flags || SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
           SDB_DMS_NOTEXIST == sdbReply.flags )
      {
         if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags ||
              SDB_DMS_NOTEXIST == sdbReply.flags )
         {
            bodyBuf = engine::rtnContextBuf() ;
            hasRecordReturned = FALSE ;
         }

         BSONObjBuilder decimalConvertBob ;
         BSONObjBuilder lastErrorObjectBuilder( resultBuilder.subobjStart(
                                     FAP_MONGO_FIELD_NAME_LASTERROROBJECT ) ) ;
         lastErrorObjectBuilder.append( FAP_MONGO_FIELD_NAME_N,
                                        hasRecordReturned ) ;
         lastErrorObjectBuilder.appendBool( FAP_MONGO_FIELD_NAME_UPDATEEXISTING,
                                            hasRecordReturned ) ;
         lastErrorObjectBuilder.doneFast() ;

         if ( hasRecordReturned )
         {
            if ( _hasInsertRecord && !_isReturnNew )
            {
               resultBuilder.appendNull( FAP_MONGO_FIELD_NAME_VALUE ) ;
            }
            else
            {
               BSONObj obj( bodyBuf.data() ) ;
               rc = sdbDecimal2MongoDecimal( obj, decimalConvertBob, hasDecimal ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to convert sdb record to mongo record, "
                            "rc: %d", rc ) ;
               if ( hasDecimal )
               {
                  resultBuilder.append( FAP_MONGO_FIELD_NAME_VALUE,
                                        decimalConvertBob.done() ) ;
               }
               else
               {
                  resultBuilder.append( FAP_MONGO_FIELD_NAME_VALUE, obj ) ;
               }
            }
         }
         else
         {
            resultBuilder.appendNull( FAP_MONGO_FIELD_NAME_VALUE ) ;
         }

         resultBuilder.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;
         bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;
      }

      rc = _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build common reply, rc: %d", rc ) ;
         goto error ;
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when building mongo findAndModify"
              " reply: %s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_FAPMONGO_FINDANDMODIFYBUILDMONGOREPLY, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 _mongoFindAndModifyCommand::_isCondHasOp( const BSONObj &cond,
                                                BOOLEAN &hasOp )
{
   INT32 rc = SDB_OK ;
   hasOp = FALSE ;

   try
   {
      BSONObjIterator itr( cond ) ;
      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;

         if ( '$' == ele.fieldName()[0] )
         {
            hasOp = TRUE ;
         }
      }
   }
   catch( std::exception &e )
   {
      rc = ossException2RC( &e ) ;
      PD_LOG( PDERROR, "An exception occurred when checking if cond has op: "
              "%s, rc: %d", e.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

}
