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

   Source File Name = omagentRemoteBase.cpp

   Dependencies: N/A


   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentRemoteBase.hpp"
#include "cmdUsrOmaUtil.hpp"
#include "pmdOptions.h"
#include "msgDef.h"
#include "pmd.hpp"
#include "ossCmdRunner.hpp"
#include "ossSocket.hpp"
#include "ossIO.hpp"
#include "oss.h"
#include "ossProc.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#if defined (_LINUX)
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pwd.h>
#else
#include <iphlpapi.h>
#pragma comment( lib, "IPHLPAPI.lib" )
#endif

using namespace bson ;

namespace engine
{
   /*
      _remoteExec implement
   */

   _remoteExec::_remoteExec()
   {
   }

   _remoteExec::~_remoteExec()
   {
   }

   INT32 _remoteExec::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj Obj( pInfomation ) ;

         _optionObj = Obj.getObjectField( "$optionObj" ) ;
         _matchObj  = Obj.getObjectField( "$matchObj" ) ;
         _valueObj  = Obj.getObjectField( "$valueObj" ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteOmaConfigs implement
   */

   _remoteOmaConfigs::_remoteOmaConfigs()
   {
   }

   _remoteOmaConfigs::~_remoteOmaConfigs()
   {
   }

   INT32  _remoteOmaConfigs::_confObj2Str( const bson::BSONObj &conf, string &str,
                                           string &errMsg,
                                           const CHAR* pExcept )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObjIterator it ( conf ) ;
      while ( it.more() )
      {
         BSONElement e = it.next() ;

         if ( pExcept && 0 != pExcept[0] &&
              0 == ossStrcmp( pExcept, e.fieldName() ) )
         {
            continue ;
         }

         ss << e.fieldName() << "=" ;
         if ( e.type() == String )
         {
            ss << e.valuestr() ;
         }
         else if ( e.type() == NumberInt )
         {
            ss << e.numberInt() ;
         }
         else if ( e.type() == NumberLong )
         {
            ss << e.numberLong() ;
         }
         else if ( e.type() == NumberDouble )
         {
            ss << e.numberDouble() ;
         }
         else if ( e.type() == Bool )
         {
            ss << ( e.boolean() ? "TRUE" : "FALSE" ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            stringstream errss ;
            errss << e.toString() << " is invalid config" ;
            errMsg = errss.str() ;
            goto error ;
         }
         ss << endl ;
      }
      str = ss.str() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteOmaConfigs::_getNodeConfigFile( string svcname,
                                                string &filePath )
   {
      INT32 rc = SDB_OK ;
      utilInstallInfo info ;
      CHAR confFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      string ewdConfPath = SDBCM_LOCAL_PATH OSS_FILE_SEP
                           + svcname + OSS_FILE_SEP PMD_DFT_CONF ;

      rc = ossGetEWD( confFile, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get ewd, rc: %d", rc ) ;
         goto error ;
      }

      rc = utilCatPath( confFile, OSS_MAX_PATHSIZE, ewdConfPath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to cat path: %d", rc ) ;
         goto error ;
      }

      rc = ossAccess( confFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to access config file: %s, rc: %d",
                 confFile, rc ) ;
         goto error ;
      }
   done:
      if ( SDB_OK == rc )
      {
         filePath = confFile ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteOmaConfigs::_getNodeConfInfo( const string & confFile,
                                              BSONObj &conf,
                                              string &errMsg,
                                              BOOLEAN allowNotExist )
   {

      INT32 rc = SDB_OK ;
      po::options_description desc ;
      po ::variables_map vm ;

      MAP_CONFIG_DESC( desc ) ;

      rc = ossAccess( confFile.c_str() ) ;
      if ( rc )
      {
         if ( allowNotExist )
         {
            rc = SDB_OK ;
            goto done ;
         }
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         errMsg = ss.str() ;
         goto error ;
      }

      rc = utilReadConfigureFile( confFile.c_str(), desc, vm ) ;
      if ( SDB_FNE == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] is not exist" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else if ( SDB_PERM == rc )
      {
         stringstream ss ;
         ss << "conf file[" << confFile << "] permission error" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else if ( rc )
      {
         stringstream ss ;
         ss << "read conf file[" << confFile << "] error" ;
         errMsg = ss.str() ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         po ::variables_map::iterator it = vm.begin() ;
         while ( it != vm.end() )
         {
            try
            {
               builder.append( it->first, it->second.as<string>() ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG_MSG( PDERROR, "Failed to append config item: %s(%s)",
                           it->first.c_str(), e.what() ) ;
            }
            ++it ;
         }
         conf = builder.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
