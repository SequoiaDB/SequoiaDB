/***************************************************************************************************
 * @Description: getCollectionStat()获取主子表的集合统计信息
 * @ATCaseID: collectionStat_2
 * @Author: Cheng Jingjing
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 11/21/2022 Cheng Jingjing get collection's statistics when the collection is main-sub cl.
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    当集合为主子表时，验证其未插入数据/插入数据/部分过期/全部过期/truncate时，返回的集合统计信息的值是否正确
 * 测试步骤：
 *    1.建主子表
 *    2.未插入数据时，查看主表、子表的集合统计信息
 *    3.插入数据，做analyze，查看主表、子表的集合统计信息
 *    4.向主表插入数据，使某一子表过期（即主表部分过期），查看主表、子表的集合统计信息的过期值
 *    5.做analyze，查看主表、子表的集合统计信息的实际值
 *    6.向主表插入数据，使全部子表过期（即主表全部过期），查看主表、子表的集合统计信息的过期值
 *    7.子表1做truncate，做analyze，查看主表、子表的集合统计信息
 *    8.主表做truncate，做analyze，查看主表、子表的集合统计信息
 *
 * 期望结果：
 *    1.未插入数据时，返回集合统计信息默认值
 *    2.做analyze之后，返回集合统计信息实际值
 *    3.集合统计信息部分过期，返回集合统计信息的过期值
 *    3.集合统计信息过期，返回集合统计信息的过期值
 *    4.truncate之后，返回集合统计信息默认值
 *
 **************************************************************************************************/
main( test ) ;

function test(){
  // 建立主子表
  var clSuffix = "_collectionStat_2" ;
  var mainCLName = "mainCL" + clSuffix;
  var subCLName1 = "subCL_1" + clSuffix;
  var subCLName2 = "subCL_2" + clSuffix;
  var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range"} ) ;
  var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { "ShardingKey": { "a": 1 } } );
  var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { "ShardingKey": { "a": 1 }, "AutoSplit": true } );
  mainCL.attachCL( COMMCSNAME + "." + subCLName1, { "LowBound": { "a": 0 }, "UpBound": { "a": 100000 } } );
  mainCL.attachCL( COMMCSNAME + "." + subCLName2, { "LowBound": { "a": 100000 }, "UpBound": { "a": 200000 } } );

  // 1.查看统计信息默认值
  // 1.1 连接子表1查询统计信息
  checkCollectionStat( subCL1, subCLName1, 1, true, false, 10, 200, 200, 1, 80000 ) ;

  // 1.2 连接子表2查询统计信息
  checkCollectionStat( subCL2, subCLName2, 1, true, false, 10, 400, 400, 2, 160000 ) ;

  // 1.3 连接主表查询统计信息
  checkCollectionStat( mainCL, mainCLName, 1, true, false, 10, 600, 600, 3, 240000 ) ;

  // 2.主表插入数据，做analyze，寻找主表/子表的统计信息
  var data = [] ;
  for( var i = 0; i < 300; i++ ){
    data.push({a:i,b:1});
  }
  mainCL.insert( data ) ;
  db.analyze( { "Collection": COMMCSNAME + "." + mainCLName } );

  // 2.1 连接子表1查询统计信息
  checkCollectionStat( subCL1, subCLName1, 1, false, false, 10, 200, 300, 1, 10800 ) ;

  // 2.2 连接子表2查询统计信息
  checkCollectionStat( subCL2, subCLName2, 1, false, false, 10, 0, 0, 0, 0 ) ;

  // 2.3 连接主表查询统计信息
  checkCollectionStat( mainCL, mainCLName, 1, false, false, 10, 200, 300, 1, 10800 ) ;

  // 3.验证过期的情况
  // 3.1 插入数据使子表subcl1过期(即使主表部分过期)
  data = [] ;
  for( var i = 0; i < 100000; i++ ){
    data.push({a:1,b:1});
  }
  mainCL.insert( data ) ;

  // 3.1.1 连接子表1查询统计信息
  checkCollectionStat( subCL1, subCLName1, 1, false, true, 10, 200, 300, 1, 10800 ) ;

  // 3.1.2 连接子表2查询统计信息
  checkCollectionStat( subCL2, subCLName2, 1, false, false, 10, 0, 0, 0, 0 ) ;

  // 3.1.3 连接主表查询统计信息
  checkCollectionStat( mainCL, mainCLName, 1, false, true, 10, 200, 300, 1, 10800 ) ;

  // 3.2 analyze,查看集合统计信息的实际值
  db.analyze( { "Collection": COMMCSNAME + "." + mainCLName } );
  // 3.2.1 连接子表1查询统计信息
  checkCollectionStat( subCL1, subCLName1, 1, false, false, 10, 200, 100300, 86, 3610800 ) ;

  // 3.2.2 连接子表2查询统计信息
  checkCollectionStat( subCL2, subCLName2, 1, false, false, 10, 0, 0, 0, 0 ) ;

  // 3.2.3 连接主表查询统计信息
  checkCollectionStat( mainCL, mainCLName, 1, false, false, 10, 200, 100300, 86, 3610800 ) ;

  // 3.3 向主表插入数据使主表mainCL全部过期
  mainCL.truncate() ;
  db.analyze( { "Collection": COMMCSNAME + "." + mainCLName } );
  data = [] ;
  for( var i = 0; i < 200000; i++ ){
    data.push({a:i,b:1});
  }
  mainCL.insert( data ) ;
  // 3.3.1 连接子表1查询统计信息
  checkCollectionStat( subCL1, subCLName1, 1, false, true, 10, 0, 0, 0, 0 ) ;

  // 3.3.2 连接子表2查询统计信息
  checkCollectionStat( subCL2, subCLName2, 1, false, true, 10, 0, 0, 0, 0 ) ;

  // 3.3.3 连接主表查询统计信息
  checkCollectionStat( mainCL, mainCLName, 1, false, true, 10, 0, 0, 0, 0 ) ;

  // 4. 做truncate
  // 4.1 子表做truncate
  db.analyze( { "Collection": COMMCSNAME + "." + mainCLName } );
  subCL1.truncate() ;
  checkCollectionStat( subCL1, subCLName1, 1, true, false, 10, 200, 200, 1, 80000 ) ;
  checkCollectionStat( mainCL, mainCLName, 1, true, false, 10, 600, 100200, 87, 3680000 ) ;

  // 4.2 主表做truncate
  mainCL.truncate() ;
  checkCollectionStat( mainCL, mainCLName, 1, true, false, 10, 600, 600, 3, 240000 ) ;
}