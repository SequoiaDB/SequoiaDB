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

using namespace bson ;

namespace fap
{

#define FAP_MONGO_PAYLOADD_MAX_SIZE 128
#define FAP_MONGO_CLIENT_VERSION_STR_MAX_SIZE 128

static INT32 getIntElement( const BSONObj &obj, const CHAR *fieldName,
                            INT32 &value )
{
   SINT32 rc = SDB_OK ;
   SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
   BSONElement ele = obj.getField ( fieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              fieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be Integer",
              obj.toString().c_str()) ;
   value = ele.numberInt() ;
done :
   return rc ;
error :
   goto done ;
}

static INT32 getStringElement ( const BSONObj &obj, const CHAR *fieldName,
                                const CHAR **value )
{
   SINT32 rc = SDB_OK ;
   SDB_ASSERT ( fieldName && value, "field name and value can't be NULL" ) ;
   BSONElement ele = obj.getField ( fieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              fieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be String",
              obj.toString().c_str()) ;
   *value = ele.valuestr() ;
done :
   return rc ;
error :
   goto done ;
}

static INT32 getArrayElement ( const BSONObj &obj, const CHAR *fieldName,
                               BSONObj &value )
{
   SINT32 rc = SDB_OK ;
   SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;
   BSONElement ele = obj.getField ( fieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              fieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be Array",
              obj.toString().c_str()) ;
   value = ele.embeddedObject() ;
done :
   return rc ;
error :
   goto done ;
}

static INT32 getNumberLongElement ( const BSONObj &obj, const CHAR *fieldName,
                                    INT64 &value )
{
   SINT32 rc = SDB_OK ;
   SDB_ASSERT ( fieldName, "field name can't be NULL" ) ;
   BSONElement ele = obj.getField ( fieldName ) ;
   PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDWARNING,
              "Can't locate field '%s': %s",
              fieldName,
              obj.toString().c_str() ) ;
   PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDWARNING,
              "Unexpected field type : %s, supposed to be number",
              obj.toString().c_str()) ;
   value = ele.numberLong() ;
done :
   return rc ;
error :
   goto done ;
}

static void appendEmptyObj2Buf( engine::rtnContextBuf &bodyBuf )
{
   BSONObj empty ;
   BSONObj org( bodyBuf.data() ) ;
   msgBuffer tmpBuffer ;
   tmpBuffer.write( org.objdata(), org.objsize() ) ;
   tmpBuffer.write( empty.objdata(), empty.objsize() ) ;
   bodyBuf = engine::rtnContextBuf( tmpBuffer.data(), tmpBuffer.size(), 2 ) ;
}

static void escapeDot( string& collectionFullName )
{
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
         collectionFullName.replace( pos, 1, "%2E" ) ;
         pos += 3 ;
      }
   }
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

static void convertProjection( BSONObj &proj )
{
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
                            BSON( "$include" << ( e.trueValue() ? 1 : 0 ) ) ) ;
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

done:
   return ;
}

