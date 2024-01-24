/************************************
*@Description: seqDB-19044 构造LobId并插入lob
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
   var clName = "cl_19044";
   var filePath = WORKDIR + "/lob19044/";
   var fileName = "file19044"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, clName );

   var cl = commCreateCL( db, csName, clName );
   var lobIDs = [];

   var lobID1 = cl.createLobID( "2019-08-08-10.08.56.124159" );
   cl.putLob( fileFullPath, lobID1 );
   lobIDs.push( lobID1 );

   var lobID2 = cl.createLobID( "2018-10-04-20.12.20.005454" );
   cl.putLob( fileFullPath, lobID2 );
   lobIDs.push( lobID2 );

   var lobID3 = cl.createLobID( "2019-1-16-08.16.02.000000" );
   cl.putLob( fileFullPath, lobID3 );
   lobIDs.push( lobID3 );

   checkLobMD5( cl, lobIDs, fileMD5 );
   deleteLob( cl, lobIDs );

   deleteTmpFile( filePath );
   commDropCL( db, csName, clName );
}
