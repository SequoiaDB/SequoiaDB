/************************************
 *@Description: 测试用例 seqDB-12179 :: 版本: 1 :: 更新主表分区键
 1、创建cs、主表、子表
 2、向主表插入数据
 3、执行update更新操作，设置flag参数为true，更新数据主表分区键字段
 4、检查更新结果
 1、更新返回失败，查看数据未更新
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12179";
var mainCLName = CHANGEDPREFIX + "_mcl_12179";
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
   var mainCL = createMainCL( csName, mainCLName, sk1 );
   db.getCS( csName ).createCL( subCLName1 );
   db.getCS( csName ).createCL( subCLName2 );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 100 }, UpBound: { "a": 1000 } } );

   //insert data
   var docs = [{ a: 1 }];
   mainCL.insert( docs );

   updateDataError( mainCL, "update", { $set: { a: "test" } } );

   //check the update result
   var expRecs = docs;
   checkResult( mainCL, null, null, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
