/* *****************************************************************************
@discretion: rename cl,than insert/update/find/delete datas, putLob/getLob/deleteLob
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   try
   {
      var clName = CHANGEDPREFIX + "_renamecl16068";
      var newCLName = CHANGEDPREFIX + "_newcl16068";
      var fileName = CHANGEDPREFIX + "_lobtest16068.file";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      commCreateCL( db, COMMCSNAME, clName );

      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

      var dbcl = db.getCS( COMMCSNAME ).getCL( newCLName );
      var recordNums = 1000;
      insertData( dbcl, recordNums );
      var srcMd5 = createFile( fileName );
      var lobIdArr = putLobs( dbcl, fileName );

      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      checkDatas( COMMCSNAME, newCLName, recordNums, srcMd5, lobIdArr );

      updateDataAndCheckResult( dbcl, recordNums );
      removeDataAndCheckResult( dbcl, recordNums );
      truncateLobAndCheckResult( dbcl, lobIdArr );

      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
   } finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf *" + fileName );
   }
}

function updateDataAndCheckResult ( dbcl, expRecordNums )
{
   dbcl.update( { $set: { "user": "testupdate" } } );
   var count = dbcl.count( { "user": "testupdate" } );
   assert.equal( count, expRecordNums );
}

function removeDataAndCheckResult ( dbcl, recordNums )
{
   var removeNums = 500;
   dbcl.remove( { a: { $gte: removeNums } } );
   var removeDataNumsInCL = dbcl.count( { a: { $gte: removeNums } } );
   var expRemoveNumsInCL = 0;
   assert.equal( removeDataNumsInCL, expRemoveNumsInCL );

   var count = dbcl.count( { a: { $lt: removeNums } } );
   var expRecordNums = recordNums - removeNums;
   assert.equal( count, expRecordNums );
}

function truncateLobAndCheckResult ( dbcl, expLobArr )
{
   var lobOid = expLobArr[0];
   var lobSize = 100;
   dbcl.truncateLob( lobOid, lobSize );

   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var lobInfo = rc.current().toObj();
      var actSize = lobInfo["Size"];
      var isNormal = lobInfo["Available"];
      assert.equal( actSize, lobSize );
      assert.equal( isNormal, true );
   }
}

function checkDatas ( csName, newCLName, expRecordNums, srcMd5, expLobArr )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );

   //check the lob
   checkLob( dbcl, expLobArr, srcMd5 );
}

function truncateCLAndCheckResult ( dbcl )
{
   dbcl.truncate();

   //check the records nums is 0
   var count = dbcl.count();
   assert.equal( count, 0 );


   //check the lob
   var lobInfos = dbcl.listLobs().next();
   assert.equal( lobInfos, undefined );

}