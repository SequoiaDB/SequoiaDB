/*******************************************************************************
*@Description : query_sync testcase common functions
*@Modify list :
*              2019-5-30 wangkexin
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/* ****************************************************
@description: insert data into collection
@parameter: 
    cl : the collection ready to insert records
    insertNum : the number of records inserted
@return: 
**************************************************** */
function insertData ( cl, insertNum )
{
   var dataArray = new Array();
   for( var i = 0; i < insertNum; i++ )
   {
      var data = { a: i };
      dataArray.push( data );
   }
   cl.insert( dataArray );
}

/* ****************************************************
@description: create a group, specify the instance id of the node
@parameter: 
    rgName : the name of dataRG
    hostName : host name of new node
@return: log paths to be backed up
**************************************************** */
function createDataGroups ( rgName, hostName, instanceidArr, logSourcePaths )
{
   var tmpArray = [];
   var dataRG = db.createRG( rgName );

   for( var i = 0; i < instanceidArr.length; i++ )
   {
      var port = parseInt( RSRVPORTBEGIN ) + ( i * 10 );
      var dataPath = RSRVNODEDIR + "data/" + port;
      var checkSucc = false;
      var times = 0;
      var maxRetryTimes = 10;
      do
      {
         try
         {
            dataRG.createNode( hostName, port, dataPath, { diaglevel: 5, instanceid: instanceidArr[i] } );
            checkSucc = true;
            var obj = new Object();
            obj.NodeName = hostName + ":" + port;
            obj.instanceid = instanceidArr[i];
            tmpArray.push( obj );
            logSourcePaths.push( hostName + ":" + CMSVCNAME + "@" + dataPath + "/diaglog/sdbdiag.log" );
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e.message == SDBCM_NODE_EXISTED || e.message == SDB_DIR_NOT_EMPTY )
            {
               port = port + 10;
               dataPath = RSRVNODEDIR + "data/" + port;
            }
            else
            {
               throw e;
            }
            times++;
         }
      }
      while( !checkSucc && times < maxRetryTimes );
   }
   dataRG.start();
   return tmpArray;
}

/* ****************************************************
@description: remove one data group
@parameter: 
    rgName : The name of data group to be removed 
@return:
**************************************************** */
function removeDataRG ( rgName )
{
   try
   {
      db.removeRG( rgName );
   }
   catch( e )
   {
      //-154 : SDB_CLS_GRP_NOT_EXIST
      if( e.message != SDB_CLS_GRP_NOT_EXIST )
      {
         throw e;
      }
   }
}

/* ****************************************************
@description: check whether the specified node is the primary or slave node in the group
@parameter: 
    node : the specified node
    groupName : the specified group
    expMaster : if expect the node is the primary node (true/false)
@return:
**************************************************** */
function checkRole ( node, groupName, expMaster )
{
   db.getRG( groupName ).getNode( node );

   var isMaster = new Sdb( node ).snapshot( 7 ).current().toObj().IsPrimary;
   assert.equal( isMaster, expMaster );
}

/* ****************************************************
@description: check node by it instance id.
@parameter: 
    actQueryNode : the actual node
    nodeInfo : node information array  
        ex:
           [0] {"NodeName":"XXXX", "instanceid":XXXX}
           [1] {"NodeName":"XXXX", "instanceid":XXXX}
           [N] ...
    instanceid : expected instance id.
@return:
**************************************************** */
function checkNodeByInstanceId ( actQueryNode, nodeInfo, instanceid )
{
   var expNodeName = "";
   for( var i = 0; i < nodeInfo.length; i++ )
   {
      if( nodeInfo[i].instanceid == instanceid )
      {
         expNodeName = nodeInfo[i].NodeName;
         break;
      }
   }
   assert.equal( actQueryNode, expNodeName );
}