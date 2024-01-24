/************************************
*@Description: 插入不同记录, 数据页超过10页
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
import( "./basic_operation/commlib.js" );
import( "./main.js" );

function insertDiffDatas ( dbcl, insertNum )
{
   //插入不同记录
   var doc = [];
   for( var i = 0; i < insertNum; i++ )
   {
      doc.push(
         {
            a: i, a0: i, a1: i, a2: i, a3: i, a4: i, a5: i, a6: i, a7: i, a8: i, a9: i,
            a10: i, a11: i, a12: i, a13: i, a14: i, a15: i, a16: i, a17: i, a18: i, a19: i,
            a20: i, a21: i, a22: i, a23: i, a24: i, a25: i, a26: i, a27: i, a28: i, a29: i,
            a30: i, a31: i, a32: i, a33: i, a34: i, a35: i, a36: i, a37: i, a38: i, a39: i,
            a40: i, a41: i, a42: i, a43: i, a44: i, a45: i, a46: i, a47: i, a48: i, a49: i,
            a50: i, a51: i, a52: i, a53: i, a54: i, a55: i, a56: i, a57: i, a58: i, a59: i,
            a60: i, a61: i, a62: i, a63: i, a64: i, a65: i, a66: i, a67: i, a68: i, a69: i,
            b: i, c: "test" + i
         } );
   }
   dbcl.insert( doc );

}

/************************************
*@Description: 插入相同记录, 数据页超过10页
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function insertSameDatas ( dbcl, insertNum, value )
{
   //插入相同的记录
   var doc = [];
   for( var i = 0; i < insertNum; i++ )
   {
      doc.push(
         {
            a: value, a0: value, a1: value, a2: value, a3: value, a4: value, a5: value, a6: value, a7: value, a8: value, a9: value,
            a10: value, a11: value, a12: value, a13: value, a14: value, a15: value, a16: value, a17: value, a18: value, a19: value,
            a20: value, a21: value, a22: value, a23: value, a24: value, a25: value, a26: value, a27: value, a28: value, a29: value,
            a30: value, a31: value, a32: value, a33: value, a34: value, a35: value, a36: value, a37: value, a38: value, a39: value,
            a40: value, a41: value, a42: value, a43: value, a44: value, a45: value, a46: value, a47: value, a48: value, a49: value,
            a50: value, a51: value, a52: value, a53: value, a54: value, a55: value, a56: value, a57: value, a58: value, a59: value,
            a60: value, a61: value, a62: value, a63: value, a64: value, a65: value, a66: value, a67: value, a68: value, a69: value,
            b: value, c: "test" + value
         } );
   }
   dbcl.insert( doc );
}


/************************************
*@Description: 获取所有组的数据节点
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function getNodesInGroups ( db, groups )
{
   var datas = new Array();

   //standalone
   if( true === commIsStandalone( db ) )
   {
      datas[0] = Array();
      datas[0][0] = db;
   }
   else
   {
      for( var i = 0; i < groups.length; ++i )
      {
         datas[i] = Array();

         var rg = db.getRG( groups[i] );
         var rgDetail = eval( "( " + rg.getDetail().toArray()[0] + " )" );
         var nodesInGroup = rgDetail.Group;
         for( var j = 0; j < nodesInGroup.length; ++j )
         {
            var hostName = nodesInGroup[j].HostName;
            var serviceName = nodesInGroup[j].Service[0].Name;
            datas[i][j] = new Sdb( hostName, serviceName );
         }

      }
   }
   return datas;
}

/************************************
*@Description: 获取主节点的lsn
*@author:      liuxiaoxuan
*@createDate:  2017.01.29
**************************************/
function getPrimaryNodeLSNs ( db, groups )
{
   var datas = getNodesInGroups( db, groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "( " + nodesInGroup[j].snapshot( 6 ).toArray()[0] + " )" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( isPrimary )
         {
            LSNs[i][0] = completeLSN;
            break;
         }
      }
   }
   return LSNs;
}

