/* *****************************************************************************
@discretion: index in cl,create index and delete index after rename cl,
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_renamecl16057";
   var newCLName = CHANGEDPREFIX + "_newcl16057";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var recordNums = 2000;
   var indexName1 = "testindex1";
   var indexName2 = "testindex2";
   var indexName3 = "testindex3";
   insertData( dbcl, recordNums );
   dbcl.createIndex( indexName1, { no: 1 } );
   dbcl.createIndex( indexName2, { a: 1 } );

   db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

   var newcl = db.getCS( COMMCSNAME ).getCL( newCLName );
   newcl.dropIndex( indexName1 );
   newcl.createIndex( indexName3, { user: 1 }, true );

   checkRenameCLResult( COMMCSNAME, clName, newCLName );
   checkIndexResult( newcl, indexName3, indexName1 );

   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );

}

function checkIndexResult ( newcl, expIndexName, expDeleteIndexName )
{
   //check index
   var indexName = newcl.find( { user: "test1" } ).explain().current().toObj().IndexName;
   assert.equal( indexName, expIndexName );
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      newcl.getIndex( expDeleteIndexName );
   } );
}