/***************************************************************************
@Description :seqDB-5571:创建Id索引，查询记录
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5571";
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, COMMCSNAME, clName, { AutoIndexId: false, Compressed: true }, true, true );

   // create Idindex
   idxCL.createIdIndex();

   // inspect the index
   inspecIndex( idxCL, "$id", "_id", 1 );

   //after create index, insert data
   var doc = [];
   for( var i = 0; i < 1000; ++i )
   {
      doc.push( { _id: i, a: "test" + i } );
   }
   idxCL.insert( doc );

   //test find by index 
   checkExplain( idxCL, { _id: 8 } );

   //check the result of find  
   var expRecs = '[{"_id":999,"a":"test999"}]';
   var rc = idxCL.find( { _id: { $gt: 998 } } ).sort( { _id: 1 } );
   checkCLData( expRecs, rc );

   //drop idIndex
   idxCL.dropIdIndex();
   // inspect the index
   commCheckIndexConsistency( idxCL, "$id", false );

   // drop collection in clean
   commDropCL( db, COMMCSNAME, clName, false, false );
}
