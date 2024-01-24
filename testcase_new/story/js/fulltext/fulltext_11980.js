/***************************************************************************
@Description :seqDB-11980 :在空集合中创建/删除全文索引 
@Modify list :
              2018-11-21  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11980";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //集合中无数据，创建全文索引
   commCreateIndex( dbcl, "fullIndex_11980", { b: "text" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_11980", 0 );

   var dbOperator = new DBOperator();
   var cappedCL = dbOperator.getCappedCLs( COMMCSNAME, clName, "fullIndex_11980" );
   var cappedCL = cappedCL[0];
   var count = cappedCL.count();
   if( count != 0 )
   {
      throw new Error( "expect record num: 0" + ",actual record num: " + count );
   }

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_11980" );
   dropIndex( dbcl, "fullIndex_11980" );
   commCheckIndexConsistency( dbcl, "fullIndex_11980", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   dropCL( db, COMMCSNAME, clName, true, true );
}
