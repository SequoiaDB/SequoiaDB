/******************************************************************************
 * @Description   : seqDB-22873:源集群上删除cs
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22873";
   var csName = "cs_22873";
   var srcCSName = "datasrcCS_22873";
   commDropCS( datasrcDB, srcCSName );
   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );
   commCreateCS( datasrcDB, csName );
   commCreateCL( datasrcDB, csName, srcCLName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   testDropCS( dataSrcName, csName, srcCSName, srcCLName );
   testDropCS( dataSrcName, csName, csName, srcCLName );

   db.dropDataSource( dataSrcName );
   datasrcDB.close();
}

function testDropCS ( dataSrcName, csName, srcCSName, srcCLName )
{
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var cl = db.getCS( csName ).getCL( srcCLName );
   cl.insert( { a: 1 } );
   db.dropCS( csName );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );
   var count = datasrcDB.getCS( srcCSName ).getCL( srcCLName ).count();
   assert.equal( 1, count );
   datasrcDB.dropCS( srcCSName );
}

