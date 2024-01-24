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

   Source File Name = authCB.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "authCB.hpp"
#include "authDef.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "authTrace.hpp"
#include "pmdCB.hpp"
#include "../bson/lib/md5.hpp"
#include "authRBAC.hpp"
#include "rtnQueryOptions.hpp"

using namespace bson ;

namespace engine
{

   const utilCSUniqueID SYS_AUTH_CSUID = UTIL_CSUNIQUEID_CAT_MIN + 5 ;
   const utilCLUniqueID SYS_AUTH_USER_CLUID =
               utilBuildCLUniqueID( SYS_AUTH_CSUID, UTIL_CSUNIQUEID_CAT_MIN + 1 ) ;
   const utilCLUniqueID SYS_AUTH_ROLE_CLUID =
               utilBuildCLUniqueID( SYS_AUTH_CSUID, UTIL_CSUNIQUEID_CAT_MIN + 2 ) ;

   _authCB::_authCB()
   :_authEnabled( TRUE )
   {
   }

   _authCB::~_authCB()
   {
   }

   INT32 _authCB::init ()
   {
      INT32 rc = SDB_OK ;
      _authEnabled = pmdGetOptionCB()->authEnabled() ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      rc = _initAuthentication( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init authentication, rc: %d", rc ) ;
         goto error ;
      }

      rc = _roleMgr.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init role manager, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _authCB::active ()
   {
      return SDB_OK ;
   }

   INT32 _authCB::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _authCB::fini ()
   {
      return SDB_OK ;
   }

   void _authCB::onConfigChange()
   {
      _authEnabled = pmdGetOptionCB()->authEnabled() ;
   }

   INT32 _authCB::_buildSecureUserInfo( BSONObjBuilder &builder,
                                        const BSONObj &origUserInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjIterator itr( origUserInfo ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 == ossStrcmp( e.fieldName(), SDB_AUTH_PASSWD ) ||
                 0 == ossStrcmp( e.fieldName(), SDB_AUTH_SCRAMSHA256 ) ||
                 0 == ossStrcmp( e.fieldName(), SDB_AUTH_SCRAMSHA1 ) )
            {
               continue ;
            }
            builder.append( e ) ;
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

   /** \fn INT32 _step1( const BSONObj &obj,
                         _pmdEDUCB *cb,
                         BSONObj *pOutUserObj )
       \brief The authentication using SCRAM-SHA256 is divided into two
              certifications in total. This is the first certification.
              In the first certification, the server needs to return
              the SCRAM-SHA256 info corresponding to the user name sent by
              the client.
       \param [in] obj Bson data in the message sent by the client.
       \param [in] cb  Pmd EDU cb.
       \param [out] pOutUserObj Bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__STEP1, "_authCB::_step1" )
   INT32 _authCB::_step1( const BSONObj &obj, _pmdEDUCB *cb,
                          BSONObj *pOutUserObj )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__STEP1 ) ;

      INT32 rc = SDB_OK ;
      INT32 type = 0 ;
      const CHAR* username = NULL ;
      const CHAR* clientNonce = NULL ;
      const CHAR* scramFiledName = NULL ;
      BSONObj userObj, scramObj ;
      string serverNonce, combineNonce ;
      BYTE serverNonceByte[ UTIL_AUTH_SCRAMSHA_NONCE_LEN ] = { 0 } ;

      try
      {

      // parse message
      rc = _parseStep1MsgObj( obj, &username, &clientNonce, type ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Faild to parse step1 message, rc: %d",
                   rc ) ;

      // upgrade
      rc = _upgradeUserInfo( username, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      // get SCRAM-SHA obj from system collection, and check compatiable.
      // If user exists before upgrade, then this user only support
      // SDB_AUTH_TYPE_MD5_PWD
      rc = getUsrInfo( username, cb, userObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get user info, rc: %d",
                   rc ) ;

      if ( SDB_AUTH_TYPE_EXTEND_PWD == type )
      {
         scramFiledName = SDB_AUTH_SCRAMSHA1 ;
         rc = rtnGetObjElement( userObj, scramFiledName, scramObj ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_AUTH_INCOMPATIBLE ;
            PD_LOG( PDERROR, "Authentication incompatible, "
                    "user[%s] only support SCRAM-SHA-256(MD5)",
                    username, rc ) ;
            goto error ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from user info[%s], rc: %d",
                         scramFiledName, userObj.toString().c_str(), rc ) ;
         }
      }
      else
      {
         scramFiledName = SDB_AUTH_SCRAMSHA256 ;
         rc = rtnGetObjElement( userObj, scramFiledName, scramObj ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from user info[%s], rc: %d",
                      scramFiledName, userObj.toString().c_str(), rc ) ;
      }

      if ( SDB_AUTH_TYPE_TEXT_PWD == type )
      {
         if ( !scramObj.hasField( SDB_AUTH_STOREDKEYTEXT ) ||
              !scramObj.hasField( SDB_AUTH_SERVERKEYTEXT ) )
         {
            rc = SDB_AUTH_INCOMPATIBLE ;
            PD_LOG( PDERROR, "Authentication incompatible, "
                    "user[%s] only support SCRAM-SHA-256(MD5)",
                    username, rc ) ;
            goto error ;
         }
      }

      // generate nonce
      rc = utilAuthGenerateNonce( serverNonceByte,
                                  UTIL_AUTH_SCRAMSHA_NONCE_LEN ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to generate nonce, rc: %d",
                   rc ) ;
      serverNonce = base64::encode( (CHAR*)serverNonceByte,
                                     UTIL_AUTH_SCRAMSHA_NONCE_LEN ) ;

      combineNonce = clientNonce ;
      combineNonce += serverNonce ;

      if ( pOutUserObj )
      {
         *pOutUserObj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_1 <<
                              SDB_AUTH_ITERATIONCOUNT <<
                              scramObj.getField( SDB_AUTH_ITERATIONCOUNT ) <<
                              SDB_AUTH_SALT <<
                              scramObj.getField( SDB_AUTH_SALT ) <<
                              SDB_AUTH_NONCE << combineNonce.c_str() ) ;
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__STEP1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _step2( const BSONObj &obj,
                         _pmdEDUCB *cb,
                         BSONObj *pOutUserObj )
       \brief The authentication using SCRAM-SHA256 is divided into two
              certifications in total. This is the second certification.
              In the second certification, the server needs to check whether
              the client's proof is legal. If it's legal, the server will return
              the server's proof to the client.
       \param [in] obj Bson data in the message sent by the client.
       \param [in] cb  Pmd EDU cb.
       \param [out] pOutUserObj Bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__STEP2, "_authCB::_step2" )
   INT32 _authCB::_step2( const BSONObj &obj, _pmdEDUCB *cb,
                          BSONObj *pOutUserObj )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__STEP2 ) ;

      INT32       rc            = SDB_OK ;
      INT32       type          = 0 ;
      UINT32      iterationCnt  = 0 ;
      const CHAR* username      = NULL ;
      const CHAR* identify      = NULL ;
      const CHAR* salt          = NULL ;
      const CHAR* storedKey     = NULL ;
      const CHAR* serverKey     = NULL ;
      const CHAR* clientProof   = NULL ;
      const CHAR* combineNonce  = NULL ;
      const CHAR* md5Passwd     = NULL ;
      BOOLEAN     isClientProofValid = FALSE ;
      BSONObj     userObj ;
      string      serverProof, hashCode ;

      // parse message
      rc = _parseStep2MsgObj( obj, &username, &identify, &clientProof,
                              &combineNonce, type ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Faild to parse step2 message, rc: %d",
                   rc ) ;

      // get iteration count, salt, storeKey, serverKey from system collection
      rc = getUsrInfo( username, cb, userObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Faild to get user info from system collection, rc: %d",
                   rc ) ;

      rc = _parseUserObj( userObj, type,
                          iterationCnt, &salt, &storedKey, &serverKey,
                          &md5Passwd ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Faild to parse user obj, rc: %d",
                   rc ) ;

      // verrify client proof
      if ( SDB_AUTH_TYPE_EXTEND_PWD == type )
      {
         rc = utilAuthVerifyClientProof1( clientProof,
                                          username, iterationCnt, salt,
                                          combineNonce, identify, storedKey,
                                          isClientProofValid ) ;
      }
      else
      {
         BOOLEAN fromSdb = SDB_AUTH_TYPE_MD5_PWD == type ? TRUE : FALSE ;
         rc = utilAuthVerifyClientProof( clientProof,
                                         username, iterationCnt, salt,
                                         combineNonce, identify,
                                         storedKey, fromSdb,
                                         isClientProofValid ) ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Faild to verify client proof, rc: %d",
                   rc ) ;

      if ( FALSE == isClientProofValid )
      {
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         PD_LOG( PDERROR, "Invalid client proof, rc: %d", rc ) ;
         goto error ;
      }

      // build object to return
      if ( pOutUserObj )
      {
         INT32 pwdLen = ossStrlen( md5Passwd ) ;
         INT32 nonceLen = ossStrlen( combineNonce ) ;
         INT32 xorNum = pwdLen > nonceLen ? pwdLen : nonceLen ;
         CHAR xorRes[ UTIL_AUTH_MD5SUM_LEN ] = { 0 } ;

         PD_CHECK( UTIL_AUTH_MD5SUM_LEN == pwdLen,
                   SDB_SYS, error, PDERROR,
                   "Invalid password length[%d], expect: %d",
                   pwdLen, UTIL_AUTH_MD5SUM_LEN ) ;

         ossMemcpy( xorRes, md5Passwd, UTIL_AUTH_MD5SUM_LEN ) ;

         for ( INT32 i = 0 ; i < xorNum ; i++ )
         {
            xorRes[ i % pwdLen ] =
               xorRes[ i % pwdLen ] ^ combineNonce[ i % nonceLen ] ;
         }

         hashCode = base64::encode( xorRes, sizeof(xorRes) ) ;

         // build server proof
         if ( SDB_AUTH_TYPE_EXTEND_PWD == type  )
         {
            rc = utilAuthCaculateServerProof1( username, iterationCnt, salt,
                                               combineNonce, identify, serverKey,
                                               serverProof ) ;
         }
         else
         {
            BOOLEAN fromSdb = SDB_AUTH_TYPE_MD5_PWD == type ? TRUE : FALSE ;
            rc = utilAuthCaculateServerProof( username, iterationCnt, salt,
                                              combineNonce, identify,
                                              serverKey, fromSdb,
                                              serverProof ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Faild to caculate server proof, rc: %d",
                      rc ) ;

         rc = _buildSCRAMSHAAuthResult( userObj, serverProof.c_str(),
                                        hashCode.c_str(), *pOutUserObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build auth result, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__STEP2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _authCB::_parseUserObj( const BSONObj &obj, INT32 type,
                                 UINT32 &iterationCnt,
                                 const CHAR **salt,
                                 const CHAR **storedKey,
                                 const CHAR **serverKey,
                                 const CHAR **md5Passwd )
   {
      INT32 rc = SDB_OK ;
      const CHAR* scramFieldName     = NULL ;
      const CHAR* storedKeyFieldName = NULL ;
      const CHAR* serverKeyFieldName = NULL ;
      BOOLEAN hasItCnt     = FALSE ;
      BOOLEAN hasSalt      = FALSE ;
      BOOLEAN hasStoredKey = FALSE ;
      BOOLEAN hasSvrKey    = FALSE ;
      BSONObj scramObj ;

      if ( SDB_AUTH_TYPE_MD5_PWD == type )
      {
         scramFieldName = SDB_AUTH_SCRAMSHA256 ;
         storedKeyFieldName = SDB_AUTH_STOREDKEY ;
         serverKeyFieldName = SDB_AUTH_SERVERKEY ;
      }
      else if ( SDB_AUTH_TYPE_TEXT_PWD == type )
      {
         scramFieldName = SDB_AUTH_SCRAMSHA256 ;
         storedKeyFieldName = SDB_AUTH_STOREDKEYTEXT ;
         serverKeyFieldName = SDB_AUTH_SERVERKEYTEXT ;
      }
      else if ( SDB_AUTH_TYPE_EXTEND_PWD == type )
      {
         scramFieldName = SDB_AUTH_SCRAMSHA1 ;
         storedKeyFieldName = SDB_AUTH_STOREDKEYEXTEND ;
         serverKeyFieldName = SDB_AUTH_SERVERKEYEXTEND ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {

      BSONObjIterator itr ;

      rc = rtnGetObjElement( obj, scramFieldName, scramObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   scramFieldName, obj.toString().c_str(), rc ) ;

      itr = BSONObjIterator( scramObj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_ITERATIONCOUNT ) )
         {
            if ( ele.type() != NumberInt )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in obj[%s], rc: %d",
                       ele.type(), SDB_AUTH_ITERATIONCOUNT,
                       scramObj.toString().c_str(), rc ) ;
               goto error ;
            }
            iterationCnt = (UINT32)ele.numberInt() ;
            hasItCnt = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_SALT ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in obj[%s], rc: %d",
                       ele.type(), SDB_AUTH_SALT,
                       scramObj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( salt )
            {
               *salt = ele.valuestr() ;
            }
            hasSalt = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), storedKeyFieldName ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in obj[%s], rc: %d",
                       ele.type(), storedKeyFieldName,
                       scramObj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( storedKey )
            {
               *storedKey = ele.valuestr() ;
            }
            hasStoredKey = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), serverKeyFieldName ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in obj[%s], rc: %d",
                       ele.type(), serverKeyFieldName,
                       scramObj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( serverKey )
            {
               *serverKey = ele.valuestr() ;
            }
            hasSvrKey = TRUE ;
         }
      }

      if ( !hasItCnt )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in obj[%s], rc: %d",
                 SDB_AUTH_ITERATIONCOUNT, scramObj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasSalt )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in obj[%s], rc: %d",
                 SDB_AUTH_SALT, scramObj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasStoredKey )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in obj[%s], rc: %d",
                 storedKeyFieldName, scramObj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasSvrKey )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in obj[%s], rc: %d",
                 serverKeyFieldName, scramObj.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( md5Passwd )
      {
         rc = rtnGetStringElement( obj, SDB_AUTH_PASSWD, md5Passwd ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s], rc: %d",
                      SDB_AUTH_PASSWD, obj.toString().c_str(), rc ) ;
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

   /** \fn INT32 SCRAMSHAAuthenticate( const BSONObj &obj,
                                       _pmdEDUCB *cb,
                                       BSONObj *pOutUserObj )
       \brief Authentication using SCRAM-SHA256.
       \param [in] obj Bson data in the message sent by the client.
       \param [in] cb  Pmd EDU cb.
       \param [out] pOutUserObj Bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_SCRAMSHAAUTHENTICATE, "_authCB::SCRAMSHAAuthenticate" )
   INT32 _authCB::SCRAMSHAAuthenticate( const BSONObj &obj,
                                        _pmdEDUCB *cb,
                                        BSONObj *pOutUserObj )
   {
      INT32 rc = SDB_OK ;
      INT32 step = 0 ;
      BOOLEAN need = TRUE ;

      PD_TRACE_ENTRY ( SDB_AUTHCB_SCRAMSHAAUTHENTICATE ) ;

      rc = needAuthenticate( cb, need ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if need to authenticate, rc: %d",
                 rc ) ;
         goto error ;
      }
      else if ( !need )
      {
         goto done ;
      }

      try
      {
         rc = rtnGetIntElement( obj, SDB_AUTH_STEP, step ) ;
         if ( rc )
         {
            PD_LOG( PDERROR,
                    "Failed to get field[%s] from SCRAM-SHA auth msg[%s], "
                    "rc: %d", SDB_AUTH_STEP, obj.toString().c_str(), rc ) ;
            goto error ;
         }

         if ( SDB_AUTH_STEP_1 == step )
         {
            rc = _step1( obj, cb, pOutUserObj ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else if ( SDB_AUTH_STEP_2 == step )
         {
            rc = _step2( obj, cb, pOutUserObj ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB_SCRAMSHAAUTHENTICATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 md5Authenticate( const BSONObj &obj,
                                  _pmdEDUCB *cb,
                                  BSONObj *pOutUserObj )
       \brief Authentication using MD5.
       \param [in] obj Bson data in the message sent by the client.
       \param [in] cb  Pmd EDU cb.
       \param [out] pOutUserObj Bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_MD5AUTHENTICATE, "_authCB::md5Authenticate" )
   INT32 _authCB::md5Authenticate( const BSONObj &obj,
                                   _pmdEDUCB *cb,
                                   BSONObj *pOutUserObj )
   {
      INT32     rc        = SDB_OK ;
      BOOLEAN   need      = TRUE ;
      SINT64    contextID = -1 ;
      SDB_DMSCB *dmsCB    = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB    = pmdGetKRCB()->getRTNCB() ;
      BSONObj dummyObj ;
      BSONObj hint ;
      rtnContextBuf buffObj ;

      PD_TRACE_ENTRY ( SDB_AUTHCB_MD5AUTHENTICATE ) ;

      rc = needAuthenticate( cb, need ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if need to authenticate, rc: %d",
                 rc ) ;
         goto error ;
      }
      else if ( !need )
      {
         goto done ;
      }

      rc = _parseMD5AuthMsgObj( obj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse md5 auth msg, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         hint = BSON( "" << AUTH_USR_INDEX_NAME ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = rtnQuery( AUTH_USR_COLLECTION,
                     dummyObj, obj, dummyObj,
                     hint, 0, cb, 0, -1, dmsCB,
                     rtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query, rc: %d",rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, rtnCB ) ;
      if ( SDB_OK != rc && SDB_DMS_EOC != rc)
      {
         PD_LOG( PDERROR, "Failed to getmore, rc: %d",rc ) ;
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }
      else if ( 0 == buffObj.recordNum() )
      {
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }
      else if ( 1 == buffObj.recordNum() )
      {
         if ( pOutUserObj )
         {
            try
            {
               BSONObj tmpObj ;
               buffObj.nextObj( tmpObj ) ;
               BSONObjBuilder builder( tmpObj.objsize() ) ;
               rc = _buildSecureUserInfo( builder, tmpObj ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build secure user "
                            "information: %d", rc ) ;
               *pOutUserObj = builder.obj() ;
            }
            catch ( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
               goto error ;
            }
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "It's impossible to get more than one record, rc: %d",
                 rc ) ;
         SDB_ASSERT( FALSE, "impossible" ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_AUTHCB_MD5AUTHENTICATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_GETUSRINFO, "_authCB::getUsrInfo" )
   INT32 _authCB::getUsrInfo( const CHAR *username, _pmdEDUCB *cb,
                              BSONObj &info )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextBuf buffObj ;
      BSONObj dummyObj ;
      BSONObj match ;
      BSONObj hint ;

      PD_TRACE_ENTRY ( SDB_AUTHCB_GETUSRINFO ) ;

      if ( NULL == username )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         match = BSON( SDB_AUTH_USER << username ) ;
         hint  = BSON( "" << AUTH_USR_INDEX_NAME ) ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = rtnQuery( AUTH_USR_COLLECTION, dummyObj, match, dummyObj, hint,
                     0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query, rc: %d",rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, rtnCB ) ;
      if ( rc && SDB_DMS_EOC != rc)
      {
         PD_LOG( PDERROR, "Failed to getmore, rc: %d",rc ) ;
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }
      else if ( 0 == buffObj.recordNum() )
      {
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }
      else if ( 1 == buffObj.recordNum() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "It's impossible to get more than one record, rc: %d",
                 rc ) ;
         SDB_ASSERT( FALSE, "impossible" ) ;
         goto error ;
      }

      try
      {
         info = BSONObj( buffObj.data() ).getOwned() ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_AUTHCB_GETUSRINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _authCB::updatePasswd( const string &user, const string &oldPasswd,
                                const string &newPasswd, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      utilUpdateResult result ;
      BSONObj updator, condition, dummyObj, hint ;
      BSONObj userObj, scramObj ;
      BSONElement ele ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      BOOLEAN foundSalt = FALSE ;
      BYTE salt[UTIL_AUTH_SCRAMSHA256_SALT_LEN] = { 0 } ;
      string saltBase64, storedKey, serverKey, clientKey ;
      BSONObjBuilder bob ;

      try
      {

      // find salt from system collection, if find nothing, generate salt
      rc = getUsrInfo( user.c_str(), cb, userObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get user[%s] info, rc: %d",
                   user.c_str(), rc ) ;

      ele = userObj.getField( SDB_AUTH_SCRAMSHA256 ) ;
      if ( Object == ele.type() )
      {
         scramObj = ele.Obj() ;
         ele = userObj.getField( SDB_AUTH_SALT ) ;
         if ( String == ele.type() )
         {
            foundSalt = TRUE ;
            saltBase64 = ele.valuestr() ;
            string saltDecode = base64::decode( saltBase64 ) ;
            ossMemcpy( salt, saltDecode.c_str(), saltDecode.length() ) ;
         }
      }
      if ( !foundSalt )
      {
         rc = utilAuthGenerateNonce( salt, UTIL_AUTH_SCRAMSHA256_SALT_LEN ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to generate nonce, rc: %d",
                      rc ) ;
         saltBase64 = base64::encode( (CHAR*)salt,
                                      UTIL_AUTH_SCRAMSHA256_SALT_LEN ) ;
      }

      // generate storeKey, serverKey
      rc = utilAuthCaculateKey( newPasswd.c_str(),
                                salt, UTIL_AUTH_SCRAMSHA256_SALT_LEN,
                                UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT,
                                storedKey,
                                serverKey,
                                clientKey ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to caculate key, rc: %d",
                   rc ) ;

      // build scram-sha-256
      bob.append( SDB_AUTH_ITERATIONCOUNT,
                  UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT ) ;
      bob.append( SDB_AUTH_SALT,      saltBase64.c_str() ) ;
      bob.append( SDB_AUTH_STOREDKEY, storedKey.c_str() ) ;
      bob.append( SDB_AUTH_SERVERKEY, serverKey.c_str() ) ;

      // build updator, condition
      updator = BSON( "$set" << BSON( SDB_AUTH_PASSWD << newPasswd <<
                                      SDB_AUTH_SCRAMSHA256 << bob.obj() ) ) ;
      condition = BSON( SDB_AUTH_USER << user <<
                        SDB_AUTH_PASSWD << oldPasswd ) ;
      hint = BSON( "" << AUTH_USR_INDEX_NAME ) ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = rtnUpdate( AUTH_USR_COLLECTION, condition, updator, hint,
                      0, cb, dmsCB, dpsCB, 1, &result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update user[%s]'s password in %s, "
                 "rc: %d", user.c_str(), AUTH_USR_COLLECTION, rc ) ;
         goto error ;
      }
      else if ( result.updateNum() <= 0 )
      {
         PD_LOG( PDERROR, "User name[%s] or password[%s] is error, rc: %d",
                 user.c_str(), oldPasswd.c_str(), rc ) ;
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_REMOVEUSR, "_authCB::removeUsr" )
   INT32 _authCB::removeUsr( const BSONObj &obj, _pmdEDUCB *cb, INT32 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_AUTHCB_REMOVEUSR ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      SINT64 contextID = -1 ;
      BSONObj dummyObj, match, hint ;
      const CHAR *username = NULL ;
      const CHAR *password = NULL ;
      const CHAR *nonce = NULL ;
      const CHAR *identify = NULL ;
      const CHAR *clientProof = NULL ;
      INT32 type = 0 ;
      rtnContextBuf buffObj ;

      rc = _parseDelUserMsgObj( obj, &username, &password,
                                &nonce, &identify, &clientProof, type ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse remove user msg, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         hint = BSON( "" << AUTH_USR_INDEX_NAME ) ;
         match = BSON( SDB_AUTH_USER << username ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      // firstly, we check the user is exist or not
      rc = rtnQuery( AUTH_USR_COLLECTION, dummyObj, match, dummyObj,
                     hint, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query, rc: %d",rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, rtnCB ) ;
      if ( SDB_OK != rc && SDB_DMS_EOC != rc)
      {
         PD_LOG( PDERROR, "Failed to getmore, rc: %d",rc ) ;
         rc = SDB_AUTH_USER_NOT_EXIST ;
         goto error ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_AUTH_USER_NOT_EXIST ;
         goto error ;
      }
      else if ( 0 == buffObj.recordNum() )
      {
         rc = SDB_AUTH_USER_NOT_EXIST ;
         goto error ;
      }
      else if ( 1 == buffObj.recordNum() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "It's impossible to get more than one record, rc: %d",
                 rc ) ;
         SDB_ASSERT( FALSE, "impossible" ) ;
         goto error ;
      }

      // then, we check user name and password is correct or not
      if ( password )
      {
         BSONObj obj = BSON( SDB_AUTH_USER << username <<
                             SDB_AUTH_PASSWD << password ) ;
         rc = md5Authenticate( obj, cb ) ;
      }
      else
      {
         BSONObj obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_2 <<
                             SDB_AUTH_USER << username <<
                             SDB_AUTH_NONCE << nonce <<
                             SDB_AUTH_IDENTIFY << identify <<
                             SDB_AUTH_TYPE << type <<
                             SDB_AUTH_PROOF << clientProof ) ;
         rc = SCRAMSHAAuthenticate( obj, cb ) ;
      }
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _checkRemoveUser( username, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Check removing user failed, rc: %d", rc ) ;

      // now we remove the record from system collection
      rc = rtnDelete( AUTH_USR_COLLECTION,
                      match, hint,
                      0, cb, dmsCB, dpsCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to remove usr[%s] from %s, rc: %d",
                 username, AUTH_USR_COLLECTION, rc ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_AUTHCB_REMOVEUSR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_INITAUTH, "_authCB::_initAuthentication" )
   INT32 _authCB::_initAuthentication( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_AUTHCB_INITAUTH ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnTestAndCreateCL( AUTH_USR_COLLECTION, cb, dmsCB, NULL,
                               SYS_AUTH_USER_CLUID, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      // create index
      {
         BSONObjBuilder builder ;
         builder.append( IXM_FIELD_NAME_KEY, BSON( SDB_AUTH_USER << 1) ) ;
         builder.append( IXM_FIELD_NAME_NAME, AUTH_USR_INDEX_NAME ) ;
         builder.appendBool( IXM_FIELD_NAME_UNIQUE, TRUE ) ;
         BSONObj indexDef = builder.obj() ;

         rc = rtnTestAndCreateIndex( AUTH_USR_COLLECTION, indexDef, cb, dmsCB,
                                     NULL, TRUE ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      rc = rtnTestAndCreateCL( AUTH_ROLE_COLLECTION, cb, dmsCB, NULL,
                               SYS_AUTH_ROLE_CLUID, TRUE );
      if ( rc )
      {
         goto error;
      }
      {
         BSONObjBuilder builder;
         builder.append( IXM_FIELD_NAME_KEY, BSON( AUTH_FIELD_NAME_ROLENAME << 1 ) );
         builder.append( IXM_FIELD_NAME_NAME, AUTH_ROLE_INDEX_NAME );
         builder.appendBool( IXM_FIELD_NAME_UNIQUE, TRUE );
         BSONObj indexDef = builder.done();

         rc = rtnTestAndCreateIndex( AUTH_ROLE_COLLECTION, indexDef, cb, dmsCB, NULL, TRUE );
         if ( SDB_OK != rc )
         {
            goto error;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB_INITAUTH, rc ) ;
      return rc ;
   error:
      PD_LOG( PDERROR, "Failed to init authentication, rc: %d", rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_NEEDAUTH, "_authCB::needAuthenticate" )
   INT32 _authCB::needAuthenticate( _pmdEDUCB *cb, BOOLEAN &need )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_AUTHCB_NEEDAUTH ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      const CHAR *collection = NULL ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      SINT64 totalCount = 0 ;

      if ( !_authEnabled )
      {
         need = FALSE ;
         goto done ;
      }

      rc = rtnResolveCollectionNameAndLock( AUTH_USR_COLLECTION,
                                            dmsCB, &su,
                                            &collection, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get %s, rc: %d",
                 AUTH_USR_COLLECTION, rc ) ;
         SDB_ASSERT( FALSE, "impossible" ) ;
         goto error ;
      }
      rc = su->countCollection ( collection, totalCount, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get count, rc: %d",rc ) ;
         SDB_ASSERT( FALSE, "impossible" ) ;
         goto error ;
      }
      PD_TRACE1 ( SDB_AUTHCB_NEEDAUTH, PD_PACK_INT ( totalCount ) ) ;
      need = ( 0 != totalCount ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_AUTHCB_NEEDAUTH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _buildUserInfo( const CHAR* username,
                                 const CHAR *passwdMd5,
                                 const CHAR* clearTextPasswd,
                                 const BSONObj &option,
                                 BSONObj &userInfo )
       \brief Generate user info. Such as:
              {
                 User: xxx,
                 Passwd: xxx,
                 Options: {}
                 SCRAM-SHA256:
                 {
                    "Salt": xxx,
                    "IterationCount": xxx,
                    "StoredKey": xxx,
                    "StoredKeyText": xxx,
                    "ServerKey": xxx,
                    "ServerKeyText": xxx
                 },
                 SCRAM-SHA1:
                 {
                    "Salt": xxx,
                    "IterationCount": xxx,
                    "StoredKeyExtend": xxx,
                    "ServerKeyExtend": xxx
                 }
              }
       \param [in] username The user name.
       \param [in] passwdMd5 The MD5 of the clear test password.
       \param [in] clearTextPasswd The clear text password.
       \param [in] option Option field which you can set audit mask.
       \param [out] userInfo The user info we will generate.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__BUILDUSERINFO, "_authCB::_buildUserInfo" )
   INT32 _authCB::_buildUserInfo( const CHAR *username,
                                  const CHAR *passwdMd5,
                                  const CHAR *clearTextPasswd,
                                  const BSONObj &option,
                                  BSONObj &userInfo )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__BUILDUSERINFO ) ;

      INT32 rc = SDB_OK ;
      const CHAR* extendPasswd = NULL ;
      string extendPwdStr ;
      string salt1Base64, salt256Base64 ;
      BYTE salt1[UTIL_AUTH_SCRAMSHA1_SALT_LEN] = { 0 } ;
      BYTE salt256[UTIL_AUTH_SCRAMSHA256_SALT_LEN] = { 0 } ;
      string serverKey,     clientKey,     storedKey ;
      string serverKeyText, clientKeyText, storedKeyText ;
      string serverKeyExt,  clientKeyExt,  storedKeyExt ;
      BSONObjBuilder userInfoBob, ss256Bob, ss1Bob ;

      SDB_ASSERT( username && passwdMd5,
                  "username or password cannot be null" ) ;

      try
      {

      // genreate extend password
      if ( clearTextPasswd )
      {
         string tmp = username ;
         tmp += ":mongo:" ;
         tmp += clearTextPasswd ;
         extendPwdStr = md5::md5simpledigest( tmp ) ;
         extendPasswd = extendPwdStr.c_str() ;
      }

      // generate salt
      if ( extendPasswd )
      {
         rc = utilAuthGenerateNonce( salt1, UTIL_AUTH_SCRAMSHA1_SALT_LEN ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to generate nonce for scram-sha-1, rc: %d",
                      rc ) ;
      }

      rc = utilAuthGenerateNonce( salt256, UTIL_AUTH_SCRAMSHA256_SALT_LEN ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to generate nonce for scram-sha-256, rc: %d",
                   rc ) ;

      if ( extendPasswd )
      {
         salt1Base64 = base64::encode( (CHAR*)salt1,
                                       UTIL_AUTH_SCRAMSHA1_SALT_LEN ) ;
      }
      salt256Base64 = base64::encode( (CHAR*)salt256,
                                      UTIL_AUTH_SCRAMSHA256_SALT_LEN ) ;

      // generate StoredKey, ServerKey and ClientKey
      rc = utilAuthCaculateKey( passwdMd5,
                                salt256, UTIL_AUTH_SCRAMSHA256_SALT_LEN,
                                UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT,
                                storedKey, serverKey, clientKey ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to caculate key by md5sum of password, rc: %d",
                   rc ) ;

      if ( clearTextPasswd )
      {
         rc = utilAuthCaculateKey( clearTextPasswd,
                                   salt256, UTIL_AUTH_SCRAMSHA256_SALT_LEN,
                                   UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT,
                                   storedKeyText, serverKeyText, clientKeyText ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to caculate key by clear text password, rc: %d",
                      rc ) ;
      }
      if ( extendPasswd )
      {
         rc = utilAuthCaculateKey1( extendPasswd,
                                    salt1, UTIL_AUTH_SCRAMSHA1_SALT_LEN,
                                    UTIL_AUTH_SCRAMSHA1_ITERATIONCOUNT,
                                    storedKeyExt, serverKeyExt, clientKeyExt ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to caculate key by extend password, rc: %d",
                      rc ) ;
      }

      // build "SCRAM-SHA256"
      ss256Bob.append( SDB_AUTH_ITERATIONCOUNT,
                       UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT ) ;
      ss256Bob.append( SDB_AUTH_SALT,          salt256Base64.c_str() ) ;
      ss256Bob.append( SDB_AUTH_STOREDKEY,     storedKey.c_str() ) ;
      ss256Bob.append( SDB_AUTH_SERVERKEY,     serverKey.c_str() ) ;
      if ( clearTextPasswd )
      {
         ss256Bob.append( SDB_AUTH_STOREDKEYTEXT, storedKeyText.c_str() ) ;
         ss256Bob.append( SDB_AUTH_SERVERKEYTEXT, serverKeyText.c_str() ) ;
      }

      // build "SCRAM-SHA1"
      if ( extendPasswd )
      {
         ss1Bob.append( SDB_AUTH_ITERATIONCOUNT,
                        UTIL_AUTH_SCRAMSHA1_ITERATIONCOUNT ) ;
         ss1Bob.append( SDB_AUTH_SALT,            salt1Base64.c_str() ) ;
         ss1Bob.append( SDB_AUTH_STOREDKEYEXTEND, storedKeyExt.c_str() ) ;
         ss1Bob.append( SDB_AUTH_SERVERKEYEXTEND, serverKeyExt.c_str() ) ;
      }

      // build user info object
      userInfoBob.append( SDB_AUTH_USER, username ) ;
      userInfoBob.append( SDB_AUTH_PASSWD, passwdMd5 ) ;
      {
         ossPoolSet< ossPoolString > roles;
         BSONObjBuilder optionsBuilder;
         for ( BSONObjIterator it( option ); it.more(); )
         {
            BSONElement ele = it.next() ;
            if ( 0 == ossStrcmp( FIELD_NAME_ROLES, ele.fieldName() ) )
            {
               BSONObj rolesArray = ele.Obj() ;
               for ( BSONObjIterator it( rolesArray ); it.more(); )
               {
                  roles.insert( it.next().poolString() ) ;
               }
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_ROLE, ele.fieldName() ) )
            {
               ossPoolString role = authGetBuiltinRoleFromOldRole( ele.valuestr() ) ;
               if ( !role.empty() )
               {
                  roles.insert( role ) ;
               }
            }
            else
            {
               optionsBuilder.append( ele ) ;
            }
         }
         BSONArrayBuilder rolesBuilder ;
         for ( ossPoolSet< ossPoolString >::iterator it = roles.begin(); it != roles.end(); ++it )
         {
            rolesBuilder.append( *it ) ;
         }
         userInfoBob.append( FIELD_NAME_ROLES, rolesBuilder.arr() );
         userInfoBob.append( FIELD_NAME_OPTIONS, optionsBuilder.obj() );
      }
      userInfoBob.append( SDB_AUTH_SCRAMSHA256, ss256Bob.obj() ) ;
      if ( extendPasswd )
      {
         userInfoBob.append( SDB_AUTH_SCRAMSHA1, ss1Bob.obj() ) ;
      }
      userInfo = userInfoBob.obj() ;

      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__BUILDUSERINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB_CREATEUSR, "_authCB::createUsr" )
   INT32 _authCB::createUsr( BSONObj &obj, _pmdEDUCB *cb,
                             BSONObj *pOutObj, INT32 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_AUTHCB_CREATEUSR ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      const CHAR *username = NULL ;
      const CHAR *passwd = NULL ;
      const CHAR *clearTextPasswd = NULL ;
      BSONObj option, userInfoObj, hint ;

      rc = _parseCrtUserMsgObj( obj, &username, &passwd, &clearTextPasswd,
                                option ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse create user msg, rc: %d",
                   rc ) ;

      rc = _buildUserInfo( username, passwd, clearTextPasswd, option,
                           userInfoObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to generate SCRAM-SHA user info, rc: %d",
                   rc ) ;

      try
      {
         hint = BSON( "" << AUTH_USR_INDEX_NAME ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = rtnInsert( AUTH_USR_COLLECTION,
                      userInfoObj, 1, 0, cb,
                      dmsCB, dpsCB, w ) ;
      if ( SDB_OK != rc && SDB_IXM_DUP_KEY != rc )
      {
         rtnDelete( AUTH_USR_COLLECTION, userInfoObj, hint, 0, cb,
                    dmsCB, dpsCB ) ;
         goto error ;
      }
      else if ( SDB_IXM_DUP_KEY == rc )
      {
         rc = SDB_AUTH_USER_ALREADY_EXIST ;
         goto error ;
      }
      else if ( pOutObj )
      {
         try
         {
            BSONObjBuilder builder( userInfoObj.objsize() ) ;
            rc = _buildSecureUserInfo( builder, userInfoObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build secure user "
                         "information: %d", rc ) ;
            *pOutObj = builder.obj() ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB_CREATEUSR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__VALIDOPTIONS, "_authCB::_validOptions" )
   INT32 _authCB::_validOptions( const BSONObj &option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_AUTHCB__VALIDOPTIONS ) ;

      BSONObjIterator itr( option ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;

         if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_AUDIT_MASK ) )
         {
            UINT32 mask = 0 ;
            if ( String != e.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Field[%s] is invalid in option[%s], rc: %d",
                           FIELD_NAME_AUDIT_MASK,
                           option.toString().c_str(), rc ) ;
               goto error ;
            }

            rc = pdString2AuditMask( e.valuestr(), mask, TRUE ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Field[%s] is invalid in option[%s], rc: %d",
                           FIELD_NAME_AUDIT_MASK,
                           option.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_ROLES ) )
         {
            if ( Array != e.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Field[%s] is invalid in option[%s], rc: %d", FIELD_NAME_ROLES,
                           option.toString().c_str(), rc ) ;
               goto error ;
            }
            for ( BSONObjIterator it( e.Obj() ); it.more(); )
            {
               BSONElement ele = it.next() ;
               if ( bson::String != ele.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Role in field[%s] must be string, rc: %d", FIELD_NAME_ROLES,
                              rc ) ;
                  goto error ;
               }
               if ( !_roleMgr.hasRole( ele.valuestr() ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", ele.valuestrsafe(), rc ) ;
                  goto error ;
               }
            }
         }
         else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_ROLE ) )
         {
            if ( String != e.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Field[%s] is invalid in option[%s], rc: %d", FIELD_NAME_ROLE,
                           option.toString().c_str(), rc ) ;
               goto error ;
            }
            else
            {
               if ( AUTH_INVALID_ROLE_ID == oldRole::authGetBuiltinRoleID( e.valuestrsafe() ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Role %s is invalid when creating a user, rc: %d",
                              e.valuestrsafe(), rc ) ;
                  goto error ;
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Field[%s] is invalid in option[%s], rc: %d",
                        e.fieldName(), option.toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__VALIDOPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _authCB::_upgradeUserInfo( const CHAR *username,
                                            _pmdEDUCB *cb )
       \brief Upgrade user info. Because of new auth mechanism, SCRAM-SHA256,
              we need to upgrade the user info in SYSAUTH.SYSUSRS. If the
              record only has MD5 info, such as
              {
                 "User": xxx,
                 "Passwd": xxx
              }
              Then we need to upgrade the record to
              {
                 "User": xxx,
                 "Passwd": xxx,
                 "SCRAM-SHA256":
                 {
                    "Salt": xxx,
                    "IterationCount": xxx,
                    "StoredKey": xxx,
                    "ServerKey": xxx,
                 }
              }
       \param [in] username Username.
       \param [in] cb Pmd EDU cb.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__UPGRADEUSERINFO, "_authCB::_upgradeUserInfo" )
   INT32 _authCB::_upgradeUserInfo( const CHAR *username, _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__UPGRADEUSERINFO ) ;

      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      _dpsLogWrapper *dpsCB = pmdGetKRCB()->getDPSCB() ;
      BYTE salt[UTIL_AUTH_SCRAMSHA256_SALT_LEN] = { 0 } ;
      const CHAR *passwdMD5  = NULL ;
      BSONObj matcher, dummyObj, updator, userObj ;
      string serverKey, clientKey, storedKey ;

      if ( NULL == username )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // check user info
      rc = getUsrInfo( username, cb, userObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get user[%s] info, rc: %d",
                   username, rc ) ;

      if ( userObj.hasField( SDB_AUTH_SCRAMSHA256 ) )
      {
         goto done ;
      }

      rc = rtnGetStringElement( userObj, SDB_AUTH_PASSWD, &passwdMD5 ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from user obj[%s], rc: %d",
                   SDB_AUTH_PASSWD, userObj.toString().c_str(), rc ) ;

      // generate salte, storedKey, serverKey for SCRAM-SHA256
      rc = utilAuthGenerateNonce( salt, UTIL_AUTH_SCRAMSHA256_SALT_LEN ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to generate nonce, rc: %d",
                   rc ) ;

      rc = utilAuthCaculateKey( passwdMD5,
                                salt, UTIL_AUTH_SCRAMSHA256_SALT_LEN,
                                UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT,
                                storedKey,
                                serverKey,
                                clientKey ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to caculate key, rc: %d",
                   rc ) ;

      // build new user object
      try
      {
         string saltBase64 = base64::encode( (CHAR*)salt, sizeof(salt) ) ;

         BSONObjBuilder bob ;
         bob.append( SDB_AUTH_ITERATIONCOUNT,
                     UTIL_AUTH_SCRAMSHA256_ITERATIONCOUNT ) ;
         bob.append( SDB_AUTH_SALT, saltBase64.c_str() ) ;
         bob.append( SDB_AUTH_STOREDKEY, storedKey.c_str() ) ;
         bob.append( SDB_AUTH_SERVERKEY, serverKey.c_str() ) ;
         updator = BSON( "$set" << BSON( SDB_AUTH_SCRAMSHA256 << bob.obj() ) ) ;

         matcher = BSON( SDB_AUTH_USER << username ) ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      rc = rtnUpdate( AUTH_USR_COLLECTION, matcher, updator,
                      dummyObj, 0, cb, dmsCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update, matcher[%s] updator[%s] rc: %d",
                   matcher.toString().c_str(), updator.toString().c_str(), rc ) ;

      PD_LOG( PDEVENT, "Upgrade user[%s] info successfully", username ) ;

   done :
      PD_TRACE_EXITRC ( SDB_AUTHCB__UPGRADEUSERINFO, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /** \fn INT32 _authCB::_parseMD5AuthMsgObj( const BSONObj &obj )
       \brief Parse the msg of authentication using MD5.
       \param [in] obj Bson data in the message sent by the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__PARSEMD5AUTHMSGOBJ, "_authCB::_parseMD5AuthMsgObj" )
   INT32 _authCB::_parseMD5AuthMsgObj( const BSONObj &obj )
   {
      INT32 rc             = SDB_OK ;
      const CHAR *username = NULL ;
      const CHAR *passwd   = NULL ;

      PD_TRACE_ENTRY ( SDB_AUTHCB__PARSEMD5AUTHMSGOBJ ) ;

      try
      {

      BSONObjIterator itr( obj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         // check username
         if ( ossStrcmp( ele.fieldName(), SDB_AUTH_USER ) == 0 )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_USER, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            username = ele.valuestr() ;
         }
         // check passwd
         else if ( ossStrcmp( ele.fieldName(), SDB_AUTH_PASSWD ) == 0 )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_PASSWD, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            passwd = ele.valuestr() ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Unknown field[%s] in md5 auth msg[%s], rc: %d",
                    ele.fieldName(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( NULL == username )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in md5 auth msg[%s], rc: %d",
                 SDB_AUTH_USER, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( NULL == passwd )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in md5 auth msg[%s], rc: %d",
                 SDB_AUTH_PASSWD, obj.toString().c_str(), rc ) ;
         goto error ;
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__PARSEMD5AUTHMSGOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _authCB::_parseCrtUserMsgObj( const BSONObj &obj,
                                               const CHAR **username,
                                               const CHAR **passwd,
                                               const CHAR **clearTextPasswd,
                                               BSONObj &option )
       \brief Parse the msg of createing user.
       \param [in] obj Bson data in the message sent by the client.
       \param [out] username User name.
       \param [out] passwd Md5sum of clear text password.
       \param [out] clearTextPasswd Clear text password.
       \param [out] option Option.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__PARSECRTUSERMSGOBJ, "_authCB::_parseCrtUserMsgObj" )
   INT32 _authCB::_parseCrtUserMsgObj( const BSONObj &obj,
                                       const CHAR **username,
                                       const CHAR **passwd,
                                       const CHAR **clearTextPasswd,
                                       BSONObj &option )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasUser = FALSE ;
      BOOLEAN hasPwd  = FALSE ;

      PD_TRACE_ENTRY ( SDB_AUTHCB__PARSECRTUSERMSGOBJ ) ;

      // we don't want to print 'obj' to diaglog, because 'obj' may has
      // clear text password

      try
      {

      BSONObjIterator itr( obj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         // check username
         if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_USER ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Field[%s] type[%d] is invalid, must be string, rc: %d",
                       SDB_AUTH_USER, ele.type(), rc ) ;
               goto error ;
            }
            if ( 0 == ossStrlen( ele.valuestr() ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Username can't be empty when create user, rc: %d",
                       rc ) ;
               goto error ;
            }
            if ( username )
            {
               *username = ele.valuestr() ;
            }
            hasUser = TRUE ;
         }
         // check passwd
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_PASSWD ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Field[%s] type[%d] is invalid, must be string, rc: %d",
                       SDB_AUTH_PASSWD, ele.type(), rc ) ;
               goto error ;
            }
            if ( 0 == ossStrlen( ele.valuestr() ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Md5sum of Password can't be empty in createUsr msg, "
                       "rc: %d", rc ) ;
               goto error ;
            }
            if ( passwd )
            {
               *passwd = ele.valuestr() ;
            }
            hasPwd = TRUE ;
         }
         // check cleartext passwd, it can be empty
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_TEXTPASSWD ) )
         {
            if ( ele.type() != String )
            {
               PD_LOG( PDERROR,
                       "Field[%s] type[%d] is invalid, must be string, rc: %d",
                       SDB_AUTH_TEXTPASSWD, ele.type(), rc ) ;
               goto error ;
            }
            if ( clearTextPasswd )
            {
               *clearTextPasswd = ele.valuestr() ;
            }
         }
         // check options
         else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_OPTIONS ) )
         {
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Field[%s] type[%d] is invalid, must be string, rc: %d",
                       FIELD_NAME_OPTIONS, ele.type(), rc ) ;
               goto error ;
            }
            option = ele.Obj() ;
            if ( SDB_OK != _validOptions( option ) )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unknown field[%s] in createUsr msg[%s], rc: %d",
                    ele.fieldName(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( !hasUser )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist when create user, rc: %d",
                 SDB_AUTH_USER, rc ) ;
         goto error ;
      }
      if ( !hasPwd )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist when create user, rc: %d",
                 SDB_AUTH_PASSWD, rc ) ;
         goto error ;
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__PARSECRTUSERMSGOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__PARSESTEP1MSGOBJ, "_authCB::_parseStep1MsgObj" )
   INT32 _authCB::_parseStep1MsgObj( const BSONObj &obj,
                                     const CHAR **username,
                                     const CHAR **clientNonce,
                                     INT32 &type )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__PARSESTEP1MSGOBJ ) ;

      INT32 rc = SDB_OK ;
      BOOLEAN hasUser = FALSE ;
      BOOLEAN hasNonce = FALSE ;
      BOOLEAN hasType = FALSE ;

      try
      {

      BSONObjIterator itr( obj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_STEP ) )
         {
            if ( !ele.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step1 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_STEP, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            // Field Step has beed parsed in _authCB::SCRAMSHAAuthenticate()
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_USER ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step1 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_USER, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( username )
            {
               *username = ele.valuestr() ;
            }
            hasUser = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_NONCE ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step1 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_NONCE, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( clientNonce )
            {
               *clientNonce = ele.valuestr() ;
            }
            hasNonce = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_TYPE ) )
         {
            if ( ele.type() != NumberInt )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step1 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_TYPE, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            type = ele.numberInt() ;
            hasType = TRUE ;
            if ( SDB_AUTH_TYPE_MD5_PWD    != type &&
                 SDB_AUTH_TYPE_TEXT_PWD   != type &&
                 SDB_AUTH_TYPE_EXTEND_PWD != type )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Filed[%s] value[%d] is invalid in step2 msg[%s], rc: %d",
                       SDB_AUTH_TYPE, type, obj.toString().c_str(), rc ) ;
               goto error ;
            }

         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Unknown field[%s] in auth step2 msg[%s], rc: %d",
                    ele.fieldName(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( !hasUser )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step1 msg[%s], rc: %d",
                 SDB_AUTH_USER, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasNonce )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step1 msg[%s], rc: %d",
                 SDB_AUTH_NONCE, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasType )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step1 msg[%s], rc: %d",
                 SDB_AUTH_TYPE, obj.toString().c_str(), rc ) ;
         goto error ;
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__PARSESTEP1MSGOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _authCB::_parseStep2MsgObj( const BSONObj &obj,
                                             const CHAR **username,
                                             const CHAR **identify,
                                             const CHAR **clientProof,
                                             const CHAR **combineNonce,
                                             INT32 &type )
       \brief Parse the msg in the second step of authentication using
              SCRAM-SHA256.
       \param [in] obj Bson data in the message sent by the client.
       \param [out] username User name.
       \param [out] identify Session identifier. When the client is C++ driver,
                    its value is "C++_Session". When the client is C driver, its
                    value is "C_Session".
       \param [out] clientProof Client proof in base64 format.
       \param [out] combineNonce Combine nonce in base64 format.
       \param [out] type Type of original password.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__PARSESTEP2MSGOBJ, "_authCB::_parseStep2MsgObj" )
   INT32 _authCB::_parseStep2MsgObj( const BSONObj &obj,
                                     const CHAR **username,
                                     const CHAR **identify,
                                     const CHAR **clientProof,
                                     const CHAR **combineNonce,
                                     INT32 &type )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__PARSESTEP2MSGOBJ ) ;

      INT32 rc = SDB_OK ;
      BOOLEAN hasUser = FALSE ;
      BOOLEAN hasIdentify = FALSE ;
      BOOLEAN hasProof = FALSE ;
      BOOLEAN hasNonce = FALSE ;
      BOOLEAN hasType = FALSE ;

      try
      {

      BSONObjIterator itr( obj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_STEP ) )
         {
            if ( !ele.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step2 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_STEP, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            // Field Step has beed parsed in _authCB::SCRAMSHAAuthenticate()
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_USER ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step2 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_USER, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( username )
            {
               *username = ele.valuestr() ;
            }
            hasUser = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_IDENTIFY ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step2 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_IDENTIFY, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( identify )
            {
               *identify = ele.valuestr() ;
            }
            hasIdentify = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_PROOF ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step2 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_PROOF, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( 0 == ossStrlen( ele.valuestr() ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Client proof can't be empty in auth step2 "
                       "msg[%s], rc: %d", obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( clientProof )
            {
               *clientProof = ele.valuestr() ;
            }
            hasProof = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_NONCE ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step2 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_NONCE, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            if ( combineNonce )
            {
               *combineNonce = ele.valuestr() ;
            }
            hasNonce = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_TYPE ) )
         {
            if ( ele.type() != NumberInt )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in step2 msg[%s], rc: %d",
                       ele.type(), SDB_AUTH_TYPE, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            type = ele.numberInt() ;
            hasType = TRUE ;
            if ( SDB_AUTH_TYPE_MD5_PWD    != type &&
                 SDB_AUTH_TYPE_TEXT_PWD   != type &&
                 SDB_AUTH_TYPE_EXTEND_PWD != type )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Filed[%s] value[%d] is invalid in step2 msg[%s], rc: %d",
                       SDB_AUTH_TYPE, type, obj.toString().c_str(), rc ) ;
               goto error ;
            }

         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Unknown field[%s] in auth step2 msg[%s], rc: %d",
                    ele.fieldName(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( !hasUser )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step2 msg[%s], rc: %d",
                 SDB_AUTH_USER, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasIdentify )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step2 msg[%s], rc: %d",
                 SDB_AUTH_IDENTIFY, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasProof )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step2 msg[%s], rc: %d",
                 SDB_AUTH_PROOF, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasNonce )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step2 msg[%s], rc: %d",
                 SDB_AUTH_NONCE, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( !hasType )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in auth step2 msg[%s], rc: %d",
                 SDB_AUTH_TYPE, obj.toString().c_str(), rc ) ;
         goto error ;
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__PARSESTEP2MSGOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _authCB::_parseDelUserMsgObj( const BSONObj &obj,
                                               const CHAR **username,
                                               const CHAR **passwd,
                                               const CHAR **nonce,
                                               const CHAR **identify,
                                               const CHAR **clientProof,
                                               INT32 &type )
       \brief Parse the msg of removing user.
       \param [in] obj Bson data in the message sent by the client.
       \param [out] username User name.
       \param [out] passwd User Password.
       \param [out] nonce Combine nonce in base64 format.
       \param [out] identify Session identifier. When the client is C++ driver,
                    its value is "C++_Session". When the client is C driver, its
                    value is "C_Session".
       \param [out] clientProof Client proof in base64 format.
       \param [out] type Type of original password.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__PARSEDELUSERMSGOBJ, "_authCB::_parseDelUserMsgObj" )
   INT32 _authCB::_parseDelUserMsgObj( const BSONObj &obj,
                                       const CHAR **username,
                                       const CHAR **passwd,
                                       const CHAR **nonce,
                                       const CHAR **identify,
                                       const CHAR **clientProof,
                                       INT32 &type )
   {
      PD_TRACE_ENTRY ( SDB_AUTHCB__PARSEDELUSERMSGOBJ ) ;

      INT32 rc = SDB_OK ;
      BOOLEAN hasUser = FALSE ;
      BOOLEAN hasPwd = FALSE ;
      BOOLEAN hasNonce = FALSE ;
      BOOLEAN hasIdentify = FALSE ;
      BOOLEAN hasType = FALSE ;
      BOOLEAN hasProof = FALSE ;

      /* There are 2 formats for deleting user:
       * 1. from sequoiadb driver
       *    { User: xxx, Passwd: xxx }
       * 2. from fap-mongo
       *    { User: xxx, Nonce: xxx, Identify: xxx, Proof: xx, Type: xxx }
       */
      try
      {

      BSONObjIterator itr( obj );
      while ( itr.more() )
      {
         BSONElement ele = itr.next();
         // check username
         if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_USER ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_USER, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            if ( username )
            {
               *username = ele.valuestr() ;
            }
            hasUser = TRUE ;
         }
         // check passwd
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_PASSWD ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_PASSWD, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            if ( passwd )
            {
               *passwd = ele.valuestr() ;
            }
            hasPwd = TRUE ;
         }
         // check identify
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_IDENTIFY ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_IDENTIFY, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            if ( identify )
            {
               *identify = ele.valuestr() ;
            }
            hasIdentify = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_PROOF ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_PROOF, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            if ( clientProof )
            {
               *clientProof = ele.valuestr() ;
            }
            hasProof = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_NONCE ) )
         {
            if ( ele.type() != String )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid field[%s] type[%d] in obj[%s], must be String",
                       SDB_AUTH_NONCE, ele.type(), obj.toString().c_str() ) ;
               goto error ;
            }
            if ( nonce )
            {
               *nonce = ele.valuestr() ;
            }
            hasNonce = TRUE ;
         }
         else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_TYPE ) )
         {
            if ( ele.type() != NumberInt )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Invalid type[%d] of field[%s] in obj[%s], rc: %d",
                       ele.type(), SDB_AUTH_TYPE, obj.toString().c_str(), rc ) ;
               goto error ;
            }
            type = ele.numberInt() ;
            hasType = TRUE ;
            if ( SDB_AUTH_TYPE_MD5_PWD    != type &&
                 SDB_AUTH_TYPE_TEXT_PWD   != type &&
                 SDB_AUTH_TYPE_EXTEND_PWD != type )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR,
                       "Filed[%s] value[%d] is invalid in obj[%s], rc: %d",
                       SDB_AUTH_TYPE, type, obj.toString().c_str(), rc ) ;
               goto error ;
            }

         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Unknown field[%s] in delUser msg[%s], rc: %d",
                    ele.fieldName(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      if ( !hasUser )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Field[%s] doesn't exist in delUser msg[%s], rc: %d",
                 SDB_AUTH_USER, obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( hasPwd )
      {
         if ( hasNonce || hasType || hasIdentify || hasProof )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Invalid delUser msg[%s], rc: %d",
                    obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( hasNonce && hasType && hasIdentify && hasProof )
         {
            // it is ok
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Invalid delUser msg[%s], rc: %d",
                    obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_AUTHCB__PARSEDELUSERMSGOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__BUILDSCRAMSHAAUTHRESULT, "_authCB::_buildSCRAMSHAAuthResult" )
   INT32 _authCB::_buildSCRAMSHAAuthResult( const BSONObj &userInfo,
                                            const CHAR *serverProof,
                                            const CHAR *hashCode,
                                            BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_AUTHCB__BUILDSCRAMSHAAUTHRESULT ) ;
      SDB_ASSERT( serverProof, "Server proof is NULL" ) ;
      SDB_ASSERT( hashCode, "Hash code is NULL" ) ;

      try
      {
         BSONObjBuilder builder( userInfo.objsize() + ossStrlen( serverProof )
                                 + ossStrlen( hashCode ) ) ;
         rc = _buildSecureUserInfo( builder, userInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build secure user information: "
                      "%d", rc ) ;

         builder.append( SDB_AUTH_STEP, SDB_AUTH_STEP_2 ) ;
         builder.append( SDB_AUTH_PROOF, serverProof ) ;
         builder.append( SDB_AUTH_HASHCODE, hashCode ) ;
         result = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTHCB__BUILDSCRAMSHAAUTHRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _authCB::_isUserRoot( const CHAR *username, _pmdEDUCB *cb, BOOLEAN *result )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( username && cb && result, "can not be nullptr" ) ;
      *result = FALSE;
      BSONObj userObj;

      rc = getUsrInfo( username, cb, userObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get user[%s] info, rc: %d",
                   username, rc ) ;

      try
      {
         BSONObj options = userObj.getObjectField( FIELD_NAME_OPTIONS ) ;
         BSONElement role = options.getField( FIELD_NAME_ROLE ) ;
         if ( 0 == ossStrcmp( VALUE_NAME_ADMIN, role.valuestrsafe() ) )
         {
            *result = TRUE ;
            goto done ;
         }

         BSONObj roles = userObj.getObjectField( FIELD_NAME_ROLES ) ;
         for ( BSONObjIterator it( roles ); it.more(); )
         {
            BSONElement ele = it.next() ;
            if ( 0 == ossStrcmp( AUTH_ROLE_ROOT, ele.valuestrsafe() ) )
            {
               *result = TRUE ;
               goto done ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // Check if any user(not me) of role _root exists.
   // Query matcher:
   // {
   //   "$and": [
   //      { "User": { "$ne": username } },
   //      { "$or": [ { "Roles": "_root" }, { "Options.Role": "admin" } } ]
   //   ]
   // }
   BSONObj buildQueryConditionForRemoveUser( const CHAR *username )
   {
      BSONObjBuilder builder;
      BSONArrayBuilder andBuilder( builder.subarrayStart( "$and" ) );
      BSONObjBuilder userBuilder( andBuilder.subobjStart() );
      BSONObjBuilder neBuilder( userBuilder.subobjStart( FIELD_NAME_USER ) );
      neBuilder.append( "$ne", username );
      neBuilder.doneFast();
      userBuilder.doneFast();
      BSONObjBuilder orObjBuilder( andBuilder.subobjStart() );
      BSONArrayBuilder orArrayBuilder( orObjBuilder.subarrayStart( "$or" ) );
      BSONObjBuilder rolesBuilder( orArrayBuilder.subobjStart() );
      rolesBuilder.append( FIELD_NAME_ROLES, AUTH_ROLE_ROOT );
      rolesBuilder.doneFast();
      BSONObjBuilder optionsBuilder( orArrayBuilder.subobjStart() );
      optionsBuilder.append( FIELD_NAME_OPTIONS "." FIELD_NAME_ROLE, VALUE_NAME_ADMIN );
      optionsBuilder.doneFast();
      orArrayBuilder.doneFast();
      orObjBuilder.doneFast();
      andBuilder.doneFast();
      return builder.obj();
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTHCB__CHECKREMOVEUSER, "_authCB::_checkRemoveUser" )
   INT32 _authCB::_checkRemoveUser( const CHAR *username, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_AUTHCB__CHECKREMOVEUSER ) ;

      try
      {
         // If the user to be removed is the last one, it's OK. Otherwise, need
         // to check if any user of role admin remains.

         INT64 count = 0 ;
         rtnQueryOptions queryOption ;
         BOOLEAN isRoot = FALSE;
         SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

         queryOption.setCLFullName( AUTH_USR_COLLECTION ) ;

         rc = rtnGetCount( queryOption, dmsCB, cb, rtnCB, &count ) ;
         PD_RC_CHECK( rc, PDERROR, "Get user number failed, rc: %d", rc ) ;

         if ( count <= 1 )
         {
            goto done ;
         }

         // If the user to delete is granted _root or admin (old version) role,
         // ensure that there is another user which granted _root or admin.
         rc = _isUserRoot( username, cb, &isRoot ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to determine whether the user[%s] is %s, rc: %d",
                      username, AUTH_ROLE_ROOT, rc );

         if ( isRoot )
         {
            // Check if any user(not me) of role _root exists.
            BSONObj query = buildQueryConditionForRemoveUser( username );

            queryOption.setQuery( query ) ;
            rc = rtnGetCount( queryOption, dmsCB, cb, rtnCB, &count ) ;
            PD_RC_CHECK( rc, PDERROR, "Get user number failed, rc: %d", rc ) ;

            if ( 0 == count )
            {
               rc = SDB_OPERATION_DENIED ;
               PD_LOG_MSG( PDERROR, "Only users without %s role remain after "
                           "removing this user, rc: %d", AUTH_ROLE_ROOT, rc ) ;
               goto error ;
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
      PD_TRACE_EXITRC( SDB_AUTHCB__CHECKREMOVEUSER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      get gloabl SDB_AUTHCB cb
   */
   SDB_AUTHCB* sdbGetAuthCB ()
   {
      static SDB_AUTHCB s_authCB ;
      return &s_authCB ;
   }

}