/************************************
*@Description: 获取备节点的lsn
*@author:      liuxiaoxuan
*@createDate:  2017.01.29
**************************************/
function getSlaveNodeLSNs ( db, groups )
{
   var datas = getNodesInGroups( db, groups );

   var LSNs = new Array();
   for( var i = 0; i < datas.length; ++i )
   {
      var nodesInGroup = datas[i];
      LSNs[i] = Array();
      var f = 0;
      for( var j = 0; j < nodesInGroup.length; ++j )
      {
         var getSnapshot6 = eval( "( " + nodesInGroup[j].snapshot( 6 ).toArray()[0] + " )" );

         var completeLSN = getSnapshot6.CompleteLSN;
         var isPrimary = getSnapshot6.IsPrimary;
         if( !isPrimary )
         {
            LSNs[i][f++] = completeLSN;
         }
      }
   }

   return LSNs;
}

/************************************
*@Description: 检查cl所在组的主备节点lsn及index是否一致
*@Author:      XiaoNi Huang 2020.01.03
**************************************/
function checkConsistency ( db, csName, clName, groups )
{
   checkLSNConsistency( db, csName, clName, groups );
   checkIndexConsistency( csName, clName );
}

/************************************
*@Description: 检查cl所在组的主备节点lsn是否一致( 超时600s )
*@author:      liuxiaoxuan
*@createDate:  2017.01.29
**************************************/
function checkLSNConsistency ( db, csName, clName, groups )
{
   if( csName == null ) { var csName = "UNDEFINED"; }
   if( clName == null ) { var clName = "UNDEFINED"; }
   if( groups == undefined ) { var groups = getGroupsFromcl( db, csName + "." + clName ); }

   //the longest waiting time is 600S
   var lsnFlag = false;
   var timeout = 600;
   var doTimes = 0;

   //get primary nodes
   var primaryNodeLSNs = getPrimaryNodeLSNs( db, groups );
   while( true )
   {
      lsnFlag = checkLSN( db, groups, primaryNodeLSNs );
      if( !lsnFlag )
      {
         if( doTimes < timeout )
         {
            ++doTimes;
            sleep( 1000 );
         }
         else
         {
            throw new Error( "check lsn time out" );
         }
      }
      else
      {
         break;
      }
   }
}

/************************************
*@Description: 检查cl所在组的主备节点index是否一致( 超时600s )
*@Author:      XiaoNi Huang  2020.01.03
**************************************/
function checkIndexConsistency ( csName, clName )
{
   if( true === commIsStandalone( db ) )
   {
      return;
   }

   if( clName === null || typeof ( clName ) === "undefined" )
   {
      return;
   }

   var groupNames = commGetCLGroups( db, csName + "." + clName );

   // get indexes of master node
   var rg = db.getRG( groupNames[0] );
   var mstDB = rg.getMaster().connect();
   var mstIdxNames = getNodeIndexes( db, csName, clName );

   // get indexes of slave node, and compare to mstIdxNames 
   var timeout = 600;
   var doTimes = 0;
   var nodes = getNodesInGroups( db, groupNames );
   for( var i = 0; i < nodes.length; i++ )
   {
      for( var j = 0; j < nodes[i].length; j++ )
      {
         var nodeDB = nodes[i][j];
         try
         {
            if( JSON.stringify( mstDB ) !== JSON.stringify( nodeDB ) )
            {
               while( true )
               {
                  var slvIdxNames = getNodeIndexes( db, csName, clName );
                  if( JSON.stringify( mstIdxNames ) !== JSON.stringify( slvIdxNames ) )
                  {
                     if( doTimes < timeout )
                     {
                        ++doTimes;
                        sleep( 1000 );
                     }
                     else
                     {
                        throw new Error( "check index time out" );
                     }
                  }
                  else
                  {
                     break;
                  }
               }
            }
         }
         catch( e )
         {
            throw new Error( e );
         }
         finally
         {
            nodeDB.close();
         }
      }
   }
}

