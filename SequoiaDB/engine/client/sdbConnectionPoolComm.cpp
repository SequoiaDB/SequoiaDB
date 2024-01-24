/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sdbConnectionPoolComm.cpp

   Descriptive Name = SDB Connection Pool Common Source File

   When/how to use: this program may be used on sequoiadb connection pool function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbConnectionPoolComm.hpp"
#include "ossErr.h"

namespace sdbclient
{
   //set user info
   void sdbConnectionPoolConf::setAuthInfo(
      const std::string &username,
      const std::string &passwd )
   {
      _userName = username ;
      _passwd = passwd ;
   }

   void sdbConnectionPoolConf::setAuthInfo(
      const string &username,
      const string &cipherFile,
      const string &token )
   {
      _userName = username ;
      _cipherFile = cipherFile ;
      _token = token ;
   }

   // set connection number info
   void sdbConnectionPoolConf::setConnCntInfo(
      INT32 initCnt,
      INT32 deltaIncCnt,
      INT32 maxIdleCnt,
      INT32 maxCnt )
   {
      _initConnCount = initCnt ;
      _deltaIncCount = deltaIncCnt ;
      _maxIdleCount = maxIdleCnt ;
      _maxCount = maxCnt ;
   }

   // set idle connection interval
   void sdbConnectionPoolConf::setCheckIntervalInfo(
      INT32 interval,
      INT32 aliveTime /*= 0*/ )
   {
      _checkInterval = interval ;
      _keepAliveTimeout = aliveTime ;
   }

   BOOLEAN sdbConnectionPoolConf::isValid()
   {
      BOOLEAN ret = TRUE ;

      // check count info
      if ( (0 >= _maxCount) || (0 > _maxIdleCount) ||
         (0 >= _deltaIncCount) || (0 > _initConnCount) )
      {
         goto error ;
      }
      if ( ( 0 >= _checkInterval ) || ( 0 > _keepAliveTimeout ) ||
         ( 0 > _syncCoordInterval ) )
      {
         goto error;
      }
      if ( _maxIdleCount > _maxCount )
      {
         goto error ;
      }
      if ( _deltaIncCount > _maxIdleCount )
      {
         _deltaIncCount = _maxIdleCount ;
      }
      if ( _initConnCount> _maxIdleCount )
      {
         _initConnCount = _maxIdleCount ;
      }
      // check idle connection interval
      if ( (0 != _keepAliveTimeout) && (_keepAliveTimeout < _checkInterval) )
         goto error ;
      // check connection strategy
      if ( (_connectStrategy < SDB_CONN_STY_SERIAL) ||
         (_connectStrategy > SDB_CONN_STY_BALANCE) )
      {
         goto error ;
      }

   done :
      return ret  ;
   error :
      ret = FALSE ;
      goto done  ;
   }
}