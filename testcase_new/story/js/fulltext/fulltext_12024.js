/******************************************************************************
@Description :   seqDB-12024:使用upsert更新全文索引字段
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
   var clName = COMMCLNAME + "_ES_12024";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_12024";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   upsert( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   dbcl.insert( { _id: 1, a: "a1" } );
   dbcl.insert( { _id: 2, a: "a2" } );
}

function upsert ( dbcl )
{
   dbcl.upsert( { $set: { a: "a" } }, { _id: 1 } );
   dbcl.upsert( { $set: { _id: 3, a: "a3" } }, { _id: 3 } );
}
