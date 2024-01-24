/******************************************************************************
 * @Description   : seqDB-33895:存在唯一索引，数据页多但记录少时查询数据查看访问计划
 * @Author        : liuli
 * @CreateTime    : 2023.11.17
 * @LastEditTime  : 2023.11.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_33895";

main( test );
function test ()
{
   var cl = testPara.testCL;
   var indexName = "index_33895";
   cl.createIndex( indexName, { "a": 1, "b": 1 }, { unique: true } );

   var docs = [];
   for( i = 0; i < 20; i++ )
   {
      docs.push( { a: "" + i * 100 + "123456789012345678901234567890" + i * 100, b: -i } );
   }
   cl.insert( docs );

   docs = [];
   for( i = 1; i < 5628; i++ )
   {
      docs.push( { a: "" + i + "123456789012345678901234567890" + i, b: i, c: i, d: i, e: i, f: i, g: i, h: i, i: i, j: i, k: i, m: i, n: i, o: i, p: i, q: i, r: i, s: i, t: i, u: i } );
   }
   cl.insert( docs );

   // 删除第二次插入的数据，使得数据页多但记录少
   cl.remove( { b: { $gte: 0 } } );

   // 收集统计信息
   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );

   // 查看访问集合
   var cursor = cl.find( { a: "16001234567890123456789012345678901600", b: -16 } ).explain();
   while( cursor.next() )
   {
      var listIndex = cursor.current().toObj();
   }
   cursor.close();

   var expectType = "ixscan";
   var scanType = listIndex.ScanType;
   var acrIndexName = listIndex.IndexName;
   assert.equal( scanType, expectType );
   assert.equal( acrIndexName, indexName );
}