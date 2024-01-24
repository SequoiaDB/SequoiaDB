/************************************
*@Description: shardedCL update ShardingKey,
                  the ShardingKey and non ShardingKeys are updated successfully
*@author:      wuyan
*@createdate:  2017.7.29
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_12156";
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
   var shardingKey = { no: 1 };
   var dbcl = createCL( COMMCSNAME, clName, shardingKey );

   //insert data 	
   var doc = [{ no: { "$timestamp": "2017-07-29-13.14.26.124233" }, a: "testa1", b: 1 },
   { no: { "$date": "2017-07-29" }, a: ["test1", "test2"], b: 2 },
   { no: "testupdate", a: "testa3", b: 3 }];
   dbcl.insert( doc );

   //update ShardingKey,set KeepShardingKey=true
   var updateCondition = { $set: { no: "testupdate", a: "testa" } };
   updateData( dbcl, updateCondition, {}, {}, true );

   //check the update result
   var expRecs = [{ no: "testupdate", a: "testa", b: 1 }, { no: "testupdate", a: "testa", b: 2 },
   { no: "testupdate", a: "testa", b: 3 }];;
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );

}

