import( "../lib/main.js" )

/************************************************************************
*@Description: 获取满足指定节点数目的组
*@author:      zhaoxiaoni
*@createDate:  2018.1.22
**************************************************************************/
function getGroupsWithNodeNum ( nodesNum )
{
   var groups = [];
   var groupArray = commGetGroups( db );
   for( var i = 0; i < groupArray.length; i++ )
   {
      var group = groupArray[i];
      if( group.length - 1 >= nodesNum )
      {
         groups.push( group );
      }
   }
   return groups;
}

/************************************************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function insertData ( cl )
{
   var docs = [];
   for( var i = 0; i < 5000; ++i )
   {
      docs.push( { a: i } );
   }
   cl.insert( docs );
}

/************************************************************************
*@Description: get svcname of the data group
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function getGroupNodes ( groupName )
{
   var groupInfo = db.getRG( groupName ).getDetail().current().toObj().Group;
   var groupNodes = [];
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      groupNodes.push( nodeInfo );
   }
   return groupNodes;
}

/************************************************************************
*@Description: 更新节点配置
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function updateConf ( db, configs, options, errno )
{
   try
   {
      db.updateConf( configs, options );
   }
   catch( e )
   {
      if( errno === undefined || e.message !== errno.toString() )
      {
         throw e;
      }
   }
}

function deleteConf ( db, configs, options, errno )
{
   try
   {
      db.deleteConf( configs, options );
   }
   catch( e )
   {
      if( errno === undefined || e.message !== errno.toString() )
      {
         throw e;
      }
   }
}

/************************************************************************
*@Description: 检查访问节点是否符合预期
*@author:      wuyan
*@createDate:  2018.1.22
**************************************************************************/
function checkAccessNodes ( cl, expAccessNodes, options )
{
   var doTimes = 0;
   var timeOut = 10000;
   var actAccessNodes = [];
   while( doTimes < timeOut )//设置instanceid后，获取访问的节点，当访问节点数组的长度等于期望结果时结束循环
   {
      db.setSessionAttr( options );
      var cursor = cl.find().explain();
      while( cursor.next() )
      {
         var actAccessNode = cursor.current().toObj().NodeName;
         if( actAccessNodes.indexOf( actAccessNode ) === -1 )
         {
            actAccessNodes.push( actAccessNode );
         }
      }

      if( actAccessNodes.length === expAccessNodes.length )
      {
         break;
      }
      else
      {
         sleep( 10 );
         doTimes++;
      }
   }

   if( doTimes >= timeOut )
   {
      throw new Error( "actAccessNodes: " + actAccessNodes + ", expAccessNodes: " + expAccessNodes );
   }

   //实际结果与预期结果比较
   for( var i in expAccessNodes )
   {
      if( actAccessNodes.indexOf( expAccessNodes[i] ) === -1 )
      {
         println( "actAccessNodes: " + actAccessNodes + "\nexpAccessNodes: " + expAccessNodes );
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}

/************************************************************************
*@Description: 按照属性排序
*@author:      Zhao xiaoni
*@createDate:  2019-11-27
**************************************************************************/
function sortBy ( props )
{
   return function( a, b )
   {
      return a[props] - b[props];
   }
}

/************************************
*@Description: bulk insert data
*@input: dbcl        db.getCS(cs).getCL(cl)
         recordNum   插入记录数
         recordStart 记录数起始值
         recordEnd   记录结束值
**************************************/
function insertBulkData ( dbcl, recordNum, recordStart, recordEnd )
{
   if( undefined == recordNum ) { recordNum = 5000; }
   if( undefined == recordStart ) { recordStart = 0; }
   if( undefined == recordEnd ) { recordEnd = 5000; }
   try
   {
      var doc = [];
      for( var i = 0; i < recordNum; i++ )
      {
         var bValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         var cValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         doc.push( { a: i, b: bValue, c: cValue } );
      }
      dbcl.insert( doc );
   }
   catch( e )
   {
      throw buildException( "insertBulkData()", e, "insert", "insert data :" + JSON.stringify( doc ), "insert fail" );
   }
   return doc;
}

/************************************************************************
*@Description: 设置会话访问属性，查看访问计划检查访问节点信息
*@input: dbcl            db.getCS(cs).getCL(cl)
         expAccessNodes  预期访问节点
**************************************************************************/
function explainAndCheckAccessNodes ( cl, expAccessNodes )
{
   if( typeof ( expAccessNodes ) == "string" ) { expAccessNodes = [expAccessNodes]; }
   var cursor = cl.find().explain();
   var actAccessNodes = [];
   while( cursor.next() )
   {
      var actAccessNode = cursor.current().toObj().NodeName;
      if( actAccessNodes.indexOf( actAccessNode ) === -1 )
      {
         actAccessNodes.push( actAccessNode );
      }
   }
   cursor.close();


   //实际结果与预期结果比较
   for( var i in actAccessNodes )
   {
      if( expAccessNodes.indexOf( actAccessNodes[i] ) === -1 )
      {
         println( "actAccessNodes: " + actAccessNodes + "\nexpAccessNodes: " + expAccessNodes );
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}

/************************************************************************
*@Description: 返回指定group中的备节点名
*@input: db            
         groups     指定的group
**************************************************************************/
function getGroupSlaveNodeName ( db, groups )
{
   if( typeof ( groups ) == "string" ) { groups = [groups]; }
   var slaveNodeName = [];
   for( var i in groups )
   {
      var cursor = db.list( SDB_LIST_GROUPS, { "GroupName": groups[i] } );
      while( cursor.next() )
      {
         var groupInfo = cursor.current().toObj();
         var primaryNode = groupInfo.PrimaryNode;
         var nodeInfos = groupInfo.Group;
         for( var i in nodeInfos )
         {
            if( nodeInfos[i].NodeID != primaryNode )
            {
               var service = nodeInfos[i].Service;
               for( var j in service )
               {
                  if( service[j].Type == 0 )
                  {
                     svcname = service[j].Name;
                  }
                  break;
               }
               slaveNodeName.push( nodeInfos[i].HostName + ":" + svcname );
            }
         }
      }
      cursor.close();
   }
   return slaveNodeName;
}

/************************************************************************
*@Description: 返回指定group中的主节点名
*@input: db            
         groups     指定的group
**************************************************************************/
function getGroupMasterNodeName ( db, groups )
{
   if( typeof ( groups ) == "string" ) { groups = [groups]; }
   var masterNodeName = [];
   for( var i in groups )
   {
      var cursor = db.list( SDB_LIST_GROUPS, { "GroupName": groups[i] } );
      while( cursor.next() )
      {
         var groupInfo = cursor.current().toObj();
         var primaryNode = groupInfo.PrimaryNode;
         var nodeInfos = groupInfo.Group;
         for( var i in nodeInfos )
         {
            if( nodeInfos[i].NodeID == primaryNode )
            {
               var service = nodeInfos[i].Service;
               for( var j in service )
               {
                  if( service[j].Type == 0 )
                  {
                     svcname = service[j].Name;
                  }
                  break;
               }
               masterNodeName.push( nodeInfos[i].HostName + ":" + svcname );
            }
         }
      }
      cursor.close();
   }
   return masterNodeName;
}

/************************************************************************
*@Description: 获取一个备节点，重新选取使其成为主节点
*@input: db            
         groups     指定的group
**************************************************************************/
function masterChangToSlave ( db, group )
{
   var rg = db.getRG( group );
   var slave = rg.getSlave();
   var hostName = slave.getHostName();
   var serviceName = slave.getServiceName();
   rg.reelect( { "HostName": hostName, "ServiceName": serviceName } );
   commCheckBusinessStatus( db );
}

/************************************************************************
*@Description: 给指定组中节点设置instanceid，instanceids与nodeNames对应，如果instanceids小于nodeNames则使用随机数
*@input: db            
         group           指定的group
         instanceids     需要设置的instanceid
**************************************************************************/
function setInstanceids ( db, group, instanceids )
{
   var useInstanceids = instanceids;
   var nodes = commGetGroupNodes( db, group );
   for( var i = 0; i < nodes.length; i++ )
   {
      if( instanceids.length - i > 0 )
      {
         updateConf( db, { instanceid: instanceids[i] }, { NodeName: nodes[i].HostName + ":" + nodes[i].svcname }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      }
      else
      {
         var instanceid = getRandomNumber( useInstanceids );
         useInstanceids.push( instanceid );
         updateConf( db, { instanceid: instanceid }, { NodeName: nodes[i].HostName + ":" + nodes[i].svcname }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      }
   }

   db.getRG( group ).stop();
   db.getRG( group ).start();

   commCheckBusinessStatus( db );
}

function getRandomNumber ( useInstanceids )
{
   var instanceid = parseInt( ( Math.random() * 255 ) + 1 );
   if( useInstanceids.indexOf( instanceid ) === -1 )
   {
      return instanceid;
   }
   else
   {
      return getRandomNumber( useInstanceids );
   }
}

/************************************************************************
*@Description: 获取节点对应的instanceid，节点不存在instanceid返回默认值0
*@input: db            
         nodeName           指定的nodeName
**************************************************************************/
function getNodeNameInstanceid ( db, nodeName )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: nodeName }, { instanceid: "" } );
   while( cursor.next() )
   {
      var instanceid = cursor.current().toObj().instanceid;
   }
   cursor.close();
   return instanceid;
}

/************************************************************************
*@Description: 获取多个节点对应的instanceid
*@input: db            
         nodeName           指定的nodeName
**************************************************************************/
function getNodeNameInstanceids ( db, nodeNames )
{
   var instanceids = [];
   for( var i = 0; i < nodeNames.length; i++ )
   {
      var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: nodeNames[i] }, { instanceid: "" } );
      while( cursor.next() )
      {
         var instanceid = cursor.current().toObj().instanceid;
         instanceids.push( instanceid );
      }
      cursor.close();
   }
   return instanceids;
}