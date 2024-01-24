/*******************************************************************
* @Description : test export with -s -p -u -w -ssl defalult
*                seqDB-13487:鉴权相关参数配置默认，sdbexprt导出数据
*                seqDB-13526:导出一个集合的一个字段             
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13487";
var doc = { a: 1 };
var csvContent = "a\n1\n";
var jsonContent = "{ \"a\": 1 }\n";

main( test );

function test ()
{
   if( COORDSVCNAME !== "11810" || !isLocal( COORDHOSTNAME ) )
   {
      return;
   }
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );

   testExprtCsv();
   testExprtJson();

   commDropCL( db, csname, clname );
}

function testExprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13487.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13487.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --file " + jsonfile +
      " --fields a";
   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   cmd.run( "rm -rf " + jsonfile );
}