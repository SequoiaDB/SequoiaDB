/***************************************************************************
@Description :seqDB-12078 :集合中的全文索引字段与普通索引字段相同，对该字段进行事务回滚  
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

   var clName = COMMCLNAME + "_ES_12078";
   var csName = "testCS_ES_12078";
   dropCS( db, csName );

   //创建全文索引及普通索引，索引字段相同
   var dbcl = commCreateCL( db, csName, clName );
   commCreateIndex( dbcl, "fullIndex", { content: "text" } );
   commCreateIndex( dbcl, "commIndex", { content: 1 } );
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      var record = { content: "a" + i, age: i + 10 };
      records.push( record );
   }
   dbcl.insert( records );

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
