/******************************************************************************
 * @Description   : seqDB-26310:存在全文索引，truncate后dropCL，恢复truncate 
 * @Author        : liuli
 * @CreateTime    : 2022.03.31
 * @LastEditTime  : 2022.04.05
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ()
{
   var groupNames = commGetDataGroupNames( db );
   var csName = "cs_26310";
   var clName = "cl_26310";
   var fullIndexName = "fullIndex_26310";

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

   // 执行truncate后dropCL
   dbcl.truncate();
   dbcs.dropCL( clName );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   count = 0;
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
   // 恢复后进行校验
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkFullSyncToES( csName, clName, fullIndexName, 1000 );

   dropCL( db, csName, clName, true, true );
   dropCS( db, csName, true );
   cleanRecycleBin( db, csName );
}