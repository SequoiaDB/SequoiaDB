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
@description: add sequoiasql-oltp business
@modify list:
   2017-09-25 JiaWen He  Init

1. Generate plan
   @parameter
      var SYS_STEP = "Generate plan" ;
      var BUS_JSON = {"TaskID":90,"Type":2,"TypeDesc":"ADD_BUSINESS","TaskName":"ADD_BUSINESS","Status":0,"StatusDesc":"INIT","AgentHost":"ubuntu-jw-01","AgentService":"11790","Info":{"SdbUser":"sdbadmin","SdbPasswd":"sdbadmin","SdbUserGroup":"sdbadmin_group","ClusterName":"myCluster1","BusinessType":"sequoiasql-oltp","BusinessName":"myModule1","DeployMod":"","Config":[{"HostName":"ubuntu-jw-01","dbpath":"/sequoiasql-oltp/database/5432","port":"5432","shared_buffers":"128MB","log_timezone":"PRC","datestyle":"iso, ymd","timezone":"PRC","lc_messages":"zh_CN.UTF-8","lc_monetary":"zh_CN","lc_numeric":"zh_CN","lc_time":"zh_CN","default_text_search_config":"pg_catalog.simple","InstallPath":"/opt/sequoiasqloltp"}]},"errno":0,"detail":"","Progress":0,"ResultInfo":[{"HostName":"ubuntu-jw-01","port":"5432","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]}
   @return
      RET_JSON: the format is: {"Plan":[[{"TaskID":90,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiasql-oltp","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-01","dbpath":"/sequoiasql-oltp/database/5432","port":"5432","shared_buffers":"128MB","log_timezone":"PRC","datestyle":"iso, ymd","timezone":"PRC","lc_messages":"zh_CN.UTF-8","lc_monetary":"zh_CN","lc_numeric":"zh_CN","lc_time":"zh_CN","default_text_search_config":"pg_catalog.simple","InstallPath":"/opt/sequoiasqloltp","AgentService":"11790"}},"ResultInfo":{"HostName":"ubuntu-jw-01","port":"5432","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":90}}]]}

2. create instance
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"TaskID":90,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiasql-oltp","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-01","dbpath":"/sequoiasql-oltp/database/5432","port":"5432","shared_buffers":"128MB","log_timezone":"PRC","datestyle":"iso, ymd","timezone":"PRC","lc_messages":"zh_CN.UTF-8","lc_monetary":"zh_CN","lc_numeric":"zh_CN","lc_time":"zh_CN","default_text_search_config":"pg_catalog.simple","InstallPath":"/opt/sequoiasqloltp","AgentService":"11790"}},"ResultInfo":{"HostName":"ubuntu-jw-01","port":"5432","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":90}}

   @return
      RET_JSON: the format is: {"HostName":"ubuntu-jw-02","Status":4,"StatusDesc":"FINISH","errno":0,"detail":"","Flow":[],"Progress":90}

3. Check result
   @parameter
      var SYS_STEP = "Check result" ;
      var BUS_JSON = {"errno":0,"detail":"","Progress":15,"ResultInfo":[{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":["Installing coord[ubuntu-jw-02:11840]","Successfully create coord[ubuntu-jw-02:11840]"]},{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}],"TaskID":25,"Info":{"Config":[{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"","dbpath":"/opt/sequoiadb/database/catalog/11840","svcname":"11840","role":"catalog","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"group1","dbpath":"/opt/sequoiadb/database/data/11850","svcname":"11850","role":"data","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","DeployMod":"vertical"}} ;
   @return
      RET_JSON: the format is: {"errno":0,"detail":""}
*/

function _getAgentPort( hostName )
{
   return Oma.getAOmaSvcName( hostName ) ;
}

function GeneratePlan( PD_LOGGER )
{
   var taskID        = BUS_JSON[FIELD_TASKID] ;
   var plan          = {} ;
   var planInfo      = BUS_JSON[FIELD_INFO] ;
   var resultInfo    = BUS_JSON[FIELD_RESULTINFO] ;
   var installTask   = [] ;
   var clusterName   = planInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = planInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = planInfo[FIELD_BUSINESS_NAME] ;
   var hostNum       = 0 ;
   var progressStep  = 0 ;

   plan[FIELD_PLAN] = [] ;

   if( isTypeOf( planInfo[FIELD_CONFIG], "object" ) == false )
   {
      var error = new SdbError( SDB_SYS,
                                sprintf( "Invalid argument, ? is not object",
                                         FIELD_CONFIG ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   hostNum = planInfo[FIELD_CONFIG].length ;
   progressStep = parseInt( 90 / hostNum ) ;

   for( var index in planInfo[FIELD_CONFIG] )
   {
      var installConfig = {} ;
      var config    = planInfo[FIELD_CONFIG][index] ;
      var hostName  = config[FIELD_HOSTNAME] ;
      var agentPort = _getAgentPort( hostName ) ;

      installConfig[FIELD_TASKID] = taskID ;
      installConfig[FIELD_INFO]   = {} ;
      installConfig[FIELD_INFO][FIELD_CLUSTER_NAME] = clusterName ;
      installConfig[FIELD_INFO][FIELD_BUSINESS_TYPE] = businessType ;
      installConfig[FIELD_INFO][FIELD_BUSINESS_NAME] = businessName ;
      installConfig[FIELD_INFO][FIELD_CONFIG] = config ;
      installConfig[FIELD_INFO][FIELD_CONFIG][FIELD_AGENT_SERVICE] = agentPort ;
      installConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      installConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;

      installTask.push( installConfig ) ;
   }

   plan[FIELD_PLAN].push( installTask ) ;

   PD_LOGGER.logTask( PDEVENT, "Finish generate plan" ) ;

   return plan ;
}

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

function _runRemoteCmd( cmd, command, arg, timeout )
{
   var error = null ;

   try
   {
      cmd.run( command, arg, timeout ) ;
   }
   catch( e )
   {
      var rc = cmd.getLastRet() ;
      var out = cmd.getLastOut() ;

      if( rc )
      {
         error = new SdbError( rc, out ) ;
      }
      else
      {
         if( typeof( e ) == "number" )
         {
            error = new SdbError( e, "failed to exec cmd" ) ;
         }
         else
         {
            error = new SdbError( SDB_SYS, "failed to exec cmd." ) ;
         }
      }
   }

   return error ;
}

//'/opt/sequoiasqloltp/database/5432/postgresql.conf'
function _setPostgresqlConf( remote, hostName, confPath, configs )
{
   var file    = null ;
   var error   = null ;

   try
   {
      file = remote.getFile( confPath, 0,
                             SDB_FILE_REPLACE | SDB_FILE_WRITEONLY ) ;

      file.write( 'listen_addresses = \'*\'\n' ) ;

      for ( key in configs )
      {
         if( FIELD_HOSTNAME != key &&
             FIELD_DBPATH   != key &&
             FIELD_INSTALL_PATH != key &&
             FIELD_AGENT_SERVICE != key )
         {
            if ( configs[key].length > 0 )
            {
               if ( isNaN( configs[key] ) == true )
               {
                  file.write( key + ' = \'' + configs[key] + '\'\n' ) ;
               }
               else
               {
                  file.write( key + ' = ' + configs[key] + '\n' ) ;
               }
            }
         }
      }
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed to set config: " +
                                     "host [?]", hostName ) ) ;
   }

   return error ;
}

function _getLogMessage( PD_LOGGER, cmd, installPath, businessName )
{
   var error = null ;
   var logFile = installPath + '/' + businessName + '.log' ;
   var timeout = 600000 ;

   error = _runRemoteCmd( cmd, 'cat', logFile, timeout ) ;
   if ( error !== null )
   {
      PD_LOGGER.logTask( PDERROR, error ) ;
      return null ;
   }

   return cmd.getLastOut() ;
}

function CreateInst( PD_LOGGER )
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var config     = taskInfo[FIELD_CONFIG] ;

   var clusterName   = taskInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = taskInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = taskInfo[FIELD_BUSINESS_NAME] ;

   var hostName      = config[FIELD_HOSTNAME] ;
   var dbpath        = config[FIELD_DBPATH] ;
   var agentPort     = config[FIELD_AGENT_SERVICE] ;
   var installPath   = config[FIELD_INSTALL_PATH] ;
   var ctlFile       = installPath + '/bin/sdb_sql_ctl' ;

   var error   = null ;
   var remote  = null ;
   var cmd     = null ;
   var exec    = ctlFile ;
   var args    = '' ;
   var timeout = 600000 ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to create instance [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to create instance [?]",
                                         hostName ) ) ;

   try
   {
      remote = new Remote( hostName, agentPort ) ;
      cmd    = remote.getCmd() ;
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

   //set LD_LIBRARY_PATH
   //export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/sequoiasqloltp/lib
   var libraryCmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' ;
   libraryCmd += installPath + '/lib ;' ;
   exec = libraryCmd + exec ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to add instance [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to add instance [?]",
                                         hostName ) ) ;

   //add inst
   args = '' ;
   args += ' addinst ' + businessName ;
   args += ' -D ' + dbpath ;
   error = _runRemoteCmd( cmd, exec, args, timeout ) ;
   if ( error !== null )
   {
      var logMsg = _getLogMessage( PD_LOGGER, cmd, installPath, businessName ) ;
      if( logMsg !== null )
      {
         PD_LOGGER.logTask( PDERROR, logMsg ) ;
      }

      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to set instance config [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to set instance config [?]",
                                         hostName ) ) ;

   //change config
   // '/opt/sequoiasqloltp/database/5432/postgresql.conf'
   var confPath = dbpath + '/postgresql.conf' ;
   error = _setPostgresqlConf( remote, hostName, confPath, config ) ;
   if ( error !== null )
   {
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to start instance [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to start instance [?]",
                                         hostName ) ) ;

   //start
   args = '' ;
   args += ' start ' + businessName ;
   error = _runRemoteCmd( cmd, exec, args, timeout ) ;
   if ( error !== null )
   {
      var logMsg = _getLogMessage( PD_LOGGER, cmd, installPath, businessName ) ;
      if( logMsg !== null )
      {
         PD_LOGGER.logTask( PDERROR, logMsg ) ;
      }

      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Finish to create instance [?]",
                                        hostName ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Finish to create instance [?]",
                                         hostName ) ) ;

   return resultInfo ;
}

function CheckResult( PD_LOGGER )
{
   var result = new commonResult() ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;

   for( var index in resultInfo )
   {
      if( resultInfo[index][FIELD_ERRNO] != SDB_OK )
      {
         var error = new SdbError( resultInfo[index][FIELD_ERRNO],
                                   "Task failed" ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;
         throw error ;
      }
   }

   PD_LOGGER.logTask( PDEVENT, "Finish check result" ) ;

   return result ;
}

function Rollback( PD_LOGGER )
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var configs    = taskInfo[FIELD_CONFIG] ;

   var clusterName   = taskInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = taskInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = taskInfo[FIELD_BUSINESS_NAME] ;

   var result     = [] ;
   var newResultInfo = {} ;

   newResultInfo[FIELD_RESULTINFO] = result ;

   for( var index in resultInfo )
   {
      var nodeResult    = resultInfo[index] ;
      var hostName      = nodeResult[FIELD_HOSTNAME] ;
      var agentPort     = _getAgentPort( hostName ) ;
      var installPath   = configs[index][FIELD_INSTALL_PATH] ;
      var ctlFile       = installPath + '/bin/sdb_sql_ctl' ;

      var error   = null ;
      var remote  = null ;
      var cmd     = null ;
      var exec    = ctlFile ;
      var args    = '' ;
      var timeout = 600000 ;

      nodeResult[FIELD_FLOW] = [] ;

      PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to rollback instance [?]",
                                           hostName ) ) ;
      nodeResult[FIELD_FLOW].push( sprintf( "Begin to rollback instance [?]",
                                            hostName ) ) ;

      try
      {
         remote = new Remote( hostName, agentPort ) ;
         cmd    = remote.getCmd() ;
      }
      catch( e )
      {
         error = _getErrorMsg( getLastError(), e,
                               sprintf( "Failed to get remote file obj: " +
                                        "host [?:?]",
                                        hostName, agentPort ) ) ;
         nodeResult[FIELD_ERRNO]  = error.getErrCode() ;
         nodeResult[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
         nodeResult[FIELD_STATUS] = STATUS_FAIL ;
         nodeResult[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
         nodeResult[FIELD_FLOW].push( error.getErrMsg() ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;

         PD_LOGGER.logTask( PDERROR, sprintf( "Failed to rollback instance [?]",
                                              hostName ) ) ;
         nodeResult[FIELD_FLOW].push( sprintf( "Failed to rollback instance [?]",
                                               hostName ) ) ;
         result.push( nodeResult ) ;
         continue ;
      }

      //set LD_LIBRARY_PATH
      //export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/sequoiasqloltp/lib
      var libraryCmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' ;
      libraryCmd += installPath + '/lib ;' ;
      exec = libraryCmd + exec ;

      /*
      3     fail to initdb
      5     fail to create directory or file
      8     instance exists
      */
      if( 3 != nodeResult[FIELD_ERRNO] &&
          5 != nodeResult[FIELD_ERRNO] &&
          8 != nodeResult[FIELD_ERRNO] )
      {
         PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to delete instance [?]",
                                              hostName ) ) ;
         nodeResult[FIELD_FLOW].push( sprintf( "Begin to delete instance [?]",
                                               hostName ) ) ;

         //del inst
         args = '' ;
         args += ' delinst ' + businessName ;
         error = _runRemoteCmd( cmd, exec, args, timeout ) ;
         if ( error !== null )
         {
            nodeResult[FIELD_ERRNO]  = error.getErrCode() ;
            nodeResult[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
            nodeResult[FIELD_STATUS] = STATUS_FAIL ;
            nodeResult[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
            nodeResult[FIELD_FLOW].push( error.getErrMsg() ) ;
            PD_LOGGER.logTask( PDERROR, error ) ;

            PD_LOGGER.logTask( PDERROR, sprintf( "Failed to rollback instance [?]",
                                                 hostName ) ) ;
            nodeResult[FIELD_FLOW].push( sprintf( "Failed to rollback instance [?]",
                                                  hostName ) ) ;
            result.push( nodeResult ) ;
            continue ;
         }
      }

      PD_LOGGER.logTask( PDEVENT, sprintf( "Finish to rollback instance [?]",
                                           hostName ) ) ;
      nodeResult[FIELD_FLOW].push( sprintf( "Finish to rollback instance [?]",
                                            hostName ) ) ;

      result.push( nodeResult ) ;
   }

   newResultInfo[FIELD_RESULTINFO] = result ;

   return newResultInfo ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiasql-oltp.js" ) ;
   var taskID = 0 ;
   var result = {} ;

   taskID = BUS_JSON[TaskID] ;

   PD_LOGGER.setTaskId( taskID ) ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Step [?]", SYS_STEP ) ) ;

   if( SYS_STEP == STEP_GENERATE_PLAN )
   {
      result = GeneratePlan( PD_LOGGER ) ;
   }
   else if( SYS_STEP == STEP_DOIT )
   {
      result = CreateInst( PD_LOGGER ) ;
   }
   else if( SYS_STEP == STEP_CHECK_RESULT )
   {
      result = CheckResult( PD_LOGGER ) ;
   }
   else if( SYS_STEP == STEP_ROLLBACK )
   {
      result = Rollback( PD_LOGGER ) ;
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
