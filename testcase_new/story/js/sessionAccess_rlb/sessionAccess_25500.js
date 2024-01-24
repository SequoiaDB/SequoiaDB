/******************************************************************************
 * @Description   : seqDB-25500:执行updateConf（）设置PerferredConstraint为SecondaryOnly，指定优先访问备节点S 
 * @Author        : liuli
 * @CreateTime    : 2022.03.15
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25500";
testConf.clName = COMMCLNAME + "_25500";
testConf.clOpt = { ReplSize: -1 };
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var cl = args.testCL;
   insertBulkData( cl, 1000 );

   var srcGroup = args.srcGroupName;

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, srcGroup, instanceids );

      // 获取cl所在组的备节点
      var slaveNodeNames = getGroupSlaveNodeName( db, srcGroup );

      // 更新节点配置preferredconstraint和preferredperiod
      db.updateConf( { "preferredconstraint": "secondaryonly", "preferredinstance": "S" } );
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var dbcl = sdb.getCS( testConf.csName ).getCL( testConf.clName );

      // 查看访问计划，访问节点为备节点
      explainAndCheckAccessNodes( dbcl, slaveNodeNames );

      // 检查会话属性
      var expResult = { PreferredConstraint: "SecondaryOnly", PreferedInstance: "S", PreferredInstance: "S" };
      checkSessionAttr( sdb, expResult );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: srcGroup }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.deleteConf( { "preferredconstraint": 1, "preferredinstance": 1 } );

      db.getRG( srcGroup ).stop();
      db.getRG( srcGroup ).start();
      commCheckBusinessStatus( db );
      sdb.close();
   }
}

function checkSessionAttr ( db, expResult )
{
   var actResult = {};
   var object = db.getSessionAttr().toObj();
   actResult.PreferredConstraint = object.PreferredConstraint;
   actResult.PreferedInstance = object.PreferedInstance;
   actResult.PreferredInstance = object.PreferredInstance;
   assert.equal( actResult, expResult );
}
