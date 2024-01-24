/************************************
 *@Description: 测试用例 seqDB-12173 :: 版本: 1 :: 主子表开启flag更新非分区键，其中子表切分在多个组 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12173";
var mainCLName = CHANGEDPREFIX + "_mcl_12173";
var subCLName1 = "subcl1";
var subCLName2 = "subcl2";
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

   commDropCS( db, csName, true, "Failed to drop CS." );
   commCreateCS( db, csName, false, "Failed to create CS." );

   var sk1 = { a: 1 };
   var sk2 = { b: 1 };
   var mainCL = createMainCL( csName, mainCLName, sk1 );
   createCL( csName, subCLName1, sk2 );
   createCL( csName, subCLName2, sk2 );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 100 }, UpBound: { "a": 1000 } } );

   //insert data 	
   var docs = [{ a: 1, b: 1, c: 1 }, { a: 101, b: 101, c: 1 }]
   mainCL.insert( docs );

   //insert data
   clSplit( csName, subCLName1, 50 );
   clSplit( csName, subCLName2, 50 );


   //upsertData( mainCL, upsertCondition, findCondition );
   mainCL.update( { $set: { c: "test" } }, {}, {}, {}, { KeepShardingKey: true } );

   //check the update result
   var expRecs = [{ a: 1, b: 1, c: "test" }, { a: 101, b: 101, c: "test" }];
   checkResult( mainCL, null, null, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
