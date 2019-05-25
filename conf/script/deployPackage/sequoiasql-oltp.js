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
@description: deploy sequoiasql-oltp package
@modify list:
   2017-09-12 JiaWen He  Init

1. Generate plan
   @parameter
      var SYS_STEP = "Generate plan" ;
      var BUS_JSON = {"_id":{"$oid":"59bb48afbe1f79df0fa98778"},"TaskID":8,"Type":7,"TypeDesc":"DEPLOY_PACKAGE","TaskName":"DEPLOY_PACKAGE","CreateTime":{"$timestamp":"2017-09-15-11.27.43.000000"},"EndTime":{"$timestamp":"2017-09-15-11.27.43.000000"},"Status":0,"StatusDesc":"INIT","AgentHost":"ubuntu-jw-02","AgentService":"11790","Info":{"ClusterName":"myCluster1","SdbUser":"sdbadmin","SdbPasswd":"sdbadmin","SdbUserGroup":"sdbadmin_group","InstallPacket":"/opt/sequoiadb/packet/sequoiasql-oltp-2.8.2-x86_64-enterprise-installer.run","PackageName":"sequoiasql-oltp","InstallPath": "/opt/sequoiasqloltp/","Enforced":true,"HostInfo":[{"HostName":"ubuntu-jw-02","IP":"192.168.3.232","AgentService":"11790","SshPort":"22","User":"root","Passwd":"123"}]},"errno":0,"detail":"","Progress":0,"ResultInfo":[{"HostName":"ubuntu-jw-01","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]};
   @return
      RET_JSON: the format is: {"Plan":[[{"cmd":"install","TaskID":8,"Info":{"PackageName":"sequoiasql-oltp","InstallPath":"/opt/sequoiasqloltp/","InstallPacket":"/opt/sequoiadb/packet/sequoiasql-oltp-2.8.2-x86_64-enterprise-installer.run","SdbUser":"sdbadmin","SdbPasswd":"sdbadmin","SdbUserGroup":"sdbadmin_group","HostInfo":{"HostName":"ubuntu-jw-01","IP":"192.168.3.231","AgentService":"11790","SshPort":"22","User":"root","Passwd":"123"}},"ResultInfo":{"HostName":"ubuntu-jw-01","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":90}}]]}

2. Install package
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"cmd":"install","TaskID":8,"Info":{"PackageName":"sequoiasql-oltp","InstallPath":"/opt/sequoiasqloltp/","InstallPacket":"/opt/sequoiadb/packet/sequoiasql-oltp-2.8.2-x86_64-enterprise-installer.run","SdbUser":"sdbadmin","SdbPasswd":"sdbadmin","SdbUserGroup":"sdbadmin_group","HostInfo":{"HostName":"ubuntu-jw-02","IP":"192.168.3.232","AgentService":"11790","SshPort":"22","User":"root","Passwd":"123"}},"ResultInfo":{"HostName":"ubuntu-jw-01","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":90}};
   @return
      RET_JSON: the format is: {"HostName":"ubuntu-jw-02","Status":4,"StatusDesc":"FINISH","errno":0,"detail":"","Flow":[],"Progress":90}

3. Check result
   @parameter
      var SYS_STEP = "Check result" ;
      var BUS_JSON = {"errno":0,"detail":"","Progress":15,"ResultInfo":[{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":["Installing coord[ubuntu-jw-02:11840]","Successfully create coord[ubuntu-jw-02:11840]"]},{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}],"TaskID":25,"Info":{"Config":[{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"","dbpath":"/opt/sequoiadb/database/catalog/11840","svcname":"11840","role":"catalog","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"group1","dbpath":"/opt/sequoiadb/database/data/11850","svcname":"11850","role":"data","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","DeployMod":"vertical"}} ;
   @return
      RET_JSON: the format is: {"errno":0,"detail":""}
*/

function _getErrorMsg( rc, e, message )
{
   var error = null ;

   if( rc == SDB_OK )
   {
      rc = SDB_SYS ;
      error = new SdbError( rc, e.message ) ;
   }
   else if( rc )
   {
      error = new SdbError( rc, message ) ;
   }

   return error ;
}

/*
return:
   0: Not installed
   1: The same version is already installed
   2: Different versions have been installed
*/
function _checkInstall( PD_LOGGER, md5, hostName, agentPort, installPath )
{
   var installStatus = 0 ;
   var remote = null ;
   var file = null ;
   var md5File = installPath + '/checksum.md5' ;

   try
   {
      remote = new Remote( hostName, agentPort ) ;
      file = remote.getFile() ;
      if ( true == file.exist( md5File ) )
      {
         var fileObj = remote.getFile( md5File ) ;
         var remoteMd5 = fileObj.read() ;
         remoteMd5 = remoteMd5.replace( /\r\n/g, '' ) ;
         remoteMd5 = remoteMd5.replace( /\n/g, '' ) ;

         if ( md5 === remoteMd5 )
         {
            installStatus = 1 ;
            PD_LOGGER.logTask( PDEVENT, sprintf( "The host has deployed the " +
                                                 "package: host [?], path[?]",
                                                 hostName, installPath ) ) ;
         }
         else
         {
            PD_LOGGER.logTask( PDERROR, sprintf( "The host has deployed the " +
                                                 "package: host [?], path[?], " +
                                                 "md5[local: ?, remote: ?]",
                                                 hostName, installPath,
                                                 md5, remoteMd5 ) ) ;
            installStatus = 2 ;
         }
      }
      else
      {
         installStatus = 0 ;
      }
   }
   catch( e )
   {
      var error = _getErrorMsg( getLastError(), e,
                                sprintf( "Failed to get remote md5: " +
                                         "host [?:?], errMsg:?",
                                          hostName, agentPort, 
                                          getLastErrObj().toObj().detail ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      installStatus = 2 ;
   }

   return installStatus ;
}

function _getLocalPackageMd5( packagePath )
{
   try
   {
      return File.md5( packagePath ) ;
   }
   catch( e )
   {
      return null ;
   }
}

function _getAgentPort( hostName )
{
   return Oma.getAOmaSvcName( hostName ) ;
}

function GeneratePlan( taskID )
{
   var PD_LOGGER = new Logger( "sequoiasql-oltp.js" ) ;
   var plan = {} ;
   var planInfo      = BUS_JSON[FIELD_INFO] ;
   var resultInfo    = BUS_JSON[FIELD_RESULTINFO] ;
   var packageName   = planInfo[FIELD_PACKAGE_NAME] ;
   var sdbUser       = planInfo[FIELD_SDBUSER] ;
   var sdbPasswd     = planInfo[FIELD_SDBPASSWD] ;
   var sdbUserGroup  = planInfo[FIELD_SDBUSERGROUP] ;
   var installPacket = planInfo[FIELD_INSTALL_PACKET] ;
   var installPath   = planInfo[FIELD_INSTALL_PATH] ;
   var enforced      = planInfo[FIELD_ENFORCED] ;
   var installTask   = [] ;
   var hostNum       = 0 ;
   var progressStep  = 0 ;
   var md5 = _getLocalPackageMd5( installPacket ) ;

   PD_LOGGER.setTaskId( taskID ) ;

   plan[FIELD_PLAN] = [] ;

   if( isTypeOf( planInfo[FIELD_HOST_INFO], "object" ) == false )
   {
      var error = new SdbError( SDB_SYS,
                                sprintf( "Invalid argument, ? is not object",
                                         FIELD_HOST_INFO ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   hostNum = planInfo[FIELD_HOST_INFO].length ;
   progressStep = parseInt( 90 / hostNum ) ;

   for( var index in planInfo[FIELD_HOST_INFO] )
   {
      var installConfig = {} ;
      var hostInfo  = planInfo[FIELD_HOST_INFO][index] ;
      var hostName  = hostInfo[FIELD_HOSTNAME] ;
      var agentPort = _getAgentPort( hostName ) ;
      var installStatus = 0 ;

      if ( false == enforced )
      {
         installStatus = _checkInstall( PD_LOGGER, md5, hostName,
                                        agentPort, installPath ) ;
      }
      else
      {
         installStatus = 0 ;
      }

      if( installStatus == 0 )
      {
         installConfig[FIELD_CMD] = "install" ;
      }
      else if( installStatus == 1 )
      {
         installConfig[FIELD_CMD] = "skip" ;
      }
      else
      {
         installConfig[FIELD_CMD] = "error" ;
      }

      installConfig[FIELD_TASKID] = taskID ;
      installConfig[FIELD_INFO]   = {} ;
      installConfig[FIELD_INFO][FIELD_PACKAGE_NAME]   = packageName ;
      installConfig[FIELD_INFO][FIELD_INSTALL_PATH]   = installPath ;
      installConfig[FIELD_INFO][FIELD_INSTALL_PACKET] = installPacket ;
      installConfig[FIELD_INFO][FIELD_SDBUSER]        = sdbUser ;
      installConfig[FIELD_INFO][FIELD_SDBPASSWD]      = sdbPasswd ;
      installConfig[FIELD_INFO][FIELD_SDBUSERGROUP]   = sdbUserGroup ;
      installConfig[FIELD_INFO][FIELD_HOST_INFO]      = hostInfo ;
      installConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      installConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;

      installTask.push( installConfig ) ;
   }

   plan[FIELD_PLAN].push( installTask ) ;

   PD_LOGGER.logTask( PDEVENT, "Finish generate plan" ) ;

   return plan ;
}

function SendPackage( taskID )
{
   var PD_LOGGER = new Logger( "sequoiasql-oltp.js" ) ;
   var error = null ;
   var taskInfo      = BUS_JSON[FIELD_INFO] ;
   var resultInfo    = BUS_JSON[FIELD_RESULTINFO] ;
   var hostInfo      = taskInfo[FIELD_HOST_INFO] ;

   var installPacket = taskInfo[FIELD_INSTALL_PACKET] ;
   var hostName      = hostInfo[FIELD_HOSTNAME] ;
   var agentPort     = _getAgentPort( hostName ) ;
   var destPath      = '/tmp/packet' ;
   var destFileAddr  = hostName + ':' + agentPort + '@' + destPath +
                       '/sequoiasql-oltp.run' ;
   var remote        = null ;
   var file          = null ;

   PD_LOGGER.setTaskId( taskID ) ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to send packet: host [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to send packet: host [?]",
                                         hostName ) ) ;

   try
   {
      remote = new Remote( hostName, agentPort ) ;
      file = remote.getFile() ;
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed to get remote file obj: " +
                                     "host [?:?]",
                                     hostName, agentPort ) ) ;
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   try
   {
      file.mkdir( destPath ) ;
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed create remote dir: host [?:?], " +
                                     "path [?]",
                                     hostName, agentPort, destPath ) ) ;
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   try
   {
      File.scp( installPacket, destFileAddr ) ;
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed copy package: host [?:?], " +
                                     "package [?]",
                                     hostName, agentPort, installPacket ) ) ;
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Finish to Send packet: host [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Finish to Send packet: host [?]",
                                         hostName ) ) ;

   return resultInfo ;
}

function _getVersion( hostName, user, pwd, sshPort, installPath )
{
   var cmd = installPath + '/bin/sdb_sql_ctl --version' ;
   var version = "" ;

   try
   {
      var ssh = new Ssh( hostName, user, pwd, parseInt( sshPort ) ) ;
      ssh.exec( cmd ) ;
      version = ssh.getLastOut() ;
      version = version.replace( /[\r]/g, '' ) ;
      version = version.replace( /[\n]/g, '' ) ;
   }
   catch( e )
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Failed to get version: host [?]",
                                         hostName ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
      version = null ;
   }

   return version ;
}

function InstallPackage( taskID )
{
   var PD_LOGGER = new Logger( "sequoiasql-oltp.js" ) ;
   var error = null ;
   var taskInfo      = BUS_JSON[FIELD_INFO] ;
   var resultInfo    = BUS_JSON[FIELD_RESULTINFO] ;
   var hostInfo      = taskInfo[FIELD_HOST_INFO] ;

   var installPath   = taskInfo[FIELD_INSTALL_PATH] ;
   var sdbUser       = taskInfo[FIELD_SDBUSER] ;
   var sdbPasswd     = taskInfo[FIELD_SDBPASSWD] ;
   var sdbUserGroup  = taskInfo[FIELD_SDBUSERGROUP] ;

   var hostName      = hostInfo[FIELD_HOSTNAME] ;
   var sshPort       = hostInfo[FIELD_SSH_PORT] ;
   var user          = hostInfo[FIELD_USER] ;
   var pwd           = hostInfo[FIELD_PASSWD] ;
   var destPath      = '/tmp/packet/sequoiasql-oltp.run' ;
   var options = "" ;

   PD_LOGGER.setTaskId( taskID ) ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to install packet: host [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to install packet: host [?]",
                                         hostName ) ) ;

   var cmd = 'chmod u+x ' + destPath ;
   try
   {
      var ssh = new Ssh( hostName, user, pwd, parseInt( sshPort ) ) ;

      ssh.exec( cmd ) ;
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed to geve package executable " +
                                     "authority: host [?], " +
                                     "detail[?]", hostName, getLastErrMsg() ) ) ;
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   options += " --mode unattended --installmode cover " + " --prefix " + installPath ;
   options += " --username " + sdbUser + " --userpasswd " + sdbPasswd ;
   options += " --groupname " + sdbUserGroup ;

   var cmd = destPath + options ;
   try
   {
      var ssh = new Ssh( hostName, user, pwd, parseInt( sshPort ) ) ;

      ssh.exec( cmd ) ;

      var version = _getVersion( hostName, user, pwd, sshPort, installPath ) ;
      resultInfo[FIELD_VERSION] = version ;
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed to install package: host [?], " +
                                     "detail[?]", hostName, getLastErrMsg() ) ) ;
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Finish to install packet: host [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Finish to install packet: host [?]",
                                         hostName ) ) ;

   return resultInfo ;
}

