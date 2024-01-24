/************************************
 *@Description: 测试用例 seqDB-12170 :: 版本: 1 :: 主子表不开启flag更新分区键，其中子表已切分
 1、创建cs、主表、子表，子表挂载到主表
 2、向主表插入数据，切分子表
 3、执行update更新操作，设置flag参数为false，更新数据包括分区键和非分区键字段
 4、检查更新结果
 1、更新返回成功，查看分区键被剔除未更新，非分区键字段更新值正确
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12170";
var mainCLName = CHANGEDPREFIX + "_mcl_12170";
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
   createCL( csName, subCLName1, sk1 );
   createCL( csName, subCLName2, sk1 );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "a": 0 }, UpBound: { "a": 100 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "a": 100 }, UpBound: { "a": 1000 } } );

   //insert data
   var docs = [{ a: 1, b: 1 }, { a: 101, b: 101 }, { a: 1, c: 0 }]
   mainCL.insert( docs );

   //split data
   clSplit( csName, subCLName1, 50 );
   clSplit( csName, subCLName2, 50 );

   //upsertData( mainCL, upsertCondition, findCondition );
   mainCL.update( { $set: { a: { a: 1 } } }, {}, {}, {}, { KeepShardingKey: false } );

   //check the update result
   var expRecs = docs;
   checkResult( mainCL, null, null, expRecs );
   expRecs = [{ "a": 1, "b": 1, "c": { "a": 1 } }, { "a": 101, "b": 101, "c": { "a": 1 } }, { "a": 1, "c": { "a": 1 } }]
   mainCL.update( { $set: { c: { a: 1 } } }, {}, {}, {}, { KeepShardingKey: false } );
   checkResult( mainCL, null, null, expRecs );

   //drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
