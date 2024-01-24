/************************************
 *@Description: 测试用例 seqDB-12178 :: 版本: 1 :: 更新分区键重复
 1、创建cs、分区表，分别设置分区键字段为_id、非_id字段
 2、向表中插入数据
 3、执行update更新操作，设置flag参数为true，更新分区键字段，其中更新值和其它分区键值重复
 4、检查更新结果
 1、分区键字段为_id时，更新返回失败，查看数据未更新
 2、分区键字段为非_id字段时，更新成功，查看更新数据正确
 *@author:      LaoJingTang
 *@createdate:  2017.8.22
 **************************************/

var csName = CHANGEDPREFIX + "_cs_12178";
var clName1 = CHANGEDPREFIX + "_cl1_12178";
var clName2 = CHANGEDPREFIX + "_cl2_12178";

//main( test );

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
   var mycl1 = createCL( csName, clName1, { "_id": 1 } );
   var mycl2 = createCL( csName, clName2, { "a": 1 } );
   //insert data
   var docs = [{ a: 1 }];
   mycl2.insert( docs );
   mycl1.insert( docs );

   //updateData
   mycl2.update( { $set: { a: "test" } }, {}, {}, { KeepShardingKey: true } );

   mycl1.update( { $set: { "_id": "test" } }, {}, {}, { KeepShardingKey: true } );

   //check the update result
   var expRecs = [{ a: "test" }];
   checkResult( mycl2, null, { "_id": { "$include": 0 } }, expRecs );
   expRecs = docs;
   checkResult( mycl1, null, null, expRecs );

   //	//drop collectionspace in clean
   commDropCS( db, csName, false, "Failed to drop CS." );
}
