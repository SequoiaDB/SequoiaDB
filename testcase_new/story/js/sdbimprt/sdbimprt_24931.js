/******************************************************************************
 * @Description   : seqDB-24931:参数中出现大量hosts时，能正确对每个hostname进行比较和排序
 * @Author        : 钟子明
 * @CreateTime    : 2022.01.12
 * @LastEditTime  : 2022.04.20
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24931";

main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   // 准备导入配置文件
   var importFile = prepareImportFile();
   // hostlist1，是用特定的主机+端口顺序来诱发旧的sdbimprt的排序问题，未修复该问题的版本会出现core dump
   var hostlist1 = "A-SSJY-SDB01:11810,A-SSJY-SDB02:11810,A-SSJY-SDB03:11810," +
      "A-SSJY-SDB04:11810,A-SSJY-SDB05:11810,A-SSJY-SDB06:11810,A-SSJY-SDB07:11810," +
      "A-SSJY-SDB08:11810,A-SSJY-SDB09:11810,A-SSJY-SDB10:11810,A-SSJY-SDB11:11810," +
      "A-SSJY-SDB12:11810,A-SSJY-SDB13:11810,A-SSJY-SDB14:11810,A-SSJY-SDB15:11810," +
      "A-SSJY-SDB16:11810,A-SSJY-SDB17:11810,A-SSJY-SDB18:11810,A-SSJY-SDB19:11810," +
      "A-SSJY-SDB20:11810,A-SSJY-SDB21:11810,A-SSJY-MTAPP01:21810,"
   importAndCheck( cl, importFile, hostlist1 );
   // hostlist2是hostlist1手动打乱顺序得到的，仍然可以诱发core dump
   var hostlist2 = "A-SSJY-SDB06:11810,A-SSJY-SDB07:11810,A-SSJY-SDB01:11810," +
      "A-SSJY-SDB02:11810,A-SSJY-SDB03:11810,A-SSJY-SDB04:11810,A-SSJY-SDB05:11810," +
      "A-SSJY-SDB09:11810,A-SSJY-SDB10:11810,A-SSJY-SDB11:11810,A-SSJY-SDB12:11810," +
      "A-SSJY-SDB13:11810,A-SSJY-SDB14:11810,A-SSJY-SDB15:11810,A-SSJY-SDB16:11810," +
      "A-SSJY-SDB17:11810,A-SSJY-SDB18:11810,A-SSJY-SDB19:11810,A-SSJY-SDB20:11810," +
      "A-SSJY-SDB08:11810,A-SSJY-SDB21:11810,A-SSJY-MTAPP01:21810,"
   importAndCheck( cl, importFile, hostlist2 );

   cmd.run( "rm -f " + importFile );
}

function prepareImportFile ()
{
   var filename = tmpFileDir + "test24391.csv";
   cmd.run( "rm -f " + filename );
   var file = new File( filename );
   file.write( "abc,1   ,ccc\n" );
   file.close();

   return filename;
}

function importAndCheck ( cl, importFile, hosts )
{
   var command = installDir + "bin/sdbimprt " + " --hosts=" + hosts + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME + " -l " + testConf.clName + " --file " + importFile + " --fields 'str1,str2,str3' ";
   // 执行导入
   cmd.run( command );

   // 检查结果
   var cursor = cl.find().sort( { "_id": 1 } );
   commCompareResults( cursor, [{ str1: "abc", str2: 1, str3: "ccc" }] );

   // 清理cl，准备下次导入
   cl.truncate();
}