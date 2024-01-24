/******************************************************************************
 * @Description   : seqDB-24414:PreferedInstance取值不是instanceid时，设置PreferedStrict
 *                : seqDB-24417:PreferedInstance由角色取值改为实例取值，PreferedStrict正常生效
 * @Author        : liuli
 * @CreateTime    : 2021.10.08
 * @LastEditTime  : 2021.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;
testConf.clName = COMMCLNAME + "_24414";
testConf.useSrcGroup = true;

main( test )
function test ( args )
{
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // seqDB-24414测试点
   db.getSessionAttr( { "PreferedInstance": "M" } );
   setAndCheckPreferedStrict( db, true );
   setAndCheckPreferedStrict( db, false );

   db.getSessionAttr( { "PreferedInstance": "A" } );
   setAndCheckPreferedStrict( db, true );
   setAndCheckPreferedStrict( db, false );

   db.getSessionAttr( { "PreferedInstance": "S" } );
   setAndCheckPreferedStrict( db, true );
   setAndCheckPreferedStrict( db, false );

   // seqDB-24417测试点
   // 设置PreferedStrict为value
   db.setSessionAttr( { "PreferedStrict": true } );

   // 获取CL所在的节点
   var nodes = commGetGroupNodes( db, srcGroup );
   var nodeName = nodes[0].HostName + ":" + nodes[0].svcname;
   try
   {
      // 节点配置instanceid
      var instanceid = 17;
      updateConf( db, { instanceid: instanceid }, { NodeName: nodeName }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( srcGroup ).getNode( nodeName ).stop();
      db.getRG( srcGroup ).getNode( nodeName ).start();

      commCheckBusinessStatus( db );
      db.invalidateCache();

      // 设置PreferedStrict为instanceid
      db.setSessionAttr( { "PreferedInstance": instanceid } );

      // 查询访问计划并校验节点
      var dbcl = db.getCS( COMMCSNAME ).getCL( testConf.clName );
      var explain = dbcl.find().explain();
      var actNodeName = explain.current().toObj()["NodeName"];
      assert.equal( actNodeName, nodeName );

      // 设置不存在的instanceid
      db.setSessionAttr( { "PreferedInstance": 11 } );
      assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
      {
         dbcl.find().toArray();
      } );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { NodeName: nodeName }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( srcGroup ).getNode( nodeName ).stop();
      db.getRG( srcGroup ).getNode( nodeName ).start();
      commCheckBusinessStatus( db );
   }
}

function setAndCheckPreferedStrict ( db, value )
{
   db.setSessionAttr( { "PreferedStrict": value } );
   var obj = db.getSessionAttr().toObj();
   assert.equal( obj.PreferedStrict, value );
}