static void convertIndexObj( BSONObj& indexObj, string clFullName )
{
   BSONObjBuilder builder ;
   BSONObj sdbIdxDef = indexObj.getObjectField( "IndexDef" ) ;

   // listIndexes command may send getMore message, we should convert fullname:
   //     foo.$cmd.listIndexes.bar => foo.bar
   string::size_type pos = clFullName.find( "$cmd.listIndexes" ) ;
   if ( string::npos != pos )
   {
      clFullName.erase( pos, sizeof( "$cmd.listIndexes" ) ) ;
   }
   unescapeDot( clFullName ) ;

   // build
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

static void convertCollectionObj( BSONObj& collectionObj )
{
   // { Name: "foo.bar" } => { name: "bar" }
   const CHAR* _clFullName = collectionObj.getStringField( "Name" ) ;
   const CHAR* dotPos = ossStrstr( _clFullName, "." ) + 1 ;
   string clShortName = dotPos ;

   unescapeDot( clShortName ) ;

   collectionObj = BSON( "name" << clShortName.c_str() ) ;
}

static INT32 parseClientInfo( const CHAR* clientName,
                              const CHAR* clientVerStr,
                              mongoClientInfo &clientInfo )
{
   INT32 rc         = SDB_OK ;
   INT32 i          = 0 ;
   CHAR *curStr     = NULL ;
   CHAR *lastParsed = NULL ;
   CHAR clientVerStrCpy[FAP_MONGO_CLIENT_VERSION_STR_MAX_SIZE+1] = { 0 } ;

   if ( NULL == clientName || NULL == clientVerStr )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ossStrlen( clientVerStr ) < 0 ||
        ossStrlen( clientVerStr ) > FAP_MONGO_CLIENT_VERSION_STR_MAX_SIZE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   ossStrncpy( clientVerStrCpy, clientVerStr, ossStrlen( clientVerStr ) ) ;

   if ( 0 == ossStrcmp( clientName, FAP_MONGO_FIELD_VALUE_NODEJS ) )
   {
      clientInfo.type = NODEJS_DRIVER ;
   }
   else if ( 0 == ossStrcmp( clientName, FAP_MONGO_FIELD_VALUE_JAVA ) )
   {
      clientInfo.type = JAVA_DRIVER ;
   }
   else if ( 0 == ossStrcmp( clientName,
                             FAP_MONGO_FIELD_VALUE_MONGOSHELL ) )
   {
      clientInfo.type = MONGO_SHELL ;
   }

   curStr = ossStrtok( clientVerStrCpy, ".", &lastParsed ) ;
   while ( '\0' != curStr )
   {
      if( 0 == i )
      {
         clientInfo.version = ossAtoi( curStr ) ;
      }
      else if( 1 == i )
      {
         clientInfo.subVersion = ossAtoi( curStr ) ;
      }
      else if( 2 == i )
      {
         clientInfo.fixVersion = ossAtoi( curStr ) ;
      }
      curStr = ossStrtok( lastParsed, ".", &lastParsed ) ;
      i++ ;
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
   if ( pCmdInfo->next )
   {
      _releaseCmdInfo ( pCmdInfo->next ) ;
      pCmdInfo->next = NULL ;
   }
   if ( pCmdInfo->sub )
   {
      _releaseCmdInfo ( pCmdInfo->sub ) ;
      pCmdInfo->sub = NULL ;
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
      _pCmdInfoRoot->next = NULL ;
      _pCmdInfoRoot->sub = NULL ;
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
   _mongoCmdInfo *newCmdInfo = NULL ;
   UINT32 sameNum = _near ( pCmdInfo->cmdName.c_str(), name ) ;

   if ( sameNum == 0 )
   {
      if ( !pCmdInfo->sub )
      {
         newCmdInfo = SDB_OSS_NEW _mongoCmdInfo ;
         newCmdInfo->cmdName = name ;
         newCmdInfo->createFunc = pFunc ;
         newCmdInfo->nameSize = ossStrlen(name) ;
         newCmdInfo->next = NULL ;
         newCmdInfo->sub = NULL ;

         pCmdInfo->sub = newCmdInfo ;
      }
      else
      {
         rc = _insert ( pCmdInfo->sub, name, pFunc ) ;
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
      else if ( !pCmdInfo->next )
      {
         newCmdInfo = SDB_OSS_NEW _mongoCmdInfo ;
         newCmdInfo->cmdName = &name[sameNum] ;
         newCmdInfo->createFunc = pFunc ;
         newCmdInfo->nameSize = ossStrlen(&name[sameNum]) ;
         newCmdInfo->next = NULL ;
         newCmdInfo->sub = NULL ;

         pCmdInfo->next = newCmdInfo ;
      }
      else
      {
         rc = _insert ( pCmdInfo->next, &name[sameNum], pFunc ) ;
      }
   }
   else
   {
      //split next node first
      newCmdInfo = SDB_OSS_NEW _mongoCmdInfo ;
      newCmdInfo->cmdName = pCmdInfo->cmdName.substr( sameNum ) ;
      newCmdInfo->createFunc = pCmdInfo->createFunc ;
      newCmdInfo->nameSize = pCmdInfo->nameSize - sameNum ;
      newCmdInfo->next = pCmdInfo->next ;
      newCmdInfo->sub = NULL ;

      pCmdInfo->next = newCmdInfo ;

      //change cur node
      pCmdInfo->cmdName = pCmdInfo->cmdName.substr ( 0, sameNum ) ;
      pCmdInfo->nameSize = sameNum ;

      if ( sameNum != ossStrlen ( name ) )
      {
         pCmdInfo->createFunc = NULL ;

         rc = _insert ( newCmdInfo, &name[sameNum], pFunc ) ;
      }
      else
      {
         pCmdInfo->createFunc = pFunc ;
      }
   }

   return rc ;
}

_mongoCommand *_mongoCmdFactory::create ( const CHAR *commandName )
{
   MONGO_CMD_NEW_FUNC pFunc = _find ( commandName ) ;
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

MONGO_CMD_NEW_FUNC _mongoCmdFactory::_find ( const CHAR * name )
{
   MONGO_CMD_NEW_FUNC pFunc = NULL ;
   _mongoCmdInfo *pCmdNode = _pCmdInfoRoot ;
   UINT32 index = 0 ;
   BOOLEAN findRoot = FALSE ;

   while ( pCmdNode )
   {
      findRoot = FALSE ;
      if ( pCmdNode->cmdName.at ( 0 ) != name[index] )
      {
         pCmdNode = pCmdNode->sub ;
      }
      else
      {
         findRoot = TRUE ;
      }

      if ( findRoot )
      {
         if ( _near ( pCmdNode->cmdName.c_str(), &name[index] )
            != pCmdNode->nameSize )
         {
            break ;
         }
         index += pCmdNode->nameSize ;
         if ( name[index] == 0 ) //find
         {
            pFunc = pCmdNode->createFunc ;
            break ;
         }

         pCmdNode = pCmdNode->next ;
      }
   }

   return pFunc ;
}

UINT32 _mongoCmdFactory::_near ( const CHAR *str1, const CHAR *str2 )
{
   UINT32 same = 0 ;
   while ( str1[same] && str2[same] && str1[same] == str2[same] )
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

static INT32 _mongoGetAndInitCommand( const CHAR *commandName,
                                      _mongoMessage *pMsg,
                                      _mongoCommand **ppCommand,
                                      mongoSessionCtx &sessCtx,
                                      msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;

   if ( NULL == ppCommand )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      // get command
      *ppCommand = getMongoCmdFactory()->create( commandName ) ;
      if ( NULL == *ppCommand )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;

         CHAR err[ 64 ] = {0} ;
         ossSnprintf( err, 63, "Command '%s' is not supported", commandName ) ;
         sessCtx.setError( rc, err ) ;

         PD_LOG( PDERROR,
                 "Unknown command name for mongo request: %s",
                 commandName ) ;
         goto error ;
      }

      // init command
      rc = (*ppCommand)->init( pMsg, sessCtx ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init command[%s], rc: %d",
                   (*ppCommand)->name(), rc ) ;

      // build message
      rc = (*ppCommand)->buildSdbMsg( sdbMsg, sessCtx ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build sdb message for command[%s], rc: %d",
                   (*ppCommand)->name(), rc ) ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR,
               "Init command[%s] exception: %s",
               *ppCommand ? (*ppCommand)->name() : "", e.what() ) ;
      rc = SDB_INVALIDARG ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 mongoGetAndInitCommand( const CHAR *pMsg,
                              _mongoCommand **ppCommand,
                              mongoSessionCtx &sessCtx,
                              msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   _mongoMessage mongoMsg ;

   if ( NULL == ppCommand )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   rc = mongoMsg.init( pMsg ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to init message, rc: %d",
                rc ) ;

   if ( MONGO_OP_QUERY == mongoMsg.opCode() )
   {
      _mongoQueryRequest req ;
      const CHAR* ptr = NULL ;
      const CHAR* commandName = NULL ;

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
         commandName = obj.firstElementFieldName() ;
      }
      else
      {
         commandName = MONGO_CMD_NAME_QUERY ;
      }

      rc = _mongoGetAndInitCommand( commandName, &req,
                                    ppCommand, sessCtx, sdbMsg ) ;
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
                                    ppCommand, sessCtx, sdbMsg ) ;
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
                                    ppCommand, sessCtx, sdbMsg ) ;
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
                                    ppCommand, sessCtx, sdbMsg ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   mongoMsg.opCode() ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 mongoPostRunCommand( _mongoCommand *pCommand,
                           const MsgOpReply &sdbReply,
                           engine::rtnContextBuf &replyBuf,
                           _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   if ( NULL == pCommand )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      rc = pCommand->buildReply( sdbReply, replyBuf, headerBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build reply for command[%s], rc: %d",
                   pCommand->name(), rc ) ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR,
               "Post run command[%s] exception: %s",
               pCommand->name(), e.what() ) ;
      rc = SDB_INVALIDARG ;
   }

done:
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

INT32 _mongoGlobalCommand::init( const _mongoMessage *pMsg,
                                 mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      BSONObj obj = BSONObj( pReq->query() ) ;
      const CHAR* commandName = obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoGlobalCommand::_buildReplyCommon( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   if ( _initMsgType == MONGO_COMMAND_MSG )
   {
      appendEmptyObj2Buf( bodyBuf ) ;

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

   return SDB_OK ;
}

INT32 _mongoDatabaseCommand::init( const _mongoMessage *pMsg,
                                   mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;

      // check command name
      const CHAR* commandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      // get cs name
      const CHAR* nameInReq = pReq->fullCollectionName() ;
      const CHAR* ptr = ossStrstr( nameInReq,
                                   FAP_MONGO_FIELD_NAME_CMD ) ;
      PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
                "Invalid collectionFullName for mongo %s request: %s",
                name(), nameInReq ) ;
      _csName.assign( nameInReq, ptr - nameInReq ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _csName = pReq->databaseName() ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoDatabaseCommand::_buildReplyCommon( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   if ( _initMsgType == MONGO_COMMAND_MSG )
   {
      appendEmptyObj2Buf( bodyBuf ) ;

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

   return SDB_OK ;
}

void _mongoDatabaseCommand::_buildFirstBatch( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { firstBatch: [ {xxx}, {xxx}... ], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   BSONObjBuilder resultBuilder ;
   BSONObjBuilder cursorBuilder ;

   if ( SDB_OK != sdbReply.flags && SDB_DMS_EOC != sdbReply.flags )
   {
      return ;
   }

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
            convertCollectionObj( obj ) ;
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
   resultBuilder.append( "ok", 1 ) ;
   bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;
}

INT32 _mongoCollectionCommand::init( const _mongoMessage *pMsg,
                                     mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;
      rc = _init( pReq ) ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;
      rc = _init( pReq ) ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

   if ( needConvertDecimal() &&
        mongoIsSupportDecimal( ctx.clientInfo ) )
   {
      BSONObjBuilder sdbMsgObjBob( _obj.objsize() ) ;
      rc = mongoDecimal2SdbDecimal( _obj, sdbMsgObjBob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert mongo msg to sdb msg: %d", rc ) ;
      _obj = sdbMsgObjBob.obj() ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoCollectionCommand::_init( const _mongoQueryRequest *pReq )
{
   INT32 rc = SDB_OK ;
   const CHAR* clShortName = NULL ;
   const CHAR* nameInReq = pReq->fullCollectionName() ;

   _obj = BSONObj( pReq->query() ) ;

   // check command name
   const CHAR* commandName = _obj.firstElementFieldName() ;
   SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
               "Invalid command name" ) ;

   // get cs name
   const CHAR* ptr = ossStrstr( nameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), nameInReq ) ;
   _csName.assign( nameInReq, ptr - nameInReq ) ;

   // get collection full name
   rc = getStringElement( _obj, commandName, &clShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                commandName, rc ) ;

   _clFullName = _csName ;
   _clFullName += "." ;
   _clFullName += clShortName ;
   escapeDot( _clFullName ) ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoCollectionCommand::_init( const _mongoCommandRequest *pReq )
{
   INT32 rc = SDB_OK ;

   _obj = BSONObj( pReq->metadata() ) ;

   // check command name
   SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
               "Invalid command name" ) ;

   // get cs cl name
   _csName = pReq->databaseName() ;

   const CHAR* clShortName = NULL ;
   rc = getStringElement( _obj, pReq->commandName(), &clShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                pReq->commandName(), rc ) ;

   _clFullName = _csName ;
   _clFullName += "." ;
   _clFullName += clShortName ;
   escapeDot( _clFullName ) ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoCollectionCommand::_buildReplyCommon( const MsgOpReply &sdbReply,
                                                  engine::rtnContextBuf &bodyBuf,
                                                  _mongoResponseBuffer &headerBuf,
                                                  BOOLEAN setCursorID )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   if ( _initMsgType == MONGO_COMMAND_MSG )
   {
      appendEmptyObj2Buf( bodyBuf ) ;

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

   return SDB_OK ;
}

INT32 _mongoCollectionCommand::_buildFirstBatch( const MsgOpReply &sdbReply,
                                                 engine::rtnContextBuf &bodyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { firstBatch: [ {xxx}, {xxx}... ], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   INT32 rc = SDB_OK ;
   BSONObjBuilder resultBuilder ;
   BSONObjBuilder cursorBuilder ;

   if ( SDB_OK != sdbReply.flags && SDB_DMS_EOC != sdbReply.flags )
   {
      return SDB_OK ;
   }

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
            convertIndexObj( obj, clFullName() ) ;
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
   resultBuilder.append( "ok", 1 ) ;
   bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoInsertCommand)
INT32 _mongoInsertCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                        mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpInsert *insert     = NULL ;

   sdbMsg.reserve( sizeof( MsgOpInsert ) ) ;
   sdbMsg.advance( sizeof( MsgOpInsert ) - 4 ) ;

   insert = ( MsgOpInsert *)sdbMsg.data() ;
   insert->header.opCode = MSG_BS_INSERT_REQ ;
   insert->header.TID = 0 ;
   insert->header.routeID.value = 0 ;
   insert->header.requestID = _requestID ;
   insert->version = 0 ;
   insert->w = 0 ;
   insert->padding = 0 ;
   insert->flags = FLG_INSERT_RETURNNUM ;

   insert->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), insert->nameLength + 1, TRUE ) ;

   // get records to be insert
   BSONObj docList ;
   rc = getArrayElement( _obj, FAP_MONGO_FIELD_NAME_DOCUMENTS, docList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_DOCUMENTS, rc ) ;
   {
      BSONObjIterator it( docList ) ;
      while ( it.more() )
      {
         BSONElement ele = it.next() ;
         PD_CHECK( ele.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Invalid object[%s] in mongo %s request",
                   docList.toString().c_str(), name() ) ;
         sdbMsg.write( ele.Obj(), TRUE ) ;
      }
   }

   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoInsertCommand::buildReply( const MsgOpReply &sdbReply,
                                       engine::rtnContextBuf &bodyBuf,
                                       _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      // reply: { n: 1, ok: 1 }
      BSONObj resObj( bodyBuf.data() ) ;
      BSONObjBuilder bob ;

      bob.append( "ok", 1 ) ;
      if ( resObj.hasField( "InsertedNum" ) )
      {
         bob.append( "n", resObj.getIntField( "InsertedNum" ) ) ;
      }

      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}


MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDeleteCommand)
INT32 _mongoDeleteCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                        mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc         = SDB_OK ;
   MsgOpDelete *del = NULL ;

   sdbMsg.reserve( sizeof( MsgOpDelete ) ) ;
   sdbMsg.advance( sizeof( MsgOpDelete ) - 4 ) ;

   del = ( MsgOpDelete *)sdbMsg.data() ;
   del->header.opCode = MSG_BS_DELETE_REQ ;
   del->header.TID = 0 ;
   del->header.routeID.value = 0 ;
   del->header.requestID = _requestID ;
   del->version = 0 ;
   del->w = 0 ;
   del->padding = 0 ;
   del->flags = FLG_DELETE_RETURNNUM ;

   del->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), del->nameLength + 1, TRUE ) ;

   // { delete: "bar", deletes: [ { q: {xxx}, limit: 0 } ] }
   BSONObj objList, deleteObj ;
   rc = getArrayElement( _obj, FAP_MONGO_FIELD_NAME_DELETES, objList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_DELETES, rc ) ;
   PD_CHECK( 1 == objList.nFields() && Object == objList.firstElement().type(),
             SDB_INVALIDARG, error, PDERROR,
             "Invalid object[%s] in mongo %s request",
             _obj.toString().c_str(), name() ) ;
   deleteObj = objList.firstElement().Obj() ;

   if ( 1 == deleteObj.getIntField( "limit" ) )
   {
      del->flags |= FLG_DELETE_ONE ;
   }

   sdbMsg.write( deleteObj.getObjectField( "q" ), TRUE ) ;
   sdbMsg.write( BSONObj(), TRUE ) ; // hint

   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoDeleteCommand::buildReply( const MsgOpReply &sdbReply,
                                       engine::rtnContextBuf &bodyBuf,
                                       _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      // reply: { n: 1, ok: 1 }
      BSONObj resObj( bodyBuf.data() ) ;
      BSONObjBuilder bob ;

      bob.append( "ok", 1 ) ;
      if ( resObj.hasField( "DeletedNum" ) )
      {
         bob.append( "n", resObj.getIntField( "DeletedNum" ) ) ;
      }
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoUpdateCommand)
INT32 _mongoUpdateCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                        mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   BOOLEAN updateMulti = FALSE ;
   MsgOpUpdate *update = NULL ;
   BSONObj query, updator, hint, setOnObj ;

   sdbMsg.reserve( sizeof( MsgOpUpdate ) ) ;
   sdbMsg.advance( sizeof( MsgOpUpdate ) - 4 ) ;

   update = ( MsgOpUpdate *)sdbMsg.data() ;
   update->header.opCode = MSG_BS_UPDATE_REQ ;
   update->header.TID = 0 ;
   update->header.routeID.value = 0 ;
   update->header.requestID = _requestID ;
   update->version = 0 ;
   update->w = 0 ;
   update->padding = 0 ;
   update->flags = FLG_UPDATE_RETURNNUM ;

   update->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), update->nameLength + 1, TRUE ) ;

   // { update: "bar",
   //   updates: [ { q: {xxx}, u: {xxx}, upsert: false, multi: true } ] }
   BSONObj objList, updateObj ;
   rc = getArrayElement( _obj, FAP_MONGO_FIELD_NAME_UPDATES, objList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_UPDATES, rc ) ;
   PD_CHECK( 1 == objList.nFields() && Object == objList.firstElement().type(),
             SDB_INVALIDARG, error, PDERROR,
             "Invalid object[%s] in mongo %s request",
             _obj.toString().c_str(), name() ) ;
   updateObj = objList.firstElement().Obj() ;

   query = updateObj.getObjectField( "q" ) ;
   updator = updateObj.getObjectField( "u" ) ;
   updateMulti = updateObj.getBoolField( "multi" ) ;
   _isUpsert = updateObj.getBoolField( "upsert" ) ;

   // set flag
   if ( FALSE == updateMulti )
   {
      update->flags |= FLG_UPDATE_ONE ;
   }
   if ( _isUpsert )
   {
      update->flags |= FLG_UPDATE_UPSERT ;
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
      BOOLEAN hasId = FALSE ;

      // has _id or not
      BSONObjIterator i( updator ) ;
      while ( i.more() )
      {
         BSONElement ele = i.next() ;
         if ( ele.isABSONObj() && ele.Obj().hasField( "_id" ) )
         {
            _idObj = BSON( "_id" << ele.Obj().getField( "_id" ) ) ;
            hasId = TRUE ;
            break ;
         }
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
      if ( !hasId )
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

   sdbMsg.write( query, TRUE ) ;
   sdbMsg.write( updator, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoUpdateCommand::buildReply( const MsgOpReply &sdbReply,
                                       engine::rtnContextBuf &bodyBuf,
                                       _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      // update reply: { ok: 1, n: 1, nModified: 1 }
      // upsert reply: { ok: 1, n: 1, nModified: 0,
      //                 upserted: [ { index: 0, _id: xxx } ] }
      BSONObj resObj( bodyBuf.data() ) ;
      BSONObjBuilder bob ;

      bob.append( "ok", 1 ) ;

      //n
      if ( resObj.hasField( "InsertedNum" ) &&
           resObj.getIntField( "InsertedNum" ) > 0 )
      {
         bob.append( "n", resObj.getIntField( "InsertedNum" ) ) ;
      }
      else if ( resObj.hasField( "UpdatedNum" ) )
      {
         bob.append( "n", resObj.getIntField( "UpdatedNum" ) ) ;
      }

      //nModified
      if ( resObj.hasField( "ModifiedNum" ) )
      {
         bob.append( "nModified", resObj.getIntField( "ModifiedNum" ) ) ;
      }

      //upserted
      if ( resObj.hasField( "InsertedNum" ) &&
           resObj.getIntField( "InsertedNum" ) > 0 )
      {
         BSONArrayBuilder sub( bob.subarrayStart( "upserted" ) ) ;
         sub.append( BSON( "index" << 0 <<
                           "_id" << _idObj.getField( "_id" ) ) ) ;
         sub.done() ;
      }
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
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

INT32 _mongoQueryCommand::init( const _mongoMessage *pMsg,
                                mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;
   const CHAR* ptr = NULL ;
   const CHAR* nameInReq = NULL ;
   const _mongoQueryRequest* pReq = NULL ;

   // convert request
   PD_CHECK( MONGO_QUERY_MSG == pMsg->type(),
             SDB_INVALIDARG, error, PDERROR,
             "Unknown message type: %d",
             pMsg->type() ) ;
   pReq = (_mongoQueryRequest*)pMsg ;

   // get cs name and cl name
   nameInReq = pReq->fullCollectionName() ;

   ptr = ossStrstr( nameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( NULL == ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), nameInReq ) ;

   ptr = ossStrstr( nameInReq, FAP_MONGO_DOT ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), nameInReq ) ;

   _csName.assign( nameInReq, ptr - nameInReq ) ;
   _clFullName = nameInReq ;
   escapeDot( _clFullName ) ;

   // get query
   _skip = pReq->nToSkip() ;
   _nReturn = pReq->nToReturn() ;
   _query = BSONObj( pReq->query() ) ;
   if ( pReq->selector() )
   {
      _selector = BSONObj( pReq->selector() ) ;
   }

   // set client type
   _client = ctx.clientInfo.type ;

   _requestID = pMsg->requestID() ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoQueryCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = FLG_QUERY_WITH_RETURNDATA ;

   query->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), query->nameLength + 1, TRUE ) ;

   query->numToSkip = _skip ;
   if ( _nReturn > 0 )
   {
      if ( 1000 == _nReturn && NODEJS_DRIVER == _client )
      {
         // The defalut value of batchSize for NODEJS-driver is 1000,
         // while other driver is 0.
         query->numToReturn = -1 ;
      }
      else
      {
         query->numToReturn = _nReturn ;
      }
   }
   else if ( 0 == _nReturn )
   {
      query->numToReturn = -1 ;
   }
   else
   {
      query->numToReturn = -_nReturn ;
   }

   convertProjection( _selector ) ;

   // eg: cl.find( { $query: {a:1}, $orderby: {b:1}, $hint: "aIdx" } )
   BSONObj cond    = _getQueryObj( _query ) ;
   BSONObj orderby = _getSortObj( _query ) ;
   BSONObj hint    = _getHintObj( _query ) ;

   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( _selector, TRUE ) ;
   sdbMsg.write( orderby, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoQueryCommand::buildReply( const MsgOpReply &sdbReply,
                                      engine::rtnContextBuf &bodyBuf,
                                      _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   // build body
   if ( sdbReply.numReturned > 1 )
   {
      INT32 offset = 0 ;
      msgBuffer tmpBuffer ;
      while ( offset < bodyBuf.size() )
      {
         bson::BSONObj obj( bodyBuf.data() + offset ) ;
         tmpBuffer.write( obj.objdata(), obj.objsize() ) ;
         offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
      }
      bodyBuf = engine::rtnContextBuf( tmpBuffer.data(),
                                       tmpBuffer.size(),
                                       sdbReply.numReturned ) ;
   }

   if ( sdbReply.flags != SDB_OK && sdbReply.flags != SDB_DMS_EOC )
   {
      // reply: { $err: "xxx", code: 1234 }
      BSONObj resObj( bodyBuf.data() ) ;
      BSONObj newObj = BSON( "$err" << resObj.getStringField( "errmsg" )<<
                             "code" << resObj.getIntField( "code" ) ) ;
      bodyBuf = engine::rtnContextBuf( newObj ) ;
   }

   // build header
   mongoResponse res ;
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

   return SDB_OK ;
}

BSONObj _mongoQueryCommand::_getQueryObj( const BSONObj &obj )
{
   bson::BSONObj query ;
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

   return query ;
}

BSONObj _mongoQueryCommand::_getSortObj( const BSONObj &obj )
{
   bson::BSONObj sort ;
   if ( obj.hasField( "$orderby" ) )
   {
      sort = obj.getObjectField( "$orderby" ) ;
   }
   else if ( obj.hasField( "orderby" ) )
   {
      sort = obj.getObjectField( "orderby" ) ;
   }

   return sort ;
}

BSONObj _mongoQueryCommand::_getHintObj( const BSONObj &obj )
{
   bson::BSONObj hint ;
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

   return hint ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoFindCommand)
INT32 _mongoFindCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = FLG_QUERY_WITH_RETURNDATA ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   query->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), query->nameLength + 1, TRUE ) ;

   BSONObj cond, orderby, hint, selector ;
   cond     = _obj.getObjectField( "filter" ) ;
   orderby  = _obj.getObjectField( "sort" ) ;
   selector = _obj.getObjectField( "projection" ) ;
   convertProjection( selector ) ;
   if ( _obj.hasField( "hint" ) )
   {
      hint = BSON( "" << _obj.getStringField( "hint" ) ) ;
   }
   if ( _obj.hasField( "limit" ) )
   {
      query->numToReturn = _obj.getIntField( "limit" ) ;
   }
   if ( _obj.hasField( "skip" ) )
   {
      query->numToSkip = _obj.getIntField( "skip" ) ;
   }

   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( selector, TRUE ) ;
   sdbMsg.write( orderby, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;
   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoFindCommand::buildReply( const MsgOpReply &sdbReply,
                                     engine::rtnContextBuf &bodyBuf,
                                     _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   if ( SDB_OK      == sdbReply.flags ||
        SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build first batch, rc: %d", rc ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

done:
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

INT32 _mongoGetmoreCommand::init( const _mongoMessage *pMsg,
                                  mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;
      rc = _init( pReq ) ;
   }
   else if ( MONGO_GETMORE_MSG == pMsg->type() )
   {
      const _mongoGetmoreRequest* pReq = (_mongoGetmoreRequest*)pMsg ;
      rc = _init( pReq ) ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;
      rc = _init( pReq ) ;
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
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreCommand::_init( const _mongoGetmoreRequest *pReq )
{
   INT32 rc = SDB_OK ;

   // get collection full name
   const CHAR* nameInReq = pReq->fullCollectionName() ;
   _clFullName = nameInReq ;

   // get cs name
   const CHAR* ptr = ossStrstr( nameInReq, FAP_MONGO_DOT ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), nameInReq ) ;
   _csName.assign( nameInReq, ptr - nameInReq ) ;

   // get cursorID
   _cursorID = pReq->cursorID() ;

   // get returned number
   _nToReturn = pReq->nToReturn() ;

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
      _initMsgType = MONGO_GETMORE_MSG ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreCommand::_init( const _mongoQueryRequest *pReq )
{
   // mongo message:
   // fullCollectionName: "foo.$cmd"
   // { getMore: <cursorID>, collection: "bar", batchSize: 1000 }
   INT32 rc = SDB_OK ;
   const CHAR* clShortName = NULL ;
   const CHAR* nameInReq = pReq->fullCollectionName() ;
   BSONObj obj( pReq->query() ) ;

   // check command name
   const CHAR* commandName = obj.firstElementFieldName() ;
   SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
               "Invalid command name" ) ;

   // get cs name
   const CHAR* ptr = ossStrstr( nameInReq, FAP_MONGO_FIELD_NAME_CMD ) ;
   PD_CHECK( ptr, SDB_INVALIDARG, error, PDERROR,
             "Invalid collectionFullName for mongo %s request: %s",
             name(), nameInReq ) ;
   _csName.assign( nameInReq, ptr - nameInReq ) ;

   // get collection full name
   rc = getStringElement( obj, FAP_MONGO_FIELD_NAME_COLLECTION, &clShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_COLLECTION, rc ) ;

   _clFullName = _csName ;
   _clFullName += "." ;
   _clFullName += clShortName ;

   // get cursorID
   rc = getNumberLongElement( obj, commandName, _cursorID ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                commandName, rc ) ;

   // get returned number
   rc = getIntElement( obj, FAP_MONGO_FIELD_NAME_BATCHSIZE, _nToReturn ) ;
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
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreCommand::_init( const _mongoCommandRequest *pReq )
{
   INT32 rc = SDB_OK ;
   const CHAR* clShortName = NULL ;
   BSONObj obj( pReq->metadata() ) ;

   // check command name
   SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
               "Invalid command name" ) ;

   // get cs name
   _csName = pReq->databaseName() ;

   // get cl name
   rc = getStringElement( obj, FAP_MONGO_FIELD_NAME_COLLECTION, &clShortName ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_COLLECTION, rc ) ;

   _clFullName = _csName ;
   _clFullName += "." ;
   _clFullName += clShortName ;

   // get cursorID
   rc = getNumberLongElement( obj, pReq->commandName(), _cursorID ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                pReq->commandName(), rc ) ;

   // get returned number
   rc = getIntElement( obj, FAP_MONGO_FIELD_NAME_BATCHSIZE, _nToReturn ) ;
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
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                         mongoSessionCtx &ctx )
{
   MsgOpGetMore *more = NULL ;

   sdbMsg.reserve( sizeof( MsgOpGetMore ) ) ;
   sdbMsg.advance( sizeof( MsgOpGetMore ) ) ;

   more = ( MsgOpGetMore * )sdbMsg.data() ;
   more->header.opCode = MSG_BS_GETMORE_REQ ;
   more->header.TID = 0 ;
   more->header.routeID.value = 0 ;
   more->header.requestID = _requestID ;
   more->contextID = MGCURSOID_TO_SDBCTXID( _cursorID ) ;
   more->numToReturn = _nToReturn ;

   sdbMsg.doneLen() ;

   return SDB_OK ;
}

INT32 _mongoGetmoreCommand::_buildGetmoreReply( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      msgBuffer tmpBuffer ;
      INT32 offset = 0 ;
      while ( offset < bodyBuf.size() )
      {
         BSONObj obj( bodyBuf.data() + offset ) ;
         offset += ossRoundUpToMultipleX( obj.objsize(), 4 ) ;

         if ( GETMORE_LISTINDEX == _type )
         {
            convertIndexObj( obj, _clFullName ) ;
         }
         else if ( GETMORE_LISTCOLLECTION == _type )
         {
            convertCollectionObj( obj ) ;
         }

         tmpBuffer.write( obj.objdata(), obj.objsize() ) ;
      }
      bodyBuf = engine::rtnContextBuf( tmpBuffer.data(),
                                       tmpBuffer.size(),
                                       bodyBuf.recordNum() ) ;
   }
   else if ( SDB_DMS_EOC == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf() ;
   }

   mongoResponse res ;
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

   return SDB_OK ;
}

INT32 _mongoGetmoreCommand::_buildQueryReply( const MsgOpReply &sdbReply,
                                              engine::rtnContextBuf &bodyBuf,
                                              _mongoResponseBuffer &headerBuf )
{
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
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreCommand::_buildCommandReply( const MsgOpReply &sdbReply,
                                                engine::rtnContextBuf &bodyBuf,
                                                _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;
   mongoCommandResponse res ;

   if ( SDB_OK == sdbReply.flags || SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildNextBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build next batch, rc: %d", rc ) ;
   }
   appendEmptyObj2Buf( bodyBuf ) ;

   res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
   res.header.responseTo = _requestID ;
   headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreCommand::buildReply( const MsgOpReply &sdbReply,
                                        engine::rtnContextBuf &bodyBuf,
                                        _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   if( MONGO_GETMORE_MSG == _initMsgType )
   {
      _buildGetmoreReply( sdbReply, bodyBuf, headerBuf ) ;
   }
   else if( MONGO_QUERY_MSG == _initMsgType )
   {
      _buildQueryReply( sdbReply, bodyBuf, headerBuf ) ;
   }
   if( MONGO_COMMAND_MSG == _initMsgType )
   {
      _buildCommandReply( sdbReply, bodyBuf, headerBuf ) ;
   }

   return SDB_OK ;
}

INT32 _mongoGetmoreCommand::_buildNextBatch( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf )
{
   // {xxx}, {xxx}... =>
   // { cursor: { nextBatch: [{xxx}, {xxx}...], id: 0, ns: "foo.bar" },
   //   ok: 1 }
   INT32 rc = SDB_OK ;
   bson::BSONObjBuilder resultBuilder ;
   bson::BSONObjBuilder cursorBuilder ;

   if ( SDB_OK != sdbReply.flags && SDB_DMS_EOC != sdbReply.flags )
   {
      return SDB_OK ;
   }

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
            convertIndexObj( obj, _clFullName ) ;
            arr.append( obj ) ;
         }
         else if ( GETMORE_LISTCOLLECTION == _type )
         {
            convertCollectionObj( obj ) ;
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
   resultBuilder.append( "ok", 1 ) ;
   bodyBuf = engine::rtnContextBuf( resultBuilder.obj() ) ;

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoKillCursorCommand)
INT32 _mongoKillCursorCommand::init( const _mongoMessage *pMsg,
                                     mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_KILL_CURSORS_MSG == pMsg->type() )
   {
      const _mongoKillCursorsRequest* pReq = (_mongoKillCursorsRequest*)pMsg ;

      // get cursorID list
      const INT64 *pCursorID = pReq->cursorIDs() ;
      for( INT32 i = 0 ; i < pReq->numCursors() ; i++ )
      {
         _cursorList.push_back( *pCursorID ) ;
         pCursorID += sizeof( INT64 ) ;
      }

      _isInitialized = TRUE ;
      _initMsgType = MONGO_KILL_CURSORS_MSG ;
   }
   else if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;
      BSONObj arr ;
      BSONObj obj( pReq->query() ) ;

      // check command name
      const CHAR* commandName = obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      // get cursorID list
      rc = getArrayElement( obj, FAP_MONGO_FIELD_NAME_CURSORS, arr ) ;
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
            _cursorList.push_back( ele.numberLong() ) ;
         }
      }

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;
      BSONObj obj, arr ;

      PD_CHECK( 0 == ossStrcmp( pReq->commandName(), name() ),
                SDB_INVALIDARG, error, PDERROR,
                "Invalid command name for mongo %s request: %s",
                name(), pReq->commandName() ) ;

      // get cursorID list
      obj = BSONObj( pReq->metadata() ) ;
      rc = getArrayElement( obj, FAP_MONGO_FIELD_NAME_CURSORS, arr ) ;
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
            _cursorList.push_back( ele.numberLong() ) ;
         }
      }

      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoKillCursorCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                            mongoSessionCtx &ctx )
{
   INT32 rc                = SDB_OK ;
   MsgOpKillContexts *kill = NULL ;

   if ( _cursorList.size() > 1 )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      ctx.setError( rc, "Killing two or more cursors is not supported " ) ;
      goto error ;
   }

   sdbMsg.reserve( sizeof( MsgOpKillContexts ) ) ;
   sdbMsg.advance( sizeof( MsgOpKillContexts ) - sizeof( SINT64 ) ) ;

   kill = ( MsgOpKillContexts * )sdbMsg.data() ;
   kill->header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
   kill->header.TID = 0 ;
   kill->header.routeID.value = 0 ;
   kill->header.requestID = _requestID ;
   kill->ZERO = 0 ;
   kill->numContexts = _cursorList.size() ;

   for( vector<INT64>::iterator it = _cursorList.begin() ;
        it != _cursorList.end() ; it++ )
   {
      INT64 contextID = MGCURSOID_TO_SDBCTXID( *it ) ;
      if ( contextID != SDB_INVALID_CONTEXTID )
      {
         sdbMsg.write( (CHAR*)&contextID, sizeof( SINT64 ) ) ;
      }
   }

   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoKillCursorCommand::buildReply( const MsgOpReply &sdbReply,
                                           engine::rtnContextBuf &bodyBuf,
                                           _mongoResponseBuffer &headerBuf )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   if ( MONGO_KILL_CURSORS_MSG == _initMsgType )
   {
      // do not reply
      headerBuf.usedSize = 0 ;
   }
   else if ( MONGO_QUERY_MSG == _initMsgType )
   {
      if ( SDB_OK == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
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
         bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
      }
      appendEmptyObj2Buf( bodyBuf ) ;

      mongoCommandResponse res ;
      res.header.msgLen = sizeof( mongoCommandResponse ) + bodyBuf.size() ;
      res.header.responseTo = _requestID ;
      headerBuf.setData( (const CHAR*)&res, sizeof( res ) ) ;
   }

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCountCommand)
INT32 _mongoCountCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   query->nameLength = ossStrlen( cmdName ) ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   if ( _obj.hasField( "skip" ) )
   {
      query->numToSkip = _obj.getIntField( "skip" ) ;
   }
   if ( _obj.hasField( "limit" ) )
   {
      query->numToReturn = _obj.getIntField( "limit" ) ;
   }

   // mongo command: db.bar.count( { a: 1 }, { hint: "aIdx" } )
   // sdb msg: matcher: { a: 1 }
   //          hint:    { Collection: "bar", Hint: { "": "aIdx" } }
   BSONObj empty, hint ;
   BSONObj matcher = _obj.getObjectField( "query" ) ;

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

   sdbMsg.write( matcher, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;
   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoCountCommand::buildReply( const MsgOpReply &sdbReply,
                                      engine::rtnContextBuf &bodyBuf,
                                      _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      // reply: { n: 1, ok: 1 }
      BSONObj resObj( bodyBuf.data() ) ;
      BSONObjBuilder bob ;
      bob.append( "n", resObj.getIntField( "Total" ) ) ;
      bob.append( "ok", 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoAggregateCommand)
void _mongoAggregateCommand::_convertAggrSumIfExist( const BSONElement& ele,
                                                     BSONObjBuilder& builder )
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

void _mongoAggregateCommand::_convertAggrGroup( const BSONObj& groupObj,
                                                vector<BSONObj>& newStageList )
{
   /* cl.aggregate( { $group: { _id: "$a" ) )
    * MongoDB return { _id: <value of a> }, but SequoiaDB return all fields.
    * So we should convert:
    * { $group: { _id: "$a", total: { $sum: "$b" } } } ==>
    * { $group: { _id: "$a", total: { $sum: "$b" }, tmp_id_field: "$a" } },
    * { $project: { _id: "$tmp_id_field", total: 1 } }
    */
   if ( 0 != ossStrcmp( groupObj.firstElementFieldName(), "$group" ) )
   {
      return ;
   }

   BSONObj groupValue = groupObj.getObjectField( "$group" ) ;
   const CHAR* idValue = groupValue.getStringField( "_id" ) ;

   if ( '$' == idValue[0] )
   {
      BSONObjBuilder bobGroup, bobProj ;
      BSONObjIterator itr( groupValue ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;
         _convertAggrSumIfExist( e, bobGroup ) ;
         if ( 0 == ossStrcmp( e.fieldName(), "_id" ) )
         {
            bobProj.append( "_id", "$tmp_id_field" ) ;
         }
         else
         {
            bobProj.append( e.fieldName(), 1 ) ;
         }
      }
      bobGroup.append( "tmp_id_field", idValue ) ;

      newStageList.push_back( BSON( "$group" << bobGroup.obj() ) ) ;
      newStageList.push_back( BSON( "$project" << bobProj.obj() ) ) ;
   }
   else
   {
      BSONObjBuilder bobGroup ;
      BSONObjIterator itr( groupValue ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;
         _convertAggrSumIfExist( e, bobGroup ) ;
      }
      newStageList.push_back( BSON( "$group" << bobGroup.obj() ) ) ;
   }
}

INT32 _mongoAggregateCommand::_convertAggrProject( BSONObj& projectObj,
                                                   BSONObj& errorObj )
{
   INT32 rc = SDB_OK ;

   if ( 0 != ossStrcmp( projectObj.firstElementFieldName(), "$project" ) )
   {
      return rc ;
   }

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
      builder.append( "ok", 0 ) ;
      builder.append( "errmsg",
                      "Exclusion fields is not supported" ) ;
      builder.append( "code", rc ) ;
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

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoAggregateCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                           mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpAggregate *aggre   = NULL ;

   sdbMsg.reserve( sizeof( MsgOpAggregate ) ) ;
   sdbMsg.advance( sizeof( MsgOpAggregate ) - 4 ) ;

   aggre = ( MsgOpAggregate * )sdbMsg.data() ;
   aggre->header.opCode = MSG_BS_AGGREGATE_REQ ;
   aggre->header.TID = 0 ;
   aggre->header.routeID.value = 0 ;
   aggre->header.requestID = _requestID ;
   aggre->version = 0 ;
   aggre->w = 0 ;
   aggre->padding = 0 ;
   aggre->flags = 0 ;
   aggre->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), aggre->nameLength + 1, TRUE ) ;

   /* eg: { pipeline: [ { $match: { b: 1 } },
                        { $group: { _id: "$a", b: { $sum: "$b" } } }
                      ] } */
   BSONObjIterator it( _obj.getObjectField( "pipeline" ) ) ;
   while ( it.more() )
   {
      BSONElement ele = it.next() ;
      if ( ele.type() != Object )
      {
         sdbMsg.write( ele.rawdata(), ele.size(), TRUE ) ;
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
         sdbMsg.write( oneStage, TRUE ) ;
      }
      else if ( 0 == ossStrcmp( oneStage.firstElementFieldName(), "$group" ) )
      {
         std::vector<BSONObj> newStageList ;
         _convertAggrGroup( oneStage, newStageList ) ;
         for( std::vector<BSONObj>::iterator it = newStageList.begin() ;
              it != newStageList.end() ; it++ )
         {
            sdbMsg.write( *it, TRUE ) ;
         }
      }
      else
      {
         sdbMsg.write( oneStage, TRUE ) ;
      }
   }
   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoAggregateCommand::buildReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf,
                                          _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   if ( _obj.hasField( "cursor" ) )
   {
      if ( SDB_OK      == sdbReply.flags ||
           SDB_DMS_EOC == sdbReply.flags )
      {
         rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to build first batch, rc: %d", rc ) ;
      }

      _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;
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
         bob.append( "ok", 1 ) ;

         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      else if ( SDB_DMS_EOC == sdbReply.flags )
      {
         bodyBuf = engine::rtnContextBuf( BSON( "result" << BSONArray() <<
                                                "ok" << 1 ) ) ;
      }

      _buildReplyCommon( sdbReply, bodyBuf, headerBuf, TRUE ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDistinctCommand)
INT32 _mongoDistinctCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                          mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   /* mongo request:
    * { distinct: "bar", key: "a", query: { b: 1 } }
    */
   MsgOpAggregate *aggre   = NULL ;

   sdbMsg.reserve( sizeof( MsgOpAggregate ) ) ;
   sdbMsg.advance( sizeof( MsgOpAggregate ) - 4 ) ;

   aggre = ( MsgOpAggregate * )sdbMsg.data() ;
   aggre->header.opCode = MSG_BS_AGGREGATE_REQ ;
   aggre->header.TID = 0 ;
   aggre->header.routeID.value = 0 ;
   aggre->header.requestID = _requestID ;
   aggre->version = 0 ;
   aggre->w = 0 ;
   aggre->padding = 0 ;
   aggre->flags = 0 ;

   aggre->nameLength = _clFullName.length() ;
   sdbMsg.write( _clFullName.c_str(), aggre->nameLength + 1, TRUE ) ;

   std::string distinctfield = "$" ;
   distinctfield += _obj.getStringField( "key" ) ;

   // distinct( "a", { b: 1 } ) =>
   // { $match: { b: 1 } },
   // { $group: { _id: "$a" } },
   // { $group: { _id: null, values: { $addtoset: "$a" } } }
   BSONObj match, group1, group2 ;
   BSONObjBuilder builder ;

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
      sdbMsg.write( match, TRUE ) ;
   }
   sdbMsg.write( group1, TRUE ) ;
   sdbMsg.write( group2, TRUE ) ;
   sdbMsg.doneLen() ;

   return SDB_OK ;
}

INT32 _mongoDistinctCommand::buildReply( const MsgOpReply &sdbReply,
                                         engine::rtnContextBuf &bodyBuf,
                                         _mongoResponseBuffer &headerBuf )
{
   // reply: { values: [ 1, 3, 4 ], ok: 1 }
   if ( SDB_OK == sdbReply.flags )
   {
      BSONObjBuilder bob ;
      bob.appendElements( BSONObj( bodyBuf.data() ) ) ;
      bob.append( "ok", 1 ) ;
      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   else if ( SDB_DMS_EOC == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "values" << BSONArray() <<
                                             "ok" << 1 ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCreateCLCommand)
INT32 _mongoCreateCLCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                          mongoSessionCtx &ctx )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   BSONObj cond, empty ;
   cond = BSON( FIELD_NAME_NAME << _clFullName.c_str() ) ;
   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoCreateCLCommand::buildReply( const MsgOpReply &sdbReply,
                                         engine::rtnContextBuf &bodyBuf,
                                         _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropCLCommand)
INT32 _mongoDropCLCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   BSONObj cond, empty ;
   cond = BSON( FIELD_NAME_NAME << _clFullName.c_str() ) ;
   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoDropCLCommand::buildReply( const MsgOpReply &sdbReply,
                                       engine::rtnContextBuf &bodyBuf,
                                       _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "ns" << _clFullName.c_str() <<
                                              "ok" << 1 ) ) ;
   }
   else if ( SDB_DMS_NOTEXIST == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "ok" << 0 <<
                                              "errmsg" << "ns not found" ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListIdxCommand)
INT32 _mongoListIdxCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                         mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES ;


   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   query->nameLength = ossStrlen( cmdName ) ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   BSONObj empty ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( BSON( FIELD_NAME_COLLECTION << _clFullName.c_str() ), TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoListIdxCommand::buildReply( const MsgOpReply &sdbReply,
                                        engine::rtnContextBuf &bodyBuf,
                                        _mongoResponseBuffer &headerBuf )
{
   INT32 rc = SDB_OK ;

   if ( SDB_OK      == sdbReply.flags ||
        SDB_DMS_EOC == sdbReply.flags )
   {
      rc = _buildFirstBatch( sdbReply, bodyBuf ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build first batch, rc: %d", rc ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

done:
   return rc ;
error:
   goto done ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCreateIdxCommand)
INT32 _mongoCreateIdxCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                           mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   query->nameLength = ossStrlen( cmdName ) ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   // { createIndexes: "bar", indexes: [ { key: {a:1}, name: "aIdx" } ] }
   BSONObj objList, obj ;
   rc = getArrayElement( _obj, FAP_MONGO_FIELD_NAME_INDEXES, objList ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to get field[%s], rc: %d",
                FAP_MONGO_FIELD_NAME_INDEXES, rc ) ;
   PD_CHECK( 1 == objList.nFields() && Object == objList.firstElement().type(),
             SDB_INVALIDARG, error, PDERROR,
             "Invalid object[%s] in mongo %s request",
             _obj.toString().c_str(), name() ) ;
   obj = objList.firstElement().Obj() ;

   {
   // mongo unique index => sequoiadb unique enforce index
   // { key: {a:1}, name: "aIdx", unique: true } =>
   // { Index: { key: {a:1}, name: "aIdx", unique: true, enforced: true },
   //   Collection: "foo.bar" }
   BSONObjBuilder bob ;
   BSONObj indexObj = BSON( IXM_FIELD_NAME_KEY <<
                            obj.getObjectField( "key" ) <<
                            IXM_FIELD_NAME_NAME <<
                            obj.getStringField( "name") <<
                            IXM_FIELD_NAME_UNIQUE <<
                            obj.getBoolField( "unique" ) <<
                            IXM_FIELD_NAME_ENFORCED <<
                            obj.getBoolField( "unique" ) );
   bob.append( FIELD_NAME_INDEX, indexObj ) ;
   bob.append( FIELD_NAME_COLLECTION, _clFullName.c_str() ) ;

   sdbMsg.write( bob.obj(), TRUE ) ;
   sdbMsg.write( BSONObj(), TRUE ) ;
   sdbMsg.write( BSONObj(), TRUE ) ;
   sdbMsg.write( BSONObj(), TRUE ) ;
   sdbMsg.doneLen() ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoCreateIdxCommand::buildReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf,
                                          _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDeleteIdxCommand)

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropIdxCommand)
INT32 _mongoDropIdxCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                         mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   query->nameLength = ossStrlen( cmdName ) ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   // mongo message:
   // { deleteIndexes: "bar", index: "aIdx" } or
   // { dropIndexes: "bar", index: "aIdx" }
   BSONObj empty ;
   BSONObj cond = BSON( FIELD_NAME_COLLECTION <<
                        _clFullName.c_str() <<
                        FIELD_NAME_INDEX <<
                        BSON( "" << _obj.getStringField( "index" ) ) ) ;

   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoDropIdxCommand::buildReply( const MsgOpReply &sdbReply,
                                        engine::rtnContextBuf &bodyBuf,
                                        _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropDatabaseCommand)
INT32 _mongoDropDatabaseCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                              mongoSessionCtx &ctx )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   BSONObj obj, empty ;
   obj = BSON( FIELD_NAME_NAME << _csName.c_str() ) ;

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoDropDatabaseCommand::buildReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "dropped" << _csName.c_str() <<
                                              "ok" << 1 ) ) ;
   }
   else if ( SDB_DMS_CS_NOTEXIST == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoCreateUserCommand)
