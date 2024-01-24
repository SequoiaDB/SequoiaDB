/************************************
*@Description: seqDB-22222:主子表上ListLobs指定Oid查询
*@author:      luweikang
*@createDate:  2020.5.26
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName = COMMCSNAME;
   var mainCLName = "cl22222_main";
   var subCLName = "cl22222_sub";
   var filePath = WORKDIR + "/lob22222/";
   deleteTmpFile( filePath );
   var fileName = "file22222";
   var fileFullPath = filePath + fileName;
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var mainCL = createMainCLAndAttachCL( db, csName, mainCLName, subCLName, "YYYYMM", 2, "202001", 2 );
   var subCL2 = db.getCS( csName ).getCL( subCLName + "_1" );
   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMM", 2, 10, 1, "20200101" );
   for( var i = 0; i < lobOids.length; i++ )
   {
      subCL2.putLob( fileFullPath, lobOids[i] );
   }
   checkLobMD5( mainCL, lobOids, fileMD5 );
   checkLobMD5( subCL2, lobOids, fileMD5 );

   for( i in lobOids )
   {
      var actLobs = [];
      var lobsCur = mainCL.listLobs( SdbQueryOption().cond( { Oid: { $oid: lobOids[i] } } ) );
      while( lobsCur.next() )
      {
         actLobs.push( lobsCur.current() );
      }
      if( actLobs.length != 1 )
      {
         throw new Error( "exp list 1 lob, but found: " + actLobs.length + " lob, list: " + actLobs.toString() );
      }
      lobsCur.close();
   }

   var actLobs1 = [];
   var lobsCur1 = mainCL.listLobs();
   while( lobsCur1.next() )
   {
      actLobs1.push( lobsCur1.current() );
   }
   if( actLobs1.length != 20 )
   {
      throw new Error( "exp list 20 lob, but found: " + actLobs1.length + " lob, list: " + actLobs1.toString() );
   }
   lobsCur1.close();

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );
   deleteTmpFile( filePath );
}
