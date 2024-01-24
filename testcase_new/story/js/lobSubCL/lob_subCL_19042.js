/************************************
*@Description: seqDB-19042 主子表进行truncate
*@author:      luweikang
*@createDate:  2019.8.12
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName = COMMCSNAME;
   var mainCLName = "cl19042_main";
   var subCLName = "cl19042_sub";
   var filePath = WORKDIR + "/lob19042/";
   var fileName = "file19042";
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var mainCL = createMainCLAndAttachCL( db, csName, mainCLName, subCLName, "YYYYMMDD", 4 );
   var nameArr = mainCL.toString().split( "." );
   var mainCLFullName = nameArr[1] + "." + nameArr[2];
   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 4 );
   checkLobMD5( mainCL, lobOids, fileMD5 );

   mainCL.truncate();

   for( i in lobOids )
   {
      assert.tryThrow( SDB_FNE, function()
      {
         mainCL.getLob( lobOids[i], WORKDIR + "/checkLob19042_" + i );
      } );
   }

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );
   deleteTmpFile( filePath );
}
