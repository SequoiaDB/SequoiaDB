/* *****************************************************************************
@discretion: rename cl
             seqDB-16073
@author��2018-10-15 chensiqin  Init
***************************************************************************** */
main( test );
function test ()
{
   var csName = CHANGEDPREFIX + "_cs16073";
   var fileName = CHANGEDPREFIX + "_lobtest16073.file";
   var clName = CHANGEDPREFIX + "_cl16073_5";

   commDropCS( db, csName, true, "drop CS " + csName );
   var cs = commCreateCS( db, csName, true, "create CS1" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in the beginning" );
   try
   {
      var recordNums = 100;
      insertData( varCL, recordNums );
      var srcMd5 = createFile( fileName );
      var lobIdArr = putLobs( varCL, fileName );

      for( var i = 1; i <= 10; i++ )
      {
         cs.renameCL( clName, CHANGEDPREFIX + "_newcl16073_" + i );
         checkRenameCLResult( csName, clName, CHANGEDPREFIX + "_newcl16073_" + i )
         clName = CHANGEDPREFIX + "_newcl16073_" + i;
      }

      checkDatas( csName, clName, recordNums, srcMd5, lobIdArr );
      commDropCS( db, csName, true, "ignoreNotExist is true" );
   } finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf *" + fileName );
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
