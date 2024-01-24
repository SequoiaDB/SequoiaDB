/******************************************************************************
 * @Description   : seqDB-24668 : 插入索引数据（索引页reorg后空间不足且插入pos改变）
 * @Author        : Wu Yan
 * @CreateTime    : 2021.11.17
 * @LastEditTime  : 2021.11.20
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_24668";
testConf.csOpt = { PageSize: 4096 };
testConf.clName = COMMCLNAME + "_24668";

main( test );
function test ( testPara )
{
   var indexName = "index24668";
   var cl = testPara.testCL;

   //插入数据，构造生成索引的空间较大且满足有多个子节点
   var arr = new Array( 500 );
   var test = arr.join( 'a' );
   var insertNum1 = 60;
   for( var i = 0; i < insertNum1; i++ )
   {
      cl.insert( { a: i, b: test } );
   }
   // 创建索引
   cl.createIndex( indexName, { a: 1, b: 1 } );

   //删除存在unused键值左有节点数据和unused键值对应数据
   var unusedValue = 35;
   var removeEndNo = 53;
   cl.remove( { a: { $lt: unusedValue } } );
   cl.remove( { a: { $gt: unusedValue, $lt: removeEndNo } } );
   cl.remove( { a: unusedValue } );

   //再次插入索引数据落到上一步删除数据位置
   var insertEndNo = 300;
   for( var i = insertNum1; i < insertEndNo; i++ )
   {
      cl.insert( { a: i, b: test } );
   }

   //插入索引记录需要split且插入的键值在删除的unused键值左节点
   var arr = new Array( 1000 );
   var test = arr.join( 'a' );
   var doc = { a: 0, b: test };
   cl.insert( doc );
   var cursor = cl.find( doc, { "_id": { "$include": 0 } } ).hint( { "": indexName } );
   var expRecs = [doc];
   commCompareResults( cursor, expRecs );
}


