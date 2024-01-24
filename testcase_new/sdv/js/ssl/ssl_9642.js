/***************************************************************************
/***************************************************************************
@Description :seqDB-9642:SSL功能开启，使用sdblobtool工具导入导出lob
@Modify list :
              2016-9-1  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9642a";
var clName1 = CHANGEDPREFIX + "_9642b";

main( dbs );
function main ( dbs )
{
   try
   {
      // create cs /cl
      var dbCL = commCreateCL( dbs, csName, clName, {}, true, true );
      var dbCL = commCreateCL( dbs, csName, clName1, {}, true, false );

      //put lob
      var lobFile = MakeLobfile();
      dbCL.putLob( lobFile );
      //exprt the lob from cl
      var exprtFile = "/tmp/" + CHANGEDPREFIX + "_exprtFile9642";
      toolExportLob( csName, clName, exprtFile );
      checkLobResult( lobFile, exprtFile );

      //import the lob to cl     
      toolImportLob( csName, clName, exprtFile );
      //migrate the lob from cl to cl1
      var testMigratFile = "/tmp/" + CHANGEDPREFIX + "_migratFile9642";
      var testImportFile = "/tmp/" + CHANGEDPREFIX + "_imprtFile9642";
      toolMigrationLob( csName, clName, clName1 );
      //exprt the lob from two cl ,then check the md5     
      toolExportLob( csName, clName1, testMigratFile );
      toolExportLob( csName, clName, testImportFile );
      checkLobResult( testImportFile, testMigratFile );

      //dropCS
      commDropCS( dbs, csName, false, "seqDB-9639: dropCS failed" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      dbs.close();
      cmd.run( "rm -rf " + "/tmp/" + CHANGEDPREFIX + "*" );
      cmd.run( "rm -rf " + "./sdblobtool.log" );
   }
}

/******************************************************************************
*@Description : the function of make lobfile to be a lob  
                创建lobfile文件作为大对象
******************************************************************************/
function MakeLobfile ()
{
   var fileName = CHANGEDPREFIX + "_lobFile9642a.txt";
   var lobfile = "/tmp/" + fileName;
   var file = new File( lobfile );
   var loopNum = 10;
   var content = null;
   for( var i = 0; i < loopNum; ++i )
   {
      content = content + i + "ABCDEFGHIJKLMNOPQRSTUVWXYZ455123";
   }
   file.write( content );
   file.close();

   return lobfile;
}
/******************************************************************************
*@Description : test lob export/import/migration with wrong parameter
*               大对象工具sdblobtool的导出导入迁移操作
******************************************************************************/
function toolExportLob ( csName, clName, exprtFile )
{
   // 执行导出操作
   try
   {
      println( "\n---Begin to export lob" );
      var exprtOption = installDir + 'bin/sdblobtool --operation export --hostname ' + COORDHOSTNAME + ' --svcname ' + COORDSVCNAME
         + ' --collection ' + csName + '.' + clName
         + ' --file ' + exprtFile
         + ' --ssl true';
      println( exprtOption );
      var rc = cmd.run( exprtOption );
   }
   catch( e )
   {
      println( "--fail to export" );
      throw e;
   }
}

function toolImportLob ( csName, clName, importFile )
{
   // 执行导入操作
   try
   {
      println( "\n---Begin to import lob" );
      var imprtOption = installDir + 'bin/sdblobtool --operation import --hostname ' + COORDHOSTNAME + ' --svcname ' + COORDSVCNAME
         + ' --collection ' + csName + '.' + clName
         + ' --file ' + importFile
         + ' --ssl true';
      println( imprtOption );
      var rc = cmd.run( imprtOption );
   }
   catch( e )
   {
      println( "--fail to import" );
      throw e;
   }

}

function toolMigrationLob ( csName, clName, clName1 )
{
   // 执行迁移操作
   try
   {
      println( "\n---Begin to Migration lob" );
      var migratOption = installDir + 'bin/sdblobtool --operation migration --hostname ' + COORDHOSTNAME + ' --svcname ' + COORDSVCNAME
         + ' --dsthost ' + COORDHOSTNAME + ' --dstservice ' + COORDSVCNAME
         + ' --collection ' + csName + '.' + clName
         + ' --dstcollection ' + csName + "." + clName1
         + ' --ssl true';
      println( migratOption );
      var rc = cmd.run( migratOption );
   }
   catch( e )
   {
      println( "---fail to migrate" );
      throw e;
   }
}