/************************************
*@Description: 获取索引列表
*@Author:      XiaoNi Huang  2020.01.03
**************************************/
function getNodeIndexes ( db, csName, clName )
{
   var idxNames = [];
   var cursor = db.getCS( csName ).getCL( clName ).listIndexes();
   while( cursor.next() )
   {
      var name = cursor.current().toObj().Name;
      idxNames.push( name );
   }
   return idxNames;
}

/************************************
*@Description: 获取当前主备节点lsn是否一致的状态
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function checkLSN ( db, groups, primaryNodeLSNs )
{
   var slaveNodeLSNs = getSlaveNodeLSNs( db, groups );

   var checkLSN = true;
   //比较主备节点lsn
   for( var i = 0; i < slaveNodeLSNs.length; ++i )
   {
      for( var j = 0; j < slaveNodeLSNs[i].length; ++j )
      {
         if( primaryNodeLSNs[i][0] > slaveNodeLSNs[i][j] )
         {
            checkLSN = false;
            return checkLSN;
         }
      }
   }

   return checkLSN;
}

/************************************
*@Description: 检查统计信息
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function checkStat ( db, csName, clName, indexName, clExistStat, indexExistStat, groups )
{
   if( clExistStat == undefined ) { clExistStat = true; }
   if( indexExistStat == undefined ) { indexExistStat = true; }
   if( groups == undefined ) { var groups = getGroupsFromcl( db, csName + "." + clName ); }

   //get all nodes
   var datas = getNodesInGroups( db, groups );

   //check each node
   for( var i = 0; i < datas.length; i++ )
   {
      var nodesInGroup = datas[i];
      for( var j = 0; j < nodesInGroup.length; j++ )
      {
         var clStatFlag = false;
         //检查cl统计表信息
         var clStats = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" ).find().toArray();

         //需要检查cl统计表信息时，统计表信息不能为空
         if( clExistStat === true && clStats.length < 1 )
         {
            throw new Error( "NO_CL_STAT" );
         }

         //cl统计表信息中存在统计信息且数据页不小于10
         for( var k = 0; k < clStats.length; k++ )
         {
            var clStat = eval( "( " + clStats[k] + " )" );
            var actualCSName = clStat.CollectionSpace;
            var actualCLName = clStat.Collection;
            var totalDataPages = clStat.TotalDataPages;
            if( actualCSName === csName &&
               actualCLName === clName &&
               totalDataPages > 10 )
            {
               clStatFlag = true;
            }
         }

         //是否存在cl统计表信息与预期结果校验
         if( ( clExistStat ^ clStatFlag ) === 1 )
         {
            throw new Error( "NO_CL_STAT" );
         }

         //检查索引统计表信息
         var indexStatFlag = false;
         var indexStats = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find().toArray();

         //需要检查索引统计表信息时，统计表信息不能为空
         if( indexExistStat === true && indexStats.length < 1 )
         {
            throw new Error( "NO_INDEX_STAT" );
         }

         //索引统计表信息中存在统计信息
         for( var h = 0; h < indexStats.length; h++ )
         {
            var indexStat = eval( "( " + indexStats[h] + " )" );
            var actualCSName = indexStat.CollectionSpace;
            var actualCLName = indexStat.Collection;
            var actualIndexName = indexStat.Index;
            if( actualCSName === csName &&
               actualCLName === clName &&
               actualIndexName === indexName &&
               indexStat.hasOwnProperty( 'MCV' ) )
            {
               indexStatFlag = true;
            }
         }

         //是否存在索引统计表信息与预期结果校验
         if( ( indexExistStat ^ indexStatFlag ) === 1 )
         {
            throw new Error( "NO_INDEX_STAT" );
         }

      }
   }
}

/************************************
*@Description: 按组获取表访问计划
*@author:      zhaoyu
*@createDate:  2017.11.10
**************************************/
function getCommonExplain ( dbcl, findConf, sortConf, hintConf )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( hintConf ) == "undefined" ) { hintConf = null; }

   //保存所有访问计划
   var explains = new Array();
   var rc = dbcl.find( findConf ).sort( sortConf ).hint( hintConf ).explain( { Run: true } ).toArray();
   var groupExplains = eval( "( " + rc + " )" );
   var explainObj = {};
   for( var f in groupExplains )
   {
      if( ( f == "ScanType" ) || ( f == "IndexName" ) || ( f == "ReturnNum" ) )
      {
         explainObj[f] = groupExplains[f];
      }
   }
   explains.push( explainObj );
   return explains;
}

