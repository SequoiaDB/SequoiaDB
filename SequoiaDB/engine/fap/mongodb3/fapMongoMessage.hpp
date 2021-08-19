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

   Source File Name = fapMongoMessage.hpp

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
#ifndef _SDB_MONGO_MESSAGE_HPP_
#define _SDB_MONGO_MESSAGE_HPP_

#include "ossUtil.hpp"
#include "mongodef.hpp"
#include "pd.hpp"

namespace fap
{

enum MONGO_MSG_TYPE
{
   MONGO_QUERY_MSG         = 1,
   MONGO_GETMORE_MSG       = 2,
   MONGO_KILL_CURSORS_MSG  = 3,
   MONGO_COMMAND_MSG       = 4,

   MONGO_UNKNOWN_MSG       = 127,
} ;

enum MONGO_MSG_OPCODE
{
   MONGO_OP_REPLY         = 1,
   MONGO_OP_UPDATE        = 2001,
   MONGO_OP_INSERT        = 2002,
   MONGO_OP_QUERY         = 2004,
   MONGO_OP_GET_MORE      = 2005,
   MONGO_OP_DELETE        = 2006,
   MONGO_OP_KILL_CURSORS  = 2007,
   MONGO_OP_COMMAND       = 2010,
   MONGO_OP_COMMAND_REPLY = 2011,
} ;

#pragma pack(1)
struct mongoMsgHeader
{
   INT32 msgLen ;
   INT32 requestId ;
   INT32 responseTo ;
   INT32 opCode ;
};
#pragma pack()

class _mongoMessage : public SDBObject
{
public:
   _mongoMessage()
   : _pMsg( NULL ),
     _msgLen( 0 ),
     _requestID( 0 ),
     _responseTo( 0 ),
     _opCode( 0 ),
     _isInitialized( FALSE ),
     _pCurrent( NULL )
   {}

   virtual ~_mongoMessage() {}

   virtual MONGO_MSG_TYPE type() const { return MONGO_UNKNOWN_MSG ; }

   virtual INT32 init( const CHAR* pMsg ) ;

   const CHAR* msg() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pMsg ;
   }
   const CHAR* data() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pMsg + sizeof( mongoMsgHeader ) ;
   }
   mongoMsgHeader* header() const
   {
      return ( mongoMsgHeader* )msg() ;
   }

   INT32 msgLen() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _msgLen ;
   }
   INT32 requestID() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _requestID ;
   }
   INT32 responseTo() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _responseTo ;
   }
   INT32 opCode() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _opCode ;
   }

protected:
   INT32 _readIntAndAdvance( INT32 &value ) ;
   INT32 _readIntAndAdvance( INT64 &value ) ;
   INT32 _readStringAndAdvance( const CHAR *&str ) ;
   INT32 _readObjectAndAdvance( const CHAR *&data ) ;
   BOOLEAN _more() const ;
   const CHAR* _current() const { return _pCurrent ;}

protected:
   const CHAR*    _pMsg ;
   INT32          _msgLen ;
   INT32          _requestID ;
   INT32          _responseTo ;
   INT32          _opCode ;

private:
   BOOLEAN        _isInitialized ;
   const CHAR*    _pCurrent ;
} ;
typedef _mongoMessage mongoMessage ;

class _mongoQueryRequest : public _mongoMessage
{
public:
   _mongoQueryRequest()
   : _flags( 0 ),
     _pCollectionFullName( NULL ),
     _nToSkip( 0 ),
     _nToReturn( 0 ),
     _pQuery( NULL ),
     _pSelector( NULL ),
     _isInitialized( FALSE )
   {}

   virtual ~_mongoQueryRequest() {}

   virtual MONGO_MSG_TYPE type() const { return MONGO_QUERY_MSG ; }

   virtual INT32 init( const CHAR* pMsg ) ;

   INT32 flags() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _flags ;
   }
   const CHAR* fullCollectionName() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pCollectionFullName ;
   }
   INT32 nToSkip() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _nToSkip ;
   }
   INT32 nToReturn() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _nToReturn ;
   }
   const CHAR* query() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pQuery ;
   }
   const CHAR* selector() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pSelector ;
   }

private:
   INT32       _flags ;
   const CHAR* _pCollectionFullName ;
   INT32       _nToSkip ;
   INT32       _nToReturn ;
   const CHAR* _pQuery ;
   const CHAR* _pSelector ;
   BOOLEAN     _isInitialized ;
} ;
typedef _mongoQueryRequest mongoQueryRequest ;

class _mongoGetmoreRequest : public _mongoMessage
{
public:
   _mongoGetmoreRequest()
   : _pCollectionFullName( NULL ),
     _nToReturn( 0 ),
     _cursorID( 0 ),
     _isInitialized( FALSE )
   {}

   virtual ~_mongoGetmoreRequest() {}

   virtual MONGO_MSG_TYPE type() const { return MONGO_GETMORE_MSG ; }

