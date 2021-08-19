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

   Source File Name = omagentSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_SESSION_HPP_
#define OMAGENT_SESSION_HPP_

#include "omagent.hpp"
#include "pmdAsyncSession.hpp"
#include "netRouteAgent.hpp"
#include <map>
#include "../bson/bson.h"
#include "sptUsrFileCommon.hpp"
using namespace bson ;

namespace engine
{

   class _omAgentNodeMgr ;

   /*
      _omaSession define
   */
   class _omaSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _omaSession ( UINT64 sessionID ) ;
         virtual ~_omaSession () ;

         virtual INT32 getServiceType() const { return SDB_OK ; }

         virtual const CHAR*      sessionName() const ;
         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual const CHAR*      className() const { return "OMAgent" ; }
         virtual EDU_TYPES eduType () const ;

         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         virtual BOOLEAN timeout ( UINT32 interval ) ;

         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;

         // fileObj operation function
         virtual INT32 newFileObj( UINT32 &fID, sptUsrFileCommon** fileObj ) ;
         virtual void releaseFileObj( UINT32 fID ) ;
         virtual sptUsrFileCommon* getFileObjByID( UINT32 fID ) ;
      protected:
         virtual void   _onDetach () ;
         virtual void   _onAttach () ;
         virtual INT32  _defaultMsgFunc ( NET_HANDLE handle, MsgHeader* msg ) ;

      // msg map function
      protected:
         INT32       _onNodeMgrReq( const NET_HANDLE &handle,
                                    MsgHeader *pMsg ) ;
         INT32       _onOMAgentReq( const NET_HANDLE &handle,
                                    MsgHeader *pMsg ) ;
         INT32       _onAuth( const NET_HANDLE &handle,
                              MsgHeader *pMsg ) ;

      protected:

         INT32 _reply ( MsgOpReply *header, const CHAR *pBody,
                        INT32 bodyLen ) ;
         INT32 _reply ( INT32 flags, MsgHeader *pSrcReqMsg,
                        const CHAR *body = NULL,
                        const INT32 *bodyLen = NULL ) ;

         INT32 _buildReplyHeader( MsgHeader *pMsg ) ;

      private:
         void _clearFileObjMap() ;

      private:
         MsgOpReply       _replyHeader ;
         BSONObj          _errorInfo ;

         _omAgentNodeMgr      *_pNodeMgr ;

         ossTimestamp         _lastRecvTime ;
         CHAR                 _detailName[SESSION_NAME_LEN+1] ;

         std::map< UINT32, sptUsrFileCommon* > _fileObjMap ;
         UINT32                                _maxFileObjID ;
   } ;

   typedef _omaSession omaSession ;
}



#endif // OMAGENT_SESSION_HPP_
