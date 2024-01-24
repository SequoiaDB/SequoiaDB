/************************************
*@Description:  seqDB-19025 创建主表，指定不同格式LobShardingKeyFirmat
seqDB-19026 删除主表集合，主表已挂载子表
seqDB-19041 主子表创建、读取、删除lob
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
   var mainCLName = "cl19025_main";
   var subCLName = "cl19025_sub";
   var filePath = WORKDIR + "/lob19025/";
   var fileName = "file19025";
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   //测试创建LobShardingKeyFormat为YYYYMMDD主表
   var mainCL = createMainCLAndAttachCL( db, csName, mainCLName, subCLName, "YYYYMMDD" );
   var nameArr = mainCL.toString().split( "." );
   var mainCLFullName = nameArr[1] + "." + nameArr[2];
   var lobOid1s = insertLob( mainCL, fileFullPath, "YYYYMMDD" );
   checkLobMD5( mainCL, lobOid1s, fileMD5 );
   checkSubCLLob( db, mainCLFullName, lobOid1s );
   checkDeleteLob( mainCL, lobOid1s );
   //TODO：1、19026的用例测试点，需要直接指定创建的子表检测子表删除结果
   cleanMainCL( db, csName, mainCLName );

   //测试创建LobShardingKeyFormat为YYYYMM主表
   createMainCLAndAttachCL( db, csName, mainCLName, subCLName, "YYYYMM" );
   var lobOid2s = insertLob( mainCL, fileFullPath, "YYYYMM" );
   checkLobMD5( mainCL, lobOid2s, fileMD5 );
   checkSubCLLob( db, mainCLFullName, lobOid2s );
   checkDeleteLob( mainCL, lobOid2s );
   cleanMainCL( db, csName, mainCLName );

   //测试创建LobShardingKeyFormat为YYYY主表
   createMainCLAndAttachCL( db, csName, mainCLName, subCLName, "YYYY" );
   var lobOid3s = insertLob( mainCL, fileFullPath, "YYYY" );
   checkLobMD5( mainCL, lobOid3s, fileMD5 );
   checkSubCLLob( db, mainCLFullName, lobOid3s );
   checkDeleteLob( mainCL, lobOid3s );
   cleanMainCL( db, csName, mainCLName );

   deleteTmpFile( filePath );
}

function checkDeleteLob ( mainCL, lobOids )
{
   for( i in lobOids )
   {
      mainCL.deleteLob( lobOids[i] );
      assert.tryThrow( SDB_FNE, function()
      {
         mainCL.getLob( lobOids[i], WORKDIR + "/checkLob19038_" + i );
      } );
   }
}
