/***************************************************************************************************
 * @Description: 域支持设置位置信息
 * @ATCaseID: locationElection_at_9
 * @Author: Jiangfeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/10/2023 Jiangfeng You  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个复制组
 * 测试场景：
 *    测试域支持设置位置信息
 * 测试步骤：
 *    1. 域中的复制组指定不同的位置信息
 *    2. 通过域设置位置信息
 *    3. 域中的复制组指定不同的位置信息
 *    4. 通过域清空位置信息
 * 期望结果：
 *    步骤2：复制组被批量设置位置信息
 *    步骤4：复制组被批量清空位置信息
 *
 * 说明：无
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
   commCheckBusinessStatus(db);

   var groupName = "group_location_rlb";
   var location = "location_locationElection_at_9";

   var domainName = "domain_location_rlb" ;
   commDropDomain( db,domainName ) ;
   var domain = db.createDomain( domainName, [groupName] ) ;

   var rg = db.getRG(groupName);
   var hostName = rg.getMaster().getHostName();

   domain.setLocation( hostName, location ) ;
   var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 34);
   var nodeList = rg.getDetailObj().toObj();
   for ( var i = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[i].Location, location ) ;
   }

   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var j = 0; i<nodeList.length; i++ )
   {
      rg.setLocation(nodeList[j].HostName,location+"_"+i+"_"+j) ;
   }

   // 设置域的位置信息
   domain.setLocation( hostName, location ) ;
   var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 34);
   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var j = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[j].Location, location ) ;
   }

   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var j = 0; i<nodeList.length; i++ )
   {
      rg.setLocation(nodeList[j].HostName,location+"_"+i+"_"+j) ;
   }

   // 清空域的位置信息
   domain.setLocation( hostName, "" ) ;
   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var i = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[i].Location, undefined ) ;
   }
   commDropDomain( db,domainName ) ;
}

