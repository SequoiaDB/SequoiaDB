/***************************************************************************
@Description :seqDB-14375 :ES中有提交记录时，不更新记录 
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

   var clName = COMMCLNAME + "_ES_14375";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   commCreateIndex( dbcl, "fullIndex_14375", { a: "text", b: "text", c: "text" } );

   var records = new Array();
   for( var i = 0; i < 8000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i, c: "c" + i };
      records.push( record );
   }
   dbcl.insert( records );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14375", 8000 );

   var dbOperator = new DBOperator();
   var esOperator = new ESOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_14375" );
   var esIndexName = esIndexNames[0];
   var actResult = esOperator.findFromES( esIndexName, '{"query" : {"match_all" : {}}, "size" : 10000}' )
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { a: "", b: "", c: "" } );

   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
