/******************************************************************************
 * @Description   : seqDB-23804:分区表且被切分到多个数据组，存在全文索引，dropCL后恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.22
 * @LastEditTime  : 2022.04.05
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ()
{
   var groupNames = commGetDataGroupNames( db );
   var csName = "cs_23804";
   var clName = "cl_23804";
   var fullIndexName = "fullIndex_23804";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建切分表，创建全文索引
   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: groupNames[0] } );
   commCreateIndex( dbcl, fullIndexName, { a: "text", b: "text" } );

   // 切分数据并校验索引
   var records = [];
   for( var i = 0; i < 1000; i++ )
   {
      var record = { a: "a" + i, b: i };
      records.push( record );
   }
   dbcl.insert( records );

   dbcl.split( groupNames[0], groupNames[1], 30 );
   checkFullSyncToES( csName, clName, fullIndexName, 1000 );
   var doTime = 0;
   var timeOut = 120;
   var count = 0;
   while( doTime < timeOut )
   {
      try
      {
         count = dbcl.count( { "": { $Text: { "query": { "match_all": {} } } } } );
         if( count == 1000 )
         {
            break;
         }
      } catch( e ) { }
      sleep( 1000 );
      doTime++;
   }
   println( "find -- " + dbcl.count( { "": { $Text: { "query": { "match_all": {} } } } } ) );
   var dbOperator = new DBOperator();
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );

   // 删除CL后，检查CL和全文索引表均被删除
   dropCL( db, csName, clName, true, true );
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcs.getCL( clName );
   } );

   checkFullIndexNotExist( groupNames[0], fullIndexName );
   checkFullIndexNotExist( groupNames[1], fullIndexName );

   // 恢复后校验数据
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   count = 0
   var dbcl = db.getCS( csName ).getCL( clName );
   while( doTime < timeOut )
   {
      try
      {
         count = dbcl.count( { "": { $Text: { "query": { "match_all": {} } } } } );
         if( count == 1000 )
         {
            break;
         }
      } catch( e ) { }
      sleep( 1000 );
      doTime++;
   }
   println( "find -- " + dbcl.count( { "": { $Text: { "query": { "match_all": {} } } } } ) );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkFullSyncToES( csName, clName, fullIndexName, 1000 );

   dropCL( db, csName, clName, true, true );
   dropCS( db, csName, true );
   cleanRecycleBin( db, csName );
}

function checkFullIndexNotExist ( groupName, fullIndexName )
{
   var data = db.getRG( groupName ).getMaster().connect();
   var rc = data.listCollections();
   while( rc.next() )
   {
      var name = rc.current().toObj().Name;
      // 校验删除CL后data上不存在全文索引表
      assert.notEqual( name.slice( -15 ), fullIndexName );
   }
   rc.close();
}