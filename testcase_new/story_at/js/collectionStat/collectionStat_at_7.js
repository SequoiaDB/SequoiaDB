/***************************************************************************************************
 * @Description: 空主表获取统计信息
 * @ATCaseID: collectionStat_7
 * @Author: Liu Yuchen
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 12/15/2022  Liu Yuchen     Get collection stat info from empty main collection
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    仅创建主表，不创建子表，获取统计信息
 * 测试步骤：
 *    1. 公共CS下创建主表maincl，不创建子表
 *    2. 获取主表maincl统计信息
 *    3. 创建子表subcl，并挂载至主表maincl上
 *    4. 获取主表maincl集合统计信息
 *
 * 期望结果：
 *    未创建并挂载子表前，主表maincl为空，报错(SDB_MAINCL_NOIDX_NOSUB)
 *    创建并挂载子表后，主表maincl可获取到默认的统计信息
 *
 **************************************************************************************************/
 main( test );
 function test(){
   var clSuffix = "_collectionStat_7"
   var mainCLName = "mainCL" + clSuffix ;
   var subCLName = "subCL" + clSuffix ;

   // 创建主表
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { "IsMainCL": true, "ShardingKey": { "a": 1 }, "ShardingType": "range"} ) ;

   // 获取统计信息
   assert.tryThrow( SDB_MAINCL_NOIDX_NOSUB, function () { mainCL.getCollectionStat(); } );

   // 创建并挂载子表
   commCreateCL( db, COMMCSNAME, subCLName, { "ShardingKey": { "a": 1 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName, { "LowBound": { "a": 0 }, "UpBound": { "a": 100000 } } );

   // 获取统计信息
   checkCollectionStat( mainCL, mainCLName, 1, true, false, 10, 200, 200, 1, 80000 ) ;

 }
