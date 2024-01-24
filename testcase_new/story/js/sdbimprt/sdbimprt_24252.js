/******************************************************************************
 * @Description   : seqDB-24252:带--autodelchar导入带多个字段分隔符的记录
 * @Author        : luweikang
 * @CreateTime    : 2021.05.26
 * @LastEditTime  : 2021.02.03
 ******************************************************************************/

testConf.clName = COMMCLNAME + "cl_24252";
main( test )

function test ( testPara )
{
   var cl = testPara.testCL;
   // 准备导入配置文件
   var importFile = prepareImportFile();
   var docs = expRecords();
   // 导入命令
   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME + " -l " + testConf.clName + " --file " + importFile + " --fields 'str1,str2,str3' --job 1 --parsers 1 --insertnum 1 --autodelchar true";
   // 执行导入
   cmd.run( command );

   // 检查结果
   var cursor = cl.find().sort( { "_id": 1 } );
   commCompareResults( cursor, docs );

   File.remove( importFile );
   cmd.run( "rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec" );
}

function prepareImportFile ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
   var filename = tmpFileDir + "test25252.csv";
   var file = new File( filename );
   file.write( "\"\"abc,1,ccc\n" );
   file.write( "abc\"\",2,ccc\n" );
   file.write( "\"\"abc\"\",3,ccc\n" );
   file.write( "\"\"   abc   ,4,ccc\n" );
   file.write( "   abc   \"\",5,ccc\n" );
   file.write( "\"\"   abc   \"\",6,ccc\n" );
   file.write( "\"\"   abc\"   ,7,ccc\n" );
   file.write( "   abc\"   \"\",8,ccc\n" );
   file.write( "\"\"   abc\"   \"\",9,ccc\n" );
   file.write( "\"\"   \"abc   ,10,ccc\n" );
   file.write( "   \"abc   \"\",11,ccc\n" );
   file.write( "\"\"   \"abc   \"\",12,ccc\n" );
   file.write( "\"\"\"   abc   \",13,ccc\n" );
   file.write( "\"   abc   \"\"\",14,ccc\n" );
   file.write( "\"\"\"   abc   \"\"\",15,ccc\n" );
   file.write( "\"\"\"   abc   ,16,ccc\n" );
   file.write( "\"   abc   \"\",17,ccc\n" );
   file.write( "\"\"\"   abc   \"\",18,ccc\n" );
   file.write( "\"\"   abc   \",19,ccc\n" );
   file.write( "   abc   \"\"\",20,ccc\n" );
   file.write( "\"\"   abc   \"\"\",21,ccc\n" );
   file.write( "\"\"   \"abc\"   ,22,ccc\n" );
   file.write( "   \"abc\"   \"\",23,ccc\n" );
   file.write( "\"\"   \"abc\"   \"\",24,ccc\n" );

   file.write( "25,\"\"   abc   ,ccc\n" );
   file.write( "26,   abc   \"\",ccc\n" );
   file.write( "27,\"\"   abc   \"\",ccc\n" );
   file.write( "28,\"\"   abc\"   ,ccc\n" );
   file.write( "29,   abc\"   \"\",ccc\n" );
   file.write( "30,\"\"   abc\"   \"\",ccc\n" );
   file.write( "31,\"\"   \"abc   ,ccc\n" );
   file.write( "32,   \"abc   \"\",ccc\n" );
   file.write( "33,\"\"   \"abc   \"\",ccc\n" );
   file.write( "34,\"\"\"   abc   \",ccc\n" );
   file.write( "35,\"   abc   \"\"\",ccc\n" );
   file.write( "36,\"\"\"   abc   \"\"\",ccc\n" );
   file.write( "37,\"\"\"   abc   ,ccc\n" );
   file.write( "38,\"   abc   \"\",ccc\n" );
   file.write( "39,\"\"\"   abc   \"\",ccc\n" );
   file.write( "40,\"\"   abc   \",ccc\n" );
   file.write( "41,   abc   \"\"\",ccc\n" );
   file.write( "42,\"\"   abc   \"\"\",ccc\n" );
   file.write( "43,\"\"   \"abc\"   ,ccc\n" );
   file.write( "44,   \"abc\"   \"\",ccc\n" );
   file.write( "45,\"\"   \"abc\"   \"\",ccc\n" );

   file.write( "46,ccc,\"\"   abc   \n" );
   file.write( "47,ccc,   abc   \"\"\n" );
   file.write( "48,ccc,\"\"   abc   \"\"\n" );
   file.write( "49,ccc,\"\"   abc\"   \n" );
   file.write( "50,ccc,   abc\"   \"\"\n" );
   file.write( "51,ccc,\"\"   abc\"   \"\"\n" );
   file.write( "52,ccc,\"\"   \"abc   \n" );
   file.write( "53,ccc,   \"abc   \"\"\n" );
   file.write( "54,ccc,\"\"   \"abc   \"\"\n" );
   file.write( "55,ccc,\"\"\"   abc   \"\n" );
   file.write( "56,ccc,\"   abc   \"\"\"\n" );
   file.write( "57,ccc,\"\"\"   abc   \"\"\"\n" );
   file.write( "58,ccc,\"\"\"   abc   \n" );
   file.write( "59,ccc,\"   abc   \"\"\n" );
   file.write( "60,ccc,\"\"\"   abc   \"\"\n" );
   file.write( "61,ccc,\"\"   abc   \"\n" );
   file.write( "62,ccc,   abc   \"\"\"\n" );
   file.write( "63,ccc,\"\"   abc   \"\"\"\n" );
   file.write( "64,ccc,\"\"   \"abc\"   \n" );
   file.write( "65,ccc,   \"abc\"   \"\"\n" );
   file.write( "66,ccc,\"\"   \"abc\"   \"\"\n" );
   file.close();
   
   return filename;
}

