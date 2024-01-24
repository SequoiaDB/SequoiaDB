/******************************************************************************
 * @Description   : seqDB-24041:创建一致性索引，快照查询,使用匹配符
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_24041";
   var clName1 = "cl_24041_1";
   var clName2 = "cl_24041_2";
   var indexName1 = "index_24041_1";
   var indexName2 = "index_24041_2";
   var indexName3 = "index_24041_3";
   var indexName4 = "index_24041_4";

   commDropCS( db, csName );
   var dbcl1 = commCreateCL( db, csName, clName1 );
   var dbcl2 = commCreateCL( db, csName, clName2, { ShardingKey: { a: 1 }, AutoSplit: true } );

   // 创建索引并校验一致性
   dbcl1.createIndex( indexName1, { a: 1 }, true, true );
   dbcl1.createIndex( indexName2, { b: 1 } );
   dbcl2.createIndex( indexName3, { c: 1 }, { NotNull: true } );
   dbcl2.createIndex( indexName4, { d: 1 }, { NotArray: true } );
   commCheckIndexConsistent( db, csName, clName1, indexName1, true );
   commCheckIndexConsistent( db, csName, clName1, indexName2, true );
   commCheckIndexConsistent( db, csName, clName2, indexName3, true );
   commCheckIndexConsistent( db, csName, clName2, indexName4, true );

   var nodes1 = getCLNodes( db, csName, clName1 );
   var nodes2 = getCLNodes( db, csName, clName2 );

   // 查询索引快照，指定匹配条件
   var explainObj = dbcl1.snapshotIndexes( { "IndexDef.name": indexName1 } );
   var attribute = { "Unique": true, "Enforced": true, "NotNull": false, "NotArray": false };
   checkSnapshotIndexes( explainObj, indexName1, { a: 1 }, attribute, nodes1 );

   var explainObj = dbcl1.snapshotIndexes( { "IndexDef.name": indexName2 } );
   var attribute = { "Unique": false, "Enforced": false, "NotNull": false, "NotArray": false };
   checkSnapshotIndexes( explainObj, indexName2, { b: 1 }, attribute, nodes1 );

   var explainObj = dbcl2.snapshotIndexes( { "IndexDef.key": { c: 1 } } );
   var attribute = { "Unique": false, "Enforced": false, "NotNull": true, "NotArray": false };
   checkSnapshotIndexes( explainObj, indexName3, { c: 1 }, attribute, nodes2 );

   var explainObj = dbcl2.snapshotIndexes( { "IndexDef.name": indexName4 } );
   var attribute = { "Unique": false, "Enforced": false, "NotNull": false, "NotArray": true };
   checkSnapshotIndexes( explainObj, indexName4, { d: 1 }, attribute, nodes2 );

   commDropCS( db, csName );
}

function checkSnapshotIndexes ( explainObj, indexName, key, attribute, nodes )
{
   var indexDef = explainObj.current().toObj().IndexDef;
   assert.equal( indexDef.name, indexName );
   assert.equal( indexDef.key, key );
   assert.equal( indexDef.unique, attribute.Unique );
   assert.equal( indexDef.enforced, attribute.Enforced );
   assert.equal( indexDef.NotNull, attribute.NotNull );
   assert.equal( indexDef.NotArray, attribute.NotArray );
   assert.equal( explainObj.current().toObj().Nodes.sort( sortBy( "NodeName" ) ), nodes.sort( sortBy( "NodeName" ) ) );
   explainObj.close();
}

// 按照snapshotIndexes显示节点形式返回cl所在的节点
function getCLNodes ( db, csName, clName )
{
   var nodes = [];
   var groups = commGetCLGroups( db, csName + "." + clName );

   for( var i = 0; i < groups.length; i++ )
   {
      var node = commGetGroupNodes( db, groups[i] );
      for( var j = 0; j < node.length; j++ )
      {
         nodes.push( { "NodeName": node[j].HostName + ":" + node[j].svcname, "GroupName": groups[i] } );
      }
   }
   return nodes;
}