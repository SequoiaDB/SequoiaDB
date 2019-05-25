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
@description: shrink business
@modify list:
   2017-08-14 JiaWen He  Init

1. Generate plan
   @parameter
      var SYS_STEP = "Generate plan" ;
      var BUS_JSON = {"_id":{"$oid":"599106592cf47fa6e5a140e5"},"TaskID":3,"Type":6,"TypeDesc":"SHRINK_BUSINESS","TaskName":"SHRINK_BUSINESS","CreateTime":{"$timestamp":"2017-08-14-10.09.29.000000"},"EndTime":{"$timestamp":"2017-08-14-10.09.29.000000"},"Status":0,"StatusDesc":"INIT","AgentHost":"ubuntu-jw-01","AgentService":"11790","Info":{"Config":[{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord","datagroupname":""},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"catalog","datagroupname":""},{"HostName":"ubuntu-jw-03","svcname":"11830","role":"data","datagroupname":"group1"}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"},{"HostName":"ubuntu-jw-02","svcname":"11810"},{"HostName":"ubuntu-jw-03","svcname":"11810"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessName":"myModule1","BusinessType":"sequoiadb","DeployMod":"distribution"},"errno":0,"detail":"","Progress":0,"ResultInfo":[{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-02","svcname":"11820","role":"catalog","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-03","svcname":"11830","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]} ;

   @return
      RET_JSON: the format is: {"Plan":[[{"TaskID":3,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"},{"HostName":"ubuntu-jw-02","svcname":"11810"},{"HostName":"ubuntu-jw-03","svcname":"11810"}],"User":"","Passwd":"","Info":{"BusinessType":"sequoiadb"},"Config":{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord","datagroupname":""},"ResultInfo":{"HostName":"ubuntu-jw-01","svcname":"11810","role":"coord","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":30}}],[{"TaskID":3,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"},{"HostName":"ubuntu-jw-02","svcname":"11810"},{"HostName":"ubuntu-jw-03","svcname":"11810"}],"User":"","Passwd":"","Info":{"BusinessType":"sequoiadb"},"Config":{"HostName":"ubuntu-jw-02","svcname":"11820","role":"catalog","datagroupname":""},"ResultInfo":{"HostName":"ubuntu-jw-02","svcname":"11820","role":"catalog","datagroupname":"","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":30}}],[{"TaskID":3,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"},{"HostName":"ubuntu-jw-02","svcname":"11810"},{"HostName":"ubuntu-jw-03","svcname":"11810"}],"User":"","Passwd":"","Info":{"BusinessType":"sequoiadb"},"Config":{"HostName":"ubuntu-jw-03","svcname":"11830","role":"data","datagroupname":"group1"},"ResultInfo":{"HostName":"ubuntu-jw-03","svcname":"11830","role":"data","datagroupname":"group1","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":30}}]]}

2. remove node
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"cmd":"create node","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","Config":{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},"ResultInfo":{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}} ;

   @return
      RET_JSON: the format is: {"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":["Installing coord[ubuntu-jw-02:11840]","Successfully create coord[ubuntu-jw-02:11840]"],"Progress":15}

