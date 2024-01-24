/******************************************************************************
@Description :   seqDB-15526:更新_id字段，类型为支持同步的类型
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
   var clName = COMMCLNAME + "_ES_15526";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_15526";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   insertData( dbcl );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateData( dbcl );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 11 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateData( dbcl );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 11 );

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
   for( var i = 0; i < 10; i++ )
   {
      dbcl.insert( { a: "a" + i } );
   }
}

function updateData ( dbcl )
{
   dbcl.update( { $set: { _id: 1 } }, { a: "a0" } );
   dbcl.update( { $set: { _id: { "$numberLong": "4354" } } }, { a: "a1" } );
   dbcl.update( { $set: { _id: 123.456 } }, { a: "a2" } );
   dbcl.update( { $set: { _id: { $decimal: "153.456" } } }, { a: "a3" } );
   dbcl.update( { $set: { _id: "string" } }, { a: "a4" } );
   dbcl.update( { $set: { _id: false } }, { a: "a5" } );
   dbcl.update( { $set: { _id: { "$date": "2012-01-01" } } }, { a: "a6" } );
   dbcl.update( { $set: { _id: { "$timestamp": "2012-01-01-13.14.26.124233" } } }, { a: "a7" } );
   dbcl.update( { $set: { _id: { "key": "value" } } }, { a: "a8" } );
   dbcl.update( { $set: { _id: { "$oid": "123abcd00ef12358902300ef" } } }, { a: "a9" } );
}