INT32 _mongoCreateUserCommand::init( const _mongoMessage *pMsg,
                                     mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* commandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoCreateUserCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;
   MsgAuthCrtUsr *user = NULL ;
   BSONObj obj ;
   const CHAR *pUserName = NULL ;
   const CHAR *pTextPasswd = NULL ;
   BOOLEAN digestPasswd = FALSE ;
   string md5PwdStr ;

   sdbMsg.reserve( sizeof( MsgAuthCrtUsr ) ) ;
   sdbMsg.advance( sizeof( MsgAuthCrtUsr ) ) ;

   user = ( MsgAuthCrtUsr * )sdbMsg.data() ;
   user->header.opCode = MSG_AUTH_CRTUSR_REQ ;
   user->header.TID = 0 ;
   user->header.routeID.value = 0 ;
   user->header.requestID = _requestID ;

   pUserName = _obj.getStringField( name() ) ;
   pTextPasswd = _obj.getStringField( FAP_MONGO_FIELD_NAME_PWD ) ;

   BSONElement ele = _obj.getField( FAP_MONGO_FIELD_NAME_DIGESTPWD ) ;
   if ( ele.eoo() )
   {
      digestPasswd = TRUE ;
   }
   else
   {
      digestPasswd = ele.trueValue() ;
   }
   if ( FALSE == digestPasswd )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      ctx.setError( rc, "'digestPasswd' should be true, "
                        "use 'db.runCommand({createUser: ... })' instead" ) ;
      goto error ;
   }

   md5PwdStr = md5::md5simpledigest( pTextPasswd ) ;

   obj = BSON( SDB_AUTH_USER << pUserName <<
               SDB_AUTH_PASSWD << md5PwdStr.c_str() <<
               SDB_AUTH_TEXTPASSWD << pTextPasswd ) ;

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoCreateUserCommand::buildReply( const MsgOpReply &sdbReply,
                                           engine::rtnContextBuf &bodyBuf,
                                           _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
   }
   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoDropUserCommand)
