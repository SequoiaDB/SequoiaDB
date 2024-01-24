/******************************************************************************
 * @Description   : seqDB-26469:主子表指定更新规则，批量插入冲突记录，涉及多个数据组，
 *                              有/无事务，更新后记录冲突
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.17
 * @LastEditTime  : 2022.06.10
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );

function test ( testPara )
{
   var mclName = COMMCLNAME + "_mcl_26469";
   var sclName1 = COMMCLNAME + "_scl_26469_1";
   var sclName2 = COMMCLNAME + "_scl_26469_2";
   var sclFullName1 = COMMCSNAME + "." + sclName1;
   var sclFullName2 = COMMCSNAME + "." + sclName2;
   var groupName1 = testPara.groups[0][0].GroupName;
   var groupName2 = testPara.groups[1][0].GroupName;

   //创建主子表，子表在不同数据组上，创建唯一索引，插入记录
   var mclOption = { IsMainCL: true, ShardingKey: { "b": 1 }, ShardingType: "range" };
   var mcl = commCreateCL( db, COMMCSNAME, mclName, mclOption );
   var sclOption1 = { Group: groupName1 };
   commCreateCL( db, COMMCSNAME, sclName1, sclOption1 );
   var sclOption2 = { Group: groupName2 };
   commCreateCL( db, COMMCSNAME, sclName2, sclOption2 );
   mcl.attachCL( sclFullName1, { LowBound: { b: 0 }, UpBound: { b: 10 } } );
   mcl.attachCL( sclFullName2, { LowBound: { b: 10 }, UpBound: { b: 20 } } );
   mcl.createIndex( "idx_a", { a: 1, b: 1 }, true, true );
   mcl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 10, b: 10 }, { a: 11, b: 11 }, { a: 13, b: 11 }] );

   //有事务，更新后记录部分冲突
   db.transBegin();
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      mcl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 10, b: 10 }, { a: 11, b: 11 }], { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } );
   } );
   db.transCommit();
   var actRes = mcl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 10, b: 10 }, { a: 11, b: 11 }, { a: 13, b: 11 }];
   commCompareResults( actRes, expRes );

   //无事务，更新后记录部分冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      mcl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 10, b: 10 }, { a: 11, b: 11 }], { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } );
   } );
   actRes = mcl.find().sort( { a: 1 } );
   expRes = [{ a: 3, b: 1 }, { a: 4, b: 2 }, { a: 11, b: 11 }, { a: 12, b: 10 }, { a: 13, b: 11 }];
   commCompareResults( actRes, expRes );
}
