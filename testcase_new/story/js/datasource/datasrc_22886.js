/******************************************************************************
 * @Description   : seqDB-22886:使用数据源创建集合空间，关联数据源上多个集合
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22886";
   var csName = "cs_22886";
   var clName = "cl_22886";
   var srcCSName = "datasrcCS_22886";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );

   var clNum = 100;
   for( var i = 0; i < clNum; i++ )
   {
      var name = clName + "_datasrc_" + i;
      commCreateCL( datasrcDB, srcCSName, name, { ShardingKey: { a: 1 } } );
   }

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   for( var i = 0; i < clNum; i++ )
   {
      var name = clName + "_datasrc_" + i;
      var dbcl = cs.getCL( name );
      insertAndfindRecords( dbcl );
   }

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function insertAndfindRecords ( dbcl )
{
   var recordNum = 2000;
   var expRecs = insertBulkData( dbcl, recordNum, 0, 4000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   var count = dbcl.count();
   assert.equal( count, recordNum );
}
