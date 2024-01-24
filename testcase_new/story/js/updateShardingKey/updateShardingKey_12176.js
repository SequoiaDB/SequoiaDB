/************************************
 *@Description: 测试用例 seqDB-12176 :: 版本: 1 :: 主子表上upsert更新分区键
 1、创建cs、主表、子表，子表挂载到主表
 2、向主表插入数据
 3、执行upsert更新子表分区键，设置flag为true，其中更新分区键满足如下条件：
 a、匹配不到记录
 b、更新分区键字段覆盖对象、嵌套对象、嵌套数据
 4、检查更新结果
 1、更新返回成功，匹配不到记录时，新插入更新数据，查看更新值正确
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12176";
var mainCLName = CHANGEDPREFIX + "_mcl_12176";
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

   var sk1 = { a: 1 };
   var sk2 = { b: 1 };
   var mainCL = createMainCL( csName, mainCLName, sk1 );

   createCL( csName, subCLName1, sk2 );
   createCL( csName, subCLName2, sk2 );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 100 }, UpBound: { "a": 1000 } } );

   var insertList = [{ a: 1, b: 1 }, { a: 1, b: { b: 1 } }, { a: 1, b: { b: { b: 1 } } }, { a: 1, b: [1, 2, 3, 4] }];
   var upsertList = [{ b: 1 }, { b: { b: 1 } }, { b: { b: { b: 1 } } }, { b: [1, 2, 3, 4] }];
   var expectList = [{ a: 1, b: 1 }, { a: 1, b: { b: 1 } }, { a: 1, b: { b: { b: 1 } } }, { a: 1, b: [1, 2, 3, 4] }];
   //b、更新分区键字段覆盖对象、嵌套对象、嵌套数据
   for( i in insertList )
   {
      mainCL.remove();
      mainCL.insert( insertList[i] );
      for( j in upsertList )
      {
         mainCL.upsert( { $set: upsertList[j] }, {}, {}, {}, { KeepShardingKey: true } );
         checkResult( mainCL, null, null, [expectList[j]] );
      }
   }

   //匹配不到记录
   for( i in upsertList )
   {
      mainCL.remove();
      mainCL.insert( { a: 1 } )
      mainCL.upsert( { $set: upsertList[i] }, {}, {}, {}, { KeepShardingKey: true } );
      checkResult( mainCL, null, null, [expectList[i]] );
   }

   commDropCS( db, csName, false, "Failed to drop CS." );
}
