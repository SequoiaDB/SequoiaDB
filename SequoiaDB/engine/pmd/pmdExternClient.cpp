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

   Source File Name = pmdExternClient.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/11/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/


#include "pmdExternClient.hpp"
#include "authDef.hpp"
#include "pmdEDU.hpp"

#if defined ( SDB_ENGINE )
   #include "clsMgr.hpp"
   #include "omManager.hpp"
   #include "msgAuth.hpp"
   #include "coordCB.hpp"
   #include "clsResourceContainer.hpp"
   #include "coordAuthOperator.hpp"
   #include "msgMessage.hpp"
   #include "netFrame.hpp"
   #include "coordCommandRole.hpp"
#endif // SDB_ENGINE

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define PMD_AUTH_RETRY_TIMES        ( 4 )

   /*
      _pmdExternClient implement
   */
   _pmdExternClient::_pmdExternClient( ossSocket *pSocket )
   {
      SDB_ASSERT( pSocket, "Socket can't be NULL" ) ;

      _isAuthed      = FALSE ;
      _privCheckEnabled = TRUE ;
      _roleID        = AUTH_INVALID_ROLE_ID ;
      _pSocket       = pSocket ;
      _pEDUCB        = NULL ;

      _localPort     = 0 ;
      _peerPort      = 0 ;
      _inMsg         = NULL ;
      _protocolVer   = SDB_PROTOCOL_VER_INVALID ;
      ossMemset( _localIP, 0, sizeof( _localIP ) ) ;
      ossMemset( _peerIP, 0, sizeof( _peerIP ) ) ;
      ossMemset( _fromIP, 0, sizeof( _fromIP ) ) ;
      ossMemset( _clientName, 0, sizeof( _clientName ) ) ;

      if ( pSocket )
      {
         _localPort = pSocket->getLocalPort() ;
         _peerPort = pSocket->getPeerPort() ;
         pSocket->getLocalAddress( _localIP, PMD_IPADDR_LEN ) ;
         pSocket->getPeerAddress( _peerIP, PMD_IPADDR_LEN ) ;

         ossStrcpy( _fromIP, _peerIP ) ;
#if defined ( SDB_ENGINE )
         if ( 0 == ossStrcmp( _peerIP, "127.0.0.1" ) )
         {
            ossIP2Str( _netFrame::getLocalAddress(), _fromIP, PMD_IPADDR_LEN ) ;
         }
#endif // SDB_ENGINE

         _step1Done = FALSE ;
      }

      _makeName() ;
   }

   _pmdExternClient::~_pmdExternClient()
   {
      _pSocket       = NULL ;
      _pEDUCB        = NULL ;
   }

   SDB_CLIENT_TYPE _pmdExternClient::clientType() const
   {
      return SDB_CLIENT_EXTERN ;
   }

   const CHAR* _pmdExternClient::clientName() const
   {
      return _clientName ;
   }

   void _pmdExternClient::_makeName()
   {
      if ( 0 == _peerIP[ 0 ] )
      {
         ossStrcpy( _clientName, "noip-Extern" ) ;
      }
      else if ( _username.empty() )
      {
         ossSnprintf( _clientName, PMD_CLIENTNAME_LEN, "%s:%u-Extern",
                      _peerIP, _peerPort ) ;
      }
      else
      {
         ossSnprintf( _clientName, PMD_CLIENTNAME_LEN, "%s@%s:%u-Extern",
                      _username.c_str(), _peerIP, _peerPort ) ;
      }
   }

   INT32 _pmdExternClient::_processAuthRequestObj( const BSONObj &reqObj,
                                                   INT32 opCode,
                                                   const CHAR **userName,
                                                   const CHAR **password )
   {
      INT32 rc = SDB_OK ;

      try
      {

      *userName = reqObj.getStringField( SDB_AUTH_USER ) ;

      if ( MSG_AUTH_VERIFY_REQ == opCode )
      {
         *password = reqObj.getStringField( SDB_AUTH_PASSWD ) ;
      }
      else if ( MSG_AUTH_VERIFY1_REQ == opCode )
      {
         *password = NULL ;

         INT32 step = reqObj.getIntField( SDB_AUTH_STEP ) ;
         if ( SDB_AUTH_STEP_2 == step )
         {
            if ( _step1Done )
            {
               _step1Done = FALSE ;

               const CHAR* nonce = reqObj.getStringField( SDB_AUTH_NONCE ) ;

               if ( 0 != ossStrcmp( nonce, _combineNonce.c_str() ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Combine nonce[%s] of auth step2 msg is "
                          "different from step1, rc: %d", nonce, rc ) ;
                  goto error ;
               }
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Authenticate step2 should be executed after "
                       "step1, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG ( PDERROR, "An exception occurred when processing auth "
                  "request obj: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdExternClient::_processAuthResponse( INT32 opCode )
   {
      INT32 rc = SDB_OK ;

      try
      {
         // When "auth" is false in catalog node configure file sdb.conf,
         // or no users in catalog, it returned empty object. And we don't
         // need to authenticate or do privilege checking.
         if ( _authReturnedObj.isEmpty() )
         {
            _isAuthed = TRUE ;
            _privCheckEnabled = FALSE ;
            goto done ;
         }

         if ( MSG_AUTH_VERIFY_REQ == opCode )
         {
            _isAuthed = TRUE ;
         }
         else if ( MSG_AUTH_VERIFY1_REQ == opCode )
         {
            INT32 step = 0 ;

            step = _authReturnedObj.getIntField( SDB_AUTH_STEP ) ;
            if ( SDB_AUTH_STEP_1 == step )
            {
               _step1Done = TRUE ;

               _combineNonce =
                  _authReturnedObj.getStringField( SDB_AUTH_NONCE ) ;

               if ( 0 == _combineNonce.length() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG ( PDERROR, "Combine nonce can't be empty, rc: %d",
                           rc ) ;
                  goto error ;
               }
            }
            else if ( SDB_AUTH_STEP_2 == step )
            {
               _isAuthed = TRUE ;

               string hashCode ;
               INT32 hashLen = 0 ;
               INT32 nonceLen = 0 ;
               INT32 xorNum = 0 ;
               CHAR xorRes[ UTIL_AUTH_MD5SUM_LEN + 1 ] = { 0 } ;

               // get hash code
               BSONElement ele =
                  _authReturnedObj.getField( SDB_AUTH_HASHCODE ) ;
               PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                         "Invalid field[%s] type[%d] in obj[%s], rc: %d",
                         SDB_AUTH_HASHCODE, ele.type(),
                         _authReturnedObj.toString().c_str(), rc ) ;

               hashCode = base64::decode( ele.valuestr() ) ;
               hashLen = hashCode.length() ;
               PD_CHECK( UTIL_AUTH_MD5SUM_LEN == hashLen,
                         SDB_SYS, error, PDERROR,
                         "Invalid field[%s] length[%d], expect: %d",
                         SDB_AUTH_HASHCODE, hashLen, UTIL_AUTH_MD5SUM_LEN ) ;

               // get md5sum of password
               nonceLen = _combineNonce.length() ;
               xorNum = hashLen > nonceLen ? hashLen : nonceLen ;
               ossMemcpy( xorRes, hashCode.c_str(), UTIL_AUTH_MD5SUM_LEN ) ;

               for ( INT32 i = xorNum - 1 ; i >= 0 ; i-- )
               {
                  xorRes[ i % hashLen ] =
                     xorRes[ i % hashLen ] ^ _combineNonce[ i % nonceLen ] ;
               }
               _password = xorRes ;

               // don't return HashCode to client
               _authReturnedObj = _authReturnedObj.filterFieldsUndotted(
                                          BSON( SDB_AUTH_HASHCODE << 1 ),
                                          false ) ;
            }
         }

         if ( _isAuthed )
         {
            rc = _parseUserRole( _authReturnedObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Parse user role by auth response "
                         "failed, rc: %d", rc ) ;
         }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 authenticate( MsgHeader *pMsg, const CHAR **authBuf )
       \brief Authentication.
       \param [in] pMsg The message sent by client.
       \param [out] authBuf The bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _pmdExternClient::authenticate( MsgHeader *pMsg,
                                         const CHAR **authBuf )
   {
#if defined ( SDB_ENGINE )
      INT32 rc = SDB_OK ;
      BSONObj authObj ;
      const CHAR *userName = NULL ;
      const CHAR *password = NULL ;

      rc = extractAuthMsg( pMsg, authObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Client[%s] extract auth msg failed, rc: %d",
                 clientName(), rc ) ;
         goto error ;
      }
      rc = _processAuthRequestObj( authObj, pMsg->opCode,
                                   &userName, &password ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Client[%s] process auth request obj failed, rc: %d",
                 clientName(), rc ) ;
         goto error ;
      }

      _isAuthed = FALSE ;
      _privCheckEnabled = TRUE ;

      if ( SDB_ROLE_STANDALONE == pmdGetDBRole() ) // not auth
      {
         _privCheckEnabled = FALSE ;
         _isAuthed = TRUE ;
         goto done ;
      }
      else if ( SDB_ROLE_OM == pmdGetDBRole() )
      {
         if ( MSG_AUTH_VERIFY_REQ == pMsg->opCode )
         {
            rc = sdbGetOMManager()->md5Authenticate( authObj, _pEDUCB,
                                                     _authReturnedObj ) ;
         }
         else if ( MSG_AUTH_VERIFY1_REQ == pMsg->opCode )
         {
            rc = sdbGetOMManager()->SCRAMSHAAuthenticate( authObj, _pEDUCB,
                                                          _authReturnedObj ) ;
         }
         if ( rc )
         {
            PD_LOG( PDERROR, "Client[%s] authenticate failed[user: %s], "
                    "rc: %d", clientName(), userName, rc ) ;
            goto error ;
         }
         rc = _processAuthResponse( pMsg->opCode ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else if ( SDB_ROLE_COORD == pmdGetDBRole() )
      {
         INT64 contextID = -1 ;
         rtnContextBuf buf ;

         coordResource *pResource = sdbGetResourceContainer()->getResource() ;

         coordAuthOperator opr ;
         rc = opr.init( pResource, _pEDUCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Client[%s]: Init operator[%s] failed, rc: %d",
                    clientName(), opr.getName(), rc ) ;
            goto error ;
         }

         rc = opr.execute( pMsg, _pEDUCB, contextID, &buf ) ;

         if ( SDB_OK == rc && ( MSG_AUTH_VERIFY1_REQ == pMsg->opCode ||
                                MSG_AUTH_VERIFY_REQ == pMsg->opCode ) )
         {
            rc = buf.nextObj( _authReturnedObj ) ;
            if ( SDB_DMS_EOC == rc )
            {
               _authReturnedObj = BSONObj() ;
               rc = SDB_OK ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to read bson obj from the reply, "
                       "rc: %d", rc ) ;
               goto error ;
            }
            _authReturnedObj = _authReturnedObj.getOwned() ;
         }

         // special handling for password verification when there is no
         // addrlist specified. Usually this happen when there is only
         // one coord node before creating the first catalog
         if ( ( MSG_AUTH_VERIFY_REQ == pMsg->opCode ||
                MSG_AUTH_VERIFY1_REQ == pMsg->opCode ) &&
              SDB_CAT_NO_ADDR_LIST == rc )
         {
            rc = SDB_OK ;
            _isAuthed = TRUE ;
            _privCheckEnabled = FALSE ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Client[%s] authenticate failed[user: %s], "
                    "rc: %d", clientName(), userName, rc ) ;
            goto error ;
         }
         else
         {
            rc = _processAuthResponse( pMsg->opCode ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      else
      {
         MsgHeader *pAuthRes = NULL ;
         shardCB *pShard = sdbGetShardCB() ;
         UINT32 retryTimes = 0 ;
         MsgOpReply replyHeader ;
         replyHeader.contextID = -1 ;
         replyHeader.numReturned = 0 ;

         while ( TRUE )
         {
            ++retryTimes ;
            rc = pShard->syncSend( pMsg, CATALOG_GROUPID, TRUE, &pAuthRes ) ;
            if ( SDB_OK != rc )
            {
               rc = pShard->syncSend( pMsg, CATALOG_GROUPID, FALSE,
                                      &pAuthRes ) ;
               PD_RC_CHECK( rc, PDERROR, "Client[%s] failed to send auth "
                            "req to catalog, rc: %d", clientName(), rc ) ;
            }
            if ( NULL == pAuthRes )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Syncsend return ok but res is NULL" ) ;
               goto error ;
            }
            rc = MSG_GET_INNER_REPLY_RC(pAuthRes) ;
            replyHeader.flags = rc ;
            replyHeader.startFrom = MSG_GET_INNER_REPLY_STARTFROM(pAuthRes) ;
            ossMemcpy( &(replyHeader.header), pAuthRes, sizeof( MsgHeader ) ) ;

            if ( ( MSG_AUTH_VERIFY_REQ == pMsg->opCode || MSG_AUTH_VERIFY1_REQ == pMsg->opCode ) &&
                 SDB_OK == rc && pAuthRes->messageLength > (INT32)sizeof( MsgOpReply ) )
            {
               try
               {
                  const CHAR* data = (const CHAR*)pAuthRes + sizeof( MsgOpReply ) ;
                  _authReturnedObj = BSONObj( data ).getOwned() ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
                  /// release recv msg before goto error
                  SDB_OSS_FREE( pAuthRes ) ;
                  pAuthRes = NULL ;
                  goto error ;
               }
            }

            /// release recv msg
            SDB_OSS_FREE( pAuthRes ) ;
            pAuthRes = NULL ;

            if ( SDB_CLS_NOT_PRIMARY == rc &&
                 retryTimes < PMD_AUTH_RETRY_TIMES )
            {
               INT32 rcTmp = SDB_OK ;
               rcTmp = pShard->updatePrimaryByReply( &(replyHeader.header) ) ;
               if ( SDB_NET_CANNOT_CONNECT == rcTmp )
               {
                  /// the node is crashed, sleep some seconds
                  PD_LOG( PDWARNING, "Catalog group primary node is crashed "
                          "but other nodes not aware, sleep %d seconds",
                          NET_NODE_FAULTUP_MIN_TIME ) ;
                  ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
               }

               if ( rcTmp )
               {
                  pShard->updateCatGroup( CLS_SHARD_TIMEOUT ) ;
               }
               continue ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Client[%s] authenticate failed[user: %s], "
                       "rc: %d", clientName(), userName, rc ) ;
               goto error ;
            }
            else
            {
               rc = _processAuthResponse( pMsg->opCode ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
            break ;
         }
      }

   done:
      if ( SDB_OK == rc && authBuf )
      {
         *authBuf = _authReturnedObj.objdata() ;
      }
      if ( SDB_OK == rc && _isAuthed )
      {
         _username = userName ;
         if ( !_username.empty() && password )
         {
            _password = password ;
         }
         _pEDUCB->setUserInfo( _username, _password ) ;

         _makeName() ;

         CHAR szTmp[ 16 ] = { 0 } ;
         ossSnprintf( szTmp, sizeof(szTmp)-1, "%llu", _pEDUCB->getID() ) ;
         PD_AUDIT_OP( AUDIT_ACCESS, pMsg->opCode, AUDIT_OBJ_SESSION,
                      szTmp, SDB_OK,
                      "User[UserName:%s, RemoteIP:%s, RemotePort:%u, "
                      "LocalIP:%s, LocalPort:%u, AuthMechanism: %s] "
                      "login succeed",
                      getUsername(), getPeerIPAddr(), getPeerPort(),
                      getLocalIPAddr(), getLocalPort(),
                      (MSG_AUTH_VERIFY1_REQ == pMsg->opCode)?
                      (SDB_AUTH_MECHANISM_SS):(SDB_AUTH_MECHANISM_MD5) ) ;
      }
      return rc ;
   error:
      if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
      {
         _pEDUCB->printInfo( EDU_INFO_ERROR, "Username or passwd is wrong" ) ;
      }
      goto done ;
#else
   _isAuthed = TRUE ;
   return SDB_OK ;
#endif // SDB_ENGINE
   }

   INT32 _pmdExternClient::authenticate( const CHAR *username,
                                         const CHAR *password )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuffer = NULL ;
      INT32 buffSize = 0 ;

      rc = msgBuildAuthMsg( &pBuffer, &buffSize, username, password, 0 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build auth msg, rc: %d", rc ) ;
         goto error ;
      }
      rc = authenticate( (MsgHeader*)pBuffer ) ;
   done:
      if ( pBuffer )
      {
         SDB_OSS_FREE( pBuffer ) ;
         pBuffer = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _pmdExternClient::logout()
   {
      if ( _isAuthed )
      {
         CHAR szTmp[ 16 ] = { 0 } ;
         ossSnprintf( szTmp, sizeof(szTmp)-1, "%llu", _pEDUCB->getID() ) ;
         PD_AUDIT( AUDIT_ACCESS, getUsername(), getPeerIPAddr(),
                   getPeerPort(), "LOGOUT", AUDIT_OBJ_SESSION,
                   szTmp, SDB_OK,
                   "User[UserName:%s, RemoteIP:%s, RemotePort:%u, "
                   "LocalIP:%s, LocalPort:%u] logout succeed",
                   getUsername(), getPeerIPAddr(), getPeerPort(),
                   getLocalIPAddr(), getLocalPort() ) ;
      }
      _isAuthed = FALSE ;
      _privCheckEnabled = TRUE ;
   }

   INT32 _pmdExternClient::disconnect()
   {
      logout() ;
      if ( _pSocket && !_pSocket->isClosed() )
      {
         _pSocket->close() ;
      }
      return SDB_OK ;
   }

   BOOLEAN _pmdExternClient::isAuthed() const
   {
      return _isAuthed ;
   }

   void _pmdExternClient::setAuthed( BOOLEAN authed )
   {
      _isAuthed = authed ;
   }

   void _pmdExternClient::setAuthInfo( BOOLEAN privCheckEnabled, UINT32 roleID )
   {
      _privCheckEnabled = privCheckEnabled ;
      _roleID = roleID ;
   }

   BOOLEAN _pmdExternClient::isClosed() const
   {
      if ( _pSocket )
      {
         return _pSocket->isClosed() ;
      }
      return TRUE ;
   }

   BOOLEAN _pmdExternClient::isConnected() const
   {
      if ( _pSocket )
      {
         return _pSocket->isConnected() ;
      }
      return FALSE ;
   }

   UINT16 _pmdExternClient::getLocalPort() const
   {
      return _localPort ;
   }

   UINT16 _pmdExternClient::getPeerPort() const
   {
      return _peerPort ;
   }

   const CHAR* _pmdExternClient::getLocalIPAddr() const
   {
      return _localIP ;
   }

   const CHAR* _pmdExternClient::getPeerIPAddr() const
   {
      return _peerIP ;
   }

   const CHAR* _pmdExternClient::getUsername() const
   {
      return _username.c_str() ;
   }

   const CHAR* _pmdExternClient::getPassword() const
   {
      return _password.c_str() ;
   }

   const CHAR* _pmdExternClient::getFromIPAddr() const
   {
      return _fromIP ;
   }

   UINT16 _pmdExternClient::getFromPort() const
   {
      return getPeerPort() ;
   }

   INT32 _pmdExternClient::_parseUserRole( const bson::BSONObj &userInfo )
   {
      INT32 rc = SDB_OK ;

      // For compatibility reason with old version, if there is no field
      // "Option", or "Option.Role", the role is set to "admin".
      // In the future, if the user creates a role without any role, the value
      // of the role is "".
      using namespace oldRole;
      try
      {
         BSONElement optEle = userInfo.getField( FIELD_NAME_OPTIONS ) ;
         if ( optEle.eoo() )
         {
            _roleID = AUTH_NULL_ROLE_ID ;
         }
         else if ( Object != optEle.type() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Options in user information should be an object" ) ;
            goto error ;
         }
         else
         {
            BSONElement roleEle = optEle.Obj().getField( FIELD_NAME_ROLE ) ;
            if ( roleEle.eoo() )
            {
               _roleID = AUTH_NULL_ROLE_ID ;
            }
            else
            {
               const CHAR *roleName = roleEle.valuestrsafe() ;
               _roleID = authGetBuiltinRoleID( roleName ) ;
               if ( AUTH_INVALID_ROLE_ID == _roleID )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Invalid role name %s for the user",
                          roleName ) ;
                  goto error ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
