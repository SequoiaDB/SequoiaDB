/************************************
*@Description: find and update on maincl,update the ShardingKey of subcl,
                        set the keepShardingKey is false;
*@author:      wuyan
*@createdate:  2017.8.22
**************************************/
var csName = CHANGEDPREFIX + "_cs_12163";
var mainCLName = CHANGEDPREFIX + "_mcl_12163";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
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
   var doc = [{ a: -1, no: 12, test: 1 }, { a: 3, no: 14, test: 3 }, { a: 4, no: 12.36 }, { a: 12, no: 1.23 },
   { a: 6, no: { "$timestamp": "2012-01-01-13.14.26.124233" } },
   { a: 15, no: "test15" }, { a: 16, no: 12.36, test: { a: 1 } }, { a: 20, no: 1.23, test: "test22" },
   { a: 23, no: "test23", test: "test23" }, { a: 29, no: { $decimal: "123.456" }, test: null }];
   mainCL.insert( doc );

   //find and update ShardingKey of subcls
   var updateCondition = { $inc: { no: 2 }, $set: { test: "testUpdate" } };
   var findCondition = { "$and": [{ "a": { "$gte": 6 } }, { "a": { "$lte": 29 } }] };
   findAndUpdateData( mainCL, findCondition, updateCondition, true, false );

   //check the update result
   var expRecs = [{ a: -1, no: 12, test: 1 }, { a: 3, no: 14, test: 3 }, { a: 4, no: 12.36 },
   { a: 12, no: 1.23, test: "testUpdate" }, { a: 6, no: { "$timestamp": "2012-01-01-13.14.26.124233" }, test: "testUpdate" },
   { a: 15, no: "test15", test: "testUpdate" }, { a: 16, no: 12.36, test: "testUpdate" },
   { a: 20, no: 1.23, test: "testUpdate" }, { a: 23, no: "test23", test: "testUpdate" },
   { a: 29, no: { $decimal: "123.456" }, test: "testUpdate" }];
   checkResult( mainCL, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );

}


function createAndAttachCL ( mainCL )
{
   var subCLName1 = COMMCLNAME + "_subcl_12163_1";
   var subCLName2 = COMMCLNAME + "_subcl_12163_2";
   var subCLName3 = COMMCLNAME + "_subcl_12163_3";

   var shardingKey = { no: 1 };
   createCL( csName, subCLName1, shardingKey );
   createCL( csName, subCLName2, shardingKey );
   createCL( csName, subCLName3, shardingKey );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": -10 }, UpBound: { "a": 10 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 10 }, UpBound: { "a": 20 } } );
   mainCL.attachCL( csName + "." + subCLName3, { LowBound: { "a": 20 }, UpBound: { "a": 30 } } );

}