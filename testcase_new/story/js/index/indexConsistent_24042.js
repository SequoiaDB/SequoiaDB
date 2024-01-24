/******************************************************************************
 * @Description   : seqDB-24042:创建一致性索引，快照查询，指定选择条件
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24042";

main( test );
function test ()
{
   var csName = "cs_24042";
   var clName = "cl_24042";
   var indexName1 = "index_24042_1";
   var indexName2 = "index_24042_2";
   var indexName3 = "index_24042_3";
   var indexName4 = "index_24042_4";

   commDropCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );

   // 创建索引并校验一致性
   dbcl.createIndex( indexName1, { a: 1 }, true, true );
   dbcl.createIndex( indexName2, { b: 1 } );
   dbcl.createIndex( indexName3, { c: 1 }, { NotNull: true } );
   dbcl.createIndex( indexName4, { d: 1 }, { NotArray: true } );
   commCheckIndexConsistent( db, csName, clName, indexName1, true );
   commCheckIndexConsistent( db, csName, clName, indexName2, true );
   commCheckIndexConsistent( db, csName, clName, indexName3, true );
   commCheckIndexConsistent( db, csName, clName, indexName4, true );

   // 索引快照指定返回索引名
   var actName = [];
   var indexNames = ["$id", indexName1, indexName2, indexName3, indexName4];
   var explainObj = dbcl.snapshotIndexes( {}, { "IndexDef.name": "" } );
   while( explainObj.next() )
   {
      var indexDef = explainObj.current().toObj().IndexDef;
      actName.push( indexDef.name );
   }
   explainObj.close();
   assert.equal( actName.sort(), indexNames.sort() );

   // 索引快照指定按顺序返回索引键
   var actKey = [];
   var expKey = [{ "_id": 1 }, { a: 1 }, { b: 1 }, { c: 1 }, { d: 1 }];
   var explainObj = dbcl.snapshotIndexes( {}, { "IndexDef.key": "" }, { "IndexDef.key": 1 } );
   while( explainObj.next() )
   {
      var indexDef = explainObj.current().toObj().IndexDef;
      actKey.push( indexDef.key );
   }
   explainObj.close();

   assert.equal( actKey, expKey );
   commDropCS( db, csName );
}