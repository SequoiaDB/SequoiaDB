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

   Source File Name = pmdLocalSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/04/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_LOCAL_SESSION_HPP_
#define PMD_LOCAL_SESSION_HPP_

#include "pmdSession.hpp"
#include "pmdDef.hpp"
#include "rtnContext.hpp"
#include "../bson/bson.h"
#include "msgConvertor.hpp"

using namespace bson ;

namespace engine
{
   /*
      _pmdLocalSession define
   */
   class _pmdLocalSession : public _pmdSession
   {
      public:
         _pmdLocalSession( SOCKET fd ) ;
         virtual ~_pmdLocalSession () ;

         virtual INT32            getServiceType() const ;
         virtual SDB_SESSION_TYPE sessionType() const ;

         virtual INT32            run() ;

         virtual INT32 checkPrivilegesForCmd( const CHAR *cmdName,
                                              const CHAR *pQuery,
                                              const CHAR *pSelector,
                                              const CHAR *pOrderby,
                                              const CHAR *pHint ) ;

         virtual INT32 checkPrivilegesForActionsOnExact( const CHAR *pCollectionName,
                                                         const authActionSet &actions ) ;

         virtual INT32 checkPrivilegesForActionsOnCluster( const authActionSet &actions ) ;

         virtual INT32 checkPrivilegesForActionsOnResource(
            const boost::shared_ptr< authResource > &resource,
            const authActionSet &actions );

         virtual INT32 getACL( boost::shared_ptr< const authAccessControlList > &acl ) ;

      protected:
         INT32          _processMsg( MsgHeader *msg ) ;
         INT32          _preprocessMsg( MsgHeader *&msg ) ;
         virtual INT32  _onMsgBegin( MsgHeader *msg ) ;
         virtual void   _onMsgEnd( INT32 result, MsgHeader *msg ) ;

         INT32          _recvSysInfoMsg( UINT32 msgSize, CHAR **ppBuff,
                                         INT32 &buffLen ) ;
         INT32          _processSysInfoRequest( const CHAR *msg ) ;

         BOOLEAN        _clientVersionMatch() const ;

         BOOLEAN        _msgConvertorEnabled() const ;

         INT32          _enableMsgConvertor() ;

         INT32          _reply( MsgOpReply* responseMsg, const CHAR *pBody,
                                INT32 bodyLen ) ;

         INT32          _replyInCompatibleMode( MsgOpReply *responseMsg,
                                                const CHAR *data,
                                                INT32 dataLen ) ;

         INT32          _replyInNormalMode( MsgOpReply *responseMsg,
                                            const CHAR *data,
                                            INT32 dataLen ) ;

      protected:
         virtual void            _onAttach () ;
         virtual void            _onDetach () ;

      private:
         void _saveOrSetMsgGlobalID( MsgHeader *pMsg ) ;

      protected:
         MsgOpReply           _replyHeader ;
         BOOLEAN              _needReply ;

         BSONObj              _errorInfo ;

         IMsgConvertor        *_inMsgConvertor ;   // For request from client.
         IMsgConvertor        *_outMsgConvertor ;  // For reply to client.

         boost::shared_ptr<const authAccessControlList> _acl;
   } ;
   typedef _pmdLocalSession pmdLocalSession ;

}

#endif //PMD_LOCAL_SESSION_HPP_


