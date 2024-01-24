/******************************************************************************
 * @Description   : seqDB-25514:设置PerferredConstraint为SecondaryOnly，指定备节点实例S，该备节点升主 
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.04.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25514";
testConf.clName = COMMCLNAME + "_25514";
testConf.clOpt = { ReplSize: -1 };
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );

   // 获取cl所在group的备节点
   var slaveNodeName = getGroupSlaveNodeName( db, srcGroup );

   // 修改会话属性，访问备节点，指定访问实例为S
   var options = { "PreferredConstraint": "SecondaryOnly", "PreferredInstance": "S" };
   db.setSessionAttr( options );

   // 查看访问计划，访问节点为备节点
   var explainSlaveNode = explainAndReturnAccessNodes( dbcl, slaveNodeName );

   // 重新选主，使第一次访问的备节点升主
   var nodeInfo = explainSlaveNode[0].split( ":" );
   var rg = db.getRG( srcGroup );
   rg.reelect( { "HostName": nodeInfo[0], "ServiceName": nodeInfo[1] } );
   commCheckBusinessStatus( db );

   // 重新获取备节点
   var slaveNodeNameNew = getGroupSlaveNodeName( db, srcGroup );
   assert.notEqual( slaveNodeName, slaveNodeNameNew );

   // 多次查询访问计划
   var explainNum = 10;
   for( var i = 0; i < explainNum; i++ )
   {
      explainAndCheckAccessNodes( dbcl, slaveNodeNameNew );
   }
}

function explainAndReturnAccessNodes ( cl, expAccessNodes )
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

   return actAccessNodes;
}