/************************************
 *@Description: 测试用例 seqDB-12187 :: 版本: 1 :: alter普通表为分区表，更新分区键 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12187";
var clName = CHANGEDPREFIX + "_cl_12187";
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
   var mycl = db.getCS( csName ).createCL( clName );
   mycl.alter( { ShardingKey: { a: 1 } } );

   //insert data 	
   var docs = [{ a: 1 }, { b: 1 }];
   mycl.insert( docs );

   //updateData
   mycl.update( { $set: { "a": "test" } }, {}, {}, { KeepShardingKey: true } );
   mycl.update( { $set: { "b": "test" } }, {}, {}, { KeepShardingKey: true } );

   //check the update result
   var expRecs = [{ "a": "test", "b": "test" }, { "a": "test", "b": "test" }];
   checkResult( mycl, null, { "_id": { "$include": 0 } }, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