/************************************
*@Description: 按组获取表访问计划
*@author:      zhaoyu
*@createDate:  2017.11.10
**************************************/
function getSplitExplain ( dbcl, findConf, sortConf, hintConf )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( hintConf ) == "undefined" ) { hintConf = null; }

   //保存所有访问计划
   var explains = new Array();
   var rc = dbcl.find( findConf ).sort( sortConf ).hint( hintConf ).explain( { Run: true } ).toArray();
   for( var i = 0; i < rc.length; i++ )
   {
      var groupExplains = eval( "( " + rc[i] + " )" );
      var explainObj = {};
      for( var f in groupExplains )
      {
         if( ( f == "GroupName" ) || ( f == "ScanType" ) || ( f == "IndexName" ) || ( f == "ReturnNum" ) )
         {
            explainObj[f] = groupExplains[f];
         }
      }
      explains.push( explainObj );
   }
   return explains;
}

/************************************
*@Description: 按组获取主子表访问计划
*@author:      zhaoyu
*@createDate:  2017.11.10
**************************************/
function getMainclExplain ( dbcl, findConf, sortConf, hintConf )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( hintConf ) == "undefined" ) { hintConf = null; }

   //保存主子表所有组的访问计划
   var explains = new Array();

   var rc = dbcl.find( findConf ).sort( sortConf ).hint( hintConf ).explain( { Run: true } ).toArray();
   for( var i = 0; i < rc.length; i++ )
   {
      //保存单个组的访问计划
      var groupExplains = eval( "( " + rc[i] + " )" );

      var groupName = groupExplains.GroupName;
      var subCollections = groupExplains.SubCollections;

      for( var j = 0; j < subCollections.length; j++ )
      {
         //保存单个组上一个子表的访问计划
         var explainObj = {};
         explainObj['GroupName'] = groupName;
         for( var f in subCollections[j] )
         {
            if( ( f == "Name" ) || ( f == "ScanType" ) || ( f == "IndexName" ) || ( f == "ReturnNum" ) )
            {
               explainObj[f] = subCollections[j][f];
            }
         }
         explains.push( explainObj );
      }
   }
   return explains;

}

/************************************
*@Description: 校验访问计划
*@author:      zhaoyu
*@createDate:  2017.11.8
**************************************/
function checkExplain ( actExplains, expExplains )
{
   //校验长度
   if( actExplains.length !== expExplains.length )
   {
      throw buildException( "check count", "COUNT_ERR", "check count failed!",
         expExplains.length, actExplains.length );
   }

   //校验访问计划，不校验元素顺序
   var newExpArray = new Array();
   var newActArray = new Array();
   for( var i = 0; i < expExplains.length; i++ )
   {
      var newObj1 = objSortByKey( actExplains[i] );
      newActArray.push( newObj1 );

      var newObj2 = objSortByKey( expExplains[i] );
      newExpArray.push( newObj2 );
   }

   for( var i = 0; i < expExplains.length; i++ )
   {
      assert.notEqual( JSON.stringify( newActArray ).indexOf( JSON.stringify( newExpArray[i] ) ), -1 );
   }
}

/************************************
*@Description: obj按照key排序
*@author:      zhaoyu
*@createDate:  2017.11.30
**************************************/
function objSortByKey ( obj )
{
   var newKey = Object.keys( obj ).sort();
   var newObj = {};
   for( var i = 0; i < newKey.length; i++ )
   {
      newObj[newKey[i]] = obj[newKey[i]];
   }
   return newObj;
}

