/******************************************************************************
 * @Description   : seqDB-31836:通过域删除ActiveLocation
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.05.24
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var domainName = "domain_31836";
   var location1 = "guangzhou.nansha_31836";
   var location2 = "guangzhou.panyu_31836";

   var dataGroupName1 = commGetDataGroupNames( db )[0];
   var dataGroupName2 = commGetDataGroupNames( db )[1];

   commDropDomain( db, domainName );
   var domain = commCreateDomain( db, domainName, [dataGroupName1, dataGroupName2] );

   // domain中的组没有设置ActiveLocation，通过域删除ActiveLocation
   domain.setActiveLocation( "" );
   checkGroupActiveLocation( db, dataGroupName1, null );
   checkGroupActiveLocation( db, dataGroupName2, null );

   // domain中组已设置ActiveLocation，通过域删除ActiveLocation
   // 给domain中的组设置ActiveLocation
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName1 );
   var rg = db.getRG( dataGroupName1 );
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );
   rg.setActiveLocation( location1 );

   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName2 );
   var rg = db.getRG( dataGroupName2 );
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );
   rg.setActiveLocation( location1 );

   checkGroupActiveLocation( db, dataGroupName1, location1 );
   checkGroupActiveLocation( db, dataGroupName2, location1 );

   domain.setActiveLocation( "" );
   checkGroupActiveLocation( db, dataGroupName1, null );
   checkGroupActiveLocation( db, dataGroupName2, null );


   // domain中的组已设置ActiveLocation，移除部分组后通过域删除ActiveLocation
   // 给domain中的组设置ActiveLocation
   var rg = db.getRG( dataGroupName1 );
   rg.setActiveLocation( location1 );
   var rg = db.getRG( dataGroupName2 );
   rg.setActiveLocation( location1 );
   checkGroupActiveLocation( db, dataGroupName1, location1 );
   checkGroupActiveLocation( db, dataGroupName2, location1 );

   domain.setGroups( { Groups: [dataGroupName1] } );

   domain.setActiveLocation( "" );
   checkGroupActiveLocation( db, dataGroupName1, null );
   checkGroupActiveLocation( db, dataGroupName2, location1 );

   // 清理环境
   commDropDomain( db, domainName );
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName1 );
   var rg = db.getRG( dataGroupName1 );
   rg.getMaster().setLocation( "" );
   rg.getNode( slaveNodes[0] ).setLocation( "" );
   rg.getNode( slaveNodes[1] ).setLocation( "" );

   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName2 );
   var rg = db.getRG( dataGroupName2 );
   rg.getMaster().setLocation( "" );
   rg.getNode( slaveNodes[0] ).setLocation( "" );
   rg.getNode( slaveNodes[1] ).setLocation( "" );
}
