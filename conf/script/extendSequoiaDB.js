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
@description: extend sequoiadb( install coord, catalog, data )
@modify list:
   2017-05-11 JiaWen He  Init

1. Generate plan
   @parameter
      var SYS_STEP = "Generate plan" ;
      var BUS_JSON = {"TaskID":25,"Info":{"Config":[{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"","dbpath":"/opt/sequoiadb/database/catalog/11840","svcname":"11840","role":"catalog","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"group1","dbpath":"/opt/sequoiadb/database/data/11850","svcname":"11850","role":"data","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","DeployMod":"vertical"},"errno":0,"detail":"","Progress":0,"ResultInfo":[{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}]} ;
   @return
      RET_JSON: the format is: {"Plan":[[{"cmd":"create node","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","Config":{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},"ResultInfo":{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}}],[{"cmd":"create node","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","Config":{"HostName":"ubuntu-jw-01","datagroupname":"","dbpath":"/opt/sequoiadb/database/catalog/11840","svcname":"11840","role":"catalog","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},"ResultInfo":{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}}],[{"cmd":"create node","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","Config":{"HostName":"ubuntu-jw-01","datagroupname":"group1","dbpath":"/opt/sequoiadb/database/data/11850","svcname":"11850","role":"data","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},"ResultInfo":{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}}],[{"cmd":"start group","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ResultInfo":{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}},{"cmd":"start group","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ResultInfo":{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}},{"cmd":"start group","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ResultInfo":{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}}]]}

2. Create node
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"cmd":"create node","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","Config":{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},"ResultInfo":{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}} ;
   @return
      RET_JSON: the format is: {"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":["Installing coord[ubuntu-jw-02:11840]","Successfully create coord[ubuntu-jw-02:11840]"],"Progress":15}

3. Start group
   @parameter
      var SYS_STEP = "Doit" ;
      var BUS_JSON = {"cmd":"start group","TaskID":25,"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ResultInfo":{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[],"Progress":15}} ;
   @return
      RET_JSON: the format is: {"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":4,"StatusDesc":"FINISH","errno":0,"detail":"","Flow":["Start coord[ubuntu-jw-02:11840]","Finish install coord[ubuntu-jw-02:11840]"],"Progress":15}

4. Check result
   @parameter
      var SYS_STEP = "Check result" ;
      var BUS_JSON = {"errno":0,"detail":"","Progress":15,"ResultInfo":[{"HostName":"ubuntu-jw-02","datagroupname":"","svcname":"11840","role":"coord","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":["Installing coord[ubuntu-jw-02:11840]","Successfully create coord[ubuntu-jw-02:11840]"]},{"HostName":"ubuntu-jw-01","datagroupname":"","svcname":"11840","role":"catalog","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]},{"HostName":"ubuntu-jw-01","datagroupname":"group1","svcname":"11850","role":"data","Status":0,"StatusDesc":"INIT","errno":0,"detail":"","Flow":[]}],"TaskID":25,"Info":{"Config":[{"HostName":"ubuntu-jw-02","datagroupname":"","dbpath":"/opt/sequoiadb/database/coord/11840","svcname":"11840","role":"coord","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"","dbpath":"/opt/sequoiadb/database/catalog/11840","svcname":"11840","role":"catalog","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""},{"HostName":"ubuntu-jw-01","datagroupname":"group1","dbpath":"/opt/sequoiadb/database/data/11850","svcname":"11850","role":"data","diaglevel":"3","logfilesz":"64","logfilenum":"20","transactionon":"false","preferedinstance":"A","numpreload":"0","maxprefpool":"200","maxreplsync":"10","logbuffsize":"1024","sortbuf":"512","hjbuf":"128","syncstrategy":"keepnormal","weight":"10","maxsyncjob":"10","syncinterval":"10000","syncrecordnum":"0","syncdeep":"false","archiveon":"false","archivecompresson":"true","archivepath":"","archivetimeout":"600","archiveexpired":"240","archivequota":"10","indexpath":"","bkuppath":"","lobpath":"","lobmetapath":""}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","DeployMod":"vertical"}} ;
   @return
      RET_JSON: the format is: {"errno":0,"detail":""}

5. Rollback
   @parameter
      var SYS_STEP = "Rollback" ;
      var BUS_JSON = {"errno":-3,"detail":"Failed to create node, Permission Error","Progress":45,"TaskID":26,"Info":{"Config":[{"HostName":"ubuntu-jw-02","datagroupname":"group4","dbpath":"/sequoiadb/database/data/11850","svcname":"11850","role":"data","archivecompresson":"true","archiveexpired":"240","archiveon":"false","archivepath":"","archivequota":"10","archivetimeout":"600","bkuppath":"","diaglevel":"3","hjbuf":"128","indexpath":"","lobmetapath":"","lobpath":"","logbuffsize":"1024","logfilenum":"20","logfilesz":"64","maxprefpool":"200","maxreplsync":"10","maxsyncjob":"10","numpreload":"0","preferedinstance":"A","sortbuf":"512","syncdeep":"false","syncinterval":"10000","syncrecordnum":"0","syncstrategy":"keepnormal","transactionon":"false","weight":"10"},{"HostName":"ubuntu-jw-03","datagroupname":"group4","dbpath":"/sequoiadb/database/data/11840","svcname":"11840","role":"data","archivecompresson":"true","archiveexpired":"240","archiveon":"false","archivepath":"","archivequota":"10","archivetimeout":"600","bkuppath":"","diaglevel":"3","hjbuf":"128","indexpath":"","lobmetapath":"","lobpath":"","logbuffsize":"1024","logfilenum":"20","logfilesz":"64","maxprefpool":"200","maxreplsync":"10","maxsyncjob":"10","numpreload":"0","preferedinstance":"A","sortbuf":"512","syncdeep":"false","syncinterval":"10000","syncrecordnum":"0","syncstrategy":"keepnormal","transactionon":"false","weight":"10"},{"HostName":"ubuntu-jw-01","datagroupname":"group4","dbpath":"/sequoiadb/database/data/11860","svcname":"11860","role":"data","archivecompresson":"true","archiveexpired":"240","archiveon":"false","archivepath":"","archivequota":"10","archivetimeout":"600","bkuppath":"","diaglevel":"3","hjbuf":"128","indexpath":"","lobmetapath":"","lobpath":"","logbuffsize":"1024","logfilenum":"20","logfilesz":"64","maxprefpool":"200","maxreplsync":"10","maxsyncjob":"10","numpreload":"0","preferedinstance":"A","sortbuf":"512","syncdeep":"false","syncinterval":"10000","syncrecordnum":"0","syncstrategy":"keepnormal","transactionon":"false","weight":"10"}],"Coord":[{"HostName":"ubuntu-jw-01","svcname":"11810"},{"HostName":"ubuntu-jw-02","svcname":"11840"}],"User":"","Passwd":"","ClusterName":"myCluster1","BusinessType":"sequoiadb","BusinessName":"myModule1","DeployMod":"horizontal"},"ResultInfo":[{"Status":2,"StatusDesc":"ROLLBACK","HostName":"ubuntu-jw-02","datagroupname":"group4","svcname":"11850","role":"data","errno":-3,"detail":"Failed to create node, Permission Error","Flow":["Installing data[ubuntu-jw-02:11850]","Failed to install data[ubuntu-jw-02:11850]"]},{"Status":2,"StatusDesc":"ROLLBACK","HostName":"ubuntu-jw-03","datagroupname":"group4","svcname":"11840","role":"data","errno":-3,"detail":"Failed to create node, Permission Error","Flow":["Installing data[ubuntu-jw-03:11840]","Failed to install data[ubuntu-jw-03:11840]"]},{"Status":2,"StatusDesc":"ROLLBACK","HostName":"ubuntu-jw-01","datagroupname":"group4","svcname":"11860","role":"data","errno":-3,"detail":"Failed to create node, Permission Error","Flow":["Installing data[ubuntu-jw-01:11860]","Failed to install data[ubuntu-jw-01:11860]"]}]} ;
   @return
      RET_JSON: the format is: {"ResultInfo":[{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-02","datagroupname":"group4","svcname":"11850","role":"data","errno":-3,"detail":"Failed to create node, Permission Error","Flow":["Rollbacking data[ubuntu-jw-02:11850]","Finish rollback data[ubuntu-jw-02:11850]"]},{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-03","datagroupname":"group4","svcname":"11840","role":"data","errno":-3,"detail":"Failed to create node, Permission Error","Flow":["Rollbacking data[ubuntu-jw-03:11840]","Finish rollback data[ubuntu-jw-03:11840]"]},{"Status":4,"StatusDesc":"FINISH","HostName":"ubuntu-jw-01","datagroupname":"group4","svcname":"11860","role":"data","errno":-3,"detail":"Failed to create node, Permission Error","Flow":["Rollbacking data[ubuntu-jw-01:11860]","Finish rollback data[ubuntu-jw-01:11860]"]}]}
*/

var PD_LOGGER = new Logger( "extendSequoiaDB.js" ) ;

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

function _createGroup( db, groupName )
{
   var rc = SDB_OK ;
   var rg = null ;

   try
   {
      PD_LOGGER.logTask( PDEVENT, sprintf( "Create data group[?]",
                                           groupName ) ) ;
      rg = db.createRG( groupName ) ;
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc == SDB_CAT_GRP_EXIST )
      {
         rg = null ;
      }
      else if( rc )
      {
         var error = new SdbError( rc,
                                   sprintf( "Failed to create group [?]",
                                            groupName ) ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;
         throw error ;
      }
   }
   return rg ;
}

function _getGroup( db, role, dataGroupName, isAutoCreate )
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
         if( rc == SDB_CLS_GRP_NOT_EXIST && role == FIELD_DATA &&
             isAutoCreate == true )
         {
            rg = _createGroup( db, dataGroupName ) ;
            if( rg == null )
            {
               sleep( 1000 ) ;
               continue ;
            }
            break ;
         }
         else if( rc == SDB_CLS_GRP_NOT_EXIST && role == FIELD_DATA )
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

function GeneratePlan( taskID )
{
   var plan = {} ;
   var planInfo    = BUS_JSON[FIELD_INFO] ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var coordList   = planInfo[FIELD_COORD2] ;
   var deployMod   = planInfo[FIELD_DEPLOYMOD] ;
   var user        = planInfo[FIELD_USER] ;
   var passwd      = planInfo[FIELD_PASSWD] ;
   var groupTask   = [] ;
   var coordTask   = [] ;
   var catalogTask = [] ;
   var dataTask    = [] ;
   var restoreTask = [] ;
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
   progressStep = parseInt( 30 / nodeNum ) ;

   for( var index in planInfo[FIELD_CONFIG] )
   {
      var taskConfig = {} ;
      var nodeConfig = planInfo[FIELD_CONFIG][index] ;

      taskConfig[FIELD_CMD]    = "create node" ;
      taskConfig[FIELD_TASKID] = taskID ;
      taskConfig[FIELD_COORD2] = coordList ;
      taskConfig[FIELD_USER]   = user ;
      taskConfig[FIELD_PASSWD] = passwd ;
      taskConfig[FIELD_CONFIG] = nodeConfig ;
      taskConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      taskConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;

      if( nodeConfig[FIELD_ROLE] == FIELD_COORD )
      {
         coordTask.push( taskConfig ) ;
      }
      else if( nodeConfig[FIELD_ROLE] == FIELD_CATALOG )
      {
         catalogTask.push( taskConfig ) ;
      }
      else if( nodeConfig[FIELD_ROLE] == FIELD_DATA )
      {
         dataTask.push( taskConfig ) ;
      }

      var groupConfig = {} ;
      groupConfig[FIELD_CMD]    = "start group" ;
      groupConfig[FIELD_TASKID] = taskID ;
      groupConfig[FIELD_COORD2] = coordList ;
      groupConfig[FIELD_USER]   = user ;
      groupConfig[FIELD_PASSWD] = passwd ;
      groupConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      groupConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;
      groupTask.push( groupConfig ) ;
      
      var restoreConfig = {} ;
      restoreConfig[FIELD_CMD]    = "restore node config" ;
      restoreConfig[FIELD_TASKID] = taskID ;
      restoreConfig[FIELD_RESULTINFO] = resultInfo[index] ;
      restoreConfig[FIELD_RESULTINFO][FIELD_PROGRESS] = progressStep ;
      restoreConfig[FIELD_USER]   = user ;
      restoreConfig[FIELD_PASSWD] = passwd ;
      if( nodeConfig[FIELD_ROLE] == FIELD_COORD )
      {
         var tmpCoordList = [] ;
         var tmpCoordInfo = {} ;

         tmpCoordInfo[FIELD_HOSTNAME] = nodeConfig[FIELD_HOSTNAME] ;
         tmpCoordInfo[FIELD_SVCNAME] = nodeConfig[FIELD_SVCNAME] ;
         tmpCoordList.push( tmpCoordInfo ) ;

         restoreConfig[FIELD_COORD2] = tmpCoordList ;
      }
      else
      {
         restoreConfig[FIELD_COORD2] = coordList ;
      }
      restoreTask.push( restoreConfig ) ;
   }

   plan[FIELD_PLAN].push( coordTask ) ;
   plan[FIELD_PLAN].push( catalogTask ) ;
   plan[FIELD_PLAN].push( dataTask ) ;
   plan[FIELD_PLAN].push( groupTask ) ;
   plan[FIELD_PLAN].push( restoreTask ) ;

   PD_LOGGER.logTask( PDEVENT, "Finish generate plan" ) ;

   return plan ;
}

function CheckResult()
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

function _checkNodeByScript( taskID, hostName, svcname )
{
   var isPrintWarning = false ;
   for( var i = 0; i < 10; ++i )
   {
      try
      {
         var agentPort = Oma.getAOmaSvcName( hostName ) ;
         var oma = new Oma( hostName, agentPort ) ;
         var config = oma.getNodeConfigs( svcname ) ;
         var configObj = JSON.parse( config ) ;
         return !(configObj[FIELD_SAC_TASKID] === taskID.toString()) ;
      }
      catch( e )
      {
         if( isPrintWarning == false )
         {
            PD_LOGGER.logTask( PDWARNING,
                               sprintf( "Failed to get node info [?:?]",
                                        hostName, svcname ) ) ;
            isPrintWarning = true ;
         }
         sleep( 1000 ) ;
      }
   }
   return true ;
}

function _removeNode( db, nodeResult, taskID )
{
   var rc = SDB_OK ;
   var result    = nodeResult ;
   var role      = result[FIELD_ROLE] ;
   var hostName  = result[FIELD_HOSTNAME] ;
   var svcname   = result[FIELD_SVCNAME] ;
   var groupName = result[FIELD_DATAGROUPNAME] ;
   var rg = null ;
   var cur = null ;
   var detail = null ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Rollbacking ?[?:?]",
                                        role, hostName, svcname ) ) ;

   result[FIELD_FLOW].push( sprintf( "Rollbacking ?[?:?]",
                                      role, hostName, svcname ) ) ;

   if( _checkNodeByScript( taskID, hostName, svcname ) )
   {
      PD_LOGGER.logTask( PDEVENT, sprintf( "Finish rollback ?[?:?]",
                                           role, hostName, svcname ) ) ;

      result[FIELD_FLOW].push( sprintf( "Finish rollback ?[?:?]",
                                         role, hostName, svcname ) ) ;
   
      return result ;
   }

   try
   {
      rg = _getGroup( db, role, groupName, false ) ;
   }
   catch( e )
   {
      if( e.getErrCode() == SDB_CLS_GRP_NOT_EXIST )
      {
         PD_LOGGER.logTask( PDEVENT, sprintf( "Finish rollback ?[?:?]",
                                              role, hostName, svcname ) ) ;

         result[FIELD_FLOW].push( sprintf( "Finish rollback ?[?:?]",
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
      try
      {
         rg.removeNode( hostName, svcname, { "enforced": true } ) ;
      }
      catch( e )
      {
         rc = getLastError() ;
         if( rc != SDB_CLS_NODE_NOT_EXIST )
         {
            var error = new SdbError( rc, sprintf( "Failed to remove node[?:?]",
                                                   hostName, svcname ) ) ;
            PD_LOGGER.logTask( PDERROR, error ) ;
            throw error ;
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

   PD_LOGGER.logTask( PDEVENT, sprintf( "Finish rollback ?[?:?]",
                                        role, hostName, svcname ) ) ;

   result[FIELD_FLOW].push( sprintf( "Finish rollback ?[?:?]",
                                      role, hostName, svcname ) ) ;

   return result ;
}

function Rollback( taskID )
{
   var taskInfo    = BUS_JSON[FIELD_INFO] ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var coordList   = taskInfo[FIELD_COORD2] ;
   var user        = taskInfo[FIELD_USER] ;
   var passwd      = taskInfo[FIELD_PASSWD] ;
   var result      = [] ;
   var db = null ;
   var newResultInfo = {} ;
   newResultInfo[FIELD_RESULTINFO] = result ;

   PD_LOGGER.logTask( PDEVENT, "Begin rollback" ) ;

   db = _connectCoord( coordList, user, passwd ) ;

   for( var index in resultInfo )
   {
      var nodeResult = resultInfo[index] ;
      nodeResult[FIELD_FLOW] = [] ;
      try
      {
         nodeResult = _removeNode( db, nodeResult, taskID ) ;
         nodeResult[FIELD_STATUS] = STATUS_FINISH ;
         nodeResult[FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;
      }
      catch( e )
      {
         var role     = nodeResult[FIELD_ROLE] ;
         var hostName = nodeResult[FIELD_HOSTNAME] ;
         var svcname  = nodeResult[FIELD_SVCNAME] ;
         
         nodeResult[FIELD_ERRNO]  = e.getErrCode() ;
         nodeResult[FIELD_DETAIL] = e.getErrMsg() ;
         nodeResult[FIELD_STATUS] = STATUS_FAIL ;
         nodeResult[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
         nodeResult[FIELD_FLOW].push( sprintf( "Failed to rollback ?[?:?], ?",
                                   role, hostName, svcname, e.getErrMsg() ) ) ;

         PD_LOGGER.logTask( PDERROR, e ) ;
      }
      
      result.push( nodeResult ) ;
   }

   newResultInfo[FIELD_RESULTINFO] = result ;

   return newResultInfo ;
}

function _createNode( coordList, user, passwd, hostName, svcname, dbpath,
                      role, nodeConfig, resultInfo )
{
   var rc   = SDB_OK ;
   var db   = null ;
   var rg   = null ;
   var node = null ;

   resultInfo[FIELD_FLOW].push( sprintf( "Installing ?[?:?]",
                                         role, hostName, svcname ) ) ;

   db = _connectCoord( coordList, user, passwd ) ;
   rg = _getGroup( db, role, nodeConfig[FIELD_DATAGROUPNAME], true ) ;

   try
   {
      node = rg.createNode( hostName, svcname, dbpath, nodeConfig ) ;
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc )
      {
         var error = new SdbError( rc, sprintf( "Failed to create node ?[?:?]",
                                                role, hostName, svcname ) ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;
         throw error ;
      }
   }

   resultInfo[FIELD_FLOW].push( sprintf( "Successfully create ?[?:?]",
                                         role, hostName, svcname ) ) ;
   return resultInfo ;
}

function InstallNode( taskID )
{
   var coordList   = BUS_JSON[FIELD_COORD2] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var nodeConfig  = BUS_JSON[FIELD_CONFIG] ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var role        = nodeConfig[FIELD_ROLE] ;
   var hostName    = nodeConfig[FIELD_HOSTNAME] ;
   var svcname     = nodeConfig[FIELD_SVCNAME] ;
   var dbpath      = nodeConfig[FIELD_DBPATH] ;

   nodeConfig[FIELD_SAC_TASKID] = taskID ;

   try
   {
      if( role == FIELD_COORD || role == FIELD_CATALOG || role == FIELD_DATA )
      {
         PD_LOGGER.logTask( PDEVENT, sprintf( "Create ?[?:?]",
                                              role, hostName, svcname ) ) ;

         resultInfo = _createNode( coordList, user, passwd,
                                   hostName, svcname, dbpath,
                                   role, nodeConfig, resultInfo ) ;
      }
      else
      {
         var error = new SdbError( SDB_CLS_NOT_PRIMARY,
                                   sprintf( "Invalid role [?]", role ) ) ;
         PD_LOGGER.logTask( PDERROR, error ) ;
         throw error ;
      }
   }
   catch( e )
   {
      resultInfo[FIELD_ERRNO] = e.getErrCode() ;
      resultInfo[FIELD_DETAIL] = e.getErrMsg() ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( sprintf( "Failed to install ?[?:?]",
                                   role, hostName, svcname ) ) ;
   }

   return resultInfo ;
}

function StartGroup()
{
   var rc = SDB_OK ;
   var coordList   = BUS_JSON[FIELD_COORD2] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var hostName    = resultInfo[FIELD_HOSTNAME] ;
   var svcname     = resultInfo[FIELD_SVCNAME] ;
   var role        = resultInfo[FIELD_ROLE] ;
   var groupName   = resultInfo[FIELD_DATAGROUPNAME] ;
   var db          = null ;
   var rg          = null ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Start ?[?:?]",
                                        role, hostName, svcname ) ) ;

   resultInfo[FIELD_FLOW].push( sprintf( "Start ?[?:?]",
                                         role, hostName, svcname ) ) ;

   db = _connectCoord( coordList, user, passwd ) ;
   rg = _getGroup( db, role, groupName, false ) ;

   try
   {
      rg.start() ;

      PD_LOGGER.logTask( PDEVENT, sprintf( "Finish install ?[?:?]",
                                            role, hostName, svcname ) ) ;

      resultInfo[FIELD_FLOW].push( sprintf( "Finish install ?[?:?]",
                                            role, hostName, svcname ) ) ;

      resultInfo[FIELD_STATUS] = STATUS_FINISH ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;
   }
   catch( e )
   {
      var errMsg = getLastErrMsg() ;
      rc = getLastError() ;
      PD_LOGGER.logTask( PDERROR, sprintf( "Failed to start ?[?:?], ?, errcode=? ",
                                           role, hostName, svcname,
                                           rc, errMsg ) ) ;

      resultInfo[FIELD_ERRNO]  = rc ;
      resultInfo[FIELD_DETAIL] = errMsg ;
      resultInfo[FIELD_STATUS] = STATUS_FAIL ;
      resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FAIL ;
      resultInfo[FIELD_FLOW].push( sprintf( "Failed to start ?[?:?]",
                                   role, hostName, svcname ) ) ;
      
   }

   return resultInfo ;
}

function _restoreNodeByScript( coordList, user, passwd,
                               taskID, hostName, svcname )
{
   try
   {
      var db = null ;
      var agentPort = Oma.getAOmaSvcName( hostName ) ;
      var oma = new Oma( hostName, agentPort ) ;
      var config = oma.getNodeConfigs( svcname ) ;
      var configObj = JSON.parse( config ) ;

      if( configObj[FIELD_SAC_TASKID] === taskID.toString() )
      {
         delete configObj[FIELD_SAC_TASKID] ;
         oma.setNodeConfigs( svcname, configObj ) ;

         db = _connectCoord( coordList, user, passwd ) ;
         db.reloadConf( { "HostName": hostName, "svcname": svcname } ) ;
      }
      return true ;
   }
   catch( e )
   {
      return false ;
   }
}

function RestoreNodeConfig( taskID )
{
   var rc = SDB_OK ;
   var resultInfo  = BUS_JSON[FIELD_RESULTINFO] ;
   var coordList   = BUS_JSON[FIELD_COORD2] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var hostName    = resultInfo[FIELD_HOSTNAME] ;
   var svcname     = resultInfo[FIELD_SVCNAME] ;

   resultInfo[FIELD_STATUS] = STATUS_FINISH ;
   resultInfo[FIELD_STATUS_DESC] = DESC_STATUS_FINISH ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Restore node config [?:?]",
                                        hostName, svcname ) ) ;

   if( _restoreNodeByScript( coordList, user, passwd,
                             taskID, hostName, svcname ) )
   {
      PD_LOGGER.logTask( PDEVENT, sprintf( "Finish restore node config [?:?]",
                                            hostName, svcname ) ) ;
   }
   else
   {
      PD_LOGGER.logTask( PDWARNING,
                         sprintf( "Failed to restore node config [?:?]",
                                  hostName, svcname ) ) ;
   }
   return resultInfo ;
}

function main()
{
   var taskID = 0 ;
   var result = {} ;

   PD_LOGGER.logComm( PDEVENT, sprintf( "Begin to run extend, Step [?]",
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

   PD_LOGGER.setTaskId( taskID ) ;

   PD_LOGGER.logTask( PDEVENT, sprintf( "Step [?]", SYS_STEP ) ) ;

   if( SYS_STEP == STEP_GENERATE_PLAN )
   {
      result = GeneratePlan( taskID ) ;
   }
   else if( SYS_STEP == STEP_DOIT )
   {
      if( BUS_JSON[FIELD_CMD] == "create node" )
      {
         result = InstallNode( taskID ) ;
      }
      else if( BUS_JSON[FIELD_CMD] == "start group" )
      {
         result = StartGroup() ;
      }
      else if( BUS_JSON[FIELD_CMD] == "restore node config" )
      {
         result = RestoreNodeConfig( taskID ) ;
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

main() ;
