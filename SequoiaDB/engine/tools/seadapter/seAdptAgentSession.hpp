/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = seAdptAgentSession.hpp

   Descriptive Name = Agent session on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_AGENT_SESSION_HPP_
#define SEADPT_AGENT_SESSION_HPP_

#include "pmdAsyncSession.hpp"
#include "utilCommObjBuff.hpp"
#include "seAdptMgr.hpp"
#include "utilESClt.hpp"
#include "seAdptContext.hpp"

using namespace engine ;

namespace seadapter
{
   class _seAdptAgentSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
   public:
      _seAdptAgentSession( UINT64 sessionID ) ;
      virtual ~_seAdptAgentSession() ;

      virtual EDU_TYPES eduType() const ;
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual const CHAR* className() const { return "SEAdptAgent" ; }

      virtual void onRecieve( const NET_HANDLE netHandle, MsgHeader * msg ) ;
      virtual BOOLEAN timeout( UINT32 interval ) ;
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

   protected:
      virtual void _onAttach() ;
      virtual void _onDetach() ;

      INT32 _onOPMsg ( NET_HANDLE handle, MsgHeader * msg ) ;

      INT32 _onQueryReq( MsgHeader *msg,
                         utilCommObjBuff &objBuff,
                         pmdEDUCB *eduCB = NULL ) ;

      INT32 _onGetmoreReq( MsgHeader *msg,
                           utilCommObjBuff &objBuff,
                           pmdEDUCB *eduCB = NULL ) ;

      INT32 _reply( MsgOpReply *header, const CHAR *buff, UINT32 size ) ;
      INT32 _defaultMsgFunc ( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _selectIndex( const CHAR *clName, const BSONObj &hint,
                          BSONObj &newHint, string &index, string &type ) ;
      BOOLEAN _isTextIdx( const IDX_META_VEC &idxMetas,
                          const string &idxName, string &esIdxName ) ;

   private:
      utilESCltMgr      *_seCltMgr ;
      utilESClt         *_esClt ;
      std::string       _scrollID ;
      seAdptContextBase *_context ;
      BSONObj           _errorInfo ;
   } ;
   typedef _seAdptAgentSession seAdptAgentSession ;
}

#endif /* SEADPT_AGENT_SESSION_HPP_ */

