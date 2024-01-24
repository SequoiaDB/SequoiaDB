/************************************************************************
*@Description:       导入时指定linepriority 为true
                     1、指定记录分隔符-r包含字符串分隔符-a执行导出再导入，检查结果
                     2、指定记录分隔符-r包含字段分隔符-e执行导出再导入，检查结果
                     3、指定字符串分隔符-a包含字段分隔符-e执行导出再导入，检查结果
                     4、指定字符串分隔符-a包含记录分隔符-r执行导出再导入，检查结果
                     5、指定字段分隔符-e包含记录分隔符-r执行导出再导入，检查结果
                     6、指定字段分隔符-e包含字符串分隔符-a执行导出再导入，检查结果
                   seqDB-18261
*@Author:   2019-4-18  chensiqin
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_18261";
   var doc = [{ c: 1, d: "exprtTest" }, { c: 2, d: "exprtTest2" }];
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "18261.csv";
   var exprtFile = tmpFileDir + "18261.csv";
   exportImportDataRContainA( csName, clName, imprtFile, exprtFile, cl, doc );
   exportImportDataRContainE( csName, clName, imprtFile, exprtFile, cl, doc );
   exportImportDataAContainE( csName, clName, imprtFile, exprtFile, cl, doc );
   exportImportDataAContainA( csName, clName, imprtFile, exprtFile, cl, doc );
   exportImportDataEContainR( csName, clName, imprtFile, exprtFile, cl, doc );
   cleanCL( csName, clName );

}

function readyData ( imprtFile, content )
{

   var file = fileInit( imprtFile );
   file.write( content );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

//1、指定记录分隔符-r包含字符串分隔符-a执行导出再导入，检查结果
function exportImportDataRContainA ( csName, clName, imprtFile, exprtFile, cl, doc )
{
   //导出
   cl.insert( doc );
   var exportOption = installDir + "bin/sdbexprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -r ',Y' -a 'Y' -e 'D' --fields='c ,d'"
      + " --file " + exprtFile;
   try
   {
      cmd.run( exportOption );
      throw new Error( "expected thow exception  actual success" );
   }
   catch( e )
   {
      if( e.message != 127 )
      {
         throw e;
      }
   }
   finally
   {
      cl.truncate();
      cmd.run( "rm -rf " + exprtFile );
   }
   //导入SEQUOIADBMAINSTREAM-4553
   /*
   readyData( imprtFile, "cDd,Y1DYexprtTestY,Y2DYexprtTest2Y,Y" );
   var imprtOption = installDir +"bin/sdbimprt -s "+ COORDHOSTNAME +" -p "+ COORDSVCNAME 
                     +" -c "+ csName +" -l "+ clName 
                     +" --type csv -r ',Y' -a 'Y' -e 'D' --headerline true --fields='c int,d string' --linepriority true"
                     +" --file "+ imprtFile;
   try{
     cmd.run( imprtOption );
     throw new Error( "importData fail,[sdbimprt results]" + 
                        "expected thow exception", 
                        "actual success" );
   }
   catch(e)
   {
      if( e.message != 127)
      {
        throw new Error( "importData fail,[sdbimprt results]" + 
                           "expected thow exception", 
                           "actual success" );
      }
   }*/
}

//2、指定记录分隔符-r包含字段分隔符-e执行导出再导入，检查结果
function exportImportDataRContainE ( csName, clName, imprtFile, exprtFile, cl, doc )
{
   //导出
   cl.insert( doc );
   var exportOption = installDir + "bin/sdbexprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -r ',Y' -e 'Y' --fields='c ,d'"
      + " --file " + exprtFile;
   cmd.run( exportOption );
   cl.truncate();
   //导入
   var imprtOption = installDir + "bin/sdbimprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -r ',Y' -e 'Y' --headerline true --fields='c int,d string' --linepriority true"
      + " --file " + exprtFile;
   testRunCommand( imprtOption );
   checkCLData( cl );
   cl.truncate();
   cmd.run( "rm -rf " + exprtFile );
}

//3、指定字符串分隔符-a包含字段分隔符-e执行导出再导入，检查结果
function exportImportDataAContainE ( csName, clName, imprtFile, exprtFile, cl, doc )
{
   //导出
   cl.insert( doc );
   var exportOption = installDir + "bin/sdbexprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -a ',Y'  -e 'Y' --fields='c ,d'"
      + " --file " + exprtFile;
   cmd.run( exportOption );
   cl.truncate();
   //导入
   var imprtOption = installDir + "bin/sdbimprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -a ',Y'  -e 'Y' --headerline true --fields='c int,d string' --linepriority true"
      + " --file " + exprtFile;
   testRunCommand( imprtOption );
   checkCLData( cl );
   cl.truncate();
   cmd.run( "rm -rf " + exprtFile );
}

//4、指定字符串分隔符-a包含记录分隔符-r执行导出再导入，检查结果
function exportImportDataAContainA ( csName, clName, imprtFile, exprtFile, cl, doc )
{
   //导出
   cl.insert( doc );
   var exportOption = installDir + "bin/sdbexprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -a ',Y' -r 'Y' --fields='c ,d'"
      + " --file " + exprtFile;
   cmd.run( exportOption );
   cl.truncate();
   //导入 SEQUOIADBMAINSTREAM-4552
   /*
   var imprtOption = installDir +"bin/sdbimprt -s "+ COORDHOSTNAME +" -p "+ COORDSVCNAME 
                     +" -c "+ csName +" -l "+ clName 
                     +" --type csv -a ',Y'  -r 'Y' --headerline true --fields='c int,d string' --linepriority true"
                     +" --file "+ exprtFile;
   testRunCommand(imprtOption);
   checkCLData( cl );
   cl.truncate();*/
   cmd.run( "rm -rf " + exprtFile );
}

//5、指定字段分隔符-e包含记录分隔符-r执行导出再导入，检查结果
function exportImportDataEContainR ( csName, clName, imprtFile, exprtFile, cl, doc )
{
   //导出
   cl.insert( doc );
   var exportOption = installDir + "bin/sdbexprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --checkdelimeter false --type csv -e ',Y' -r 'Y' --fields='c ,d'"
      + " --file " + exprtFile;
   cmd.run( exportOption );
   cl.truncate();
   //导入 SEQUOIADBMAINSTREAM-4548
   /*
   var imprtOption = installDir +"bin/sdbimprt -s "+ COORDHOSTNAME +" -p "+ COORDSVCNAME 
                     +" -c "+ csName +" -l "+ clName 
                     +" --type csv -e ',Y'  -r 'Y' --headerline true --fields='c int,d string' --linepriority true"
                     +" --file "+ exprtFile;
   testRunCommand(imprtOption);
   checkCLData( cl );
   cl.truncate();*/
   cmd.run( "rm -rf " + exprtFile );
}

function testRunCommand ( command )
{
   var rc = cmd.run( command );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 2";
   var expImportedRecords = "Imported records: 2";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }
}

//比较cl具体内容
function checkCLData ( cl )
{

   var rc = cl.find();
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 2;
   var actCnt = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + "]" +
         "[cnt:" + actCnt + "]" );
   }

}