/************************************
*@Description: get SrcGroup and TargetGroup info, the groups information
include GroupName, HostName and svcname
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function getSplitGroups ( csName, clName, targetGrMaxNums )
{
   var allGroupInfo = getGroupName( db, true );
   var srcGroupName = getSrcGroup( csName, clName );
   var splitGroups = new Array();
   if( targetGrMaxNums >= allGroupInfo.length - 1 )
   {
      targetGrMaxNums = allGroupInfo.length - 1;
   }
   var index = 1;

   for( var i = 0; i != allGroupInfo.length; ++i )
   {
      if( srcGroupName == allGroupInfo[i][0] )
      {
         splitGroups[0] = new Object();
         splitGroups[0].GroupName = allGroupInfo[i][0];
         splitGroups[0].HostName = allGroupInfo[i][1];
         splitGroups[0].svcname = allGroupInfo[i][2];
      }
      else
      {
         if( index > targetGrMaxNums )
         {
            continue;
         }
         splitGroups[index] = new Object();
         splitGroups[index].GroupName = allGroupInfo[i][0];
         splitGroups[index].HostName = allGroupInfo[i][1];
         splitGroups[index].svcname = allGroupInfo[i][2];
         index++;
      }
   }
   return splitGroups;

}

/************************************
*@Description: get the informations of the srcGroups and targetGroups, then split cl with different options, 
only split 1 times
return the informations of the srcGroups and targetGroups
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function ClSplitOneTimes ( csName, clName, startCondition, endCondition )
{

   var targetGroupNums = 1;
   var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
   var srcGrName = groupsInfo[0].GroupName;
   var tarGrName = groupsInfo[1].GroupName;
   var CL = db.getCS( csName ).getCL( clName );
   if( typeof ( startCondition ) === "number" )//percentage split
   {
      CL.split( srcGrName, tarGrName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && endCondition === undefined )//range split without end condition
   {
      CL.split( srcGrName, tarGrName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" )//range split with end condition
   {
      CL.split( srcGrName, tarGrName, startCondition, endCondition );
   }
   return groupsInfo;
}

/************************************
*@Description: get Group name and Service name
*@author：wuyan 2015/10/20
**************************************/
function getGroupName ( db, mustBePrimary )
{
   var RGname = db.listReplicaGroups().toArray();
   var j = 0;
   var arrGroupName = Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '( ' + RGname[i] + ' )' );
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = Array();
         var primaryNodeID = eRGname["PrimaryNode"];
         var groups = eRGname["Group"];
         for( var m = 0; m < groups.length; m++ )
         {
            if( true == mustBePrimary )
            {
               var nodeID = groups[m]["NodeID"];
               if( primaryNodeID != nodeID )
                  continue;
            }
            arrGroupName[j].push( eRGname["GroupName"] );
            arrGroupName[j].push( groups[m]["HostName"] );
            arrGroupName[j].push( groups[m]["Service"][0]["Name"] );
            break;
         }
         ++j;
      }
   }
   return arrGroupName;
}

/************************************
*@Description: get SrcGroup name, update getPG to getSrcGroup
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function getSrcGroup ( csName, clName )
{
   if( undefined == csName || undefined == clName )
   {
      throw new Error( "cs or cl name is undefined" );
   }
   var tableName = csName + "." + clName;
   var cataMaster = db.getCatalogRG().getMaster().toString().split( ":" );
   var catadb = new Sdb( cataMaster[0], cataMaster[1] );
   var Group = catadb.SYSCAT.SYSCOLLECTIONS.find().toArray();
   var srcGroupName;
   for( var i = 0; i < Group.length; ++i )
   {
      var eachID = eval( "( " + Group[i] + " )" );
      if( tableName == eachID["Name"] )
      {
         srcGroupName = eachID["CataInfo"][0]["GroupName"];
         break;
      }
   }
   return srcGroupName;
}

/************************************
*@Description: 手工修改索引统计表
*@author:      liuxiaoxuan
*@createDate:  2017.11.13
**************************************/
function updateIndexStateInfo ( db, csName, clName, indexName, mcvValues, fracs )
{
   var groups = getGroupsFromcl( db, csName + "." + clName );
   var datas = getNodesInGroups( db, groups );

   //update all nodes
   for( var i in datas )
   {
      var nodesInGroup = datas[i];
      for( var j in nodesInGroup )
      {
         var rec = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find().toArray();
         var rule = { "$set": { "MCV": { "Values": mcvValues, "Frac": fracs } } };
         var matcher = { "CollectionSpace": csName, "Collection": clName, "Index": indexName };
         nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).upsert( rule, matcher );
      }
   }
}

