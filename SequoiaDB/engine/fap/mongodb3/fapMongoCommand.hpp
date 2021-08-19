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

   Source File Name = fapMongoCommand.hpp

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
#ifndef _FAP_MONGO_COMMAND_HPP_
#define _FAP_MONGO_COMMAND_HPP_

#include "ossUtil.hpp"
#include "fapMongoCommandDef.hpp"
#include "fapMongoMessage.hpp"
#include "msgBuffer.hpp"
#include "rtnContextBuff.hpp"
#include "msg.hpp"

using namespace std ;
using namespace bson ;

namespace fap
{

#define MONGO_DECLARE_CMD_AUTO_REGISTER() \
   public: \
      static _mongoCommand *newThis () ; \

#define MONGO_IMPLEMENT_CMD_AUTO_REGISTER(theClass) \
_mongoCommand *theClass::newThis () \
{ \
   return SDB_OSS_NEW theClass() ;\
} \
_mongoCmdAssit theClass##Assit ( theClass::newThis ) ; \

class _mongoCommand : public SDBObject
{
   public:
      _mongoCommand() {}
      virtual ~_mongoCommand() {}

      virtual MONGO_CMD_TYPE type() const = 0 ;
      virtual const CHAR* name() const = 0 ;

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) = 0 ;

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) = 0 ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &bodyBuf,
                                _mongoResponseBuffer &headerBuf ) = 0 ;

      virtual BOOLEAN needProcessByEngine() const = 0 ;

      virtual const CHAR* csName() const = 0 ;
      virtual const CHAR* clFullName() const = 0 ;

      virtual BOOLEAN isInitialized() const = 0 ;
} ;
typedef _mongoCommand mongoCommand ;

typedef _mongoCommand* (*MONGO_CMD_NEW_FUNC)() ;

class _mongoCmdAssit : public SDBObject
{
   public:
      _mongoCmdAssit ( MONGO_CMD_NEW_FUNC pFunc ) ;
      ~_mongoCmdAssit () {}
};

struct _mongoCmdInfo : public SDBObject
{
   std::string        cmdName ;
   UINT32             nameSize ;
   MONGO_CMD_NEW_FUNC createFunc ;

   _mongoCmdInfo*     sub ;
   _mongoCmdInfo*     next ;
} ;

class _mongoCmdFactory : public SDBObject
{
   friend class _mongoCmdAssit ;

   public:
      _mongoCmdFactory () ;
      ~_mongoCmdFactory () ;

      _mongoCommand *create ( const CHAR *commandName ) ;
      void  release ( _mongoCommand *pCommand ) ;

   protected:
      INT32 _register( const CHAR * name, MONGO_CMD_NEW_FUNC pFunc ) ;

      INT32 _insert( _mongoCmdInfo * pCmdInfo,
                      const CHAR * name, MONGO_CMD_NEW_FUNC pFunc ) ;
      MONGO_CMD_NEW_FUNC _find( const CHAR * name ) ;

      void _releaseCmdInfo( _mongoCmdInfo *pCmdInfo ) ;

      UINT32 _near( const CHAR *str1, const CHAR *str2 ) ;

   private:
      _mongoCmdInfo *_pCmdInfoRoot ;
};

_mongoCmdFactory* getMongoCmdFactory() ;

INT32 mongoGetAndInitCommand( const CHAR *pMsg,
                              _mongoCommand **ppCommand,
                              mongoSessionCtx &sessCtx,
                              msgBuffer &sdbMsg ) ;

INT32 mongoPostRunCommand( _mongoCommand *pCommand,
                           const MsgOpReply &sdbReply,
                           engine::rtnContextBuf &replyBuf,
                           _mongoResponseBuffer &headerBuf ) ;

INT32 mongoReleaseCommand( _mongoCommand **ppCommand ) ;

class _mongoGlobalCommand : public _mongoCommand
{
   public:
      _mongoGlobalCommand()
      : _requestID( 0 ), _isInitialized( FALSE ),
        _initMsgType( MONGO_UNKNOWN_MSG )
      {}
      virtual ~_mongoGlobalCommand() {}

