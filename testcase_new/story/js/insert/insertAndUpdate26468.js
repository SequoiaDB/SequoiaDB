/******************************************************************************
 * @Description   :  seqDB-26468:主子表指定更新规则，批量插入冲突记录，涉及多个数据组，更新后记录不冲突
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.16
 * @LastEditTime  : 2022.05.16
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipGroupLessThanThree = true;

main( test );

function test ( testPara )
{
   var mclName1 = COMMCLNAME + "_mcl_26468_1";
   var mclName2 = COMMCLNAME + "_mcl_26468_2";
   var sclName1 = COMMCLNAME + "_scl_26468_1";
   var sclName2 = COMMCLNAME + "_scl_26468_2";
   var sclName3 = COMMCLNAME + "_scl_26468_3";
   var sclName4 = COMMCLNAME + "_scl_26468_4";
   var sclFullName1 = COMMCSNAME + "." + sclName1;
   var sclFullName2 = COMMCSNAME + "." + sclName2;
   var sclFullName3 = COMMCSNAME + "." + sclName3;
   var sclFullName4 = COMMCSNAME + "." + sclName4;
   var groupName1 = testPara.groups[0][0].GroupName;
   var groupName2 = testPara.groups[1][0].GroupName;
   var groupName3 = testPara.groups[2][0].GroupName;

   //创建主子表，子表为普通表，子表在不同数据组上，创建唯一索引，插入记录
   var mclOption1 = { IsMainCL: true, ShardingKey: { "b": 1 }, ShardingType: "range" };
   var mcl1 = commCreateCL( db, COMMCSNAME, mclName1, mclOption1 );
   var sclOption1 = { Group: groupName1 };
   commCreateCL( db, COMMCSNAME, sclName1, sclOption1 );
   var sclOption2 = { Group: groupName2 };
   commCreateCL( db, COMMCSNAME, sclName2, sclOption2 );
   mcl1.attachCL( sclFullName1, { LowBound: { b: 0 }, UpBound: { b: 10 } } );
   mcl1.attachCL( sclFullName2, { LowBound: { b: 10 }, UpBound: { b: 20 } } );
   mcl1.createIndex( "idx_a", { a: 1, b: 1 }, true, true );
   mcl1.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 11, b: 11 }, { a: 12, b: 12 }] );

   //指定UpdateOnDup和更新规则，批量插入冲突记录，涉及多个数据组
   mcl1.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 11, b: 11 }, { a: 12, b: 12 }], { UpdateOnDup: true, Update: { "$inc": { "a": 2 } } } );
   var actRes1 = mcl1.find().sort( { a: 1 } );
   var expRes1 = [{ a: 3, b: 1 }, { a: 4, b: 2 }, { a: 13, b: 11 }, { a: 14, b: 12 }];
   commCompareResults( actRes1, expRes1 );

   //创建主子表，子表为分区表，子表在不同数据组上，创建唯一索引，插入记录
   var mclOption2 = { IsMainCL: true, ShardingKey: { "b": 1 }, ShardingType: "range" };
   var mcl2 = commCreateCL( db, COMMCSNAME, mclName2, mclOption2 );
   var sclOption3 = { Group: groupName1, ShardingKey: { "b": 1 }, ShardingType: "range" };
   var scl3 = commCreateCL( db, COMMCSNAME, sclName3, sclOption3 );
   scl3.split( groupName1, groupName3, { b: 5 }, { b: 10 } )
   var sclOption4 = { Group: groupName2, ShardingKey: { "b": 1 }, ShardingType: "range" };
   var scl4 = commCreateCL( db, COMMCSNAME, sclName4, sclOption4 );
   scl4.split( groupName2, groupName3, { b: 15 }, { b: 20 } )
   mcl2.attachCL( sclFullName3, { LowBound: { b: 0 }, UpBound: { b: 10 } } );
   mcl2.attachCL( sclFullName4, { LowBound: { b: 10 }, UpBound: { b: 20 } } );
   mcl2.createIndex( "idx_a", { a: 1, b: 1 }, true, true );
   mcl2.insert( [{ a: 1, b: 1 }, { a: 6, b: 6 }, { a: 11, b: 11 }, { a: 16, b: 16 }] );

   //指定UpdateOnDup和更新规则，批量插入冲突记录，涉及多个数据组
   mcl2.insert( [{ a: 1, b: 1 }, { a: 6, b: 6 }, { a: 11, b: 11 }, { a: 16, b: 16 }], { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   var actRes2 = mcl2.find().sort( { a: 1 } );
   var expRes2 = [{ a: 2, b: 1 }, { a: 7, b: 6 }, { a: 12, b: 11 }, { a: 17, b: 16 }];
   commCompareResults( actRes2, expRes2 );
}
