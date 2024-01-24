/************************************
 *@Description: 测试用例 seqDB-12183 :: 版本: 1 :: 更新分区键为原值 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12183";
var clName = CHANGEDPREFIX + "_cl_12183";
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
   var docs = [{ a: 1 }];
   mycl.insert( docs );

   //updateData
   mycl.update( { $set: { "a": 1 } }, {}, {}, { KeepShardingKey: true } );

   //check the update result
   var expRecs = docs;
   checkResult( mycl, null, { "_id": { "$include": 0 } }, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


