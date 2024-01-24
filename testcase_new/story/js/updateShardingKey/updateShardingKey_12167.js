/************************************
*@Description: find and update on maincl,update the ShardingKey of subcl,
                        a subcl has been split,find and update data on other subcl success.
*@author:      wuyan
*@createdate:  2017.8.22
**************************************/
var csName = CHANGEDPREFIX + "_cs_12167";
var mainCLName = CHANGEDPREFIX + "_mcl_12167";
var subCLName1 = COMMCLNAME + "_subcl_12167_1";
var subCLName2 = COMMCLNAME + "_subcl_12167_2";

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

   //create maincl/subcl  
   commCreateCS( db, csName, false, "Failed to create CS." );
   var mainclShardingKey = { a: 1 };
   var mainCL = createMainCL( csName, mainCLName, mainclShardingKey );
   createAndAttachCL( mainCL );

   //insert data 	
   var doc = [{ a: -1, no: 12, test: 1 }, { a: 2, no: { $decimal: "125.456" }, test: 2 }, { a: 3, no: 14, test: 3 },
   { a: 4, no: 12.36 }, { a: 12, no: 1.23 }, { a: 6, no: { "$timestamp": "2012-01-01-13.14.26.124233" } },
   { a: 15, no: "test15" }, { a: 16, no: 12.36, test: { a: 1 } }];
   mainCL.insert( doc );

   //one of the subcls split                  
   var percent = 50;
   clSplit( csName, subCLName1, percent )

   //findCondition is maincl shardingKey,update ShardingKey of subcl success,
   var updateCondition = { $inc: { no: 2 }, $set: { test: "testUpdate" } };
   var findCondition = { "a": { "$gt": 12 } };
   findAndUpdateData( mainCL, findCondition, updateCondition, true, true );

   var expRecs = [{ a: -1, no: 12, test: 1 }, { a: 2, no: { $decimal: "125.456" }, test: 2 }, { a: 3, no: 14, test: 3 },
   { a: 4, no: 12.36 }, { a: 12, no: 1.23, }, { a: 6, no: { "$timestamp": "2012-01-01-13.14.26.124233" } },
   { a: 15, no: "test15", test: "testUpdate" }, { a: 16, no: 14.36, test: "testUpdate" }];
   checkResult( mainCL, null, null, expRecs, { _id: 1 } );

   //findCondition is no maincl shardingKey,update ShardingKey of subcl fail,
   var updateCondition = { $inc: { no: 1 }, $set: { test: "testfail" } };
   var findCondition = { "test": "test15" };
   updateDataError( mainCL, "findAndUpdate", updateCondition, findCondition );

   var expRecs1 = expRecs;
   checkResult( mainCL, null, null, expRecs1, { _id: 1 } );


   // drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}


function createAndAttachCL ( mainCL )
{
   var shardingKey = { no: 1 };
   createCL( csName, subCLName1, shardingKey );
   createCL( csName, subCLName2, shardingKey );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": -10 }, UpBound: { "a": 10 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 10 }, UpBound: { "a": 20 } } );

}

