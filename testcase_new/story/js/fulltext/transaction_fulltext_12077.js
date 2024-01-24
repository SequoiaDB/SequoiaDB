/***************************************************************************
@Description :seqDB-12077 :集合中存在全文索引进行事务回滚  
@Modify list :
              2018-11-06  YinZhen  Create
****************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12077";
   var csName = "testCS_ES_12077";
   dropCS( db, csName );

   //创建全文索引及普通索引
   var dbcl = commCreateCL( db, csName, clName );
   commCreateIndex( dbcl, "fullIndex", { content: "text" } );
   commCreateIndex( dbcl, "commIndex", { about: 1 } );
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      var record = { content: "a" + i, about: "a" + i };
      records.push( record );
   }
   dbcl.insert( records );

   //操作字段覆盖：上面：全文索引字段、下面：普通索引字段
   //insert
   db.transBegin();
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      var record = { content: "a" + i, age: i + 10 };
      records.push( record );
   }
   dbcl.insert( records );
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 10 );
   checkConsistency( csName, clName );

   var dbOperator = new DBOperator();
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var esOperator = new ESOperator();
   var esIndexNames = dbOperator.getESIndexNames( csName, clName, "fullIndex" );
   var esIndexName = esIndexNames[0];
   var queryCond = '{"query" : {"exists" : {"field" : "content"}}}';
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   print( "actResult: " + JSON.stringify( actResult ) + " \nexpResult: " + JSON.stringify( expResult ) );
   checkResult( expResult, actResult );

   db.transBegin();
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      var record = { about: "a" + i, age: i + 10 };
      records.push( record );
   }
   dbcl.insert( records );
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 10 );
   checkConsistency( csName, clName );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   checkResult( expResult, actResult );

   //update
   db.transBegin();
   dbcl.update( { $set: { content: "i can not do it" } }, { content: "a2" } );
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 10 );
   checkConsistency( csName, clName );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   checkResult( expResult, actResult );

   db.transBegin();
   dbcl.update( { $set: { about: "how are you" } }, { about: "a3" } );
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 10 );
   checkConsistency( csName, clName );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   checkResult( expResult, actResult );

   //delete
   db.transBegin();
   dbcl.remove( { content: "a3" } );
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 10 );
   checkConsistency( csName, clName );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   checkResult( expResult, actResult );

   db.transBegin();
   dbcl.remove( { about: "a4" } );
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 10 );
   checkConsistency( csName, clName );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   checkResult( expResult, actResult );

   //truncate
   db.transBegin();
   dbcl.truncate();
   db.transRollback();

   checkFullSyncToES( csName, clName, "fullIndex", 0 );
   checkConsistency( csName, clName );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { content: "" } );
   var actResult = esOperator.findFromES( esIndexName, queryCond );

   actResult.sort( compare( "content" ) );
   expResult.sort( compare( "content" ) );
   checkResult( expResult, actResult );

   dropCS( db, csName );
}