function expRecords ()
{
   var tmpArrs = new Array();
   tmpArrs.push( { str1: "abc\"", str2: 2, str3: "ccc" } );
   tmpArrs.push( { str1: "   abc   \"", str2: 5, str3: "ccc" } );
   tmpArrs.push( { str1: "abc   \",11,ccc" } );
   tmpArrs.push( { str1: "\"   abc   ", str2: 13, str3: "ccc" } );
   tmpArrs.push( { str1: "   abc   \"", str2: 14, str3: "ccc" } );
   tmpArrs.push( { str1: "\"   abc   \"", str2: 15, str3: "ccc" } );
   tmpArrs.push( { str1: "\"   abc   ,16,ccc" } );
   tmpArrs.push( { str1: "   abc   \",17,ccc" } );
   tmpArrs.push( { str1: "\"   abc   \",18,ccc" } );
   tmpArrs.push( { str1: "abc   \"", str2: 20, str3: "ccc" } );

   tmpArrs.push( { str1: 26, str2: "   abc   \"", str3: "ccc" } );
   tmpArrs.push( { str1: 32, str2: "abc   \",ccc" } );
   tmpArrs.push( { str1: 34, str2: "\"   abc   ", str3: "ccc" } );
   tmpArrs.push( { str1: 35, str2: "   abc   \"", str3: "ccc" } );
   tmpArrs.push( { str1: 36, str2: "\"   abc   \"", str3: "ccc" } );
   tmpArrs.push( { str1: 37, str2: "\"   abc   ,ccc" } );
   tmpArrs.push( { str1: 38, str2: "   abc   \",ccc" } );
   tmpArrs.push( { str1: 39, str2: "\"   abc   \",ccc" } );
   tmpArrs.push( { str1: 41, str2: "abc   \"", str3: "ccc" } );

   tmpArrs.push( { str1: 47, str2: "ccc", str3: "   abc   \"" } );
   tmpArrs.push( { str1: 53, str2: "ccc", str3: "abc   \"" } );
   tmpArrs.push( { str1: 55, str2: "ccc", str3: "\"   abc   " } );
   tmpArrs.push( { str1: 56, str2: "ccc", str3: "   abc   \"" } );
   tmpArrs.push( { str1: 57, str2: "ccc", str3: "\"   abc   \"" } );
   tmpArrs.push( { str1: 58, str2: "ccc", str3: "\"   abc   " } );
   tmpArrs.push( { str1: 59, str2: "ccc", str3: "   abc   \"" } );
   tmpArrs.push( { str1: 60, str2: "ccc", str3: "\"   abc   \"" } );
   tmpArrs.push( { str1: 62, str2: "ccc", str3: "abc   \"" } );

   return tmpArrs
}