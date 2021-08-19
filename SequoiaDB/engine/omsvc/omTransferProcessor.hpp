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

   Source File Name = omTransferProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/08/2015  Lin YouBin Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_TRNASFERPROCESSOR_HPP__
#define OM_TRNASFERPROCESSOR_HPP__

#include "pmdProcessor.hpp"
#include "pmdRemoteSession.hpp"
#include "omSdbConnector.hpp"
#include <map>
#include <string>

using namespace bson;

namespace engine
{
   typedef struct _omNodeInfo 
   {
      string hostName ;
      string service ;
      string user ;
      string passwd ;
      INT32 preferedInstance ;

      _omNodeInfo() 
      {
         hostName = "" ;
         service  = "" ;
         user     = "" ;
         passwd   = "" ;
         preferedInstance = 1 ;
      }

      _omNodeInfo( const _omNodeInfo& nodeInfo )
      {
         hostName = nodeInfo.hostName ;
         service  = nodeInfo.service ;
         user     = nodeInfo.user ;
         passwd   = nodeInfo.passwd ;
         preferedInstance = nodeInfo.preferedInstance ;
      }

   } omNodeInfo ;

   class _omTransferProcessor : public pmdProcessor
   {
      public:
         _omTransferProcessor( list<_omNodeInfo> &nodeList ) ;

         virtual            ~_omTransferProcessor() ;

          

      public:
         virtual INT32                 processMsg( MsgHeader *msg,
                                                   rtnContextBuf &contextBuff,
                                                   INT64 &contextID,
                                                   BOOLEAN &needReply,
                                                   BOOLEAN &needRollback,
                                                   BSONObjBuilder &builder ) ;

         virtual INT32                 doRollback() ;
         virtual INT32                 doCommit() ;

         virtual const CHAR*           processorName() const ;
         virtual SDB_PROCESSOR_TYPE    processorType() const ;

         void                          attach( pmdSession *session ) ;
         void                          detach() ;

      protected:
         virtual void                  _onAttach () ;
         virtual void                  _onDetach () ;

      protected:
         void                          _clearRemoteSession( 
                                             pmdRemoteSessionMgr *rsManager,
                                             pmdRemoteSession *remoteSession ) ;
         INT32                         _sendMsg2Target( 
                                                     const omNodeInfo &nodeInfo, 
                                                     MsgHeader *msg, 
                                                     omSdbConnector **connector,
                                                     MsgHeader **result ) ;

      private:
         list< _omNodeInfo >           _nodeList ;
   };
}

#endif /* OM_TRNASFERPROCESSOR_HPP__ */



