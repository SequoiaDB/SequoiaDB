/******************************************************************************
 * @Description   : seqDB-24040:创建一致性索引，快照查询
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24040";

main( test );

function test ( testPara )
{

   var cl = testPara.testCL;
   var indexName1 = "index_24040_1";
   var indexName2 = "index_24040_2";
   var indexName3 = "index_24040_3";

   cl.createIndex( indexName1, { a: 1 }, true, true );
   cl.createIndex( indexName2, { b: 1 } );
   cl.createIndex( indexName3, { c: 1 }, { NotNull: true } );

   var actName = [];
   var actKey = [];
   var cursor = cl.snapshotIndexes();
   while( cursor.next() )
   {
      var name = cursor.current().toObj().IndexDef.name;
      actName.push( name );
      var key = cursor.current().toObj().IndexDef.key;
      actKey.push( key );
   }
   cursor.close();

   var expName = ["$id", indexName1, indexName2, indexName3];
   var expKey = [{ "_id": 1 }, { "a": 1 }, { "b": 1 }, { "c": 1 }];
   commCompareObject( actName.sort(), expName.sort() );
   commCompareObject( actKey.sort(), expKey.sort() );
}