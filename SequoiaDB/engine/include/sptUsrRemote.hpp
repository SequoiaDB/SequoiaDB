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

   Source File Name = sptUsrRemote.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/07/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRREMOTE_HPP__
#define SPT_USRREMOTE_HPP__

#include "sptUsrRemoteAssit.hpp"
#include "sptApi.hpp"
#include "../bson/bsonobj.h"
#include "oss.hpp"

namespace engine
{
   class _sptUsrRemote : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrRemote )

   public:
      _sptUsrRemote() ;

      ~_sptUsrRemote() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 getInfo( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 runCommand( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 runCommand( const string &command,
                        const bson::BSONObj &optionObj,
                        const bson::BSONObj &matchObj,
                        const bson::BSONObj &valueObj,
                        bson::BSONObj &errDetail, bson::BSONObj &retObj,
                        BOOLEAN needRecv = TRUE ) ;
   private:
      INT32 _mergeArg( const bson::BSONObj& optionObj,
                       const bson::BSONObj& matchObj,
                       const bson::BSONObj& valueObj,
                       bson::BSONObjBuilder& builder ) ;

      INT32 _initBSONObj( BSONObj &recvObj, const CHAR* buf ) ;
   private:
      sptUsrRemoteAssit _assit ;
      string  _hostname ;
      string  _svcname ;

   } ;

}
#endif