/************************************
*@Description: 执行查询
*@author:      liuxiaoxuan
*@createDate:  2017.01.18
**************************************/
function query ( dbcl, findConf, sortConf, hintConf, expRecordNum )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( hintConf ) == "undefined" ) { hintConf = null; }

   //执行查询并校验记录数
   var rc = dbcl.find( findConf ).sort( sortConf ).hint( hintConf );
   var count = 0;
   while( rc.next() )
   {
      count++;
   }
   assert.equal( count, expRecordNum );
}

/************************************
*@Description: 获取普通表访问计划快照
*@author:      liuxiaoxuan
*@createDate:  2017.01.18
**************************************/
function getCommonAccessPlans ( db, findConf, selectorConf, sortConf )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( selectorConf ) == "undefined" ) { selectorConf = null; }

   var accessPlans = new Array();
   //获取快照信息
   var rc = db.snapshot( 11, findConf, selectorConf, sortConf ).toArray();
   for( var i = 0; i < rc.length; i++ )
   {
      var accessPlan = eval( "( " + rc[i] + " )" );
      var accessPlanObj = {};
      for( var f in accessPlan )
      {
         if( ( f == "ScanType" ) || ( f == "IndexName" ) )
         {
            accessPlanObj[f] = accessPlan[f];
         }
      }

      accessPlans.push( accessPlanObj );
   }
   return accessPlans;
}

/************************************
*@Description: 获取切分表访问计划快照
*@author:      liuxiaoxuan
*@createDate:  2017.01.18
**************************************/
function getSplitAccessPlans ( db, findConf, selectorConf, sortConf )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( selectorConf ) == "undefined" ) { selectorConf = null; }

   var accessPlans = new Array();

   //获取快照信息
   var rc = db.snapshot( 11, findConf, selectorConf, sortConf ).toArray();
   for( var i = 0; i < rc.length; i++ )
   {
      var accessPlan = eval( "( " + rc[i] + " )" );

      //过滤hash、range切分时生成的查询计划
      if( accessPlan['IndexName'] == '$id' )
         continue;
      if( accessPlan['ScanType'] == 'tbscan'
         && JSON.stringify( accessPlan['Query'] ) == "{}" )
         continue;
      if( accessPlan['IndexName'] == '$shard'
         && JSON.stringify( accessPlan['Query'] ) == "{}" )
         continue;

      var accessPlanObj = {};
      for( var f in accessPlan )
      {
         if( ( f == "GroupName" ) || ( f == "ScanType" ) || ( f == "IndexName" ) )
         {
            accessPlanObj[f] = accessPlan[f];
         }
      }

      accessPlans.push( accessPlanObj );
   }
   return accessPlans;
}

