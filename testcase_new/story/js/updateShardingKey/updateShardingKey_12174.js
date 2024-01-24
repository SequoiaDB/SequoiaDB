/************************************
 *@Description: 测试用例 seqDB-12174 :: 版本: 1 :: 主表和子表分区键相同，更新分区键 
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12174";
var mainCLName = CHANGEDPREFIX + "_mcl_12174";
var subCLName1 = "subcl1";
var subCLName2 = "subcl2";
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   commDropCS( db, csName, true, "Failed to drop CS." );
   commCreateCS( db, csName, false, "Failed to create CS." );

   //TODO:b场景部分相同覆盖不全，用例测试点可参考，主表分区键为{a：1，b：1}，子表分区键为{a：1，c：1}，
   //文本用例已增加实例，请参考更新
   var sk1 = { a: 1 };
   var sk2 = { b: 1 };
   var mainCL = createMainCL( csName, mainCLName, sk1 );
   createCL( csName, subCLName1, sk1 );
   createCL( csName, subCLName2, sk2 );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 100 }, UpBound: { "a": 1000 } } );

   //insert data 	
   var docs = new Array();
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );

   //upsertData( mainCL, upsertCondition, findCondition );
   updateData( mainCL, { $set: { a: "test" } }, {} );
   updateData( mainCL, { $set: { b: "test" } }, {} );

   //check the update result
   var expRecs = []
   checkResult( mainCL, { a: { "$et": "test" } }, null, expRecs );
   checkResult( mainCL, { b: { "$et": "test" } }, null, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
