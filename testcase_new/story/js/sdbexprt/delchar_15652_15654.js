/***********************************************************************
* @Description : test export with -a random ascii
*                seqDB-15652:自定义任意16进制、10进制ascii码为字符串分隔符，导出到json文件
*                seqDB-15654:自定义任意16进制、10进制ascii码为字符串分隔符，导出到csv文件         
* @author      : wangkexin
* 
************************************************************************/

main( test );

function test ()
{
   var clname = COMMCLNAME + "_sdbexprt15652";
   var clname1 = COMMCLNAME + "_sdbimprt15652";
   var doc = { a: 1, b: "exprtTest" };

   var cl = commCreateCL( db, COMMCSNAME, clname );
   var cl1 = commCreateCL( db, COMMCSNAME, clname1 );
   cl.insert( doc );

   //JSON文件
   //16进制与字母混合多字符分隔符
   testExprtDelcharJson( "abc0x2A0x3f", clname );
   checkResultByImprtJson( "abc*?", clname1, cl1 );
   testExprtDelcharJson( "0x2Aabc0x3f", clname );
   checkResultByImprtJson( "'*abc?'", clname1, cl1 );
   //10进制ascii码和字母混合分隔符
   testExprtDelcharJson( "\\\\107a", clname );
   checkResultByImprtJson( "'ka'", clname1, cl1 )
   //10进制ascii码和数字混合分隔符
   testExprtDelcharJson( "\\\\1071", clname );
   checkResultByImprtJson( "'k1'", clname1, cl1 );
   //16进制分隔符
   testExprtDelcharJson( "0x2A", clname );
   checkResultByImprtJson( "'*'", clname1, cl1 );
   //不能识别的分隔符
   testExprtDelcharJson( "0xx", clname );
   checkResultByImprtJson( "'0xx'", clname1, cl1 );
   //汉字分隔符
   testExprtDelcharJson( "分隔符", clname );
   checkResultByImprtJson( "'分隔符'", clname1, cl1 );

   //CSV文件
   //16进制与字母混合多字符分隔符
   testExprtDelcharCsv( "abc0x2A0x3f", clname );
   checkResultByImprtCsv( "abc*?", clname1, cl1 );
   testExprtDelcharCsv( "0x2Aabc0x3f", clname );
   checkResultByImprtCsv( "'*abc?'", clname1, cl1 );
   //10进制ascii码和字母混合分隔符
   testExprtDelcharCsv( "\\\\107a", clname );
   checkResultByImprtCsv( "'ka'", clname1, cl1 );
   //10进制ascii码和数字混合分隔符
   testExprtDelcharCsv( "\\\\1071", clname );
   checkResultByImprtCsv( "'k1'", clname1, cl1 );
   //16进制分隔符
   testExprtDelcharCsv( "0x2A", clname );
   checkResultByImprtCsv( "'*'", clname1, cl1 );
   //不能识别的分隔符
   testExprtDelcharCsv( "0xx", clname );
   checkResultByImprtCsv( "'0xx'", clname1, cl1 );
   //汉字分隔符
   testExprtDelcharCsv( "分隔符", clname );
   checkResultByImprtCsv( "'分隔符'", clname1, cl1 );


   commDropCL( db, COMMCSNAME, clname );
   commDropCL( db, COMMCSNAME, clname1 );
}

function testExprtDelcharJson ( asc, clname )
{
   var jsonfile = tmpFileDir + "sdbexprt15652.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " -a " + asc +
      " --fields a,b";
   testRunCommand( command );
}

function testExprtDelcharCsv ( asc, clname )
{
   var csvfile = tmpFileDir + "sdbexprt15652.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " -a " + asc +
      " --fields a,b";
   testRunCommand( command );
}

//通过导入cl后的数据与原数据进行对比验证
function checkResultByImprtJson ( asc1, clname1, cl1 )
{
   var jsonfile = tmpFileDir + "sdbexprt15652.json";
   //导入json格式文件到cl1上
   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clname1 +
      " --file " + jsonfile +
      " --type json" +
      " -a " + asc1 +
      " --headerline true " +
      " --fields='a int,b string'";
   testRunCommand( command );
   cmd.run( "rm -rf " + jsonfile );

   //检查cl1上数据是否与原数据相符
   var expRecs = ["{\"a\":1,\"b\":\"exprtTest\"}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cl1.truncate();
}

function checkResultByImprtCsv ( asc1, clname1, cl1 )
{
   var csvfile = tmpFileDir + "sdbexprt15652.csv";
   //导入csv格式文件到cl1上
   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv" +
      " -a " + asc1 +
      " --headerline true " +
      " --fields='a int,b string'";
   testRunCommand( command );
   cmd.run( "rm -rf " + csvfile );

   //检查cl1上数据是否与原数据相符
   var expRecs = ["{\"a\":1,\"b\":\"exprtTest\"}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cl1.truncate();
}