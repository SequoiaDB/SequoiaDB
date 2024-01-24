/************************************
 *@Description: 测试用例 seqDB-12185 :: 版本: 1 :: 更新分区键为嵌套数组 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12185";
var clName = CHANGEDPREFIX + "_cl_12185";
//main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }

   commDropCS( db, csName, true, "Failed to drop CS." );
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mycl = createCL( csName, clName, { "a": 1 } );
   //insert data 	
   var docs = [{ a: [["b", "c"]] }];
   mycl.insert( docs );

   //updateData
   mycl.update( { $set: { "a": ["b"] } }, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: ["b"] }] );

   mycl.update( { $set: { "a": [["b", "c"]] } }, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: [["b", "c"]] }] );

   mycl.remove();
   mycl.upsert( { $set: { "a": ["b"] } }, {}, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: ["b"] }] );

   mycl.upsert( { $set: { "a": [["b", "c"]] } }, {}, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: [["b", "c"]] }] );

   //check the update result
   var expRecs = docs;

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


