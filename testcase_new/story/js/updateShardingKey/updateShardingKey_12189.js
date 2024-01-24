/************************************
*@Description: update ShardingKey use operator inc,set the flag is false,kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_kickShardingKey_12189";
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

   //insert numberic data,array with 3 layer and common object  
   var doc = [{ no: -2147483640, test: { test: { a: 12.36, lastName: "test1" } }, c: 12.3 },
   {
      no: { $decimal: "12.35" }, test: { test: { a: { $numberLong: "9223372036854775799" } } },
      c: { $decimal: "12.35" }
   }];
   dbcl.insert( doc );

   //update use $inc,the operate object is exist or not exist
   var updateCondition = { $inc: { no: 1, "test.test.a": 100, c: 1 } };
   updateData( dbcl, updateCondition )

   //check the update result
   var expRecs = [{ no: -2147483640, test: { test: { a: 12.36, lastName: "test1" } }, c: 13.3 },
   {
      no: { $decimal: "12.35" }, test: { test: { a: { $numberLong: "9223372036854775799" } } },
      c: { $decimal: "13.35" }
   }];
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}