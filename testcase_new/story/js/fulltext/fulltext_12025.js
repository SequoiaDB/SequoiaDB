/******************************************************************************
@Description :   seqDB-12025:使用find.update更新全文索引字段
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
   var clName = COMMCLNAME + "_ES_12025";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_12025";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   dbcl.insert( { _id: 1, a: "a1" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   update( dbcl );
   dbcl.insert( { _id: 3, a: "a2" } );

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
   var cursor = dbcl.find().update( { $set: { _id: "2", a: "a2" } } );
   while( cursor.next() ) { }
}