   virtual INT32 init( const CHAR* pMsg ) ;

   const CHAR* fullCollectionName() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pCollectionFullName ;
   }
   INT32 nToReturn() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _nToReturn ;
   }
   INT64 cursorID() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _cursorID ;
   }

private:
   const CHAR* _pCollectionFullName ;
   INT32       _nToReturn ;
   INT64       _cursorID ;
   BOOLEAN     _isInitialized ;
} ;
typedef _mongoGetmoreRequest mongoGetmoreRequest ;

class _mongoKillCursorsRequest : public _mongoMessage
{
public:
   _mongoKillCursorsRequest()
   : _numCursors( 0 ),
     _cursorList( NULL ),
     _isInitialized( FALSE )
   {}

   virtual ~_mongoKillCursorsRequest() {}

   virtual MONGO_MSG_TYPE type() const { return MONGO_KILL_CURSORS_MSG ; }

   virtual INT32 init( const CHAR* pMsg ) ;

   INT32 numCursors() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _numCursors ;
   }
   const INT64* cursorIDs() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _cursorList ;
   }

private:
   INT32        _numCursors ; // number of cursorIDs in message
   const INT64* _cursorList ;
   BOOLEAN      _isInitialized ;
} ;
typedef _mongoKillCursorsRequest mongoKillCursorsRequest ;

class _mongoCommandRequest : public _mongoMessage
{
public:
   _mongoCommandRequest()
   : _databaseName( NULL ),
     _commandName( NULL ),
     _pMetadata( NULL ),
     _pCommandArgs( NULL ),
     _isInitialized( FALSE )
   {}

   virtual ~_mongoCommandRequest() {}

   virtual MONGO_MSG_TYPE type() const { return MONGO_COMMAND_MSG ; }

   virtual INT32 init( const CHAR* pMsg ) ;

   const CHAR* databaseName() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _databaseName ;
   }
   const CHAR* commandName() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _commandName ;
   }
   const CHAR* metadata() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pMetadata ;
   }
   const CHAR* commandArgs() const
   {
      SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;
      return _pCommandArgs ;
   }

private:
   const CHAR* _databaseName ;
   const CHAR* _commandName ;
   const CHAR* _pMetadata ;
   const CHAR* _pCommandArgs ;
   BOOLEAN     _isInitialized ;
} ;
typedef _mongoCommandRequest mongoCommandRequest ;

// Use these flags when replying messages to mongo client
enum MONGO_REPLY_FLAG
{
   MONGO_REPLY_FLAG_NONE               = 0,

   // returned, with zero results, when getMore is called but the cursor id
   // is not valid at server.
   MONGO_REPLY_FLAG_CURSOR_NOT_FOUND   = 1 << 0,

   // { $err: ... } is being returned
   MONGO_REPLY_FLAG_QUERY_FAILURE      = 1 << 1,

   MONGO_REPLY_FALG_SHARD_CONFIG_STALE = 1 << 2,

   MONGO_REPLY_FALG_AWAIT_CAPABLE      = 1 << 3,
} ;

#pragma pack(1)
struct _mongoResponse
{
   mongoMsgHeader header ;
   INT32 reservedFlags ;
   SINT64 cursorId ;
   INT32 startingFrom ;
   INT32 nReturned ;

   _mongoResponse()
   {
      header.msgLen = 0 ;
      header.requestId = 0 ;
      header.responseTo = 0 ;
      header.opCode = MONGO_OP_REPLY ;
      reservedFlags = MONGO_REPLY_FLAG_NONE ;
      cursorId = MONGO_INVALID_CURSORID ;
      startingFrom = 0 ;
      nReturned = 0 ;
   }
} ;
typedef _mongoResponse mongoResponse ;
#pragma pack()

struct _mongoCommandResponse
{
   mongoMsgHeader header ;

   _mongoCommandResponse()
   {
      header.msgLen = 0 ;
      header.requestId = 0 ;
      header.responseTo = 0 ;
      header.opCode = MONGO_OP_COMMAND_REPLY ;
   }
} ;
typedef _mongoCommandResponse mongoCommandResponse ;

// sizeof( mongoResponse ) is 36, sizeof( mongoCommandResponse ) is 16.
// Allocate a space to store response, no matter what kind of response.
#define MONG_RESPONSE_MAX_SIZE 36

struct _mongoResponseBuffer
{
   CHAR data[MONG_RESPONSE_MAX_SIZE] ;
   INT32 usedSize ;

   _mongoResponseBuffer()
   {
      ossMemset( data, 0, MONG_RESPONSE_MAX_SIZE ) ;
      usedSize = 0 ;
   }

   void setData( const CHAR* p, INT32 size )
   {
      ossMemcpy( data, p, size ) ;
      usedSize = size ;
   }
} ;
typedef _mongoResponseBuffer mongoResponseBuffer ;
}
#endif
