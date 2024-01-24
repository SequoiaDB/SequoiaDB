/******************************************************************************
 * @Description   : seqDB-22895: 设置会话超时属性访问数据源
 * @Author        : Wu Yan
 * @CreateTime    : 2021.06.01
 * @LastEditTime  : 2021.06.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22895";
   var clName = CHANGEDPREFIX + "_datasource22895";
   var srcCSName = "datasrcCS_22895";
   var csName = "DS_22895";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB );
   var groupName = groups[0][0].GroupName;
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );

   var cs = db.createCS( csName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var recordNum = 50000;
   var timeoutValue = 1000;
   insertBulkData( dbcl, recordNum );

   db.setSessionAttr( { PreferedInstance: "s", Timeout: timeoutValue } );
   var updateValue = "asdgasdgasdgasdgasdgasdgas@#$$%%FFFFFFFFFFFFFFFFF";
   dbcl = db.getCS( csName ).getCL( clName );
   assert.tryThrow( [SDB_TIMEOUT], function()
   {
      dbcl.update( { $set: { c: updateValue } } );
   } );

   timeoutValue = 20000;
   db.setSessionAttr( { PreferedInstance: "s", Timeout: timeoutValue } );
   dbcl = db.getCS( csName ).getCL( clName );
   dbcl.update( { $set: { c: updateValue } } );
   var updateCount = dbcl.find( { c: updateValue } ).count();
   assert.equal( updateCount, recordNum );

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   commDropCL( db, csName, clName, false, false );
   clearDataSource( csName, dataSrcName );
}

