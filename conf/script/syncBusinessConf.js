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
@description: sync business configure ( sequoiadb )
@modify list:
   2017-07-14 JiaWen He  Init

@parameter
   var BUS_JSON = var BUS_JSON = { "ClusterName": "myCluster1", "BusinessName": "myModule1", "BusinessType": "sequoiadb", "User": "admin", "Passwd": "admin", "Address": [ { "HostName": "ubuntu-jw-01", "svcname": "11810" } ] } ;
@return
   RET_JSON: the format is: {"HostInfo":[{"HostName":"ubuntu-jw-02","ClusterName":"myCluster1","BusinessName":"myModule1","BusinessType":"sequoiadb","DeployMod":"distribution","Config":[{"archivecompresson":"TRUE","archiveexpired":"240","archiveon":"FALSE","archivepath":"/opt/sequoiadb/database/catalog/11820/archivelog/","archivequota":"10","archivetimeout":"600","bkuppath":"/opt/sequoiadb/database/catalog/11820/bakfile/","businessname":"myModule1","catalogaddr":"ubuntu-jw-01:11823,ubuntu-jw-02:11823,ubuntu-jw-03:11823","clustername":"myCluster1","dbpath":"/opt/sequoiadb/database/catalog/11820/","diaglevel":"3","hjbuf":"128","indexpath":"/opt/sequoiadb/database/catalog/11820/","lobmetapath":"/opt/sequoiadb/database/catalog/11820/","lobpath":"/opt/sequoiadb/database/catalog/11820/","logbuffsize":"1024","logfilenum":"20","logfilesz":"64","maxprefpool":"0","maxreplsync":"0","maxsyncjob":"10","numpreload":"0","preferedinstance":"A","role":"catalog","sortbuf":"256","svcname":"11820","syncdeep":"FALSE","syncinterval":"10000","syncrecordnum":"10","syncstrategy":"None","transactionon":"TRUE","usertag":"","weight":"10","datagroupname":""}]},{"HostName":"ubuntu-jw-03","ClusterName":"myCluster1","BusinessName":"myModule1","BusinessType":"sequoiadb","DeployMod":"distribution","Config":[{"archivecompresson":"true","archiveexpired":"240","archiveon":"false","archivepath":"","archivequota":"10","archivetimeout":"600","bkuppath":"","businessname":"myModule1","catalogaddr":"ubuntu-jw-01:11823,ubuntu-jw-02:11823,ubuntu-jw-03:11823","clustername":"myCluster1","dbpath":"/opt/sequoiadb/database/data/11830","diaglevel":"3","hjbuf":"128","indexpath":"","lobmetapath":"","lobpath":"","logbuffsize":"1024","logfilenum":"20","logfilesz":"64","maxprefpool":"200","maxreplsync":"10","maxsyncjob":"10","numpreload":"0","preferedinstance":"A","role":"data","sortbuf":"256","svcname":"11830","syncdeep":"false","syncinterval":"10000","syncrecordnum":"0","syncstrategy":"keepnormal","transactionon":"false","usertag":"","weight":"10","datagroupname":"group1"}]},{"HostName":"ubuntu-jw-01","ClusterName":"myCluster1","BusinessName":"myModule1","BusinessType":"sequoiadb","DeployMod":"distribution","Config":[{"archivecompresson":"true","archiveexpired":"240","archiveon":"false","archivepath":"","archivequota":"10","archivetimeout":"600","bkuppath":"","businessname":"myModule1","catalogaddr":"ubuntu-jw-01:11823,ubuntu-jw-02:11823,ubuntu-jw-03:11823","clustername":"myCluster1","dbpath":"/opt/sequoiadb/database/coord/11810","diaglevel":"3","hjbuf":"128","indexpath":"","lobmetapath":"","lobpath":"","logbuffsize":"1024","logfilenum":"20","logfilesz":"64","maxprefpool":"200","maxreplsync":"10","maxsyncjob":"10","numpreload":"0","preferedinstance":"A","role":"coord","sortbuf":"256","svcname":"11810","syncdeep":"false","syncinterval":"10000","syncrecordnum":"0","syncstrategy":"keepnormal","transactionon":"false","usertag":"","weight":"10","datagroupname":""}]}]}
*/

var PD_LOGGER = new Logger( "syncBusinessConf.js" ) ;

