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
@description: restart sequoiasql-postgresql business
@modify list:
   2018-12-13 JiaWen He  Init

1. Generate plan
   @parameter
      var SYS_STEP = "Generate plan" ;
      var BUS_JSON = {"AgentHost":"ubuntu-jw-01","AgentService":"11790","CreateTime":{"$timestamp":"2018-12-13-11.13.53.000000"},"EndTime":{"$timestamp":"2018-12-13-11.13.54.000000"},"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiasql-postgresql","BusinessName":"myModule3","Config":[{"HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/"}]},"Progress":100,"ResultInfo":[{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/","errno":0,"detail":"","Flow":[]}],"Status":4,"StatusDesc":"FINISH","TaskID":36,"TaskName":"RESTART_BUSINESS","Type":8,"TypeDesc":"RESTART_BUSINESS","_id":{"$oid":"5c11ce714430570bf1dc2dcd"},"detail":"","errno":0}
   @return
      RET_JSON: the format is: {"Plan":[[{"TaskID":90,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiasql-postgresql","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/"}},"ResultInfo":{"HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":90}}]]}

2. create instance
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"TaskID":90,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiasql-postgresql","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/"}},"ResultInfo":{"HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":90}}

   @return
      RET_JSON: the format is: {"HostName":"ubuntu-jw-02","port":"5432","Status":4,"StatusDesc":"FINISH","errno":0,"detail":"","Flow":[],"Progress":90}

3. Check result
   @parameter
      var SYS_STEP = "Check result" ;
      var BUS_JSON = {"AgentHost":"ubuntu-jw-01","AgentService":"11790","CreateTime":{"$timestamp":"2018-12-13-11.13.53.000000"},"EndTime":{"$timestamp":"2018-12-13-11.13.54.000000"},"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiasql-postgresql","BusinessName":"myModule3","Config":[{"HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/"}]},"Progress":100,"ResultInfo":[{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-02","port":"5432","InstallPath":"/opt/sequoiasql/postgresql/","errno":0,"detail":"","Flow":[]}],"Status":4,"StatusDesc":"FINISH","TaskID":36,"TaskName":"RESTART_BUSINESS","Type":8,"TypeDesc":"RESTART_BUSINESS","_id":{"$oid":"5c11ce714430570bf1dc2dcd"},"detail":"","errno":0}
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
   var clusterName   = planInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = planInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = planInfo[FIELD_BUSINESS_NAME] ;
   var nodeNum       = 0 ;
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

   nodeNum = planInfo[FIELD_CONFIG].length ;
   progressStep = parseInt( 90 / nodeNum ) ;

   var planTask = [] ;

   for( var index in planInfo[FIELD_CONFIG] )
   {
      var planConfig = {} ;
      var config    = planInfo[FIELD_CONFIG][index] ;
      var hostName  = config[FIELD_HOSTNAME] ;
      var agentPort = _getAgentPort( hostName ) ;

      planConfig[FIELD_TASKID] = taskID ;
      planConfig[FIELD_INFO]   = {} ;
      planConfig[FIELD_INFO][FIELD_CLUSTER_NAME] = clusterName ;
      planConfig[FIELD_INFO][FIELD_BUSINESS_TYPE] = businessType ;
      planConfig[FIELD_INFO][FIELD_BUSINESS_NAME] = businessName ;
      planConfig[FIELD_INFO][FIELD_CONFIG] = config ;
      planConfig[FIELD_INFO][FIELD_CONFIG][FIELD_AGENT_SERVICE] = agentPort ;
      planConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      planConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;

      planTask.push( planConfig ) ;
   }

   plan[FIELD_PLAN] = [ planTask ] ;

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

function RestartModule( PD_LOGGER )
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var config     = taskInfo[FIELD_CONFIG] ;

   var clusterName   = taskInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = taskInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = taskInfo[FIELD_BUSINESS_NAME] ;

   var hostName      = config[FIELD_HOSTNAME] ;
   var installPath   = config[FIELD_INSTALL_PATH] ;
   var agentPort     = config[FIELD_AGENT_SERVICE] ;
   var ctlFile       = installPath + '/bin/sdb_sql_ctl' ;

   var error   = null ;
   var remote  = null ;
   var cmd     = null ;
   var exec    = ctlFile ;
   var args    = '' ;
   var timeout = 600000 ;

   PD_LOGGER.logTask( PDEVENT, "Begin to restart" ) ;
   resultInfo[FIELD_FLOW].push( "Begin to restart" ) ;

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

   var libraryCmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' ;
   libraryCmd += installPath + '/lib ;' ;
   exec = libraryCmd + exec ;

   //restart
   args = ' restart ' + businessName + ' --print' ;
   error = _runRemoteCmd( cmd, exec, args, timeout ) ;
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

   resultInfo[FIELD_STATUS] = STATUS_FINISH ;
   resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;

   PD_LOGGER.logTask( PDEVENT, "Finish to restart" ) ;
   resultInfo[FIELD_FLOW].push( "Finish to restart" ) ;

   return resultInfo ;
}

function CheckResult( PD_LOGGER )
{
   var result = new commonResult() ;

   PD_LOGGER.logTask( PDEVENT, "Finish check result" ) ;

   return result ;
}

function _rollbackNode( PD_LOGGER, businessName, config, resultInfo )
{
   var hostName      = config[FIELD_HOSTNAME] ;
   var installPath   = config[FIELD_INSTALL_PATH] ;
   var agentPort     = config[FIELD_AGENT_SERVICE] ;
   var ctlFile       = installPath + '/bin/sdb_sql_ctl' ;

   var error   = null ;
   var remote  = null ;
   var cmd     = null ;
   var exec    = ctlFile ;
   var args    = '' ;
   var timeout = 600000 ;

   PD_LOGGER.logTask( PDEVENT, "Begin to rollback" ) ;
   resultInfo[FIELD_FLOW].push( "Begin to rollback" ) ;

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

   var libraryCmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:' ;
   libraryCmd += installPath + '/lib ;' ;
   exec = libraryCmd + exec ;

   //add inst
   args = '' ;
   args += ' start ' + businessName ;
   args += ' --print' ;
   error = _runRemoteCmd( cmd, exec, args, timeout ) ;
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

   PD_LOGGER.logTask( PDEVENT, "Finish to rollback" ) ;
   resultInfo[FIELD_FLOW].push( "Finish to rollback" ) ;

   return resultInfo ;
}

function Rollback( PD_LOGGER )
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var config     = taskInfo[FIELD_CONFIG] ;

   var clusterName   = taskInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = taskInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = taskInfo[FIELD_BUSINESS_NAME] ;

   for( var index in config )
   {
      resultInfo[index] = _rollbackNode( PD_LOGGER,
                                         businessName,
                                         config[index],
                                         resultInfo[index] ) ;
   }

   return resultInfo ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiasql-postgresql.js" ) ;
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
      result = RestartModule( PD_LOGGER ) ;
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
