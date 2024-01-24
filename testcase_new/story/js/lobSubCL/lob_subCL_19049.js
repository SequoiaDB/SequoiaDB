/************************************
*@Description: seqDB-19049 rename主表集合空间名
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

   var oldMainCSName = "mainCS_19049old";
   var newMainCSName = "mainCS_19049new";
   var mainCLName = "mainCL_19049";
   var subCSName = COMMCSNAME;
   var subCLName = "subCL_19049";
   var filePath = WORKDIR + "/lob19049/";
   var fileName = "file19049"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCS( db, oldMainCSName );
   commDropCS( db, newMainCSName );
   commDropCL( db, subCSName, subCLName );

   var mainCL = createMainCLAndAttachCL( db, oldMainCSName, mainCLName, subCLName, "YYYYMMDD", 1, 20190801, 5 );
   commCreateCL( db, subCSName, subCLName );
   mainCL.attachCL( subCSName + "." + subCLName, { "LowBound": { "date": "20190806" }, "UpBound": { "date": "20190811" } } );
   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 2, "20190801" );

   db.renameCS( oldMainCSName, newMainCSName );
   db.renameCS( newMainCSName, oldMainCSName );
   db.renameCS( oldMainCSName, newMainCSName );
   var mainCL = db.getCS( newMainCSName ).getCL( mainCLName );

   var lobOids2 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 2, "20190801" );
   checkLobMD5( mainCL, lobOids1, fileMD5 );
   checkLobMD5( mainCL, lobOids2, fileMD5 );
   deleteLob( mainCL, lobOids1 );

   deleteTmpFile( filePath );
   commDropCS( db, oldMainCSName );
   commDropCS( db, newMainCSName );
   commDropCL( db, subCSName, subCLName );
}
