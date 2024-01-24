/************************************
*@Description: maincl update ShardingKey,the subcl has been split;
*@author:      wuyan
*@createdate:  2017.8.22
**************************************/
var csName = CHANGEDPREFIX + "_cs_12166";
var mainCLName = CHANGEDPREFIX + "_mcl_12166";
var subCLName = COMMCLNAME + "_scl_12166";
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
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
   var shardingKey = { no: 1 };
   createCL( csName, subCLName, shardingKey );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { "a": -10 }, UpBound: { "a": 100 } } );

   //insert data 	
   var doc = [{ a: 1, no: 12, test: 1 }, { a: 4, no: 12.36 }, { a: 5, no: 1.23 }, { a: 6, no: "test6" }];
   mainCL.insert( doc );

   //split subcl                 
   var percent = 50;
   clSplit( csName, subCLName, percent )

   //update ShardingKey of subcl
   var updateCondition = { $inc: { no: 12, test: 1 } };
   updateDataError( mainCL, "update", updateCondition );

   //check the update result
   var expRecs = doc;
   checkResult( mainCL, null, null, expRecs, { _id: 1 } );

   // drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );

}

