/************************************
*@Description: update ShardingKey use operator $pop,set the flag is false,kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_kickShardingKey_12193";
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var options = { ShardingKey: { no: 1, a: -1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, options, true, true );

   //insert data 	
   var doc = [{ no: [1, "test", 3], a: 1, test: [1, 2, 3, 5, 7, 8] },
   { no: [1, [2.34, "test", 33], 67], a: 2, test: ["test", 2, 56, "test", 8, 90.89] }];
   dbcl.insert( doc );

   //updateShardingKey use $pop,the array is nested array
   var updateCondition = { $pop: { "no.1": 1, test: 2 } };
   updateData( dbcl, updateCondition )
   //check the update result
   var expRecs = [{ no: [1, "test", 3], a: 1, test: [1, 2, 3, 5] },
   { no: [1, [2.34, "test", 33], 67], a: 2, test: ["test", 2, 56, "test"] }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   //updateShardingKey use $pop,the array is not nested array
   var updateCondition1 = { $pop: { "no": 2, test: 1 } };
   var findCondition = { a: 1 };
   updateData( dbcl, updateCondition1, findCondition )
   //check the update result
   var expRecs1 = [{ no: [1, "test", 3], a: 1, test: [1, 2, 3] },
   { no: [1, [2.34, "test", 33], 67], a: 2, test: ["test", 2, 56, "test"] }];;
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}