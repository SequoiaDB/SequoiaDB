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

#include "util.hpp"
#include "dpsLogWrapper.hpp"
#include "rtnContextBuff.hpp"
#include "pmdSession.hpp"
#include "mongodef.hpp"
#include "parser.hpp"
#include "mongoConverter.hpp"

/*
   _mongoSession define
*/
class _mongoSession : public engine::pmdSession
{
public:
   _mongoSession( SOCKET fd, engine::IResource *resource ) ;
   virtual ~_mongoSession() ;

   virtual INT32 getServiceType() const ;
   virtual engine::SDB_SESSION_TYPE sessionType() const ;

   virtual INT32 run() ;

protected:
   virtual void  _onAttach() {}
   virtual void  _onDetach() {}

protected:
   BOOLEAN _preProcessMsg( msgParser &parser,
                           engine::IResource *resource,
                           engine::rtnContextBuf &buff ) ;
   INT32 _processMsg( const CHAR *pMsg ) ;
   INT32 _onMsgBegin( MsgHeader *msg ) ;
   INT32 _onMsgEnd( INT32 result, MsgHeader *msg ) ;
   INT32 _reply( MsgOpReply *replyHeader, const CHAR *pBody, const INT32 len ) ;

private:
   void  _resetBuffers() ;
   INT32 _setSeesionAttr() ;
   void  _handleResponse( const INT32 opType, engine::rtnContextBuf &buff ) ;

private:
   mongoConverter          _converter ;
   MsgOpReply              _replyHeader ;
   BOOLEAN                 _masterRead ;
   BOOLEAN                 _authed ;
   cursorStartFrom         _cursorStartFrom ;
   engine::rtnContextBuf   _contextBuff ;
   BSONObj                 _errorInfo ;

   msgBuffer               _inBuffer ;
   msgBuffer               _outBuffer ;
   msgBuffer               _tmpBuffer ;
   engine::IResource      *_resource ;
} ;

typedef _mongoSession mongoSession ;

#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
