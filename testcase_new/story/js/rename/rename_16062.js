/* *****************************************************************************
@discretion: rename capped cl, than insert datas
@author��2018-10-15 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   var csName = CHANGEDPREFIX + "_renamecs16062";
   var clName = CHANGEDPREFIX + "_renamecl16062";
   var newCLName = CHANGEDPREFIX + "_newcl16062";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var options = { Capped: true };
   var dbcs = commCreateCS( db, csName, false, "create cappedCS", options );
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   commCreateCL( db, csName, clName, optionObj, false, false, "create cappedCL" );

   dbcs.renameCL( clName, newCLName );
   checkRenameCLResult( csName, clName, newCLName );

   var recordNums = 2000;
   var dbcl = dbcs.getCL( newCLName );
   insertData( dbcl, recordNums );
   checkDatas( csName, newCLName, recordNums );

   commDropCS( db, csName, true, "drop CS in the ending" );
}

function checkDatas ( csName, newCLName, expRecordNums )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count( { "$and": [{ a: { "$gte": 0 } }, { a: { "$lt": expRecordNums } }] } );
   assert.equal( count, expRecordNums );
}
