/***************************************************************************************************
 * @Description: 数据节点调用getCollectionStat()
 * @ATCaseID: collectionStat_6
 * @Author: Cheng Jingjing
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ============== =========================================================
 * 11/21/2022 Cheng Jingjing Data node call the func of getCollectionStat()
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    以普通表为例，验证数据节点调用getCollectionStat()，返回字段是否与协调节点调用相同
 * 测试步骤：
 *    1.建普通表
 *    2.数据节点调用getCollectionStat()
 *
 * 期望结果：
 *    数据节点调用getCollectionStat(),返回字段与协调节点相同
 *
 **************************************************************************************************/
 testConf.clName = COMMCLNAME + "_collectionStat_6"
 testConf.useSrcGroup = true ;
 main( test );
 function test(){
   // 数据节点调用该接口
   var groupName = testPara.srcGroupName ;
   var group = commGetGroups( db, "", groupName )[0] ;
   var primaryPos = group[0].PrimaryPos ;
   var data = new Sdb( group[primaryPos]["HostName"], group[primaryPos]["svcname"] ) ;
   var actResult = data.getCS( testConf.csName ).getCL( testConf.clName ).getCollectionStat().toObj() ;
   var curInternalVersion = 1 ;
   delete( actResult.StatTimestamp ) ;
   var expResult = {
     "Collection": COMMCSNAME + "." + testConf.clName,
     "InternalV": curInternalVersion,
     "IsDefault": true,
     "IsExpired": false,
     "AvgNumFields": 10,
     "SampleRecords": 200,
     "TotalRecords": 200,
     "TotalDataPages": 1,
     "TotalDataSize":80000
   } ;
   if( !commCompareObject( expResult, actResult ) ){
     throw new Error( "\Expected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) ) ;
   }
 }