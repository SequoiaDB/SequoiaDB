/******************************************************************************
 * @Description   : seqDB-26464:普通表带自增字段，指定更新规则，单条/批量插入冲突记录
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.13
 * @LastEditTime  : 2022.06.10
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_26464";
testConf.clOpt = { AutoIncrement: { Field: "id" } };

main( test );

function test ( testPara )
{
   //带自增字段的普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }] );

   //指定UpdateOnDup和更新规则，单条插入冲突记录
   cl.insert( { a: 1, b: 1 }, { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } );
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 2, b: 2, "id": 2 }, { a: 3, b: 1, "id": 1 }];
   commCompareResults( actRes, expRes );

   //指定UpdateOnDup和更新规则，批量插入冲突记录
   cl.insert( [{ a: 2, b: 2 }, { a: 3, b: 1 }], { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 4, b: 2, "id": 2 }, { a: 5, b: 1, "id": 1 }];
   commCompareResults( actRes, expRes );

   //指定更新规则，更新操作为$replace，单条插入冲突记录，查看自增字段值
   //SEQUOIADBMAINSTREAM-8499
   /*
   cl.insert( { a: 4, b: 2 }, { UpdateOnDup: true, Update: { "$replace": { "a": 2 } } } );
   actRes = cl.find().sort( { a: 1 } );
   expRes = [{ a: 2, "id": 2 }, { a: 5, b: 1, "id": 1 }];
   commCompareResults( actRes, expRes );
   */
}
