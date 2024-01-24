/***************************************************************************************************
 * @Description: 域支持设置位置信息
 * @ATCaseID: locationElection_at_7
 * @Author: Jiangfeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/07/2023 Jiangfeng You  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个复制组
 * 测试场景：
 *    测试域 支持设置位置信息
 * 测试步骤：
 *    1. 指定主机名 与 SYSNODES中的主机名不一致
 *    2. 设置节点location大于256个字符
 *    3. 指定hostName 与 SYSNODES中的主机名一致
 *    4. 清空域的位置信息
 * 期望结果：
 *    步骤1：域没有位置信息
 *    步骤2：报错
 *    步骤3：域有位置信息
 *    步骤4：域没有位置信息
 *
 * 说明：无
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
   commCheckBusinessStatus(db);

   var groupName = "group_location_rlb";
   var location = "location_locationElection_at_7";

   var domainName = "domain_location_rlb" ;
   commDropDomain( db,domainName ) ;
   var domain = db.createDomain( domainName, [groupName] ) ;

   var hostName = "localhost" ;

   // 指定hostName 与 SYSNODES中的hostname不一致
   domain.setLocation( hostName, location ) ;
   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var i = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[i].Location, undefined ) ;
   }

   // 设置节点location大于256个字符
   var arr = new Array( 258 );
   var locationName = arr.join( "a" );
   var rg = db.getRG(groupName);
   var hostName = rg.getMaster().getHostName();
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      domain.setLocation( hostName, locationName );
   } );

   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var i = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[i].Location, undefined ) ;
   }

   // 指定hostName 与 SYSNODES中的hostname一致
   var rg = db.getRG(groupName);
   var hostName = rg.getMaster().getHostName();

   domain.setLocation( hostName, location ) ;
   var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 34);
   var nodeList = rg.getDetailObj().toObj();
   for ( var i = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[i].Location, location ) ;
   }

   domain.setLocation( hostName, location ) ;
   var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 34);
   var groupsObj = domain.listGroups(domainName).current().toObj().Groups ;
   for ( var i = 0; i<groupsObj.length; i++ )
   {
      var groupName = groupsObj[i].GroupName ;
      var rg = db.getRG(groupName);
      var nodeList = rg.getDetailObj().toObj();
      for ( var j = 0; i<nodeList.length; i++ )
      {
         assert.equal( nodeList[j].Location, location ) ;
      }
   }

   // 清空域的位置信息
   domain.setLocation( hostName, "" ) ;
   for ( var i = 0; i<groupsObj.length; i++ )
   {
      var groupName = groupsObj[i].GroupName ;
      var rg = db.getRG(groupName);
      var nodeList = rg.getDetailObj().toObj();
      for ( var j = 0; i<nodeList.length; i++ )
      {
         assert.equal( nodeList[j].Location, undefined ) ;
      }
   }
   var rg = db.getRG(groupName);
   var nodeList = rg.getDetailObj().toObj();
   for ( var i = 0; i<nodeList.length; i++ )
   {
      assert.equal( nodeList[i].Location, undefined ) ;
   }
   commDropDomain( db,domainName ) ;
}

