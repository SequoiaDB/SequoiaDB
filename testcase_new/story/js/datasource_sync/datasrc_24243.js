/******************************************************************************
 * @Description   : seqDB-24243:源集群设置会话访问属性，设置preferdinstance指定实例和preferedPeriod属性
 * @Author        : Wu Yan
 * @CreateTime    : 2021.06.04
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/

testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc24243";
   var clName = CHANGEDPREFIX + "_datasource24243";
   var srcCSName = "datasrcCS_24243";
   var csName = "DS_24243";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB )[0];;
   var groupName = groups[0].GroupName;
   var primaryPos = groups[0].PrimaryPos;
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );

   var cs = db.createCS( csName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];

   var instanceid = 10;
   var srcSlaveNode = datasrcDB.getRG( groupName ).getSlave();
   var srcMasterNode = datasrcDB.getRG( groupName ).getMaster();

   try
   {
      updateConf( datasrcDB, { instanceid: instanceid }, { NodeName: srcSlaveNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      srcSlaveNode.stop();
      srcSlaveNode.start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      var expAccessNodeS = [srcSlaveNode.toString()];
      var expAccessNodeM = [srcMasterNode.toString()];

      //preferedPeriod为0     
      db.setSessionAttr( { PreferedInstance: instanceid, PreferedPeriod: 0 } );
      dbcl.insert( docs );
      findAndCheckAccessNodes( dbcl, expAccessNodeS );

      //preferedPeriod为5  
      db.setSessionAttr( { PreferedPeriod: 5 } );
      dbcl.insert( docs );
      findAndCheckAccessNodes( dbcl, expAccessNodeM );
      //5s后检查会话访问节点为备节点,时间是一个cpu的tick，不精准，改成10s后查看
      sleep( 10000 );
      findAndCheckAccessNodes( dbcl, expAccessNodeS );

      //preferedPeriod为-1
      db.setSessionAttr( { PreferedPeriod: -1 } )
      dbcl.insert( docs );
      findAndCheckAccessNodes( dbcl, expAccessNodeM );
      //5s后检查会话访问节点还是主节点
      sleep( 5000 );
      findAndCheckAccessNodes( dbcl, expAccessNodeM );

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


