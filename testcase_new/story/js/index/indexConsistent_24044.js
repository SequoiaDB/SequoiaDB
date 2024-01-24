/******************************************************************************
 * @Description   : seqDB-24044:创建一致性索引，查询索引快照，指定匹配符、选择条件、排序
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24044";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var indexName1 = "index_24043_1";
   var indexName2 = "index_24043_2";
   var indexName3 = "index_24043_3";
   var indexName4 = "index_24043_4";

   // 创建索引并校验一致性
   dbcl.createIndex( indexName1, { a: 1 }, true, true );
   dbcl.createIndex( indexName2, { b: 1 } );
   dbcl.createIndex( indexName3, { c: 1 }, { NotNull: true } );
   dbcl.createIndex( indexName4, { d: 1 }, { NotArray: true } );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24044", indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24044", indexName2, true );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24044", indexName3, true );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24044", indexName4, true );

   // 指定匹配和选择
   var cursor = dbcl.snapshotIndexes( { "IndexDef.name": indexName1 }, { "IndexDef.name": "" } );
   assert.equal( cursor.current().toObj(), { "IndexDef": { "name": indexName1 } } );

   // 指定选择和排序
   var indexNames = ["$id", indexName1, indexName2, indexName3, indexName4];
   var cursor = dbcl.snapshotIndexes( {}, { "IndexDef.name": "" }, { "IndexDef.name": 1 } );
   var actName = [];
   while( cursor.next() )
   {
      actName.push( cursor.current().toObj().IndexDef.name );
   }
   cursor.close();
   assert.equal( actName, indexNames.sort() );
}