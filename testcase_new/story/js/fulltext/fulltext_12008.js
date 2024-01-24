/******************************************************************************
@Description :    seqDB-12008:指定_id插入/更新记录
@Modify list :   2018-9-25  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_12008";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_12008";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( { _id: 1, a: "a1" } );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   dbcl.update( { $set: { _id: "_id" + 1 } }, { _id: 1 } );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   dbcl.insert( { _id: 1, a: "a1" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   dbcl.update( { $set: { _id: "_id" + 1 } }, { _id: 1 } );
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