      virtual MONGO_CMD_TYPE type() const = 0 ;
      virtual const CHAR* name() const    = 0 ;

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) { return SDB_OK ; }

      virtual const CHAR* csName() const          { return NULL ; }
      virtual const CHAR* clFullName() const      { return NULL ; }
      virtual BOOLEAN needProcessByEngine() const { return FALSE ; }
      virtual BOOLEAN isInitialized() const       { return _isInitialized ; }

   protected:
      INT32 _buildReplyCommon( const MsgOpReply &sdbReply,
                               engine::rtnContextBuf &bodyBuf,
                               _mongoResponseBuffer &headerBuf ) ;

   protected:
      INT32          _requestID ;
      BOOLEAN        _isInitialized ;
      MONGO_MSG_TYPE _initMsgType ;
} ;
typedef _mongoGlobalCommand mongoGlobalCommand ;

class _mongoDatabaseCommand : public _mongoCommand
{
   public:
      _mongoDatabaseCommand()
      : _requestID( 0 ),
        _isInitialized( FALSE ),
        _initMsgType( MONGO_UNKNOWN_MSG )
      {}
      virtual ~_mongoDatabaseCommand() {}

      virtual MONGO_CMD_TYPE type() const = 0 ;
      virtual const CHAR* name() const    = 0 ;

      virtual const CHAR* csName() const     { return _csName.c_str() ; }
      virtual const CHAR* clFullName() const { return NULL ; }
      virtual BOOLEAN isInitialized() const  { return _isInitialized ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

   protected:
      INT32 _buildReplyCommon( const MsgOpReply &sdbReply,
                               engine::rtnContextBuf &bodyBuf,
                               _mongoResponseBuffer &headerBuf ) ;
      void _buildFirstBatch( const MsgOpReply &sdbReply,
                             engine::rtnContextBuf &bodyBuf ) ;

   protected:
      string         _csName ;
      BSONObj        _obj ;
      INT32          _requestID ;
      BOOLEAN        _isInitialized ;
      MONGO_MSG_TYPE _initMsgType ;
} ;
typedef _mongoDatabaseCommand mongoDatabaseCommand ;

class _mongoCollectionCommand : public _mongoCommand
{
   public:
      _mongoCollectionCommand()
      : _requestID( 0 ), _isInitialized( FALSE ),
        _initMsgType( MONGO_UNKNOWN_MSG )
      {}
      virtual ~_mongoCollectionCommand() {}

      virtual MONGO_CMD_TYPE type() const = 0 ;
      virtual const CHAR* name() const    = 0 ;

      virtual const CHAR* csName() const { return _csName.c_str() ; }
      virtual const CHAR* clFullName() const { return _clFullName.c_str() ; }
      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }
      virtual BOOLEAN isInitialized() const       { return _isInitialized ; }
      virtual BOOLEAN needConvertDecimal() const { return FALSE ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;

   protected:
      virtual INT32 _buildReplyCommon( const MsgOpReply &sdbReply,
                                       engine::rtnContextBuf &bodyBuf,
                                       _mongoResponseBuffer &headerBuf,
                                       BOOLEAN setCursorID = FALSE ) ;
      INT32 _buildFirstBatch( const MsgOpReply &sdbReply,
                              engine::rtnContextBuf &bodyBuf ) ;

   private:
      INT32 _init( const _mongoQueryRequest *pReq ) ;
      INT32 _init( const _mongoCommandRequest *pReq ) ;

   protected:
      string         _csName ;
      string         _clFullName ;
      BSONObj        _obj ;
      INT32          _requestID ;
      BOOLEAN        _isInitialized ;
      MONGO_MSG_TYPE _initMsgType ;
} ;
typedef _mongoCollectionCommand mongoCollectionCommand ;

class _mongoInsertCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoInsertCommand() {}
      virtual ~_mongoInsertCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_INSERT ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_INSERT ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoInsertCommand mongoInsertCommand ;

class _mongoDeleteCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDeleteCommand() {}
      virtual ~_mongoDeleteCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_DELETE ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_DELETE ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoDeleteCommand _mongoDeleteCommand ;

class _mongoUpdateCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoUpdateCommand() : _isUpsert( FALSE ) {}
      virtual ~_mongoUpdateCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_UPDATE ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_UPDATE ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      BOOLEAN isUpsert() { return _isUpsert ; }

   private:
      BOOLEAN _isUpsert ;
      BSONObj _idObj ;
} ;
typedef _mongoUpdateCommand mongoUpdateCommand ;

