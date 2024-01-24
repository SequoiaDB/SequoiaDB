/***************************************************************************************************
 * @Description: 验证所有导致集合统计信息缓存过期的情况
 * @ATCaseID: collectionStat_4
 * @Author: Cheng Jingjing
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 11/21/2022 Cheng Jingjing
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    以普通表为例，验证三种导致集合统计信息缓存过期的情况
 * 测试步骤：
 *    1.建普通表
 *    2.使集合统计信息缓存过期
 *    2.1 通过插入数据使当前数据页大于costthreshold，而缓存中的统计信息数据页小于costthreshold
 *    2.2 若当前数据页与缓存中的数据页均大于costthreshold，则发生以下情况则判定为过期
 *        2.2.1 当 collection 的当前数据页数总大小 小于等于 8G 时，每增加 256 MB，判定为 过时
 *        2.2.2 当 collection 的当前数据页数总大小 大于 8G 时，每增加 1GB，判定为 过时
 *
 * 期望结果：
 *   以上每种情况，通过getCollectionStat()获取集合统计信息，IsExpired字段应为true
 *
 **************************************************************************************************/
testConf.clName = COMMCLNAME + "_collectionStat_4"
main( test );

function test( testPara ){
db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } ) ;
checkCollectionStat( testPara.testCL, testConf.clName, 1, false, false, 10, 0, 0, 0, 0 ) ;

// 1.插入数据，使当前数据页大于costthreshold
var data = [] ;
for( var i = 0; i < 50000; i++ ){
  data.push({a:i,b:1});
}
testPara.testCL.insert( data ) ;
checkCollectionStat( testPara.testCL, testConf.clName, 1, false, true, 10, 0, 0, 0, 0 ) ;

// 2.向集合中插入大于258MB的记录使集合统计信息的缓存过期
// 插入时间过长已手工验证

// 3.向大于8G的集合中插入大于1G的记录使集合统计信息的缓存过期
// 插入时间过长已手工验证
}