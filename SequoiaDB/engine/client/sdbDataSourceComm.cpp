/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY ; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbDataSourceComm.cpp

   Descriptive Name = SDB Data Source Common Source File

   When/how to use: this program may be used on sequoiadb data source function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbDataSourceComm.hpp"
#include "ossErr.h"

namespace sdbclient
{
   void sdbDataSourceConf::setUserInfo( 
      const std::string &username,
      const std::string &passwd )
   {
      _userName = username ;
      _passwd = passwd ;
   }

   void sdbDataSourceConf::setConnCntInfo( 
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

   void sdbDataSourceConf::setCheckIntervalInfo( 
      INT32 interval, 
      INT32 aliveTime /*= 0*/ )
   {
      _checkInterval = interval ;
      _keepAliveTimeout = aliveTime ;
   }
   
   BOOLEAN sdbDataSourceConf::isValid()
   {
      BOOLEAN ret = TRUE ;
      
      if ( (0 >= _maxCount) || (0 >= _maxIdleCount) || 
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
         _deltaIncCount = _maxIdleCount ;    // TODO: why ?需求讨论后决定这样做
      }
      if ( _initConnCount> _maxIdleCount )
      {
         _initConnCount = _maxIdleCount ;    // TODO: why ?同上
      }
      if ( (0 != _keepAliveTimeout) && (_keepAliveTimeout < _checkInterval) )
         goto error ;
      if ( (_connectStrategy < DS_STY_SERIAL) ||
         (_connectStrategy > DS_STY_BALANCE) )
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