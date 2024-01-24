/************************************
*@Description: seqDB-22773:导出并导入数据，分隔符为空
*@Author:      2020/09/28  liuli
**************************************/
testConf.clName = CHANGEDPREFIX + "_22773";
var csvfile = tmpFileDir + "sdbexprt_22773.csv";

main( test );

function test ( args )
{
   var csvContent = "a,b,c\n1,b_22773\,c_22773\n";
   var docs = [{ "a": 1, "b": "b_22773", "c": "c_22773" }];
   var cl = args.testCL;

   cmd.run( "rm -rf " + csvfile );
   cl.insert( docs );

   // -a '', return success
   testSdbexprt( "-a ''" );
   checkFileContent( csvfile, csvContent );
   cl.remove();
   testSdbimprt( "-a ''" );
   commCompareResults( cl.find(), docs );

   // -e '', return failed
   // cmd.run return 127, actual failure message is 'delfield can't be empty'
   testSdbexprt( "-e ''", 127 );
   testSdbimprt( "-e ''", 127 );

   // -r '', return failed
   // cmd.run return 127, actual failure message is 'delrecord can't be empty'
   testSdbexprt( "-r ''", 127 );
   testSdbimprt( "-r ''", 127 );

   cmd.run( "rm -rf " + csvfile );

}

function testSdbexprt ( str, error )
{
   cmd.run( "rm -rf " + csvfile );
   var commandSdbexprt = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + testConf.csName +
      " -l " + testConf.clName +
      " --file " + csvfile +
      " --type csv" +
      " --fields a,b,c " +
      str;
   testRunCommand( commandSdbexprt, error );
}

function testSdbimprt ( str, error )
{
   var commandSdbimprt = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + testConf.csName +
      " -l " + testConf.clName +
      " --type csv " +
      "--fields a,b,c " +
      "--file " + csvfile +
      " --headerline true " +
      str;
   testRunCommand( commandSdbimprt, error );
}