class _mongoQueryCommand : public _mongoCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoQueryCommand() ;
      virtual ~_mongoQueryCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_QUERY ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_QUERY ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual const CHAR* csName() const          { return _csName.c_str() ; }
      virtual const CHAR* clFullName() const      { return _clFullName.c_str() ; }
      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }
      virtual BOOLEAN isInitialized() const       { return _isInitialized ; }

   private:
      BSONObj _getQueryObj( const BSONObj &obj ) ;
      BSONObj _getSortObj( const BSONObj &obj ) ;
      BSONObj _getHintObj( const BSONObj &obj ) ;

   private:
      string _csName ;
      string _clFullName ;
      BSONObj _query ;
      BSONObj _selector ;
      INT32 _skip ;
      INT32 _nReturn ;
      MONGO_CLIENT_TYPE _client ;
      INT32 _requestID ;
      BOOLEAN _isInitialized ;
} ;
typedef _mongoQueryCommand mongoQueryCommand ;

class _mongoFindCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoFindCommand() {}
      virtual ~_mongoFindCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_FIND ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_FIND ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoFindCommand mongoFindCommand ;

class _mongoGetmoreCommand : public _mongoCommand
{
   enum GETMORE_TYPE
   {
      GETMORE_LISTCOLLECTION,
      GETMORE_LISTINDEX,
      GETMORE_OTHER // real query or aggregate
   } ;

   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoGetmoreCommand() ;
      virtual ~_mongoGetmoreCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_GETMORE ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_GETMORE ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual const CHAR* csName() const { return _csName.c_str() ; }
      virtual const CHAR* clFullName() const { return _clFullName.c_str() ; }

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }
      virtual BOOLEAN isInitialized() const       { return _isInitialized ; }

      INT64 cursorID() const { return _cursorID ; }

   private:
      INT32 _init( const _mongoGetmoreRequest *pReq ) ;
      INT32 _init( const _mongoQueryRequest *pReq ) ;
      INT32 _init( const _mongoCommandRequest *pReq ) ;

      INT32 _buildGetmoreReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &bodyBuf,
                                _mongoResponseBuffer &headerBuf ) ;
      INT32 _buildQueryReply( const MsgOpReply &sdbReply,
                              engine::rtnContextBuf &bodyBuf,
                              _mongoResponseBuffer &headerBuf ) ;
      INT32 _buildCommandReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &bodyBuf,
                                _mongoResponseBuffer &headerBuf ) ;
      INT32 _buildNextBatch( const MsgOpReply &sdbReply,
                             engine::rtnContextBuf &bodyBuf ) ;

   private:
      string _csName ;
      string _clFullName ;
      INT32 _nToReturn ;
      INT64 _cursorID ;
      INT32 _requestID ;
      GETMORE_TYPE _type ;
      BOOLEAN _isInitialized ;
      MONGO_MSG_TYPE _initMsgType ;
} ;
typedef _mongoGetmoreCommand mongoGetmoreCommand ;

class _mongoKillCursorCommand : public _mongoCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoKillCursorCommand()
      : _requestID( 0 ),
        _isInitialized( FALSE ),
        _initMsgType( MONGO_UNKNOWN_MSG )
      {}
      virtual ~_mongoKillCursorCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_KILL_CURSORS ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_KILL_CURSORS ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual const CHAR* csName() const          { return NULL ; }
      virtual const CHAR* clFullName() const      { return NULL ; }
      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }
      virtual BOOLEAN isInitialized() const       { return _isInitialized ; }
      const vector<INT64>& cursorList() const     { return _cursorList ; }

   private:
      vector<INT64> _cursorList ;
      INT32 _requestID ;
      BOOLEAN _isInitialized ;
      MONGO_MSG_TYPE _initMsgType ;
} ;
typedef _mongoKillCursorCommand mongoKillCursorCommand ;

class _mongoCountCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoCountCommand() {}
      virtual ~_mongoCountCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_COUNT ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_COUNT ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoCountCommand mongoCountCommand ;

