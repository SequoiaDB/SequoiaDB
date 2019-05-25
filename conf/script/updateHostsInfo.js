/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: update host info in local hosts table
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "HostName": "susetzb", "IP": "192.168.20.42", "User": "root", "Passwd": "sequoiadb", "HostInfo": [ { "HostName": "rhel64-test8", "IP": "192.168.20.165", "AgentService": "11790" }, { "HostName": "rhel64-test9", "IP": "192.168.20.166", "AgentService": "20000" }, { "HostName": "susetzb", "IP": "192.168.20.42", "AgentService": "11790" } ] } ;
   SYS_JSON: the format is:
   ENV_JSON:
@return
   RET_JSON: the format is: {}
*/

var RET_JSON          = new Object() ;
var rc                = SDB_OK ;
var errMsg            = "" ;

var host_ip           = "" ;
var FILE_NAME_UPDATE_HOSTS_INFO = "updateHostsInfo.js" ;

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   try
   {
      host_ip = BUS_JSON[IP] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_ROLLBACKSTANDALONE,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
   PD_LOG( arguments, PDEVENT, FILE_NAME_UPDATE_HOSTS_INFO,
           sprintf( "Begin to update hosts table in host[?]", host_ip ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG( arguments, PDEVENT, FILE_NAME_UPDATE_HOSTS_INFO,
           sprintf( "Finish updating hosts table in host[?]", host_ip ) ) ;
}

/* *****************************************************************************
@discretion: update host table
@author: Tanzhaobo
@parameter
   localhostip[string]: ip of localhost
   arr[Array]: update info
@return void
***************************************************************************** */
function _updateHostInfo( localhostip, arr )
{
   var omtool = null ;
   var cmd = new Cmd() ;
   if ( SYS_LINUX == SYS_TYPE )
   {
      omtool = adaptPath( System.getEWD() ) + 'sdbomtool' ;
   }
   else
   {
      // TODO: windows
   }
   for ( var i = 0; i < arr.length; i++ )
   {
      var str      = null ;
      var obj      = arr[i] ;
      var hostname = obj[HostName] ;
      var ip       = obj[IP] ;
      str = omtool + ' -m addhost --hostname ' + hostname + ' --ip ' + ip ;
      try
      {
         cmd.run( str ) ;
      }
      catch ( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to add info [ ? ? ] to hosts table in host[?]", ip, hostname, localhostip ) ;
         PD_LOG( arguments, PDERROR, FILE_NAME_UPDATE_HOSTS_INFO,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
}

/* *****************************************************************************
@discretion: update sdbcm config file
@author: Tanzhaobo
@parameter
   localhostip[string]: ip of localhost
   arr[Array]: update info
@return void
***************************************************************************** */
function _updateSdbcmCfgFile( lcoalhostip, arr )
{
   var agentport = null ;
   var hostname  = null ;
   var configobj = null ;
   try
   {
      configobj = eval ( '(' + Oma.getOmaConfigs() + ')' ) ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to get OM Agent's config info in host[" + lcoalhostip + "]" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_UPDATE_HOSTS_INFO,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   for ( var i = 0; i < arr.length; i++ )
   {
      try
      {
         agentport = arr[i][AgentService] ;
         hostname  = arr[i][HostName] ;
      }
      catch ( e )
      {
         continue ;
      }

      var str = hostname + OMA_MISC_CONFIG_PORT ;
      if ( OMA_PORT_DEFAULT_SDBCM_PORT != ( "" + agentport ) )
      {
         configobj[str] = agentport ; 
      }
      else
      {
         Oma.delAOmaSvcName( hostname ) ;
         if( typeof( configobj[str] ) === 'string' )
         {
            delete configobj[str] ;
         }
      }
   }

   try
   {
      Oma.setOmaConfigs( configobj ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to set OM Agent's config info in host[" + lcoalhostip + "]" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_UPDATE_HOSTS_INFO,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   _init()
   var ip            = null ;
   var user          = null ;
   var passwd        = null ;
   var updateInfoArr = null ;
   try
   {
      ip               = BUS_JSON[IP] ;
      user             = BUS_JSON[User] ;
      passwd           = BUS_JSON[Passwd] ;
      updateInfoArr    = BUS_JSON[HostInfo] ;

      // update host table
      _updateHostInfo( ip, updateInfoArr ) ;     
      // update sdbcm comfig file
      _updateSdbcmCfgFile( ip, updateInfoArr ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to update hosts table in host[" + ip + "]" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_UPDATE_HOSTS_INFO,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      _final() ;
      exception_handle( rc, errMsg ) ;
   }
   _final()
   return RET_JSON ;
}

// execute
main() ;

