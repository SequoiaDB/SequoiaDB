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

   Source File Name = omagentRemoteUsrSystem.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentRemoteUsrSystem.hpp"
#include "omagentMgr.hpp"
#include "omagentDef.hpp"
#include "ossCmdRunner.hpp"
#include "sptUsrSystemCommon.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace bson ;
using std::pair ;

namespace engine
{
   /*
      _remoteSystemPing implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemPing )

   _remoteSystemPing::_remoteSystemPing()
   {
   }

   _remoteSystemPing::~_remoteSystemPing()
   {
   }

   const CHAR* _remoteSystemPing::name()
   {
      return OMA_REMOTE_SYS_PING ;
   }

   INT32 _remoteSystemPing::doit( BSONObj &retObj )
   {

      INT32 rc = SDB_OK ;
      string err ;
      string host ;
      // get argument
      if ( _matchObj.isEmpty() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      host = _matchObj.getStringField( "hostname" ) ;
      if ( "" == host )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::ping( host, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemType implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemType )

    _remoteSystemType::_remoteSystemType()
   {
   }

   _remoteSystemType::~_remoteSystemType()
   {
   }

   const CHAR* _remoteSystemType::name()
   {
      return OMA_REMOTE_SYS_TYPE ;
   }

   INT32 _remoteSystemType::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string type ;
      string err ;

      // check argument
      if ( FALSE == _optionObj.isEmpty( ) ||
           FALSE == _matchObj.isEmpty( ) ||
           FALSE == _valueObj.isEmpty( ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "type() should have non arguments" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::type( err, type ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "type" << type ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetReleaseInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetReleaseInfo )

     _remoteSystemGetReleaseInfo::_remoteSystemGetReleaseInfo()
   {
   }

   _remoteSystemGetReleaseInfo::~_remoteSystemGetReleaseInfo()
   {
   }

    const CHAR* _remoteSystemGetReleaseInfo::name()
   {
      return OMA_REMOTE_SYS_GET_RELEASE_INFO ;
   }

   INT32 _remoteSystemGetReleaseInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      // check argument
      if ( FALSE == _optionObj.isEmpty( ) ||
           FALSE == _matchObj.isEmpty( ) ||
           FALSE == _valueObj.isEmpty( ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "getReleaseInfo() should have non arguments" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getReleaseInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetHostsMap implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetHostsMap )

   _remoteSystemGetHostsMap::_remoteSystemGetHostsMap()
   {
   }

   _remoteSystemGetHostsMap::~_remoteSystemGetHostsMap()
   {
   }

   const CHAR* _remoteSystemGetHostsMap::name()
   {
      return OMA_REMOTE_SYS_GET_HOSTS_MAP ;
   }

   INT32 _remoteSystemGetHostsMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder objBuilder ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      if ( FALSE == _optionObj.isEmpty( ) ||
           FALSE == _matchObj.isEmpty( ) ||
           FALSE == _valueObj.isEmpty( ) )
      {
         PD_LOG_MSG( PDERROR, "getHostsMap() should have non arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getHostsMap( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetAHostMap
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetAHostMap )

   _remoteSystemGetAHostMap::_remoteSystemGetAHostMap()
   {
   }

   _remoteSystemGetAHostMap::~_remoteSystemGetAHostMap()
   {
   }

   INT32 _remoteSystemGetAHostMap::init( const CHAR *pInfomation  )
   {
      int rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // check argument
      if ( FALSE == _matchObj.hasField( "hostname" ) ||
           jstNULL == _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }

      if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }

      _hostname = _matchObj.getStringField( "hostname" ) ;
      if ( "" == _hostname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteSystemGetAHostMap::name()
   {
      return OMA_REMOTE_SYS_GET_AHOST_MAP ;
   }

   INT32 _remoteSystemGetAHostMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      string ip ;
      rc = _sptUsrSystemCommon::getAHostMap( _hostname, err, ip ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "ip" << ip ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemAddAHostMap
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemAddAHostMap )

   _remoteSystemAddAHostMap::_remoteSystemAddAHostMap()
   {
   }

   _remoteSystemAddAHostMap::~_remoteSystemAddAHostMap()
   {
   }

   INT32 _remoteSystemAddAHostMap::init( const CHAR *pInfomation  )
   {
      int rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // check argument
      if ( FALSE == _valueObj.hasField( "hostname" ) ||
           jstNULL == _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      _hostname = _valueObj.getStringField( "hostname" ) ;
      if ( "" == _hostname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }

      if ( FALSE == _valueObj.hasField( "ip" ) ||
           jstNULL == _valueObj.getField( "ip" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "ip must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "ip" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "ip must be string" ) ;
         goto error ;
      }
      _ip = _valueObj.getStringField( "ip" ) ;
      if ( "" == _ip )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "ip can't be empty" ) ;
         goto error ;
      }

      if ( !isValidIPV4( _ip.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "ip is not ipv4" ) ;
         goto error ;
      }

      if ( FALSE == _valueObj.hasField( "isReplace" ) ||
           jstNULL == _valueObj.getField( "isReplace" ).type() ||
           Undefined == _valueObj.getField( "isReplace" ).type() )
      {
         _isReplace = TRUE ;
         goto done ;
      }

      if ( Bool != _valueObj.getField( "isReplace" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "isReplace must be BOOLEAN" ) ;
         goto error ;
      }
      _isReplace = _valueObj.getBoolField( "isReplace" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteSystemAddAHostMap::name()
   {
      return OMA_REMOTE_SYS_ADD_AHOST_MAP ;
   }

   INT32 _remoteSystemAddAHostMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::addAHostMap( _hostname, _ip, _isReplace, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemDelAHostMap
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemDelAHostMap )

   _remoteSystemDelAHostMap::_remoteSystemDelAHostMap()
   {
   }

   _remoteSystemDelAHostMap::~_remoteSystemDelAHostMap()
   {
   }

   INT32 _remoteSystemDelAHostMap::init( const CHAR *pInfomation  )
   {
      int rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      if ( FALSE == _matchObj.hasField( "hostname" ) ||
           jstNULL == _valueObj.getField( "hostname" ).type() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "hostname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "hostname" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname must be string" ) ;
         goto error ;
      }
      _hostname = _matchObj.getStringField( "hostname" ) ;
      if ( "" == _hostname )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "hostname can't be empty" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteSystemDelAHostMap::name()
   {
      return OMA_REMOTE_SYS_DEL_AHOST_MAP ;
   }

   INT32 _remoteSystemDelAHostMap::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::delAHostMap( _hostname, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetCpuInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetCpuInfo )

   _remoteSystemGetCpuInfo::_remoteSystemGetCpuInfo()
   {
   }

   _remoteSystemGetCpuInfo::~_remoteSystemGetCpuInfo()
   {
   }

    const CHAR* _remoteSystemGetCpuInfo::name()
   {
      return OMA_REMOTE_SYS_GET_CPU_INFO ;
   }

   INT32 _remoteSystemGetCpuInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getCpuInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemSnapshotCpuInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSnapshotCpuInfo )

   _remoteSystemSnapshotCpuInfo::_remoteSystemSnapshotCpuInfo()
   {
   }

   _remoteSystemSnapshotCpuInfo::~_remoteSystemSnapshotCpuInfo()
   {
   }

    const CHAR* _remoteSystemSnapshotCpuInfo::name()
   {
      return OMA_REMOTE_SYS_SNAPSHOT_CPU_INFO ;
   }

   INT32 _remoteSystemSnapshotCpuInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      rc = _sptUsrSystemCommon::snapshotCpuInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetMemInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetMemInfo )

   _remoteSystemGetMemInfo::_remoteSystemGetMemInfo()
   {
   }

   _remoteSystemGetMemInfo::~_remoteSystemGetMemInfo()
   {
   }

   const CHAR* _remoteSystemGetMemInfo::name()
   {
      return OMA_REMOTE_SYS_GET_MEM_INFO ;
   }

   INT32 _remoteSystemGetMemInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getMemInfo( err ,retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetDiskInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetDiskInfo )

   _remoteSystemGetDiskInfo::_remoteSystemGetDiskInfo()
   {
   }

   _remoteSystemGetDiskInfo::~_remoteSystemGetDiskInfo()
   {
   }

   const CHAR* _remoteSystemGetDiskInfo::name()
   {
      return OMA_REMOTE_SYS_GET_DISK_INFO ;
   }

   INT32 _remoteSystemGetDiskInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getDiskInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetNetcardInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetNetcardInfo )

   _remoteSystemGetNetcardInfo::_remoteSystemGetNetcardInfo()
   {
   }

   _remoteSystemGetNetcardInfo::~_remoteSystemGetNetcardInfo()
   {
   }

   const CHAR* _remoteSystemGetNetcardInfo::name()
   {
      return OMA_REMOTE_SYS_GET_NETCARD_INFO ;
   }

   INT32 _remoteSystemGetNetcardInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getNetcardInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemSnapshotNetcardInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSnapshotNetcardInfo )

   _remoteSystemSnapshotNetcardInfo::_remoteSystemSnapshotNetcardInfo()
   {
   }

   _remoteSystemSnapshotNetcardInfo::~_remoteSystemSnapshotNetcardInfo()
   {
   }

   const CHAR* _remoteSystemSnapshotNetcardInfo::name()
   {
      return OMA_REMOTE_SYS_SNAPSHOT_NETCARD_INFO ;
   }

   INT32 _remoteSystemSnapshotNetcardInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::snapshotNetcardInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

   }

   /*
      _remoteSystemGetIpTablesInfo implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetIpTablesInfo )

   _remoteSystemGetIpTablesInfo::_remoteSystemGetIpTablesInfo()
   {
   }

   _remoteSystemGetIpTablesInfo::~_remoteSystemGetIpTablesInfo()
   {
   }

   const CHAR* _remoteSystemGetIpTablesInfo::name()
   {
      return OMA_REMOTE_SYS_GET_IPTABLES_INFO ;
   }

   INT32 _remoteSystemGetIpTablesInfo::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getIpTablesInfo( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetHostName implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetHostName )

   _remoteSystemGetHostName::_remoteSystemGetHostName()
   {
   }

   _remoteSystemGetHostName::~_remoteSystemGetHostName()
   {
   }

   const CHAR* _remoteSystemGetHostName::name()
   {
      return OMA_REMOTE_SYS_GET_HOSTNAME ;
   }

   INT32 _remoteSystemGetHostName::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      string hostname ;

      rc = _sptUsrSystemCommon::getHostName( err, hostname ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "hostname" << hostname ) ;
   done:
      return rc ;
   error:
      goto done ;

   }

   /*
      _remoteSystemSniffPort implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSniffPort )

   _remoteSystemSniffPort::_remoteSystemSniffPort()
   {
   }

   _remoteSystemSniffPort::~_remoteSystemSniffPort()
   {
   }

   const CHAR* _remoteSystemSniffPort::name()
   {
      return OMA_REMOTE_SYS_SNIFF_PORT ;
   }

   INT32 _remoteSystemSniffPort::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get argument" ) ;
         goto error ;
      }

      if ( FALSE == _matchObj.hasField( "port" ) ||
           jstNULL == _matchObj.getField( "port" ).type())
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "not specified the port to sniff" ) ;
         goto error ;
      }

      if ( NumberInt == _matchObj.getField( "port" ).type() )
      {
         _port = _matchObj.getIntField( "port" ) ;
      }
      else if ( String == _matchObj.getField( "port" ).type() )
      {
         UINT16 tempPort ;
         string svcname = _matchObj.getStringField( "port" ) ;
         rc = ossGetPort( svcname.c_str(), tempPort ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG_MSG ( PDERROR, "Invalid svcname: %s", svcname.c_str() ) ;
            goto error ;
         }
         _port = tempPort ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "port must be number or string" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemSniffPort::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::sniffPort( _port, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemListProcess implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListProcess )

   _remoteSystemListProcess::_remoteSystemListProcess()
   {
   }

   _remoteSystemListProcess::~_remoteSystemListProcess()
   {
   }

   const CHAR* _remoteSystemListProcess::name()
   {
      return OMA_REMOTE_SYS_LIST_PROC ;
   }

   INT32 _remoteSystemListProcess::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to get argument" ) ;
         goto error ;
      }
      _showDetail = _optionObj.getBoolField( "detail" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteSystemListProcess::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::listProcess( _showDetail, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemAddUser implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemAddUser )

   _remoteSystemAddUser::_remoteSystemAddUser()
   {
   }

   _remoteSystemAddUser::~_remoteSystemAddUser()
   {
   }

   const CHAR * _remoteSystemAddUser::name()
   {
      return OMA_REMOTE_SYS_ADD_USER ;
   }

   INT32 _remoteSystemAddUser::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::addUser( _valueObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemSetUserConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSetUserConfigs )

   _remoteSystemSetUserConfigs::_remoteSystemSetUserConfigs()
   {
   }

   _remoteSystemSetUserConfigs::~_remoteSystemSetUserConfigs()
   {
   }

   const CHAR * _remoteSystemSetUserConfigs::name()
   {
      return OMA_REMOTE_SYS_SET_USER_CONFIGS ;
   }

   INT32 _remoteSystemSetUserConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::setUserConfigs( _valueObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemDelUser implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemDelUser )

   _remoteSystemDelUser::_remoteSystemDelUser()
   {
   }

   _remoteSystemDelUser::~_remoteSystemDelUser()
   {
   }

   const CHAR * _remoteSystemDelUser::name()
   {
      return OMA_REMOTE_SYS_DEL_USER ;
   }

   INT32 _remoteSystemDelUser::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::delUser( _matchObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemAddGroup implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemAddGroup )

   _remoteSystemAddGroup::_remoteSystemAddGroup()
   {
   }

   _remoteSystemAddGroup::~_remoteSystemAddGroup()
   {
   }

   const CHAR * _remoteSystemAddGroup::name()
   {
      return OMA_REMOTE_SYS_ADD_GROUP ;
   }

   INT32 _remoteSystemAddGroup::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::addGroup( _valueObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemDelGroup implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemDelGroup )

   _remoteSystemDelGroup::_remoteSystemDelGroup()
   {
   }

   _remoteSystemDelGroup::~_remoteSystemDelGroup()
   {
   }

   const CHAR * _remoteSystemDelGroup::name()
   {
      return OMA_REMOTE_SYS_DEL_GROUP ;
   }

   INT32 _remoteSystemDelGroup::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string groupName ;
      string err ;

      if( FALSE == _matchObj.hasField( "name" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be config" ) ;
      }
      if( String != _matchObj.getField( "name" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;
      groupName = _matchObj.getStringField( "name" ) ;

      rc = _sptUsrSystemCommon::delGroup( groupName, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemListLoginUsers implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListLoginUsers )

   _remoteSystemListLoginUsers::_remoteSystemListLoginUsers()
   {
   }

   _remoteSystemListLoginUsers::~_remoteSystemListLoginUsers()
   {
   }

   const CHAR * _remoteSystemListLoginUsers::name()
   {
      return OMA_REMOTE_SYS_LIST_LOGIN_USERS ;
   }

   INT32 _remoteSystemListLoginUsers::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::listLoginUsers( _optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemListAllUsers implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListAllUsers )

   _remoteSystemListAllUsers::_remoteSystemListAllUsers()
   {
   }

   _remoteSystemListAllUsers::~_remoteSystemListAllUsers()
   {
   }

   const CHAR * _remoteSystemListAllUsers::name()
   {
      return OMA_REMOTE_SYS_LIST_ALL_USERS ;
   }

   INT32 _remoteSystemListAllUsers::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::listAllUsers( _optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemListGroups implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemListGroups )

   _remoteSystemListGroups::_remoteSystemListGroups()
   {
   }

   _remoteSystemListGroups::~_remoteSystemListGroups()
   {
   }

   const CHAR * _remoteSystemListGroups::name()
   {
      return OMA_REMOTE_SYS_LIST_GROUPS ;
   }

   INT32 _remoteSystemListGroups::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::listGroups( _optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetCurrentUser implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetCurrentUser )

   _remoteSystemGetCurrentUser::_remoteSystemGetCurrentUser()
   {
   }

   _remoteSystemGetCurrentUser::~_remoteSystemGetCurrentUser()
   {
   }

   const CHAR * _remoteSystemGetCurrentUser::name()
   {
      return OMA_REMOTE_SYS_GET_CURRENT_USER ;
   }

   INT32  _remoteSystemGetCurrentUser::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getCurrentUser( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemKillProcess implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemKillProcess )

   _remoteSystemKillProcess::_remoteSystemKillProcess ()
   {
   }

   _remoteSystemKillProcess::~_remoteSystemKillProcess ()
   {
   }

   const CHAR* _remoteSystemKillProcess::name()
   {
      return OMA_REMOTE_SYS_KILL_PROCESS ;
   }

   INT32 _remoteSystemKillProcess::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::killProcess( _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetProcUlimitConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetProcUlimitConfigs )

   _remoteSystemGetProcUlimitConfigs::_remoteSystemGetProcUlimitConfigs ()
   {
   }

   _remoteSystemGetProcUlimitConfigs::~_remoteSystemGetProcUlimitConfigs ()
   {
   }

   const CHAR* _remoteSystemGetProcUlimitConfigs::name()
   {
      return OMA_REMOTE_SYS_GET_PROC_ULIMIT_CONFIGS ;
   }

   INT32 _remoteSystemGetProcUlimitConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getProcUlimitConfigs( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemSetProcUlimitConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemSetProcUlimitConfigs )

   _remoteSystemSetProcUlimitConfigs::_remoteSystemSetProcUlimitConfigs ()
   {
   }

   _remoteSystemSetProcUlimitConfigs::~_remoteSystemSetProcUlimitConfigs ()
   {
   }

   const CHAR* _remoteSystemSetProcUlimitConfigs::name()
   {
      return OMA_REMOTE_SYS_SET_PROC_ULIMIT_CONFIGS ;
   }

   INT32 _remoteSystemSetProcUlimitConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      BSONObj configsObj ;
      if( FALSE == _valueObj.hasField( "configs" ) )
      {
         PD_LOG_MSG( PDERROR, "configsObj must be config" ) ;
         goto error ;
      }
      else if( Object != _valueObj.getField( "configs" ).type() )
      {
         PD_LOG_MSG( PDERROR, "configsObj must be obj" ) ;
         goto error ;
      }
      configsObj = _valueObj.getObjectField( "configs" ) ;
      rc = _sptUsrSystemCommon::setProcUlimitConfigs( configsObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetSystemConfigs implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetSystemConfigs )

   _remoteSystemGetSystemConfigs::_remoteSystemGetSystemConfigs ()
   {
   }

   _remoteSystemGetSystemConfigs::~_remoteSystemGetSystemConfigs ()
   {
   }

   const CHAR* _remoteSystemGetSystemConfigs::name()
   {
      return OMA_REMOTE_SYS_GET_SYSTEM_CONFIGS ;
   }

   INT32 _remoteSystemGetSystemConfigs::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      string type ;

      if ( TRUE == _optionObj.hasField( "type" ) )
      {
         if( String != _optionObj.getField( "type" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "type must be string" ) ;
            goto error ;
         }
         type =  _optionObj.getStringField( "type" ) ;
      }

      rc = _sptUsrSystemCommon::getSystemConfigs( type, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemRunService implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemRunService )

   _remoteSystemRunService::_remoteSystemRunService ()
   {
   }

   _remoteSystemRunService::~_remoteSystemRunService ()
   {
   }

   const CHAR* _remoteSystemRunService::name()
   {
      return OMA_REMOTE_SYS_RUN_SERVICE ;
   }

   INT32 _remoteSystemRunService::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string serviceName ;
      string command ;
      string options ;
      string err ;
      string retStr ;
      // check argument
      if ( FALSE == _matchObj.hasField( "serviceName" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "serviceName must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( "serviceName" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "serviceName must be string" ) ;
         goto error ;
      }
      serviceName = _matchObj.getStringField( "serviceName" ) ;

      if ( FALSE == _optionObj.hasField( "command" ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "command must be config" ) ;
         goto error ;
      }
      if ( String != _optionObj.getField( "command" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "command must be string" ) ;
         goto error ;
      }
      command = _optionObj.getStringField( "command" ) ;

      if ( TRUE == _optionObj.hasField( "options" ) )
      {
         if ( String != _optionObj.getField( "options" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "options must be string" ) ;
            goto error ;
         }
         options = _optionObj.getStringField( "options" ) ;
      }

      rc = _sptUsrSystemCommon::runService( serviceName, command, options,
                                           err, retStr ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "outStr" << retStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemBuildTrusty implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemBuildTrusty )

   _remoteSystemBuildTrusty::_remoteSystemBuildTrusty ()
   {
   }

   _remoteSystemBuildTrusty::~_remoteSystemBuildTrusty ()
   {
   }

   const CHAR* _remoteSystemBuildTrusty::name()
   {
      return OMA_REMOTE_SYS_BUILD_TRUSTY ;
   }

   INT32 _remoteSystemBuildTrusty::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32           rc = SDB_OK ;
      UINT32          exitCode    = 0 ;
      stringstream    cmd ;
      stringstream    grepCmd ;
      stringstream    echoCmd ;
      _ossCmdRunner   runner ;
      string          outStr ;
      string          pubKey ;
      string          homePath ;
      string          sshPath ;
      string          keyPath ;
      string          err ;
      // get pub key
      if ( FALSE == _valueObj.hasField( "key" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "key must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "key" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "key must be string" ) ;
         goto error ;
      }
      pubKey = _valueObj.getStringField( "key" ) ;

      rc = _sptUsrSystemCommon::getHomePath( homePath, err ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }

      sshPath = homePath + "/.ssh/" ;
      rc = ossAccess( sshPath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc )
         {
            rc = ossMkdir( sshPath.c_str(), OSS_RWXU ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "Failed to mkdir, rc:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Failed to access dir: %s", sshPath.c_str() ) ;
            goto error ;
         }
      }

      // check whether it had build Trusty
      grepCmd << "touch " << homePath << "/.ssh/authorized_keys; grep -x \""
              << pubKey << "\" "<< homePath << "/.ssh/authorized_keys" ;
      rc = runner.exec( grepCmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << grepCmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if( '\n' == outStr[ outStr.size()-1 ] )
      {
         outStr.erase( outStr.size()-1, 1 ) ;
      }

      // if not, build Trusty
      if ( outStr.empty() )
      {
         // echo pub key to authorized_keys
         cmd << "echo -n \"" << pubKey << "\" >> " << homePath
             << "/.ssh/authorized_keys" ;
         rc = runner.exec( cmd.str().c_str(), exitCode,
                           FALSE, -1, FALSE, NULL, TRUE ) ;
         if ( SDB_OK != rc )
         {
            stringstream ss ;
            ss << "failed to exec cmd " << cmd.str() << ",rc:"
               << rc
               << ",exit:"
               << exitCode ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }

         // set authorizedkeys mode
         echoCmd << "chmod 755 " << homePath << ";"
                 << "chmod 700 " << homePath << "/.ssh;"
                 << "chmod 644 " << homePath << "/.ssh/authorized_keys" ;
         rc = runner.exec( echoCmd.str().c_str(), exitCode, FALSE,
                           -1, FALSE, NULL, TRUE ) ;
         if ( SDB_OK != rc )
         {
            stringstream ss ;
            ss << "failed to exec cmd " << "chmod 600 authorized_keys" << ",rc:"
               << rc
               << ",exit:"
               << exitCode ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   /*
      _remoteSystemRemoveTrusty implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemRemoveTrusty )

   _remoteSystemRemoveTrusty::_remoteSystemRemoveTrusty ()
   {
   }

   _remoteSystemRemoveTrusty::~_remoteSystemRemoveTrusty ()
   {
   }

   const CHAR* _remoteSystemRemoveTrusty::name()
   {
      return OMA_REMOTE_SYS_REMOVE_TRUSTY ;
   }

   INT32 _remoteSystemRemoveTrusty::doit( BSONObj &retObj )
   {
#if defined(_LINUX)
      INT32  rc          = SDB_OK ;
      UINT32 exitCode    = 0 ;
      string             matchStr ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      string             homePath ;
      string             err ;

      if ( FALSE == _valueObj.hasField( "matchStr" ) )
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG_MSG( PDERROR, "matchStr must be config" ) ;
         goto error ;
      }

      if ( String != _valueObj.getField( "matchStr" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "matchStr must be string" ) ;
         goto error ;
      }
      matchStr = _valueObj.getStringField( "matchStr" ) ;

      rc = _sptUsrSystemCommon::getHomePath( homePath, err ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err.c_str() ) ;
         goto error ;
      }

      // get authorized_keys content
      cmd << "cat " << homePath << "/.ssh/authorized_keys";

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }

      rc = _removeKey( outStr.c_str(), matchStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to remove key" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _remoteSystemRemoveTrusty::_removeKey( const CHAR* buf,
                                                string matchStr )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      string fileDir ;
      vector<string> splited ;
      string err ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      if ( '\n' == matchStr[ matchStr.size()-1] )
      {
         matchStr.erase( matchStr.size()-1, 1 ) ;
      }

      rc = _sptUsrSystemCommon::getHomePath( fileDir, err ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      fileDir += "/.ssh/authorized_keys" ;

      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }
      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      rc = ossOpen( fileDir.c_str(),
                    OSS_READWRITE | OSS_REPLACE,
                    OSS_RWXU, file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file" ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end(); itr++ )
      {
         // check if pubkey exist
         if ( matchStr != *itr )
         {
            rc = ossWriteN( &file, ( *itr + "\n" ).c_str() , itr->size() + 1 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to write file" ) ;
               goto error ;
            }
         }
      }

      if ( file.isOpened() )
      {
         rc  = ossClose( file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to close file" ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   /*
      _remoteSystemGetUserEnv implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetUserEnv )

   _remoteSystemGetUserEnv::_remoteSystemGetUserEnv ()
   {
   }

   _remoteSystemGetUserEnv::~_remoteSystemGetUserEnv ()
   {
   }

   const CHAR* _remoteSystemGetUserEnv::name()
   {
      return OMA_REMOTE_SYS_GET_USER_ENV ;
   }

   INT32 _remoteSystemGetUserEnv::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrSystemCommon::getUserEnv( err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetPID implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetPID )

   _remoteSystemGetPID::_remoteSystemGetPID ()
   {
   }

   _remoteSystemGetPID::~_remoteSystemGetPID ()
   {
   }

   const CHAR* _remoteSystemGetPID::name()
   {
      return OMA_REMOTE_SYS_GET_PID ;
   }

   INT32 _remoteSystemGetPID::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      string err ;

      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No need arguments" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getPID( id, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "PID" << id ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetTID implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetTID )

   _remoteSystemGetTID::_remoteSystemGetTID ()
   {
   }

   _remoteSystemGetTID::~_remoteSystemGetTID ()
   {
   }

   const CHAR* _remoteSystemGetTID::name()
   {
      return OMA_REMOTE_SYS_GET_TID ;
   }

   INT32 _remoteSystemGetTID::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      string err ;

      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No need arguments" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getTID( id, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "TID" << id ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteSystemGetEWD implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteSystemGetEWD )

   _remoteSystemGetEWD::_remoteSystemGetEWD ()
   {
   }

   _remoteSystemGetEWD::~_remoteSystemGetEWD ()
   {
   }

   const CHAR* _remoteSystemGetEWD::name()
   {
      return OMA_REMOTE_SYS_GET_EWD ;
   }

   INT32 _remoteSystemGetEWD::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string ewd ;
      string err ;

      if ( FALSE == _optionObj.isEmpty() ||
           FALSE == _matchObj.isEmpty() ||
           FALSE == _valueObj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No need arguments" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getEWD( ewd, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( "EWD" << ewd ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
}
