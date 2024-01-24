/************************************
*@Description: update ShardingKey use operator $replace,set the flag is true,update the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_13052";
//main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, options, true, true );

   //insert data 	
   var doc = [{ no: "testno", a: 1 }, { no: ["test2"], a: 2 },
   { no: 2, a: ["test1", "test2"] }];
   dbcl.insert( doc );

   //updateShardingKey use $replace
   var updateCondition = { $replace: { no: 5, a: { $date: "2017-07-20" } } };
   updateData( dbcl, updateCondition, {}, {}, true );

   //check the update result
   var expRecs = [{ no: 5, a: { $date: "2017-07-20" } },
   { no: 5, a: { $date: "2017-07-20" } },
   { no: 5, a: { $date: "2017-07-20" } }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}
