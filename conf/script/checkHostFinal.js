/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

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
@description: After check the target host, clean up the environment in target host.
              Here, we are going to remove the processes and js files, and drop the
              temporary director
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "HostInfo": [ { "IP": "192.168.20.42", "HostName": "susetzb", "User": "root", "Passwd": "sequoiadb", "InstallPath": "/opt/sequoiadb", "SshPort": "22", "AgentService": "11790" }, { "IP": "192.168.20.165", "HostName": "rhel64-test8", "User": "root", "Passwd": "sequoiadb", "InstallPath": "/opt/sequoiadb", "SshPort": "22", "AgentService": "11790" } ] } ;
   SYS_JSON:
   ENV_JSON:
   OTHER_JSON:
@return
   RET_JSON: the format is:  { "HostInfo": [ { "IP": "192.168.20.165", "errno": 0, "detail": "" }, { "IP": "192.168.20.166", "errno": 0, "detail": "" } ] }
*/

var FILE_NAME_CHECK_HOST_FINAL = "checkHostFinal.js" ;
var RET_JSON       = new Object() ;
RET_JSON[HostInfo] = [] ;
var rc             = SDB_OK ;
var errMsg         = "" ;


function main()
{
   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_FINAL, "Begin to post-check host" ) ;
   
   var infoArr = null ;
   var arrLen = null ;
   
   try
   {
      infoArr = BUS_JSON[HostInfo] ;
      arrLen = infoArr.length ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Js receive invalid argument" ;
      // record error message in log
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_FINAL,
              sprintf( errMsg + ", rc: ?, error: ?", rc, GETLASTERRMSG() )  ) ;
      // tell to user error happen
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   if ( arrLen == 0 )
   {
      errMsg = "Not specified any host to post-check" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_FINAL, errMsg ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }

   for ( var i = 0; i < arrLen; i++ )
   {
      var ssh        = null ;
      var obj        = null ;
      var ip         = null ;
      var user       = null ;
      var passwd     = null ;
      var sshport    = null ;
      var retObj     = new postCheckResult() ;
      
      try
      {
         obj         = infoArr[i]
         ip          = obj[IP] ;
         user        = obj[User] ;
         passwd      = obj[Passwd] ;
         sshport     = parseInt(obj[SshPort]) ;
         retObj[IP]  = ip ;
         
         // 1. ssh
         ssh = new Ssh( ip, user, passwd, sshport ) ;
         
         // 2. remove the temporary directory in target host but leave the log file
         removeTmpDir2( ssh, true ) ;
      }
      catch ( e )
      {
         SYSEXPHANDLE( e ) ;
         retObj[IP] = ip ;
         retObj[Errno] = GETLASTERROR() ;
         retObj[Detail] = GETLASTERRMSG() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_FINAL,
                 "Failed to post-check host[" + ip + "], rc: " + retObj[Errno] + ", detail: " + retObj[Detail] ) ;
         try
         {
            removeTmpDir2( ssh, true ) ;
         }
         catch( e ){}
      }
      
      RET_JSON[HostInfo].push( retObj ) ;
   }

   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_FINAL, "Finish post-checking host" ) ;

   // return the result
   return RET_JSON ;
}

// execute
   main() ;