function _connectSdb( addressList, user, passwd )
{
   var rc = SDB_OK ;
   var db = null ;
   var hostName ;
   var svcname ;
   var result = {} ;

   if( addressList.length == 0 )
   {
      rc = SDB_SYS ;
      var error = new SdbError( rc, "coord address is empty" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   for( var index in addressList )
   {
      try
      {
         rc = SDB_OK ;
         hostName = addressList[index][FIELD_HOSTNAME] ;
         svcname  = addressList[index][FIELD_SVCNAME] ;
         PD_LOGGER.log( PDEVENT, sprintf( "Connect sequoiadb[?:?]",
                                          hostName, svcname ) ) ;
         db = new Sdb( hostName, svcname, user, passwd ) ;
         result = addressList[index] ;
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

   if( rc || db == null )
   {
      var error = new SdbError( rc, "Failed to connect coord" ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   result["db"] = db ;
   return result ;
}

function _getNodeList( db )
{
   var rc    = SDB_OK ;
   var error = null ;
   var nodeList = [] ;

   try
   {
      var cursor = null ;
      var record = null ;

      cursor = db.listReplicaGroups() ;
      while( record = cursor.next() )
      {
         var groupInfo = record.toObj() ;
         var groupName = groupInfo[FIELD_GROUPNAME] ;

         for( var index in groupInfo[FIELD_GROUP] )
         {
            var nodeInfo = groupInfo[FIELD_GROUP][index] ;
            var nodeAddress = {} ;

            nodeAddress[FIELD_HOSTNAME] = nodeInfo[FIELD_HOSTNAME] ;
            nodeAddress[FIELD_SVCNAME]  = nodeInfo[FIELD_SERVICE][0][FIELD_NAME] ;
            nodeAddress[FIELD_GROUPNAME] = groupName ;
            nodeList.push( nodeAddress ) ;
         }
      }
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc == SDB_OK )
      {
         rc = SDB_SYS ;
         error = new SdbError( rc, e.message ) ;
      }
      else
      {
         error = new SdbError( rc, "Failed to get node list" ) ;
      }
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   return nodeList ;
}

function _updateConfig( hostName, svcname, agentService, config )
{
   var agentPort ;
   if( typeof( agentService ) == 'string' && agentService.length > 0 )
   {
      agentPort = agentService ;
   }
   else
   {
      agentPort = Oma.getAOmaSvcName( hostName ) ;
   }
   var oma = new Oma( hostName, agentPort ) ;
   oma.setNodeConfigs( svcname, config ) ;
}

function _getConfig( hostName, svcname, agentService )
{
   var agentPort ;
   if( typeof( agentService ) == 'string' && agentService.length > 0 )
   {
      agentPort = agentService ;
   }
   else
   {
      agentPort = Oma.getAOmaSvcName( hostName ) ;
   }
   var oma = new Oma( hostName, agentPort ) ;
   var configStr = oma.getNodeConfigs( svcname ) ;

   return JSON.parse( configStr ) ;
}

function _getNodeConfig( hostRemoval, hostList, hostName, svcname, groupName )
{
   var rc     = SDB_OK ;
   var error  = null ;
   var config = {} ;
   var clusterName  = BUS_JSON[FIELD_CLUSTER_NAME] ;
   var businessName = BUS_JSON[FIELD_BUSINESS_NAME] ;
   var deployMod ;
   var index ;

   try
   {
      index = hostRemoval[hostName] ;

      config = _getConfig( hostName, svcname ) ;
      if( config[FIELD_ROLE] == FIELD_COORD ||
          config[FIELD_ROLE] == FIELD_CATALOG )
      {
         config[FIELD_DATAGROUPNAME] = "" ;
         deployMod = OMA_DEPLOY_CLUSTER ;
      }
      else if( config[FIELD_ROLE] == FIELD_DATA )
      {
         config[FIELD_DATAGROUPNAME] = groupName ;
         deployMod = OMA_DEPLOY_CLUSTER ;
      }
      else if( config[FIELD_ROLE] == OMA_DEPLOY_STANDALONE )
      {
         config[FIELD_DATAGROUPNAME] = "" ;
         deployMod = OMA_DEPLOY_STANDALONE ;
      }
      else
      {
         return ;
      }
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc == SDB_OK )
      {
         rc = SDB_SYS ;
         error = new SdbError( rc, e.message ) ;
      }
      else
      {
         error = new SdbError( rc, sprintf( "Failed to get node config [?:?]",
                                            hostName, svcname ) ) ;
      }
      PD_LOGGER.log( PDERROR, error ) ;
   }

   config[FIELD_CLUSTER_NAME2] = clusterName ;
   config[FIELD_BUSINESS_NAME2] = businessName ;
   _updateConfig( hostName, svcname, null, config ) ;

   if( isNaN( index ) == true )
   {
      var hostInfo = {} ;

      index = hostList.length ;
      hostRemoval[hostName] = index ;

      hostInfo[FIELD_ERRNO]         = rc ;
      hostInfo[FIELD_DETAIL]        = rc == SDB_OK ? "" : error.toString() ;
      hostInfo[FIELD_HOSTNAME]      = hostName ;
      hostInfo[FIELD_CLUSTER_NAME]  = clusterName ;
      hostInfo[FIELD_BUSINESS_NAME] = businessName ;
      hostInfo[FIELD_BUSINESS_TYPE] = FIELD_SEQUOIADB ;
      hostInfo[FIELD_DEPLOYMOD]     = deployMod ;
      hostInfo[FIELD_CONFIG]        = [] ;

      hostInfo[FIELD_CONFIG].push( config ) ;
      hostList.push( hostInfo ) ;
   }
   else
   {
      if ( hostList[index][FIELD_ERRNO] == SDB_OK && rc != SDB_OK )
      {
         hostList[index][FIELD_ERRNO]  = rc ;
         hostList[index][FIELD_DETAIL] = rc == SDB_OK ? "" : error.toString() ;
      }
      hostList[index][FIELD_CONFIG].push( config ) ;
   }
}

function _getHostMap( connectInfo )
{
   var hostMap = [] ;
   var hostName = connectInfo[FIELD_HOSTNAME] ;
   var port     = connectInfo[FIELD_AGENT_SERVICE] ;
   if( typeof( port ) != 'string' || port.length == 0 )
   {
      port = Oma.getAOmaSvcName( hostName ) ;
   }

   try
   {
      var remote = new Remote( hostName, port ) ;
      var remoteSys = remote.getSystem() ;
      var maps = remoteSys.getHostsMap().toObj() ;
      var hosts = maps[FIELD_HOSTS] ;

      for( var index in hosts )
      {
         if( hosts[index][FIELD_HOSTNAME] != "localhost" &&
             hosts[index][FIELD_IP2] != "127.0.0.1" )
         {
            var hostInfo = {} ;

            hostInfo[FIELD_HOSTNAME] = hosts[index][FIELD_HOSTNAME] ;
            hostInfo[FIELD_IP]       = hosts[index][FIELD_IP2] ;
            hostMap.push( hostInfo ) ;
         }
      }
   }
   catch( e )
   {
   }

   return hostMap ;
}

function _syncSdbConfig()
{
   var result      = {}
   var addressList = BUS_JSON[FIELD_ADDRESS] ;
   var user        = BUS_JSON[FIELD_USER] ;
   var passwd      = BUS_JSON[FIELD_PASSWD] ;
   var nodeList    = [] ;
   var hostList    = [] ;
   var hostRemoval = {} ;
   var db = null ;
   var nodeConfig ;
   var connectInfo ;

   connectInfo = _connectSdb( addressList, user, passwd ) ;

   db = connectInfo["db"] ;

   result[FIELD_HOSTS] = _getHostMap( connectInfo ) ;

   try
   {
      nodeConfig = _getConfig( connectInfo[FIELD_HOSTNAME],
                               connectInfo[FIELD_SVCNAME],
                               connectInfo[FIELD_AGENT_SERVICE] ) ;
   }
   catch( e )
   {
      rc = getLastError() ;
      if( rc == SDB_OK )
      {
         rc = SDB_SYS ;
         error = new SdbError( rc, e.message ) ;
      }
      else
      {
         error = new SdbError( rc, sprintf( "Failed to get node config [?:?]",
                                            connectInfo[FIELD_HOSTNAME],
                                            connectInfo[FIELD_SVCNAME] ) ) ;
      }
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   if( nodeConfig[FIELD_ROLE] == FIELD_COORD )
   {
      nodeList = _getNodeList( db ) ;
      for( var index in nodeList )
      {
         var hostName  = nodeList[index][FIELD_HOSTNAME] ;
         var svcname   = nodeList[index][FIELD_SVCNAME] ;
         var groupName = nodeList[index][FIELD_GROUPNAME] ;
   
         _getNodeConfig( hostRemoval, hostList, hostName, svcname, groupName ) ;
      }
   }
   else if( nodeConfig[FIELD_ROLE] == OMA_DEPLOY_STANDALONE )
   {
      _getNodeConfig( hostRemoval, hostList,
                      connectInfo[FIELD_HOSTNAME], connectInfo[FIELD_SVCNAME],
                      "" ) ;
   }
   else
   {
      var rc = SDB_SYS ;
      error = new SdbError( rc, sprintf( "failed to sync business info [?:?:?] is not supported",
                                         connectInfo[FIELD_HOSTNAME],
                                         connectInfo[FIELD_SVCNAME],
                                         nodeConfig[FIELD_ROLE] ) ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   result[FIELD_HOST_INFO] = hostList ;

   return result ;
}

function main()
{
   var result = {} ;

   PD_LOGGER.log( PDEVENT, "Begin to sync business configure" ) ;

   var businessName = BUS_JSON[FIELD_BUSINESS_NAME] ;
   var businessType = BUS_JSON[FIELD_BUSINESS_TYPE] ;

   if ( businessType == FIELD_SEQUOIADB )
   {
      result = _syncSdbConfig() ;
   }
   else
   {
      rc = SDB_SYS ;
      error = new SdbError( rc, sprintf( "Sync business info [?,?] is not supported",
                                         businessName, businessType ) ) ;
      PD_LOGGER.log( PDERROR, error ) ;
      throw error ;
   }

   PD_LOGGER.log( PDEVENT, "finish sync business configure" ) ;
   
   return result ;
}

main() ;
 