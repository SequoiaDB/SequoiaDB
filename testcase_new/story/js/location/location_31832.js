/******************************************************************************
 * @Description   : seqDB-31832:通过复制组删除ActiveLocation
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.25
 * @LastEditTime  : 2023.06.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31832_1";

   var dataGroupName = commGetDataGroupNames( db )[0];
   // 验证data复制组
   deleteActiveLocation( db, location, dataGroupName );
   // 验证cata复制组
   deleteActiveLocation( db, location, CATALOG_GROUPNAME );
}

function deleteActiveLocation ( db, location, group )
{
   // 获取group 中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, group );
   var rg = db.getRG( group );
   // 复制组不存在ActiveLocation，删除ActiveLocation
   rg.setActiveLocation( "" );
   checkGroupActiveLocation( db, group, undefined );

   // 复制组存在ActiveLocation，删除ActiveLocation
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   slaveNode1.setLocation( location );
   slaveNode2.setLocation( location );

   // 设置ActiveLocation为location
   rg.setActiveLocation( location );
   checkGroupActiveLocation( db, group, location );

   rg.setActiveLocation( "" );
   checkGroupActiveLocation( db, group, undefined );

   // 移除group设置的location
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}