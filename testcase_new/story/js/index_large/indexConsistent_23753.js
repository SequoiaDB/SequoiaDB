/******************************************************************************
 * @Description   : seqDB-23753:创建多个索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23753";

main( test );
function test ( testPara )
{
   var indexName = "idx23753_";
   var indexNames = [];
   var recsNum = 63;
   var cl = testPara.testCL;

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
   cl.insert( docs );

   // 创建大量索引
   var indexDef = new Object();
   for( var i = 0; i < recsNum; i++ )
   {
      indexDef["a" + i] = 1;
      cl.createIndex( indexName + i, indexDef );
      delete indexDef["a" + i];
      indexNames.push( indexName + i );
   }

   // 校验任务和索引一致性
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexNames, 0 );
   for( var i = 0; i < indexNames.length; i++ )
   {
      commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexNames[i], true );
   }

   // 随机匹配一个字段查询
   var key = parseInt( Math.random() * recsNum );
   var value = parseInt( Math.random() * 100 );
   var bson = {};
   bson["a" + key] = value;
   var actResult = cl.find( bson );
   var expResult = [bson];
   commCompareResults( actResult, expResult );
   checkExplain( cl, bson, "ixscan", indexName + key );

   // 再次创建一个索引
   assert.tryThrow( SDB_DMS_MAX_INDEX, function()
   {
      cl.createIndex( indexName + 64, { b: 1 } );
   } );

   // 校验任务和索引一致性
   checkIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexName + 64, SDB_DMS_MAX_INDEX );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName + 64, false );
}