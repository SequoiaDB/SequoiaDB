/******************************************************************************
 * @Description   : seqDB-24369:相同节点创建多个本地索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2021.09.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24369";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexNames = [];
   var recsNum = 63;
   var indexName = "index_24369_";

   // 插入数据，每个索引字段插入100条数据
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      for( var j = 0; j < 100; j++ )
      {
         var bson = {};
         bson['a' + i] = j;
         docs.push( bson );
      }
   }
   dbcl.insert( docs );

   // 创建大量索引
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );
   var indexDef = new Object();
   for( var i = 0; i < recsNum; i++ )
   {
      indexDef["a" + i] = 1;
      dbcl.createIndex( indexName + i, indexDef, { Standalone: true }, { NodeName: nodeName } );
      delete indexDef["a" + i];
      indexNames.push( indexName + i );
   }

   // 查看索引任务信息
   for( var i = 0; i < indexNames.length; i++ )
   {
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexNames[i], nodeName, true );
   }

   checkListIndexes( COMMCSNAME, testConf.clName, nodeName, indexNames );

   // 随机匹配一个字段查询
   var key = parseInt( Math.random() * recsNum );
   var value = parseInt( Math.random() * 100 );
   var bson = {};
   bson["a" + key] = value;
   var actResult = dbcl.find( bson );
   var expResult = [bson];
   commCompareResults( actResult, expResult );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, bson, "ixscan", indexName + key );
}

function checkListIndexes ( csName, clName, nodeName, indexNames )
{
   var data = new Sdb( nodeName );
   var dbcl = data.getCS( csName ).getCL( clName );
   var cur = dbcl.listIndexes();
   var actIndexes = [];
   while( cur.next() )
   {
      var indexName = cur.current().toObj().IndexDef.name;
      if( indexName != "$id" )
      {
         assert.equal( cur.current().toObj().IndexDef.Standalone, true, JSON.stringify( cur.current().toObj().IndexDef ) );
         actIndexes.push( indexName );
      }
   }
   assert.equal( actIndexes, indexNames );
   cur.close();
   data.close();
}