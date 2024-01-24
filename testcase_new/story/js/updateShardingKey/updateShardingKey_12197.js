/************************************
*@Description: update ShardingKey use operator $push_all,set the flag is false,kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_kickShardingKey_12197";
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
   var options = { ShardingKey: { no: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, options, true, true );

   //insert data 	
   var doc = [{ no: "testno", a: 1, test: [3.4e+10, 34] },
   { no: [2, 3, 4], test: ["test1", "test2"] }];
   dbcl.insert( doc );

   //updateShardingKey use $push
   var updateCondition = { $push_all: { no: ["addno1", 5], a: ["testa", 1, 2], test: ["addtest1", "add2", 3, 4] } };
   updateData( dbcl, updateCondition )

   //check the update result
   var expRecs = [{ no: "testno", a: 1, test: [34000000000, 34, "addtest1", "add2", 3, 4] },
   { no: [2, 3, 4], a: ["testa", 1, 2], test: ["test1", "test2", "addtest1", "add2", 3, 4] }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}