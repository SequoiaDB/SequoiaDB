/******************************************************************************
 * @Description   : seqDB-23823:分区表且被切分到多个数据组，存在全文索引，truncate后恢复
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
   var csName = "cs_23823";
   var clName = "cl_23823";
   var fullIndexName = "fullIndex_23823";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建切分表，创建全文索引
   commCreateCS( db, csName );
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

   // 执行truncate后恢复
   dbcl.truncate();
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

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
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkFullSyncToES( csName, clName, fullIndexName, 1000 );

   dropCL( db, csName, clName, true, true );
   dropCS( db, csName, true );
   cleanRecycleBin( db, csName );
   cleanRecycleBin( db, "local_test" );
}