/************************************
*@Description: 检查访问计划快照
              由于snapshot可能从正在重放的备节点拿，导致实际拿到的访问计划多于期待访问计划快照，暂无办法规避
              故无法做到全校验，只能期望结果是实际结果的子集
*@author:      liuxiaoxuan
*@createDate:  2018.01.15
**************************************/
function checkSnapShotAccessPlans ( clFullName, expectAccessPlans, actAccessPlans )
{
   var newExpAccessPlans = new Array();
   var newActAccessPlans = new Array();
   for( var i = 0; i < expectAccessPlans.length; i++ )
   {
      var newObj1 = objSortByKey( expectAccessPlans[i] );
      newExpAccessPlans.push( newObj1 );
   }
   for( var i = 0; i < actAccessPlans.length; i++ )
   {
      var newObj2 = objSortByKey( actAccessPlans[i] );
      newActAccessPlans.push( newObj2 );
   }
   var actStr = JSON.stringify( newActAccessPlans );
   for( var i = 0; i < newExpAccessPlans.length; i++ )
   {
      // 期望是实际的子集
      var expStr = JSON.stringify( newExpAccessPlans[i] );
      if( actStr.indexOf( expStr ) === -1 )
      {
         throw new Error( "check access plan fail" + "\nexp:" + JSON.stringify( newExpAccessPlans ) + "\nact:", actStr );
      }
   }
}
/************************************
*@Description: 按组获取主子表访问计划快照
*@author:      zhaoyu
*@createDate:  2018.1.24
**************************************/
function getMainclAccessPlans ( db, findConf, sortConf, selectorConf )
{
   if( typeof ( findConf ) == "undefined" ) { findConf = null; }
   if( typeof ( sortConf ) == "undefined" ) { sortConf = null; }
   if( typeof ( selectorConf ) == "undefined" ) { selectorConf = null; }

   //保存主子表所有组的访问计划
   var accessPlans = new Array();

   var rc = db.snapshot( 11, findConf, sortConf, selectorConf ).toArray();
   for( var i = 0; i < rc.length; i++ )
   {
      //保存单个组的访问计划快照
      var groupAccessPlans = eval( "( " + rc[i] + " )" );
      var accessPlanObj = {};
      for( var f in groupAccessPlans )
      {
         if( f == "GroupName" || f == "ScanType" || f == "IndexName" )
         {
            accessPlanObj[f] = groupAccessPlans[f];
         }
      }
      accessPlans.push( accessPlanObj );
   }
   return accessPlans;
}

/************************************
*@Description: 检查访问计划快照
*@author:      liuxiaoxuan
*@createDate:  2018.01.15
**************************************/
function checkMainclAccessPlans ( expAccessPlans, actAccessPlans )
{
   var newExpAccessPlans = new Array();
   var newActAccessPlans = new Array();
   for( var i = 0; i < expAccessPlans.length; i++ )
   {
      var newObj1 = objSortByKey( expAccessPlans[i] );
      newExpAccessPlans.push( newObj1 );
   }
   for( var i = 0; i < actAccessPlans.length; i++ )
   {
      var newObj2 = objSortByKey( actAccessPlans[i] );
      newActAccessPlans.push( newObj2 );
   }
   var actStr = JSON.stringify( newActAccessPlans );
   for( var i = 0; i < newExpAccessPlans.length; i++ )
   {
      // 期望是实际的子集
      var expStr = JSON.stringify( newExpAccessPlans[i] );
      if( actStr.indexOf( expStr ) === -1 )
      {
         throw new Error( "check access plan fail" + "\nexp:" + JSON.stringify( newExpAccessPlans ) + "\nact:", actStr );
      }
   }
}

