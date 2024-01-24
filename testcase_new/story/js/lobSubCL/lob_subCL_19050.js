/************************************
*@Description: seqDB-19050 rename主表集合名
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

   var mainCSName = COMMCSNAME;
   var oldMainCLName = "mainCL_19050old";
   var newMainCLName = "mainCL_19050new";
   var subCSName = "subCS_19050";
   var subCLName = "subCL_19050";
   var filePath = WORKDIR + "/lob19050/";
   var fileName = "file19050"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, mainCSName, oldMainCLName );
   commDropCS( db, subCSName );

   var mainCL = createMainCLAndAttachCL( db, mainCSName, oldMainCLName, subCLName, "YYYYMMDD", 1, 20190801, 5 );
   commCreateCL( db, subCSName, subCLName );
   mainCL.attachCL( subCSName + "." + subCLName, { "LowBound": { "date": "20190806" }, "UpBound": { "date": "20190811" } } );
   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 2, "20190801" );

   db.getCS( mainCSName ).renameCL( oldMainCLName, newMainCLName );
   db.getCS( mainCSName ).renameCL( newMainCLName, oldMainCLName );
   db.getCS( mainCSName ).renameCL( oldMainCLName, newMainCLName );
   var mainCL = db.getCS( mainCSName ).getCL( newMainCLName );

   var lobOids2 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 2, "20190801" );
   checkLobMD5( mainCL, lobOids1, fileMD5 );
   checkLobMD5( mainCL, lobOids2, fileMD5 );
   deleteLob( mainCL, lobOids1 );

   commDropCL( db, mainCSName, newMainCLName );
   commDropCS( db, subCSName );
   deleteTmpFile( filePath );
}
