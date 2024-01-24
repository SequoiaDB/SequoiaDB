/***************************************************************************
@Description : seqDB-5580:指定Id索引查询，使用sort、limit和skip
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5580";
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, COMMCSNAME, clName, { AutoIndexId: false, Compressed: true }, true, true );

   // create Idindex
   idxCL.createIdIndex( null );

   // inspect the index
   inspecIndex( idxCL, "$id", "_id", 1 );

   //after create index, insert data
   var doc = [];
   for( var i = 0; i < 100; ++i )
   {
      doc.push( { _id: i, a: "test" + i } );
   }
   idxCL.insert( doc );

   //test find by index 
   checkExplain( idxCL, { _id: 19 } );

   //check the result of find  
   var expRecs = '[{"_id":96,"a":"test96"}]';
   var rc = idxCL.find( { _id: { $lt: 100 } } ).hint( { "": "$id" } ).sort( { _id: -1 } ).limit( 1 ).skip( 3 );
   checkCLData( expRecs, rc );

   //drop idIndex
   idxCL.dropIdIndex();
   // inspect the index
   commCheckIndexConsistency( idxCL, "$id", false );

   // drop collection in clean
   commDropCL( db, COMMCSNAME, clName, false, false );
}
