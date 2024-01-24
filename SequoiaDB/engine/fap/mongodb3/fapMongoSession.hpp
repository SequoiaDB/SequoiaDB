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

   Source File Name = fapMongoSession.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/03/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_MSG_CONVERTER_HPP_
#define _SDB_MONGO_MSG_CONVERTER_HPP_

#include "dpsLogWrapper.hpp"
#include "rtnContextBuff.hpp"
#include "pmdSession.hpp"
#include "fapMongoMessage.hpp"
#include "fapMongoUtil.hpp"
#include "fapMongoCommand.hpp"
#include "pmdRemoteSession.hpp"
#include "fapMongoCursor.hpp"
#include "ossQueue.hpp"

namespace fap
{

/*
   _mongoSession define
*/
class _mongoSession : public engine::pmdSession, public engine::IRemoteMsgPreprocessor
{
public:
   _mongoSession( SOCKET fd, engine::IResource *pResource ) ;
   virtual ~_mongoSession() ;

   virtual INT32 getServiceType() const ;
   virtual engine::SDB_SESSION_TYPE sessionType() const ;

   virtual INT32 run() ;

   virtual BOOLEAN preProcess( engine::pmdEDUEvent &event ) ;

protected:
   virtual void  _onAttach() {}
   virtual void  _onDetach() {}

private:
   INT32 _processMsg( const CHAR *pMsg, const _mongoCommand *pCommand,
                      BSONObj &errorObj ) ;
   INT32 _processMsg( const CHAR *pMsg, BSONObj &errorObj ) ;

   INT32 _onMsgBegin( MsgHeader *pMsg ) ;
   void  _onMsgEnd( INT32 result, MsgHeader *pMsg ) ;

   INT32 _recvMsgFromClient( CHAR *&pMsg, BOOLEAN &hasMsg ) ;
   INT32 _recvMsgFromInterior( engine::pmdEDUEvent &event, BOOLEAN &hasMsg ) ;

   INT32 _buildResponse( _mongoCommand *pCommand,
                         CHAR *&pRes ) ;
   INT32 _reply( _mongoCommand *pCommand, const CHAR* pMsg,
                 INT32 errCode, BSONObj &errObj ) ;
   INT32 _reply( engine::pmdEDUEvent &event ) ;

   void  _resetBuffers() ;
   INT32 _autoCreateCS( const CHAR *pCsName, BSONObj &errorObj ) ;
   INT32 _autoCreateCL( const CHAR *pCSName, const CHAR *pClFullName,
                        BSONObj &errorObj ) ;
   INT32 _autoInsert( const CHAR *pClFullName, const BSONObj &matcher,
                      const BSONObj &updatorObj, BSONObj &target,
                      BSONObj &errorObj ) ;
   INT32 _autoKillCursor( UINT64 requestID, INT64 contextID ) ;

   void  _getCursorInfo( const _mongoCommand *pCommand,
                         mongoCursorInfo &cursorInfo,
                         BOOLEAN &isOwned ) ;
   INT32 _manageCursor( const _mongoCommand *pCommand,
                        const MsgOpReply &sdbReply ) ;

   BOOLEAN _shouldAutoCrtCS( const _mongoCommand *pCommand ) ;
   BOOLEAN _shouldAutoCrtCL( const _mongoCommand *pCommand ) ;
   BOOLEAN _shouldBuildGetMoreMsg( const _mongoCommand *pCommand ) ;

   void    _postInnerErrorEvent( INT32 errorCode,
                                 engine::pmdEDUEvent &event ) ;
   void    _eduEventRelease( engine::pmdEDUEvent &event ) ;
   void    _buildErrResponseMsg( CHAR* pMsg, INT32 errorCode,
                                 INT32 &msgLen ) ;

   INT32   _processClientMsg( const CHAR* pMsg,
                              _mongoCommand *&pCommand,
                              mongoSessionCtx &sessCtx,
                              BOOLEAN &msgForwarded ) ;
   INT32   _processOwnedClientMsg( const CHAR* pMsg,
                                   _mongoCommand *pCommand,
                                   mongoSessionCtx &sessCtx,
                                   BOOLEAN &needNext ) ;
   INT32   _processNonOwnedClientMsg( const CHAR* pMsg,
                                      _mongoCommand *pCommand,
                                      mongoCursorInfo cursorInfo,
                                      mongoSessionCtx &sessCtx ) ;

   INT32   _processInteriorMsg( _mongoCommand *&pCommand,
                                engine::pmdEDUEvent &event,
                                mongoSessionCtx &sessCtx,
                                BOOLEAN &hasRecvResponse ) ;
   void    _processRequestMsg( _mongoCommand *&pCommand,
                               engine::pmdEDUEvent &event,
                               mongoSessionCtx &sessCtx ) ;

   void    _buildErrorObj( const engine::rtnContextBuf &contextBuff,
                           INT32 errCode, BSONObj &errObj ) ;

private:
   MsgOpReply              _replyHeader ;
   BOOLEAN                 _masterRead ;
   engine::rtnContextBuf   _contextBuff ;
   mongoMsgBuffer          _inBuffer ;
   mongoMsgBuffer          _tmpBuffer ;
   engine::IResource      *_pResource ;
   std::set<INT64>         _cursorList ;
   BOOLEAN                 _isAuthed ;
   INT32                   _requestIDOfPostEvent ;
   INT32                   _opCodeOfPostEvent ;
   INT64                   _cursorIdOfPostEvent ;
   ossQueue<engine::pmdEDUEvent> _fapEvents ;
   ossQueue<engine::pmdEDUEvent> _coordEvents ;
   const CHAR*             _clFullName ;
} ;

typedef _mongoSession mongoSession ;

}
#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
