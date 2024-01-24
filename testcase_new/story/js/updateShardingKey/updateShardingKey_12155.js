/************************************
*@Description: maincl update ShardingKey,set kick the shardingKey;
*@author:      wuyan
*@createdate:  2017.7.20
**************************************/
var csName = CHANGEDPREFIX + "_cs_12155";
var mainCLName = CHANGEDPREFIX + "_mcl_12155";
var subCLName = COMMCLNAME + "_scl_12155";
main( test );

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

   //clean environment before test
   commDropCS( db, csName, true, "Failed to drop CS." );

   //create maincl.subcl
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mainclShardingKey = { a: 1 };
   var mainCL = createMainCL( csName, mainCLName, mainclShardingKey );
   var shardingKey = { no: 1 };
   createCL( csName, subCLName, shardingKey );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { "a": -10 }, UpBound: { "a": 100 } } );

   //insert data 	
   var doc = [{ a: 1, no: 12, test: 1 }, { a: 2, no: 13, test: 2 }, { a: 3, no: 14, test: 3 },
   { a: 4, no: 12.36 }, { a: 5, no: 1.23 }, { a: 6, no: "test6" }];
   mainCL.insert( doc );

   //split data from srcgroup to targetgrpup by percent 50                   
   var percent = 50;
   clSplit( csName, subCLName, percent );

   //upsert ShardingKey 
   var upsertCondition = { $inc: { no: 2, a: 10, test: 1 } };
   var findCondition = { a: { $lt: 12 } };

   upsertData( mainCL, upsertCondition, findCondition, {}, {}, false );

   //check the update result
   var expRecs = [{ a: 1, no: 12, test: 2 }, { a: 2, no: 13, test: 3 }, { a: 3, no: 14, test: 4 },
   { a: 4, no: 12.36, test: 1 }, { a: 5, no: 1.23, test: 1 }, { a: 6, no: "test6", test: 1 }];
   checkResult( mainCL, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );

}