4. Check result
   @parameter
      var SYS_STEP = "Check result" ;
      var BUS_JSON = {"errno":0,"detail":"","Progress":15,"ResultInfo":[{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":["Installing coord[ubuntu-jw-02:11840]","Successfully create coord[ubuntu-jw-02:11840]"]},{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}],"TaskID":25,"Info":{"Config":[{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"","dbpath":"/opt/sequoiadb/database/catalog/11840","svcname":"11840","role":"catalog","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"group1","dbpath":"/opt/sequoiadb/database/data/11850","svcname":"11850","role":"data","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","DeployMod":"vertical"}} ;
   @return
      RET_JSON: the format is: {"errno":0,"detail":""}

*/

var PD_LOGGER = new Logger( "shrinkBusiness.js" ) ;

function _connectCoord( coordList, user, passwd )
{
   var rc = SDB_OK ;
   var db = null ;

   for( var index in coordList )
   {
      try
      {
         rc = SDB_OK ;
         PD_LOGGER.logTask( PDEVENT, sprintf( "Connect coord[?:?]",
                                              coordList[index][FIELD_HOSTNAME],
                                              coordList[index][FIELD_SVCNAME] ) ) ;
         db = new Sdb( coordList[index][FIELD_HOSTNAME],
                       coordList[index][FIELD_SVCNAME],
                       user, passwd ) ;
         break ;
      }
      catch( e )
      {
         rc = getLastError() ;
         if( rc == SDB_OK )
         {
            break ;
         }
      }
   }

   if( rc )
   {
      var error = new SdbError( rc, "Failed to connect coord" ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   return db ;
}

function _getGroup( db, role, dataGroupName )
{
   var rc = SDB_OK ;
   var i  = 0 ;
   var rg = null ;

   for( i = 0; i < OMA_WAIT_CATALOG_TRY_TIMES; ++i )
   {
      try
      {
         if( role == FIELD_COORD )
         {
            rg = db.getCoordRG() ;
         }
         else if( role == FIELD_CATALOG )
         {
            rg = db.getCatalogRG() ;
         }
         else if( role == FIELD_DATA )
         {
            rg = db.getRG( dataGroupName ) ;
         }
         break ;
      }
      catch( e )
      {
         rc = getLastError() ;
         if( rc == SDB_CLS_GRP_NOT_EXIST && role == FIELD_DATA )
         {
            var error = new SdbError( rc, "Failed to get data group" ) ;
            throw error ;
         }
         else if( rc == SDB_CLS_NOT_PRIMARY )
         {
            PD_LOGGER.logTask( PDWARNING, "Catalog has no primary" ) ;
            sleep( 1000 ) ;
            continue ;
         }
         else if( rc )
         {
            var error = new SdbError( rc, "Failed to get data group" ) ;
            PD_LOGGER.logTask( PDERROR, error ) ;
            throw error ;
         }
      }
   }

   if( i == OMA_WAIT_CATALOG_TRY_TIMES )
   {
      var error = new SdbError( SDB_CLS_NOT_PRIMARY,
                                "Catalog has no primary" ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   return rg ;
}

function _getGroupIndex( groupList, groupName )
{
   var newIndex = -1 ;

   for( var index in groupList )
   {
      if( groupList[index].indexOf( groupName ) < 0 )
      {
         newIndex = index ;
         groupList[index].push( groupName ) ;
         break ;
      }
   }

   if( newIndex < 0 )
   {
      newIndex = groupList.length ;
      groupList.push( [ groupName ] ) ;
   }

   return newIndex ;
}

function GeneratePlan( taskID )
{
   var plan = {} ;
   var planInfo    = BUS_JSON[FIELD_INFO] ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var coordList   = planInfo[FIELD_COORD2] ;
   var user        = planInfo[FIELD_USER] ;
   var passwd      = planInfo[FIELD_PASSWD] ;
   var coordTask   = [] ;
   var catalogTask = [] ;
   var dataTask    = [] ;
   var dataList    = [] ;
   var groupList   = [] ;
   var nodeNum     = 0 ;
   var progressStep = 0 ;

   plan[FIELD_PLAN] = [] ;

   if( isTypeOf( planInfo[FIELD_CONFIG], "object" ) == false )
   {
      var error = new SdbError( SDB_CLS_NOT_PRIMARY,
                                sprintf( "Invalid argument, ? is not object",
                                         FIELD_CONFIG ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   nodeNum = planInfo[FIELD_CONFIG].length ;

   progressStep = parseInt( 90 / nodeNum ) ;

   for( var index in planInfo[FIELD_CONFIG] )
   {
      var taskConfig = {} ;
      var nodeConfig = planInfo[FIELD_CONFIG][index] ;
      var businessInfo = {} ;
      businessInfo[FIELD_BUSINESS_TYPE] = planInfo[FIELD_BUSINESS_TYPE] ;

      taskConfig[FIELD_TASKID] = taskID ;
      taskConfig[FIELD_COORD2] = coordList ;
      taskConfig[FIELD_USER]   = user ;
      taskConfig[FIELD_PASSWD] = passwd ;
      taskConfig[FIELD_INFO]   = businessInfo ;
      taskConfig[FIELD_CONFIG] = nodeConfig ;
      taskConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      taskConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;

      if( nodeConfig[FIELD_ROLE] == FIELD_COORD )
      {
         coordTask.push( [ taskConfig ] ) ;
      }
      else if( nodeConfig[FIELD_ROLE] == FIELD_CATALOG )
      {
         catalogTask.push( [ taskConfig ] ) ;
      }
      else if( nodeConfig[FIELD_ROLE] == FIELD_DATA )
      {
         dataList.push( taskConfig ) ;
      }
   }

   for( var index in dataList )
   {
      var groupName = dataList[index][FIELD_CONFIG][FIELD_DATAGROUPNAME] ;
      var newIndex  = _getGroupIndex( groupList, groupName ) ;

      if( dataTask.length <= newIndex )
      {
         dataTask.push( [] ) ;
      }
      dataTask[newIndex].push( dataList[index] ) ;
   }

   plan[FIELD_PLAN] = plan[FIELD_PLAN].concat( coordTask ) ;
   plan[FIELD_PLAN] = plan[FIELD_PLAN].concat( catalogTask ) ;
   plan[FIELD_PLAN] = plan[FIELD_PLAN].concat( dataTask ) ;

   PD_LOGGER.logTask( PDEVENT, "Finish generate plan" ) ;

   return plan ;
}

function CheckResult()
{
   var result = new commonResult() ;

   PD_LOGGER.logTask( PDEVENT, "Finish check result" ) ;

   return result ;
}

function Rollback( taskID )
{
   var taskInfo    = BUS_JSON[FIELD_INFO] ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var result      = [] ;
   var newResultInfo = {} ;

   newResultInfo[FIELD_RESULTINFO] = result ;

   for( var index in resultInfo )
   {
      var nodeResult = resultInfo[index] ;

      nodeResult[FIELD_FLOW] = [] ;
      nodeResult[FIELD_STATUS] = STATUS_FINISH ;
      nodeResult[FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;
      result.push( nodeResult ) ;
   }

   newResultInfo[FIELD_RESULTINFO] = result ;

   return newResultInfo ;
}

function _removeNode( taskID )
{
   var rc = SDB_OK ;
   var coordList   = BUS_JSON[FIELD_COORD2] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var nodeConfig  = BUS_JSON[FIELD_CONFIG] ;
   var result      = BUS_JSON[FIELD_RESULTINFO] ;
   var role        = nodeConfig[FIELD_ROLE] ;
   var hostName    = nodeConfig[FIELD_HOSTNAME] ;
   var svcname     = nodeConfig[FIELD_SVCNAME] ;
   var groupName   = nodeConfig[FIELD_DATAGROUPNAME] ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Remove node ?[?:?]",
                                        role, hostName, svcname ) ) ;

   result[FIELD_FLOW].push( sprintf( "Remove node ?[?:?]",
                                      role, hostName, svcname ) ) ;

   db = _connectCoord( coordList, user, passwd ) ;

   try
   {
      rg = _getGroup( db, role, groupName ) ;
   }
   catch( e )
   {
      if( e.getErrCode() == SDB_CLS_GRP_NOT_EXIST )
      {
         PD_LOGGER.logTask( PDEVENT, sprintf( "Finish remove node ?[?:?]",
                                              role, hostName, svcname ) ) ;

         result[FIELD_FLOW].push( sprintf( "Finish remove node ?[?:?]",
                                           role, hostName, svcname ) ) ;
         return result ;
      }
      else
      {
         throw e ;
      }
   }

   cur = rg.getDetail() ;
   detail = cur.next().toObj() ;
  
   if( detail[FIELD_GROUP].length > 1 )
   {
      while( true )
      {
         try
         {
            rg.removeNode( hostName, svcname, { "enforced": true } ) ;
            break ;
         }
         catch( e )
         {
            rc = getLastError() ;
            if( rc == SDB_CATA_RM_CATA_FORBIDDEN )
            {
               var options = {} ;

               PD_LOGGER.logTask( PDWARNING,
                               sprintf( "?:? is primary node, try to reelect",
                                        hostName, svcname ) ) ;
               options[FIELD_SECONDS] = 60 ;
               rg.reelect( options ) ;
               continue ;
            }
            if( rc != SDB_CLS_NODE_NOT_EXIST )
            {
               var error = new SdbError( rc, sprintf( "Failed to remove node[?:?]",
                                                      hostName, svcname ) ) ;
               PD_LOGGER.logTask( PDERROR, error ) ;
               throw error ;
            }
            else
            {
               PD_LOGGER.logTask( PDWARNING, sprintf( "?:? not exist",
                                                      hostName, svcname ) ) ;
               break ;
            }
         }
      }
   }

   if( detail[FIELD_GROUP].length == 1 || detail[FIELD_GROUP].length == 0 )
   {
      if( role == FIELD_DATA )
      {
         try
         {
            db.removeRG( groupName ) ;
         }
         catch( e )
         {
            rc = getLastError() ;
            var error = new SdbError( rc, sprintf( "Failed to remove Group[?]",
                                                   groupName ) ) ;
            PD_LOGGER.logTask( PDERROR, error ) ;
            throw error ;
         }
      }
   }

   PD_LOGGER.logTask( PDEVENT, sprintf( "Finish remove node ?[?:?]",
                                        role, hostName, svcname ) ) ;

   result[FIELD_FLOW].push( sprintf( "Finish remove node ?[?:?]",
                                      role, hostName, svcname ) ) ;

   return result ;
}

function RemoveNodes( taskID )
{
   var nodeConfig  = BUS_JSON[FIELD_CONFIG] ;
   var result      = BUS_JSON[FIELD_RESULTINFO] ;
   var role        = nodeConfig[FIELD_ROLE] ;
   var hostName    = nodeConfig[FIELD_HOSTNAME] ;
   var svcname     = nodeConfig[FIELD_SVCNAME] ;

   try
   {
      result = _removeNode( taskID ) ;

      result[FIELD_STATUS] = STATUS_FINISH ;
      result[FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;
   }
   catch( e )
   {
      result[FIELD_ERRNO] = e.getErrCode() ;
      result[FIELD_DETAIL] = e.getErrMsg() ;
      result[FIELD_STATUS] = STATUS_FAIL ;
      result[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      result[FIELD_FLOW].push( sprintf( "Failed to remove node ?[?:?]",
                                   role, hostName, svcname ) ) ;
   }

   return result ;
}

function ShrinkSequoiaDB( taskID )
{
   var result = {} ;

   if( SYS_STEP == STEP_GENERATE_PLAN )
   {
      result = GeneratePlan( taskID ) ;
   }
   else if( SYS_STEP == STEP_DOIT )
   {
      result = RemoveNodes( taskID ) ;
   }
   else if( SYS_STEP == STEP_CHECK_RESULT )
   {
      result = CheckResult() ;
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

function main()
{
   var taskID = 0 ;
   var businessType = "" ;
   var result = {} ;

   PD_LOGGER.logComm( PDEVENT, sprintf( "Begin to run shrink business,Step [?]",
                      SYS_STEP ) ) ;

   if( isTypeOf( BUS_JSON[TaskID], "number" ) == false ||
       BUS_JSON[TaskID] <= 0 )
   {
      var error = new SdbError( SDB_INVALIDARG,
                                "TaskID is missed" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   taskID = BUS_JSON[TaskID] ;
   businessType = BUS_JSON[FIELD_INFO][FIELD_BUSINESS_TYPE] ;

   PD_LOGGER.setTaskId( taskID ) ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Step [?]", SYS_STEP ) ) ;

   if( FIELD_SEQUOIADB == businessType )
   {
      result = ShrinkSequoiaDB( taskID ) ;
   }
   else
   {
      var error = new SdbError( SDB_INVALIDARG,
                                sprintf( "Unknow business type [?]",
                                         businessType ) ) ;
      PD_LOGGER.logTask( PDERROR, error ) ;
      throw error ;
   }

   return result ;
}

main() ;
