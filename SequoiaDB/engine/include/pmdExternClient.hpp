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

   Source File Name = pmdExternClient.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/11/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_EXTERN_CLIENT_HPP_
#define PMD_EXTERN_CLIENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossSocket.hpp"
#include "sdbInterface.hpp"
#include "pmdDef.hpp"
#include "../bson/bson.hpp"
#include <string>
#include "utilAuthSCRAMSHA.hpp"

using namespace std ;

namespace engine
{

   class _pmdEDUCB ;

   /*
      _pmdExternClient define
   */
   class _pmdExternClient : public IClient
   {
      public:
         _pmdExternClient( ossSocket *pSocket ) ;
         virtual ~_pmdExternClient() ;

         void attachCB( _pmdEDUCB *cb ) { _pEDUCB = cb ; }
         void detachCB() { _pEDUCB = NULL ; }

      public:
         virtual SDB_CLIENT_TYPE clientType() const ;
         virtual const CHAR*     clientName() const ;

         virtual INT32        authenticate( MsgHeader *pMsg,
                                            const CHAR **authBuf = NULL ) ;
         virtual INT32        authenticate( const CHAR *username,
                                            const CHAR *password ) ;
         virtual void         logout() ;
         virtual INT32        disconnect() ;

         virtual BOOLEAN      isAuthed() const ;
         virtual BOOLEAN      isConnected() const ;
         virtual BOOLEAN      isClosed() const ;

         virtual UINT16       getLocalPort() const ;
         virtual const CHAR*  getLocalIPAddr() const ;
         virtual UINT16       getPeerPort() const ;
         virtual const CHAR*  getPeerIPAddr() const ;
         virtual const CHAR*  getUsername() const ;
         virtual const CHAR*  getPassword() const ;

         virtual const CHAR*  getFromIPAddr() const ;
         virtual UINT16       getFromPort() const ;

         void                 setAuthed( BOOLEAN authed ) ;

      public:
         ossSocket*           getSocket() { return _pSocket ; }

      protected:
         void                 _makeName() ;
         INT32                _processAuthRequestObj( const bson::BSONObj &reqObj,
                                                      INT32 opCode,
                                                      const CHAR **userName,
                                                      const CHAR **password ) ;
         INT32                _processAuthResponse( INT32 opCode ) ;

      protected:
         string               _username ;
         string               _password ;
         BOOLEAN              _isAuthed ;
         ossSocket*           _pSocket ;
         _pmdEDUCB*           _pEDUCB ;
         bson::BSONObj        _authReturnedObj ; // object returned by authenticate
         BOOLEAN              _step1Done ;
         CHAR                 _combineNonce[ UTIL_AUTH_SCRAMSHA_COMBINE_NONCE_LEN + 1 ] ;

         UINT16               _localPort ;
         UINT16               _peerPort ;
         CHAR                 _localIP[ PMD_IPADDR_LEN + 1 ] ;
         CHAR                 _peerIP[ PMD_IPADDR_LEN + 1 ] ;
         CHAR                 _fromIP[ PMD_IPADDR_LEN + 1 ] ;
         CHAR                 _clientName[ PMD_CLIENTNAME_LEN + 1 ] ;

   } ;
   typedef _pmdExternClient pmdExternClient ;

}

#endif //PMD_EXTERN_CLIENT_HPP_

