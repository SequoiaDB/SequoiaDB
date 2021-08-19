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

   Source File Name = authCB.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef AUTHCB_HPP__
#define AUTHCB_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.h"
#include "sdbInterface.hpp"
#include "utilAuthSCRAMSHA.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;

   /*
      _authCB define
   */
   class _authCB : public _IControlBlock
   {
   public:
      _authCB() ;
      ~_authCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_AUTH ; }
      virtual const CHAR* cbName() const { return "AUTHCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

   public:

      INT32 createUsr( BSONObj &obj, _pmdEDUCB *cb,
                       BSONObj *pOutObj = NULL,
                       INT32 w = 1 ) ;

      INT32 getUsrInfo( const CHAR *username, _pmdEDUCB *cb, BSONObj &info ) ;

      INT32 updatePasswd( const string &user, const string &oldPasswd,
                          const string &newPasswd, _pmdEDUCB *cb ) ;

      INT32 removeUsr( const BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;

      INT32 md5Authenticate( const BSONObj &obj,
                             _pmdEDUCB *cb,
                             BSONObj *pOutUserObj = NULL ) ;

      INT32 SCRAMSHAAuthenticate( const BSONObj &obj,
                                  _pmdEDUCB *cb,
                                  BSONObj *pOutUserObj = NULL ) ;

      INT32 needAuthenticate( _pmdEDUCB *cb, BOOLEAN &need ) ;

      BOOLEAN authEnabled() const
      {
         return _authEnabled ;
      }

   private:
      INT32   _initAuthentication( _pmdEDUCB *cb ) ;
      BSONObj _desensitization( const BSONObj &userObj ) ;

      INT32   _buildUserInfo( const CHAR *username,
                              const CHAR *passwdMd5,
                              const CHAR *clearTextPasswd,
                              const BSONObj &option,
                              BSONObj &userInfo ) ;
      INT32   _upgradeUserInfo( const CHAR *username, _pmdEDUCB *cb ) ;
      INT32   _step1( const BSONObj &obj, _pmdEDUCB *cb,
                      BSONObj *pOutUserObj = NULL ) ;
      INT32   _step2( const BSONObj &obj, _pmdEDUCB *cb,
                      BSONObj *pOutUserObj = NULL ) ;
      INT32   _validOptions( const BSONObj &option ) ;
      INT32   _parseMD5AuthMsgObj( const BSONObj &obj ) ;
      INT32   _parseCrtUserMsgObj( const BSONObj &obj,
                                   const CHAR **username,
                                   const CHAR **passwd,
                                   const CHAR **clearTextPasswd,
                                   BSONObj &option ) ;
      INT32   _parseDelUserMsgObj( const BSONObj &obj,
                                   const CHAR **username,
                                   const CHAR **passwd,
                                   const CHAR **nonce,
                                   const CHAR **identify,
                                   const CHAR **clientProof,
                                   INT32 &type ) ;
      INT32   _parseStep1MsgObj( const BSONObj &obj,
                                 const CHAR **username,
                                 const CHAR **clientNonce,
                                 INT32 &type ) ;
      INT32   _parseStep2MsgObj( const BSONObj &obj,
                                 const CHAR **username,
                                 const CHAR **identify,
                                 const CHAR **clientProof,
                                 const CHAR **combineNonce,
                                 INT32 &type ) ;
      INT32   _parseUserObj( const BSONObj &obj, INT32 type,
                             UINT32 &iterationCnt,
                             const CHAR **salt,
                             const CHAR **storedKey,
                             const CHAR **serverKey,
                             const CHAR **md5Passwd ) ;
   private:
      BOOLEAN     _authEnabled ;
   } ;

   typedef class _authCB SDB_AUTHCB ;

   /*
      get gloabl SDB_AUTHCB cb
   */
   SDB_AUTHCB* sdbGetAuthCB () ;

}

#endif // AUTHCB_HPP__

