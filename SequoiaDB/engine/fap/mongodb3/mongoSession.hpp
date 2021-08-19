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

   Source File Name = mongoSession.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_MSG_CONVERTER_HPP_
#define _SDB_MONGO_MSG_CONVERTER_HPP_

#include "dpsLogWrapper.hpp"
#include "rtnContextBuff.hpp"
#include "pmdSession.hpp"
#include "fapMongoMessage.hpp"
#include "msgBuffer.hpp"
#include "fapMongoCommand.hpp"
#include "pmdRemoteSession.hpp"

namespace fap
{

/*
   _mongoSession define
*/
class _mongoSession : public engine::pmdSession, public engine::IRemoteMsgPreprocessor
{
public:
   _mongoSession( SOCKET fd, engine::IResource *resource ) ;
   virtual ~_mongoSession() ;

   virtual INT32 getServiceType() const ;
   virtual engine::SDB_SESSION_TYPE sessionType() const ;

   virtual INT32 run() ;

   virtual BOOLEAN preProcess( engine::pmdEDUEvent &event ) ;

protected:
   virtual void  _onAttach() {}
   virtual void  _onDetach() {}

protected:
   INT32 _processMsg( const CHAR *pMsg, const _mongoCommand *pCommand ) ;
   INT32 _processMsg( const CHAR *pMsg ) ;
   INT32 _onMsgBegin( MsgHeader *msg ) ;
   INT32 _onMsgEnd( INT32 result, MsgHeader *msg ) ;
   INT32 _recvMsg( CHAR *&pMsg,
                   BOOLEAN &recvFromEvent,
                   engine::pmdEDUEvent &event ) ;
   INT32 _recvFromSocket( CHAR *&pMsg, BOOLEAN &recvSomething ) ;
   INT32 _buildResponse( _mongoCommand *pCommand,
                         CHAR *&pRes ) ;
   INT32 _reply( _mongoCommand *pCommand ) ;
   INT32 _reply( _mongoCommand *pCommand, const CHAR* pMsg,
                 INT32 errCode, const BSONObj &errObj ) ;

private:
   void  _resetBuffers() ;
   INT32 _setSeesionAttr() ;
   INT32 _autoCreateCS( const CHAR *csName ) ;
   INT32 _autoCreateCL( const CHAR *clFullName ) ;
   BOOLEAN _isOwnedCursor( const _mongoCommand *pCommand,
                           UINT64 &ownedEDUID,
                           BOOLEAN &needAuth ) ;
   INT32 _manageCursor( const _mongoCommand *pCommand,
                        const MsgOpReply &sdbReply ) ;

private:
   MsgOpReply              _replyHeader ;
   BOOLEAN                 _masterRead ;
   engine::rtnContextBuf   _contextBuff ;
   BSONObj                 _errorInfo ;

   msgBuffer               _inBuffer ;
   msgBuffer               _tmpBuffer ;
   engine::IResource      *_resource ;

   std::set<INT64>         _cursorList ;
   BOOLEAN                 _needWaitResponse ;

   BOOLEAN                 _isAuthed ;
   queue<engine::pmdEDUEvent> _tmpEventQue ;
} ;

typedef _mongoSession mongoSession ;

}
#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
