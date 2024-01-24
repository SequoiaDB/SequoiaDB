/************************************
 *@Description: 测试用例 seqDB-12184 :: 版本: 1 :: 更新分区键为嵌套对象 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12184";
var clName = CHANGEDPREFIX + "_cl_12184";
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
   var docs = [{ a: { b: { c: 1 } } }];
   mycl.insert( docs );

   //updateData
   mycl.update( { $set: { "a": { b: 1 } } }, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: { b: 1 } }] );

   mycl.update( { $set: { "a": { b: { c: 1 } } } }, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: { b: { c: 1 } } }] );

   mycl.update( { $set: { "a": 1 } }, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: 1 }] );

   mycl.remove();
   mycl.upsert( { $set: { "a": { b: 1 } } }, {}, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: { b: 1 } }] );

   mycl.remove();
   mycl.upsert( { $set: { "a": { b: { c: 1 } } } }, {}, {}, {}, { KeepShardingKey: true } );
   checkResult( mycl, null, null, [{ a: { b: { c: 1 } } }] );

   //check the update result
   var expRecs = docs;

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


