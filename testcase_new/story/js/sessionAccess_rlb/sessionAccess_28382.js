/******************************************************************************
 * @Description   : seqDB-28382:设置会话访问属性，指定instanceID对应组内多个节点
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.10.22
 * @LastEditTime  : 2022.11.10
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_28382";
testConf.clName = COMMCLNAME + "_28382";
testConf.clOpt = { ReplSize: -1 };
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var cl = args.testCL;
   cl.insert( { a: 1 } );
   var srcGroup = args.srcGroupName;

   try
   {
      var config = { instanceid: 1 };
      var options = { GroupName: srcGroup };
      updateConf( db, config, options, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( srcGroup ).stop();
      db.getRG( srcGroup ).start();
      commCheckBusinessStatus( db );

      // 获取 cl 所在节点
      var fullclName = testConf.csName + "." + testConf.clName;
      var nodes = commGetCLNodes( db, fullclName );
      var nodeArr = {};
      for( var i = 0; i < nodes.length; i++ )
      {
         nodeArr[nodes[i]["HostName"] + ":" + nodes[i]["svcname"]] = 0;
      }

      var recordNum = 9000;
      var options = { PreferredInstance: 1, PreferredInstanceMode: "ordered", PreferredPeriod: 0 };
      db.setSessionAttr( options );
      for( var i = 0; i < recordNum; i++ ) 
      {
         var cursor = cl.find().explain();
         var node = cursor.current().toObj()["NodeName"];
         if( node in nodeArr ) 
         {
            nodeArr[node] += 1;
         }
      }

      // 误差率控制在0.5内
      // 期望数的计算方式: 总的记录数 / 复制组中节点的个数, 误差率的计算方式: Math.abs(节点的记录数 - 期望数) / 期望数
      var count = nodes.length;
      var expNum = recordNum / count;
      for( var node in nodeArr ) 
      {
         if( Math.abs( nodeArr[node] - expNum ) / expNum >= 0.5 )
         {
            throw new Error( JSON.stringify( nodeArr ) + ", error rate should be less than 0.5" );
         }
      }
   }
   finally
   {
      var config = { instanceid: 1 };
      var options = { Group: srcGroup };
      var errArr = [SDB_COORD_NOT_ALL_DONE, SDB_RTN_CONF_NOT_TAKE_EFFECT];
      deleteConf( db, config, options, errArr );
      db.getRG( srcGroup ).stop();
      db.getRG( srcGroup ).start();
      commCheckBusinessStatus( db );
   }
}

function deleteConf ( db, config, options, errArr )
{
   try
   {
      db.deleteConf( config, options );
   }
   catch( e )
   {
      if( errArr.indexOf( Number( e ) ) == -1 )
      {
         throw e;
      }
   }
}