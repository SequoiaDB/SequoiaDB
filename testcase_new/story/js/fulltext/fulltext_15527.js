/******************************************************************************
@Description :    seqDB-15527:更新_id字段，类型为不支持同步的类型
@Modify list :   2018-9-28  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_15527";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_15527";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   insertData( dbcl );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   update( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   update( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   for( var i = 0; i < 4; i++ )
   {
      dbcl.insert( { a: "a" + i } );
   }
}

function update ( dbcl )
{
   dbcl.update( { $set: { _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a0" } );
   dbcl.update( { $set: { _id: null } }, { a: "a1" } );
   dbcl.update( { $set: { _id: { "$minKey": 1 } } }, { a: "a2" } );
   dbcl.update( { $set: { _id: { "$maxKey": 1 } } }, { a: "a3" } );
}
