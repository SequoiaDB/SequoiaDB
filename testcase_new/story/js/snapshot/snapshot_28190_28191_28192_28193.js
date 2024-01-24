/******************************************************************************
 * @Description   : seqDB-28190:集合快照中新增创建/更新时间信息验证
 *                  seqDB-28191:集合空间快照中新增创建/更新时间信息验证 
 *                  seqDB-28192:存储单元列表信息新增创建时间和修改时间        
 *                  seqDB-28193:编目快照中新增创建/更新时间信息验证 
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.10.19
 * @LastEditTime  : 2022.11.04
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var csName = "csName_28190";
   var clName = "clName_28190";
   var clName1 = "clName_28190_1";
   var newClName = "newClName_28190";
   var newCsName = "newCsName_28190";
   var groupName = commGetDataGroupNames( db )[0];
   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();
   var cond = { Name: csName + "." + clName, HostName: node.getHostName(), ServiceName: node.getServiceName() };

   // 1.创建CS和CL
   commDropCS( db, csName );
   commCreateCS( db, csName );
   db.getCS( csName ).createCL( clName, { Group: groupName } );
   // 查看集合空间快照信息
   var cursor1_1 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName, NodeName: nodeName } );
   var createTime1_1 = cursor1_1.current().toObj()["CreateTime"];
   var updateTime1_1 = cursor1_1.current().toObj()["UpdateTime"];
   // 查看集合快照信息
   var cursor2_1 = db.snapshot( SDB_SNAP_COLLECTIONS, cond );
   var createTime2_1 = cursor2_1.current().toObj()["Details"][0]["Group"][0]["CreateTime"];
   var updateTime2_1 = cursor2_1.current().toObj()["Details"][0]["Group"][0]["UpdateTime"];
   // 查看编目快照信息
   var cursor3_1 = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var createTime3_1 = cursor3_1.current().toObj()["CreateTime"];
   var updateTime3_1 = cursor3_1.current().toObj()["UpdateTime"];
   // 查看存储单元列表信息
   var db1 = new Sdb( node.getHostName(), node.getServiceName() );
   var cursor4_1 = db1.list( SDB_LIST_STORAGEUNITS, { Name: csName } );
   var createTime4_1 = cursor4_1.current().toObj()["CreateTime"];
   var updateTime4_1 = cursor4_1.current().toObj()["UpdateTime"];

   // 2.更新CL属性信息 
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      db.getCS( csName ).getCL( clName ).disableCompression();
   } );
   // 查看集合空间快照信息
   var cursor1_2 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName, NodeName: nodeName } );
   var createTime1_2 = cursor1_2.current().toObj()["CreateTime"];
   var updateTime1_2 = cursor1_2.current().toObj()["UpdateTime"];
   assert.equal( createTime1_2, createTime1_1, "expected createTime equal" );
   if( updateTime1_2 != updateTime1_1 )
   {
      throw new Error( "expected post-updateTime to be equal to pre-updateTime" );
   }
   // 查看集合快照信息
   var cursor2_2 = db.snapshot( SDB_SNAP_COLLECTIONS, cond );
   var createTime2_2 = cursor2_2.current().toObj()["Details"][0]["Group"][0]["CreateTime"];
   var updateTime2_2 = cursor2_2.current().toObj()["Details"][0]["Group"][0]["UpdateTime"];
   assert.equal( createTime2_2, createTime2_1, "expected createTime equal" );
   if( updateTime2_2 != updateTime2_1 )
   {
      throw new Error( "expected post-updateTime to be equal to pre-updateTime" );
   }
   // 查看编目快照信息
   var cursor3_2 = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var createTime3_2 = cursor3_2.current().toObj()["CreateTime"];
   var updateTime3_2 = cursor3_2.current().toObj()["UpdateTime"];
   assert.equal( createTime3_2, createTime3_1, "expected createTime equal" );
   if( updateTime3_2 != updateTime3_1 )
   {
      throw new Error( "expected post-updateTime to be equal to pre-updateTime" );
   }

   // 3.修改CL名
   db.getCS( csName ).renameCL( clName, newClName );
   // 查看集合空间快照信息
   var cursor1_3 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName, NodeName: nodeName } );
   var createTime1_3 = cursor1_3.current().toObj()["CreateTime"];
   var updateTime1_3 = cursor1_3.current().toObj()["UpdateTime"];
   assert.equal( createTime1_3, createTime1_2, "expected createTime equal" );
   if( updateTime1_3 <= updateTime1_2 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }
   // 查看集合快照信息
   var cond = { Name: csName + "." + newClName, HostName: node.getHostName(), ServiceName: node.getServiceName() };
   var cursor2_3 = db.snapshot( SDB_SNAP_COLLECTIONS, cond );
   var createTime2_3 = cursor2_3.current().toObj()["Details"][0]["Group"][0]["CreateTime"];
   var updateTime2_3 = cursor2_3.current().toObj()["Details"][0]["Group"][0]["UpdateTime"];
   assert.equal( createTime2_3, createTime2_2, "expected createTime equal" );
   if( updateTime2_3 <= updateTime2_2 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }
   // 查看编目快照信息
   var cursor3_3 = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + newClName } );
   var createTime3_3 = cursor3_3.current().toObj()["CreateTime"];
   var updateTime3_3 = cursor3_3.current().toObj()["UpdateTime"];
   assert.equal( createTime3_3, createTime3_2, "expected createTime equal" );
   if( updateTime3_3 <= updateTime3_2 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }

   // 4.修改CS名
   db.renameCS( csName, newCsName );
   // 查看集合空间快照信息
   var cursor1_4 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: newCsName, NodeName: nodeName } );
   var createTime1_4 = cursor1_4.current().toObj()["CreateTime"];
   var updateTime1_4 = cursor1_4.current().toObj()["UpdateTime"];
   assert.equal( createTime1_4, createTime1_3, "expected createTime equal" );
   if( updateTime1_4 <= updateTime1_3 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }

   // 5.CS下新增CL
   db.getCS( newCsName ).createCL( clName1, { Group: groupName } );
   // 查看集合空间快照信息
   var cursor1_5 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: newCsName, NodeName: nodeName } );
   var createTime1_5 = cursor1_5.current().toObj()["CreateTime"];
   var updateTime1_5 = cursor1_5.current().toObj()["UpdateTime"];
   assert.equal( createTime1_5, createTime1_4, "expected createTime equal" );
   if( updateTime1_5 <= updateTime1_4 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }
   // 查看存储单元列表信息
   var cursor4_2 = db1.list( SDB_LIST_STORAGEUNITS, { Name: newCsName } );
   var createTime4_2 = cursor4_2.current().toObj()["CreateTime"];
   var updateTime4_2 = cursor4_2.current().toObj()["UpdateTime"];
   assert.equal( createTime4_2, createTime4_1, "expected createTime equal" );
   if( updateTime4_2 <= updateTime4_1 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }

   // 6.删除CL
   db.getCS( newCsName ).dropCL( clName1 );
   // 查看集合空间快照信息
   var cursor1_6 = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: newCsName, NodeName: nodeName } );
   var createTime1_6 = cursor1_6.current().toObj()["CreateTime"];
   var updateTime1_6 = cursor1_6.current().toObj()["UpdateTime"];
   assert.equal( createTime1_6, createTime1_5, "expected createTime equal" );
   if( updateTime1_6 <= updateTime1_5 )
   {
      throw new Error( "expected post-updateTime to be more than pre-updateTime" );
   }

   commDropCS( db, csName );
   commDropCS( db, newCsName );
}
