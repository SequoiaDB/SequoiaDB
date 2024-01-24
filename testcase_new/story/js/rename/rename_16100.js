/************************************
*@Description: 修改cs名后，执行LOB读写删查操作
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16100
**************************************/

main( test );

function test ()
{
   var oldcsName = COMMCSNAME + "_16100_old";
   var newcsName = COMMCSNAME + "_16100_new";
   var clName = CHANGEDPREFIX + "_16100_cl";
   var fileName = CHANGEDPREFIX + "_16100lob";
   var lobNum = 10;

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var cl = commCreateCL( db, oldcsName, clName, {}, false, false, "create CL in the begin" );

   var lobMD5 = createFile( fileName );

   var lobIdArr = putLobs( cl, fileName, lobNum, 5 );

   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );
   cl = db.getCS( newcsName ).getCL( clName );

   checkLob( cl, lobIdArr, lobMD5 );

   deleteLobs( cl, lobIdArr );

   var lobMD5new = createFile( fileName + "_new" );

   var lobArrnew = putLobs( cl, fileName + "_new", lobNum, 5 );

   checkLob( cl, lobArrnew, lobMD5new );

   commDropCS( db, newcsName, true, "clean cs---" );
   deleteFile( fileName );
   deleteFile( fileName + "_new" );
}