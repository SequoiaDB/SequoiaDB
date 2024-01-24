/******************************************************************************
 * @Description   : seqDB-22861 :: seqDB-22861:查看数据源列表
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "数据源datasrc22861";
   var csName = "cs_22861";
   var srcCSName = "datasrcCS_22861";
   var srcCLName = "srccl_22861";
   var datasourceNum = 200;
   commDropCS( datasrcDB, srcCSName );
   clearDataSources( csName, datasourceNum, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName, { ShardingKey: { a: 1 } } );
   var cs = db.createCS( csName );
   var datasourceNames = createDataSourceAndCreateCL( cs, dataSrcName, datasourceNum, srcCSName, srcCLName );

   var cur1 = db.listDataSources( { "Name": Regex( dataSrcName, "i" ) } );
   listAndCheckResult( cur1, datasourceNames );

   var cur2 = db.list( SDB_LIST_DATASOURCES, { "Name": Regex( dataSrcName, "i" ) } );
   listAndCheckResult( cur2, datasourceNames );

   var cur3 = db.list( 22, { "Name": Regex( dataSrcName, "i" ) } );
   listAndCheckResult( cur3, datasourceNames );

   clearDataSources( csName, datasourceNum, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function listAndCheckResult ( cur, expDataSourceNames )
{
   var actDataSourceNames = [];
   while( cur.next() )
   {
      var actName = cur.current().toObj().Name;
      actDataSourceNames.push( actName );
   }
   cur.close();
   expDataSourceNames.sort();
   actDataSourceNames.sort();
   assert.equal( actDataSourceNames, expDataSourceNames );
}

function createDataSourceAndCreateCL ( cs, dataSrcName, datasourceNum, srcCSName, srcCLName )
{
   var datasourceNames = [];
   for( var i = 0; i < datasourceNum; i++ )
   {
      var name = dataSrcName + "_" + i;
      var clNameSub = "cl_22861" + "_" + i;
      db.createDataSource( name, datasrcUrl, userName, passwd );
      datasourceNames.push( name );
      cs.createCL( clNameSub, { DataSource: name, Mapping: srcCSName + "." + srcCLName } );
   }
   return datasourceNames;
}

function clearDataSources ( csName, datasouceNum, dataSrcName )
{
   try
   {
      db.dropCS( csName );
   }
   catch( e )
   {
      if( e != SDB_DMS_CS_NOTEXIST )
      {
         throw new Error( e );
      }
   }

   var cur = db.listDataSources( { "Name": Regex( "数据源datasrc", "i" ) } );
   while( cur.next() )
   {
      var actName = cur.current().toObj().Name;
      db.dropDataSource( actName );
   }
   cur.close();
}

