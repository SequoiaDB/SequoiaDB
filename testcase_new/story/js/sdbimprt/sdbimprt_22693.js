/************************************************************************
*@Description:  检查base64解码并发执行会串改。原因：中间变量用了静态变量，会导致多线程并发不安全 
*@Author:   2020-08-25 zhaohailin
************************************************************************/
var recsNum = 10000;
testConf.csName = COMMCLNAME;
testConf.clName = COMMCLNAME + "_22693";

main( test );

function test ( args )
{

   var cl = args.testCL;
   var imprtFile = tmpFileDir + "testdata_22693.csv";
   readyData( imprtFile );
   importData( testConf.csName, testConf.clName, imprtFile );
   checkCLData( cl );
   cleanCL( testConf.csName, testConf.clName );

}
function readyData ( imprtFile )
{
   var file = fileInit( imprtFile );
   for( var i = 1; i <= recsNum; i++ )
   {
      file.write( i + ",binaryStr," + "\"aGVsbG8gd29ybGQ=\",\"aTE=\"\n" );
   }
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --file ' + imprtFile
      + ' --type csv --fields "num int,type string,v1 binary,v2 binary"'
      + ' --jobs 15 --parsers 15';
   var rc = cmd.run( imprtOption );
}

function checkCLData ( cl )
{
   var actNum = cl.count( { "v1": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "0" }, "v2": { "$binary": "aTE=", "$type": "0" } } );
   if( actNum != recsNum )
   {
      throw new Error( "check error! expCount :" + recsNum + " , actCount :" + actNum );
   }
}
