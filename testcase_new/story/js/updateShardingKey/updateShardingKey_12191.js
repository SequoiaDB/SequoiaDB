/************************************
*@Description: update ShardingKey use operator unset,set the flag is false,kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_kickShardingKey_12191";
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
   var options = { ShardingKey: { no: 1, test: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, options, true, true );

   //insert data 
   var doc = [{ no: { $numberLong: "9223372036854775799" }, test: [1, "test", 3], c: 12.3, d: [1, 2, 3] },
   { no: { $decimal: "12.35" }, test: { test: { a: { $date: "2017-07-16" } } }, c: true }];
   dbcl.insert( doc );

   //updateShardingKey use $unset
   var updateCondition = { $unset: { no: "", "test.1": "", c: "", "d.1": "" } };
   updateData( dbcl, updateCondition )

   //check the update result
   var expRecs = [{ no: { $numberLong: "9223372036854775799" }, test: [1, "test", 3], d: [1, null, 3] },
   { no: { $decimal: "12.35" }, test: { test: { a: { $date: "2017-07-16" } } } }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}