/************************************
*@Description: split
*@author:      zhaoyu
*@createdate:  2018.1.25
**************************************/
function split ( csName, clName, srcGroupName, desGroupName, startCondition, endCondition )
{
   var CL = db.getCS( csName ).getCL( clName );
   if( typeof ( startCondition ) === "number" )//percentage split
   {
      CL.split( srcGroupName, desGroupName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && endCondition === undefined )//range split without end condition
   {
      CL.split( srcGroupName, desGroupName, startCondition );
   }
   else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" )//range split with end condition
   {
      CL.split( srcGroupName, desGroupName, startCondition, endCondition );
   }
}

/************************************
*@Description: get groups from cl
*@author:      zhaoyu
*@createdate:  2019.3.27
**************************************/
function getGroupsFromcl ( db, clFullName )
{
   var tmpArray = commGetCLGroups( db, clFullName );
   var groupNames = new Array();
   for( var i = 0; i < tmpArray.length; i++ )
   {
      if( groupNames.indexOf( tmpArray[i] ) === -1 )
      {
         groupNames.push( tmpArray[i] );
      }
   }
   return groupNames;
}


/************************************
*@Description: 检查统计信息, 优化用例后新增加方法
*@author:      zhaoyu
*@createDate:  2019.3.27
**************************************/
function checkStats ( db, csName, clNames, indexName, clExistStat, indexExistStat, groups )
{
   if( clExistStat == undefined ) { clExistStat = true; }
   if( indexExistStat == undefined ) { indexExistStat = true; }
   if( groups == undefined ) { var groups = getGroupsFromcl( db, csName + "." + clNames[0] ); }

   var indexNames = new Array();
   if( Array.isArray( indexName ) == true )
   {
      indexNames = indexName;
   }
   else
   {
      indexNames.push( indexName );
   }

   //get all nodes
   var datas = getNodesInGroups( db, groups );
   //check each node
   for( var i = 0; i < datas.length; i++ )
   {
      var nodesInGroup = datas[i];
      for( var j = 0; j < nodesInGroup.length; j++ )
      {
         //检查cl统计表信息
         var clStats = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" ).find().toArray();

         //需要检查cl统计表信息时，统计表信息不能为空
         if( clExistStat === true && clStats.length < 1 )
         {
            throw new Error( "NO_CL_STAT" );
         }

         //cl统计表信息中存在统计信息且数据页不小于10
         var clExistsStatCount = 0;
         for( var k = 0; k < clStats.length; k++ )
         {
            var clStat = eval( "( " + clStats[k] + " )" );
            var actualCSName = clStat.CollectionSpace;
            var actualCLName = clStat.Collection;
            var totalDataPages = clStat.TotalDataPages;
            if( actualCSName === csName &&
               -1 != clNames.indexOf( actualCLName ) && totalDataPages > 10 )
            {
               clExistsStatCount++;
            }
         }

         //是否存在cl统计表信息与预期结果校验
         if( clExistStat && clExistsStatCount !== clNames.length )
         {
            throw new Error( "CL_HAS_STAT_ERR" );
         }
         if( !clExistStat && clExistsStatCount !== 0 )
         {
            throw new Error( "CL_HAS_NO_STAT_ERR" );
         }

         //检查索引统计表信息
         var indexStats = nodesInGroup[j].getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ).find().toArray();

         //需要检查索引统计表信息时，统计表信息不能为空
         if( indexExistStat === true && indexStats.length < 1 )
         {
            throw new Error( "NO_INDEX_STAT" );
         }

         //索引统计表信息中存在统计信息
         var indexExistsStatCount = 0;
         for( var h = 0; h < indexStats.length; h++ )
         {
            var indexStat = eval( "( " + indexStats[h] + " )" );
            var actualCSName = indexStat.CollectionSpace;
            var actualCLName = indexStat.Collection;
            var actualIndexName = indexStat.Index;
            if( actualCSName === csName &&
               -1 != clNames.indexOf( actualCLName ) &&
               -1 != indexNames.indexOf( actualIndexName ) &&
               indexStat.hasOwnProperty( 'MCV' ) )
            {
               indexExistsStatCount++;
            }
         }

         //是否存在索引统计表信息与预期结果校验
         if( indexExistStat && indexExistsStatCount !== clNames.length * indexNames.length )
         {
            throw new Error( "INDEX_HAS_STAT_ERR" );
         }

         if( !indexExistStat && indexExistsStatCount !== 0 )
         {
            throw new Error( "INDEX_HAS_NO_STAT_ERR" );
         }
      }
   }

}

/************************************
*@Description: 判断所有数据组中是否有任意1个组存在1节点
*@author:      liuxiaoxuan
*@createDate:  2020.2.25
**************************************/
function isOnlyOneNodeInGroup ()
{
   //获取所有数据组
   var allGroupObj = db.listReplicaGroups();
   //遍历所有数据组
   while( allGroupObj.next() )
   {
      var rec = allGroupObj.current().toObj();
      if( rec.GroupName == 'SYSCatalogGroup' || rec.GroupName == 'SYSCoord' )
      {
         continue;
      }

      //判断1节点模式
      if( 1 == rec.Group.length ) 
      {
         return true;
      }
   }

   return false;
}
