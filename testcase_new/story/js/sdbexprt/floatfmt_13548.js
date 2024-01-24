/*******************************************************************
* @Description : test export with --floatfmt
*                seqDB-13548:double边界值，指定导出格式为%.f
*                seqDB-13549:double边界值，指定导出格式为%.xf
*                seqDB-13550:double边界值，指定导出格式为%.+xf
*                seqDB-13551:double边界值，指定导出格式为%.G
*                seqDB-13552:double边界值，指定导出格式为%.xG
*                seqDB-13553:double边界值，指定导出格式为%.+xG
*                seqDB-13554:double边界值，指定导出格式为%.E
*                seqDB-13555:double边界值，指定导出格式为%.xE
*                seqDB-13556:double边界值，指定导出格式为%.+xE
*                seqDB-13557:double边界值，指定导出格式为db2
*                seqDB-12300:sdbexprt导出csv和json数据，
*                            指定--floatfmt参数               
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var docs = [{ a: -1.7E+308 }, { a: 1.7E+308 }, { a: 12345.67890 }];
var doubleMax = "16999999999999999388307957886599817433334607430407" +
   "587450277311919353772917816056586433009178758470798" +
   "857226246798318891916991610559335717426836996206247" +
   "363529647463651566046493566304068495784430352436781" +
   "502855327271229898638631082864451321235392112325331" +
   "1675499856875650512437415429217994623324794855339589632";
var doubleMin = "-" + doubleMax;
var floatfmts = ["%.f", "%.4f", "%+.4f", "%.G", "%.4G", "%+.4G",
   "%.E", "%.4E", "%+.4E", "db2"];
var csvContents = [
   "a\n" + doubleMin + ".0\n" + doubleMax + ".0\n12346.0\n",
   "a\n" + doubleMin + ".0000\n" + doubleMax + ".0000\n12345.6789\n",
   "a\n" + doubleMin + ".0000\n+" + doubleMax + ".0000\n+12345.6789\n",
   "a\n" + "-2E+308\n" + "2E+308\n" + "1E+04\n",
   "a\n" + "-1.7E+308\n" + "1.7E+308\n" + "1.235E+04\n",
   "a\n" + "-1.7E+308\n" + "+1.7E+308\n" + "+1.235E+04\n",
   "a\n" + "-2E+308\n" + "2E+308\n" + "1E+04\n",
   "a\n" + "-1.7000E+308\n" + "1.7000E+308\n" + "1.2346E+04\n",
   "a\n" + "-1.7000E+308\n" + "+1.7000E+308\n" + "+1.2346E+04\n",
   "a\n" + "-1.70000000000000E+308\n" +
   "+1.70000000000000E+308\n" +
   "+1.23456789000000E+04\n"
];
var jsonContents = [
   "{ \"a\": " + doubleMin + ".0 }\n" + "{ \"a\": " +
   doubleMax + ".0 }\n" + "{ \"a\": 12346.0 }\n",
   "{ \"a\": " + doubleMin + ".0000 }\n" + "{ \"a\": " +
   doubleMax + ".0000 }\n" + "{ \"a\": 12345.6789 }\n",
   "{ \"a\": " + doubleMin + ".0000 }\n" + "{ \"a\": +" +
   doubleMax + ".0000 }\n" + "{ \"a\": +12345.6789 }\n",
   "{ \"a\": -2E+308 }\n" +
   "{ \"a\": 2E+308 }\n" + "{ \"a\": 1E+04 }\n",
   "{ \"a\": -1.7E+308 }\n" +
   "{ \"a\": 1.7E+308 }\n" + "{ \"a\": 1.235E+04 }\n",
   "{ \"a\": -1.7E+308 }\n" +
   "{ \"a\": +1.7E+308 }\n" + "{ \"a\": +1.235E+04 }\n",
   "{ \"a\": -2E+308 }\n" +
   "{ \"a\": 2E+308 }\n" + "{ \"a\": 1E+04 }\n",
   "{ \"a\": -1.7000E+308 }\n" +
   "{ \"a\": 1.7000E+308 }\n" + "{ \"a\": 1.2346E+04 }\n",
   "{ \"a\": -1.7000E+308 }\n" +
   "{ \"a\": +1.7000E+308 }\n" + "{ \"a\": +1.2346E+04 }\n",
   "{ \"a\": -1.70000000000000E+308 }\n" +
   "{ \"a\": +1.70000000000000E+308 }\n" + "{ \"a\": +1.23456789000000E+04 }\n"
];

main( test );

function test ()
{
   for( var i = 0; i < floatfmts.length; i++ )
   {
      testFloatFmtCsv( floatfmts[i], csvContents[i] );
      testFloatFmtJson( floatfmts[i], jsonContents[i] );
   }
}

function testFloatFmtCsv ( floatfmt, content )
{
   var clname = COMMCLNAME + "_sdbexprt13548";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13548.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --floatfmt " + floatfmt +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
}

function testFloatFmtJson ( floatfmt, content )
{
   var clname = COMMCLNAME + "_sdbexprt13548";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   var jsonfile = tmpFileDir + "sdbexprt13548.json";
   cmd.run( "rm -rf " + jsonfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " --floatfmt " + floatfmt +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( jsonfile, content );

   cmd.run( "rm -rf " + jsonfile );
   commDropCL( db, csname, clname );
}