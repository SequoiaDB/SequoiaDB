/************************************
*@Description: maincl update ShardingKey,no set KeepShardingKey;
*@author:      wuyan
*@createdate:  2017.8.22
**************************************/
var csName = CHANGEDPREFIX + "_cs_12162";
var mainCLName = CHANGEDPREFIX + "_mcl_12162";
var subCLName = COMMCLNAME + "_scl_12162";
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
   var shardingKey = { no: 1 };
   createCL( csName, subCLName, shardingKey );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { "a": -10 }, UpBound: { "a": 100 } } );

   //insert data 	
   var doc = [{ a: 1, no: 12, test: 1 }, { a: 2, no: 13, test: 2 }, { a: 3, no: 14, test: 3 },
   { a: 4, no: 12.36 }, { a: 5, no: 1.23 }, { a: 6, no: "test6" }];
   mainCL.insert( doc );

   //update ShardingKey of subcl
   var updateCondition = { $inc: { no: 12, test: 1 } };
   var findCondition = { a: { $lt: 5 } };
   updateData( mainCL, updateCondition, findCondition );

   //check the update result
   var expRecs = [{ a: 1, no: 12, test: 2 }, { a: 2, no: 13, test: 3 }, { a: 3, no: 14, test: 4 },
   { a: 4, no: 12.36, test: 1 }, { a: 5, no: 1.23 }, { a: 6, no: "test6" }];
   checkResult( mainCL, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );

}