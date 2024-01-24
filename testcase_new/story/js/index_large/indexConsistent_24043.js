/******************************************************************************
 * @Description   : seqDB-24043:创建一致性索引，快照查询，指定排序
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_24043";
   var mainCLName = "maincl_24043";
   var subCLName1 = "subcl_24043_1";
   var subCLName2 = "subcl_24043_2";
   var indexName1 = "index_24043_1";
   var indexName2 = "index_24043_2";
   var indexName3 = "index_24043_3";
   var indexName4 = "index_24043_4";
   var indexNames = [indexName1, indexName2, indexName3, indexName4];

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 50 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 50 }, UpBound: { a: 100 } } );

   // 创建索引并校验一致性
   maincl.createIndex( indexName1, { a: 1 }, true, true );
   maincl.createIndex( indexName2, { b: 1 } );
   maincl.createIndex( indexName3, { c: 1 }, { NotNull: true } );
   maincl.createIndex( indexName4, { d: 1 }, { NotArray: true } );

   for( var i = 0; i < indexNames.length; i++ )
   {
      commCheckIndexConsistent( db, csName, subCLName1, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName2, indexNames[i], true );
   }

   // 获取所有子表所在的节点并去重
   var mainNodes = getCLNodes( db, csName, [subCLName1, subCLName2] );
   var attribute = [];
   var indexNames = ["$id", "$shard", indexName1, indexName2, indexName3, indexName4];
   var keys = [{ "_id": 1 }, { "a": 1 }, { "a": 1 }, { "b": 1 }, { "c": 1 }, { "d": 1 }];
   attribute.push( { "Unique": true, "Enforced": true, "NotNull": false, "NotArray": true } );
   attribute.push( { "Unique": false, "Enforced": false, "NotNull": false, "NotArray": false } );
   attribute.push( { "Unique": true, "Enforced": true, "NotNull": false, "NotArray": false } );
   attribute.push( { "Unique": false, "Enforced": false, "NotNull": false, "NotArray": false } );
   attribute.push( { "Unique": false, "Enforced": false, "NotNull": true, "NotArray": false } );
   attribute.push( { "Unique": false, "Enforced": false, "NotNull": false, "NotArray": true } );

   // 查询指定顺序并校验
   var cursor = maincl.snapshotIndexes( {}, {}, { "IndexDef.name": 1 } );
   var indexInfo = [];
   while( cursor.next() )
   {
      indexInfo.push( cursor.current().toObj() );
   }
   cursor.close();

   assert.equal( indexInfo.length, indexNames.length );
   for( var i = 0; i < indexNames.length; i++ )
   {
      checkSnapshotIndexes( indexInfo[i], indexNames[i], keys[i], attribute[i], mainNodes );
   }

   // 查询指定逆序并校验
   var cursor = maincl.snapshotIndexes( {}, {}, { "IndexDef.name": -1 } );
   var indexInfo = [];
   while( cursor.next() )
   {
      indexInfo.push( cursor.current().toObj() );
   }
   cursor.close();

   assert.equal( indexInfo.length, indexNames.length );
   for( var i = 0; i < indexNames.length; i++ )
   {
      checkSnapshotIndexes( indexInfo[5 - i], indexNames[i], keys[i], attribute[i], mainNodes );
   }

   commDropCS( db, csName );
}

function checkSnapshotIndexes ( explainObj, indexName, key, attribute, nodes )
{
   var indexDef = explainObj.IndexDef;
   assert.equal( indexDef.name, indexName, JSON.stringify( indexDef ) );
   assert.equal( indexDef.key, key, JSON.stringify( indexDef ) );
   assert.equal( indexDef.unique, attribute.Unique, JSON.stringify( indexDef ) );
   assert.equal( indexDef.enforced, attribute.Enforced, JSON.stringify( indexDef ) );
   assert.equal( indexDef.NotNull, attribute.NotNull, JSON.stringify( indexDef ) );
   assert.equal( indexDef.NotArray, attribute.NotArray, JSON.stringify( indexDef ) );
   assert.equal( explainObj.Nodes.sort( sortBy( "NodeName" ) ), nodes.sort( sortBy( "NodeName" ) ) );
}

// 按照snapshotIndexes显示节点形式返回所有subcl所在的节点并去重
function getCLNodes ( db, csName, subclNames )
{
   var nodes = [];
   var groups = [];
   var mainGroups = [];

   for( var i = 0; i < subclNames.length; i++ )
   {
      var groupNames = commGetCLGroups( db, csName + "." + subclNames[i] );
      groups = groups.concat( groupNames );
   }

   for( var i = 0; i < groups.length; i++ )
   {
      if( mainGroups.indexOf( groups[i] ) == -1 )
      {
         mainGroups.push( groups[i] );
      }
   }

   for( var i = 0; i < mainGroups.length; i++ )
   {
      var node = commGetGroupNodes( db, mainGroups[i] );
      for( var j = 0; j < node.length; j++ )
      {
         nodes.push( { "NodeName": node[j].HostName + ":" + node[j].svcname, "GroupName": mainGroups[i] } );
      }
   }
   return nodes;
}