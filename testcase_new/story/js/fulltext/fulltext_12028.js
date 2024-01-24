/******************************************************************************
@Description :   seqDB-12028:更新全文索引字段但不更新值
@Modify list :   2018-10-08  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_12028";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_12028";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   dbcl.insert( { _id: 1, a: "text1" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   dbcl.update( { $set: { a: "text1" } } );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
