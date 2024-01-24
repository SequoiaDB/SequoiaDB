/******************************************************************************
 * @Description   : seqDB-25505:设置PerferredConstraint为SecondaryOnly，指定多个instance实例顺序选取（其中第一个为主节点instanceid）
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25505";
testConf.clName = COMMCLNAME + "_25505";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );

   var group = args.srcGroupName;

   var nodes = commGetGroupNodes( db, group );
   var nodeNames = [];
   for( var i in nodes )
   {
      nodeNames.push( nodes[i].HostName + ":" + nodes[i].svcname );
   }

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, group, instanceids );

      // 获取所有节点的instanceid
      var instanceids = getNodeNameInstanceids( db, nodeNames );

      // 获取主节点的nodeName
      var masterNodeName = getGroupMasterNodeName( db, group );
      // 将主节点的instanceid排在第一位
      var instanceidsNew = masterInstanceidInFirst( instanceids, nodeNames, masterNodeName );

      var options = { "PreferredConstraint": "SecondaryOnly", "PreferredInstance": instanceidsNew, "PreferredInstanceMode": "ordered" };
      // 修改会话属性
      db.setSessionAttr( options );

      // 获取第二个instanceid对应的节点
      var secondNodeName = getSecondNodeName( db, instanceidsNew[1] );

      // 查看访问计划，访问节点为备节点
      explainAndCheckAccessNodes( dbcl, secondNodeName );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: group }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( group ).stop();
      db.getRG( group ).start();
      commCheckBusinessStatus( db );
   }
}

function masterInstanceidInFirst ( instanceids, nodeNames, masterNodeName )
{
   var masterInstanceidInFirst = [];
   for( var i in instanceids )
   {
      if( nodeNames[i] == masterNodeName )
      {
         masterInstanceidInFirst.push( instanceids[i] );
         instanceids.splice( i, 1 );
      }
   }

   for( var i in instanceids )
   {
      masterInstanceidInFirst.push( instanceids[i] );
   }

   return masterInstanceidInFirst;
}

function getSecondNodeName ( db, secondInstanceid )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { instanceid: secondInstanceid }, { NodeName: "" } );
   while( cursor.next() )
   {
      var secondNodeName = cursor.current().toObj().NodeName;
   }
   cursor.close();
   return secondNodeName;
}
