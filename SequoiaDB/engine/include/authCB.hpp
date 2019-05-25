/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

      INT32 createUsr( BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;

      INT32 getUsrInfo( const string &user, _pmdEDUCB *cb, BSONObj &info ) ;

      INT32 updatePasswd( const string &user, const string &oldPasswd, 
                          const string &newPasswd, _pmdEDUCB *cb ) ;

      INT32 removeUsr( BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;

      INT32 authenticate( BSONObj &obj, _pmdEDUCB *cb,
                          BOOLEAN chkPasswd = TRUE ) ;

      INT32 needAuthenticate( _pmdEDUCB *cb, BOOLEAN &need ) ;

      BOOLEAN authEnabled() const
      {
         return _authEnabled ;
      }

   private:
      INT32 _initAuthentication( _pmdEDUCB *cb ) ;
      INT32 _createUsr( BSONObj &obj, _pmdEDUCB *cb, INT32 w = 1 ) ;
      INT32 _valid( BSONObj &obj, BOOLEAN notEmpty ) ;
      INT32 _validSource( BSONObj &obj, BOOLEAN chkPasswd ) ;

   private:
      BOOLEAN _authEnabled ;
   } ;

   typedef class _authCB SDB_AUTHCB ;

   /*
      get gloabl SDB_AUTHCB cb
   */
   SDB_AUTHCB* sdbGetAuthCB () ;

}

#endif // AUTHCB_HPP__

