/* *****************************************************************************
@discretion: rename capped cs, than create cappedcl and delete cappedcl
@author��2018-10-16 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{

   var csName = CHANGEDPREFIX + "_renamecs16114";
   var newCSName = CHANGEDPREFIX + "_newcs16114";
   var cLName = CHANGEDPREFIX + "_renamecl16114";
   commDropCS( db, csName, true, "drop CS in the beginning" );
   commDropCS( db, newCSName, true, "drop CS in the beginning" );

   var options = { Capped: true };
   var dbcs = commCreateCS( db, csName, false, "create cappedCS", options );
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, cLName, optionObj, false, false, "create cappedCL" );

   var recordNums = 2000;
   insertData( dbcl, recordNums );
   db.renameCS( csName, newCSName );
   checkRenameCSResult( csName, newCSName, 1 );
   checkDatas( newCSName, cLName, recordNums );

   var dbnewCL = db.getCS( newCSName ).getCL( cLName );
   checkDatas( newCSName, cLName, recordNums );
   dropCappedCLAndCheckResult( newCSName, cLName );
   createCappedCLAndCheckResult( newCSName, cLName );

   commDropCS( db, newCSName, true, "drop CS in the ending" );

}

function dropCappedCLAndCheckResult ( cappedCSName, cappedCLName )
{
   var dbcs = db.getCS( cappedCSName );
   dbcs.dropCL( cappedCLName );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcs.getCL( cappedCLName );
   } );
}

function createCappedCLAndCheckResult ( cappedCSName, cappedCLName )
{

   //create cappedcl,the cl name is the same as the deleted name    
   var dbcs = db.getCS( cappedCSName );
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, cappedCSName, cappedCLName, optionObj, false, false, "create cappedCL" );

   var count = dbcl.count();
   assert.equal( count, 0 );
}


function checkDatas ( csName, newCLName, expRecordNums )
{

   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count( { "$and": [{ a: { "$gte": 0 } }, { a: { "$lt": expRecordNums } }] } );
   assert.equal( count, expRecordNums );
}
