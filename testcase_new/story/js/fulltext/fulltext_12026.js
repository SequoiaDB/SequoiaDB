/******************************************************************************
@Description :   seqDB-12026:AutoIndexId为false时更新全文索引字段值
@Modify list :   2018-10-10  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_12026";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "", b: "" };
   var textIndexName = "a_12026";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIndexId: false } );

   commCreateIndex( dbcl, textIndexName, { a: "text", b: "text" } );

   dbcl.insert( { a: "text1", b: "text1" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   update( dbcl );
   dbcl.insert( { a: "new", b: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function update ( dbcl )
{
   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      dbcl.update( { $set: { a: "text2", b: "text2" } } );
   } );
}
