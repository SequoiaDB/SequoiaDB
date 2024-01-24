/***************************************************************************************************
 * @Description: DC支持设置位置信息
 * @ATCaseID: locationElection_at_8
 * @Author: Jiangfeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/10/2023 Jiangfeng You  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个DC
 * 测试场景：
 *    测试DC支持设置位置信息
 * 测试步骤：
 *    1. 指定主机名 与 SYSNODES中的主机名不一致
 *    2. 设置节点location大于256个字符
 *    3. 指定hostName 与 SYSNODES中的主机名一致
 *    4. 清空DC的位置信息
 * 期望结果：
 *    步骤1：域没有位置信息
 *    步骤2：报错
 *    步骤3：DC有位置信息
 *    步骤4：DC没有位置信息
 *
 * 说明：无
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
   var dc = db.getDC() ;

   var hostName = "localhost" ;
   var location = "dc_location" ;
   var groups = commGetGroups( db, "", "",false, false );

   dc.setLocation( hostName, location ) ;
   for( var i=0; i<groups.length;i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for( var j = 1;j<groups[i].length;j++ )
      {
         if ( hostName === groups[i][j].HostName )
         {
            var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
            checkNodeLocation(node,undefined);
         }
      }
   }

   var hostName = groups[0][1].HostName ;

   // 设置节点location大于256个字符
   var arr = new Array( 258 );
   var locationName = arr.join( "a" );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dc.setLocation( hostName, locationName );
   } );

   dc.setLocation( hostName, location ) ;

   for( var i=0; i<groups.length;i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for( var j = 1;j<groups[i].length;j++ )
      {
         if ( hostName === groups[i][j].HostName )
         {
            var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
            checkNodeLocation(node,location);
         }
      }
   }

   dc.setLocation( hostName, "" ) ;

   for( var i=0; i<groups.length;i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for( var j = 1;j<groups[i].length;j++ )
      {
         if ( hostName === groups[i][j].HostName )
         {
            var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
            checkNodeLocation(node,undefined);
         }
      }
   }

}


