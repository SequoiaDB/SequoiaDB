/* *****************************************************************************
@discretion: rename cl,index in cl
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_renamecl16056";
   var newCLName = CHANGEDPREFIX + "_newcl16056";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var recordNums = 2000;
   var indexName = "testindex";
   insertData( dbcl, recordNums );
   dbcl.createIndex( indexName, { no: 1 } );

   db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

   checkRenameCLResult( COMMCSNAME, clName, newCLName );
   checkFindResult( COMMCSNAME, newCLName, indexName, recordNums );

   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
}

function checkFindResult ( csName, newCLName, expIndexName, expRecordNums )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );

   //check index
   var indexName = dbcl.find( { no: 1 } ).explain().current().toObj().IndexName;
   assert.equal( indexName, expIndexName );
}