INT32 _mongoDropUserCommand::init( const _mongoMessage *pMsg,
                                   mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* commandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoDropUserCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;
   MsgAuthDelUsr *auth = NULL ;
   BSONObj obj ;

   sdbMsg.reserve( sizeof( MsgAuthDelUsr ) ) ;
   sdbMsg.advance( sizeof( MsgAuthDelUsr ) ) ;

   auth = ( MsgAuthDelUsr * )sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_DELUSR_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = _requestID ;

   // eg: { dropUser: "myuser" }
   const CHAR* userName = _obj.getStringField( name() ) ;
   if ( 0 != ossStrcmp( userName, ctx.userName.c_str() ) )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      ctx.setError( rc, "Only current user can be dropped" ) ;
      goto error ;
   }

   obj = BSON( SDB_AUTH_USER << userName <<
               SDB_AUTH_NONCE << ctx.authInfo.nonce <<
               SDB_AUTH_IDENTIFY << ctx.authInfo.identify <<
               SDB_AUTH_PROOF << ctx.authInfo.clientProof <<
               SDB_AUTH_TYPE << ctx.authInfo.type ) ;
   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoDropUserCommand::buildReply( const MsgOpReply &sdbReply,
                                         engine::rtnContextBuf &bodyBuf,
                                         _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( FAP_MONGO_FIELD_NAME_OK << 1 ) ) ;
   }
   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListUserCommand)