class _mongoAggregateCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoAggregateCommand() {}
      virtual ~_mongoAggregateCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_AGGREGATE ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_AGGREGATE ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

   private:
      INT32 _convertAggrProject( BSONObj& projectObj,
                                 BSONObj& errorObj ) ;
      void _convertAggrSumIfExist( const BSONElement& ele,
                                   BSONObjBuilder& builder ) ;
      void _convertAggrGroup( const BSONObj& groupObj,
                              vector<BSONObj>& newStageList ) ;
} ;
typedef _mongoAggregateCommand mongoAggregateCommand ;

class _mongoDistinctCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDistinctCommand() {}
      virtual ~_mongoDistinctCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_DISTINCT ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_DISTINCT ; }
      virtual BOOLEAN needConvertDecimal() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoDistinctCommand mongoDistinctCommand ;

class _mongoCreateCLCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoCreateCLCommand() {}
      virtual ~_mongoCreateCLCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_COLLECTION_CREATE ; }
      virtual const CHAR* name() const
      {
         return MONGO_CMD_NAME_CREATE_COLLECTION ;
      }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

} ;
typedef _mongoCreateCLCommand _mongoCreateCLCommand ;

class _mongoDropCLCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDropCLCommand() {}
      virtual ~_mongoDropCLCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_COLLECTION_DROP ; }
      virtual const CHAR* name() const
      {
         return MONGO_CMD_NAME_DROP_COLLECTION ;
      }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

   private:
      vector<BSONObj> _indexes ;
} ;
typedef _mongoDropCLCommand mongoDropCLCommand ;

class _mongoListIdxCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoListIdxCommand() {}
      virtual ~_mongoListIdxCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_LIST_INDEX ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_LIST_INDEX ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoListIdxCommand mongoListIdxCommand ;

class _mongoCreateIdxCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoCreateIdxCommand() {}
      virtual ~_mongoCreateIdxCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_INDEX_CREATE ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_CREATE_INDEX ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoCreateIdxCommand mongoCreateIdxCommand ;

class _mongoDropIdxCommand : public _mongoCollectionCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDropIdxCommand() {}
      virtual ~_mongoDropIdxCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_INDEX_DROP ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_DROP_INDEX ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoDropIdxCommand mongoDropIdxCommand ;

class _mongoDeleteIdxCommand : public _mongoDropIdxCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDeleteIdxCommand() {}
      virtual ~_mongoDeleteIdxCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_INDEX_DELETE ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_DELETE_INDEX ; }
} ;
typedef _mongoDeleteIdxCommand mongoDeleteIdxCommand ;

class _mongoDropDatabaseCommand : public _mongoDatabaseCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDropDatabaseCommand() {}
      virtual ~_mongoDropDatabaseCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_DATABASE_DROP ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_DROP_DATABASE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoDropDatabaseCommand mongoDropDatabaseCommand ;

class _mongoCreateUserCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoCreateUserCommand() {}
      virtual ~_mongoCreateUserCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_USER_CREATE ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_CREATE_USER ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

   protected:
      BSONObj _obj ;
} ;
typedef _mongoCreateUserCommand mongoCreateUserCommand ;

class _mongoDropUserCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoDropUserCommand() {}
      virtual ~_mongoDropUserCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_USER_DROP ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_DROP_USER ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

   protected:
      BSONObj _obj ;
} ;
typedef _mongoDropUserCommand mongoDropUserCommand ;

class _mongoListUserCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoListUserCommand() {}
      virtual ~_mongoListUserCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_LIST_USER ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_USERS_INFO ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }
} ;
typedef _mongoListUserCommand mongoListUserCommand ;

class _mongoSaslStartCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoSaslStartCommand() {}
      virtual ~_mongoSaslStartCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_SASL_START ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_SASL_START ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

   protected:
      BSONObj _obj ;
} ;
typedef _mongoSaslStartCommand mongoSaslStartCommand ;

class _mongoSaslContinueCommand : public _mongoGlobalCommand
{
   enum MONGO_AUTH_STEP
   {
      MONGO_AUTH_STEP1,    // sasl start
      MONGO_AUTH_STEP2,    // sasl continue
      MONGO_AUTH_STEP3,    // sasl continue
   } ;
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoSaslContinueCommand() : _step( MONGO_AUTH_STEP2 ) {}
      virtual ~_mongoSaslContinueCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_SASL_CONTINUE ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_SASL_CONTINUE ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

