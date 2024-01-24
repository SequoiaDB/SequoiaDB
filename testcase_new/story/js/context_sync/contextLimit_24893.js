/******************************************************************************
 * @Description   : seqDB-24893:节点shard平面开启上下文超过限制   
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.05
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_context_24893";

main( test );

function test ( testPara )
{
   var varCL = testPara.testCL;
   var maxContextnum = 1000;
   var maxsessioncontextnum = 100;
   var nodes = commGetGroupNodes( db, "SYSCoord" )
   if( nodes.length < 2 )
   {
      return;
   }
   insertData( varCL, 1000 );

   var config = { "maxcontextnum": maxContextnum };
   db.updateConf( config );

   var sdbs = [];
   try
   {
      // 链接第一个coord节点打开500个上下文
      for( var i = 0; i < 5; i++ )
      {
         var db1 = new Sdb( nodes[0].HostName, nodes[0].svcname );
         sdbs.push( db1 );
         var cl1 = db1.getCS( testConf.csName ).getCL( testConf.clName );
         for( var j = 0; j < maxsessioncontextnum; j++ )
         {
            var cursor = cl1.find();
            var obj = cursor.next();
         }
      }
      // 链接第二个coord节点打开600个上下文
      for( var i = 0; i < 6; i++ )
      {
         var db2 = new Sdb( nodes[1].HostName, nodes[1].svcname );
         sdbs.push( db2 );
         var cl2 = db2.getCS( testConf.csName ).getCL( testConf.clName );
         for( var j = 0; j < maxsessioncontextnum; j++ )
         {
            var cursor = cl2.find();
            var obj = cursor.next();
         }
      }
   } finally
   {
      db.deleteConf( { "maxcontextnum": 1 } );
      if( sdbs.length !== 0 )
      {
         for( var i in sdbs )
         {
            var db1 = sdbs[i];
            db1.close();
         }
      }
   }
}