INT32 _mongoListUserCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   INT32 rc            = SDB_OK ;
   MsgOpQuery *query   = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_USERS ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   BSONObj empty ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoListUserCommand::buildReply( const MsgOpReply &sdbReply,
                                         engine::rtnContextBuf &bodyBuf,
                                         _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK == sdbReply.flags )
   {
      BSONObjBuilder bob ;
      BSONArrayBuilder arr( bob.subarrayStart( FAP_MONGO_FIELD_NAME_USERS ) ) ;
      INT32 offset = 0 ;
      while ( offset < bodyBuf.size() )
      {
         BSONObj obj( bodyBuf.data() + offset ) ;
         // { User: "myuser", ... } => { user: "myuser" }
         arr.append( BSON( FAP_MONGO_FIELD_NAME_USER <<
                           obj.getStringField( FIELD_NAME_USER ) ) ) ;
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

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoSaslStartCommand)
INT32 _mongoSaslStartCommand::init( const _mongoMessage *pMsg,
                                    mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* commandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSaslStartCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;
   MsgAuthentication *auth = NULL ;
   const CHAR* userName = NULL ;
   const CHAR* clientNonce = NULL ;
   CHAR* p = NULL ;
   CHAR* nextPtr = NULL ;
   const CHAR* payload = NULL ;
   INT32 payloadLen = 0 ;
   CHAR payloadCpy[ FAP_MONGO_PAYLOADD_MAX_SIZE ] = { 0 } ;
   BSONObj obj ;

   // get payload
   BSONElement ele = _obj.getField( FAP_MONGO_FIELD_NAME_PAYLOAD ) ;
   payload = ele.binData( payloadLen ) ;
   if ( payloadLen > FAP_MONGO_PAYLOADD_MAX_SIZE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossMemcpy( payloadCpy, payload, payloadLen ) ;

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
   userName = p + 2 ;

   p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
   if ( NULL == p )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   clientNonce = p + 2 ;

   // build message
   sdbMsg.reserve( sizeof( MsgAuthentication ) ) ;
   sdbMsg.advance( sizeof( MsgAuthentication ) ) ;

   auth = ( MsgAuthentication * ) sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_VERIFY1_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = _requestID ;

   obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_1 <<
               SDB_AUTH_USER << userName <<
               SDB_AUTH_NONCE << clientNonce <<
               SDB_AUTH_TYPE << SDB_AUTH_TYPE_EXTEND_PWD ) ;
   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

   ctx.userName = userName ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSaslStartCommand::buildReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf,
                                          _mongoResponseBuffer &headerBuf )
{
  /* Generate server-first-message on the form:
   * r=client-nonce+server-nonce,s=user-salt,i=iteration-count
   */
   stringstream ss ;
   string payload ;
   BSONObjBuilder bob ;
   BSONObj replyObj( bodyBuf.data() ) ;
   BOOLEAN rebuildBuf = FALSE ;

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
         const CHAR* salt = replyObj.getStringField( SDB_AUTH_SALT ) ;
         UINT32 iterationCount = replyObj.getIntField( SDB_AUTH_ITERATIONCOUNT ) ;
         const CHAR* nonce = replyObj.getStringField( SDB_AUTH_NONCE ) ;

         ss << FAP_MONGO_SASL_MSG_RANDOM  FAP_MONGO_EQUAL << nonce
            << FAP_MONGO_COMMA
            << FAP_MONGO_SASL_MSG_SALT    FAP_MONGO_EQUAL << salt
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

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoSaslContinueCommand)
INT32 _mongoSaslContinueCommand::init( const _mongoMessage *pMsg,
                                       mongoSessionCtx &ctx )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   if ( MONGO_QUERY_MSG == pMsg->type() )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      _obj = BSONObj( pReq->query() ) ;
      const CHAR* commandName = _obj.firstElementFieldName() ;
      SDB_ASSERT( 0 == ossStrcmp( commandName, name() ),
                  "Invalid command name" ) ;

      _isInitialized = TRUE ;
      _initMsgType = MONGO_QUERY_MSG ;
   }
   else if ( MONGO_COMMAND_MSG == pMsg->type() )
   {
      const _mongoCommandRequest* pReq = (_mongoCommandRequest*)pMsg ;

      SDB_ASSERT( 0 == ossStrcmp( pReq->commandName(), name() ),
                  "Invalid command name" ) ;

      _obj = BSONObj( pReq->metadata() ) ;
      _isInitialized = TRUE ;
      _initMsgType = MONGO_COMMAND_MSG ;
   }
   else
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Unknown message type: %d",
                   pMsg->type() ) ;
   }

   _requestID = pMsg->requestID() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSaslContinueCommand::buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx )
{
   INT32 rc = SDB_OK ;
   MsgAuthentication *auth = NULL ;
   const CHAR* channel = NULL ;
   const CHAR* nonce = NULL ;
   const CHAR* clientProof = NULL ;
   CHAR* p = NULL ;
   CHAR* nextPtr = NULL ;
   const CHAR* payload = NULL ;
   INT32 payloadLen = 0 ;
   CHAR payloadCpy[ FAP_MONGO_PAYLOADD_MAX_SIZE ] = { 0 } ;
   BSONObj obj ;

   // get payload
   BSONElement ele = _obj.getField( FAP_MONGO_FIELD_NAME_PAYLOAD ) ;
   payload = ele.binData( payloadLen ) ;
   if ( payloadLen > FAP_MONGO_PAYLOADD_MAX_SIZE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if( 0 == payloadLen || 'v' == payload[0] )
   {
      // step 3 format: { payload: "" } or { payload: "v=ServerSignature" }
      _step = MONGO_AUTH_STEP3 ;
      goto done ;
   }

   ossMemcpy( payloadCpy, payload, payloadLen ) ;

  /* The payload forms is:
   * c=channel-binding(base64),r=client-nonce+server-nonce,p=ClientProof
   */
   p = ossStrtok( payloadCpy, FAP_MONGO_COMMA, &nextPtr ) ;
   if ( NULL == p )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   channel = p + 2 ;

   p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
   if ( NULL == p )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   nonce = p + 2 ;

   p = ossStrtok( NULL, FAP_MONGO_COMMA, &nextPtr ) ;
   if ( NULL == p )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   clientProof = p + 2 ;

   // build message
   sdbMsg.reserve( sizeof( MsgAuthentication ) ) ;
   sdbMsg.advance( sizeof( MsgAuthentication ) ) ;

   auth = ( MsgAuthentication * ) sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_VERIFY1_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = _requestID ;

   obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_2 <<
               SDB_AUTH_USER << ctx.userName <<
               SDB_AUTH_NONCE << nonce <<
               SDB_AUTH_IDENTIFY << channel <<
               SDB_AUTH_PROOF << clientProof <<
               SDB_AUTH_TYPE << SDB_AUTH_TYPE_EXTEND_PWD ) ;
   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

   ctx.authInfo.nonce = nonce ;
   ctx.authInfo.identify = channel ;
   ctx.authInfo.clientProof = clientProof ;
   ctx.authInfo.type = SDB_AUTH_TYPE_EXTEND_PWD ;
done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoSaslContinueCommand::buildReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
{
   BSONObjBuilder bob ;
   BOOLEAN rebuildBuf = FALSE ;

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
         const CHAR* serverProof = replyObj.getStringField( SDB_AUTH_PROOF ) ;
         string payload = FAP_MONGO_SASL_MSG_VALUE FAP_MONGO_EQUAL ;
         payload += serverProof ;

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

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListCollectionCommand)
INT32 _mongoListCollectionCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                                mongoSessionCtx &ctx )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   // filter cs[foo]'s collections
   // condition: { Name: { $gt: "foo.", $lt: "foo/" }, ... }
   BSONObj empty ;
   BSONObjBuilder builder ;

   string lowBound = _csName ;
   lowBound += "." ;
   string upBound = _csName ;
   upBound += "/" ;
   builder.appendElements( _obj.getObjectField( "filter" ) ) ;
   builder.append( "Name", BSON( "$gt" << lowBound << "$lt" << upBound ) ) ;

   sdbMsg.write( builder.obj(), TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoListCollectionCommand::buildReply( const MsgOpReply &sdbReply,
                                               engine::rtnContextBuf &bodyBuf,
                                               _mongoResponseBuffer &headerBuf )
{
   if ( SDB_OK      == sdbReply.flags ||
        SDB_DMS_EOC == sdbReply.flags )
   {
      _buildFirstBatch( sdbReply, bodyBuf ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoListDatabaseCommand)
INT32 _mongoListDatabaseCommand::buildSdbMsg( msgBuffer &sdbMsg,
                                              mongoSessionCtx &ctx )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc            = SDB_OK ;
   MsgOpQuery *query   = NULL ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES ;

   sdbMsg.reserve( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = _requestID ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   query->nameLength = ossStrlen( cmdName ) ;
   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   BSONObj empty ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 _mongoListDatabaseCommand::buildReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
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
      bob.append( "ok", 1 ) ;

      bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
   }
   else if ( SDB_DMS_EOC == sdbReply.flags )
   {
      bodyBuf = engine::rtnContextBuf( BSON( "databases" << BSONArray() <<
                                             "ok" << 1 ) ) ;
   }

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetLogCommand)
INT32 _mongoGetLogCommand::buildReply( const MsgOpReply &sdbReply,
                                       engine::rtnContextBuf &bodyBuf,
                                       _mongoResponseBuffer &headerBuf )
{
   bson::BSONObjBuilder bob ;
   bob.append( "totalLinesWritten", 0 ) ;
   bob.append( "log", BSONArray() ) ;
   bob.append( "ok", 1 ) ;
   bodyBuf = engine::rtnContextBuf( bob.obj() ) ;

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
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
      goto error ;
   }

   if ( MONGO_QUERY_MSG == pMsg->type() && !ctx.hasParsedClientInfo )
   {
      const _mongoQueryRequest* pReq = (_mongoQueryRequest*)pMsg ;

      BSONObj obj = BSONObj( pReq->query() ) ;
      if ( obj.hasField( "client" ) )
      {
         BSONObj clientObj = obj.getObjectField( "client" ) ;
         BSONObj driverObj = clientObj.getObjectField( "driver" ) ;
         const CHAR* driverName   = driverObj.getStringField( "name" ) ;
         const CHAR* driverVerStr = driverObj.getStringField( "version" ) ;

         rc = parseClientInfo( driverName, driverVerStr, ctx.clientInfo ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to parse client info, rc: %d", rc ) ;

         ctx.hasParsedClientInfo = TRUE ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _mongoIsMasterCommand::buildReply( const MsgOpReply &sdbReply,
                                         engine::rtnContextBuf &bodyBuf,
                                         _mongoResponseBuffer &headerBuf )
{
   BSONObjBuilder bob ;
   bob.append( "ismaster", TRUE ) ;
   bob.append( "maxBsonObjectSize", 16*1024*1024 ) ;
   bob.append( "maxMessageSizeBytes", SDB_MAX_MSG_LENGTH ) ;
   bob.append( "maxWriteBatchSize", 1000 ) ;
   bob.append( "maxWireVersion", 4 ) ; // correspions to mongodb3.2
   bob.append( "minWireVersion", 0 ) ;
   bob.append( "ok", 1 ) ;
   bodyBuf = engine::rtnContextBuf( bob.obj() ) ;

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoBuildinfoCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoBuildInfoCommand)
INT32 _mongoBuildInfoCommand::buildReply( const MsgOpReply &sdbReply,
                                          engine::rtnContextBuf &bodyBuf,
                                          _mongoResponseBuffer &headerBuf )
{
   bson::BSONObjBuilder bob ;
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
   bob.append( "ok", 1 ) ;
   bodyBuf = engine::rtnContextBuf( bob.obj() ) ;

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetLastErrorCommand)
INT32 _mongoGetLastErrorCommand::init( const _mongoMessage *pMsg,
                                       mongoSessionCtx &ctx )
{
   _errorInfoObj = ctx.lastErrorObj.getOwned() ;

   return _mongoGlobalCommand::init( pMsg, ctx ) ;
}

INT32 _mongoGetLastErrorCommand::buildReply( const MsgOpReply &sdbReply,
                                             engine::rtnContextBuf &bodyBuf,
                                             _mongoResponseBuffer &headerBuf )
{
   bson::BSONObjBuilder bob ;
   INT32 errCode = SDB_OK ;

   BSONElement ele = _errorInfoObj.getField( OP_ERRNOFIELD ) ;
   if ( !ele.eoo() )
   {
      errCode = ele.numberInt() ;
   }

   if ( SDB_OK == errCode )
   {
      bob.append( "ok", 1 ) ;
      bob.appendNull( "err" ) ;
   }
   else
   {
      bob.append( "ok", 1 ) ;
      bob.append( "code", errCode ) ;
      bob.append( "err", _errorInfoObj.getStringField( OP_ERRDESP_FIELD ) ) ;
   }

   bodyBuf = engine::rtnContextBuf( bob.obj() ) ;

   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

INT32 _mongoDummyCommand::buildReply( const MsgOpReply &sdbReply,
                                      engine::rtnContextBuf &bodyBuf,
                                      _mongoResponseBuffer &headerBuf )
{
   bodyBuf = engine::rtnContextBuf( BSON( "ok" << 1 ) ) ;
   _buildReplyCommon( sdbReply, bodyBuf, headerBuf ) ;

   return SDB_OK ;
}

MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoWhatsMyUriCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoLogoutCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetReplStatCommand)
MONGO_IMPLEMENT_CMD_AUTO_REGISTER(_mongoGetCmdLineOptsCommand)

}
