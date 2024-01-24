/************************************
*@Description: shardedCL find And update ShardingKey,
                  the ShardingKey and non ShardingKeys are updated successfully
*@author:      wuyan
*@createdate:  2017.8.16
**************************************/
var clName = CHANGEDPREFIX + "_updateShardingKey_12157";
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

   //findAndUpdate ShardingKey,set KeepShardingKey=true
   var findCondition = { b: { $gte: 2 } };
   var updateCondition = { $set: { no: "testupdate12157", a: "testa12157" } };
   findAndUpdateData( dbcl, findCondition, updateCondition, true, true );

   //check the update result
   var expRecs = [{ no: { "$timestamp": "2017-07-29-13.14.26.124233" }, a: "testa1", b: 1 },
   { no: "testupdate12157", a: "testa12157", b: 2 },
   { no: "testupdate12157", a: "testa12157", b: 3 }];
   checkResult( dbcl, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCL( db, COMMCSNAME, clName, false, false );

}