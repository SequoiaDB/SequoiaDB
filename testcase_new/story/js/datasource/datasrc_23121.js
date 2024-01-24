/******************************************************************************
 * @Description   : seqDB-23121:创建数据源，指定地址为非coord节点地址
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc23121";

   clearDataSource( "nocs", dataSrcName );

   // 指定连接 catalog 节点
   var rgInfo = datasrcDB.getCataRG().getDetail().current().toObj().Group;
   var hostname = rgInfo[0].HostName;
   var svcname = rgInfo[0].Service[0].Name;
   var url = hostname + ":" + svcname;
   assert.tryThrow( [SDB_RTN_COORD_ONLY], function() 
   {
      db.createDataSource( dataSrcName, url, userName, passwd );
   } );

   // 指定连接 data 节点
   var dataGroups = commGetDataGroupNames( datasrcDB );
   var rgInfo = datasrcDB.getRG( dataGroups[0] ).getDetail().current().toObj().Group;
   var hostname = rgInfo[0].HostName;
   var svcname = rgInfo[0].Service[0].Name;
   var url = hostname + ":" + svcname;
   assert.tryThrow( [SDB_RTN_COORD_ONLY], function() 
   {
      db.createDataSource( dataSrcName, url, userName, passwd );
   } );
}