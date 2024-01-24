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
@description: restart sequoiadb business
@modify list:
   2018-10-24 JiaWen He  Init

1. Generate plan
   @parameter
      var SYS_STEP = "Generate plan" ;
      var BUS_JSON = {"AgentHost":"ubuntu-jw-01","AgentService":"11790","CreateTime":{"$timestamp":"2018-10-25-10.55.50.000000"},"EndTime":{"$timestamp":"2018-10-25-10.55.50.000000"},"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule2","Config":[{"HostName":"ubuntu-jw-02","svcname":"11810","role":"coord","datagroupname":""},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"catalog","datagroupname":""},{"HostName":"ubuntu-jw-02","svcname":"11830","role":"data","datagroupname":"group1"}]},"Progress":0,"ResultInfo":[{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-02","svcname":"11810","errno":0,"detail":"","Flow":[]},{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-02","svcname":"11820","errno":0,"detail":"","Flow":[]},{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-02","svcname":"11830","errno":0,"detail":"","Flow":[]}],"Status":4,"StatusDesc":"FINISH","TaskID":5,"TaskName":"RESTART_BUSINESS","Type":8,"TypeDesc":"RESTART_BUSINESS","_id":{"$oid":"5bd130b6e8abe5ed4c64396e"},"detail":"","errno":0}
   @return
      RET_JSON: the format is: {"Plan":[[{"TaskID":65,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-01","NodeInfo":[{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord","datagroupname":""},{"HostName":"ubuntu-jw-01","svcname":"11820","role":"catalog","datagroupname":""},{"HostName":"ubuntu-jw-01","svcname":"11830","role":"data","datagroupname":"group1"}],"AgentService":"11790"}},"ResultInfo":[{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","svcname":"11820","role":"catalog","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","svcname":"11830","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]},{"TaskID":65,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-02","NodeInfo":[{"HostName":"ubuntu-jw-02","svcname":"11810","role":"coord","datagroupname":""},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"data","datagroupname":"group1"},{"HostName":"ubuntu-jw-02","svcname":"11830","role":"data","datagroupname":"group1"}],"AgentService":"11790"}},"ResultInfo":[{"HostName":"ubuntu-jw-02","svcname":"11810","role":"coord","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-02","svcname":"11830","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]}]]}