      virtual BOOLEAN needProcessByEngine() const
      {
         return _step == MONGO_AUTH_STEP2 ? TRUE : FALSE ;
      }

   protected:
      BSONObj _obj ;
      MONGO_AUTH_STEP _step ;
} ;
typedef _mongoSaslContinueCommand mongoSaslContinueCommand ;

class _mongoListCollectionCommand : public _mongoDatabaseCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoListCollectionCommand() {}
      virtual ~_mongoListCollectionCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_LIST_COLLECTION ; }
      virtual const CHAR* name() const
      {
         return MONGO_CMD_NAME_LIST_COLLECTION ;
      }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoListCollectionCommand mongoListCollectionCommand ;

class _mongoListDatabaseCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoListDatabaseCommand() {}
      virtual ~_mongoListDatabaseCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_LIST_DATABASE ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_LIST_DATABASE ; }

      virtual BOOLEAN needProcessByEngine() const { return TRUE ; }

      virtual INT32 buildSdbMsg( msgBuffer &sdbMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoListDatabaseCommand mongoListDatabaseCommand ;

class _mongoGetLogCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoGetLogCommand() {}
      virtual ~_mongoGetLogCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_GET_LOG ; }
      virtual const CHAR* name() const { return MONGO_CMD_NAME_GET_LOG ; }

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoGetLogCommand mongoGetLogCommand ;

class _mongoIsMasterCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoIsMasterCommand() {}
      virtual ~_mongoIsMasterCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_IS_MASTER ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_IS_MASTER ; }

      virtual INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoIsMasterCommand mongoIsMasterCommand ;

class _mongoIsmasterCommand : public mongoIsMasterCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_ISMASTER ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_ISMASTER ; }
} ;
typedef _mongoIsmasterCommand mongoIsmasterCommand ;

class _mongoBuildInfoCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoBuildInfoCommand() {}
      virtual ~_mongoBuildInfoCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_BUILD_INFO ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_BUILD_INFO ; }

      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoBuildInfoCommand mongoBuildInfoCommand ;

class _mongoBuildinfoCommand : public _mongoBuildInfoCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_BUILDINFO ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_BUILDINFO ; }
} ;
typedef _mongoBuildinfoCommand mongoBuildinfoCommand ;

class _mongoGetLastErrorCommand : public _mongoGlobalCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      _mongoGetLastErrorCommand() {}
      virtual ~_mongoGetLastErrorCommand() {}

      virtual MONGO_CMD_TYPE type() const { return CMD_GET_LAST_ERROR ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_GET_LAST_ERROR ; }

      INT32 init( const _mongoMessage *pMsg, mongoSessionCtx &ctx ) ;
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;

   private:
      BSONObj _errorInfoObj ;
} ;
typedef _mongoGetLastErrorCommand mongoGetLastErrorCommand ;

class _mongoDummyCommand : public _mongoGlobalCommand
{
   public:
      _mongoDummyCommand() {}
      virtual ~_mongoDummyCommand() {}
      virtual INT32 buildReply( const MsgOpReply &sdbReply,
                                engine::rtnContextBuf &replyBuf,
                                _mongoResponseBuffer &resHeader ) ;
} ;
typedef _mongoDummyCommand mongoDummyCommand ;

class _mongoWhatsMyUriCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_WHATS_MY_URI ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_WHATS_MY_URI ; }
} ;
typedef _mongoWhatsMyUriCommand mongoWhatsMyUriCommand ;

class _mongoLogoutCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_LOGOUT ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_LOGOUT ; }
} ;
typedef _mongoLogoutCommand mongoLogoutCommand ;

class _mongoGetReplStatCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_REPL_STAT ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_REPL_STAT ; }
} ;
typedef _mongoGetReplStatCommand mongoGetReplStatCommand ;

class _mongoGetCmdLineOptsCommand : public _mongoDummyCommand
{
   MONGO_DECLARE_CMD_AUTO_REGISTER()
   public:
      virtual MONGO_CMD_TYPE type() const { return CMD_GET_CMD_LINE ; }
      virtual const CHAR* name() const    { return MONGO_CMD_NAME_GET_CMD_LINE ; }
} ;
typedef _mongoGetCmdLineOptsCommand mongoGetCmdLineOptsCommand ;

}
#endif
