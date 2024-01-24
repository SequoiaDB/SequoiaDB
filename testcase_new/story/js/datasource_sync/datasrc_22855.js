/******************************************************************************
 * @Description   : seqDB-22855:修改数据源InheritSessionAttr属性
 * @Author        : Wu Yan
 * @CreateTime    : 2021.06.04
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var dataSrcName = "datasrc22855";
   var clName = "datasource22855";
   var srcCSName = "datasrcCS_22855";
   var csName = "DS_22855";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );
   var groupNames = commGetCLGroups( datasrcDB, srcCSName + "." + clName );
   var groupName = groupNames[0];

   var cs = db.createCS( csName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { InheritSessionAttr: true } );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];

   var instanceid = 10;
   var srcSlaveNode = datasrcDB.getRG( groupName ).getSlave();
   var srcMasterNode = datasrcDB.getRG( groupName ).getMaster();
   var expAccessNodeS = [srcSlaveNode.toString()];
   var expAccessNodeM = [srcMasterNode.toString()];

   try
   {
      updateConf( datasrcDB, { instanceid: instanceid }, { NodeName: srcSlaveNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      srcSlaveNode.stop();
      srcSlaveNode.start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      //数据源指定继承会话属性    
      db.setSessionAttr( { PreferedInstance: instanceid, PreferedPeriod: 0 } );
      dbcl.insert( docs );
      //访问指定备节点
      findAndCheckAccessNodes( dbcl, expAccessNodeS );

      //修改为不继承会话属性,停留在了当前设置的属性状态
      var ds = db.getDataSource( dataSrcName );
      ds.alter( { InheritSessionAttr: false } );
      findAndCheckAccessNodes( dbcl, expAccessNodeS );
      db.setSessionAttr( { PreferedInstance: 'M', PreferedPeriod: 0 } );
      findAndCheckAccessNodes( dbcl, expAccessNodeS );
      //新建连接
      var dbnew = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var dbclnew = dbnew.getCS( csName ).getCL( clName );
      findAndCheckAccessNodes( dbclnew, expAccessNodeM );
      dbnew.close();

      //修改为继承会话属性
      db.setSessionAttr( { PreferedInstance: instanceid, PreferedPeriod: 0 } );
      ds.alter( { InheritSessionAttr: true } );
      findAndCheckAccessNodes( dbcl, expAccessNodeS );

   }
   finally
   {
      srcSlaveNode.start();
      deleteConf( datasrcDB, { instanceid: 1 }, { NodeName: srcSlaveNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      srcSlaveNode.stop();
      srcSlaveNode.start();
      commCheckBusinessStatus( datasrcDB );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( csName, dataSrcName );
}