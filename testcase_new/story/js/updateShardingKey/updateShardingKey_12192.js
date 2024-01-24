/************************************
*@Description: update ShardingKey use operator addtoset,set the flag is false,kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_kickShardingKey_12192";
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
   var doc = [{ no: [1, "test", 3], a: 34.5, test: [1, 2, 3] },
   { no: [1, [2.34, "test", 33], 67], a: 222222, test: ["test", 2, 56] }];
   dbcl.insert( doc );

   //updateShardingKey use $addtoset
   var updateCondition = { $addtoset: { "no.1": ["update", 8], a: [66], test: [8] } };
   updateData( dbcl, updateCondition )

   //check the update result
   var expRecs = [{ no: [1, "test", 3], a: 34.5, test: [1, 2, 3, 8] },
   { no: [1, [2.34, "test", 33], 67], a: 222222, test: ["test", 2, 56, 8] }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}