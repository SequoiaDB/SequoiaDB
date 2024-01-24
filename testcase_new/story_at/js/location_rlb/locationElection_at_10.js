/***************************************************************************************************
 * @Description: DC支持设置位置信息
 * @ATCaseID: locationElection_at_10
 * @Author: Jiangfeng You
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 04/10/2023 Jiangfeng You  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含两个复制组的域
 * 测试场景：
 *    测试 DC支持设置位置信息
 * 测试步骤：
 *    1. DC的复制组指定不同的位置信息
 *    2. 通过DC设置位置信息
 *    3. DC的复制组指定不同的位置信息
 *    4. 通过DC清空位置信息
 * 期望结果：
 *    步骤2：复制组被批量设置位置信息
 *    步骤4：复制组被批量清空位置信息
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

   var hostName = groups[0][1].HostName ;

   for ( var i =0;i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for ( var j =1; j<groups[i].length; j++ )
      {
         var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
         node.setLocation(location+"_"+i+"_"+j);
      }
   }

   dc.setLocation( hostName, location ) ;
   commCheckBusinessStatus(db);

   for( var i=0; i<groups.length;i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for ( var j =1; j<groups[i].length; j++ )
      {
         if ( hostName === groups[i][j].HostName )
         {
            var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
            checkNodeLocation(node,location);
         }
      }
   }

   for ( var i =0;i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for ( var j =1; j<groups[i].length; j++ )
      {
         var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
         node.setLocation(location+"_"+i+"_"+j);
      }
   }

   dc.setLocation( hostName, "" ) ;
   commCheckBusinessStatus(db);

   for( var i=0; i<groups.length;i++ )
   {
      var rg = db.getRG( groups[i][0].GroupName );
      for ( var j =1; j<groups[i].length; j++ )
      {
         if ( hostName === groups[i][j].HostName )
         {
            var node = rg.getNode(groups[i][j].HostName,groups[i][j].svcname) ;
            checkNodeLocation(node,undefined);
         }
      }
   }

}


