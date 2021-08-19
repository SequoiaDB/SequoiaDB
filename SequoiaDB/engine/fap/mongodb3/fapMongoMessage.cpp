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

   Source File Name = fapMongoMessage.cpp

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
#include "fapMongoMessage.hpp"
#include "../../bson/bson.hpp"

using namespace bson ;

namespace fap
{

INT32 _mongoMessage::init( const CHAR* pMsg )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   _pMsg = pMsg ;
   _pCurrent = pMsg ;

   mongoMsgHeader* header = ( mongoMsgHeader* )_pMsg ;
   _msgLen = header->msgLen ;
   _requestID = header->requestId ;
   _responseTo = header->responseTo ;
   _opCode = header->opCode ;

   _pCurrent += sizeof( mongoMsgHeader ) ;
   if ( _pCurrent <= _pMsg + _msgLen )
   {
      _isInitialized = TRUE ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }

   return rc ;
}

INT32 _mongoMessage::_readIntAndAdvance( INT32 &value )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;

   if ( _pCurrent + sizeof( value ) <= _pMsg + _msgLen )
   {
      value = *((INT32*)_pCurrent) ;
      _pCurrent += sizeof( value ) ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }

   return rc ;
}

INT32 _mongoMessage::_readIntAndAdvance( INT64 &value )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;

   if ( _pCurrent + sizeof( value ) <= _pMsg + _msgLen )
   {
      value = *((INT64*)_pCurrent) ;
      _pCurrent += sizeof( value ) ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }

   return rc ;
}

INT32 _mongoMessage::_readStringAndAdvance( const CHAR *&str )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;

   INT32 strLen= ossStrlen( _pCurrent ) + 1 ;
   if ( _pCurrent + strLen <= _pMsg + _msgLen )
   {
      str = _pCurrent ;
      _pCurrent += strLen ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }

   return rc ;
}

INT32 _mongoMessage::_readObjectAndAdvance( const CHAR *&data )
{
   SDB_ASSERT ( _isInitialized, "must be initialized first" ) ;

   INT32 rc = SDB_OK ;

   try
   {
      BSONObj obj( _pCurrent ) ;
      if ( _pCurrent + obj.objsize() <= _pMsg + _msgLen )
      {
         data = _pCurrent ;
         _pCurrent = _pCurrent + obj.objsize() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
   catch ( std::exception &e )
   {
      PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                   "Occured exception: %s", e.what() ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

BOOLEAN _mongoMessage::_more() const
{
   SDB_ASSERT ( _pCurrent && _pMsg, "_pCurrent or _pMsg is null" ) ;
   return _pCurrent < _pMsg + _msgLen ? TRUE : FALSE ;
}

INT32 _mongoQueryRequest::init( const CHAR* pMsg )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;
   /*
    * The structure of QUERY message is as follows:
    *
    *    mongoMsgHeader header
    *    INT32 flags
    *    string fullCollectionName
    *    INT32 numToSkip
    *    INT32 numToReturn
    *    BSONObj query
    *    BSONObj returnFieldsSelector  // optional
    */
   rc = _mongoMessage::init( pMsg ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( MONGO_OP_QUERY != _opCode )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _readIntAndAdvance( _flags ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readStringAndAdvance( _pCollectionFullName ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readIntAndAdvance( _nToSkip ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readIntAndAdvance( _nToReturn ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readObjectAndAdvance( _pQuery ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( !_more() )
   {
      goto done ;
   }

   rc = _readObjectAndAdvance( _pSelector ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( _more() )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoGetmoreRequest::init( const CHAR* pMsg )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;
   INT32 reservedFlags = 0 ;

   /*
    * The structure of GETMORE message is as follows:
    *
    *    mongoMsgHeader header
    *    INT32 ZERO
    *    string fullCollectionName
    *    INT32 numToReturn
    *    INT64 cursorID
    */
   rc = _mongoMessage::init( pMsg ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( MONGO_OP_GET_MORE != _opCode )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _readIntAndAdvance( reservedFlags ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readStringAndAdvance( _pCollectionFullName ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readIntAndAdvance( _nToReturn ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readIntAndAdvance( _cursorID ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( _more() )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoKillCursorsRequest::init( const CHAR* pMsg )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;
   INT32 reservedFlags = 0 ;

   /*
    * The structure of KILLCURSORS message is as follows:
    *
    *    mongoMsgHeader header
    *    INT32 ZERO
    *    INT32 numOfCursorIDs
    *    INT64[] cursorIDs
    */
   rc = _mongoMessage::init( pMsg ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( MONGO_OP_KILL_CURSORS != _opCode )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _readIntAndAdvance( reservedFlags ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readIntAndAdvance( _numCursors ) ;
   if ( rc )
   {
      goto error ;
   }

   for( INT32 i = 0 ; i < _numCursors ; i++ )
   {
      _cursorList = (INT64*)_current() ;
      INT64 cursorID = 0 ;
      rc = _readIntAndAdvance( cursorID ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   if ( _more() )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _mongoCommandRequest::init( const CHAR* pMsg )
{
   SDB_ASSERT( !_isInitialized, "alreadby initialized" ) ;

   INT32 rc = SDB_OK ;

   /*
    * The structure of COMMAND message is as follows:
    *
    *    mongoMsgHeader header
    *    string database
    *    string commandName
    *    BSONObj metadata
    *    BSONObj commandArgs
    *    BSONObj inputDocs     // optional
    */
   rc = _mongoMessage::init( pMsg ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( MONGO_OP_COMMAND != _opCode )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _readStringAndAdvance( _databaseName ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readStringAndAdvance( _commandName ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readObjectAndAdvance( _pMetadata ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _readObjectAndAdvance( _pCommandArgs ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   if ( SDB_OK == rc )
   {
      _isInitialized = TRUE ;
   }
   return rc ;
error:
   goto done ;
}

}
