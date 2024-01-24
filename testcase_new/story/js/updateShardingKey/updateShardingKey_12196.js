/************************************
*@Description: update ShardingKey use operator $push,set the flag is false,kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_kickShardingKey_12196";
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
   var doc = [{ no: [1, "test", 2.234, 3], a: 1, test: [1, 2, 3, 5, 7, 8] },
   { no: [1, [2.34, "test", 33], 67], test: [2, 90.89] }];
   dbcl.insert( doc );

   //updateShardingKey use $push
   var updateCondition = { $push: { no: "addno", a: "testa", test: "addtest" } };
   updateData( dbcl, updateCondition )

   //check the update result
   var expRecs = [{ no: [1, "test", 2.234, 3], a: 1, test: [1, 2, 3, 5, 7, 8, "addtest"] },
   { no: [1, [2.34, "test", 33], 67], test: [2, 90.89, "addtest"] }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}