2. create instance
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"TaskID":70,"Info":{"ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","Config":{"HostName":"ubuntu-jw-02","NodeInfo":[{"HostName":"ubuntu-jw-02","svcname":"11810","role":"coord","datagroupname":""},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"data","datagroupname":"group1"},{"HostName":"ubuntu-jw-02","svcname":"11830","role":"data","datagroupname":"group1"}],"AgentService":"11790"}},"ResultInfo":[{"HostName":"ubuntu-jw-02","svcname":"11810","role":"coord","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15},{"HostName":"ubuntu-jw-02","svcname":"11830","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}]}

   @return
      RET_JSON: the format is: {"ResultInfo":[{"HostName":"ubuntu-jw-02","svcname":"11810","role":"coord","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},15,{"HostName":"ubuntu-jw-02","svcname":"11820","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-02","svcname":"11830","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]}

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
   var useIndexList = [] ;
   var jobNum = 0 ;

   while( jobNum < nodeNum )
   {
      var planJob = [] ;
      var tmpHostList = [] ;

      for( var index in planInfo[FIELD_CONFIG] )
      {
         var config    = planInfo[FIELD_CONFIG][index] ;
         var hostName  = config[FIELD_HOSTNAME] ;
         var agentPort = _getAgentPort( hostName ) ;
         var jobIndex  = tmpHostList.indexOf( hostName ) ;

         if ( useIndexList.indexOf( index ) >= 0 )
         {
            continue ;
         }
         else if ( jobIndex >= 0 )
         {
            var nodeIndex = planJob[jobIndex][FIELD_RESULTINFO].length ;

            planJob[jobIndex][FIELD_INFO][FIELD_CONFIG][FIELD_NODE_INFO].push( config ) ;
            planJob[jobIndex][FIELD_RESULTINFO].push( resultInfo[index] ) ;
            planJob[jobIndex][FIELD_RESULTINFO][nodeIndex][FIELD_PROGRESS] = progressStep ;

            useIndexList.push( index ) ;
         }
         else
         {
            var planConfig = {} ;

            planConfig[FIELD_TASKID] = taskID ;
            planConfig[FIELD_INFO]   = {} ;
            planConfig[FIELD_INFO][FIELD_CLUSTER_NAME] = clusterName ;
            planConfig[FIELD_INFO][FIELD_BUSINESS_TYPE] = businessType ;
            planConfig[FIELD_INFO][FIELD_BUSINESS_NAME] = businessName ;
            planConfig[FIELD_INFO][FIELD_CONFIG] = {} ;
            planConfig[FIELD_INFO][FIELD_CONFIG][FIELD_HOSTNAME] = hostName ;
            planConfig[FIELD_INFO][FIELD_CONFIG][FIELD_NODE_INFO] = [ config ] ;
            planConfig[FIELD_INFO][FIELD_CONFIG][FIELD_AGENT_SERVICE] = agentPort ;
            planConfig[FIELD_RESULTINFO] = [ resultInfo[index] ] ;
            planConfig[FIELD_RESULTINFO][0][FIELD_PROGRESS] = progressStep ;

            planJob.push( planConfig ) ;

            tmpHostList.push( hostName ) ;
            useIndexList.push( index ) ;
         }

         ++jobNum ;
      }

      planTask.push( planJob ) ;
   }

   plan[FIELD_PLAN] = planTask ;

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

function RestartNode( PD_LOGGER )
{
   var taskInfo   = BUS_JSON[FIELD_INFO] ;
   var resultInfo = BUS_JSON[FIELD_RESULTINFO] ;
   var config     = taskInfo[FIELD_CONFIG] ;

   var clusterName   = taskInfo[FIELD_CLUSTER_NAME] ;
   var businessType  = taskInfo[FIELD_BUSINESS_TYPE] ;
   var businessName  = taskInfo[FIELD_BUSINESS_NAME] ;

   var hostName      = config[FIELD_HOSTNAME] ;
   var agentPort     = config[FIELD_AGENT_SERVICE] ;

   var error   = null ;
   var oma     = null ;

   var nodeList = [] ;

   for( var index in config[FIELD_NODE_INFO] )
   {
      var svcname = config[FIELD_NODE_INFO][index][FIELD_SVCNAME] ;

      PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to restart node [?:?]",
                                           hostName, svcname ) ) ;
      resultInfo[index][FIELD_FLOW].push( sprintf( "Begin to restart node [?:?]",
                                                   hostName, svcname ) ) ;

      nodeList.push( svcname ) ;
   }

   try
   {
      oma = new Oma( hostName, agentPort ) ;
      oma.stopNodes( nodeList ) ;
      oma.startNodes( nodeList ) ;
   }
   catch( e )
   {
      var obj = getLastErrObj() ;

      if ( obj === undefined )
      {
         obj = {} ;

         if ( typeof( e ) == 'number' )
         {
            obj[FIELD_ERRNO] = e ;
         }
         else
         {
            obj[FIELD_ERRNO] = SDB_SYS ;
         }

         obj[FIELD_DESCRIPTION2] = getLastErrMsg() ;
      }
      else
      {
         obj = obj.toObj() ;
      }

      if ( obj[FIELD_ERRNO] == SDB_COORD_NOT_ALL_DONE )
      {
         var errNodes = obj[FIELD_ERR_NODES] ;

         for ( var index in errNodes )
         {
            var nodeErrno   = errNodes[index][FIELD_ERRNO] ;
            var nodeSvcname = errNodes[index][FIELD_SVCNAME] ;
            var desc        = errNodes[index][FIELD_DESCRIPTION2] ;
            var detail      = errNodes[index][FIELD_DETAIL] ;

            for( var index2 in resultInfo )
            {
               if ( resultInfo[index2][FIELD_SVCNAME] == nodeSvcname )
               {
                  resultInfo[index2][FIELD_ERRNO]  = nodeErrno ;
                  resultInfo[index2][FIELD_DETAIL] = desc ;
                  resultInfo[index2][FIELD_STATUS] = STATUS_FAIL ;
                  resultInfo[index2][FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
                  resultInfo[index2][FIELD_FLOW].push( desc ) ;
                  break ;
               }
            }

            PD_LOGGER.logTask( PDERROR, detail ) ;
         }
      }
      else
      {
         var rc   = obj[FIELD_ERRNO] ;

         for( var index in resultInfo )
         {
            var svcname = resultInfo[index][FIELD_SVCNAME] ;
            var desc = obj[FIELD_DESCRIPTION2] ;

            resultInfo[index][FIELD_ERRNO]  = rc ;
            resultInfo[index][FIELD_DETAIL] = desc ;
            resultInfo[index][FIELD_STATUS] = STATUS_FAIL ;
            resultInfo[index][FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
            resultInfo[index][FIELD_FLOW].push( desc ) ;

            PD_LOGGER.logTask( PDERROR, sprintf( "Failed to restart node [?:?], ?",
                               hostName, svcname, desc ) ) ;
         }

         return resultInfo ;
      }
   }

   for( var index in resultInfo )
   {
      if ( resultInfo[index][FIELD_ERRNO] == SDB_OK )
      {
         var svcname = resultInfo[index][FIELD_SVCNAME] ;

         resultInfo[index][FIELD_STATUS] = STATUS_FINISH ;
         resultInfo[index][FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;

         PD_LOGGER.logTask( PDEVENT, sprintf( "Finish to restart node [?:?]",
                                              hostName, svcname ) ) ;
         resultInfo[index][FIELD_FLOW].push( sprintf( "Finish to restart node [?:?]",
                                                      hostName, svcname ) ) ;
      }
   }

   return resultInfo ;
}

function CheckResult( PD_LOGGER )
{
   var result = new commonResult() ;

   PD_LOGGER.logTask( PDEVENT, "Finish check result" ) ;

   return result ;
}

function _rollbackNode( PD_LOGGER, config, resultInfo )
{
   var hostName      = config[FIELD_HOSTNAME] ;
   var svcname       = config[FIELD_SVCNAME] ;
   var agentPort     = config[FIELD_AGENT_SERVICE] ;

   var error   = null ;
   var oma     = null ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Begin to rollback node [?:?]",
                                        hostName, svcname ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Begin to rollback node [?:?]",
                                         hostName, svcname ) ) ;

   try
   {
      oma = new Oma( hostName, agentPort ) ;
      oma.startNode( svcname ) ;
   }
   catch( e )
   {
      error = _getErrorMsg( getLastError(), e,
                            sprintf( "Failed to rollback node [?:?]",
                                     hostName, svcname ) ) ;
      resultInfo[FIELD_ERRNO]  = error.getErrCode() ;
      resultInfo[FIELD_DETAIL] = getErr( error.getErrCode() ) ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( error.getErrMsg() ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      return resultInfo ;
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Finish to rollback node [?:?]",
                                        hostName, svcname ) ) ;
   resultInfo[FIELD_FLOW].push( sprintf( "Finish to rollback node [?:?]",
                                         hostName, svcname ) ) ;

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
      resultInfo[index] = _rollbackNode( PD_LOGGER, config[index],
                                         resultInfo[index] ) ;
   }

   return resultInfo ;
}

function run()
{
   var PD_LOGGER = new Logger( "sequoiadb.js" ) ;
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
      result = RestartNode( PD_LOGGER ) ;
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
