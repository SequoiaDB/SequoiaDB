/***************************************************************************
@Description :seqDB-11984 :记录中字段长度最大(略<16M)，创建全文索引 
@Modify list :
              2018-10-25  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11984";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var a = new Array( 1024 * 1024 * 16 - 5 * 1024 ).join( "a" );
   dbcl.insert( { a: a } );
   dbcl.createIndex( "a_11984", { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, "a_11984", 1 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { a: "" } );
   var expResult = dbOperator.findFromCL( dbcl, null, { a: "" } );

   if( expResult.length != 1 )
   {
      throw new Error( "expect record num: 1,actual record num: " + expResult.length );
   }
   if( expResult[0]["a"] != actResult[0]["a"] )
   {
      throw new Error( "expect record: " + expResult[0]["a"] + ",actual record: " + actResult[0]["a"] );
   }

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "a_11984" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