function Skip()
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var hostInfo   = taskInfo[FIELD_HOST_INFO] ;

   var installPath   = taskInfo[FIELD_INSTALL_PATH] ;
   var hostName      = hostInfo[FIELD_HOSTNAME] ;

   resultInfo[FIELD_FLOW].push( sprintf( "The host has deployed the " +
                                         "package: host [?], path[?]",
                                         hostName, installPath ) ) ;

   return resultInfo ;
}

function Error()
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var hostInfo   = taskInfo[FIELD_HOST_INFO] ;

   var installPath   = taskInfo[FIELD_INSTALL_PATH] ;
   var hostName      = hostInfo[FIELD_HOSTNAME] ;

   resultInfo[FIELD_ERRNO]  = SDB_SYS ;
   resultInfo[FIELD_DETAIL] = getErr( SDB_SYS ) ;
   resultInfo[FIELD_STATUS] = STATUS_FAIL ;
   resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
   resultInfo[FIELD_FLOW].push( sprintf( "The host has deployed the " +
                                         "package: host [?], path[?]",
                                         hostName, installPath ) ) ;

   return resultInfo ;
}

function CheckResult( taskID )
{
   var PD_LOGGER = new Logger( "sequoiasql-oltp.js" ) ;
   var result = new commonResult() ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;

   PD_LOGGER.setTaskId( taskID ) ;

   for( var index in resultInfo )
   {
      if( resultInfo[index][FIELD_ERRNO] != SDB_OK )
      {
         var error = new SdbError( resultInfo[index][FIELD_ERRNO],
                                   "Task failed" ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;
      }
   }

   PD_LOGGER.logTask( PDEVENT, "Finish check result" ) ;

   return result ;
}

function Rollback()
{
   return BUS_JSON[FIELD_RESULTINFO] ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiasql-oltp.js" ) ;
   var taskID = 0 ;
   var result = {} ;

   taskID = BUS_JSON[FIELD_TASKID] ;

   PD_LOGGER.setTaskId( taskID ) ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Step [?]", SYS_STEP ) ) ;

   if( SYS_STEP == STEP_GENERATE_PLAN )
   {
      result = GeneratePlan( taskID ) ;
   }
   else if( SYS_STEP == STEP_DOIT )
   {
      if( BUS_JSON[FIELD_CMD] == "install" )
      {
         result = SendPackage( taskID ) ;
         if ( SDB_OK == result[FIELD_ERRNO] )
         {
            result = InstallPackage( taskID ) ;
         }
      }
      else if( BUS_JSON[FIELD_CMD] == "skip" )
      {
         result = Skip() ;
      }
      else if( BUS_JSON[FIELD_CMD] == "error" )
      {
         result = Error() ;
      }
      else
      {
         var error = new SdbError( SDB_INVALIDARG,
                                   sprintf( "Unknow command [?]",
                                            BUS_JSON[FIELD_CMD] ) ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;
         throw error ;
      }
   }
   else if( SYS_STEP == STEP_CHECK_RESULT )
   {
      result = CheckResult( taskID ) ;
   }
   else if( SYS_STEP == STEP_ROLLBACK )
   {
      result = Rollback( taskID ) ;
   }
   else
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Unknow step [?]", SYS_STEP ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   return result ;
}
