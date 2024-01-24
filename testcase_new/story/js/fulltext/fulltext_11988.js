/***************************************************************************
@Description : seqDB-11988 :hash切分表加入域使用自动切分，创建/删除全文索引 
@Modify list : 2018-11-02  YinZhen  Create
****************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11988";
   dropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { "ShardingType": "hash", "ShardingKey": { "a": 1 }, "AutoSplit": true } );

   var records = new Array();
   for( var i = 0; i < 100; i++ )
   {
      var record = { a: "a_" + i, b: "b_" + i };
      records.push( record );
   }
   cl.insert( records );

   //创建全文索引
   commCreateIndex( cl, "fullTextIndex_11988", { "a": "text", "b": "text" } );
   commCheckIndexConsistency( cl, "fullTextIndex_11988", true );
   checkFullSyncToES( COMMCSNAME, clName, "fullTextIndex_11988", 100 );

   var dbOperator = new DBOperator();
   var cappedCL = dbOperator.getCappedCLs( COMMCSNAME, clName, "fullTextIndex_11988" );
   var cappedCL = cappedCL[0];
   var count = cappedCL.count();
   if( count != 0 )
   {
      throw new Error( "expect record num:0, actual record num: " + count );
   }

   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   //删除全文索引
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullTextIndex_11988" );
   dropIndex( cl, "fullTextIndex_11988" );
   commCheckIndexConsistency( cl, "fullTextIndex_11988", false );
   checkIndexNotExistInES( esIndexNames );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   dropCL( db, COMMCSNAME, clName, true, true );
}
